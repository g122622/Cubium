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

#include "server/world/gen/noise/NormalNoise.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/Random.hpp"
#include "server/world/gen/noise/PerlinNoise.hpp"
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
    // 【注意】原版 NormalNoise.create 收的是 RandomSource，这里的种子构造是本项目
    // 自有的便利重载；走的是 Xoroshiro flavor（与 RandomState::getOrCreateNoise 一致）。
    math::Random rng(seed);
    m_first = std::make_unique<PerlinNoise>(rng.forkPositional(), m_firstOctave, m_amplitudes);
    m_second = std::make_unique<PerlinNoise>(rng.forkPositional(), m_firstOctave, m_amplitudes);

    computeValueFactor();
}

NormalNoise::NormalNoise(math::IRandom& rng, i32 firstOctave, std::vector<f64> amplitudes)
    : m_seed(std::nullopt) // 通过 math::IRandom& 构造，种子未知
    , m_firstOctave(firstOctave)
    , m_amplitudes(std::move(amplitudes))
{
    // MC 1.21: 两次调用 forkPositional() 获取不同的 PositionalRandomFactory
    // 与 MC NormalNoise(RandomSource, NoiseParameters) 一致
    // 注意：通过 math::IRandom& 构造时无法提取种子，clone() 将无法正确工作
    m_first = std::make_unique<PerlinNoise>(rng.forkPositional(), m_firstOctave, m_amplitudes);
    m_second = std::make_unique<PerlinNoise>(rng.forkPositional(), m_firstOctave, m_amplitudes);

    computeValueFactor();
}

std::shared_ptr<NormalNoise> NormalNoise::createLegacyNetherBiome(
    math::IRandom& rng, i32 firstOctave, std::vector<f64> amplitudes)
{
    // 走 PerlinNoise 的**旧版**构造（共享 RandomSource 顺序消费），两个 PerlinNoise
    // 依次从同一个随机源取数；这正是原版 createLegacyForLegacyNetherBiome 的语义。
    // 私有构造只能经 new 取用（make_shared 无法访问私有构造函数中的默认构造）。
    std::shared_ptr<NormalNoise> noise(new NormalNoise());
    noise->m_seed = std::nullopt;
    noise->m_firstOctave = firstOctave;
    noise->m_amplitudes = std::move(amplitudes);
    noise->m_first = std::make_unique<PerlinNoise>(rng, firstOctave, noise->m_amplitudes);
    noise->m_second = std::make_unique<PerlinNoise>(rng, firstOctave, noise->m_amplitudes);
    noise->computeValueFactor();
    return noise;
}

f64 NormalNoise::getValue(f64 x, f64 y, f64 z) const
{
    // 第二个噪声使用缩放坐标（乘以 INPUT_FACTOR）
    const f64 sx = x * INPUT_FACTOR;
    const f64 sy = y * INPUT_FACTOR;
    const f64 sz = z * INPUT_FACTOR;

    // 先舍入两个分量之和，再乘系数：原版是逐步舍入，`(a+b)*c` 被融合成 fma 会改动末位。
    const volatile f64 sum = m_first->getValue(x, y, z) + m_second->getValue(sx, sy, sz);
    const volatile f64 scaled = sum * m_valueFactor;
    return scaled;
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

    // 全零振幅（旧版维度的位移噪声）时没有任何倍频参与，原版的 k - j 落在 int 溢出语义上
    // （MIN_VALUE - MAX_VALUE 回绕成 1）；这里按同一口径取 1，避免减法溢出成 UB。
    const i32 octaveRange = (minNonZero > maxNonZero) ? 1 : (maxNonZero - minNonZero);
    m_valueFactor = VALUE_FACTOR_BASE / expectedDeviation(octaveRange);
    m_maxValue = (m_first->maxValue() + m_second->maxValue()) * m_valueFactor;
}

std::unique_ptr<NormalNoise> NormalNoise::clone() const
{
    // 只有通过种子构造的 NormalNoise 才能正确克隆
    // 通过 math::IRandom& 构造的实例种子为 nullopt，无法正确克隆
    MC_ASSERT_RELEASE_MSG(m_seed.has_value(), "Cannot clone NormalNoise constructed from math::IRandom& without seed");
    return std::make_unique<NormalNoise>(*m_seed, m_firstOctave, m_amplitudes);
}

} // namespace mc::world::gen::noise
