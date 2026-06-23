/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted copies of the following:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

// ============================================================================
// ChunkStep::getRequiredStatusAtRadius (byRadius[] 查找表) 单元测试
//
// 验证每个生成步骤的 byRadius[] 与 BUG 文档第 8 节的累积依赖表一致。
// byRadius[radius] 表示：生成 targetStatus 时，距离中心 radius（Chebyshev 距离）
// 的邻居必须至少达到的 ChunkStatus。
//
// 关键不变量：
//   byRadius[radius] == accumulatedDependencies.get(radius)  (对 0..accumulatedRadius)
//   byRadius[0] == targetStatus.parent()  (中心区块需要前一步状态)
//   byRadius[accumulatedRadius] == 最外圈邻居状态（通常 STRUCTURE_STARTS）
//
// 对齐 Moonrise ChunkStepMixin.moonrise$getRequiredStatusAtRadius。
// ============================================================================

#include "common/world/chunk/gen/ChunkPyramid.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"

#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::chunk;

namespace {

// 辅助：获取生成金字塔中某状态的步骤
const ChunkStep& genStep(const ChunkStatus& status)
{
    return ChunkPyramid::generationPyramid().getStepTo(status);
}

// 辅助：断言 byRadius[radius] 与 accumulatedDependencies.get(radius) 完全一致
void expectByRadiusMatchesAccumulated(const ChunkStatus& target)
{
    const ChunkStep& step = genStep(target);
    ASSERT_TRUE(step.hasRequiredStatusByRadius()) << "step " << target.name() << " should have byRadius[] built";

    const i32 radius = step.accumulatedRadius();
    for (i32 r = 0; r <= radius; ++r) {
        const ChunkStatus* byRadius = step.getRequiredStatusAtRadius(r);
        const ChunkStatus* accDep = step.accumulatedDependencies().get(r);
        EXPECT_EQ(byRadius, accDep) << "byRadius[" << r << "] mismatch for target " << target.name()
                                    << ": byRadius=" << (byRadius ? byRadius->name() : std::string("null"))
                                    << " vs accumulated=" << (accDep ? accDep->name() : std::string("null"));
    }
}

} // namespace

// ============================================================================
// hasRequiredStatusByRadius / neighbourReadRadius 基础
// ============================================================================

TEST(ChunkStepByRadiusTest, GenerationPyramidBuildsByRadiusForAllSteps)
{
    // generationPyramid() 构造时已为每个步骤调用 buildRequiredStatusByRadius
    for (const ChunkStatus& status : ChunkStatus::getAll()) {
        const ChunkStep& step = genStep(status);
        EXPECT_TRUE(step.hasRequiredStatusByRadius()) << "step " << status.name() << " missing byRadius[]";
    }
}

TEST(ChunkStepByRadiusTest, NeighbourReadRadiusEqualsAccumulatedRadius)
{
    for (const ChunkStatus& status : ChunkStatus::getAll()) {
        const ChunkStep& step = genStep(status);
        EXPECT_EQ(step.neighbourReadRadius(), step.accumulatedRadius())
            << "neighbourReadRadius should equal accumulatedRadius for " << status.name();
    }
}

TEST(ChunkStepByRadiusTest, ByRadiusZeroIsAccumulatedDependencyZero)
{
    // byRadius[0] = 中心区块需要的前一步状态 = accumulatedDependencies.get(0)
    // 注意：Cubium 的 ChunkStatus 将根状态 EMPTY 的 parent 设为自身（非 nullptr），
    // 所以不能用 status.parent() 判断（EMPTY 会得到自身）。
    // accumulatedDependencies.get(0) 对 EMPTY 返回 nullptr，对其他状态返回前一步状态，
    // 这正是 byRadius[0] 应有的语义。
    for (const ChunkStatus& status : ChunkStatus::getAll()) {
        const ChunkStep& step = genStep(status);
        const ChunkStatus* byRadius0 = step.getRequiredStatusAtRadius(0);
        const ChunkStatus* accDep0 = step.accumulatedDependencies().get(0);
        EXPECT_EQ(byRadius0, accDep0) << "byRadius[0] for " << status.name()
                                      << " should equal accumulatedDependencies.get(0) = "
                                      << (accDep0 ? accDep0->name() : std::string("null"));
    }
}

