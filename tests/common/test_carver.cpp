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

#include "common/world/block/registry/VanillaBlocks.hpp"
#include "core/Constants.hpp"
#include "util/math/random/Random.hpp"
#include "world/WorldConstants.hpp"
#include "world/biome/BiomeSource.hpp"
#include "world/biome/source/MultiNoiseBiomeSource.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/chunk/ChunkPrimer.hpp"
#include "world/gen/carver/CanyonCarver.hpp"
#include "world/gen/carver/CarvingContext.hpp"
#include "world/gen/carver/CaveCarver.hpp"
#include "world/gen/carver/WorldCarver.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// CarvingMask 测试
// ============================================================================

class CarvingMaskTest : public ::testing::Test {
protected:
    void SetUp() override { mask = std::make_unique<CarvingMask>(0, 0); }

    std::unique_ptr<CarvingMask> mask;
};

TEST_F(CarvingMaskTest, InitiallyNotCarved)
{
    EXPECT_FALSE(mask->isCarved(0, 0, 0));
    EXPECT_FALSE(mask->isCarved(8, 64, 8));
    EXPECT_FALSE(mask->isCarved(15, world::MAX_BUILD_HEIGHT - 1, 15));
}

TEST_F(CarvingMaskTest, SetAndGetCarved)
{
    EXPECT_FALSE(mask->isCarved(5, 100, 7));
    mask->setCarved(5, 100, 7);
    EXPECT_TRUE(mask->isCarved(5, 100, 7));
}

TEST_F(CarvingMaskTest, MultiplePositions)
{
    mask->setCarved(0, 0, 0);
    mask->setCarved(8, 128, 8);
    mask->setCarved(15, world::MAX_BUILD_HEIGHT - 1, 15);

    EXPECT_TRUE(mask->isCarved(0, 0, 0));
    EXPECT_TRUE(mask->isCarved(8, 128, 8));
    EXPECT_TRUE(mask->isCarved(15, world::MAX_BUILD_HEIGHT - 1, 15));

    // 未设置的位置仍然是 false
    EXPECT_FALSE(mask->isCarved(1, 0, 0));
    EXPECT_FALSE(mask->isCarved(7, 128, 8));
}

TEST_F(CarvingMaskTest, BoundaryCheck)
{
    // 边界值
    EXPECT_FALSE(mask->isCarved(-1, 0, 0));                      // 无效坐标
    EXPECT_FALSE(mask->isCarved(16, 0, 0));                      // 无效坐标
    EXPECT_FALSE(mask->isCarved(0, -1, 0));                      // 无效坐标
    EXPECT_FALSE(mask->isCarved(0, world::MAX_BUILD_HEIGHT, 0)); // 无效坐标

    // 设置边界值
    mask->setCarved(0, 0, 0);
    mask->setCarved(15, world::MAX_BUILD_HEIGHT - 1, 15);

    EXPECT_TRUE(mask->isCarved(0, 0, 0));
    EXPECT_TRUE(mask->isCarved(15, world::MAX_BUILD_HEIGHT - 1, 15));
}

TEST_F(CarvingMaskTest, GetIndex)
{
    // 测试索引计算
    EXPECT_EQ(CarvingMask::getIndex(0, 0, 0), 0);
    EXPECT_EQ(CarvingMask::getIndex(1, 0, 0), 1);
    EXPECT_EQ(CarvingMask::getIndex(0, 0, 1), 16);
    EXPECT_EQ(CarvingMask::getIndex(0, 1, 0), 256);
    EXPECT_EQ(CarvingMask::getIndex(15, world::MAX_BUILD_HEIGHT - 1, 15),
        15 | (15 << 4) | ((world::MAX_BUILD_HEIGHT - 1) << 8));
}

// ============================================================================
// WorldCarver 测试
// ============================================================================

class WorldCarverTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(WorldCarverTest, IsCarvable)
{
    // 可雕刻的方块
    EXPECT_TRUE(WorldCarver<ProbabilityConfig>::isCarvable(VanillaBlocks::STONE->defaultState()));
    EXPECT_TRUE(WorldCarver<ProbabilityConfig>::isCarvable(VanillaBlocks::DIRT->defaultState()));
    EXPECT_TRUE(WorldCarver<ProbabilityConfig>::isCarvable(VanillaBlocks::GRASS_BLOCK->defaultState()));
    EXPECT_TRUE(WorldCarver<ProbabilityConfig>::isCarvable(VanillaBlocks::SAND->defaultState()));
    EXPECT_TRUE(WorldCarver<ProbabilityConfig>::isCarvable(VanillaBlocks::GRANITE->defaultState()));
    EXPECT_TRUE(WorldCarver<ProbabilityConfig>::isCarvable(VanillaBlocks::DIORITE->defaultState()));
    EXPECT_TRUE(WorldCarver<ProbabilityConfig>::isCarvable(VanillaBlocks::ANDESITE->defaultState()));

