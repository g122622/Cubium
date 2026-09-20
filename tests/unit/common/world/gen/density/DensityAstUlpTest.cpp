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

/**
 * @file DensityAstUlpTest.cpp
 * @brief 噪声采样层 SoA 向量化路径 vs 标量 reference 的数值漂移监控测试
 *
 * 配套 PerlinNoiseSoA 引入的 SIMD 加速(效仿 C2ME c2me-opts-natives-math 的 octave 并行)。
 * 本测试对照 SoA 向量化路径(PerlinNoise::getValue 走 SoA、perlinSampleSoA 内核、
 * BlendedNoise::compute 走 SoA)与标量 reference(PerlinLayer::noise/noiseWithSmear 逐层循环),
 * 定位倍频索引错位 / 累加顺序错 / 缩放因子错 / epsilon 陷阱等结构级隐患。
 *
 * 三档覆盖:
 * 1. PerlinNoise::getValue SoA vs 标量(无涂抹,NormalNoise 路径)——验证 octave 循环向量化
 *    后采样写扁平数组 + 标量累加与逐层循环一致。多 octave 喂饱 SIMD f64 通道。
 * 2. 涂抹内核 perlinSampleSoA(yScale!=0) vs PerlinLayer::noiseWithSmear——验证 epsilon
 *    static_cast<f64>(1.0e-7f) 陷阱(1.0e-7 double 与 1.0e-7f→double 不同,边界 floor 跨越
 *    会导致 smearOffset 差一个 yScale 量级)。
 * 3. BlendedNoise::compute SoA vs 标量重建——验证 min/max 反向索引 + d11=2^k 缩放序列
 *    + flag1/flag2 短路的 SoA 路径与逐层标量 compute 一致。
 *
 * 判据:绝对误差 ≤ kAbsTolerance(1e-12)。
 *
 * 为什么不用 ULP 距离作判据:两条路径的**算法**逐项一致(同一表达式、同一累加顺序、
 * 同一舍入点),但项目全局开启 -ffast-math(-ffp-contract=fast),clang 对 SoA 向量化
 * 循环与标量 SLP 向量化的**收缩 FMA / 结合顺序**决策可以不同,这属于编译器代码生成
 * 差异而非算法差异:
 *   - Windows clang-cl(x86-64 AVX2)实测 0 ULP(两条路径代码生成恰好一致);
 *   - macOS arm64 + Apple clang 21 实测档1 最大 252 ULP、档2 最大 778 ULP、档3 最大 131 ULP,
 *     但绝对差恒 ≤ 5e-16(≈1 ulp of 量级 O(1) 的中间量)。
 * ULP 距离以结果量级折算,而噪声结果经常因抵消而接近 0,此时 1 ulp 的中间量漂移会被
 * 放大成上万 ULP,不适合直接作为跨平台判据(故仅作为诊断量 RecordProperty 记录)。
 * 绝对误差则不受抵消影响:结构级缺陷(倍频索引错位、d11=2^k 序列错、epsilon 取双精度
 * 1.0e-7 而非 1.0e-7f)会产生 ≥1e-3 的绝对偏差,与 1e-12 的预算相差 9 个数量级,
 * 该判据仍能可靠捕获所有已知缺陷形态。
 */