// ============================================================================
// 与 accumulatedDependencies.get(radius) 一致性（核心不变量）
// ============================================================================

TEST(ChunkStepByRadiusTest, ByRadiusMatchesAccumulatedDepsForAllSteps)
{
    for (const ChunkStatus& status : ChunkStatus::getAll()) {
        expectByRadiusMatchesAccumulated(status);
    }
}

// ============================================================================
// 具体步骤验证（对照 BUG 文档第 8 节累积依赖表）
// ============================================================================

TEST(ChunkStepByRadiusTest, StructureStartsByRadius)
{
    // STRUCTURE_STARTS: accumulatedDeps = [0:EMPTY], accumulatedRadius = 0
    const ChunkStep& step = genStep(ChunkStatuses::STRUCTURE_STARTS);
    EXPECT_EQ(step.accumulatedRadius(), 0);
    EXPECT_EQ(step.getRequiredStatusAtRadius(0), &ChunkStatuses::EMPTY);
    // 越界返回 nullptr
    EXPECT_EQ(step.getRequiredStatusAtRadius(1), nullptr);
    EXPECT_EQ(step.getRequiredStatusAtRadius(-1), nullptr);
}

TEST(ChunkStepByRadiusTest, StructureReferencesByRadius)
{
    // STRUCTURE_REFERENCES: accumulatedDeps = [0-8:SS], accumulatedRadius = 8
    const ChunkStep& step = genStep(ChunkStatuses::STRUCTURE_REFERENCES);
    EXPECT_EQ(step.accumulatedRadius(), 8);
    // byRadius[0] = parent = STRUCTURE_STARTS（注意：SS.parent() = EMPTY，但 byRadius[0] 取 parent）
    EXPECT_EQ(step.getRequiredStatusAtRadius(0), &ChunkStatuses::STRUCTURE_STARTS);
    for (i32 r = 1; r <= 8; ++r) {
        EXPECT_EQ(step.getRequiredStatusAtRadius(r), &ChunkStatuses::STRUCTURE_STARTS) << "radius " << r;
    }
    EXPECT_EQ(step.getRequiredStatusAtRadius(9), nullptr);
}

TEST(ChunkStepByRadiusTest, BiomesByRadius)
{
    // BIOMES: accumulatedDeps = [0:SR, 1-8:SS], accumulatedRadius = 8
    const ChunkStep& step = genStep(ChunkStatuses::BIOMES);
    EXPECT_EQ(step.accumulatedRadius(), 8);
    EXPECT_EQ(step.getRequiredStatusAtRadius(0), &ChunkStatuses::STRUCTURE_REFERENCES); // parent = SR
    EXPECT_EQ(step.getRequiredStatusAtRadius(1), &ChunkStatuses::STRUCTURE_STARTS);
    for (i32 r = 2; r <= 8; ++r) {
        EXPECT_EQ(step.getRequiredStatusAtRadius(r), &ChunkStatuses::STRUCTURE_STARTS) << "radius " << r;
    }
}

TEST(ChunkStepByRadiusTest, NoiseByRadius)
{
    // NOISE: accumulatedDeps = [0:BIO, 1:BIO, 2-9:SS], accumulatedRadius = 9
    const ChunkStep& step = genStep(ChunkStatuses::NOISE);
    EXPECT_EQ(step.accumulatedRadius(), 9);
    EXPECT_EQ(step.getRequiredStatusAtRadius(0), &ChunkStatuses::BIOMES); // parent
    EXPECT_EQ(step.getRequiredStatusAtRadius(1), &ChunkStatuses::BIOMES);
    for (i32 r = 2; r <= 9; ++r) {
        EXPECT_EQ(step.getRequiredStatusAtRadius(r), &ChunkStatuses::STRUCTURE_STARTS) << "radius " << r;
    }
    EXPECT_EQ(step.getRequiredStatusAtRadius(10), nullptr);
}

