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

    // 设置随机偏移
    m_xOffset = rng.nextFloat(0.0f, 256.0f);
    m_yOffset = rng.nextFloat(0.0f, 256.0f);
    m_zOffset = rng.nextFloat(0.0f, 256.0f);
}

// ============================================================================
// 噪声采样
// ============================================================================

f32 ImprovedNoiseGenerator::noise(f32 x, f32 y, f32 z) const
{
    // 添加偏移
    x += m_xOffset;
    y += m_yOffset;
    z += m_zOffset;

    // 找到单位立方体的整数坐标
    const i32 ix = static_cast<i32>(std::floor(x));
    const i32 iy = static_cast<i32>(std::floor(y));
    const i32 iz = static_cast<i32>(std::floor(z));

    // 计算立方体内的小数坐标
    const f32 dx = x - static_cast<f32>(ix);
    const f32 dy = y - static_cast<f32>(iy);
    const f32 dz = z - static_cast<f32>(iz);

    // 计算 fade 曲线值
    const f32 fx = fade(dx);
    const f32 fy = fade(dy);
    const f32 fz = fade(dz);

    return noiseRaw(ix, iy, iz, dx, dy, dz, fx, fy, fz);
}

f32 ImprovedNoiseGenerator::noise(f32 x, f32 y, f32 z, f32 yScale, f32 yBound) const noexcept
{
    // 添加偏移
    x += m_xOffset;
    y += m_yOffset;
    z += m_zOffset;

    // 找到单位立方体的整数坐标
    const i32 ix = static_cast<i32>(std::floor(x));
    const i32 iy = static_cast<i32>(std::floor(y));
    const i32 iz = static_cast<i32>(std::floor(z));

    // 计算立方体内的小数坐标
    f32 dy = y - static_cast<f32>(iy);

    // Y 轴缩放
    if (yScale != 0.0f) {
        const f32 clampedY = std::min(yBound, dy);
        dy = std::floor(clampedY / yScale) * yScale;
    }

    const f32 dx = x - static_cast<f32>(ix);
    const f32 dz = z - static_cast<f32>(iz);

    // 计算 fade 曲线值
    const f32 fx = fade(dx);
    const f32 fy = fade(dy);
    const f32 fz = fade(dz);

    return noiseRaw(ix, iy, iz, dx, dy, dz, fx, fy, fz);
}

f32 ImprovedNoiseGenerator::noiseRaw(
    i32 x, i32 y, i32 z, f32 deltaX, f32 deltaY, f32 deltaZ, f32 fadeX, f32 fadeY, f32 fadeZ) const noexcept
{
    // 哈希索引
    const i32 i0 = _getPermut(x);
    const i32 i1 = _getPermut(x + 1);

    const i32 j0 = _getPermut(i0 + y);
    const i32 j1 = _getPermut(i1 + y);
    const i32 j2 = _getPermut(i0 + y + 1);
    const i32 j3 = _getPermut(i1 + y + 1);

    // 8 个角的梯度值
    const f32 n000 = grad(_getPermut(j0 + z), deltaX, deltaY, deltaZ);
    const f32 n100 = grad(_getPermut(j1 + z), deltaX - 1.0f, deltaY, deltaZ);
    const f32 n010 = grad(_getPermut(j2 + z), deltaX, deltaY - 1.0f, deltaZ);
    const f32 n110 = grad(_getPermut(j3 + z), deltaX - 1.0f, deltaY - 1.0f, deltaZ);
    const f32 n001 = grad(_getPermut(j0 + z + 1), deltaX, deltaY, deltaZ - 1.0f);
    const f32 n101 = grad(_getPermut(j1 + z + 1), deltaX - 1.0f, deltaY, deltaZ - 1.0f);
    const f32 n011 = grad(_getPermut(j2 + z + 1), deltaX, deltaY - 1.0f, deltaZ - 1.0f);
    const f32 n111 = grad(_getPermut(j3 + z + 1), deltaX - 1.0f, deltaY - 1.0f, deltaZ - 1.0f);

    // 三线性插值
    return math::lerp3(fadeX, fadeY, fadeZ, n000, n100, n010, n110, n001, n101, n011, n111);
}

// ============================================================================
// 梯度计算
// ============================================================================

f32 ImprovedNoiseGenerator::grad(i32 hash, f32 x, f32 y, f32 z) noexcept
{
    const i32 h = hash & 15;
    const f32* gradVec = PERLIN_GRADIENTS[h];
    return gradVec[0] * x + gradVec[1] * y + gradVec[2] * z;
}

} // namespace mc
