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

#include "ImprovedNoiseGenerator.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include <algorithm>
#include <cmath>

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

ImprovedNoiseGenerator::ImprovedNoiseGenerator(u64 seed)
{
    math::Random rng(seed);
    _initPermutation(rng);
}

ImprovedNoiseGenerator::ImprovedNoiseGenerator(math::IRandom& rng)
{
    _initPermutation(rng);
}

// ============================================================================
// 初始化
// ============================================================================

void ImprovedNoiseGenerator::_initPermutation(math::IRandom& rng)
{
    // 初始化排列数组
    for (i32 i = 0; i < 256; ++i) {
        m_permutation[static_cast<size_t>(i)] = static_cast<u8>(i);
    }

    // Fisher-Yates 洗牌
    for (i32 i = 0; i < 256; ++i) {
        const u32 j = static_cast<u32>(i) + static_cast<u32>(rng.nextInt(256 - i));
        std::swap(m_permutation[static_cast<size_t>(i)], m_permutation[static_cast<size_t>(j)]);
    }

    // 复制到工作数组
    for (i32 i = 0; i < 256; ++i) {
        m_p[static_cast<size_t>(i)] = m_permutation[static_cast<size_t>(i)];
        m_p[static_cast<size_t>(i + 256)] = m_permutation[static_cast<size_t>(i)];
    }

    // 设置随机偏移（MC 使用 nextDouble() * 256.0，f64 精度）
    m_xOffset = rng.nextDouble() * 256.0;
    m_yOffset = rng.nextDouble() * 256.0;
    m_zOffset = rng.nextDouble() * 256.0;
}

// ============================================================================
// 噪声采样（f32 接口，兼容 INoiseGenerator）
// ============================================================================

f32 ImprovedNoiseGenerator::noise(f32 x, f32 y, f32 z) const
{
    return static_cast<f32>(noise(static_cast<f64>(x), static_cast<f64>(y), static_cast<f64>(z)));
}

// ============================================================================
// 噪声采样（f64 精度）
// ============================================================================

f64 ImprovedNoiseGenerator::noise(f64 x, f64 y, f64 z) const
{
    // 加上随机偏移
    const f64 dx = x + m_xOffset;
    const f64 dy = y + m_yOffset;
    const f64 dz = z + m_zOffset;

    // 整数网格坐标
    const i32 cellX = math::floorTo<i32>(dx);
    const i32 cellY = math::floorTo<i32>(dy);
    const i32 cellZ = math::floorTo<i32>(dz);

    // 单元格内小数坐标
    const f64 fracX = dx - static_cast<f64>(cellX);
    const f64 fracY = dy - static_cast<f64>(cellY);
    const f64 fracZ = dz - static_cast<f64>(cellZ);

    return sampleAndLerp(cellX, cellY, cellZ, fracX, fracY, fracZ);
}

// ============================================================================
// 噪声采样（带 Y 轴缩放，f64 版本）
// ============================================================================

f64 ImprovedNoiseGenerator::noise(f64 x, f64 y, f64 z, f64 yScale, f64 maxY) const
{
    // 加上随机偏移
    const f64 dx = x + m_xOffset;
    const f64 dy = y + m_yOffset;
    const f64 dz = z + m_zOffset;

    // 整数网格坐标
    const i32 cellX = math::floorTo<i32>(dx);
    const i32 cellY = math::floorTo<i32>(dy);
    const i32 cellZ = math::floorTo<i32>(dz);

    // 单元格内小数坐标
    const f64 fracX = dx - static_cast<f64>(cellX);
    const f64 fracY = dy - static_cast<f64>(cellY);
    const f64 fracZ = dz - static_cast<f64>(cellZ);

    // Y 轴缩放（与 MC ImprovedNoise 一致）
    f64 adjustedFracY = 0.0;
    if (yScale != 0.0) {
        const f64 clampedY = (maxY >= 0.0 && maxY < fracY) ? maxY : fracY;
        adjustedFracY = std::floor(clampedY / yScale + 1.0e-7) * yScale;
    }

    return sampleAndLerp(cellX, cellY, cellZ, fracX, fracY - adjustedFracY, fracZ);
}

