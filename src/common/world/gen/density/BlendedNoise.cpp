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
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/world/gen/noise/PerlinNoise.hpp"
#include "common/world/gen/noise/PerlinSoA.hpp"
#include <array>
#include <cstddef>
#include <memory>
#include <string>
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
    buildSoA();
}

namespace {

/// 按 getOctaveNoise(i) 顺序（i=0 最高频）收集 PerlinNoise 的非空 octave 为 SoA。
/// d11 从 1.0 起每层 /=2（BlendedNoise 的每层缩放因子，同时用于坐标缩放与结果除法），
/// 存入 PerlinSoALayer::inputFactor 字段（BlendedNoise 复用该字段存 d11，amplitude/valueFactor 不用）。
/// 空层（getOctaveNoise 返回 nullptr）跳过但仍推进 d11（与原 compute 循环语义一致，bit-exact）。
void collectBlendedSoA(const noise::PerlinNoise& noise, i32 octaveCount, std::vector<noise::PerlinSoALayer>& out)
{
    out.clear();
    f64 d11 = 1.0;
    for (i32 i = 0; i < octaveCount; ++i) {
        const noise::PerlinNoise::PerlinLayer* layer = noise.getOctaveNoise(i);
        if (layer != nullptr) {
            noise::PerlinSoALayer soa;
            std::copy_n(layer->permutation().data(), 256, soa.permutation.data());
            soa.originX = layer->xOffset();
            soa.originY = layer->yOffset();
            soa.originZ = layer->zOffset();
            soa.inputFactor = d11; // 复用 inputFactor 字段存 d11
            out.push_back(soa);
        }
        d11 /= 2.0;
    }
}

} // namespace

void BlendedNoise::buildSoA()
{
    // main 8 倍频（-7..0），min/max 各 16 倍频（-15..0），均全 1.0 振幅故无空层。
    collectBlendedSoA(*m_mainNoise, 8, m_mainSoA);
    collectBlendedSoA(*m_minLimitNoise, 16, m_minLimitSoA);
    collectBlendedSoA(*m_maxLimitNoise, 16, m_maxLimitSoA);
    // min/max 必须等长且 mulFactor 对齐（同 firstOctave + 全 1.0 振幅），compute 合并循环依赖此。
    MC_ASSERT_RELEASE_MSG(m_minLimitSoA.size() == m_maxLimitSoA.size(),
        "BlendedNoise: minLimit/maxLimit octave count mismatch (compute 合并循环要求等长)");
}

// ============================================================================
// compute — 核心密度计算
// ============================================================================

f64 BlendedNoise::compute(i32 blockX, i32 blockY, i32 blockZ) const
{
    // SoA 求值：遍历预拍平的 main/min/max SoA 数组，调 perlinSample 替代 noiseWithSmear。
    // 累加顺序、d11 序列、涂抹参数（yScale/yMax）、短路 flag 与原 compute 完全一致（bit-exact）。
    // min/max 合并循环共享 d12/d13/d14/yScale/yMax（同 d11，仅 layer 不同），对齐原合并循环结构。
    const f64 xzMultiplier = m_xzMultiplier;
    const f64 yMultiplier = m_yMultiplier;

    const f64 d0 = static_cast<f64>(blockX) * xzMultiplier;
    const f64 d1 = static_cast<f64>(blockY) * yMultiplier;
    const f64 d2 = static_cast<f64>(blockZ) * xzMultiplier;

    const f64 d3 = d0 / m_xzFactor; // main 层 xz 坐标
    const f64 d4 = d1 / m_yFactor;  // main 层 y 坐标
    const f64 d5 = d2 / m_xzFactor; // main 层 xz 坐标

    const f64 d6 = yMultiplier * m_smearScaleMultiplier; // min/max 层涂抹因子
    const f64 d7 = d6 / m_yFactor;                       // main 层涂抹因子

    // ---- 第一阶段：mainNoise 8 倍频，计算插值因子 ----
    f64 d10 = 0.0; // mainNoise 累积值
    for (const auto& layer : m_mainSoA) {
        const f64 d11 = layer.inputFactor; // d11 = 1.0/2^i
        const f64 nx = noise::PerlinNoise::wrap(d3 * d11);
        const f64 ny = noise::PerlinNoise::wrap(d4 * d11);
        const f64 nz = noise::PerlinNoise::wrap(d5 * d11);
        const f64 sampled = noise::perlinSample(layer.permutation.data(),
            layer.originX,
            layer.originY,
            layer.originZ,
            nx,
            ny,
            nz,
            d7 * d11,  // yScale（涂抹间隔）
            d4 * d11); // yMax（原始 Y 分数）
        d10 += sampled / d11;
    }

    // 插值因子 d16 = (mainResult/10 + 1) / 2，不做预裁剪（clampedLerp 处理越界）
    const f64 d16 = (d10 / 10.0 + 1.0) / 2.0;
    const bool flag1 = d16 >= 1.0; // 只采样 minLimitNoise
    const bool flag2 = d16 <= 0.0; // 只采样 maxLimitNoise

    // ---- 第二阶段：minLimitNoise 和 maxLimitNoise 16 倍频（合并循环，共享坐标）----
    f64 d8 = 0.0; // minLimitNoise 累积值
    f64 d9 = 0.0; // maxLimitNoise 累积值
    const size_t limitCount = m_minLimitSoA.size();
    for (size_t k = 0; k < limitCount; ++k) {
        const f64 d11 = m_minLimitSoA[k].inputFactor; // min/max 同 d11（等长对齐）
        const f64 d12 = noise::PerlinNoise::wrap(d0 * d11);
        const f64 d13 = noise::PerlinNoise::wrap(d1 * d11);
        const f64 d14 = noise::PerlinNoise::wrap(d2 * d11);
        const f64 yScale = d6 * d11; // 涂抹间隔
        const f64 yMax = d1 * d11;   // 原始 Y 分数

        if (!flag1) {
            const auto& layer = m_minLimitSoA[k];
            d8 += noise::perlinSample(layer.permutation.data(),
                      layer.originX,
                      layer.originY,
                      layer.originZ,
                      d12,
                      d13,
                      d14,
                      yScale,
                      yMax) /
                d11;
        }

        if (!flag2) {
            const auto& layer = m_maxLimitSoA[k];
            d9 += noise::perlinSample(layer.permutation.data(),
                      layer.originX,
                      layer.originY,
                      layer.originZ,
                      d12,
                      d13,
                      d14,
                      yScale,
                      yMax) /
                d11;
        }
    }

    // ---- 最终结果 ----
    // clampedLerp(d16, minResult/512, maxResult/512) / 128
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
