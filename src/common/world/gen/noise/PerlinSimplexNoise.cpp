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
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/world/gen/noise/SimplexNoise.hpp"
#include <cmath>
#include <cstddef>
#include <memory>
#include <set>
#include <utility>
#include <vector>

namespace mc::world::gen::noise {

PerlinSimplexNoise::PerlinSimplexNoise(math::JavaLegacyRandom& rng, std::vector<i32> octaves)
{
    if (octaves.empty()) {
        MC_ASSERT_RELEASE(false && "PerlinSimplexNoise needs at least one octave");
    }

    // MC: PerlinSimplexNoise 的构造需要 JavaLegacyRandom（MC 的 LegacyRandomSource）
    // consumeCount(262) 推进 262 步 LCG 状态，与 IRandom::skip(262) 不同
    // skip(262) 调用 nextU64() 262 次（每次 2 步 LCG，共 524 步），consumeCount(262) 每次只 1 步

    // MC: 排序倍频索引并找到范围
    std::set<i32> octaveSet(octaves.begin(), octaves.end());

    const i32 minOctaveNeg = -(*octaveSet.begin()); // i = -firstInt()
    const i32 maxOctave = *octaveSet.rbegin();      // j = lastInt()
    const i32 k = minOctaveNeg + maxOctave + 1;

    MC_ASSERT_RELEASE(k >= 1 && "Total number of octaves needs to be >= 1");

    m_noiseLevels.resize(static_cast<size_t>(k));
    m_maxOctave = maxOctave;

    // MC: 创建第一个 SimplexNoise（用于种子派生，也共享给 octave 0）
    auto firstNoise = std::make_unique<SimplexNoise>(rng);
    // 保存原始指针，因为 firstNoise 可能被 move 到 m_noiseLevels 中
    const SimplexNoise* firstNoisePtr = firstNoise.get();

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
            // MC: p_230543_.consumeCount(262) — 推进 262 步 LCG 状态
            rng.consumeCount(262);
        }
    }

    // MC: 填充负倍频 (maxOctave-1 到 0)
    // 使用第一个 simplexnoise 的 3D 评估值派生种子
    if (maxOctave > 0) {
        // firstNoisePtr 始终有效：在 move 之前已保存原始指针
        MC_ASSERT_RELEASE(firstNoisePtr != nullptr);

        // MC: WorldgenRandom(new LegacyRandomSource(seed))
        // Java 使用 float 精度乘法: (long)(simplexnoise.getValue(...) * 9.223372E18F)
        // 注意：Java 中 getValue() 返回 double，9.223372E18F 是 float 字面量
        // Java 二元数值提升：float 自动拓宽为 double，乘法在 double 精度下进行
        // 因此 C++ 必须先拓宽 float 常量为 double，再在 double 精度下做乘法
        const f64 derivedSeed =
            firstNoisePtr->getValue(firstNoisePtr->xOffset(), firstNoisePtr->yOffset(), firstNoisePtr->zOffset());
        const i64 seed = static_cast<i64>(derivedSeed * static_cast<f64>(9.223372E18f));

        // 使用 JavaLegacyRandom 复刻 MC 的 WorldgenRandom(LegacyRandomSource(seed))
        // Java LegacyRandomSource 使用 48 位 LCG（A=25214903917, C=11, mask=(1<<48)-1）
        // 种子初始化: state = (seed ^ 0x5DEECE66D) & mask
        math::JavaLegacyRandom secondaryRng(static_cast<u64>(seed));

        for (i32 j1 = maxOctave - 1; j1 >= 0; --j1) {
            if (j1 < k && octaveSet.count(maxOctave - j1)) {
                m_noiseLevels[static_cast<size_t>(j1)] = std::make_unique<SimplexNoise>(secondaryRng);
            } else {
                // MC: secondaryRng.consumeCount(262)
                secondaryRng.consumeCount(262);
            }
        }
    }

    m_highestFreqInputFactor = std::ldexp(1.0, maxOctave);
    m_highestFreqValueFactor = 1.0 / (std::ldexp(1.0, k) - 1.0);
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
