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
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/PositionalRandomFactory.hpp"
#include "common/util/math/random/Random.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc::world::gen::noise {

// ============================================================================
// PerlinNoise::PerlinLayer 实现
// ============================================================================

PerlinNoise::PerlinLayer::PerlinLayer(math::IRandom& rng)
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

f64 PerlinNoise::PerlinLayer::noiseWithSmear(f64 x, f64 y, f64 z, f64 yOffset, f64 yFraction) const
{
    const f64 dx = x + m_xOffset;
    const f64 dy = y + m_yOffset;
    const f64 dz = z + m_zOffset;

    const i32 cellX = math::floorTo<i32>(dx);
    const i32 cellY = math::floorTo<i32>(dy);
    const i32 cellZ = math::floorTo<i32>(dz);

    f64 fracX = dx - static_cast<f64>(cellX);
    f64 fracY = dy - static_cast<f64>(cellY);
    const f64 fracZ = dz - static_cast<f64>(cellZ);

    // MC 1.21.11: ImprovedNoise.noise(x, y, z, yOffset, yFraction)
    // 涂抹效果：将 Y 分数吸附到 yOffset 间隔的网格线上
    // 当 0 <= yFraction < fracY 时，使用 yFraction 作为吸附基准（原始缩放坐标），
    // 否则使用 fracY（偏移后的分数坐标）
    f64 smearOffset = 0.0;
    if (yOffset != 0.0) {
        f64 d7;
        if (yFraction >= 0.0 && yFraction < fracY) {
            d7 = yFraction;
        } else {
            d7 = fracY;
        }
        smearOffset = std::floor(d7 / yOffset + static_cast<f64>(1.0e-7f)) * yOffset;
    }

    // 注意：梯度计算使用修改后的 fracY (fracY - smearOffset)，
    // 但 smoothstep 插值使用原始 fracY
    return sampleAndLerp(cellX, cellY, cellZ, fracX, fracY - smearOffset, fracZ, fracY);
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

f64 PerlinNoise::PerlinLayer::sampleAndLerp(
    i32 cellX, i32 cellY, i32 cellZ, f64 fracX, f64 fracY, f64 fracZ, f64 smoothstepY) const
{
    // Smoothstep 插值因子
    // 当 smoothstepY >= 0 时（涂抹模式），Y 轴 smoothstep 使用原始分数，
    // 而 fracY 是修改后的分数用于梯度计算
    const f64 sx = fracX * fracX * fracX * (fracX * (fracX * 6.0 - 15.0) + 10.0);
    const f64 syRaw = (smoothstepY >= 0.0) ? smoothstepY : fracY;
    const f64 sy = syRaw * syRaw * syRaw * (syRaw * (syRaw * 6.0 - 15.0) + 10.0);
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
    buildSoA();
}

PerlinNoise::PerlinNoise(const math::PositionalRandomFactory& factory, i32 firstOctave, std::vector<f64> amplitudes)
    : m_firstOctave(firstOctave)
    , m_amplitudes(std::move(amplitudes))
{
    initLayers(factory);
    buildSoA();
}

PerlinNoise::PerlinNoise(math::JavaLegacyRandom& rng, i32 firstOctave, std::vector<f64> amplitudes)
    : m_firstOctave(firstOctave)
    , m_amplitudes(std::move(amplitudes))
{
    initLayersLegacy(rng);
    buildSoA();
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
    // 参考 MC 1.21.11: PerlinNoise 构造函数
    // Java 无条件计算这两个因子，C++ 同样无条件计算
    // 注：amplitudes.size() 而非非零振幅数，与 MC 一致
    m_lowestFreqInputFactor = std::pow(2.0, static_cast<f64>(m_firstOctave));
    const auto amplitudeCount = static_cast<i32>(m_amplitudes.size());
    m_lowestFreqValueFactor =
        std::pow(2.0, static_cast<f64>(amplitudeCount - 1)) / (std::pow(2.0, static_cast<f64>(amplitudeCount)) - 1.0);

    m_maxValue = edgeValue(2.0);
}

void PerlinNoise::initLayersLegacy(math::JavaLegacyRandom& rng)
{
    // MC 1.21.11: PerlinNoise(RandomSource, Pair<Integer, DoubleList>, false) 旧版构造路径
    // 不使用 PositionalRandomFactory，从同一个 RandomSource 顺序消费随机数
    const i32 octaveCount = static_cast<i32>(m_amplitudes.size());
    m_layers.resize(static_cast<size_t>(octaveCount));

    // j = -firstOctave：octave 0 在振幅列表中的索引
    const i32 j = -m_firstOctave;

    // 创建第一个 PerlinLayer（从 rng 消费随机数）
    auto firstLayer = std::make_unique<PerlinLayer>(rng);

    // octave 0 索引在范围内且振幅非零时，放入第一个 PerlinLayer
    if (j >= 0 && j < octaveCount) {
        if (m_amplitudes[static_cast<size_t>(j)] != 0.0) {
            m_layers[static_cast<size_t>(j)] = std::move(firstLayer);
        }
        // 振幅为 0 时 firstLayer 被丢弃，rng 状态已被正确消费
    }

    // 从 j-1 向下到 0 填充低频倍频层
    for (i32 i1 = j - 1; i1 >= 0; --i1) {
        if (i1 < octaveCount && m_amplitudes[static_cast<size_t>(i1)] != 0.0) {
            m_layers[static_cast<size_t>(i1)] = std::make_unique<PerlinLayer>(rng);
        } else {
            // 跳过此倍频层：consumeCount(262) 消费与 PerlinLayer 等量的随机数
            rng.consumeCount(262);
        }
    }

    // 验证：非空层数必须等于非零振幅数
    i32 nonNullCount = 0;
    i32 nonZeroAmplitudeCount = 0;
    for (i32 i = 0; i < octaveCount; ++i) {
        if (m_layers[static_cast<size_t>(i)] != nullptr) {
            ++nonNullCount;
        }
        if (m_amplitudes[static_cast<size_t>(i)] != 0.0) {
            ++nonZeroAmplitudeCount;
        }
    }
    MC_ASSERT_RELEASE(nonNullCount == nonZeroAmplitudeCount);

    // 禁止正倍频：j < octaveCount - 1 表示有正倍频（索引 > j 的层未被填充）
    // MC 抛出 IllegalArgumentException("Positive octaves are temporarily disabled")
    MC_ASSERT_RELEASE(j >= octaveCount - 1);

    // 计算缩放因子
    // MC: this.lowestFreqInputFactor = Math.pow(2.0, -j)
    // 由于 j = -firstOctave，-j = firstOctave
    // 对于 firstOctave=-15: lowestFreqInputFactor = 2^(-15) ≈ 3.05e-5
    m_lowestFreqInputFactor = std::pow(2.0, static_cast<f64>(-j));
    const auto amplitudeCount = static_cast<i32>(m_amplitudes.size());
    m_lowestFreqValueFactor =
        std::pow(2.0, static_cast<f64>(amplitudeCount - 1)) / (std::pow(2.0, static_cast<f64>(amplitudeCount)) - 1.0);

    m_maxValue = edgeValue(2.0);
}

void PerlinNoise::buildSoA()
{
    // 先数非空 octave 数 N。
    u32 count = 0;
    for (const auto& layer : m_layers) {
        if (layer != nullptr) {
            ++count;
        }
    }
    if (count == 0) {
        return; // m_soa 保持默认空载体
    }

    // 分配容纳 count 个 octave 的连续 SoA 块(64 字节对齐)。
    m_soa = PerlinNoiseSoA(count);

    // 遍历 m_layers,把非空 layer 的置换表 + 偏移 + 振幅 + 缩放因子写入 SoA。
    // inputFactor 从 lowestFreqInputFactor 起每层 ×2,valueFactor 从 lowestFreqValueFactor 起每层 ÷2,
    // 与 getValue 循环语义一致(空层跳过采样但仍推进缩放序列——此处空层不进 SoA,
    // 但 inputFactor/valueFactor 仍按原循环推进,保证非空层缩放因子正确)。
    u32 out = 0;
    f64 inputFactor = m_lowestFreqInputFactor;
    f64 valueFactor = m_lowestFreqValueFactor;
    for (size_t i = 0; i < m_layers.size(); ++i) {
        if (m_layers[i] != nullptr) {
            const auto& perm = m_layers[i]->permutation();
            std::memcpy(m_soa.permsData() + static_cast<size_t>(out) * 256ull, perm.data(), 256ull);
            m_soa.originXData()[out] = m_layers[i]->xOffset();
            m_soa.originYData()[out] = m_layers[i]->yOffset();
            m_soa.originZData()[out] = m_layers[i]->zOffset();
            m_soa.amplitudeData()[out] = m_amplitudes[i];
            m_soa.inputFactorData()[out] = inputFactor;
            m_soa.valueFactorData()[out] = valueFactor;
            ++out;
        }
        inputFactor *= 2.0;
        valueFactor /= 2.0;
    }
    MC_ASSERT_RELEASE_MSG(out == count, "PerlinNoise::buildSoA: octave count mismatch during fill");
}

f64 PerlinNoise::getValue(f64 x, f64 y, f64 z) const
{
    // SoA 向量化路径:每个 SIMD 通道算一个 octave(各自独立置换表做独立 gather 链),
    // hash 链 p[p[p[h]+y]+z] 内部串行不碰。结果先写扁平数组 ds[i],再标量顺序累加(保 bit-exact)。
    const u32 count = m_soa.count();
    if (count == 0) {
        return 0.0;
    }

    alignas(64) f64 ds[kMaxPerlinOctaves];
    const PerlinNoiseSoA& soa = m_soa;

#pragma clang loop vectorize_width(4) interleave_count(2)
    for (u32 i = 0; i < count; ++i) {
        const f64 nx = perlinWrap(x * soa.inputFactor()[i]);
        const f64 ny = perlinWrap(y * soa.inputFactor()[i]);
        const f64 nz = perlinWrap(z * soa.inputFactor()[i]);
        ds[i] = perlinSampleSoA(soa, i, nx, ny, nz, /*yScale=*/0.0, /*yMax=*/0.0);
    }

    // 标量顺序累加:与原 getValue 循环顺序一致 → bit-exact。
    f64 result = 0.0;
    for (u32 i = 0; i < count; ++i) {
        result += soa.amplitude()[i] * ds[i] * soa.valueFactor()[i];
    }
    return result;
}

f64 PerlinNoise::getValueScalar(f64 x, f64 y, f64 z) const
{
    // 纯标量路径:逐层调用 PerlinLayer::noise,不经 SoA 向量化。
    // 数值与 getValue bit-exact(两者都复刻原循环顺序)。
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
    // 参考 MC 1.21.11: PerlinNoise.edgeValue()
    // 不使用 abs()，直接使用原始振幅值
    f64 result = 0.0;
    f64 valueFactor = m_lowestFreqValueFactor;

    for (size_t i = 0; i < m_amplitudes.size(); ++i) {
        if (m_layers[i] != nullptr) {
            result += m_amplitudes[i] * maxInputValue * valueFactor;
        }
        valueFactor /= 2.0;
    }

    return result;
}

f64 PerlinNoise::maxBrokenValue(f64 maxInputValue) const
{
    // 参考 MC 1.21.11: PerlinNoise.maxBrokenValue()
    // maxBrokenValue 调用 edgeValue(maxInputValue + 2.0)，
    // +2.0 用于补偿涂抹效果带来的额外范围
    return edgeValue(maxInputValue + 2.0);
}

f64 PerlinNoise::getValueWithSmear(f64 x, f64 y, f64 z, f64 smearScaleMultiplier) const
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
            const f64 yOffset = smearScaleMultiplier * inputFactor;
            const f64 yFraction = y * inputFactor;
            result += amplitude * m_layers[i]->noiseWithSmear(nx, ny, nz, yOffset, yFraction) * valueFactor;
        }
        inputFactor *= 2.0;
        valueFactor /= 2.0;
    }

    return result;
}

} // namespace mc::world::gen::noise
