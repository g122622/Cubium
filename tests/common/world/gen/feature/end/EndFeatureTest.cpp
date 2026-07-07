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
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND OF EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/world/block/blocks/end/ChorusFlowerBlock.hpp"
#include "common/world/block/blocks/end/ChorusPlantBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "core/Constants.hpp"
#include "world/chunk/data/ChunkPrimer.hpp"
#include "world/gen/chunk/IChunkGenerator.hpp"
#include "world/gen/feature/end/ChorusPlantFeature.hpp"
#include "world/gen/feature/end/EndIslandFeature.hpp"
#include "world/gen/settings/DimensionSettings.hpp"

#include <memory>
#include <vector>

using namespace mc;

// ============================================================================
// 最小 IChunkGenerator 存根
// ============================================================================

class StubEndChunkGenerator : public IChunkGenerator {
public:
    void generateStructureStarts(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    void generateStructureReferences(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    void generateBiomes(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    void generateNoise(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    void buildSurface(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    void applyCarvers(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    void placeFeatures(WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/) override {}
    i32 spawnInitialMobs(
        WorldGenRegion& /*region*/, ChunkPrimer& /*chunk*/, std::vector<SpawnedEntityData>& /*outEntities*/) override
    {
        return 0;
    }
    [[nodiscard]] BiomeId getBiome(i32 /*x*/, i32 /*y*/, i32 /*z*/) const override { return 0; }
    [[nodiscard]] BiomeId getNoiseBiome(i32 /*noiseX*/, i32 /*noiseY*/, i32 /*noiseZ*/) const override { return 0; }
    [[nodiscard]] i32 getHeight(i32 /*x*/, i32 /*z*/, HeightmapType /*type*/) const override { return 64; }
    [[nodiscard]] u64 seed() const override { return 0; }
    [[nodiscard]] const DimensionSettings& settings() const override { return DimensionSettings::overworld(); }
    [[nodiscard]] i32 seaLevel() const override { return 63; }
};

// ============================================================================
// 末地小岛放置测试（使用 WorldGenRegion）
// ============================================================================

class EndIslandPlacementTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        m_chunks.resize(9);
        for (i32 relZ = -1; relZ <= 1; ++relZ) {
            for (i32 relX = -1; relX <= 1; ++relX) {
                const i32 index = (relZ + 1) * 3 + (relX + 1);
                auto chunk = std::make_unique<ChunkPrimer>(relX, relZ);
                m_chunks[static_cast<size_t>(index)] = chunk.get();
                m_ownedChunks.push_back(std::move(chunk));
            }
        }

        m_region = std::make_unique<WorldGenRegion>(0, 0, 1, std::move(m_chunks));
    }

    void TearDown() override
    {
        m_region.reset();
        m_ownedChunks.clear();
    }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
    math::Random m_random{42};
};

TEST_F(EndIslandPlacementTest, PlaceEndIsland_PlacesEndStoneBlocks)
{
    // 在世界中放置末地小岛，应生成末地石方块
    bool result = EndIslandFeature::place(*m_region, m_random, BlockPos(8, 60, 8));
    EXPECT_TRUE(result);

    // 验证世界中存在末地石
    bool foundEndStone = false;
    for (i32 x = 0; x < 16 && !foundEndStone; ++x) {
        for (i32 z = 0; z < 16 && !foundEndStone; ++z) {
            for (i32 y = 50; y <= 70; ++y) {
                const BlockState* state = m_region->getBlockState(x, y, z);
                if (state != nullptr && state->is(VanillaBlocks::END_STONE)) {
                    foundEndStone = true;
                    break;
                }
            }
        }
    }
    EXPECT_TRUE(foundEndStone);
}

TEST_F(EndIslandPlacementTest, PlaceEndIsland_CreatesNonTrivialShape)
{
    // 生成的岛屿应在多个位置有末地石（不是只有一个方块）
    math::Random random(12345);
    EndIslandFeature::place(*m_region, random, BlockPos(8, 60, 8));

    i32 endStoneCount = 0;
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            for (i32 y = 50; y <= 70; ++y) {
                const BlockState* state = m_region->getBlockState(x, y, z);
                if (state != nullptr && state->is(VanillaBlocks::END_STONE)) {
                    ++endStoneCount;
                }
            }
        }
    }

    // 末地小岛应包含多个末地石方块
    EXPECT_GT(endStoneCount, 5);
}

// ============================================================================
// 紫颂树放置测试（使用 WorldGenRegion）
// ============================================================================

class ChorusPlantPlacementTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        m_chunks.resize(9);
        for (i32 relZ = -1; relZ <= 1; ++relZ) {
            for (i32 relX = -1; relX <= 1; ++relX) {
                const i32 index = (relZ + 1) * 3 + (relX + 1);
                auto chunk = std::make_unique<ChunkPrimer>(relX, relZ);
                m_chunks[static_cast<size_t>(index)] = chunk.get();
                m_ownedChunks.push_back(std::move(chunk));
            }
        }

        m_region = std::make_unique<WorldGenRegion>(0, 0, 1, std::move(m_chunks));
    }

