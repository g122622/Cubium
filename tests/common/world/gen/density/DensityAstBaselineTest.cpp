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
 * @file DensityAstBaselineTest.cpp
 * @brief 完整 NoiseChunk finalDensity 多点采样固定基线测试（AST 编译器优化的数值回归门禁）
 *
 * 效仿 C2ME DFC 的 AST 编译器优化（维度级编译 + 区块级 Marker 原地替换 + 结构去重）
 * 将重构密度函数树求值路径。本测试在重构前用当时代码（5e8b9b074，原始 mapAll 两步法：
 * createRouterCopy + NoiseChunk::apply mapAll）采集"完整 NoiseChunk（createRouterCopy +
 * Beardifier 叠加 + mapAll 替换全部 Marker）"的 finalDensity 多点采样值，硬编码为基线。
 * 重构后（方案X：维度级编译 + 区块级 newInstance + OOP 组装 Beardifier）全程必须继续
 * 通过——这是整个 AST 编译器工程的数值回归门禁（地形命脉）。
 *
 * 采样协议复刻 iterateNoiseColumn（NoiseChunkGenerator.cpp:771-791）：
 *   cellCountXZ=1 + BeardifierMarker 空占位（Beardifier 贡献为 0，数值可复现），
 *   cellWidth=noise.sizeHorizontal*4 / cellHeight=noise.sizeVertical*4（与生成器一致）。
 *   并复刻其完整初始化序列 initializeForFirstCellX() + advanceCellX(0)。
 *
 * sampleFinalDensity 直接调 m_router.finalDensity().compute()，走委托路径：
 *   未进入插值循环时，CellCache(m_filled=false)/NoiseInterpolator(m_valueReady=false)/
 *   CacheOnce(interpolating()=false) 全部委托原始 filler，FlatCache 构造期已预计算。
 *   因此采样值纯由密度函数树拓扑决定，与插值器状态无关——这正是 SharedTopologyParityTest
 *   采用的稳定基线形式，且能覆盖 mapAll 替换后的 FlatCache/Cache2D/CacheOnce/CellCache/
 *   Interpolated/BeardifierMarker 全路径。
 *
 * 容差 1e-9：密度值量级在 [-0.5, 0.5]，1e-9 足以捕捉任何真实拓扑回归，又允许优化 pass
 * 常量折叠引起的 ULP 级浮点漂移。
 *
 * 依赖 tests/main.cpp 的 WorldGenRegistryEnvironment 预加载 NoiseSettingsRegistry 等数据包。
 */

#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/density/Beardifier.hpp"
#include "common/world/gen/density/DensityFunctions.hpp"
#include "common/world/gen/density/NoiseChunk.hpp"
#include "common/world/gen/density/NoiseRouter.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"

#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

using namespace mc;
using namespace mc::world::gen::density;

namespace {

/// 采样点：覆盖区块内多个相对位置、负坐标、多 Y 层（含地下/海平面/高空）。
/// 顺序与下方各维度多 seed 的硬编码期望值数组一一对应。
const std::vector<std::tuple<i32, i32, i32>>& baselineSamplePoints()
{
    static const std::vector<std::tuple<i32, i32, i32>> points = {
        {0, 0, 0},     // 0: 原点
        {8, 64, 8},    // 1: 主世界海平面附近
        {16, 0, 16},   // 2: 区块边界
        {31, 31, 31},  // 3: 区块内偏角
        {-8, 64, -8},  // 4: 负坐标
        {64, -32, 64}, // 5: 深层
        {3, 100, 3},   // 6: 高空
        {12, 320, 12}, // 7: 主世界顶
        {4, -60, 4},   // 8: 主世界底
    };
    return points;
}

/// 构造完整 NoiseChunk（cellCountXZ=1 + BeardifierMarker 空占位），返回其采样值数组。
/// 复刻 iterateNoiseColumn（NoiseChunkGenerator.cpp:755-791）的构造参数与初始化序列。
std::vector<f64> sampleNoiseChunkFinalDensity(
    const world::gen::RandomState& state, const DimensionSettings& settings, i32 chunkX, i32 chunkZ)
{
    const auto& noise = settings.noise;
    // 对齐 NoiseChunkGenerator 构造：cellWidth=sizeHorizontal*4, cellHeight=sizeVertical*4
    // 主世界/下界 4×8，末地 8×4
    const i32 cellWidth = noise.sizeHorizontal * 4;
    const i32 cellHeight = noise.sizeVertical * 4;
    const i32 cellCountY = math::floorDiv(noise.height, cellHeight);
    const i32 startX = chunkX * world::CHUNK_WIDTH;
    const i32 startZ = chunkZ * world::CHUNK_WIDTH;
    const i32 startBlockY = noise.minY;

    // 方案X 阶段5-7：传 state（RandomState），从维度级编译产物 newInstance 组装区块级 router。
    // BeardifierMarker 空占位 + cellCountXZ=1（对齐 iterateNoiseColumn）
    auto noiseChunk = std::make_unique<NoiseChunk>(
        state, cellWidth, cellHeight, cellCountY, startX, startBlockY, startZ, std::make_unique<BeardifierMarker>(), 1);

    // 复刻 iterateNoiseColumn 的初始化序列（fillSlice 预热 + 修正 cellStartBlockX）
    noiseChunk->initializeForFirstCellX();
    noiseChunk->advanceCellX(0);

    const auto& points = baselineSamplePoints();
    std::vector<f64> values;
    values.reserve(points.size());
    for (const auto& [x, y, z] : points) {
        values.push_back(noiseChunk->sampleFinalDensity(x, y, z));
    }
    return values;
}

/// 单个基线用例：世界种子 + 9 个采样点的期望密度值（顺序对齐 baselineSamplePoints）。
struct BaselineCase {
    u64 seed;
    std::array<f64, 9> expected;
};

/// 校验单个维度的一组基线用例。每个用例构造 NoiseChunk 采样后逐点 EXPECT_NEAR。
void expectBaselineMatches(
    const DimensionSettings& settings, const std::vector<BaselineCase>& cases, const std::string& dimensionLabel)
{
    const auto& points = baselineSamplePoints();
    for (const auto& c : cases) {
        auto state = world::gen::RandomState::create(settings, c.seed);
        ASSERT_NE(state, nullptr) << dimensionLabel << " seed=" << c.seed << ": RandomState::create returned null";
        const auto values = sampleNoiseChunkFinalDensity(*state, settings, 0, 0);
        ASSERT_EQ(values.size(), points.size()) << dimensionLabel << " seed=" << c.seed << ": sample count mismatch";
        for (size_t i = 0; i < values.size(); ++i) {
            const auto& [x, y, z] = points[i];
            EXPECT_NEAR(values[i], c.expected[i], 1e-9)
                << dimensionLabel << " seed=" << c.seed << " point#" << i << " (" << x << "," << y << "," << z
                << ") diverged from baseline";
        }
    }
}

} // namespace

