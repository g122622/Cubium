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

#include "common/world/block/registry/VanillaBlocks.hpp"
#include "core/Constants.hpp"
#include "util/property/Properties.hpp"
#include "world/chunk/data/ChunkPrimer.hpp"
#include "world/gen/chunk/IChunkGenerator.hpp"
#include "world/gen/feature/ocean/BlueIceFeature.hpp"
#include "world/gen/feature/ocean/CoralFeature.hpp"
#include "world/gen/feature/ocean/KelpFeature.hpp"
#include "world/gen/feature/ocean/SeaPickleFeature.hpp"
#include "world/gen/feature/ocean/SeagrassFeature.hpp"

#include <array>
#include <memory>
#include <vector>

using namespace mc;

class OceanFeatureWorldTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        m_chunks.resize(9);
        for (i32 relZ = -1; relZ <= 1; ++relZ) {
            for (i32 relX = -1; relX <= 1; ++relX) {
                const i32 index = (relZ + 1) * 3 + (relX + 1);
                auto chunk = std::make_unique<ChunkPrimer>(relX, relZ);

                // 构建海底测试场景：y=40 为地面，y=41..62 为水层。
                for (i32 x = 0; x < 16; ++x) {
                    for (i32 z = 0; z < 16; ++z) {
                        chunk->setBlockState(x, 40, z, &VanillaBlocks::SAND->defaultState());
                        for (i32 y = 41; y <= 62; ++y) {
                            chunk->setBlockState(x, y, z, &VanillaBlocks::WATER->defaultState());
                        }
                    }
                }

                m_chunks[static_cast<size_t>(index)] = chunk.get();
                m_ownedChunks.push_back(std::move(chunk));
            }
        }

        m_region = std::make_unique<WorldGenRegion>(0, 0, 1, std::move(m_chunks));
    }

    void setWorldBlock(i32 x, i32 y, i32 z, const BlockState* state)
    {
        ASSERT_TRUE(m_region->setBlockState(x, y, z, state));
    }

    [[nodiscard]] const BlockState* getWorldBlock(i32 x, i32 y, i32 z) const
    {
        return m_region->getBlockState(x, y, z);
    }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
};

TEST_F(OceanFeatureWorldTest, KelpFeaturePlacesKelpInWater)
{
    // 数据驱动迁移：原 KelpFeatures::createColdKelp() 胶水已删除，
    // 测试直接内联构造 KelpFeatureConfig（与原工厂同值：kelp_plant/kelp, tries=120, maxH=10）。
    KelpFeatureConfig config;
    if (VanillaBlocks::KELP_PLANT != nullptr && VanillaBlocks::KELP != nullptr) {
        config.kelpState = &VanillaBlocks::KELP_PLANT->defaultState();
        config.kelpTopState = &VanillaBlocks::KELP->defaultState();
    }
    config.tries = 120;
    config.maxHeight = 10;

    ASSERT_NE(config.kelpState, nullptr);
    ASSERT_NE(config.kelpTopState, nullptr);

    KelpFeature feature;
    math::Random random(12345);

    EXPECT_TRUE(feature.place(*m_region, random, BlockPos(0, 0, 0), config));

    bool foundKelp = false;
    for (i32 x = 0; x < 16 && !foundKelp; ++x) {
        for (i32 z = 0; z < 16 && !foundKelp; ++z) {
            for (i32 y = 41; y <= 62; ++y) {
                const BlockState* planted = getWorldBlock(x, y, z);
                if (planted == nullptr) {
                    continue;
                }
                const bool isKelp = (VanillaBlocks::KELP != nullptr && planted->is(VanillaBlocks::KELP));
                const bool isKelpPlant =
                    (VanillaBlocks::KELP_PLANT != nullptr && planted->is(VanillaBlocks::KELP_PLANT));
                if (isKelp || isKelpPlant) {
                    foundKelp = true;
                    break;
                }
            }
        }
    }
    EXPECT_TRUE(foundKelp);
}