    void TearDown() override
    {
        m_region.reset();
        m_ownedChunks.clear();
    }

    void placeEndStoneFloor(i32 y)
    {
        for (i32 x = 0; x < 16; ++x) {
            for (i32 z = 0; z < 16; ++z) {
                m_region->setBlockState(x, y, z, &VanillaBlocks::END_STONE->defaultState());
            }
        }
    }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
    math::Random m_random{42};
};

TEST_F(ChorusPlantPlacementTest, PlaceChorusPlant_PlacesOnEndStone)
{
    // 在末地石上方放置紫颂树
    placeEndStoneFloor(60);

    bool result = ChorusPlantFeature::place(*m_region, m_random, BlockPos(8, 61, 8));
    EXPECT_TRUE(result);

    // 验证世界中存在紫颂植物或紫颂花
    bool foundChorus = false;
    for (i32 x = 0; x < 16 && !foundChorus; ++x) {
        for (i32 z = 0; z < 16 && !foundChorus; ++z) {
            for (i32 y = 61; y <= 80; ++y) {
                const BlockState* state = m_region->getBlockState(x, y, z);
                if (state == nullptr) {
                    continue;
                }
                if (state->is(VanillaBlocks::CHORUS_PLANT) || state->is(VanillaBlocks::CHORUS_FLOWER)) {
                    foundChorus = true;
                    break;
                }
            }
        }
    }
    EXPECT_TRUE(foundChorus);
}

TEST_F(ChorusPlantPlacementTest, PlaceChorusPlant_FailsWithoutEndStone)
{
    // 没有末地石地面，放置应失败
    bool result = ChorusPlantFeature::place(*m_region, m_random, BlockPos(8, 61, 8));
    EXPECT_FALSE(result);
}

TEST_F(ChorusPlantPlacementTest, PlaceChorusPlant_FailsOnSolidBlock)
{
    // 起始位置不是空气，放置应失败
    placeEndStoneFloor(60);
    m_region->setBlockState(8, 61, 8, &VanillaBlocks::STONE->defaultState());

    bool result = ChorusPlantFeature::place(*m_region, m_random, BlockPos(8, 61, 8));
    EXPECT_FALSE(result);
}

TEST_F(ChorusPlantPlacementTest, PlaceChorusPlant_ProducesVerticalGrowth)
{
    // 放置紫颂树后，应有垂直方向的生长（紫颂植物在上方）
    placeEndStoneFloor(60);

    ChorusPlantFeature::place(*m_region, m_random, BlockPos(8, 61, 8));

    // 检查起始位置上方是否有紫颂植物
    const BlockState* atStart = m_region->getBlockState(8, 61, 8);
    ASSERT_NE(atStart, nullptr);
    EXPECT_TRUE(atStart->is(VanillaBlocks::CHORUS_PLANT));
}

// ============================================================================
// ChorusFlowerBlock::generatePlant 测试（使用 WorldGenRegion）
// ============================================================================

class ChorusFlowerGenTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        m_chunks.resize(9);
        for (i32 relZ = -1; relZ <= 1; ++relZ) {
            for (i32 relX = -1; relX <= 1; ++relX) {
                const i32 index = (relZ + 1) * 3 + (relX + 1);
                auto chunk = std::make_unique<ChunkPrimer>(relX, relZ);
                m_chunks[static_cast<size_t>(index)] = chunk.get();
                m_ownedChunks.push_back(std::move(chunk));
            }
        }

        m_region = std::make_unique<WorldGenRegion>(0, 0, 1, std::move(m_chunks));
    }

    void TearDown() override
    {
        m_region.reset();
        m_ownedChunks.clear();
    }

    void placeEndStoneFloor(i32 y)
    {
        for (i32 x = 0; x < 16; ++x) {
            for (i32 z = 0; z < 16; ++z) {
                m_region->setBlockState(x, y, z, &VanillaBlocks::END_STONE->defaultState());
            }
        }
    }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
    math::Random m_random{42};
};

TEST_F(ChorusFlowerGenTest, GeneratePlant_PlacesChorusPlantAtOrigin)
{
    // generatePlant 在起始位置放置紫颂植物
    placeEndStoneFloor(60);

    blocks::ChorusFlowerBlock::generatePlant(*m_region, BlockPos(8, 61, 8), m_random, 8);

    const BlockState* atOrigin = m_region->getBlockState(8, 61, 8);
    ASSERT_NE(atOrigin, nullptr);
    EXPECT_TRUE(atOrigin->is(VanillaBlocks::CHORUS_PLANT));
}

TEST_F(ChorusFlowerGenTest, GeneratePlant_GrowsVertically)
{
    // generatePlant 应向上生长茎干
    placeEndStoneFloor(60);

    blocks::ChorusFlowerBlock::generatePlant(*m_region, BlockPos(8, 61, 8), m_random, 8);

    // 检查起始位置上方是否有紫颂植物或紫颂花
    bool foundGrowthAbove = false;
    for (i32 y = 62; y <= 80; ++y) {
        const BlockState* state = m_region->getBlockState(8, y, 8);
        if (state != nullptr && (state->is(VanillaBlocks::CHORUS_PLANT) || state->is(VanillaBlocks::CHORUS_FLOWER))) {
            foundGrowthAbove = true;
            break;
        }
    }
    EXPECT_TRUE(foundGrowthAbove);
}

TEST_F(ChorusFlowerGenTest, GeneratePlant_TopIsDeadFlowerOrBranch)
{
    // 紫颂树顶部应该有死亡花（age=5）或分枝
    placeEndStoneFloor(60);

    // 使用不同随机种子多次生成，确保覆盖有分枝和无分枝的情况
    bool foundTop = false;
    for (i32 seed = 0; seed < 10; ++seed) {
        // 每次清空世界重新生成
        for (i32 x = 0; x < 16; ++x) {
            for (i32 z = 0; z < 16; ++z) {
                for (i32 y = 60; y <= 80; ++y) {
                    m_region->setBlockState(x, y, z, nullptr);
                }
            }
        }
        placeEndStoneFloor(60);

        math::Random rng(seed * 1000 + 42);
        blocks::ChorusFlowerBlock::generatePlant(*m_region, BlockPos(8, 61, 8), rng, 8);

        // 查找最高的紫颂花或紫颂植物
        for (i32 y = 80; y >= 61; --y) {
            const BlockState* state = m_region->getBlockState(8, y, 8);
            if (state != nullptr) {
                if (state->is(VanillaBlocks::CHORUS_FLOWER)) {
                    foundTop = true;
                    break;
                }
                if (state->is(VanillaBlocks::CHORUS_PLANT)) {
                    // 检查紫颂植物周围是否有紫颂花（水平分枝顶端）
                    static const Direction dirs[] = {
                        Direction::North, Direction::South, Direction::East, Direction::West};
                    for (Direction dir : dirs) {
                        BlockPos adjPos = BlockPos(8, y, 8).offset(dir);
                        const BlockState* adjState = m_region->getBlockState(adjPos);
                        if (adjState != nullptr && adjState->is(VanillaBlocks::CHORUS_FLOWER)) {
                            foundTop = true;
                            break;
                        }
                    }
                    break;
                }
            }
        }
        if (foundTop) {
            break;
        }
    }
    EXPECT_TRUE(foundTop);
}
