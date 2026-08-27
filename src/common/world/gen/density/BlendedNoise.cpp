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
#include "common/world/gen/noise/PerlinNoiseSoA.hpp"
#include <cmath>
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

    // ---- 第一阶段:mainNoise 8 倍频(向量化采样 + 标量累加)----
    // SoA 路径:每个 SIMD 通道算一个 octave(各自独立置换表做独立 gather 链)。
    // mainNoise = PerlinNoise(-7, 8 个全 1.0 振幅) → SoA count = 8,索引 0..7 对应最低频→最高频。
    // 原循环 getOctaveNoise(i) = SoA index (count-1-i),i=0 先累加最高频层(d11=1.0)。
    // SoA 正向 index k → 原循环 i = count-1-k → d11_k = 2^(k-(count-1))。
    // 采样写扁平数组 ds[k],再按 k=count-1..0 反向标量累加(复刻原 i=0..N-1 顺序 → bit-exact)。
    f64 d10 = 0.0; // mainNoise 累积值
    const noise::PerlinNoiseSoA& mainSoa = m_mainNoise->soa();
    const u32 mainCount = mainSoa.count();
    if (mainCount > 0) {
        alignas(64) f64 mainDs[noise::kMaxPerlinOctaves];
        // 预计算 d11_k = 2^(k-(count-1)) 数组:std::ldexp 是不可向量化的 libm 调用,
        // 放循环外预填(此预填循环不强求向量化),采样循环只读数组 → 可向量化。
        alignas(64) f64 mainD11[noise::kMaxPerlinOctaves];
        for (u32 k = 0; k < mainCount; ++k) {
            mainD11[k] = std::ldexp(1.0, static_cast<i32>(k) - static_cast<i32>(mainCount - 1));
        }

#pragma clang loop vectorize_width(4) interleave_count(2)
        for (u32 k = 0; k < mainCount; ++k) {
            const f64 d11 = mainD11[k];
            const f64 nx = noise::perlinWrap(d3 * d11);
            const f64 ny = noise::perlinWrap(d4 * d11);
            const f64 nz = noise::perlinWrap(d5 * d11);
            const f64 yScale = d7 * d11;
            const f64 yMax = d4 * d11;
            // yScale!=0 → 启用 Y 涂抹(perlinSampleSoA yScale/yMax 语义)。
            mainDs[k] = noise::perlinSampleSoA(mainSoa, k, nx, ny, nz, yScale, yMax);
        }

        // 反向标量累加:k=count-1..0 对应原循环 i=0..count-1。每项除以 d11_k(=2 的整数幂,精确)。
        for (u32 k = mainCount; k-- > 0;) {
            d10 += mainDs[k] / mainD11[k];
        }
    }

    // MC 1.21: 插值因子 d16 = (mainResult/10 + 1) / 2,不做预裁剪
    // MC 原版不预裁剪 d16,clampedLerp 内部处理越界情况
    const f64 d16 = (d10 / 10.0 + 1.0) / 2.0;
    const bool flag1 = d16 >= 1.0; // 只采样 minLimitNoise
    const bool flag2 = d16 <= 0.0; // 只采样 maxLimitNoise

    // ---- 第二阶段:minLimitNoise/maxLimitNoise 16 倍频(向量化采样 + 标量短路累加)----
    // 原循环 j=0..15 内 d12/d13/d14 用 d11_j=2^-j(共享 min/max),!flag1/!flag2 标量短路。
    // min/max 各为 PerlinNoise(-15, 16 个全 1.0 振幅) → SoA count = 16。
    // SoA 正向 index k → 原循环 j = count-1-k → d11_k = 2^(k-(count-1))。
    // min/max 同 count(均 16),可同一 k 循环各自独立向量化采样,再反向标量累加(复刻原 j=0..15 顺序)。
    f64 d8 = 0.0; // minLimitNoise 累积值
    f64 d9 = 0.0; // maxLimitNoise 累积值

    const noise::PerlinNoiseSoA& minSoa = m_minLimitNoise->soa();
    const noise::PerlinNoiseSoA& maxSoa = m_maxLimitNoise->soa();
    const u32 minCount = minSoa.count();
    const u32 maxCount = maxSoa.count();

    // 两路都短路时直接跳过采样(flag1 && flag2 理论不可能:d16>=1 与 d16<=0 互斥,但写防御性分支)。
    if (!flag1 || !flag2) {
        alignas(64) f64 minDs[noise::kMaxPerlinOctaves];
        alignas(64) f64 maxDs[noise::kMaxPerlinOctaves];

        // 预计算 d11_k 数组(std::ldexp 不可向量化,放循环外预填)。
        alignas(64) f64 minD11[noise::kMaxPerlinOctaves];
        alignas(64) f64 maxD11[noise::kMaxPerlinOctaves];
        for (u32 k = 0; k < minCount; ++k) {
            minD11[k] = std::ldexp(1.0, static_cast<i32>(k) - static_cast<i32>(minCount - 1));
        }
        for (u32 k = 0; k < maxCount; ++k) {
            maxD11[k] = std::ldexp(1.0, static_cast<i32>(k) - static_cast<i32>(maxCount - 1));
        }

        const u32 minLoop = flag1 ? 0 : minCount;
        const u32 maxLoop = flag2 ? 0 : maxCount;

        // min 采样向量化(flag1 时跳过)。
#pragma clang loop vectorize_width(4) interleave_count(2)
        for (u32 k = 0; k < minLoop; ++k) {
            const f64 d11 = minD11[k];
            const f64 d12 = noise::perlinWrap(d0 * d11);
            const f64 d13 = noise::perlinWrap(d1 * d11);
            const f64 d14 = noise::perlinWrap(d2 * d11);
            const f64 d15 = d6 * d11;
            minDs[k] = noise::perlinSampleSoA(minSoa, k, d12, d13, d14, d15, d1 * d11);
        }

        // max 采样向量化(flag2 时跳过)。
#pragma clang loop vectorize_width(4) interleave_count(2)
        for (u32 k = 0; k < maxLoop; ++k) {
            const f64 d11 = maxD11[k];
            const f64 d12 = noise::perlinWrap(d0 * d11);
            const f64 d13 = noise::perlinWrap(d1 * d11);
            const f64 d14 = noise::perlinWrap(d2 * d11);
            const f64 d15 = d6 * d11;
            maxDs[k] = noise::perlinSampleSoA(maxSoa, k, d12, d13, d14, d15, d1 * d11);
        }

        // 反向标量累加(复刻原 j=0..15 顺序)。
        if (!flag1) {
            for (u32 k = minCount; k-- > 0;) {
                d8 += minDs[k] / minD11[k];
            }
        }
        if (!flag2) {
            for (u32 k = maxCount; k-- > 0;) {
                d9 += maxDs[k] / maxD11[k];
            }
        }
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