TEST(ChunkStepByRadiusTest, FeaturesByRadius)
{
    // FEATURES: accumulatedDeps = [0:CAR, 1:CAR, 2:BIO, 3-10:SS], accumulatedRadius = 10
    const ChunkStep& step = genStep(ChunkStatuses::FEATURES);
    EXPECT_EQ(step.accumulatedRadius(), 10);
    EXPECT_EQ(step.getRequiredStatusAtRadius(0), &ChunkStatuses::CARVERS); // parent
    EXPECT_EQ(step.getRequiredStatusAtRadius(1), &ChunkStatuses::CARVERS);
    EXPECT_EQ(step.getRequiredStatusAtRadius(2), &ChunkStatuses::BIOMES);
    for (i32 r = 3; r <= 10; ++r) {
        EXPECT_EQ(step.getRequiredStatusAtRadius(r), &ChunkStatuses::STRUCTURE_STARTS) << "radius " << r;
    }
    EXPECT_EQ(step.getRequiredStatusAtRadius(11), nullptr);
}

TEST(ChunkStepByRadiusTest, FullByRadius)
{
    // FULL: accumulatedDeps = [0:SPAWN, 1:IL, 2:CAR, 3:BIO, 4-11:SS], accumulatedRadius = 11
    // 这是 BUG 文档复现场景的中心区块状态。
    const ChunkStep& step = genStep(ChunkStatuses::FULL);
    EXPECT_EQ(step.accumulatedRadius(), 11);
    EXPECT_EQ(step.getRequiredStatusAtRadius(0), &ChunkStatuses::SPAWN); // parent
    EXPECT_EQ(step.getRequiredStatusAtRadius(1), &ChunkStatuses::INITIALIZE_LIGHT);
    EXPECT_EQ(step.getRequiredStatusAtRadius(2), &ChunkStatuses::CARVERS);
    EXPECT_EQ(step.getRequiredStatusAtRadius(3), &ChunkStatuses::BIOMES);
    for (i32 r = 4; r <= 11; ++r) {
        EXPECT_EQ(step.getRequiredStatusAtRadius(r), &ChunkStatuses::STRUCTURE_STARTS) << "radius " << r;
    }
    EXPECT_EQ(step.getRequiredStatusAtRadius(12), nullptr);
}

TEST(ChunkStepByRadiusTest, LightByRadius)
{
    // LIGHT: accumulatedDeps = [0:IL, 1:IL, 2:CAR, 3:BIO, 4-11:SS], accumulatedRadius = 11
    const ChunkStep& step = genStep(ChunkStatuses::LIGHT);
    EXPECT_EQ(step.accumulatedRadius(), 11);
    EXPECT_EQ(step.getRequiredStatusAtRadius(0), &ChunkStatuses::INITIALIZE_LIGHT); // parent
    EXPECT_EQ(step.getRequiredStatusAtRadius(1), &ChunkStatuses::INITIALIZE_LIGHT);
    EXPECT_EQ(step.getRequiredStatusAtRadius(2), &ChunkStatuses::CARVERS);
    EXPECT_EQ(step.getRequiredStatusAtRadius(3), &ChunkStatuses::BIOMES);
    for (i32 r = 4; r <= 11; ++r) {
        EXPECT_EQ(step.getRequiredStatusAtRadius(r), &ChunkStatuses::STRUCTURE_STARTS) << "radius " << r;
    }
}

