/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
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

#include "../../core/Constants.hpp"
#include "../../core/Types.hpp"
#include "MathConstants.hpp"
#include "common/world/WorldConstants.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace mc::math {

// ============================================================================
// 度数/弧度转换常量
// ============================================================================

/// 度转弧度乘数
constexpr f32 DEG_TO_RAD = PI / 180.0f;

/// 弧度转度乘数
constexpr f32 RAD_TO_DEG = 180.0f / PI;

// ============================================================================
// 基本数学函数
// ============================================================================

/**
 * @brief 度转弧度
 */
[[nodiscard]] inline constexpr f32 toRadians(f32 degrees) noexcept
{
    return degrees * DEG_TO_RAD;
}

/**
 * @brief 弧度转度
 */
[[nodiscard]] inline constexpr f32 toDegrees(f32 radians) noexcept
{
    return radians * RAD_TO_DEG;
}

/**
 * @brief 数值限制在范围内
 */
template <typename T>
[[nodiscard]] inline constexpr T clamp(T value, T minVal, T maxVal) noexcept
{
    return std::clamp(value, minVal, maxVal);
}

/**
 * @brief 线性插值 (f32 版本)
 */
[[nodiscard]] inline constexpr f32 lerp(f32 a, f32 b, f32 t) noexcept
{
    return a + (b - a) * t;
}

/**
 * @brief 线性插值 (f64 版本)
 */
[[nodiscard]] inline constexpr f64 lerp(f64 a, f64 b, f64 t) noexcept
{
    return a + (b - a) * t;
}

/**
 * @brief 三线性插值 (Trilinear Interpolation)
 *
 * 沿三个轴向依次进行线性插值，用于 3D 噪声生成等场景。
 *
 * 插值顺序：X 轴 → Z 轴 → Y 轴
 *
 * 8 个角点的编号约定（参考 Perlin 噪声）：
 * - v0: (0, 0, 0)  v1: (1, 0, 0)  沿 X 轴：t1
 * - v2: (0, 0, 1)  v3: (1, 0, 1)  沿 Z 轴：t2
 * - v4: (0, 1, 0)  v5: (1, 1, 0)  沿 Y 轴：t3
 * - v6: (0, 1, 1)  v7: (1, 1, 1)
 *
 * @param t1 X 轴插值因子 [0, 1]
 * @param t2 Z 轴插值因子 [0, 1]
 * @param t3 Y 轴插值因子 [0, 1]
 * @param v0 角点 (0, 0, 0) 的值
 * @param v1 角点 (1, 0, 0) 的值
 * @param v2 角点 (0, 0, 1) 的值
 * @param v3 角点 (1, 0, 1) 的值
 * @param v4 角点 (0, 1, 0) 的值
 * @param v5 角点 (1, 1, 0) 的值
 * @param v6 角点 (0, 1, 1) 的值
 * @param v7 角点 (1, 1, 1) 的值
 * @return 插值结果
 *
 * 三线性插值，插值顺序为 X → Y → Z，与 MC 的 Mth.lerp3 一致。
 *
 * @param sx X 方向插值因子（smoothstep 后的值）
 * @param sy Y 方向插值因子
 * @param sz Z 方向插值因子
 * @param d0 角点 (0,0,0) 的值
 * @param d1 角点 (1,0,0) 的值
 * @param d2 角点 (0,1,0) 的值
 * @param d3 角点 (1,1,0) 的值
 * @param d4 角点 (0,0,1) 的值
 * @param d5 角点 (1,0,1) 的值
 * @param d6 角点 (0,1,1) 的值
 * @param d7 角点 (1,1,1) 的值
 * @return 插值结果
 */
[[nodiscard]] inline f32 lerp3(
    f32 sx, f32 sy, f32 sz, f32 d0, f32 d1, f32 d2, f32 d3, f32 d4, f32 d5, f32 d6, f32 d7) noexcept
{
    // X 轴插值
    const f32 x0 = lerp(d0, d1, sx); // (0,0,0)→(1,0,0)
    const f32 x1 = lerp(d2, d3, sx); // (0,1,0)→(1,1,0)
    const f32 x2 = lerp(d4, d5, sx); // (0,0,1)→(1,0,1)
    const f32 x3 = lerp(d6, d7, sx); // (0,1,1)→(1,1,1)

    // Y 轴插值
    const f32 y0 = lerp(x0, x1, sy); // z=0 平面
    const f32 y1 = lerp(x2, x3, sy); // z=1 平面

    // Z 轴插值
    return lerp(y0, y1, sz);
}

