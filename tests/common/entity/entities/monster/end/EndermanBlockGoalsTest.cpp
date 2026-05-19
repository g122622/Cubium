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
 * @file EndermanBlockGoalsTest.cpp
 * @brief 末影人方块放置和拾取目标测试
 *
 * 测试末影人的方块操作功能：
 * - EndermanPlaceBlockGoal 放置方块目标
 * - EndermanTakeBlockGoal 拾取方块目标
 * - BlockTags::ENDERMAN_HOLDABLE 方块标签
 */

#include <gtest/gtest.h>

#include "common/entity/ai/goal/goals/special/EndermanGoals.hpp"
#include "common/entity/entities/monster/end/EndermanEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"

namespace mc {
namespace test {

// ==================== BlockTags::ENDERMAN_HOLDABLE 测试 ====================

class EndermanHoldableTagTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化方块标签
        BlockTags::initialize();
    }
};

TEST_F(EndermanHoldableTagTest, TagExists)
{
    // 标签应该存在
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_EQ(tag.getId(), ResourceLocation("minecraft", "enderman_holdable"));
}

TEST_F(EndermanHoldableTagTest, ContainsGrassBlock)
{
    // 草方块应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "grass_block")));
}

TEST_F(EndermanHoldableTagTest, ContainsDirt)
{
    // 泥土应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "dirt")));
}

TEST_F(EndermanHoldableTagTest, ContainsSand)
{
    // 沙子应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "sand")));
}

TEST_F(EndermanHoldableTagTest, ContainsRedSand)
{
    // 红沙应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "red_sand")));
}

TEST_F(EndermanHoldableTagTest, ContainsGravel)
{
    // 沙砾应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "gravel")));
}

TEST_F(EndermanHoldableTagTest, ContainsBrownMushroom)
{
    // 棕色蘑菇应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "brown_mushroom")));
}

TEST_F(EndermanHoldableTagTest, ContainsRedMushroom)
{
    // 红色蘑菇应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "red_mushroom")));
}

TEST_F(EndermanHoldableTagTest, ContainsTNT)
{
    // TNT应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "tnt")));
}

TEST_F(EndermanHoldableTagTest, ContainsCactus)
{
    // 仙人掌应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "cactus")));
}

TEST_F(EndermanHoldableTagTest, ContainsClay)
{
    // 黏土块应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "clay")));
}

TEST_F(EndermanHoldableTagTest, ContainsPumpkin)
{
    // 南瓜应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "pumpkin")));
}

TEST_F(EndermanHoldableTagTest, ContainsCarvedPumpkin)
{
    // 雕刻南瓜应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "carved_pumpkin")));
}

TEST_F(EndermanHoldableTagTest, ContainsMelon)
{
    // 西瓜应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "melon")));
}

TEST_F(EndermanHoldableTagTest, ContainsMycelium)
{
    // 菌丝体应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "mycelium")));
}

// 下界方块测试（1.16新增）
TEST_F(EndermanHoldableTagTest, ContainsCrimsonFungus)
{
    // 绯红菌应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "crimson_fungus")));
}

TEST_F(EndermanHoldableTagTest, ContainsCrimsonNylium)
{
    // 绯红菌岩应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "crimson_nylium")));
}

TEST_F(EndermanHoldableTagTest, ContainsWarpedFungus)
{
    // 诡异菌应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "warped_fungus")));
}

TEST_F(EndermanHoldableTagTest, ContainsWarpedNylium)
{
    // 诡异菌岩应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "warped_nylium")));
}

// 小花朵测试
TEST_F(EndermanHoldableTagTest, ContainsDandelion)
{
    // 蒲公英应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "dandelion")));
}

TEST_F(EndermanHoldableTagTest, ContainsPoppy)
{
    // 虞美人应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "poppy")));
}

TEST_F(EndermanHoldableTagTest, ContainsWitherRose)
{
    // 凋零玫瑰应该可以被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_TRUE(tag.contains(ResourceLocation("minecraft", "wither_rose")));
}

// 不能被拾取的方块测试
TEST_F(EndermanHoldableTagTest, DoesNotContainStone)
{
    // 石头不应该被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft", "stone")));
}

TEST_F(EndermanHoldableTagTest, DoesNotContainBedrock)
{
    // 基岩不应该被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft", "bedrock")));
}