TEST(ChunkStepByRadiusTest, SpawnByRadius)
{
    // SPAWN: accumulatedDeps = [0:LIG, 1:IL, 2:CAR, 3:BIO, 4-11:SS], accumulatedRadius = 11
    const ChunkStep& step = genStep(ChunkStatuses::SPAWN);
    EXPECT_EQ(step.accumulatedRadius(), 11);
    EXPECT_EQ(step.getRequiredStatusAtRadius(0), &ChunkStatuses::LIGHT); // parent
    EXPECT_EQ(step.getRequiredStatusAtRadius(1), &ChunkStatuses::INITIALIZE_LIGHT);
    EXPECT_EQ(step.getRequiredStatusAtRadius(2), &ChunkStatuses::CARVERS);
    EXPECT_EQ(step.getRequiredStatusAtRadius(3), &ChunkStatuses::BIOMES);
    for (i32 r = 4; r <= 11; ++r) {
        EXPECT_EQ(step.getRequiredStatusAtRadius(r), &ChunkStatuses::STRUCTURE_STARTS) << "radius " << r;
    }
}

// ============================================================================
// EMPTY 特殊情况
// ============================================================================

TEST(ChunkStepByRadiusTest, EmptyStepByRadius)
{
    // EMPTY: 无前序，accumulatedRadius = 0，accumulatedDependencies 为空，
    // byRadius[0] = accumulatedDependencies.get(0) = nullptr（无前一步状态）
    // 注意：Cubium 的 EMPTY.parent() 返回自身（非 nullptr），所以 byRadius[0] 用
    // accumulatedDependencies.get(0) 而非 parent() 来保证空依赖时返回 nullptr。
    const ChunkStep& step = genStep(ChunkStatuses::EMPTY);
    EXPECT_EQ(step.accumulatedRadius(), 0);
    EXPECT_TRUE(step.hasRequiredStatusByRadius());
    EXPECT_EQ(step.getRequiredStatusAtRadius(0), nullptr); // 空依赖，无前一步状态
    EXPECT_EQ(step.getRequiredStatusAtRadius(1), nullptr); // 越界
}

// ============================================================================
// getRadiusOf 语义验证（byRadius 构建依赖此语义）
// ============================================================================

TEST(ChunkStepByRadiusTest, GetRadiusOfReturnsLastCoveringRadius)
{
    // 验证 getRadiusOf 语义：返回覆盖该状态的最大半径（外圈）
    // 这是 byRadius[] 构建正确的关键前提。
    const ChunkStep& fullStep = genStep(ChunkStatuses::FULL);
    const ChunkDependencies& acc = fullStep.accumulatedDependencies();

    // FULL 的累积依赖 [0:SPAWN, 1:IL, 2:CAR, 3:BIO, 4-11:SS]
    // getRadiusOf 对每个状态返回其在 m_radiusByDependency 中的值（最后覆盖半径）
    EXPECT_EQ(acc.getRadiusOf(ChunkStatuses::STRUCTURE_STARTS), 11);
    EXPECT_EQ(acc.getRadiusOf(ChunkStatuses::BIOMES), 3);
    EXPECT_EQ(acc.getRadiusOf(ChunkStatuses::CARVERS), 2);
    EXPECT_EQ(acc.getRadiusOf(ChunkStatuses::INITIALIZE_LIGHT), 1);
    EXPECT_EQ(acc.getRadiusOf(ChunkStatuses::SPAWN), 0);
}

// ============================================================================
// 加载金字塔也构建了 byRadius[]（loadingPyramid 的依赖更简单，但仍需一致）
// ============================================================================

TEST(ChunkStepByRadiusTest, LoadingPyramidBuildsByRadiusForAllSteps)
{
    for (const ChunkStatus& status : ChunkStatus::getAll()) {
        const ChunkStep& step = ChunkPyramid::loadingPyramid().getStepTo(status);
        EXPECT_TRUE(step.hasRequiredStatusByRadius()) << "loading step " << status.name() << " missing byRadius[]";
        // 加载金字塔 byRadius[] 也应与 accumulatedDependencies 一致
        const i32 radius = step.accumulatedRadius();
        for (i32 r = 0; r <= radius; ++r) {
            EXPECT_EQ(step.getRequiredStatusAtRadius(r), step.accumulatedDependencies().get(r))
                << "loading byRadius[" << r << "] for " << status.name();
        }
    }
}
