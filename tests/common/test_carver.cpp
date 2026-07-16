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

#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/carver/CanyonCarver.hpp"
#include "common/world/gen/carver/CarverConfiguration.hpp"
#include "common/world/gen/carver/CarvingContext.hpp"
#include "common/world/gen/carver/CarvingMask.hpp"
#include "common/world/gen/carver/CaveCarver.hpp"
#include "common/world/gen/carver/NetherWorldCarver.hpp"
#include "common/world/gen/carver/WorldCarver.hpp"
#include "common/world/gen/surface/SurfaceRules.hpp"
#include "common/world/gen/valueprovider/FloatProvider.hpp"
#include "common/world/gen/valueprovider/HeightProvider.hpp"
#include "core/Constants.hpp"
#include "util/math/random/Random.hpp"
#include "world/WorldConstants.hpp"
#include "world/biome/BiomeSource.hpp"
#include "world/biome/source/MultiNoiseBiomeSource.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/chunk/data/ChunkPrimer.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::gen::valueprovider;
using namespace mc::world::gen::surface;

// ============================================================================
// CarvingMask 测试
// ============================================================================

class CarvingMaskTest : public ::testing::Test {
protected:
    void SetUp() override { mask = std::make_unique<CarvingMask>(0, 0, world::MIN_BUILD_HEIGHT, world::CHUNK_HEIGHT); }

    std::unique_ptr<CarvingMask> mask;
};

TEST_F(CarvingMaskTest, InitiallyNotCarved)
{
    EXPECT_FALSE(mask->isCarved(0, 0, 0));
    EXPECT_FALSE(mask->isCarved(8, 64, 8));
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
    mask->setCarved(15, world::MIN_BUILD_HEIGHT + world::CHUNK_HEIGHT - 1, 15);

    EXPECT_TRUE(mask->isCarved(0, 0, 0));
    EXPECT_TRUE(mask->isCarved(8, 128, 8));
    EXPECT_TRUE(mask->isCarved(15, world::MIN_BUILD_HEIGHT + world::CHUNK_HEIGHT - 1, 15));

    // 未设置的位置仍然是 false
    EXPECT_FALSE(mask->isCarved(1, 0, 0));
    EXPECT_FALSE(mask->isCarved(7, 128, 8));
}

TEST_F(CarvingMaskTest, BoundaryCheck)
{
    // 边界值
    EXPECT_FALSE(mask->isCarved(-1, 0, 0)); // 无效坐标
    EXPECT_FALSE(mask->isCarved(16, 0, 0)); // 无效坐标

    // 设置边界值
    mask->setCarved(0, 0, 0);
    mask->setCarved(15, world::MIN_BUILD_HEIGHT + world::CHUNK_HEIGHT - 1, 15);

    EXPECT_TRUE(mask->isCarved(0, 0, 0));
    EXPECT_TRUE(mask->isCarved(15, world::MIN_BUILD_HEIGHT + world::CHUNK_HEIGHT - 1, 15));
}

// ============================================================================
// WorldCarver 测试
// ============================================================================

class WorldCarverTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(WorldCarverTest, CanReplaceBlockWithTag)
{
    CaveCarver carver;
    const BlockTag* replaceable = &BlockTags::OVERWORLD_CARVER_REPLACEABLES();
    // 手构造洞穴配置（原 ConfiguredCarvers::createOverworldCaveConfig 已随数据驱动迁移删除）
    CaveCarverConfiguration config(0.15f,
        UniformHeight::create(VerticalAnchor::aboveBottom(8), VerticalAnchor::absolute(180)),
        UniformFloat::create(0.1f, 0.9f),
        VerticalAnchor::aboveBottom(8),
        replaceable,
        UniformFloat::create(0.7f, 1.4f),
        UniformFloat::create(0.8f, 1.3f),
        UniformFloat::create(-1.0f, -0.4f));

    // 可雕刻的方块（在 OVERWORLD_CARVER_REPLACEABLES tag 中）
    EXPECT_TRUE(carver.canReplaceBlock(*VanillaBlocks::getState(VanillaBlocks::STONE), config));
    EXPECT_TRUE(carver.canReplaceBlock(*VanillaBlocks::getState(VanillaBlocks::DIRT), config));
    EXPECT_TRUE(carver.canReplaceBlock(*VanillaBlocks::getState(VanillaBlocks::GRANITE), config));

    // 不可雕刻的方块
    EXPECT_FALSE(carver.canReplaceBlock(*VanillaBlocks::getState(VanillaBlocks::AIR), config));
    EXPECT_FALSE(carver.canReplaceBlock(*VanillaBlocks::getState(VanillaBlocks::WATER), config));
}

// ============================================================================
// CaveCarver 测试
// ============================================================================

class CaveCarverTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        carver = std::make_unique<CaveCarver>();
        chunk = std::make_unique<ChunkPrimer>(0, 0);
        mask = std::make_unique<CarvingMask>(0, 0, world::MIN_BUILD_HEIGHT, world::CHUNK_HEIGHT);
        auto settings = DimensionSettings::overworld();
        auto randomState = world::gen::RandomState::create(settings, 12345);
        biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        context = std::make_unique<CarvingContext>(world::MIN_BUILD_HEIGHT, world::CHUNK_HEIGHT, nullptr);
        // 手构造洞穴配置（原 ConfiguredCarvers::createOverworldCaveConfig 已随数据驱动迁移删除）
        config = CaveCarverConfiguration(0.15f,
            UniformHeight::create(VerticalAnchor::aboveBottom(8), VerticalAnchor::absolute(180)),
            UniformFloat::create(0.1f, 0.9f),
            VerticalAnchor::aboveBottom(8),
            &BlockTags::OVERWORLD_CARVER_REPLACEABLES(),
            UniformFloat::create(0.7f, 1.4f),
            UniformFloat::create(0.8f, 1.3f),
            UniformFloat::create(-1.0f, -0.4f));
    }

    std::unique_ptr<CaveCarver> carver;
    std::unique_ptr<ChunkPrimer> chunk;
    std::unique_ptr<CarvingMask> mask;
    std::unique_ptr<world::biome::BiomeSource> biomeSource;
    std::unique_ptr<CarvingContext> context;
    CaveCarverConfiguration config;
};

TEST_F(CaveCarverTest, ShouldCarveWithProbability)
{
    math::Random rng(12345);

    // 100% 概率
    CaveCarverConfiguration highProbConfig(1.0f,
        UniformHeight::create(VerticalAnchor::aboveBottom(8), VerticalAnchor::absolute(180)),
        UniformFloat::create(0.1f, 0.9f),
        VerticalAnchor::aboveBottom(8),
        &BlockTags::OVERWORLD_CARVER_REPLACEABLES(),
        UniformFloat::create(0.7f, 1.4f),
        UniformFloat::create(0.8f, 1.3f),
        UniformFloat::create(-1.0f, -0.4f));
    EXPECT_TRUE(carver->shouldCarve(rng, 0, 0, highProbConfig));

    // 0% 概率
    CaveCarverConfiguration lowProbConfig(0.0f,
        UniformHeight::create(VerticalAnchor::aboveBottom(8), VerticalAnchor::absolute(180)),
        UniformFloat::create(0.1f, 0.9f),
        VerticalAnchor::aboveBottom(8),
        &BlockTags::OVERWORLD_CARVER_REPLACEABLES(),
        UniformFloat::create(0.7f, 1.4f),
        UniformFloat::create(0.8f, 1.3f),
        UniformFloat::create(-1.0f, -0.4f));
    EXPECT_FALSE(carver->shouldCarve(rng, 0, 0, lowProbConfig));
}

TEST_F(CaveCarverTest, GetRange)
{
    // 默认范围应该是 4
    EXPECT_EQ(carver->getRange(), 4);
}

// ============================================================================
// CanyonCarver 测试
// ============================================================================

class CanyonCarverTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        carver = std::make_unique<CanyonCarver>();
        chunk = std::make_unique<ChunkPrimer>(0, 0);
        mask = std::make_unique<CarvingMask>(0, 0, world::MIN_BUILD_HEIGHT, world::CHUNK_HEIGHT);
        auto settings = DimensionSettings::overworld();
        auto randomState = world::gen::RandomState::create(settings, 12345);
        biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        context = std::make_unique<CarvingContext>(world::MIN_BUILD_HEIGHT, world::CHUNK_HEIGHT, nullptr);
        // 手构造峡谷配置（原 ConfiguredCarvers::createOverworldCanyonConfig 已随数据驱动迁移删除）
        config = CanyonCarverConfiguration(0.01f,
            UniformHeight::create(VerticalAnchor::absolute(10), VerticalAnchor::absolute(67)),
            ConstantFloat::create(3.0f),
            VerticalAnchor::aboveBottom(8),
            &BlockTags::OVERWORLD_CARVER_REPLACEABLES(),
            UniformFloat::create(-0.125f, 0.125f),
            CanyonShapeConfiguration(UniformFloat::create(0.75f, 1.0f),
                TrapezoidFloat::create(0.0f, 6.0f, 2.0f),
                3,
                UniformFloat::create(0.75f, 1.0f),
                1.0f,
                0.0f));
    }

    std::unique_ptr<CanyonCarver> carver;
    std::unique_ptr<ChunkPrimer> chunk;
    std::unique_ptr<CarvingMask> mask;
    std::unique_ptr<world::biome::BiomeSource> biomeSource;
    std::unique_ptr<CarvingContext> context;
    CanyonCarverConfiguration config;
};

