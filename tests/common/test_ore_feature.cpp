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

#include "common/core/Constants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/DeepslateBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/feature/Feature.hpp"
#include "common/world/gen/feature/ore/OreFeature.hpp"
#include "common/world/gen/placement/Placement.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// RuleTest 测试
// ============================================================================

class RuleTestTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(RuleTestTest, StoneRuleTest)
{
    auto rule = createOreTarget(OreTargetType::NaturalStone);
    math::Random random(12345);

    // 石头应该匹配
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    ASSERT_NE(stone, nullptr);
    EXPECT_TRUE(rule->test(*stone, random));

    // 空气不应该匹配
    const BlockState* air = &VanillaBlocks::AIR->defaultState();
    ASSERT_NE(air, nullptr);
    EXPECT_FALSE(rule->test(*air, random));
}

TEST_F(RuleTestTest, BlockIdRuleTest)
{
    auto rule = createOreTarget(OreTargetType::Netherrack);
    math::Random random(12345);

    // 下界岩应该匹配
    const BlockState* netherrack = &VanillaBlocks::NETHERRACK->defaultState();
    if (netherrack) {
        EXPECT_TRUE(rule->test(*netherrack, random));
    }

    // 石头不应该匹配
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    ASSERT_NE(stone, nullptr);
    EXPECT_FALSE(rule->test(*stone, random));
}

// ============================================================================
// OreFeatureConfig 测试
// ============================================================================

class OreFeatureConfigTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(OreFeatureConfigTest, CreateConfig)
{
    auto config = std::make_unique<OreFeatureConfig>(
        createOreTarget(OreTargetType::NaturalStone), &VanillaBlocks::COAL_ORE->defaultState(), 17);

    EXPECT_FALSE(config->targets.empty());
    EXPECT_TRUE(config->targets[0].state->is(VanillaBlocks::COAL_ORE));
    EXPECT_EQ(config->size, 17);
    EXPECT_NE(config->targets[0].target, nullptr);
}

TEST_F(OreFeatureConfigTest, NaturalStoneTarget)
{
    auto target = OreFeatureConfig::naturalStone();
    EXPECT_NE(target, nullptr);
}

// ============================================================================
// Placement 测试
// ============================================================================

class PlacementTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(PlacementTest, CountPlacement)
{
    CountPlacement placement;
    CountPlacementConfig config(5); // 5次尝试
    ChunkPrimer chunk(0, 0);
    math::Random random(12345);

    // 创建一个简单的 WorldGenRegion 需要太多依赖，暂时跳过位置测试
    // 这里只测试配置
    EXPECT_EQ(config.count, 5);
}

TEST_F(PlacementTest, HeightRangePlacementConfig)
{
    HeightRangePlacementConfig config(world::MIN_BUILD_HEIGHT, 128, world::MAX_BUILD_HEIGHT);

    math::Random random(12345);
    for (int i = 0; i < 100; ++i) {
        i32 y = config.getRandomY(random);
        EXPECT_GE(y, world::MIN_BUILD_HEIGHT);
        EXPECT_LT(y, world::MAX_BUILD_HEIGHT);
    }
}

TEST_F(PlacementTest, HeightRangePlacementConfigUniform)
{
    auto config = HeightRangePlacementConfig::uniform(10, 50);

    math::Random random(12345);
    for (int i = 0; i < 100; ++i) {
        i32 y = config.getRandomY(random);
        EXPECT_GE(y, 10);
        EXPECT_LT(y, 50);
    }
}

TEST_F(PlacementTest, BiomePlacementConfig)
{
    BiomePlacementConfig config({1, 5, 10});

    EXPECT_TRUE(config.isAllowed(1));
    EXPECT_TRUE(config.isAllowed(5));
    EXPECT_TRUE(config.isAllowed(10));
    EXPECT_FALSE(config.isAllowed(2));
    EXPECT_FALSE(config.isAllowed(100));
}

TEST_F(PlacementTest, ChancePlacementConfig)
{
    ChancePlacementConfig config(0.5f); // 50% 概率

    math::Random random(12345);
    int success = 0;
    for (int i = 0; i < 1000; ++i) {
        if (random.nextFloat() < config.chance) {
            success++;
        }
    }

    // 应该接近 500 次（允许一定误差）
    EXPECT_GT(success, 400);
    EXPECT_LT(success, 600);
}

// ============================================================================
// SimpleBlockStateProvider 测试
// ============================================================================

class BlockStateProviderTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(BlockStateProviderTest, SimpleProvider)
{
    SimpleBlockStateProvider provider(&VanillaBlocks::STONE->defaultState());
    math::Random random(12345);

    const BlockState* state = provider.getState(random, 0, 0, 0);
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->is(VanillaBlocks::STONE));
}