/**
 * @brief f64 版本的三线性插值，用于 NoiseInterpolator.fillingCell 模式
 *
 * MC 的 Mth.lerp3 使用 double 参数。
 * 参数顺序: d0=(0,0,0), d1=(1,0,0), d2=(0,1,0), d3=(1,1,0),
 *           d4=(0,0,1), d5=(1,0,1), d6=(0,1,1), d7=(1,1,1)
 */
[[nodiscard]] inline f64 lerp3(
    f64 sx, f64 sy, f64 sz, f64 d0, f64 d1, f64 d2, f64 d3, f64 d4, f64 d5, f64 d6, f64 d7) noexcept
{
    // X 轴插值
    const f64 x0 = lerp(d0, d1, sx);
    const f64 x1 = lerp(d2, d3, sx);
    const f64 x2 = lerp(d4, d5, sx);
    const f64 x3 = lerp(d6, d7, sx);

    // Y 轴插值
    const f64 y0 = lerp(x0, x1, sy);
    const f64 y1 = lerp(x2, x3, sy);

    // Z 轴插值
    return lerp(y0, y1, sz);
}

/**
 * @brief 线性插值，但将插值因子限制在 [0, 1] 范围内
 * @param a 起始值
 * @param b 目标值
 * @param t 插值因子（会被 clamp 到 [0, 1]）
 * @return 插值结果
 *
 * 参考 MC 1.16.5: MathHelper.clampedLerp
 */
[[nodiscard]] inline f32 clampedLerp(f32 a, f32 b, f32 t) noexcept
{
    return lerp(a, b, clamp(t, 0.0f, 1.0f));
}

/**
 * @brief 线性插值，将输入值从输入范围映射到输出范围
 * @param outputMin 输出最小值
 * @param outputMax 输出最大值
 * @param inputMin 输入最小值
 * @param inputMax 输入最大值
 * @param inputValue 输入值
 * @return 插值结果
 *
 * 参考 MC 1.16.5: LinearPosTest 使用的映射插值
 */
[[nodiscard]] inline f32 mappedLerp(f32 outputMin, f32 outputMax, f32 inputMin, f32 inputMax, f32 inputValue) noexcept
{
    if (inputMax <= inputMin) {
        return outputMin;
    }
    f32 t = (inputValue - inputMin) / (inputMax - inputMin);
    return lerp(outputMin, outputMax, clamp(t, 0.0f, 1.0f));
}

/**
 * @brief 平滑插值 (smoothstep)
 */
