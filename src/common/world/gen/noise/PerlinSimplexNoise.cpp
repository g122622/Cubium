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

#include "PerlinSimplexNoise.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/LcgRandom.hpp"
#include <algorithm>
#include <cmath>
#include <set>

namespace mc::world::gen::noise {

PerlinSimplexNoise::PerlinSimplexNoise(math::IRandom& rng, std::vector<i32> octaves)
{
    if (octaves.empty()) {
        MC_ASSERT_RELEASE(false && "PerlinSimplexNoise needs at least one octave");
    }

    // MC: 排序倍频索引并找到范围
    std::set<i32> octaveSet(octaves.begin(), octaves.end());

    const i32 minOctaveNeg = -(*octaveSet.begin()); // i = -firstInt()
    const i32 maxOctave = *octaveSet.rbegin();      // j = lastInt()
    const i32 k = minOctaveNeg + maxOctave + 1;

    MC_ASSERT_RELEASE(k >= 1 && "Total number of octaves needs to be >= 1");

    m_noiseLevels.resize(static_cast<size_t>(k));

    // MC: 创建第一个 SimplexNoise（共享给 octave 0）
    auto firstNoise = std::make_unique<SimplexNoise>(rng);

    // MC: 如果 octave 0 在集合中，放入 noiseLevels[maxOctave]
    if (maxOctave >= 0 && maxOctave < k && octaveSet.count(0)) {
        m_noiseLevels[static_cast<size_t>(maxOctave)] = std::move(firstNoise);
    }

    // MC: 填充正倍频 (maxOctave+1 到 k-1)
    // 对应 octaves 从 l-1 递减（l = maxOctave，所以 l-(maxOctave+1) = -1, l-(maxOctave+2) = -2, ...）
    for (i32 i1 = maxOctave + 1; i1 < k; ++i1) {
        if (i1 >= 0 && octaveSet.count(maxOctave - i1)) {
            m_noiseLevels[static_cast<size_t>(i1)] = std::make_unique<SimplexNoise>(rng);
        } else {
            rng.skip(262);
        }
    }

    // MC: 填充负倍频 (maxOctave-1 到 0)
    // 使用第一个 simplexnoise 的 3D 评估值派生种子
    if (maxOctave > 0) {
        const SimplexNoise* firstNoisePtr = m_noiseLevels[static_cast<size_t>(maxOctave)].get();
        MC_ASSERT_RELEASE(firstNoisePtr != nullptr);

        const f64 derivedSeed =
            firstNoisePtr->getValue(firstNoisePtr->xOffset(), firstNoisePtr->yOffset(), firstNoisePtr->zOffset());
        const i64 seed = static_cast<i64>(derivedSeed * 9.223372E18);

        // MC: WorldgenRandom(new LegacyRandomSource(seed))
        // 使用 LcgRandom 近似 LegacyRandomSource 的行为
        math::LcgRandom secondaryRng(static_cast<u64>(seed));

        for (i32 j1 = maxOctave - 1; j1 >= 0; --j1) {
            if (j1 < k && octaveSet.count(maxOctave - j1)) {
                m_noiseLevels[static_cast<size_t>(j1)] = std::make_unique<SimplexNoise>(secondaryRng);
            } else {
                secondaryRng.skip(262);
            }
        }
    }

    m_highestFreqInputFactor = std::pow(2.0, static_cast<f64>(maxOctave));
    m_highestFreqValueFactor = 1.0 / (std::pow(2.0, static_cast<f64>(k)) - 1.0);
}

f64 PerlinSimplexNoise::getValue(f64 x, f64 y, bool useOffset) const
{
    f64 result = 0.0;
    f64 freqInputFactor = m_highestFreqInputFactor;
    f64 freqValueFactor = m_highestFreqValueFactor;

    for (const auto& noise : m_noiseLevels) {
        if (noise) {
            result += noise->getValue(x * freqInputFactor + (useOffset ? noise->xOffset() : 0.0),
                          y * freqInputFactor + (useOffset ? noise->yOffset() : 0.0)) *
                freqValueFactor;
        }
        freqInputFactor /= 2.0;
        freqValueFactor *= 2.0;
    }

    return result;
}

} // namespace mc::world::gen::noise
