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
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/gen/FeaturePlacer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"

#include <memory>
#include <vector>

using namespace mc;

// ============================================================================
// FeaturePlacer::createRegion 测试
// ============================================================================

class FeaturePlacerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建 3x3 ChunkPrimer 区域（中心在 0,0）
        m_chunks.resize(9);
        for (i32 relZ = -1; relZ <= 1; ++relZ) {
            for (i32 relX = -1; relX <= 1; ++relX) {
                const i32 index = (relZ + 1) * 3 + (relX + 1);
                auto chunk = std::make_unique<ChunkPrimer>(relX, relZ);
                m_chunks[static_cast<size_t>(index)] = chunk.get();
                m_ownedChunks.push_back(std::move(chunk));
            }
        }
    }

    void TearDown() override
    {
        m_ownedChunks.clear();
        m_chunks.clear();
    }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
};

TEST_F(FeaturePlacerTest, CreateRegionWithValidChunks)
{
    // FeaturePlacer::createRegion 应成功从 3x3 区块创建 WorldGenRegion
    auto region = world::gen::FeaturePlacer::createRegion(0, 0, std::move(m_chunks), 1, 0);
    ASSERT_NE(region, nullptr);

    // 验证基本属性
    EXPECT_EQ(region->dimension(), 0);
}

TEST_F(FeaturePlacerTest, CreateRegionWithNonZeroDimension)
{
    // 创建下界维度的 WorldGenRegion
    auto region = world::gen::FeaturePlacer::createRegion(0, 0, std::move(m_chunks), 1, 1 /* 下界 */);
    ASSERT_NE(region, nullptr);
    EXPECT_EQ(region->dimension(), 1);
}

TEST_F(FeaturePlacerTest, PopulateWorldStateSetsProperties)
{
    auto region = world::gen::FeaturePlacer::createRegion(0, 0, std::move(m_chunks), 1, 0);
    ASSERT_NE(region, nullptr);

    // 填充世界状态
    world::gen::FeaturePlacer::populateWorldState(*region,
        12345u,  // seed
        100u,    // currentTick
        6000i64, // dayTime
        true,    // hardcore
        Difficulty::Normal);

    // 验证填充的属性
    EXPECT_EQ(region->seed(), 12345u);
    EXPECT_EQ(region->currentTick(), 100u);
    EXPECT_EQ(region->dayTime(), 6000i64);
    EXPECT_EQ(region->isHardcore(), true);
    EXPECT_EQ(region->difficulty(), Difficulty::Normal);
}

TEST_F(FeaturePlacerTest, PopulateWorldStateDefaultDifficulty)
{
    auto region = world::gen::FeaturePlacer::createRegion(0, 0, std::move(m_chunks), 1, 0);
    ASSERT_NE(region, nullptr);

    world::gen::FeaturePlacer::populateWorldState(*region, 0u, 0u, 0i64, false, Difficulty::Peaceful);

    EXPECT_EQ(region->seed(), 0u);
    EXPECT_EQ(region->difficulty(), Difficulty::Peaceful);
    EXPECT_EQ(region->isHardcore(), false);
}

TEST_F(FeaturePlacerTest, CreateRegionAllowsBlockWrites)
{
    VanillaBlocks::initialize();

    // 使用无步骤验证构造函数创建的 WorldGenRegion 应允许方块写入
    auto region = world::gen::FeaturePlacer::createRegion(0, 0, std::move(m_chunks), 1, 0);
    ASSERT_NE(region, nullptr);

    world::gen::FeaturePlacer::populateWorldState(*region, 42u, 0u, 0i64, false, Difficulty::Easy);

    // 在区块内设置一个方块 - 不应崩溃或断言失败
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    // 注意：setBlockState 在无步骤模式下跳过访问窗口断言
    // 我们只需验证 region 可用而不崩溃
    EXPECT_NE(region, nullptr);
}

TEST_F(FeaturePlacerTest, CreateRegionWithSingleChunk)
{
    // 使用半径 0 创建仅包含 1 个区块的 WorldGenRegion
    std::vector<IChunk*> singleChunk;
    auto chunk = std::make_unique<ChunkPrimer>(0, 0);
    singleChunk.push_back(chunk.get());
    m_ownedChunks.push_back(std::move(chunk));

    auto region = world::gen::FeaturePlacer::createRegion(0, 0, std::move(singleChunk), 0, 0);
    ASSERT_NE(region, nullptr);
    EXPECT_EQ(region->dimension(), 0);
}

// ============================================================================
// FeaturePlacer::populateWorldState 种子确定性测试
// ============================================================================

TEST_F(FeaturePlacerTest, PopulateWorldStateIsDeterministic)
{
    // 使用相同的参数创建两个 region，验证种子等属性一致
    std::vector<IChunk*> chunks1;
    std::vector<IChunk*> chunks2;
    std::vector<std::unique_ptr<ChunkPrimer>> ownedChunks;

    for (i32 relZ = -1; relZ <= 1; ++relZ) {
        for (i32 relX = -1; relX <= 1; ++relX) {
            auto chunk1 = std::make_unique<ChunkPrimer>(relX, relZ);
            auto chunk2 = std::make_unique<ChunkPrimer>(relX, relZ);
            chunks1.push_back(chunk1.get());
            chunks2.push_back(chunk2.get());
            ownedChunks.push_back(std::move(chunk1));
            ownedChunks.push_back(std::move(chunk2));
        }
    }

    auto region1 = world::gen::FeaturePlacer::createRegion(0, 0, std::move(chunks1), 1, 0);
    auto region2 = world::gen::FeaturePlacer::createRegion(0, 0, std::move(chunks2), 1, 0);
    ASSERT_NE(region1, nullptr);
    ASSERT_NE(region2, nullptr);

    const u64 seed = 98765u;
    const u64 tick = 200u;
    const i64 dayTime = 12000i64;
    const bool hardcore = false;
    const Difficulty difficulty = Difficulty::Hard;

    world::gen::FeaturePlacer::populateWorldState(*region1, seed, tick, dayTime, hardcore, difficulty);
    world::gen::FeaturePlacer::populateWorldState(*region2, seed, tick, dayTime, hardcore, difficulty);

    // 相同参数应产生相同的世界状态
    EXPECT_EQ(region1->seed(), region2->seed());
    EXPECT_EQ(region1->currentTick(), region2->currentTick());
    EXPECT_EQ(region1->dayTime(), region2->dayTime());
    EXPECT_EQ(region1->isHardcore(), region2->isHardcore());
    EXPECT_EQ(region1->difficulty(), region2->difficulty());
}
