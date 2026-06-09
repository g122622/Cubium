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

#include <gtest/gtest.h>

#include "common/world/chunk/ChunkLevel.hpp"
#include "common/world/chunk/ChunkPyramid.hpp"
#include "common/world/chunk/ChunkStatus.hpp"

using namespace mc;
using namespace mc::world::chunk;

// ============================================================================
// ChunkLevel 测试
// ============================================================================

TEST(ChunkLevel, Constants)
{
    EXPECT_EQ(ChunkLevel::ENTITY_TICKING_LEVEL, 31);
    EXPECT_EQ(ChunkLevel::BLOCK_TICKING_LEVEL, 32);
    EXPECT_EQ(ChunkLevel::FULL_CHUNK_LEVEL, 33);
}

TEST(ChunkLevel, RadiusAroundFullChunk)
{
    // FULL 步骤的累积依赖半径 = 11（由 buildAccumulatedDependencies 的偏移合并计算得出）
    EXPECT_EQ(ChunkLevel::radiusAroundFullChunk(), 11);
}

TEST(ChunkLevel, MaxLevel)
{
    // MAX_LEVEL = 33 + 11 = 44
    EXPECT_EQ(ChunkLevel::maxLevel(), 44);
}

TEST(ChunkLevel, GenerationStatusFromLevel)
{
    // 级别 <= 33 → FULL
    EXPECT_EQ(ChunkLevel::generationStatus(33), &ChunkStatuses::FULL);
    EXPECT_EQ(ChunkLevel::generationStatus(31), &ChunkStatuses::FULL);
    EXPECT_EQ(ChunkLevel::generationStatus(0), &ChunkStatuses::FULL);

    // FULL 的 accumulatedDependencies = [SPAWN, IL, CARVERS, BIOMES, SS, SS, SS, SS, SS, SS, SS, SS]
    // 级别 34 → accumulatedDependencies[1] = INITIALIZE_LIGHT
    EXPECT_EQ(ChunkLevel::generationStatus(34), &ChunkStatuses::INITIALIZE_LIGHT);

    // 级别 35 → accumulatedDependencies[2] = CARVERS
    EXPECT_EQ(ChunkLevel::generationStatus(35), &ChunkStatuses::CARVERS);

    // 级别 36 → accumulatedDependencies[3] = BIOMES
    EXPECT_EQ(ChunkLevel::generationStatus(36), &ChunkStatuses::BIOMES);

    // 级别 37-44 → STRUCTURE_STARTS
    EXPECT_EQ(ChunkLevel::generationStatus(37), &ChunkStatuses::STRUCTURE_STARTS);
    EXPECT_EQ(ChunkLevel::generationStatus(44), &ChunkStatuses::STRUCTURE_STARTS);

    // 超过 maxLevel → nullptr
    EXPECT_EQ(ChunkLevel::generationStatus(45), nullptr);
    EXPECT_EQ(ChunkLevel::generationStatus(46), nullptr);
}

TEST(ChunkLevel, ByStatus)
{
    // FULL → 33（特殊情况，返回 0 半径）
    EXPECT_EQ(ChunkLevel::byStatus(ChunkStatuses::FULL), 33);

    // SPAWN → 33 + 0 = 33
    EXPECT_EQ(ChunkLevel::byStatus(ChunkStatuses::SPAWN), 33);

    // LIGHT → 33 + 0 = 33
    EXPECT_EQ(ChunkLevel::byStatus(ChunkStatuses::LIGHT), 33);

    // INITIALIZE_LIGHT → 33 + 1 = 34
    EXPECT_EQ(ChunkLevel::byStatus(ChunkStatuses::INITIALIZE_LIGHT), 34);

    // FEATURES → 33 + 1 = 34
    EXPECT_EQ(ChunkLevel::byStatus(ChunkStatuses::FEATURES), 34);

    // CARVERS → 33 + 2 = 35
    EXPECT_EQ(ChunkLevel::byStatus(ChunkStatuses::CARVERS), 35);

    // SURFACE → 33 + 2 = 35
    EXPECT_EQ(ChunkLevel::byStatus(ChunkStatuses::SURFACE), 35);

    // NOISE → 33 + 2 = 35
    EXPECT_EQ(ChunkLevel::byStatus(ChunkStatuses::NOISE), 35);

    // BIOMES → 33 + 3 = 36
    EXPECT_EQ(ChunkLevel::byStatus(ChunkStatuses::BIOMES), 36);

    // STRUCTURE_REFERENCES → 33 + 3 = 36
    EXPECT_EQ(ChunkLevel::byStatus(ChunkStatuses::STRUCTURE_REFERENCES), 36);

    // STRUCTURE_STARTS → 33 + 11 = 44
    EXPECT_EQ(ChunkLevel::byStatus(ChunkStatuses::STRUCTURE_STARTS), 44);

    // EMPTY → 33 + 11 = 44
    EXPECT_EQ(ChunkLevel::byStatus(ChunkStatuses::EMPTY), 44);
}

