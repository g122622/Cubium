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

#include "common/world/gen/density/BlendedNoise.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc::world::gen::density {

// ============================================================================
// 构造函数
// ============================================================================

BlendedNoise::BlendedNoise(u64 seed, f64 xzScale, f64 yScale, f64 xzFactor, f64 yFactor, f64 smearScaleMultiplier)
    : m_xzScale(xzScale)
    , m_yScale(yScale)
    , m_xzFactor(xzFactor)
    , m_yFactor(yFactor)
    , m_smearScaleMultiplier(smearScaleMultiplier)
    , m_seed(seed)
{
    // MC 1.21.11: BlendedNoise(RandomSource, ...)
    // 使用 PerlinNoise.createLegacyForBlendedNoise 创建三个 PerlinNoise：
    // - minLimitNoise: rangeClosed(-15, 0) = 16 倍频
    // - maxLimitNoise: rangeClosed(-15, 0) = 16 倍频
    // - mainNoise: rangeClosed(-7, 0) = 8 倍频
    //
    // createLegacyForBlendedNoise 使用旧的种子派生方式：
    // 各倍频使用连续种子 (seed, seed+1, seed+2, ...) 而非 fromHashOf("octave_N")
    math::Random rng(seed);
    auto factory = rng.forkPositional();

    // 为 createLegacyForBlendedNoise，需要使用 LegacyRandomSource 风格种子派生
    // MC 中 PerlinNoise.createLegacyForBlendedNoise 使用 IntStream.rangeClosed
    // 创建振幅全为 1.0 的倍频序列，然后通过 consume 前进随机数生成器
    const auto makeLegacyAmplitudes = [](i32 firstOctave, i32 lastOctave) {
        const size_t count = static_cast<size_t>(lastOctave - firstOctave + 1);
        std::vector<f64> amps(count, 1.0);
        return amps;
    };

    // minLimitNoise 和 maxLimitNoise: firstOctave=-15, 16 octaves (amplitudes all 1.0)
    auto minAmps = makeLegacyAmplitudes(-15, 0);
    m_minLimitNoise = std::make_unique<noise::PerlinNoise>(factory, -15, std::move(minAmps));

    auto maxAmps = makeLegacyAmplitudes(-15, 0);
    m_maxLimitNoise = std::make_unique<noise::PerlinNoise>(factory, -15, std::move(maxAmps));

    // mainNoise: firstOctave=-7, 8 octaves (amplitudes all 1.0)
    auto mainAmps = makeLegacyAmplitudes(-7, 0);
    m_mainNoise = std::make_unique<noise::PerlinNoise>(factory, -7, std::move(mainAmps));

    initFromNoises();
}

BlendedNoise::BlendedNoise(const math::PositionalRandomFactory& factory,
    f64 xzScale,
    f64 yScale,
    f64 xzFactor,
    f64 yFactor,
    f64 smearScaleMultiplier)
    : m_xzScale(xzScale)
    , m_yScale(yScale)
    , m_xzFactor(xzFactor)
    , m_yFactor(yFactor)
    , m_smearScaleMultiplier(smearScaleMultiplier)
    , m_seed(0)
{
    const auto makeLegacyAmplitudes = [](i32 firstOctave, i32 lastOctave) {
        const size_t count = static_cast<size_t>(lastOctave - firstOctave + 1);
        std::vector<f64> amps(count, 1.0);
        return amps;
    };

    auto minAmps = makeLegacyAmplitudes(-15, 0);
    m_minLimitNoise = std::make_unique<noise::PerlinNoise>(factory, -15, std::move(minAmps));

    auto maxAmps = makeLegacyAmplitudes(-15, 0);
    m_maxLimitNoise = std::make_unique<noise::PerlinNoise>(factory, -15, std::move(maxAmps));

    auto mainAmps = makeLegacyAmplitudes(-7, 0);
    m_mainNoise = std::make_unique<noise::PerlinNoise>(factory, -7, std::move(mainAmps));

    initFromNoises();
}

BlendedNoise::BlendedNoise(std::unique_ptr<noise::PerlinNoise> minLimitNoise,
    std::unique_ptr<noise::PerlinNoise> maxLimitNoise,
    std::unique_ptr<noise::PerlinNoise> mainNoise,
    f64 xzScale,
    f64 yScale,
    f64 xzFactor,
    f64 yFactor,
    f64 smearScaleMultiplier)
    : m_minLimitNoise(std::move(minLimitNoise))
    , m_maxLimitNoise(std::move(maxLimitNoise))
    , m_mainNoise(std::move(mainNoise))
    , m_xzScale(xzScale)
    , m_yScale(yScale)
    , m_xzFactor(xzFactor)
    , m_yFactor(yFactor)
    , m_smearScaleMultiplier(smearScaleMultiplier)
    , m_seed(0)
{
    initFromNoises();
}

void BlendedNoise::initFromNoises()
{
    m_xzMultiplier = 684.412 * m_xzScale;
    m_yMultiplier = 684.412 * m_yScale;
    // MC 1.21.11: maxValue = minLimitNoise.maxBrokenValue(yMultiplier)
    m_maxValue = m_minLimitNoise->maxBrokenValue(m_yMultiplier);
}

// ============================================================================
// compute — 核心密度计算
// ============================================================================

