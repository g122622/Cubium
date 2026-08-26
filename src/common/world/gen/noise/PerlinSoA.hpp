/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"

#include <array>
#include <cmath>
#include <cstddef>

namespace mc::world::gen::noise {

// ============================================================================
// Perlin SoA 采样内核（效仿 C2ME c2me-opts-natives-math 的 ext_math.h）
//
// 把 NormalNoise / BlendedNoise 内部多个 PerlinNoise 的非零 octave 子层在构造期
// 拍平成连续 SoA 数组（排列表 + 偏移 + 振幅 + 缩放因子），运行期单循环遍历求值，
// 消除逐层虚分发与 PerlinLayer 对象的 cache miss，并让 octave 循环可向量化。
//
// 采样内核 perlinSample 与 Cubium PerlinLayer::noise/noiseWithSmear 数值等价：
//   先 wrap(coord*inputFactor) 再加 originOffset，8 角梯度点乘 + smoothstep + 三线性插值。
// 梯度表用扁平 FLAT_SIMPLEX_GRAD[64]（16 梯度 × 4 槽，hash<<2 索引）替代两级查表，
// 内容与 PerlinLayer::GRADIENTS 逐项一致。
//
// Y 涂抹语义：yScale!=0 时把 Y 分数吸附到 yScale 间隔网格线（用于 BlendedNoise 条纹）；
//             yScale==0 即不涂抹（NormalNoise 路径）。yMax 是原始 Y 分数，控制吸附基准。
// ============================================================================

/// 扁平梯度表：16 个梯度 × 4 个 double 槽（前 3 分量有效，第 4 槽填充对齐）。
/// 索引方式：hash & 0xF 得 0..15，(hash<<2) 定位梯度首槽。内容与 PerlinLayer::GRADIENTS
/// 逐项一致（含末 4 项的重复梯度布局，非前 12 项简单重复）。
alignas(64) inline constexpr std::array<f64, 64> kFlatSimplexGrad = {
    1.0,
    1.0,
    0.0,
    0.0,
    -1.0,
    1.0,
    0.0,
    0.0,
    1.0,
    -1.0,
    0.0,
    0.0,
    -1.0,
    -1.0,
    0.0,
    0.0,
    1.0,
    0.0,
    1.0,
    0.0,
    -1.0,
    0.0,
    1.0,
    0.0,
    1.0,
    0.0,
    -1.0,
    0.0,
    -1.0,
    0.0,
    -1.0,
    0.0,
    0.0,
    1.0,
    1.0,
    0.0,
    0.0,
    -1.0,
    1.0,
    0.0,
    0.0,
    1.0,
    -1.0,
    0.0,
    0.0,
    -1.0,
    -1.0,
    0.0,
    1.0,
    1.0,
    0.0,
    0.0,
    0.0,
    -1.0,
    1.0,
    0.0,
    -1.0,
    1.0,
    0.0,
    0.0,
    0.0,
    -1.0,
    -1.0,
    0.0,
};

/// smoothstep 衰减因子：t*t*t*(t*(t*6-15)+10)。
[[nodiscard]] inline f64 perlinFade(f64 value)
{
    return value * value * value * (value * (value * 6.0 - 15.0) + 10.0);
}

/// 线性插值：start + delta*(end-start)。
[[nodiscard]] inline f64 perlinLerp(f64 delta, f64 start, f64 end)
{
    return start + delta * (end - start);
}

/// 三线性插值（8 角点值 → 单值）。
[[nodiscard]] inline f64 perlinLerp3(
    f64 dx, f64 dy, f64 dz, f64 v000, f64 v100, f64 v010, f64 v110, f64 v001, f64 v101, f64 v011, f64 v111)
{
    const f64 y0 = perlinLerp(dx, v000, v100);
    const f64 y1 = perlinLerp(dx, v010, v110);
    const f64 y2 = perlinLerp(dx, v001, v101);
    const f64 y3 = perlinLerp(dx, v011, v111);
    const f64 z0 = perlinLerp(dy, y0, y1);
    const f64 z1 = perlinLerp(dy, y2, y3);
    return perlinLerp(dz, z0, z1);
}

/// 钳位插值：delta<0→start，delta>1→end，否则 lerp。语义同 MC clampedLerp。
[[nodiscard]] inline f64 perlinClampedLerp(f64 start, f64 end, f64 delta)
{
    if (delta < 0.0) {
        return start;
    }
    return delta > 1.0 ? end : perlinLerp(delta, start, end);
}

/// 坐标环绕，防止大坐标精度丢失（2^25 周期）。等价 PerlinNoise::wrap。
[[nodiscard]] inline f64 perlinWrap(f64 value)
{
    constexpr f64 WRAP_PERIOD = 33554432.0;
    return value - std::floor(value / WRAP_PERIOD + 0.5) * WRAP_PERIOD;
}

/**
 * @brief SoA 单点 Perlin 采样内核（含 Y 涂抹）
 *
 * 数值等价 PerlinLayer::noiseWithSmear（yScale!=0）与 PerlinLayer::noise（yScale==0）。
 * 排列表 perm 为 256 项 u8（已洗牌），查表用 & 0xFF 折回（等价 PerlinLayer::m_p 双倍表）。
 *
 * @param perm 256 项排列表（u8）
 * @param originX/originY/originZ 构造期随机偏移（PerlinLayer::m_xOffset 等）
 * @param x/y/z 已 wrap 的采样坐标（调用前需 wrap(coord*inputFactor)）
 * @param yScale Y 涂抹间隔（0 = 不涂抹）
 * @param yMax 原始 Y 分数，控制吸附基准（NormalNoise 路径传 0）
 */
[[nodiscard]] inline f64 perlinSample(
    const u8* perm, f64 originX, f64 originY, f64 originZ, f64 x, f64 y, f64 z, f64 yScale, f64 yMax)
{
    const f64 d = x + originX;
    const f64 e = y + originY;
    const f64 f = z + originZ;
    const i32 cellX = math::floorTo<i32>(d);
    const i32 cellY = math::floorTo<i32>(e);
    const i32 cellZ = math::floorTo<i32>(f);
    const f64 fracX = d - static_cast<f64>(cellX);
    const f64 fracY = e - static_cast<f64>(cellY);
    const f64 fracZ = f - static_cast<f64>(cellZ);

    // Y 涂抹：yScale!=0 时把 fracY 吸附到 yScale 间隔网格线（用 yMax 或 fracY 作基准）。
    // smoothstep 用原始 fracY，梯度点乘用吸附后的 (fracY - smearOffset)。
    // epsilon 用 1.0e-7f 提升 double，精确匹配 PerlinLayer::noiseWithSmear 的涂抹量化（bit-exact）。
    f64 smearOffset = 0.0;
    if (yScale != 0.0) {
        const f64 base = (yMax >= 0.0 && yMax < fracY) ? yMax : fracY;
        smearOffset = std::floor(base / yScale + static_cast<f64>(1.0e-7f)) * yScale;
    }
    const f64 gradY = fracY - smearOffset;

    // 8 角梯度点乘：hash 链 perm[perm[perm[x]+y)&0xFF]+z)&0xFF] & 0xF，梯度取 kFlatSimplexGrad[hash<<2]。
    auto gradDot = [perm](i32 px, i32 py, i32 pz, f64 gx, f64 gy, f64 gz) -> f64 {
        const u32 ax = static_cast<u32>(px) & 0xFFu;
        const u32 ay = static_cast<u32>(py) & 0xFFu;
        const u32 az = static_cast<u32>(pz) & 0xFFu;
        const u32 hash = perm[(perm[(perm[ax] + ay) & 0xFFu] + az) & 0xFFu] & 0x0Fu;
        const f64* grad = &kFlatSimplexGrad[hash << 2u];
        return grad[0] * gx + grad[1] * gy + grad[2] * gz;
    };

    const f64 v000 = gradDot(cellX, cellY, cellZ, fracX, gradY, fracZ);
    const f64 v100 = gradDot(cellX + 1, cellY, cellZ, fracX - 1.0, gradY, fracZ);
    const f64 v010 = gradDot(cellX, cellY + 1, cellZ, fracX, gradY - 1.0, fracZ);
    const f64 v110 = gradDot(cellX + 1, cellY + 1, cellZ, fracX - 1.0, gradY - 1.0, fracZ);
    const f64 v001 = gradDot(cellX, cellY, cellZ + 1, fracX, gradY, fracZ - 1.0);
    const f64 v101 = gradDot(cellX + 1, cellY, cellZ + 1, fracX - 1.0, gradY, fracZ - 1.0);
    const f64 v011 = gradDot(cellX, cellY + 1, cellZ + 1, fracX, gradY - 1.0, fracZ - 1.0);
    const f64 v111 = gradDot(cellX + 1, cellY + 1, cellZ + 1, fracX - 1.0, gradY - 1.0, fracZ - 1.0);

    const f64 dx = perlinFade(fracX);
    const f64 dy = perlinFade(fracY);
    const f64 dz = perlinFade(fracZ);
    return perlinLerp3(dx, dy, dz, v000, v100, v010, v110, v001, v101, v011, v111);
}

/**
 * @brief 拍平的单个 Perlin octave 子层数据（SoA 数组的一个元素）
 *
 * NormalNoise 把 first+second 的非零 octave 各收集成一个 PerlinSoALayer；
 * BlendedNoise 把 minLimit/maxLimit/main 三层的非零 octave 各自收集。
 * 运行期单循环遍历 PerlinSoALayer 数组求值，数据连续利于 cache 与向量化。
 */
struct PerlinSoALayer {
    std::array<u8, 256> permutation; ///< 洗牌后的排列表（拷贝自 PerlinLayer::m_permutation）
    f64 originX = 0.0;               ///< 构造期随机偏移 X
    f64 originY = 0.0;               ///< 构造期随机偏移 Y
    f64 originZ = 0.0;               ///< 构造期随机偏移 Z
    f64 amplitude = 0.0;             ///< 该 octave 振幅（PerlinNoise::m_amplitudes[i]）
    f64 inputFactor = 0.0;           ///< 该 octave 输入缩放（lowestFreqInputFactor * 2^i）
    f64 valueFactor = 0.0;           ///< 该 octave 值缩放（lowestFreqValueFactor / 2^i）
};

} // namespace mc::world::gen::noise