[[nodiscard]] inline constexpr f32 smoothstep(f32 edge0, f32 edge1, f32 x) noexcept
{
    const f32 t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

/**
 * @brief 平方
 */
template <typename T>
[[nodiscard]] inline constexpr T square(T x) noexcept
{
    return x * x;
}

/**
 * @brief 线性映射，将值从 [fromMin, fromMax] 映射到 [toMin, toMax]
 * 与 mappedLerp 不同，此函数不 clamp 输入值
 *
 * 参考 MC 1.21: Mth.map
 */
[[nodiscard]] inline f64 map(f64 value, f64 fromMin, f64 fromMax, f64 toMin, f64 toMax) noexcept
{
    return toMin + (value - fromMin) * (toMax - toMin) / (fromMax - fromMin);
}

[[nodiscard]] inline f32 map(f32 value, f32 fromMin, f32 fromMax, f32 toMin, f32 toMax) noexcept
{
    return toMin + (value - fromMin) * (toMax - toMin) / (fromMax - fromMin);
}

/**
 * @brief 钳位线性映射，将值从 [fromMin, fromMax] 映射到 [toMin, toMax] 并 clamp
 *
 * 参考 MC 1.21: Mth.clampedMap
 */
[[nodiscard]] inline f64 clampedMap(f64 value, f64 fromMin, f64 fromMax, f64 toMin, f64 toMax) noexcept
{
    f64 t = (value - fromMin) / (fromMax - fromMin);
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return toMin + t * (toMax - toMin);
}

/**
 * @brief 立方
 */
template <typename T>
[[nodiscard]] inline constexpr T cube(T x) noexcept
{
    return x * x * x;
}

/**
 * @brief 判断是否接近零
 */
[[nodiscard]] inline bool isZero(f32 x, f32 epsilon = EPSILON) noexcept
{
    return std::abs(x) < epsilon;
}

/**
 * @brief 判断两个浮点数是否近似相等
 */
[[nodiscard]] inline bool approxEqual(f32 a, f32 b, f32 epsilon = EPSILON) noexcept
{
    return std::abs(a - b) < epsilon;
}

/**
 * @brief 快速逆平方根
 *
 * 计算 $1 / \sqrt{x}$ 的值。
 *
 * 历史上使用著名的"卡马克快速逆平方根"算法（0x5f3759df 魔数），
 * 但现代 CPU 有硬件指令（rsqrtss）和优化的 sqrt 实现，编译器
 * 会自动生成高效的代码，因此直接使用 `1.0f / std::sqrt(value)` 即可。
 *
 * @param value 正数输入值
 * @return $1 / \sqrt{value}$
 */
[[nodiscard]] inline f32 fastInverseSqrt(f32 value) noexcept
{
    return 1.0f / std::sqrt(value);
}

/**
 * @brief 向上取整到整数
 */
template <typename T>
[[nodiscard]] inline T ceilTo(f32 x) noexcept
{
    return static_cast<T>(std::ceil(x));
}

/**
 * @brief 向上取整到整数 (f64 版本)
 */
template <typename T>
[[nodiscard]] inline T ceilTo(f64 x) noexcept
{
    return static_cast<T>(std::ceil(x));
}

/**
 * @brief 向下取整到整数
 */
template <typename T>
[[nodiscard]] inline T floorTo(f32 x) noexcept
{
    return static_cast<T>(std::floor(x));
}

/**
 * @brief 向下取整到整数 (f64 版本)
 */
template <typename T>
[[nodiscard]] inline T floorTo(f64 x) noexcept
{
    return static_cast<T>(std::floor(x));
}

/**
 * @brief 四舍五入到整数
 */
template <typename T>
[[nodiscard]] inline T roundTo(f32 x) noexcept
{
    return static_cast<T>(std::round(x));
}

/**
 * @brief 四舍五入到整数 (f64 版本)
 */
template <typename T>
[[nodiscard]] inline T roundTo(f64 x) noexcept
{
    return static_cast<T>(std::round(x));
}

// ============================================================================
// 区块相关计算
// ============================================================================

/**
 * @brief 世界坐标转区块坐标
 */
[[nodiscard]] inline ChunkCoord toChunkCoord(f32 worldCoord) noexcept
{
    return static_cast<ChunkCoord>(std::floor(worldCoord / static_cast<f32>(world::CHUNK_WIDTH)));
}

/**
 * @brief 世界坐标转区块坐标（整数版本）
 */
[[nodiscard]] inline constexpr ChunkCoord toChunkCoord(BlockCoord worldCoord) noexcept
{
    return worldCoord >= 0 ? worldCoord / world::CHUNK_WIDTH
                           : (worldCoord - world::CHUNK_WIDTH + 1) / world::CHUNK_WIDTH;
}

/**
 * @brief 区块坐标转世界坐标（区块原点）
 */
[[nodiscard]] inline constexpr BlockCoord toWorldCoord(ChunkCoord chunkCoord) noexcept
{
    return chunkCoord * world::CHUNK_WIDTH;
}

/**
 * @brief 世界坐标转区块内坐标
 */
[[nodiscard]] inline BlockCoord toLocalCoord(f32 worldCoord) noexcept
{
    return static_cast<BlockCoord>(std::floor(worldCoord)) & world::CHUNK_MASK;
}

/**
 * @brief 世界坐标转区块内坐标（整数版本）
 */
[[nodiscard]] inline constexpr BlockCoord toLocalCoord(BlockCoord worldCoord) noexcept
{
    return worldCoord & world::CHUNK_MASK;
}

/**
 * @brief 区块坐标转换为64位唯一ID
 */
[[nodiscard]] inline constexpr u64 chunkPosToId(ChunkCoord x, ChunkCoord z) noexcept
{
    const u64 ux = static_cast<u64>(static_cast<i64>(x) & 0xFFFFFFFFLL);
    const u64 uz = static_cast<u64>(static_cast<i64>(z) & 0xFFFFFFFFLL);
    return (ux << 32) | uz;
}

/**
 * @brief 从64位ID解包区块坐标
 */
inline void idToChunkPos(u64 id, ChunkCoord& x, ChunkCoord& z) noexcept
{
    x = static_cast<ChunkCoord>(static_cast<i32>((id >> 32) & 0xFFFFFFFF));
    z = static_cast<ChunkCoord>(static_cast<i32>(id & 0xFFFFFFFF));
}

// ============================================================================
// 角度处理函数
// ============================================================================

/**
 * @brief 将角度规范化到 [-180, 180) 范围
 *
 * @param degrees 输入角度（度）
 * @return 规范化后的角度
 */
[[nodiscard]] inline f32 wrapDegrees(f32 degrees) noexcept
{
    degrees = std::fmod(degrees, 360.0f);
    if (degrees >= 180.0f) {
        degrees -= 360.0f;
    } else if (degrees < -180.0f) {
        degrees += 360.0f;
    }
    return degrees;
}

/**
 * @brief 将角度规范化到 [0, 360) 范围
 *
 * @param degrees 输入角度（度）
 * @return 规范化后的角度
 */
[[nodiscard]] inline f32 wrapDegreesPositive(f32 degrees) noexcept
{
    degrees = std::fmod(degrees, 360.0f);
    if (degrees < 0.0f) {
        degrees += 360.0f;
    }
    return degrees;
}

/**
 * @brief 限制角度变化量
 *
 * 计算从 sourceAngle 到 targetAngle 的最短路径，
 * 并限制最大变化量为 maximumChange。
 *
 * @param sourceAngle 起始角度（度）
 * @param targetAngle 目标角度（度）
 * @param maximumChange 最大变化量（度）
 * @return 调整后的角度
 */
[[nodiscard]] inline f32 clampAngle(f32 sourceAngle, f32 targetAngle, f32 maximumChange) noexcept
{
    f32 diff = wrapDegrees(targetAngle - sourceAngle);
    if (diff > maximumChange) {
        diff = maximumChange;
    } else if (diff < -maximumChange) {
        diff = -maximumChange;
    }
    return wrapDegreesPositive(sourceAngle + diff);
}

/**
 * @brief MC 1.16.5 clampedRotate - 限制角度旋转速度
 *
 * 与 clampAngle 不同，此函数不包装结果到 [0, 360)，
 * 而是直接返回 sourceAngle + clamped(diff)。
 * 这是 MC 1.16.5 中 LookController 和 MovementController 使用的方法。
 *
 * @param from 当前角度（度）
 * @param to 目标角度（度）
 * @param maxDelta 最大角度变化（度）
 * @return 限制后的角度
 */
[[nodiscard]] inline f32 clampedRotate(f32 from, f32 to, f32 maxDelta) noexcept
{
    f32 diff = wrapDegrees(to - from);
    f32 clamped = std::clamp(diff, -maxDelta, maxDelta);
    return from + clamped;
}

/**
 * @brief MC 1.16.5 approachTargetAngle (MathHelper.func_219800_b)
 *
 * 将角度从当前值向目标值接近，限制最大变化量。
 * 与 clampedRotate 不同，此函数的结果更接近目标值。
 *
 * 参考 MC 1.16.5: float approachTargetAngle(float from, float to, float max)
 * 用于 LookController 在有导航路径时限制头部角度。
 *
 * 注意：MC 原版不包装结果到 [0, 360)，结果可能为负值或大于 360。
 *
 * @param sourceAngle 当前角度（度）
 * @param targetAngle 目标角度（度）
 * @param maximumChange 最大角度变化（度）
 * @return 调整后的角度（不包装到 [0, 360)）
 */
[[nodiscard]] inline f32 approachTargetAngle(f32 sourceAngle, f32 targetAngle, f32 maximumChange) noexcept
{
    f32 diff = wrapDegrees(targetAngle - sourceAngle);
    if (diff > maximumChange) {
        diff = maximumChange;
    } else if (diff < -maximumChange) {
        diff = -maximumChange;
    }
    // MC 1.16.5: 不包装结果，直接返回
    return targetAngle - diff;
}

/**
 * @brief 计算两点之间的水平距离平方
 *
 * @param x1 第一个点的X坐标
 * @param z1 第一个点的Z坐标
 * @param x2 第二个点的X坐标
 * @param z2 第二个点的Z坐标
 * @return 水平距离的平方
 */
[[nodiscard]] inline f32 distanceHorizontalSq(f32 x1, f32 z1, f32 x2, f32 z2) noexcept
{
    const f32 dx = x2 - x1;
    const f32 dz = z2 - z1;
    return dx * dx + dz * dz;
}

/**
 * @brief 计算两点之间的距离平方
 *
 * @param x1 第一个点的X坐标
 * @param y1 第一个点的Y坐标
 * @param z1 第一个点的Z坐标
 * @param x2 第二个点的X坐标
 * @param y2 第二个点的Y坐标
 * @param z2 第二个点的Z坐标
 * @return 距离的平方
 */
[[nodiscard]] inline f32 distanceSq(f32 x1, f32 y1, f32 z1, f32 x2, f32 y2, f32 z2) noexcept
{
    const f32 dx = x2 - x1;
    const f32 dy = y2 - y1;
    const f32 dz = z2 - z1;
    return dx * dx + dy * dy + dz * dz;
}

/**
 * @brief 地板除运算 (MC MathHelper.floorDiv)
 *
 * 与 C++ 的 / 运算符不同，结果总是向负无穷方向取整。
 * 例如: floorDiv(-1, 40) = -1 (而 -1 / 40 = 0 在 C++ 中向零取整)
 *
 * 参考 MC 1.16.5: MathHelper.floorDiv(int, int)
 *
 * @param value 被除数
 * @param divisor 除数（必须非零）
 * @return 地板除结果
 */
[[nodiscard]] inline constexpr i32 floorDiv(i32 value, i32 divisor) noexcept
{
    // MC 1.16.5: Math.floorDiv(int, int) 实现
    // 向负无穷方向取整
    i32 q = value / divisor;
    i32 r = value % divisor;
    return (r != 0 && ((r > 0) != (divisor > 0))) ? (q - 1) : q;
}

/**
 * @brief 地板除运算 (64位版本)
 *
 * @param value 被除数
 * @param divisor 除数（必须非零）
 * @return 地板除结果
 */
[[nodiscard]] inline constexpr i64 floorDiv(i64 value, i64 divisor) noexcept
{
    i64 q = value / divisor;
    i64 r = value % divisor;
    return (r != 0 && ((r > 0) != (divisor > 0))) ? (q - 1) : q;
}

/**
 * @brief 地板模运算 (MC MathHelper.floorMod)
 *
 * 与 C++ 的 % 运算符不同，结果总是与除数同号。
 * 例如: floorMod(-1, 40) = 39 (而 -1 % 40 = -1)
 *
 * 参考 MC 1.16.5: MathHelper.floorMod(int, int)
 *
 * @param value 被除数
 * @param divisor 除数（必须为正数）
 * @return 地板模结果 [0, divisor)
 */
[[nodiscard]] inline i64 floorMod(i64 value, i64 divisor) noexcept
{
    // MC 1.16.5: Math.floorMod(int, int) 实现
    // 结果与 divisor 同号
    const i64 result = value % divisor;
    return (result < 0) ? (result + divisor) : result;
}

/**
 * @brief 地板模运算 (32位版本)
 *
 * @param value 被除数
 * @param divisor 除数（必须为正数）
 * @return 地板模结果 [0, divisor)
 */
[[nodiscard]] inline i32 floorMod(i32 value, i32 divisor) noexcept
{
    const i32 result = value % divisor;
    return (result < 0) ? (result + divisor) : result;
}

/**
 * @brief 地板模运算 (浮点版本)
 *
 * 参考 MC 1.16.5: MathHelper.func_226168_f_(double, double)
 * 用于计算周期性动画时间
 *
 * @param value 被除数
 * @param divisor 除数（必须为正数）
 * @return 地板模结果 [0, divisor)
 */
[[nodiscard]] inline f32 floorMod(f32 value, f32 divisor) noexcept
{
    f32 result = std::fmod(value, divisor);
    return (result < 0.0f) ? (result + divisor) : result;
}

/**
 * @brief 角度插值（处理角度环绕）
 *
 * 参考 MC 1.16.5 ModelUtils.func_228283_a_
 * 用于平滑过渡两个角度值，考虑角度环绕问题。
 *
 * @param current 当前角度（弧度）
 * @param target 目标角度（弧度）
 * @param factor 插值因子（0-1，值越大越接近目标）
 * @return 插值后的角度
 */
[[nodiscard]] inline f32 lerpAngleRadians(f32 current, f32 target, f32 factor) noexcept
{
    f32 diff = target - current;
    // 规范化到 [-PI, PI)
    while (diff < -PI) {
        diff += TWO_PI;
    }
    while (diff >= PI) {
        diff -= TWO_PI;
    }
    return current + factor * diff;
}

// ============================================================================
// 区块/方块位置哈希
// ============================================================================

/**
 * @brief 计算方块位置的确定性哈希值（用于结构完整度等随机数种子）
 *
 * 参考 MC 1.16.5: MathHelper.getPositionRandom / getCoordinateRandom
 * 用于位置相关的随机数生成，确保同一位置的方块在相同种子下行为一致
 *
 * 算法: i = (x * 3129871) XOR (z * 116129781) XOR y
 *       i = i * i * 42317861 + i * 11
 *       return i >> 16
 *
 * @param x X坐标
 * @param y Y坐标
 * @param z Z坐标
 * @return 64位哈希值，可用作随机数种子
 */
[[nodiscard]] inline u64 hashBlockPos(i32 x, i32 y, i32 z) noexcept
{
    // MC 1.16.5: MathHelper.getCoordinateRandom
    // 注意：MC 使用 (x * 3129871) XOR (z * 116129781) XOR y
    // 但这里的 XOR 操作顺序很重要
    i64 i = static_cast<i64>(x * 3129871) ^ (static_cast<i64>(z) * 116129781LL) ^ static_cast<i64>(y);
    i = i * i * 42317861LL + i * 11LL;
    return static_cast<u64>(i >> 16);
}

/**
 * @brief 计算方块位置的确定性随机种子（MC 1.16.5 兼容）
 *
 * 参考 MC 1.16.5: MathHelper.getPositionRandom
 * 用于结构完整度、规则处理器等需要基于位置确定性随机的场景
 *
 * @param x X坐标
 * @param y Y坐标
 * @param z Z坐标
 * @return 可用于 Random 构造函数的种子值
 */
[[nodiscard]] inline u64 getPositionRandom(i32 x, i32 y, i32 z) noexcept
{
    return hashBlockPos(x, y, z);
}

/**
 * @brief 计算方块位置的确定性随机种子（仅 XZ 坐标）
 *
 * 参考 MC 1.16.5: 用于某些不需要 Y 坐标的场景
 *
 * @param x X坐标
 * @param z Z坐标
 * @return 可用于 Random 构造函数的种子值
 */
[[nodiscard]] inline u64 getPositionRandomXZ(i32 x, i32 z) noexcept
{
    i64 i = static_cast<i64>(x * 3129871) ^ (static_cast<i64>(z) * 116129781LL);
    i = i * i * 42317861LL + i * 11LL;
    return static_cast<u64>(i >> 16);
}

// ============================================================================
// 帧率无关的衰减/插值函数
// ============================================================================

/**
 * @brief 计算指数衰减的帧率无关纠正因子
 *
 * 使用指数衰减公式计算纠正因子，使纠正速度与帧率无关。
 * 无论帧率高低，每秒的纠正总量保持一致。
 *
 * 数学公式: correctionFactor = 1 - (1 - ratePerSecond)^deltaTime
 *
 * 例如，ratePerSecond = 0.5 表示每秒纠正约 50% 的差值：
 * - 60 FPS 时: 每帧纠正因子 ≈ 0.0115
 * - 30 FPS 时: 每帧纠正因子 ≈ 0.0228
 * - 无论帧率如何，1秒内总纠正量都约为 50%
 *
 * @param ratePerSecond 每秒的衰减/纠正速率 [0, 1]
 * @param deltaTime 帧间隔时间（秒）
 * @return 该帧的纠正因子 [0, 1]
 *
 * @note 当 deltaTime 为 0 时返回 0
 * @note 当 ratePerSecond 为 0 时返回 0
 * @note 当 ratePerSecond 为 1 时返回 1（立即纠正）
 */
[[nodiscard]] inline f32 exponentialDecayFactor(f32 ratePerSecond, f32 deltaTime) noexcept
{
    // 边界情况处理
    if (deltaTime <= 0.0f || ratePerSecond <= 0.0f) {
        return 0.0f;
    }
    if (ratePerSecond >= 1.0f) {
        return 1.0f;
    }
    // 公式: 1 - (1 - r)^dt
    // 确保结果在 [0, 1] 范围内（浮点精度保护）
    return std::clamp(1.0f - std::pow(1.0f - ratePerSecond, deltaTime), 0.0f, 1.0f);
}

} // namespace mc::math