TEST_F(OceanFeatureWorldTest, SeagrassMixedFeaturePlacesSeaPlant)
{
    // 数据驱动迁移：原 SeagrassFeatures::createMixedSeagrass() 胶水已删除，
    // 测试直接内联构造 SeagrassFeatureConfig（与原工厂同值：tallChance=0.3, tries=48, spread=8）。
    SeagrassFeatureConfig config;
    if (VanillaBlocks::SEAGRASS != nullptr) {
        config.seagrassState = &VanillaBlocks::SEAGRASS->defaultState();
    }
    if (VanillaBlocks::TALL_SEAGRASS != nullptr) {
        config.tallSeagrassLowerState = &VanillaBlocks::TALL_SEAGRASS->defaultState().with(
            BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Lower);
        config.tallSeagrassUpperState = &VanillaBlocks::TALL_SEAGRASS->defaultState().with(
            BlockStateProperties::DOUBLE_BLOCK_HALF(), BlockStateProperties::DoubleBlockHalf::Upper);
    }
    config.tallSeagrassChance = 0.3f;
    config.tries = 48;
    config.horizontalSpread = 8;

    ASSERT_NE(config.seagrassState, nullptr);
    ASSERT_NE(config.tallSeagrassLowerState, nullptr);
    ASSERT_NE(config.tallSeagrassUpperState, nullptr);

    SeagrassFeature feature;
    math::Random random(22334);

    EXPECT_TRUE(feature.place(*m_region, random, BlockPos(0, 0, 0), config));

    bool foundSeagrass = false;
    for (i32 x = 0; x < 16 && !foundSeagrass; ++x) {
        for (i32 z = 0; z < 16 && !foundSeagrass; ++z) {
            for (i32 y = 41; y <= 42; ++y) {
                const BlockState* planted = getWorldBlock(x, y, z);
                if (planted == nullptr) {
                    continue;
                }
                const bool isSeagrass = (VanillaBlocks::SEAGRASS != nullptr && planted->is(VanillaBlocks::SEAGRASS));
                const bool isTallSeagrass =
                    (VanillaBlocks::TALL_SEAGRASS != nullptr && planted->is(VanillaBlocks::TALL_SEAGRASS));
                if (isSeagrass || isTallSeagrass) {
                    foundSeagrass = true;
                    break;
                }
            }
        }
    }
    EXPECT_TRUE(foundSeagrass);
}

TEST_F(OceanFeatureWorldTest, CoralFeaturePlacesConfiguredCoralBlock)
{
    CoralFeature feature;
    CoralFeatureConfig config(blocks::CoralColor::Tube, true);
    math::Random random(99887);

    ASSERT_NE(VanillaBlocks::TUBE_CORAL_BLOCK, nullptr);
    EXPECT_TRUE(feature.place(*m_region, random, BlockPos(0, 0, 0), config));

    bool foundCoralBlock = false;
    for (i32 x = 0; x < 16 && !foundCoralBlock; ++x) {
        for (i32 z = 0; z < 16 && !foundCoralBlock; ++z) {
            for (i32 y = 41; y <= 70; ++y) {
                const BlockState* state = getWorldBlock(x, y, z);
                if (state != nullptr && state->is(VanillaBlocks::TUBE_CORAL_BLOCK)) {
                    foundCoralBlock = true;
                    break;
                }
            }
        }
    }
    EXPECT_TRUE(foundCoralBlock);
}

TEST_F(OceanFeatureWorldTest, CoralFeaturePlacesDeadCoralBlock)
{
    CoralFeature feature;
    CoralFeatureConfig config(blocks::CoralColor::Tube, true, true);
    math::Random random(77889);

    ASSERT_NE(VanillaBlocks::DEAD_TUBE_CORAL_BLOCK, nullptr);
    EXPECT_TRUE(feature.place(*m_region, random, BlockPos(0, 0, 0), config));

    bool foundDeadCoralBlock = false;
    for (i32 x = 0; x < 16 && !foundDeadCoralBlock; ++x) {
        for (i32 z = 0; z < 16 && !foundDeadCoralBlock; ++z) {
            for (i32 y = 41; y <= 70; ++y) {
                const BlockState* state = getWorldBlock(x, y, z);
                if (state != nullptr && state->is(VanillaBlocks::DEAD_TUBE_CORAL_BLOCK)) {
                    foundDeadCoralBlock = true;
                    break;
                }
            }
        }
    }
    EXPECT_TRUE(foundDeadCoralBlock);
}

TEST_F(OceanFeatureWorldTest, SeaPickleFeaturePlacesOnSolidOceanFloor)
{
    // 数据驱动迁移：原 SeaPickleFeatures::createNormalSeaPickle() 胶水已删除，
    // 测试直接内联构造 SeaPickleFeatureConfig（tries=20, maxCount=4）。
    SeaPickleFeatureConfig config;
    if (VanillaBlocks::SEA_PICKLE != nullptr) {
        config.seaPickleState = &VanillaBlocks::SEA_PICKLE->defaultState();
    }
    config.tries = 20;
    config.maxCount = 4;

    ASSERT_NE(config.seaPickleState, nullptr);

    SeaPickleFeature feature;
    math::Random random(33445);

    EXPECT_TRUE(feature.place(*m_region, random, BlockPos(0, 0, 0), config));
}