TEST(ChunkLevel, RoundTrip)
{
    // byStatus(*generationStatus(n)) 的语义：多个级别可能映射到同一状态，
    // byStatus 返回该状态所需的最低级别的最大半径对应级别。
    // 验证：generationStatus(byStatus(generationStatus(n))) == generationStatus(n)
    for (i32 level = 33; level <= 44; ++level) {
        const ChunkStatus* status = ChunkLevel::generationStatus(level);
        ASSERT_NE(status, nullptr) << "generationStatus(" << level << ") returned nullptr";
        i32 roundTripLevel = ChunkLevel::byStatus(*status);
        // roundTripLevel 必须指向同一状态
        EXPECT_EQ(ChunkLevel::generationStatus(roundTripLevel), status)
            << "generationStatus(byStatus(generationStatus(" << level << "))) != generationStatus(" << level << ")";
        // roundTripLevel >= level：因为 byStatus 返回覆盖该状态的最远级别
        EXPECT_GE(roundTripLevel, level) << "byStatus(generationStatus(" << level << ")) = " << roundTripLevel << " < "
                                         << level;
    }
}

TEST(ChunkLevel, GetStatusAroundFullChunk)
{
    // 距离 0 → FULL
    EXPECT_EQ(ChunkLevel::getStatusAroundFullChunk(0), &ChunkStatuses::FULL);

    // 距离 1 → INITIALIZE_LIGHT
    EXPECT_EQ(ChunkLevel::getStatusAroundFullChunk(1), &ChunkStatuses::INITIALIZE_LIGHT);

    // 距离 2 → CARVERS
    EXPECT_EQ(ChunkLevel::getStatusAroundFullChunk(2), &ChunkStatuses::CARVERS);

    // 距离 3 → BIOMES
    EXPECT_EQ(ChunkLevel::getStatusAroundFullChunk(3), &ChunkStatuses::BIOMES);

    // 距离 4-11 → STRUCTURE_STARTS
    EXPECT_EQ(ChunkLevel::getStatusAroundFullChunk(4), &ChunkStatuses::STRUCTURE_STARTS);
    EXPECT_EQ(ChunkLevel::getStatusAroundFullChunk(11), &ChunkStatuses::STRUCTURE_STARTS);

    // 超出范围 → nullptr
    EXPECT_EQ(ChunkLevel::getStatusAroundFullChunk(12), nullptr);
    EXPECT_EQ(ChunkLevel::getStatusAroundFullChunk(-1), &ChunkStatuses::FULL);
}

// ============================================================================
// LOADING_PYRAMID 测试
// ============================================================================

TEST(ChunkPyramid, LoadingPyramidExists)
{
    const ChunkPyramid& pyramid = ChunkPyramid::loadingPyramid();
    EXPECT_EQ(pyramid.steps().size(), 12u);
}

TEST(ChunkPyramid, LoadingPyramidStepsHaveMinimalDependencies)
{
    const ChunkPyramid& pyramid = ChunkPyramid::loadingPyramid();

    // 加载金字塔中大多数步骤的直接依赖半径为 0 或 1
    for (const auto& step : pyramid.steps()) {
        i32 directRadius = step.directRadius();
        // 加载金字塔的直接依赖半径不超过 1（只有 LIGHT 需要 INITIALIZE_LIGHT(1)）
        EXPECT_LE(directRadius, 1) << "Loading pyramid step " << step.targetStatus()->name() << " has directRadius "
                                   << directRadius;
    }
}

