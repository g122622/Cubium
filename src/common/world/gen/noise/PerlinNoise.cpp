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
 */

#include "common/world/gen/noise/PerlinNoise.hpp"
#include "common/util/math/MathUtils.hpp"
#include <algorithm>
#include <cmath>

namespace mc::world::gen::noise {

// ============================================================================
// PerlinNoise::PerlinLayer 实现
// ============================================================================

PerlinNoise::PerlinLayer::PerlinLayer(math::Random& rng)
{
    // 随机偏移
    m_xOffset = rng.nextDouble() * 256.0;
    m_yOffset = rng.nextDouble() * 256.0;
    m_zOffset = rng.nextDouble() * 256.0;

    // 初始化并洗牌排列表
    m_permutation.resize(256);
    for (i32 i = 0; i < 256; ++i) {
        m_permutation[static_cast<size_t>(i)] = static_cast<u8>(i);
    }
    for (i32 i = 0; i < 256; ++i) {
        const i32 j = i + rng.nextInt(256 - i);
        std::swap(m_permutation[static_cast<size_t>(i)], m_permutation[static_cast<size_t>(j)]);
    }

    // 双倍排列表用于快速查找
    m_p.resize(512);
    for (i32 i = 0; i < 512; ++i) {
        m_p[static_cast<size_t>(i)] = m_permutation[i & 255];
    }
}

f64 PerlinNoise::PerlinLayer::noise(f64 x, f64 y, f64 z) const
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

i32 PerlinNoise::PerlinLayer::p(i32 index) const
{
    return m_p[static_cast<size_t>(index & 255)];
}

f64 PerlinNoise::PerlinLayer::gradDot(i32 hash, f64 x, f64 y, f64 z)
{
    // MC 使用与 SimplexNoise 共享的 16 条目梯度表，ImprovedNoise.gradDot 使用 & 15
    static constexpr f64 GRADIENTS[16][3] = {{1.0, 1.0, 0.0},
        {-1.0, 1.0, 0.0},
        {1.0, -1.0, 0.0},
        {-1.0, -1.0, 0.0},
        {1.0, 0.0, 1.0},
        {-1.0, 0.0, 1.0},
        {1.0, 0.0, -1.0},
        {-1.0, 0.0, -1.0},
        {0.0, 1.0, 1.0},
        {0.0, -1.0, 1.0},
        {0.0, 1.0, -1.0},
        {0.0, -1.0, -1.0},
        {1.0, 1.0, 0.0},
        {0.0, -1.0, 1.0},
        {-1.0, 1.0, 0.0},
        {0.0, -1.0, -1.0}};
    const i32 idx = hash & 15;
    return GRADIENTS[idx][0] * x + GRADIENTS[idx][1] * y + GRADIENTS[idx][2] * z;
}

f64 PerlinNoise::PerlinLayer::sampleAndLerp(i32 cellX, i32 cellY, i32 cellZ, f64 fracX, f64 fracY, f64 fracZ) const
{
    // Smoothstep 插值因子
    const f64 sx = fracX * fracX * fracX * (fracX * (fracX * 6.0 - 15.0) + 10.0);
    const f64 sy = fracY * fracY * fracY * (fracY * (fracY * 6.0 - 15.0) + 10.0);
    const f64 sz = fracZ * fracZ * fracZ * (fracZ * (fracZ * 6.0 - 15.0) + 10.0);

    // 8 个角的梯度计算
    const i32 i = p(cellX);
    const i32 j = p(cellX + 1);
    const i32 k = p(i + cellY);
    const i32 l = p(i + cellY + 1);
    const i32 i1 = p(j + cellY);
    const i32 j1 = p(j + cellY + 1);

    const f64 v000 = gradDot(p(k + cellZ), fracX, fracY, fracZ);
    const f64 v100 = gradDot(p(i1 + cellZ), fracX - 1.0, fracY, fracZ);
    const f64 v010 = gradDot(p(l + cellZ), fracX, fracY - 1.0, fracZ);
    const f64 v110 = gradDot(p(j1 + cellZ), fracX - 1.0, fracY - 1.0, fracZ);
    const f64 v001 = gradDot(p(k + cellZ + 1), fracX, fracY, fracZ - 1.0);
    const f64 v101 = gradDot(p(i1 + cellZ + 1), fracX - 1.0, fracY, fracZ - 1.0);
    const f64 v011 = gradDot(p(l + cellZ + 1), fracX, fracY - 1.0, fracZ - 1.0);
    const f64 v111 = gradDot(p(j1 + cellZ + 1), fracX - 1.0, fracY - 1.0, fracZ - 1.0);

    // 三线性插值
    const f64 lerpX0 = v000 + sx * (v100 - v000);
    const f64 lerpX1 = v010 + sx * (v110 - v010);
    const f64 lerpX2 = v001 + sx * (v101 - v001);
    const f64 lerpX3 = v011 + sx * (v111 - v011);

    const f64 lerpY0 = lerpX0 + sy * (lerpX1 - lerpX0);
    const f64 lerpY1 = lerpX2 + sy * (lerpX3 - lerpX2);

    return lerpY0 + sz * (lerpY1 - lerpY0);
}

// ============================================================================
// PerlinNoise 实现
// ============================================================================

PerlinNoise::PerlinNoise(u64 seed, i32 firstOctave, std::vector<f64> amplitudes)
    : m_firstOctave(firstOctave)
    , m_amplitudes(std::move(amplitudes))
{
    // MC 1.21: 使用 PositionalRandomFactory 风格的种子派生
    math::Random rng(seed);
    const math::PositionalRandomFactory factory = rng.forkPositional();
    initLayers(factory);
}

PerlinNoise::PerlinNoise(const math::PositionalRandomFactory& factory, i32 firstOctave, std::vector<f64> amplitudes)
    : m_firstOctave(firstOctave)
    , m_amplitudes(std::move(amplitudes))
{
    initLayers(factory);
}

void PerlinNoise::initLayers(const math::PositionalRandomFactory& factory)
{
    const i32 octaveCount = static_cast<i32>(m_amplitudes.size());
    m_layers.resize(static_cast<size_t>(octaveCount));

    // MC 1.21: 使用 PositionalRandomFactory.fromHashOf("octave_" + octaveIndex)
    // 为每个倍频创建独立的随机数生成器
    for (i32 i = 0; i < octaveCount; ++i) {
        if (m_amplitudes[static_cast<size_t>(i)] != 0.0) {
            const i32 octaveIndex = m_firstOctave + i;
            const std::string key = "octave_" + std::to_string(octaveIndex);
            auto octaveRng = factory.fromHashOf(key);
            m_layers[static_cast<size_t>(i)] = std::make_unique<PerlinLayer>(*octaveRng);
        }
    }

    // 查找非零振幅的范围
    i32 minNonZero = std::numeric_limits<i32>::max();
    i32 maxNonZero = std::numeric_limits<i32>::min();

    for (i32 i = 0; i < octaveCount; ++i) {
        if (m_amplitudes[static_cast<size_t>(i)] != 0.0) {
            minNonZero = std::min(minNonZero, i);
            maxNonZero = std::max(maxNonZero, i);
        }
    }

    // 计算最低频率的输入和值缩放因子
    if (minNonZero < octaveCount) {
        const i32 nonZeroRange = maxNonZero - minNonZero;
        if (nonZeroRange == 0) {
            m_lowestFreqInputFactor = 1.0;
            m_lowestFreqValueFactor = 1.0;
        } else {
            m_lowestFreqInputFactor = std::pow(2.0, -static_cast<f64>(m_firstOctave + minNonZero));
            const i32 nonZeroCount = nonZeroRange + 1;
            m_lowestFreqValueFactor = std::pow(2.0, nonZeroCount - 1) / (std::pow(2.0, nonZeroCount) - 1.0);
        }
    }

    m_maxValue = edgeValue(2.0);
}

f64 PerlinNoise::getValue(f64 x, f64 y, f64 z) const
{
    f64 result = 0.0;
    f64 inputFactor = m_lowestFreqInputFactor;
    f64 valueFactor = m_lowestFreqValueFactor;

    for (size_t i = 0; i < m_layers.size(); ++i) {
        if (m_layers[i] != nullptr) {
            const f64 amplitude = m_amplitudes[i];
            const f64 nx = wrap(x * inputFactor);
            const f64 ny = wrap(y * inputFactor);
            const f64 nz = wrap(z * inputFactor);
            result += amplitude * m_layers[i]->noise(nx, ny, nz) * valueFactor;
        }
        inputFactor *= 2.0;
        valueFactor /= 2.0;
    }

    return result;
}

f64 PerlinNoise::wrap(f64 value)
{
    // 2^25 = 33554432.0，防止大坐标精度丢失
    constexpr f64 WRAP_PERIOD = 33554432.0;
    return value - std::floor(value / WRAP_PERIOD + 0.5) * WRAP_PERIOD;
}

f64 PerlinNoise::edgeValue(f64 maxInputValue) const
{
    f64 result = 0.0;
    f64 valueFactor = m_lowestFreqValueFactor;

    for (size_t i = 0; i < m_amplitudes.size(); ++i) {
        if (m_layers[i] != nullptr) {
            result += std::abs(m_amplitudes[i]) * maxInputValue * valueFactor;
        }
        valueFactor /= 2.0;
    }

    return result;
}

} // namespace mc::world::gen::noise