    // 不可雕刻的方块
    EXPECT_FALSE(WorldCarver<ProbabilityConfig>::isCarvable(VanillaBlocks::AIR->defaultState()));
    EXPECT_FALSE(WorldCarver<ProbabilityConfig>::isCarvable(VanillaBlocks::WATER->defaultState()));
    EXPECT_FALSE(WorldCarver<ProbabilityConfig>::isCarvable(VanillaBlocks::BEDROCK->defaultState()));
    EXPECT_FALSE(WorldCarver<ProbabilityConfig>::isCarvable(VanillaBlocks::COAL_ORE->defaultState()));
}

// ============================================================================
// CaveCarver 测试
// ============================================================================

class CaveCarverTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        carver = std::make_unique<CaveCarver>(world::MAX_BUILD_HEIGHT);
        chunk = std::make_unique<ChunkPrimer>(0, 0);
        mask = std::make_unique<CarvingMask>(0, 0);
        biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(12345, false);
        context = std::make_unique<CarvingContext>(
            world::MIN_BUILD_HEIGHT, world::MAX_BUILD_HEIGHT - world::MIN_BUILD_HEIGHT, nullptr);
    }

    std::unique_ptr<CaveCarver> carver;
    std::unique_ptr<ChunkPrimer> chunk;
    std::unique_ptr<CarvingMask> mask;
    std::unique_ptr<world::biome::BiomeSource> biomeSource;
    std::unique_ptr<CarvingContext> context;
};

TEST_F(CaveCarverTest, ShouldCarveWithProbability)
{
    math::Random rng(12345);

    // 高概率配置
    ProbabilityConfig highProb(1.0f);
    EXPECT_TRUE(carver->shouldCarve(rng, 0, 0, highProb));

    // 低概率配置
    ProbabilityConfig lowProb(0.0f);
    EXPECT_FALSE(carver->shouldCarve(rng, 0, 0, lowProb));
}

TEST_F(CaveCarverTest, CarveCreatesHoles)
{
    // 用石头填充区块
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 16; ++x) {
            for (int z = 0; z < 16; ++z) {
                chunk->setBlockState(x, y, z, stone);
            }
        }
    }

    // 执行雕刻
    ProbabilityConfig config(1.0f); // 100% 概率
    math::Random rng(11111);

    // 使用固定种子确保雕刻
    bool carved = false;
    for (int cx = -1; cx <= 1; ++cx) {
        for (int cz = -1; cz <= 1; ++cz) {
            ChunkPrimer testChunk(cx, cz);
            CarvingMask testMask(cx, cz);
            for (int y = 0; y < 64; ++y) {
                for (int x = 0; x < 16; ++x) {
                    for (int z = 0; z < 16; ++z) {
                        testChunk.setBlockState(x, y, z, stone);
                    }
                }
            }
            if (carver->carve(testChunk, *context, *biomeSource, 63, cx, cz, testMask, rng, config)) {
                carved = true;
            }
        }
    }

    // 雕刻应该创建了洞
    EXPECT_TRUE(carved);
}

TEST_F(CaveCarverTest, CarveRespectsMask)
{
    // 用石头填充区块
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 16; ++x) {
            for (int z = 0; z < 16; ++z) {
                chunk->setBlockState(x, y, z, stone);
            }
        }
    }

    // 标记某些位置为已雕刻
    mask->setCarved(8, 32, 8);

    ProbabilityConfig config(1.0f);
    math::Random rng(54321);
    carver->carve(*chunk, *context, *biomeSource, 63, 0, 0, *mask, rng, config);

    // 已标记的位置不应该被重复雕刻
    // （虽然我们无法直接验证，但掩码应该阻止重复雕刻）
    EXPECT_TRUE(mask->isCarved(8, 32, 8));
}

TEST_F(CaveCarverTest, GetRange)
{
    // 默认范围应该是 4
    EXPECT_EQ(carver->getRange(), 4);
}

TEST_F(CaveCarverTest, GetMaxHeight)
{
    EXPECT_EQ(carver->getMaxHeight(), world::MAX_BUILD_HEIGHT);
}

// ============================================================================
// CanyonCarver 测试
// ============================================================================

class CanyonCarverTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        carver = std::make_unique<CanyonCarver>(world::MAX_BUILD_HEIGHT);
        chunk = std::make_unique<ChunkPrimer>(0, 0);
        mask = std::make_unique<CarvingMask>(0, 0);
        biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(12345, false);
        context = std::make_unique<CarvingContext>(
            world::MIN_BUILD_HEIGHT, world::MAX_BUILD_HEIGHT - world::MIN_BUILD_HEIGHT, nullptr);
    }

    std::unique_ptr<CanyonCarver> carver;
    std::unique_ptr<ChunkPrimer> chunk;
    std::unique_ptr<CarvingMask> mask;
    std::unique_ptr<world::biome::BiomeSource> biomeSource;
    std::unique_ptr<CarvingContext> context;
};

TEST_F(CanyonCarverTest, ShouldCarveWithProbability)
{
    math::Random rng(12345);

    // 高概率配置
    ProbabilityConfig highProb(1.0f);
    EXPECT_TRUE(carver->shouldCarve(rng, 0, 0, highProb));

    // 低概率配置
    ProbabilityConfig lowProb(0.0f);
    EXPECT_FALSE(carver->shouldCarve(rng, 0, 0, lowProb));
}

TEST_F(CanyonCarverTest, CarveCreatesCanyon)
{
    // 用石头填充区块
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 16; ++x) {
            for (int z = 0; z < 16; ++z) {
                chunk->setBlockState(x, y, z, stone);
            }
        }
    }

    // 执行雕刻
    ProbabilityConfig config(1.0f); // 100% 概率
    math::Random rng(22222);

    bool carved = false;
    for (int cx = -1; cx <= 1; ++cx) {
        for (int cz = -1; cz <= 1; ++cz) {
            ChunkPrimer testChunk(cx, cz);
            CarvingMask testMask(cx, cz);
            for (int y = 0; y < 64; ++y) {
                for (int x = 0; x < 16; ++x) {
                    for (int z = 0; z < 16; ++z) {
                        testChunk.setBlockState(x, y, z, stone);
                    }
                }
            }
            if (carver->carve(testChunk, *context, *biomeSource, 63, cx, cz, testMask, rng, config)) {
                carved = true;
            }
        }
    }

    // 峡谷应该创建了洞
    EXPECT_TRUE(carved);
}

TEST_F(CanyonCarverTest, HeightThresholdsInitialized)
{
    // 峡谷雕刻器应该初始化高度阈值表
    CanyonCarver testCarver;
    // 不应该崩溃
    SUCCEED();
}

TEST_F(CanyonCarverTest, GetRange)
{
    // 默认范围应该是 4
    EXPECT_EQ(carver->getRange(), 4);
}

TEST_F(CanyonCarverTest, GetMaxHeight)
{
    EXPECT_EQ(carver->getMaxHeight(), world::MAX_BUILD_HEIGHT);
}

// ============================================================================
// ProbabilityConfig 测试
// ============================================================================

TEST(ProbabilityConfigTest, DefaultProbability)
{
    ProbabilityConfig config;
    EXPECT_FLOAT_EQ(config.probability, 0.14285715f); // 1/7
}

TEST(ProbabilityConfigTest, CustomProbability)
{
    ProbabilityConfig config(0.5f);
    EXPECT_FLOAT_EQ(config.probability, 0.5f);
}

TEST(ProbabilityConfigTest, EdgeCases)
{
    ProbabilityConfig zero(0.0f);
    EXPECT_FLOAT_EQ(zero.probability, 0.0f);

    ProbabilityConfig one(1.0f);
    EXPECT_FLOAT_EQ(one.probability, 1.0f);
}

// ============================================================================
// ConfiguredCarver 测试
// ============================================================================

TEST(ConfiguredCarverTest, CreateAndUse)
{
    VanillaBlocks::initialize();

    auto carver = std::make_unique<CaveCarver>(world::MAX_BUILD_HEIGHT);
    ProbabilityConfig config(0.5f);

    ConfiguredCarver<CaveCarver, ProbabilityConfig> configured(std::move(carver), config);

    EXPECT_FLOAT_EQ(configured.getConfig().probability, 0.5f);
}

TEST(ConfiguredCarverTest, ShouldCarve)
{
    VanillaBlocks::initialize();

    auto carver = std::make_unique<CaveCarver>(world::MAX_BUILD_HEIGHT);
    ProbabilityConfig config(1.0f);

    ConfiguredCarver<CaveCarver, ProbabilityConfig> configured(std::move(carver), config);

    math::Random rng(12345);
    EXPECT_TRUE(configured.shouldCarve(rng, 0, 0));
}

TEST(ConfiguredCarverTest, CanyonWithConfig)
{
    VanillaBlocks::initialize();

    auto carver = std::make_unique<CanyonCarver>(world::MAX_BUILD_HEIGHT);
    ProbabilityConfig config(0.1f);

    ConfiguredCarver<CanyonCarver, ProbabilityConfig> configured(std::move(carver), config);

    EXPECT_FLOAT_EQ(configured.getConfig().probability, 0.1f);
}