TEST_F(EndermanHoldableTagTest, DoesNotContainWater)
{
    // 水不应该被末影人拾取
    auto& tag = BlockTags::ENDERMAN_HOLDABLE();
    EXPECT_FALSE(tag.contains(ResourceLocation("minecraft", "water")));
}

// ==================== EndermanEntity 持有方块测试 ====================

class EndermanHeldBlockTest : public ::testing::Test {
protected:
    void SetUp() override { enderman = std::make_unique<EndermanEntity>(EntityId(1)); }

    void TearDown() override { enderman.reset(); }

    std::unique_ptr<EndermanEntity> enderman;
};

TEST_F(EndermanHeldBlockTest, IsNotHoldingBlockInitially)
{
    // 初始状态不应该持有方块
    EXPECT_FALSE(enderman->isHoldingBlock());
    EXPECT_EQ(enderman->getHeldBlockState(), nullptr);
}

TEST_F(EndermanHeldBlockTest, SetHeldBlockStateSetsHoldingFlag)
{
    // 设置方块状态应该设置持有标志
    auto* dirtBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "dirt"));
    if (dirtBlock != nullptr) {
        const BlockState& defaultState = dirtBlock->defaultState();
        enderman->setHeldBlockState(&defaultState);

        EXPECT_TRUE(enderman->isHoldingBlock());
        EXPECT_EQ(enderman->getHeldBlockState(), &defaultState);
    }
}

TEST_F(EndermanHeldBlockTest, SetHeldBlockStateNullClearsHoldingFlag)
{
    // 设置为空应该清除持有标志
    auto* dirtBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "dirt"));
    if (dirtBlock != nullptr) {
        const BlockState& defaultState = dirtBlock->defaultState();
        enderman->setHeldBlockState(&defaultState);
        EXPECT_TRUE(enderman->isHoldingBlock());

        enderman->setHeldBlockState(nullptr);
        EXPECT_FALSE(enderman->isHoldingBlock());
        EXPECT_EQ(enderman->getHeldBlockState(), nullptr);
    }
}

// ==================== EndermanPlaceBlockGoal 测试 ====================

class EndermanPlaceBlockGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        enderman = std::make_unique<EndermanEntity>(EntityId(1));
        goal = std::make_unique<entity::ai::goal::EndermanPlaceBlockGoal>(enderman.get());
    }

    void TearDown() override
    {
        goal.reset();
        enderman.reset();
    }

    std::unique_ptr<EndermanEntity> enderman;
    std::unique_ptr<entity::ai::goal::EndermanPlaceBlockGoal> goal;
};

TEST_F(EndermanPlaceBlockGoalTest, ShouldExecuteReturnsFalseWhenNotHolding)
{
    // 不持有时不应该执行
    EXPECT_FALSE(enderman->isHoldingBlock());
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(EndermanPlaceBlockGoalTest, TypeNameReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "EndermanPlaceBlockGoal");
}

TEST_F(EndermanPlaceBlockGoalTest, ResetTaskDoesNotThrow)
{
    // resetTask 不应该抛出异常
    EXPECT_NO_THROW(goal->resetTask());
}

// ==================== EndermanTakeBlockGoal 测试 ====================

class EndermanTakeBlockGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        enderman = std::make_unique<EndermanEntity>(EntityId(1));
        goal = std::make_unique<entity::ai::goal::EndermanTakeBlockGoal>(enderman.get());
    }

    void TearDown() override
    {
        goal.reset();
        enderman.reset();
    }

    std::unique_ptr<EndermanEntity> enderman;
    std::unique_ptr<entity::ai::goal::EndermanTakeBlockGoal> goal;
};

TEST_F(EndermanTakeBlockGoalTest, ShouldExecuteReturnsFalseWhenAlreadyHolding)
{
    // 已经持有时不应该执行拾取
    auto* dirtBlock = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "dirt"));
    if (dirtBlock != nullptr) {
        const BlockState& defaultState = dirtBlock->defaultState();
        enderman->setHeldBlockState(&defaultState);
        EXPECT_TRUE(enderman->isHoldingBlock());
        EXPECT_FALSE(goal->shouldExecute());
    }
}

TEST_F(EndermanTakeBlockGoalTest, TypeNameReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "EndermanTakeBlockGoal");
}

TEST_F(EndermanTakeBlockGoalTest, ResetTaskDoesNotThrow)
{
    // resetTask 不应该抛出异常
    EXPECT_NO_THROW(goal->resetTask());
}

} // namespace test
} // namespace mc