// ============================================================================
// 带导数的噪声采样
// ============================================================================

f64 ImprovedNoiseGenerator::noiseWithDerivative(f64 x, f64 y, f64 z, f64 derivatives[3]) const
{
    // 加上随机偏移
    const f64 dx = x + m_xOffset;
    const f64 dy = y + m_yOffset;
    const f64 dz = z + m_zOffset;

    // 整数网格坐标
    const i32 cellX = math::floorTo<i32>(dx);
    const i32 cellY = math::floorTo<i32>(dy);
    const i32 cellZ = math::floorTo<i32>(dz);

    // 单元格内小数坐标
    const f64 fracX = dx - static_cast<f64>(cellX);
    const f64 fracY = dy - static_cast<f64>(cellY);
    const f64 fracZ = dz - static_cast<f64>(cellZ);

    return sampleWithDerivative(cellX, cellY, cellZ, fracX, fracY, fracZ, derivatives);
}

// ============================================================================
// 采样并三线性插值
// ============================================================================

f64 ImprovedNoiseGenerator::sampleAndLerp(
    i32 cellX, i32 cellY, i32 cellZ, f64 fracX, f64 fracY, f64 fracZ) const noexcept
{
    // Smoothstep 插值因子
    const f64 sx = fade(fracX);
    const f64 sy = fade(fracY);
    const f64 sz = fade(fracZ);

    // 哈希索引
    const i32 i0 = _getPermut(cellX);
    const i32 i1 = _getPermut(cellX + 1);

    const i32 j0 = _getPermut(i0 + cellY);
    const i32 j1 = _getPermut(i1 + cellY);
    const i32 j2 = _getPermut(i0 + cellY + 1);
    const i32 j3 = _getPermut(i1 + cellY + 1);

    // 8 个角的梯度点积
    const f64 v000 = gradDot(_getPermut(j0 + cellZ), fracX, fracY, fracZ);
    const f64 v100 = gradDot(_getPermut(j1 + cellZ), fracX - 1.0, fracY, fracZ);
    const f64 v010 = gradDot(_getPermut(j2 + cellZ), fracX, fracY - 1.0, fracZ);
    const f64 v110 = gradDot(_getPermut(j3 + cellZ), fracX - 1.0, fracY - 1.0, fracZ);
    const f64 v001 = gradDot(_getPermut(j0 + cellZ + 1), fracX, fracY, fracZ - 1.0);
    const f64 v101 = gradDot(_getPermut(j1 + cellZ + 1), fracX - 1.0, fracY, fracZ - 1.0);
    const f64 v011 = gradDot(_getPermut(j2 + cellZ + 1), fracX, fracY - 1.0, fracZ - 1.0);
    const f64 v111 = gradDot(_getPermut(j3 + cellZ + 1), fracX - 1.0, fracY - 1.0, fracZ - 1.0);

    // 三线性插值（X→Y→Z 顺序，与 MC Mth.lerp3 一致）
    const f64 lerpX0 = lerp(v000, v100, sx);
    const f64 lerpX1 = lerp(v010, v110, sx);
    const f64 lerpX2 = lerp(v001, v101, sx);
    const f64 lerpX3 = lerp(v011, v111, sx);

    const f64 lerpY0 = lerp(lerpX0, lerpX1, sy);
    const f64 lerpY1 = lerp(lerpX2, lerpX3, sy);

    return lerp(lerpY0, lerpY1, sz);
}

// ============================================================================
// 带导数的采样
// ============================================================================