// ============================================================================
// 固定基线校验：主世界/下界/末地多 seed 的 finalDensity 多点采样值。
// 数值由当前代码（5e8b9b074，原始 mapAll 两步法）采集，重构后必须继续通过。
// ============================================================================

TEST(DensityAstBaselineTest, OverworldMultipleSeeds)
{
    const std::vector<BaselineCase> cases = {
        {0ULL,
            {-0.013124738648118072,
                -0.16754305218568638,
                -0.0007202165352208586,
                0.14411684927036622,
                -0.12143252439471416,
                0.1844796581340602,
                -0.45833333333333331,
                -0.024994791666666669,
                0.062277019534064039}},
        {1ULL,
            {0.031903597286961127,
                -0.11430344320685366,
                0.024934518022713691,
                0.11422839078965669,
                -0.085229081450255859,
                0.11422839078965669,
                -0.45833333333333331,
                -0.024994791666666669,
                0.050329364907569452}},
        {42ULL,
            {0.046253835289808358,
                -0.061529216652209659,
                0.039523533045558681,
                0.023982077837881442,
                0.0019967670880604903,
                0.046253835289808358,
                -0.45833333333333331,
                -0.024994791666666669,
                0.031698620427618486}},
        {12345ULL,
            {0.15025076924075875,
                -0.040335598622778426,
                0.13208432476266568,
                0.15025076924075875,
                -0.092755937874121314,
                -0.0091209161303311866,
                -0.45787262617923247,
                -0.024994791666666669,
                0.056424551966537465}},
    };
    expectBaselineMatches(DimensionSettings::overworld(), cases, "overworld");
}

TEST(DensityAstBaselineTest, NetherMultipleSeeds)
{
    const std::vector<BaselineCase> cases = {
        {0ULL,
            {0.45833333333333331,
                0.058245132939739735,
                0.45833333333333331,
                -0.0040847865664029532,
                0.11347367352675632,
                0.45833333333333331,
                0.076969506498398663,
                0.29099999999999998,
                0.45833333333333331}},
        {7ULL,
            {0.45833333333333331,
                -0.076885233812977849,
                0.45833333333333331,
                -0.011202140938099741,
                -0.081705998088752521,
                0.45833333333333331,
                0.022351463624091239,
                0.29099999999999998,
                0.45833333333333331}},
        {99ULL,
            {0.45833333333333331,
                0.1172828719435018,
                0.45833333333333331,
                0.014103239244161491,
                0.12401482860527686,
                0.45833333333333331,
                0.18165123516696008,
                0.29099999999999998,
                0.45833333333333331}},
    };
    expectBaselineMatches(DimensionSettings::nether(), cases, "nether");
}

TEST(DensityAstBaselineTest, EndMultipleSeeds)
{
    const std::vector<BaselineCase> cases = {
        {0ULL,
            {-0.074859375000000006,
                -0.078945273046713949,
                -0.074859375000000006,
                0.1227159443321398,
                -0.088985362006404556,
                -0.074859375000000006,
                -0.45833333333333331,
                -0.45833333333333331,
                -0.074859375000000006}},
        {5ULL,
            {-0.074859375000000006,
                0.0012327923112534901,
                -0.074859375000000006,
                0.13688447998107087,
                -0.0066341082590966891,
                -0.074859375000000006,
                -0.45833333333333331,
                -0.45833333333333331,
                -0.074859375000000006}},
        {256ULL,
            {-0.074859375000000006,
                -0.044190061280489731,
                -0.074859375000000006,
                0.17745162259428537,
                -0.090626180431414155,
                -0.074859375000000006,
                -0.45833333333333331,
                -0.45833333333333331,
                -0.074859375000000006}},
    };
    expectBaselineMatches(DimensionSettings::end(), cases, "end");
}
