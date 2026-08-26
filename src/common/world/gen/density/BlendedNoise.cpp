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
#include "common/core/Types.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/world/gen/noise/PerlinNoise.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc::world::gen::density {

namespace {

/// 创建全 1.0 振幅列表，对应 MC IntStream.rangeClosed(firstOctave, lastOctave)
std::vector<f64> makeLegacyAmplitudes(i32 firstOctave, i32 lastOctave)
{
    return std::vector<f64>(static_cast<size_t>(lastOctave - firstOctave + 1), 1.0);
}

} // namespace

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
    // MC 1.21.11: BlendedNoise(RandomSource, ...) 使用旧版种子派生
    // 三个 PerlinNoise 共享同一个 JavaLegacyRandom，顺序消费随机数。
    // 对应 MC: PerlinNoise.createLegacyForBlendedNoise(p_230462_, IntStream.rangeClosed(-15, 0))
    math::JavaLegacyRandom rng(seed);

    m_minLimitNoise = std::make_unique<noise::PerlinNoise>(rng, -15, makeLegacyAmplitudes(-15, 0));
    m_maxLimitNoise = std::make_unique<noise::PerlinNoise>(rng, -15, makeLegacyAmplitudes(-15, 0));
    m_mainNoise = std::make_unique<noise::PerlinNoise>(rng, -7, makeLegacyAmplitudes(-7, 0));

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
    // MC 1.21.11: createUnseeded 使用 seed=0 的旧版构造路径。
    // 仅用于序列化占位，运行时通过 withNewRandom 替换种子。
    math::JavaLegacyRandom rng(0);

    auto minLimitNoise = std::make_unique<noise::PerlinNoise>(rng, -15, makeLegacyAmplitudes(-15, 0));
    auto maxLimitNoise = std::make_unique<noise::PerlinNoise>(rng, -15, makeLegacyAmplitudes(-15, 0));
    auto mainNoise = std::make_unique<noise::PerlinNoise>(rng, -7, makeLegacyAmplitudes(-7, 0));

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