f64 ImprovedNoiseGenerator::sampleWithDerivative(
    i32 cellX, i32 cellY, i32 cellZ, f64 fracX, f64 fracY, f64 fracZ, f64 derivatives[3]) const noexcept
{
    // Smoothstep 插值因子及其导数
    const f64 sx = fade(fracX);
    const f64 sy = fade(fracY);
    const f64 sz = fade(fracZ);
    const f64 dsx = fadeDerivative(fracX);
    const f64 dsy = fadeDerivative(fracY);
    const f64 dsz = fadeDerivative(fracZ);

    // 哈希索引
    const i32 i0 = _getPermut(cellX);
    const i32 i1 = _getPermut(cellX + 1);

    const i32 j0 = _getPermut(i0 + cellY);
    const i32 j1 = _getPermut(i1 + cellY);
    const i32 j2 = _getPermut(i0 + cellY + 1);
    const i32 j3 = _getPermut(i1 + cellY + 1);

    // 8 个角的梯度点积
    const f64 v000 = gradDot(_getPermut(j0 + cellZ), fracX, fracY, fracZ);
    const f64 v100 = gradDot(_getPermut(j1 + cellZ), fracX - 1.0, fracY, fracZ);
    const f64 v010 = gradDot(_getPermut(j2 + cellZ), fracX, fracY - 1.0, fracZ);
    const f64 v110 = gradDot(_getPermut(j3 + cellZ), fracX - 1.0, fracY - 1.0, fracZ);
    const f64 v001 = gradDot(_getPermut(j0 + cellZ + 1), fracX, fracY, fracZ - 1.0);
    const f64 v101 = gradDot(_getPermut(j1 + cellZ + 1), fracX - 1.0, fracY, fracZ - 1.0);
    const f64 v011 = gradDot(_getPermut(j2 + cellZ + 1), fracX, fracY - 1.0, fracZ - 1.0);
    const f64 v111 = gradDot(_getPermut(j3 + cellZ + 1), fracX - 1.0, fracY - 1.0, fracZ - 1.0);

    // 三线性插值
    const f64 lerpX0 = lerp(v000, v100, sx);
    const f64 lerpX1 = lerp(v010, v110, sx);
    const f64 lerpX2 = lerp(v001, v101, sx);
    const f64 lerpX3 = lerp(v011, v111, sx);

    const f64 lerpY0 = lerp(lerpX0, lerpX1, sy);
    const f64 lerpY1 = lerp(lerpX2, lerpX3, sy);

    const f64 result = lerp(lerpY0, lerpY1, sz);

    // 计算导数（链式法则）
    // d/dx: dsx * (v100 - v000) * (1-sy)*(1-sz) + ... 展开各轴
    const f64 dxInner0 = v100 - v000;
    const f64 dxInner1 = v110 - v010;
    const f64 dxInner2 = v101 - v001;
    const f64 dxInner3 = v111 - v011;

    const f64 dLerpX0_dx = dsx * dxInner0;
    const f64 dLerpX1_dx = dsx * dxInner1;
    const f64 dLerpX2_dx = dsx * dxInner2;
    const f64 dLerpX3_dx = dsx * dxInner3;

    derivatives[0] = lerp(lerp(dLerpX0_dx, dLerpX1_dx, sy), lerp(dLerpX2_dx, dLerpX3_dx, sy), sz);

    // d/dy
    const f64 dLerpY0_dy = dsy * (lerpX1 - lerpX0);
    const f64 dLerpY1_dy = dsy * (lerpX3 - lerpX2);
    derivatives[1] = lerp(dLerpY0_dy, dLerpY1_dy, sz);

    // d/dz
    derivatives[2] = dsz * (lerpY1 - lerpY0);

    return result;
}

// ============================================================================
// 梯度点积（f64 精度）
// ============================================================================

f64 ImprovedNoiseGenerator::gradDot(i32 hash, f64 x, f64 y, f64 z) noexcept
{
    const i32 idx = hash & 15;
    return PERLIN_GRADIENTS_F64[idx][0] * x + PERLIN_GRADIENTS_F64[idx][1] * y + PERLIN_GRADIENTS_F64[idx][2] * z;
}

} // namespace mc
