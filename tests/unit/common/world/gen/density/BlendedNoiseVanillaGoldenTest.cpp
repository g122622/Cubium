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
    // 期望值由原版 1.21.11 反混淆 jar 直接反射调用 BlendedNoise.compute 产出
    // （见 tests/diff 的 Java oracle 做法），非本项目实现导出。
    const std::vector<GoldenCase> cases = {
        {0ULL,
            {0.022578701874563640,
                0.16442767702437316,
                0.038475864898372264,
                0.054528414048208090,
                0.055908642859561610,
                -0.045159593088048070,
                0.19249562697541080,
                0.16533790809860982}},
        {1ULL,
            {-0.0042688671701099290,
                -0.019751077449583168,
                0.020658119787915820,
                0.080283572855038660,
                -0.11212823196206523,
                -0.029034659002976304,
                0.031326735742376490,
                0.099260612282676500}},
        {42ULL,
            {0.28183906987154830,
                -0.20532239479236078,
                -0.027251686628652655,
                0.28502065721090940,
                0.42749716536331794,
                -0.022913515377939758,
                0.24481389758314798,
                0.20946615618139486}},
        {12345ULL,
            {-0.12566864434717037,
                -0.11608136337759956,
                0.051152844374942230,
                -0.012344608512175850,
                -0.16223873136901842,
                0.55701443812542970,
                -0.20879036769638760,
                0.13498415639640415}},
        {static_cast<u64>(-6671478382168981129LL),
            {-0.11586198005705065,
                0.19210302387707426,
                0.15595459927642838,
                0.23544844637272877,
                0.10714348331082058,
                0.68133897481991550,
                0.92110769378614600,
                -0.10676743584475613}},
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
    // 期望值由原版 1.21.11 反混淆 jar 反射调用 BlendedNoise.compute 产出。
    const std::vector<GoldenCase> cases = {
        {0ULL,
            {0.31377066460601910,
                0.069004046472004870,
                0.24473905312264770,
                0.38943558734525380,
                0.10530479216945227,
                0.22242741812482236,
                0.043595538314607800,
                0.18202118316739557}},
        {1ULL,
            {-0.15508627339567538,
                -0.21967061270265137,
                0.0058057486299972620,
                0.035227845740948170,
                -0.38863114794263864,
                -0.60097208048283060,
                -0.29700758014258610,
                0.12771569502509544}},
        {42ULL,
            {0.17498762833457276,
                0.13316695826748530,
                0.29984288064148590,
                0.16795867461020011,
                0.33759699503721136,
                0.22128440244426448,
                0.012784685364918953,
                0.096120375529751940}},
        {12345ULL,
            {0.23942659667784655,
                0.18707917137790545,
                0.24854081314574167,
                0.22540855122790202,
                0.14405217140220420,
                -0.23962042876970210,
                -0.22386745248458934,
                0.043368847586044050}},
        {static_cast<u64>(-6671478382168981129LL),
            {-0.096625182369393050,
                -0.25969953343332647,
                -0.028404208444151424,
                0.0060393835329964896,
                -0.17476803253068202,
                -0.13116608746362018,
                -0.31798691172241000,
                0.0083855568812571390}},
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
    // 期望值由原版 1.21.11 反混淆 jar 反射调用 BlendedNoise.compute 产出。
    const std::vector<GoldenCase> cases = {
        {0ULL,
            {0.31377066460601910,
                0.15354575942905713,
                0.24473905312264770,
                0.11993750292954392,
                0.10878533638976347,
                0.10342872172871452,
                0.028156605814881747,
                0.47531794120291293}},
        {1ULL,
            {-0.15508627339567538,
                -0.10034875951231918,
                0.0058057486299972620,
                -0.056650442612461520,
                -0.23150750674230056,
                -0.51118570692809650,
                -0.081854815086991550,
                0.25765902834326970}},
        {42ULL,
            {0.17498762833457276,
                -0.14791970177008748,
                0.29984288064148590,
                0.33999150845139026,
                0.074649353551522160,
                0.28939672592956760,
                0.18629529028919090,
                -0.096357405999941080}},
        {12345ULL,
            {0.23942659667784655,
                0.18185158956510863,
                0.24854081314574167,
                0.17651802014893830,
                0.27151399303062830,
                0.24268859229748380,
                0.39332395253981520,
                0.055169694313253580}},
        {static_cast<u64>(-6671478382168981129LL),
            {-0.096625182369393050,
                -0.066647214365412920,
                -0.028404208444151424,
                -0.011104420083255795,
                -0.14937326187161626,
                -0.043059930806650220,
                -0.54128385131420130,
                0.23032447612640983}},
    };

    for (const auto& c : cases) {
        math::JavaLegacyRandom rng(c.seed);
        const BlendedNoise noise(rng, 0.25, 0.25, 80.0, 160.0, 4.0);
        expectGolden(noise, c, "end");
    }
}

} // namespace
} // namespace mc