TEST(ChunkPyramid, LoadingPyramidAllWriteRadiusNegative)
{
    const ChunkPyramid& pyramid = ChunkPyramid::loadingPyramid();

    // 加载路径不写方块
    for (const auto& step : pyramid.steps()) {
        EXPECT_EQ(step.blockStateWriteRadius(), -1)
            << "Loading pyramid step " << step.targetStatus()->name() << " has blockStateWriteRadius != -1";
    }
}

TEST(ChunkPyramid, LoadingPyramidLightRequiresInitializeLight)
{
    const ChunkPyramid& pyramid = ChunkPyramid::loadingPyramid();

    // LIGHT 步骤依赖 INITIALIZE_LIGHT(1)
    const ChunkStep& lightStep = pyramid.getStepTo(ChunkStatuses::LIGHT);
    const ChunkDependencies& deps = lightStep.directDependencies();
    EXPECT_GE(deps.size(), 2);
    EXPECT_EQ(deps.get(1), &ChunkStatuses::INITIALIZE_LIGHT);
}

TEST(ChunkPyramid, LoadingPyramidOtherStepsDependOnlyOnParent)
{
    const ChunkPyramid& pyramid = ChunkPyramid::loadingPyramid();

    // 除了 LIGHT 外，所有步骤的直接依赖半径为 0（仅依赖前一步）
    const auto& steps = pyramid.steps();
    for (size_t i = 0; i < steps.size(); ++i) {
        const auto& step = steps[i];
        if (step.targetStatus() == &ChunkStatuses::LIGHT) {
            continue;
        }
        EXPECT_EQ(step.directRadius(), 0)
            << "Loading pyramid step " << step.targetStatus()->name() << " has directRadius != 0";
    }
}

TEST(ChunkPyramid, LoadingPyramidAccumulatedRadius)
{
    const ChunkPyramid& pyramid = ChunkPyramid::loadingPyramid();

    // 加载金字塔的累积半径应该比生成金字塔小得多
    // EMPTY 的累积半径为 0
    EXPECT_EQ(pyramid.getStepTo(ChunkStatuses::EMPTY).accumulatedRadius(), 0);

    // LIGHT 是唯一有非零直接依赖的步骤，累积半径为 1
    EXPECT_EQ(pyramid.getStepTo(ChunkStatuses::LIGHT).accumulatedRadius(), 1);

    // FULL 步骤的累积半径 = 1（因为 LIGHT 的半径传播到 FULL）
    EXPECT_EQ(pyramid.getStepTo(ChunkStatuses::FULL).accumulatedRadius(), 1);
}

TEST(ChunkPyramid, GenerationVsLoading)
{
    const ChunkPyramid& genPyramid = ChunkPyramid::generationPyramid();
    const ChunkPyramid& loadPyramid = ChunkPyramid::loadingPyramid();

    // 两者有相同数量的步骤
    EXPECT_EQ(genPyramid.steps().size(), loadPyramid.steps().size());

    // 生成金字塔 FULL 的累积半径远大于加载金字塔
    EXPECT_GT(genPyramid.getStepTo(ChunkStatuses::FULL).accumulatedRadius(),
        loadPyramid.getStepTo(ChunkStatuses::FULL).accumulatedRadius());

    // 结构引用步骤在生成金字塔中有大半径依赖，在加载金字塔中只有半径 0
    const ChunkStep& genStructRef = genPyramid.getStepTo(ChunkStatuses::STRUCTURE_REFERENCES);
    const ChunkStep& loadStructRef = loadPyramid.getStepTo(ChunkStatuses::STRUCTURE_REFERENCES);
    EXPECT_GT(genStructRef.directRadius(), 0);
    EXPECT_EQ(loadStructRef.directRadius(), 0);

    // FEATURES 在生成金字塔中有写半径 1，在加载金字塔中为 -1
    const ChunkStep& genFeatures = genPyramid.getStepTo(ChunkStatuses::FEATURES);
    const ChunkStep& loadFeatures = loadPyramid.getStepTo(ChunkStatuses::FEATURES);
    EXPECT_EQ(genFeatures.blockStateWriteRadius(), 1);
    EXPECT_EQ(loadFeatures.blockStateWriteRadius(), -1);
}
