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

#include "common/world/gen/noise/NormalNoise.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/gen/noise/PerlinNoise.hpp"
#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace mc::world::gen::noise {

NormalNoise::NormalNoise(u64 seed, i32 firstOctave, std::vector<f64> amplitudes)
    : m_seed(seed)
    , m_firstOctave(firstOctave)
    , m_amplitudes(std::move(amplitudes))
{
    // MC 1.21: 两个 PerlinNoise 共享同一个 RandomSource
    // 第一个 PerlinNoise 调用 forkPositional() 消耗两次 nextLong()
    // 第二个 PerlinNoise 再调用 forkPositional() 消耗两次 nextLong()
    // 因此两个工厂的种子不同，产生独立的噪声模式
    math::Random rng(seed);
    m_first = std::make_unique<PerlinNoise>(rng.forkPositional(), m_firstOctave, m_amplitudes);
    m_second = std::make_unique<PerlinNoise>(rng.forkPositional(), m_firstOctave, m_amplitudes);

    computeValueFactor();
    buildSoA();
}

NormalNoise::NormalNoise(math::Random& rng, i32 firstOctave, std::vector<f64> amplitudes)
    : m_seed(std::nullopt) // 通过 Random& 构造，种子未知
    , m_firstOctave(firstOctave)
    , m_amplitudes(std::move(amplitudes))
{
    // MC 1.21: 两次调用 forkPositional() 获取不同的 PositionalRandomFactory
    // 与 MC NormalNoise(RandomSource, NoiseParameters) 一致
    // 注意：通过 Random& 构造时无法提取种子，clone() 将无法正确工作
    m_first = std::make_unique<PerlinNoise>(rng.forkPositional(), m_firstOctave, m_amplitudes);
    m_second = std::make_unique<PerlinNoise>(rng.forkPositional(), m_firstOctave, m_amplitudes);

    computeValueFactor();
    buildSoA();
}

namespace {

/// 遍历 PerlinNoise 的非空 octave 子层，收集为 SoA 数组。
/// inputFactor 从 lowestFreqInputFactor 起每层 ×2，valueFactor 从 lowestFreqValueFactor 起每层 ÷2，
/// 空层（nullptr）跳过采样但仍推进缩放序列（与 PerlinNoise::getValue 循环语义一致）。
/// amplitude 取 amplitudes[i]（i 为该 octave 在 amplitudes 数组中的原始索引，与 getValue 累加项一致）。
void collectSoALayers(const PerlinNoise& noise, const std::vector<f64>& amplitudes, std::vector<PerlinSoALayer>& out)
{
    out.clear();
    const auto& layers = noise.layers();
    f64 inputFactor = noise.lowestFreqInputFactor();
    f64 valueFactor = noise.lowestFreqValueFactor();
    for (size_t i = 0; i < layers.size(); ++i) {
        if (layers[i] != nullptr) {
            PerlinSoALayer soa;
            std::copy_n(layers[i]->permutation().data(), 256, soa.permutation.data());
            soa.originX = layers[i]->xOffset();
            soa.originY = layers[i]->yOffset();
            soa.originZ = layers[i]->zOffset();
            soa.amplitude = amplitudes[i];
            soa.inputFactor = inputFactor;
            soa.valueFactor = valueFactor;
            out.push_back(soa);
        }
        inputFactor *= 2.0;
        valueFactor /= 2.0;
    }
}

} // namespace

void NormalNoise::buildSoA()
{
    // 收集 first/second 的非空 octave 为 SoA。m_first/m_second 共享同一 m_amplitudes，
    // 且非空 octave 索引一致（同一 firstOctave + amplitudes），故 amplitudes[i] 直接取。
    collectSoALayers(*m_first, m_amplitudes, m_firstSoA);
    collectSoALayers(*m_second, m_amplitudes, m_secondSoA);
}

f64 NormalNoise::getValue(f64 x, f64 y, f64 z) const
{
    // SoA 求值：first 各 octave 累加得 d1，second 各 octave 累加得 d2（second 坐标乘 INPUT_FACTOR），
    // 结果 (d1 + d2) * m_valueFactor。累加顺序与原 (m_first->getValue + m_second->getValue) 一致。
    // yScale=0/yMax=0 即不涂抹（NormalNoise 路径无 Y 涂抹，等价 PerlinLayer::noise）。
    auto sumSoA = [](const std::vector<PerlinSoALayer>& soa, f64 sx, f64 sy, f64 sz) -> f64 {
        f64 sum = 0.0;
        const size_t count = soa.size();
        for (size_t i = 0; i < count; ++i) {
            const PerlinSoALayer& layer = soa[i];
            const f64 nx = perlinWrap(sx * layer.inputFactor);
            const f64 ny = perlinWrap(sy * layer.inputFactor);
            const f64 nz = perlinWrap(sz * layer.inputFactor);
            const f64 sampled = perlinSample(
                layer.permutation.data(), layer.originX, layer.originY, layer.originZ, nx, ny, nz, 0.0, 0.0);
            sum += layer.amplitude * sampled * layer.valueFactor;
        }
        return sum;
    };

    const f64 d1 = sumSoA(m_firstSoA, x, y, z);
    const f64 sx = x * INPUT_FACTOR;
    const f64 sy = y * INPUT_FACTOR;
    const f64 sz = z * INPUT_FACTOR;
    const f64 d2 = sumSoA(m_secondSoA, sx, sy, sz);

    return (d1 + d2) * m_valueFactor;
}

f64 NormalNoise::expectedDeviation(i32 octaveRange)
{
    return 0.1 * (1.0 + 1.0 / static_cast<f64>(octaveRange + 1));
}

void NormalNoise::computeValueFactor()
{
    const i32 octaveCount = static_cast<i32>(m_amplitudes.size());
    i32 minNonZero = std::numeric_limits<i32>::max();
    i32 maxNonZero = std::numeric_limits<i32>::min();

    for (i32 i = 0; i < octaveCount; ++i) {
        if (m_amplitudes[static_cast<size_t>(i)] != 0.0) {
            minNonZero = std::min(minNonZero, i);
            maxNonZero = std::max(maxNonZero, i);
        }
    }

    const i32 octaveRange = maxNonZero - minNonZero;
    m_valueFactor = VALUE_FACTOR_BASE / expectedDeviation(octaveRange);
    m_maxValue = (m_first->maxValue() + m_second->maxValue()) * m_valueFactor;
}

std::unique_ptr<NormalNoise> NormalNoise::clone() const
{
    // 只有通过种子构造的 NormalNoise 才能正确克隆
    // 通过 Random& 构造的实例种子为 nullopt，无法正确克隆
    MC_ASSERT_RELEASE_MSG(m_seed.has_value(), "Cannot clone NormalNoise constructed from Random& without seed");
    return std::make_unique<NormalNoise>(*m_seed, m_firstOctave, m_amplitudes);
}

} // namespace mc::world::gen::noise