#include "common/core/Types.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "server/world/gen/density/BlendedNoise.hpp"
#include "server/world/gen/noise/NormalNoise.hpp"
#include "server/world/gen/noise/PerlinNoise.hpp"
#include "server/world/gen/noise/PerlinNoiseSoA.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace mc {
namespace {

using namespace world::gen::noise;
using world::gen::density::BlendedNoise;

/// 标量 reference:复刻原 PerlinNoise::getValue(回退前)逐层循环。
/// 最低频起第 i 个非空层,调 PerlinLayer::noise(无涂抹)。累加顺序 i=0..N-1。
[[nodiscard]] f64 perlinNoiseGetValueScalar(const PerlinNoise& noise, f64 x, f64 y, f64 z)
{
    f64 result = 0.0;
    f64 inputFactor = noise.lowestFreqInputFactor();
    f64 valueFactor = noise.lowestFreqValueFactor();
    const auto& layers = noise.layers();
    const auto& amplitudes = noise.amplitudesVec();
    for (size_t i = 0; i < layers.size(); ++i) {
        if (layers[i] != nullptr) {
            const f64 nx = PerlinNoise::wrap(x * inputFactor);
            const f64 ny = PerlinNoise::wrap(y * inputFactor);
            const f64 nz = PerlinNoise::wrap(z * inputFactor);
            result += amplitudes[i] * layers[i]->noise(nx, ny, nz) * valueFactor;
        }
        inputFactor *= 2.0;
        valueFactor /= 2.0;
    }
    return result;
}

// ============================================================================
// 档1:PerlinNoise::getValue(SoA 向量化)vs 标量逐层累加(无涂抹,NormalNoise 路径)
// ============================================================================

/// 判据预算:绝对误差上限。噪声值量级 O(1),编译器 FMA 收缩/结合顺序差异带来的
/// 漂移绝对量级恒 ≤ 5e-16(≈1 ulp of 中间量);结构级缺陷则 ≥1e-3。取 1e-12:
/// 对已知代码生成差异留 ~2000 倍余量,同时对结构级缺陷仍保持 9 个数量级的识别力。
inline constexpr f64 kAbsTolerance = 1e-12;

/// 多组 firstOctave/amplitudes 覆盖不同 octave 数(喂饱 SIMD f64 通道 + 留余量)。
/// 原版 JAGGED 为 firstOctave=-16 且 16 个 1.0 振幅(见 jagged.json),BlendedNoise
/// main 8 / min&max 16,故覆盖 4/8/16/17。
struct PerlinNoiseUlpCase {
    u64 seed;
    i32 firstOctave;
    std::vector<f64> amplitudes;
    const char* label;
};

const std::vector<PerlinNoiseUlpCase>& perlinUlpCases()
{
    static const std::vector<PerlinNoiseUlpCase> cases = {
        {0ULL, -2, {1.0, 1.0, 1.0, 1.0}, "4-octave"},
        {12345ULL, -7, std::vector<f64>(8, 1.0), "8-octave"},
        {999ULL, -15, std::vector<f64>(16, 1.0), "16-octave"},
        {42ULL, -16, std::vector<f64>(16, 1.0), "16-octave(JAGGED)"},
        {42ULL, -8, std::vector<f64>(17, 1.0), "17-octave"},
        // 含零振幅层(空层跳过采样但推进缩放序列),验证 buildSoA 的 inputFactor/valueFactor 推进
        {7ULL, -3, {1.0, 0.0, 1.0, 0.0, 1.0}, "5-octave-with-zeros"},
    };
    return cases;
}

TEST(DensityAstUlpTest, PerlinNoiseGetValueSoAVsScalar)
{
    i64 maxUlp = 0;
    f64 maxAbsDiff = 0.0;
    for (const auto& c : perlinUlpCases()) {
        const PerlinNoise noise(c.seed, c.firstOctave, c.amplitudes);
        for (int s = 0; s < 40; ++s) {
            const f64 x = static_cast<f64>(s * 17.3) + 0.5;
            const f64 y = static_cast<f64>(s * 31.7) - 2.25;
            const f64 z = static_cast<f64>(s * 53.1) + 7.75;
            const f64 soa = noise.getValue(x, y, z);
            const f64 scalar = perlinNoiseGetValueScalar(noise, x, y, z);
            const i64 ulp = world::gen::noise::ulpDistance(soa, scalar);
            maxUlp = std::max(maxUlp, ulp);
            maxAbsDiff = std::max(maxAbsDiff, std::abs(soa - scalar));
            EXPECT_LE(std::abs(soa - scalar), kAbsTolerance)
                << c.label << " sample#" << s << " SoA=" << soa << " scalar=" << scalar << " ulp=" << ulp;
        }
    }
    // ULP 距离仅作诊断记录(近零结果会放大 ULP,不适合作跨平台判据;判据见文件头注释)。
    RecordProperty("maxUlp", std::to_string(maxUlp));
    RecordProperty("maxAbsDiff", std::to_string(maxAbsDiff));
}

// ============================================================================
// 档2:涂抹内核 perlinSampleSoA(yScale!=0)vs PerlinLayer::noiseWithSmear
// 验证 epsilon static_cast<f64>(1.0e-7f) 陷阱修复(BlendedNoise Y 涂抹路径)。
// ============================================================================

TEST(DensityAstUlpTest, PerlinSampleSoASmearKernelVsLayerNoiseWithSmear)
{
    i64 maxUlp = 0;
    f64 maxAbsDiff = 0.0;
    // 多 seed 构造 PerlinNoise,取其 SoA 与首层 PerlinLayer 对照涂抹内核。
    for (const auto& c : perlinUlpCases()) {
        const PerlinNoise noise(c.seed, c.firstOctave, c.amplitudes);
        const auto& soa = noise.soa();
        if (soa.count() == 0) {
            continue;
        }
        // 对照 SoA index k 对应的 PerlinLayer:SoA index k = 最低频起第 k 个非空层。
        // 遍历每个 SoA octave 单独验证内核(yScale!=0 涂抹)。
        const auto& layers = noise.layers();
        std::vector<const PerlinNoise::PerlinLayer*> nonNullLayers;
        for (const auto& layer : layers) {
            if (layer != nullptr) {
                nonNullLayers.push_back(layer.get());
            }
        }
        ASSERT_EQ(nonNullLayers.size(), soa.count());

        for (int s = 0; s < 30; ++s) {
            // 涂抹参数仿 BlendedNoise:d11=2^-i, yScale=d7*d11, yMax=d4*d11。
            // 这里直接构造一组覆盖边界的 yScale/yMax,重点测 base/yMax<fracY 分支与 floor 跨越。
            const f64 baseY = static_cast<f64>(s) * 0.137 + 0.03;
            const f64 x = static_cast<f64>(s * 1.7);
            const f64 z = static_cast<f64>(s * 2.3);
            // yScale 取小值使 base/yScale+epsilon 边界密集,放大 epsilon 差异影响。
            const f64 yScale = 0.25 + 0.01 * static_cast<f64>(s % 7);
            for (u32 k = 0; k < soa.count(); ++k) {
                const f64 yMax = baseY; // 控制 yMax<fracY 分支
                const f64 soaVal = world::gen::noise::perlinSampleSoA(soa, k, x, baseY, z, yScale, yMax);
                const f64 layerVal = nonNullLayers[k]->noiseWithSmear(x, baseY, z, yScale, yMax);
                const i64 ulp = world::gen::noise::ulpDistance(soaVal, layerVal);
                maxUlp = std::max(maxUlp, ulp);
                maxAbsDiff = std::max(maxAbsDiff, std::abs(soaVal - layerVal));
                EXPECT_LE(std::abs(soaVal - layerVal), kAbsTolerance)
                    << c.label << " oct#" << k << " sample#" << s << " soa=" << soaVal << " layer=" << layerVal
                    << " ulp=" << ulp;
            }
        }
    }
    RecordProperty("maxUlp", std::to_string(maxUlp));
    RecordProperty("maxAbsDiff", std::to_string(maxAbsDiff));
}

// ============================================================================
// 档3:BlendedNoise::compute(SoA 路径)vs 标量重建 compute
// 验证 min/max 反向索引 + d11=2^k 缩放序列 + flag1/flag2 短路 SoA 路径与标量一致。
// ============================================================================

/// 标量 reference:复刻原 BlendedNoise::compute(回退前)逐层循环,调 getOctaveNoise + noiseWithSmear。
/// 独立于 BlendedNoise::compute 的 SoA 实现,用作 ULP 对照 ground truth。
[[nodiscard]] f64 blendedNoiseComputeScalar(const BlendedNoise& noise,
    f64 xzMultiplier,
    f64 yMultiplier,
    f64 xzFactor,
    f64 yFactor,
    f64 smearScaleMultiplier,
    i32 blockX,
    i32 blockY,
    i32 blockZ,
    const PerlinNoise& minLimit,
    const PerlinNoise& maxLimit,
    const PerlinNoise& mainNoise)
{
    const f64 d0 = static_cast<f64>(blockX) * xzMultiplier;
    const f64 d1 = static_cast<f64>(blockY) * yMultiplier;
    const f64 d2 = static_cast<f64>(blockZ) * xzMultiplier;
    const f64 d3 = d0 / xzFactor;
    const f64 d4 = d1 / yFactor;
    const f64 d5 = d2 / xzFactor;
    const f64 d6 = yMultiplier * smearScaleMultiplier;
    const f64 d7 = d6 / yFactor;

    f64 d10 = 0.0;
    f64 d11 = 1.0;
    for (i32 i = 0; i < 8; ++i) {
        const PerlinNoise::PerlinLayer* layer = mainNoise.getOctaveNoise(i);
        if (layer != nullptr) {
            const f64 nx = PerlinNoise::wrap(d3 * d11);
            const f64 ny = PerlinNoise::wrap(d4 * d11);
            const f64 nz = PerlinNoise::wrap(d5 * d11);
            d10 += layer->noiseWithSmear(nx, ny, nz, d7 * d11, d4 * d11) / d11;
        }
        d11 /= 2.0;
    }
    const f64 d16 = (d10 / 10.0 + 1.0) / 2.0;
    const bool flag1 = d16 >= 1.0;
    const bool flag2 = d16 <= 0.0;

    f64 d8 = 0.0;
    f64 d9 = 0.0;
    d11 = 1.0;
    for (i32 j = 0; j < 16; ++j) {
        const f64 d12 = PerlinNoise::wrap(d0 * d11);
        const f64 d13 = PerlinNoise::wrap(d1 * d11);
        const f64 d14 = PerlinNoise::wrap(d2 * d11);
        const f64 d15 = d6 * d11;
        if (!flag1) {
            const PerlinNoise::PerlinLayer* layer = minLimit.getOctaveNoise(j);
            if (layer != nullptr) {
                d8 += layer->noiseWithSmear(d12, d13, d14, d15, d1 * d11) / d11;
            }
        }
        if (!flag2) {
            const PerlinNoise::PerlinLayer* layer = maxLimit.getOctaveNoise(j);
            if (layer != nullptr) {
                d9 += layer->noiseWithSmear(d12, d13, d14, d15, d1 * d11) / d11;
            }
        }
        d11 /= 2.0;
    }

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

TEST(DensityAstUlpTest, BlendedNoiseComputeSoAVsScalar)
{
    i64 maxUlp = 0;
    f64 maxAbsDiff = 0.0;
    // 三维度 BlendedNoise 参数(主世界/下界/末地),多 seed。
    struct BlendedCase {
        f64 xzScale, yScale, xzFactor, yFactor, smearScaleMultiplier;
        const char* label;
    };
    const std::vector<BlendedCase> bcases = {
        {0.25, 0.125, 80.0, 160.0, 8.0, "overworld"},
        {0.25, 0.375, 80.0, 60.0, 8.0, "nether"},
        {0.25, 0.25, 80.0, 160.0, 4.0, "end"},
    };
    const std::vector<u64> seeds = {0ULL, 1ULL, 42ULL, 12345ULL};

    for (const auto& bc : bcases) {
        for (const u64 seed : seeds) {
            // BlendedNoise(seed,...) 内部用 JavaLegacyRandom 顺序构造 min/max/main。
            // 标量重建须用同 rng 序列构造三个独立的 PerlinNoise 复刻相同置换表。
            math::JavaLegacyRandom rng(seed);
            const auto rebuildAmplitudes = [](i32 first, i32 last) {
                return std::vector<f64>(static_cast<size_t>(last - first + 1), 1.0);
            };
            PerlinNoise minLimit(rng, -15, rebuildAmplitudes(-15, 0));
            PerlinNoise maxLimit(rng, -15, rebuildAmplitudes(-15, 0));
            PerlinNoise mainNoise(rng, -7, rebuildAmplitudes(-7, 0));

            const BlendedNoise noise(seed, bc.xzScale, bc.yScale, bc.xzFactor, bc.yFactor, bc.smearScaleMultiplier);
            const f64 xzMultiplier = 684.412 * bc.xzScale;
            const f64 yMultiplier = 684.412 * bc.yScale;

            for (int s = 0; s < 20; ++s) {
                const i32 bx = s * 7 - 3;
                const i32 by = s * 13 + 32;
                const i32 bz = s * 11 - 5;
                const f64 soa = noise.compute(bx, by, bz);
                const f64 scalar = blendedNoiseComputeScalar(noise,
                    xzMultiplier,
                    yMultiplier,
                    bc.xzFactor,
                    bc.yFactor,
                    bc.smearScaleMultiplier,
                    bx,
                    by,
                    bz,
                    minLimit,
                    maxLimit,
                    mainNoise);
                const i64 ulp = world::gen::noise::ulpDistance(soa, scalar);
                maxUlp = std::max(maxUlp, ulp);
                maxAbsDiff = std::max(maxAbsDiff, std::abs(soa - scalar));
                EXPECT_LE(std::abs(soa - scalar), kAbsTolerance)
                    << bc.label << " seed=" << seed << " sample#" << s << " soa=" << soa << " scalar=" << scalar
                    << " ulp=" << ulp;
            }
        }
    }
    RecordProperty("maxUlp", std::to_string(maxUlp));
    RecordProperty("maxAbsDiff", std::to_string(maxAbsDiff));
}

} // namespace
} // namespace mc