f64 BlendedNoise::compute(i32 blockX, i32 blockY, i32 blockZ) const
{
    // MC 1.21.11: BlendedNoise.compute()
    const f64 xzMultiplier = m_xzMultiplier;
    const f64 yMultiplier = m_yMultiplier;

    const f64 d0 = static_cast<f64>(blockX) * xzMultiplier;
    const f64 d1 = static_cast<f64>(blockY) * yMultiplier;
    const f64 d2 = static_cast<f64>(blockZ) * xzMultiplier;

    const f64 d3 = d0 / m_xzFactor; // xzCoord / xzFactor
    const f64 d4 = d1 / m_yFactor;  // yCoord / yFactor
    const f64 d5 = d2 / m_xzFactor; // xzCoord / xzFactor

    const f64 d6 = yMultiplier * m_smearScaleMultiplier; // smearFactor
    const f64 d7 = d6 / m_yFactor;                       // smearFactor / yFactor

    // ---- 第一阶段：mainNoise 8 倍频，计算插值因子 ----
    f64 d10 = 0.0; // mainNoise 累积值
    f64 d11 = 1.0; // 倍频缩放

    for (i32 i = 0; i < 8; ++i) {
        const noise::PerlinNoise::PerlinLayer* layer = m_mainNoise->getOctaveNoise(i);
        if (layer != nullptr) {
            const f64 nx = noise::PerlinNoise::wrap(d3 * d11);
            const f64 ny = noise::PerlinNoise::wrap(d4 * d11);
            const f64 nz = noise::PerlinNoise::wrap(d5 * d11);
            const f64 yOffset = d7 * d11;
            const f64 yFraction = d4 * d11;
            d10 += layer->noiseWithSmear(nx, ny, nz, yOffset, yFraction) / d11;
        }
        d11 /= 2.0;
    }

    // MC 1.21: 插值因子 d16 = (mainResult/10 + 1) / 2，不做预裁剪
    // MC 原版不预裁剪 d16，clampedLerp 内部处理越界情况
    const f64 d16 = (d10 / 10.0 + 1.0) / 2.0;
    const bool flag1 = d16 >= 1.0; // 只采样 minLimitNoise
    const bool flag2 = d16 <= 0.0; // 只采样 maxLimitNoise

    // ---- 第二阶段：minLimitNoise 和 maxLimitNoise 16 倍频 ----
    f64 d8 = 0.0; // minLimitNoise 累积值
    f64 d9 = 0.0; // maxLimitNoise 累积值
    d11 = 1.0;

    for (i32 j = 0; j < 16; ++j) {
        const f64 d12 = noise::PerlinNoise::wrap(d0 * d11);
        const f64 d13 = noise::PerlinNoise::wrap(d1 * d11);
        const f64 d14 = noise::PerlinNoise::wrap(d2 * d11);
        const f64 d15 = d6 * d11; // smearFactor * scale

        if (!flag1) {
            const noise::PerlinNoise::PerlinLayer* layer = m_minLimitNoise->getOctaveNoise(j);
            if (layer != nullptr) {
                d8 += layer->noiseWithSmear(d12, d13, d14, d15, d1 * d11) / d11;
            }
        }

        if (!flag2) {
            const noise::PerlinNoise::PerlinLayer* layer = m_maxLimitNoise->getOctaveNoise(j);
            if (layer != nullptr) {
                d9 += layer->noiseWithSmear(d12, d13, d14, d15, d1 * d11) / d11;
            }
        }

        d11 /= 2.0;
    }

    // ---- 最终结果 ----
    // MC 1.21: clampedLerp(d16, minResult/512, maxResult/512) / 128
    // clampedLerp(delta, from, to): delta < 0 → from, delta > 1 → to, else lerp
    // from = d8/512 (minLimit), to = d9/512 (maxLimit)
    f64 result;
    if (d16 < 0.0) {
        result = d8 / 512.0;
    } else if (d16 > 1.0) {
        result = d9 / 512.0;
    } else {
        result = d8 / 512.0 + d16 * (d9 / 512.0 - d8 / 512.0);
    }

    return result / 128.0;
}

// ============================================================================
// createUnseeded — 用于序列化/注册的未种子化构造
// ============================================================================

std::unique_ptr<BlendedNoise> BlendedNoise::createUnseeded(
    f64 xzScale, f64 yScale, f64 xzFactor, f64 yFactor, f64 smearScaleMultiplier)
{
    // MC 1.21.11: createUnseeded 使用 seed=0 的 XoroshiroRandomSource
    auto minAmps = std::vector<f64>(16, 1.0);
    auto maxAmps = std::vector<f64>(16, 1.0);
    auto mainAmps = std::vector<f64>(8, 1.0);

    math::Random rng(0);
    auto factory = rng.forkPositional();

    auto minLimitNoise = std::make_unique<noise::PerlinNoise>(factory, -15, std::move(minAmps));
    auto maxLimitNoise = std::make_unique<noise::PerlinNoise>(factory, -15, std::move(maxAmps));
    auto mainNoise = std::make_unique<noise::PerlinNoise>(factory, -7, std::move(mainAmps));

    return std::make_unique<BlendedNoise>(std::move(minLimitNoise),
        std::move(maxLimitNoise),
        std::move(mainNoise),
        xzScale,
        yScale,
        xzFactor,
        yFactor,
        smearScaleMultiplier);
}

} // namespace mc::world::gen::density
