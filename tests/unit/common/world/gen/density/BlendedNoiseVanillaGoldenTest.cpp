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

// ============================================================================
// BlendedNoise（old_blended_noise / base_3d_noise）原版黄金值回归测试
//
// 本文件是**外部真值**门禁：期望值不是从本项目某次运行导出的，而是由一份逐行复刻
// MC 1.21.11 BlendedNoise / PerlinNoise / ImprovedNoise / XoroshiroRandomSource /
// LegacyRandomSource / RandomSupport 的 Java 参考实现独立算出的。因此它能捕捉
// "实现与实现自洽、但与原版不一致" 这类单元测试无法发现的偏差。
//
// ---------------------------------------------------------------------------
// 为什么必须有这份测试
// ---------------------------------------------------------------------------
// 此前 BlendedNoise 的种子派生路径是错的，且**没有任何测试能发现**：
//
//   原版 RandomState.NoiseWiringHelper.wrapNew 的 BlendedNoise 分支：
//     RandomSource randomsource = flag
//         ? this.newLegacyInstance(0L)                                  // LegacyRandomSource(worldSeed)
//         : RandomState.this.random.fromHashOf(Identifier("terrain"));  // Xoroshiro
//     return blendednoise.withNewRandom(randomsource);
//   —— 即把**这个 RandomSource 本身**交给 BlendedNoise，由它顺序构造三层 PerlinNoise。
//
//   项目此前取 `terrainRng->nextLong()` 当种子，再 `new JavaLegacyRandom(seed)`：
//     (a) 丢掉了随机流本身（只用了第一个 long），
//     (b) 换了 RNG 类型（Xoroshiro → LCG）。
//   结果三层 PerlinNoise 的置换表与原点偏移全部错位，base_3d_noise 整体变形。
//   但 DensityAstBaselineTest 的期望值是从这个错误实现导出的，所以它一直"通过"。
//
// flag 即 noise_settings.legacy_random_source：
//   主世界 / 放大化 / 大型群系 → false（Xoroshiro）
//   下界 / 末地 / 洞穴 / 浮岛   → true （Legacy）
// 两种都要覆盖，因为它们的 RNG 类型不同，且 consumeCount 的推进量也不同
// （Xoroshiro 循环 nextLong，Legacy 循环 nextInt）。
//
// 容差取 1e-12（与 DensityAstUlpTest 的 kAbsTolerance 同一预算）：
//   - 本项目 BlendedNoise::compute 走 SoA 向量化路径，与 Java 标量循环的 FMA 收缩/
//     结合顺序不同，实测最大漂移约 2e-15（约 1 ulp 中间量）——属于 DensityAstUlpTest
//     文件头已归档的**已知代码生成差异**，不是实现偏差。
//   - 本测试要防的结构级缺陷（随机源类型错、种子派生错、consumeCount 推进量错）会让
//     置换表/原点偏移整体错位，实测偏差在 1e-6 ~ 1e-5 量级。
//   1e-12 对良性漂移留 500 倍余量，对结构性缺陷仍有 6 个数量级的识别力。
// ============================================================================

#include "common/core/Types.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/PositionalRandomFactory.hpp"
#include "server/world/gen/RandomState.hpp"
#include "server/world/gen/density/BlendedNoise.hpp"
#include "server/world/gen/settings/DimensionSettings.hpp"

#include <array>
#include <gtest/gtest.h>