TEST_F(BlockStateProviderTest, DifferentBlocks)
{
    SimpleBlockStateProvider grassProvider(&VanillaBlocks::GRASS_BLOCK->defaultState());
    SimpleBlockStateProvider dirtProvider(&VanillaBlocks::DIRT->defaultState());
    math::Random random(12345);

    const BlockState* grass = grassProvider.getState(random, 0, 0, 0);
    const BlockState* dirt = dirtProvider.getState(random, 0, 0, 0);

    ASSERT_NE(grass, nullptr);
    ASSERT_NE(dirt, nullptr);
    EXPECT_TRUE(grass->is(VanillaBlocks::GRASS_BLOCK));
    EXPECT_TRUE(dirt->is(VanillaBlocks::DIRT));
}

// ============================================================================
// OreFeature 测试
// ============================================================================

class OreFeatureTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        chunk = std::make_unique<ChunkPrimer>(0, 0);
        random = std::make_unique<math::Random>(12345);
    }

    void fillWithStone()
    {
        const BlockState* stone = &VanillaBlocks::STONE->defaultState();
        ASSERT_NE(stone, nullptr);

        for (int y = 0; y < 64; ++y) {
            for (int x = 0; x < 16; ++x) {
                for (int z = 0; z < 16; ++z) {
                    chunk->setBlockState(x, y, z, stone);
                }
            }
        }
    }

    std::unique_ptr<ChunkPrimer> chunk;
    std::unique_ptr<math::Random> random;
};

TEST_F(OreFeatureTest, CreateOreFeature)
{
    EXPECT_STREQ(OreFeature::name(), "ore");
}

TEST_F(OreFeatureTest, PlaceSmallOre)
{
    fillWithStone();

    auto config = std::make_unique<OreFeatureConfig>(createOreTarget(OreTargetType::NaturalStone),
        &VanillaBlocks::COAL_ORE->defaultState(),
        8); // 小矿脉

    // 注意：WorldGenRegion 需要完整实现才能测试 place 方法
    // 这里只验证配置正确创建
    EXPECT_FALSE(config->targets.empty());
    EXPECT_TRUE(config->targets[0].state->is(VanillaBlocks::COAL_ORE));
    EXPECT_EQ(config->size, 8);
}

// ============================================================================
// 多目标矿石配置测试（深板岩变体）
// ============================================================================

TEST_F(OreFeatureConfigTest, StoneAndDeepslateOreTargets)
{
    auto targets = OreFeatureConfig::stoneAndDeepslateOre(&VanillaBlocks::IRON_ORE->defaultState(),
        block_registry::DeepslateBlocks::DEEPSLATE_IRON_ORE
            ? &block_registry::DeepslateBlocks::DEEPSLATE_IRON_ORE->defaultState()
            : nullptr);

    EXPECT_EQ(targets.size(), 2u);

    // 第一个目标是石头规则
    EXPECT_NE(targets[0].target, nullptr);
    EXPECT_TRUE(targets[0].state->is(VanillaBlocks::IRON_ORE));

    // 第二个目标是深板岩规则
    EXPECT_NE(targets[1].target, nullptr);
    if (block_registry::DeepslateBlocks::DEEPSLATE_IRON_ORE) {
        EXPECT_TRUE(targets[1].state->is(block_registry::DeepslateBlocks::DEEPSLATE_IRON_ORE));
    }
}

TEST_F(OreFeatureConfigTest, MultiTargetOreConfig)
{
    auto config = std::make_unique<OreFeatureConfig>(
        OreFeatureConfig::stoneAndDeepslateOre(&VanillaBlocks::DIAMOND_ORE->defaultState(),
            block_registry::DeepslateBlocks::DEEPSLATE_DIAMOND_ORE
                ? &block_registry::DeepslateBlocks::DEEPSLATE_DIAMOND_ORE->defaultState()
                : nullptr),
        8);

    EXPECT_EQ(config->targets.size(), 2u);
    EXPECT_EQ(config->size, 8);

    // 石头应该匹配第一个目标
    math::Random random(42);
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    EXPECT_TRUE(config->targets[0].target->test(*stone, random));
    EXPECT_TRUE(config->targets[0].state->is(VanillaBlocks::DIAMOND_ORE));

    // 深板岩应该匹配第二个目标
    if (block_registry::DeepslateBlocks::DEEPSLATE) {
        const BlockState* deepslate = &block_registry::DeepslateBlocks::DEEPSLATE->defaultState();
        EXPECT_FALSE(config->targets[0].target->test(*deepslate, random)); // 石头规则不匹配深板岩
        EXPECT_TRUE(config->targets[1].target->test(*deepslate, random));  // 深板岩规则匹配深板岩
    }
}
