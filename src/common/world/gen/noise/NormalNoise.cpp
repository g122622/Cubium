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
}

f64 NormalNoise::getValue(f64 x, f64 y, f64 z) const
{
    // 第二个噪声使用缩放坐标（乘以 INPUT_FACTOR）
    const f64 sx = x * INPUT_FACTOR;
    const f64 sy = y * INPUT_FACTOR;
    const f64 sz = z * INPUT_FACTOR;

    return (m_first->getValue(x, y, z) + m_second->getValue(sx, sy, sz)) * m_valueFactor;
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
