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

#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace {

class WorldGenRegionAlignmentTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }

    // 创建 3x3 的 WorldGenRegion（中心区块 + 1 半径邻居）
    void SetUp() override
    {
        const i32 radius = 1;
        const i32 diameter = radius * 2 + 1;
        const ChunkCoord mainX = 5;
        const ChunkCoord mainZ = 5;

        m_primers.clear();
        m_chunkPtrs.clear();

        for (i32 dz = -radius; dz <= radius; ++dz) {
            for (i32 dx = -radius; dx <= radius; ++dx) {
                m_primers.push_back(std::make_unique<ChunkPrimer>(mainX + dx, mainZ + dz));
            }
        }

        for (auto& p : m_primers) {
            m_chunkPtrs.push_back(p.get());
        }

        m_region = std::make_unique<WorldGenRegion>(mainX, mainZ, radius, m_chunkPtrs, 0);
    }

    std::vector<std::unique_ptr<ChunkPrimer>> m_primers;
    std::vector<IChunk*> m_chunkPtrs;
    std::unique_ptr<WorldGenRegion> m_region;
};

// ============================================================================
// 构造与基本属性
// ============================================================================

TEST_F(WorldGenRegionAlignmentTest, Construction_BasicProperties)
{
    EXPECT_EQ(m_region->mainX(), 5);
    EXPECT_EQ(m_region->mainZ(), 5);
    EXPECT_EQ(m_region->chunkRadius(), 1);
    EXPECT_NE(m_region->getMainChunk(), nullptr);
    EXPECT_EQ(m_region->dimension(), 0); // 主世界
}

// ============================================================================
// 种子/时间/难度设置
// ============================================================================

TEST_F(WorldGenRegionAlignmentTest, SetSeed_WorksCorrectly)
{
    m_region->setSeed(12345);
    EXPECT_EQ(m_region->seed(), 12345);
}

TEST_F(WorldGenRegionAlignmentTest, SetCurrentTick_WorksCorrectly)
{
    m_region->setCurrentTick(100);
    EXPECT_EQ(m_region->currentTick(), 100u);
}

TEST_F(WorldGenRegionAlignmentTest, SetDayTime_WorksCorrectly)
{
    m_region->setDayTime(6000);
    EXPECT_EQ(m_region->dayTime(), 6000);
}

TEST_F(WorldGenRegionAlignmentTest, SetHardcore_WorksCorrectly)
{
    m_region->setHardcore(true);
    EXPECT_TRUE(m_region->isHardcore());
    m_region->setHardcore(false);
    EXPECT_FALSE(m_region->isHardcore());
}

TEST_F(WorldGenRegionAlignmentTest, SetDifficulty_WorksCorrectly)
{
    m_region->setDifficulty(Difficulty::Hard);
    EXPECT_EQ(m_region->difficulty(), Difficulty::Hard);
    m_region->setDifficulty(Difficulty::Peaceful);
    EXPECT_EQ(m_region->difficulty(), Difficulty::Peaceful);
}

// ============================================================================
// 维度感知高度
// ============================================================================

TEST_F(WorldGenRegionAlignmentTest, DimensionHeight_Overworld)
{
    // 主世界: minY=-64, maxY=320
    EXPECT_EQ(m_region->getMinBuildHeight(), -64);
    EXPECT_EQ(m_region->getMaxBuildHeight(), 320);
}

// ============================================================================
// 区块访问
// ============================================================================

TEST_F(WorldGenRegionAlignmentTest, GetIChunk_CenterChunk)
{
    IChunk* center = m_region->getIChunk(5, 5);
    ASSERT_NE(center, nullptr);
    EXPECT_EQ(center->x(), 5);
    EXPECT_EQ(center->z(), 5);
}

TEST_F(WorldGenRegionAlignmentTest, GetIChunk_NeighborChunk)
{
    IChunk* neighbor = m_region->getIChunk(4, 5);
    ASSERT_NE(neighbor, nullptr);
    EXPECT_EQ(neighbor->x(), 4);
}

TEST_F(WorldGenRegionAlignmentTest, GetIChunk_OutOfRange_ReturnsNull)
{
    IChunk* outOfRange = m_region->getIChunk(3, 5);
    EXPECT_EQ(outOfRange, nullptr);
}

// ============================================================================
// ensureCanWrite
// ============================================================================

TEST_F(WorldGenRegionAlignmentTest, EnsureCanWrite_NoStep_ReturnsFalse)
{
    // 没有生成步骤时，ensureCanWrite 应返回 false
    EXPECT_FALSE(m_region->ensureCanWrite(0, 0, 0));
}

// ============================================================================
// currentlyGenerating
// ============================================================================

TEST_F(WorldGenRegionAlignmentTest, CurrentlyGenerating_SetAndClear)
{
    m_region->setCurrentlyGenerating("minecraft:village");
    EXPECT_EQ(m_region->currentlyGenerating(), "minecraft:village");

    m_region->clearCurrentlyGenerating();
    EXPECT_TRUE(m_region->currentlyGenerating().empty());
}

// ============================================================================
// 区块坐标访问
// ============================================================================

TEST_F(WorldGenRegionAlignmentTest, ChunkCoordinateBounds)
{
    EXPECT_EQ(m_region->minChunkX(), 4);
    EXPECT_EQ(m_region->maxChunkX(), 6);
    EXPECT_EQ(m_region->minChunkZ(), 4);
    EXPECT_EQ(m_region->maxChunkZ(), 6);
}

} // namespace
} // namespace mc