TEST_F(OceanFeatureWorldTest, SeaPickleFeaturePlacesOnLivingCoral)
{
    ASSERT_NE(VanillaBlocks::TUBE_CORAL_BLOCK, nullptr);
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            setWorldBlock(x, 40, z, &VanillaBlocks::TUBE_CORAL_BLOCK->defaultState());
        }
    }

    // 数据驱动迁移：原 SeaPickleFeatures::createNormalSeaPickle() 胶水已删除，
    // 测试直接内联构造 SeaPickleFeatureConfig（tries=20, maxCount=4）。
    SeaPickleFeatureConfig config;
    if (VanillaBlocks::SEA_PICKLE != nullptr) {
        config.seaPickleState = &VanillaBlocks::SEA_PICKLE->defaultState();
    }
    config.tries = 20;
    config.maxCount = 4;

    SeaPickleFeature feature;
    math::Random random(44556);

    EXPECT_TRUE(feature.place(*m_region, random, BlockPos(0, 0, 0), config));

    ASSERT_NE(VanillaBlocks::SEA_PICKLE, nullptr);
    bool foundSeaPickle = false;
    for (i32 x = 0; x < 16 && !foundSeaPickle; ++x) {
        for (i32 z = 0; z < 16 && !foundSeaPickle; ++z) {
            const BlockState* state = getWorldBlock(x, 41, z);
            if (state != nullptr && state->is(VanillaBlocks::SEA_PICKLE)) {
                foundSeaPickle = true;
            }
        }
    }
    EXPECT_TRUE(foundSeaPickle);
}

TEST_F(OceanFeatureWorldTest, BlueIceFeaturePlacesBlueIceInWater)
{
    ASSERT_NE(VanillaBlocks::PACKED_ICE, nullptr);

    // 与特征内部一致地预采样起点，保证存在一个打包冰邻居。
    math::Random probeRandom(1234567);
    const i32 startX = probeRandom.nextInt(16);
    const i32 startZ = probeRandom.nextInt(16);

    i32 oceanFloorY = m_region->getTopBlockY(startX, startZ, HeightmapType::OceanFloorWG);
    if (oceanFloorY <= 0) {
        for (i32 y = mc::world::MAX_BUILD_HEIGHT - 1; y >= mc::world::MIN_BUILD_HEIGHT + 1; --y) {
            const BlockState* state = getWorldBlock(startX, y, startZ);
            if (state == nullptr || state->isAir()) {
                continue;
            }
            if (VanillaBlocks::WATER != nullptr && state->is(VanillaBlocks::WATER)) {
                continue;
            }
            oceanFloorY = y;
            break;
        }
    }
    ASSERT_GT(oceanFloorY, 0);
    const i32 startY = oceanFloorY + 1;

    const i32 packedX = (startX < 15) ? (startX + 1) : (startX - 1);
    setWorldBlock(packedX, startY, startZ, &VanillaBlocks::PACKED_ICE->defaultState());

    // 数据驱动迁移：原 BlueIceFeatures::createBlueIce() 胶水已删除，
    // 测试直接内联构造 BlueIceFeatureConfig（spreadAttempts 由测试覆盖为 120）。
    BlueIceFeatureConfig config;
    config.blueIceState = VanillaBlocks::getState(VanillaBlocks::BLUE_ICE);
    config.packedIceState = VanillaBlocks::getState(VanillaBlocks::PACKED_ICE);
    config.spreadAttempts = 120;

    BlueIceFeature feature;
    math::Random random(1234567);

    EXPECT_TRUE(feature.place(*m_region, random, BlockPos(0, 0, 0), config, 63));

    ASSERT_NE(VanillaBlocks::BLUE_ICE, nullptr);
    bool foundBlueIce = false;
    for (i32 x = 0; x < 16 && !foundBlueIce; ++x) {
        for (i32 z = 0; z < 16 && !foundBlueIce; ++z) {
            for (i32 y = 41; y <= 62; ++y) {
                const BlockState* state = getWorldBlock(x, y, z);
                if (state != nullptr && state->is(VanillaBlocks::BLUE_ICE)) {
                    foundBlueIce = true;
                    break;
                }
            }
        }
    }

    EXPECT_TRUE(foundBlueIce);
}

TEST_F(OceanFeatureWorldTest, BlueIceFeatureFailsWithoutPackedIceNeighbor)
{
    // 数据驱动迁移：原 BlueIceFeatures::createBlueIce() 胶水已删除，
    // 测试直接内联构造 BlueIceFeatureConfig（默认 spreadAttempts=200）。
    BlueIceFeatureConfig config;
    config.blueIceState = VanillaBlocks::getState(VanillaBlocks::BLUE_ICE);
    config.packedIceState = VanillaBlocks::getState(VanillaBlocks::PACKED_ICE);

    BlueIceFeature feature;
    math::Random random(1234567);

    EXPECT_FALSE(feature.place(*m_region, random, BlockPos(0, 0, 0), config, 63));
}