TEST_F(CanyonCarverTest, ShouldCarveWithProbability)
{
    math::Random rng(12345);

    // 高概率配置
    CanyonCarverConfiguration highProbConfig(1.0f,
        UniformHeight::create(VerticalAnchor::absolute(10), VerticalAnchor::absolute(67)),
        ConstantFloat::create(3.0f),
        VerticalAnchor::aboveBottom(8),
        &BlockTags::OVERWORLD_CARVER_REPLACEABLES(),
        UniformFloat::create(-0.125f, 0.125f),
        CanyonShapeConfiguration(UniformFloat::create(0.75f, 1.0f),
            TrapezoidFloat::create(0.0f, 6.0f, 2.0f),
            3,
            UniformFloat::create(0.75f, 1.0f),
            1.0f,
            0.0f));
    EXPECT_TRUE(carver->shouldCarve(rng, 0, 0, highProbConfig));

    // 低概率配置
    CanyonCarverConfiguration lowProbConfig(0.0f,
        UniformHeight::create(VerticalAnchor::absolute(10), VerticalAnchor::absolute(67)),
        ConstantFloat::create(3.0f),
        VerticalAnchor::aboveBottom(8),
        &BlockTags::OVERWORLD_CARVER_REPLACEABLES(),
        UniformFloat::create(-0.125f, 0.125f),
        CanyonShapeConfiguration(UniformFloat::create(0.75f, 1.0f),
            TrapezoidFloat::create(0.0f, 6.0f, 2.0f),
            3,
            UniformFloat::create(0.75f, 1.0f),
            1.0f,
            0.0f));
    EXPECT_FALSE(carver->shouldCarve(rng, 0, 0, lowProbConfig));
}

TEST_F(CanyonCarverTest, GetRange)
{
    EXPECT_EQ(carver->getRange(), 4);
}

// ============================================================================
// NetherWorldCarver 测试
// ============================================================================

TEST(NetherWorldCarverTest, Construction)
{
    VanillaBlocks::initialize();
    NetherWorldCarver carver;
    // 验证下界雕刻器可以正常构造
    EXPECT_EQ(carver.getRange(), 4);
}

// ============================================================================
// ConfiguredCarver 测试
// ============================================================================

TEST(ConfiguredCarverTest, CreateAndUse)
{
    VanillaBlocks::initialize();

    auto carver = std::make_unique<CaveCarver>();
    // 手构造洞穴配置（原 ConfiguredCarvers::createOverworldCaveConfig 已随数据驱动迁移删除）
    auto config = CaveCarverConfiguration(0.15f,
        UniformHeight::create(VerticalAnchor::aboveBottom(8), VerticalAnchor::absolute(180)),
        UniformFloat::create(0.1f, 0.9f),
        VerticalAnchor::aboveBottom(8),
        &BlockTags::OVERWORLD_CARVER_REPLACEABLES(),
        UniformFloat::create(0.7f, 1.4f),
        UniformFloat::create(0.8f, 1.3f),
        UniformFloat::create(-1.0f, -0.4f));

    ConfiguredCarver<CaveCarver, CaveCarverConfiguration> configured(std::move(carver), std::move(config));

    EXPECT_FLOAT_EQ(configured.getConfig().probability, 0.15f);
}

TEST(ConfiguredCarverTest, ShouldCarve)
{
    VanillaBlocks::initialize();

    auto carver = std::make_unique<CaveCarver>();
    // 手构造洞穴配置（原 ConfiguredCarvers::createOverworldCaveConfig 已随数据驱动迁移删除）
    auto config = CaveCarverConfiguration(0.15f,
        UniformHeight::create(VerticalAnchor::aboveBottom(8), VerticalAnchor::absolute(180)),
        UniformFloat::create(0.1f, 0.9f),
        VerticalAnchor::aboveBottom(8),
        &BlockTags::OVERWORLD_CARVER_REPLACEABLES(),
        UniformFloat::create(0.7f, 1.4f),
        UniformFloat::create(0.8f, 1.3f),
        UniformFloat::create(-1.0f, -0.4f));

    ConfiguredCarver<CaveCarver, CaveCarverConfiguration> configured(std::move(carver), std::move(config));

    math::Random rng(12345);
    // 15% 概率，多次尝试应该有时成功有时失败
    bool anyTrue = false;
    bool anyFalse = false;
    for (int i = 0; i < 100; ++i) {
        if (configured.shouldCarve(rng, i, i)) {
            anyTrue = true;
        } else {
            anyFalse = true;
        }
    }
    // 至少各有一次
    EXPECT_TRUE(anyTrue);
    EXPECT_TRUE(anyFalse);
}