namespace mc {
namespace {

using world::gen::RandomState;
using world::gen::density::BlendedNoise;

/// 采样点：覆盖原点、海平面、区块边界、负坐标、深层、高空、极端远处。
constexpr std::array<std::array<i32, 3>, 8> kSamplePoints = {{
    {0, 0, 0},
    {8, 64, 8},
    {16, 0, 16},
    {31, 31, 31},
    {-8, 64, -8},
    {64, -32, 64},
    {4, -60, 4},
    {100, 50, -100},
}};

constexpr f64 kTolerance = 1e-12;

/// 单个用例：世界种子 + 8 个采样点的期望密度值（顺序对齐 kSamplePoints）。
struct GoldenCase {
    u64 seed;
    std::array<f64, 8> expected;
};

/**
 * @brief 用给定的 BlendedNoise 逐点采样并与黄金值比对
 */
void expectGolden(const BlendedNoise& noise, const GoldenCase& c, const char* label)
{
    for (size_t i = 0; i < kSamplePoints.size(); ++i) {
        const auto& p = kSamplePoints[i];
        const f64 actual = noise.compute(p[0], p[1], p[2]);
        EXPECT_NEAR(actual, c.expected[i], kTolerance)
            << label << " seed=" << c.seed << " point#" << i << " (" << p[0] << "," << p[1] << "," << p[2]
            << ") 与原版黄金值不一致：actual=" << actual << " expected=" << c.expected[i];
    }
}

// ============================================================================
// 主世界：legacy_random_source=false → terrain 随机源是 Xoroshiro
// ============================================================================

TEST(BlendedNoiseVanillaGoldenTest, OverworldMatchesVanilla)
{
    // 参数取自数据包 overworld/base_3d_noise.json
    const std::vector<GoldenCase> cases = {
        {0ULL,
            {0.022583846157211124,
                0.16443276512080698,
                0.038480084517568690,
                0.054532067917451514,
                0.055912701095427090,
                -0.045159553908039330,
                0.19250803164444363,
                0.16533874044498756}},
        {1ULL,
            {-0.0042692465589890816,
                -0.019750552516033956,
                0.020657370407656153,
                0.080281038268483150,
                -0.11213081052612330,
                -0.029029122239448923,
                0.031327161883376975,
                0.099258899361662070}},
        {42ULL,
            {0.28183935328731446,
                -0.20532337469007456,
                -0.027254032721566220,
                0.28502933711140005,
                0.42749796280288810,
                -0.022915539328524248,
                0.24481358277210480,
                0.20946791887582340}},
        {12345ULL,
            {-0.12566992911603778,
                -0.11608215172825060,
                0.051155471625478610,
                -0.012343938224455522,
                -0.16224138681864342,
                0.55701541728713170,
                -0.20879080912353373,
                0.13498714133036777}},
        {static_cast<u64>(-6671478382168981129LL),
            {-0.11586262014179928,
                0.19210594463933300,
                0.15595991153518780,
                0.23545225518074240,
                0.10714656614982102,
                0.68134007207096700,
                0.92110936553225900,
                -0.10676482374244880}},
    };

    for (const auto& c : cases) {
        // 走生产同一条路径：RandomState.positionalRandom().fromHashOf("minecraft:terrain")
        // 得到的 Xoroshiro 随机源**本身**（不是它的 nextLong()）交给 BlendedNoise。
        auto state = RandomState::create(DimensionSettings::overworld(), c.seed);
        ASSERT_NE(state, nullptr) << "seed=" << c.seed << ": RandomState::create 返回空";
        auto terrainRng = state->positionalRandom().fromHashOf("minecraft:terrain");
        const BlendedNoise noise(*terrainRng, 0.25, 0.125, 80.0, 160.0, 8.0);
        expectGolden(noise, c, "overworld");
    }
}

// ============================================================================
// 下界：legacy_random_source=true → newLegacyInstance(0L) = LegacyRandomSource(worldSeed)
// ============================================================================

TEST(BlendedNoiseVanillaGoldenTest, NetherMatchesVanilla)
{
    // 参数取自数据包 nether/base_3d_noise.json
    const std::vector<GoldenCase> cases = {
        {0ULL,
            {0.31376903275900230,
                0.069004125056169260,
                0.24473646778078384,
                0.38942882533666956,
                0.10529932051177275,
                0.22242898971904390,
                0.043598123835915270,
                0.18202281779547203}},
        {1ULL,
            {-0.15508925525877312,
                -0.21967202845302940,
                0.0057995574309877150,
                0.035220220756165710,
                -0.38863022118508000,
                -0.60097159150365130,
                -0.29700848560527970,
                0.12771889763417443}},
        {42ULL,
            {0.17498688498112133,
                0.13316806560912497,
                0.29983960718380530,
                0.16795732932100188,
                0.33759490753708754,
                0.22128755645034837,
                0.012785737085114438,
                0.096117945816329570}},
        {12345ULL,
            {0.23942730205267540,
                0.18708427854670545,
                0.24854284108481212,
                0.22541005776585143,
                0.14405526722215320,
                -0.23961883735477910,
                -0.22386712830583824,
                0.043364960028138194}},
        {static_cast<u64>(-6671478382168981129LL),
            {-0.096629543658123010,
                -0.25970069515091926,
                -0.028408437843682424,
                0.0060390367966306760,
                -0.17476824622086443,
                -0.13116629581416378,
                -0.31798686518422814,
                0.0083860356907997090}},
    };

    for (const auto& c : cases) {
        // 原版 newLegacyInstance(0L) = new LegacyRandomSource(worldSeed + 0)
        math::JavaLegacyRandom rng(c.seed);
        const BlendedNoise noise(rng, 0.25, 0.375, 80.0, 60.0, 8.0);
        expectGolden(noise, c, "nether");
    }
}

// ============================================================================
// 末地：legacy_random_source=true → LegacyRandomSource(worldSeed)
// ============================================================================

TEST(BlendedNoiseVanillaGoldenTest, EndMatchesVanilla)
{
    // 参数取自数据包 end/base_3d_noise.json
    const std::vector<GoldenCase> cases = {
        {0ULL,
            {0.31376903275900230,
                0.15354316947770852,
                0.24473646778078384,
                0.11993470428616937,
                0.10878300999963493,
                0.10343512750676108,
                0.028161882034073243,
                0.47532181767668790}},
        {1ULL,
            {-0.15508925525877312,
                -0.10035429468230178,
                0.0057995574309877150,
                -0.056658094558585980,
                -0.23151350810893767,
                -0.51118716492152140,
                -0.081872023836088520,
                0.25766168760022884}},
        {42ULL,
            {0.17498688498112133,
                -0.14792388290094640,
                0.29983960718380530,
                0.33999109391401866,
                0.074649413656331260,
                0.28939942197749460,
                0.18630498309560200,
                -0.096354758907437940}},
        {12345ULL,
            {0.23942730205267540,
                0.18185546567660604,
                0.24854284108481212,
                0.17652031425265668,
                0.27151568818439420,
                0.24268451985293013,
                0.39331781935685994,
                0.055168527912335310}},
        {static_cast<u64>(-6671478382168981129LL),
            {-0.096629543658123010,
                -0.066647865987784120,
                -0.028408437843682424,
                -0.011105567069515043,
                -0.14937316851740723,
                -0.043059519501978780,
                -0.54128484820573470,
                0.23032557065191828}},
    };

    for (const auto& c : cases) {
        math::JavaLegacyRandom rng(c.seed);
        const BlendedNoise noise(rng, 0.25, 0.25, 80.0, 160.0, 4.0);
        expectGolden(noise, c, "end");
    }
}

} // namespace
} // namespace mc
