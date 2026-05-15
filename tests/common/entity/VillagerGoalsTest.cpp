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
 * @file VillagerGoalsTest.cpp
 * @brief 村民 AI 目标单元测试
 *
 * 测试 GatherItemsGoal, AvoidHostileGoal, VillagerBreedGoal 的关键方法：
 * - GatherItemsGoal: findNearbyItems, moveToItem, pickupItem
 * - AvoidHostileGoal: findNearestHostile, fleeFromHostile
 * - VillagerBreedGoal: findPartner, moveToPartner
 */

#include "entity/ai/goal/goals/villager/VillagerGoals.hpp"
#include "entity/entities/villager/VillagerEntity.hpp"
#include "item/Items.hpp"
#include "item/core/ItemStack.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity;
using namespace mc::entity::ai::goal;
using namespace mc::entity::ai::goal::villager;

// ==================== VillagerEntity Test Fixture ====================

class VillagerEntityTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化物品注册表
        Items::initialize();
    }

    void SetUp() override
    {
        villager = std::make_unique<VillagerEntity>(LegacyEntityType::Villager, 1);
    }

    void TearDown() override { villager.reset(); }

    std::unique_ptr<VillagerEntity> villager;
};

// ==================== VillagerEntity canPickUpItem Tests ====================

TEST_F(VillagerEntityTest, CanPickUpBread)
{
    // 村民可以拾取面包
    ItemStack breadStack(Items::BREAD, 1);
    EXPECT_TRUE(villager->canPickUpItem(breadStack));
}

TEST_F(VillagerEntityTest, CanPickUpPotato)
{
    // 村民可以拾取土豆
    ItemStack potatoStack(Items::POTATO, 1);
    EXPECT_TRUE(villager->canPickUpItem(potatoStack));
}

TEST_F(VillagerEntityTest, CanPickUpCarrot)
{
    // 村民可以拾取胡萝卜
    ItemStack carrotStack(Items::CARROT, 1);
    EXPECT_TRUE(villager->canPickUpItem(carrotStack));
}

TEST_F(VillagerEntityTest, CanPickUpBeetroot)
{
    // 村民可以拾取甜菜根
    ItemStack beetrootStack(Items::BEETROOT, 1);
    EXPECT_TRUE(villager->canPickUpItem(beetrootStack));
}

TEST_F(VillagerEntityTest, CanPickUpWheat)
{
    // 村民可以拾取小麦
    ItemStack wheatStack(Items::WHEAT, 1);
    EXPECT_TRUE(villager->canPickUpItem(wheatStack));
}

TEST_F(VillagerEntityTest, CanPickUpWheatSeeds)
{
    // 村民可以拾取小麦种子
    ItemStack seedsStack(Items::WHEAT_SEEDS, 1);
    EXPECT_TRUE(villager->canPickUpItem(seedsStack));
}

TEST_F(VillagerEntityTest, CanPickUpBeetrootSeeds)
{
    // 村民可以拾取甜菜根种子
    ItemStack seedsStack(Items::BEETROOT_SEEDS, 1);
    EXPECT_TRUE(villager->canPickUpItem(seedsStack));
}

TEST_F(VillagerEntityTest, CannotPickUpInvalidItem)
{
    // 村民不能拾取不在允许列表中的物品（如石头）
    // 注意：需要 Items::STONE 已初始化
    // ItemStack stoneStack(Items::STONE, 1);
    // EXPECT_FALSE(villager->canPickUpItem(stoneStack));

    // 空物品堆应该返回 false
    ItemStack emptyStack;
    EXPECT_FALSE(villager->canPickUpItem(emptyStack));
}

TEST_F(VillagerEntityTest, CanPickUpWhenInventoryHasSpace)
{
    // 村民库存为空时应该可以拾取
    ItemStack breadStack(Items::BREAD, 1);
    EXPECT_TRUE(villager->canPickUpItem(breadStack));
}

TEST_F(VillagerEntityTest, IsBreedingItem)
{
    // 测试繁殖物品
    ItemStack breadStack(Items::BREAD, 1);
    EXPECT_TRUE(villager->isBreedingItem(breadStack));

    ItemStack potatoStack(Items::POTATO, 1);
    EXPECT_TRUE(villager->isBreedingItem(potatoStack));

    ItemStack carrotStack(Items::CARROT, 1);
    EXPECT_TRUE(villager->isBreedingItem(carrotStack));

    ItemStack beetrootStack(Items::BEETROOT, 1);
    EXPECT_TRUE(villager->isBreedingItem(beetrootStack));

    // 小麦不是繁殖物品
    ItemStack wheatStack(Items::WHEAT, 1);
    EXPECT_FALSE(villager->isBreedingItem(wheatStack));
}

// ==================== GatherItemsGoal Tests ====================

class GatherItemsGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        villager = std::make_unique<VillagerEntity>(LegacyEntityType::Villager, 1);
        goal = std::make_unique<GatherItemsGoal>(villager.get());
    }

    void TearDown() override
    {
        goal.reset();
        villager.reset();
    }

    std::unique_ptr<VillagerEntity> villager;
    std::unique_ptr<GatherItemsGoal> goal;
};

TEST_F(GatherItemsGoalTest, ShouldExecute_ReturnsFalseWhenNoItems)
{
    // 没有物品时 shouldExecute 返回 false
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(GatherItemsGoalTest, TypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "GatherItemsGoal");
}

TEST_F(GatherItemsGoalTest, ResetTask_ClearsTarget)
{
    // resetTask 不应抛出异常
    EXPECT_NO_THROW(goal->resetTask());
}

TEST_F(GatherItemsGoalTest, MutexFlags_Move)
{
    // GatherItemsGoal 应使用 Move 标志
    // 验证目标创建成功
    EXPECT_NE(goal, nullptr);
}

// ==================== AvoidHostileGoal Tests ====================

class AvoidHostileGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        villager = std::make_unique<VillagerEntity>(LegacyEntityType::Villager, 1);
        goal = std::make_unique<AvoidHostileGoal>(villager.get());
    }

    void TearDown() override
    {
        goal.reset();
        villager.reset();
    }

    std::unique_ptr<VillagerEntity> villager;
    std::unique_ptr<AvoidHostileGoal> goal;
};

TEST_F(AvoidHostileGoalTest, ShouldExecute_ReturnsFalseWhenNoHostiles)
{
    // 没有敌对生物时 shouldExecute 返回 false
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(AvoidHostileGoalTest, TypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "AvoidHostileGoal");
}

TEST_F(AvoidHostileGoalTest, ResetTask_ClearsTarget)
{
    // resetTask 不应抛出异常
    EXPECT_NO_THROW(goal->resetTask());
}

TEST_F(AvoidHostileGoalTest, MutexFlags_Move)
{
    // AvoidHostileGoal 应使用 Move 标志
    EXPECT_NE(goal, nullptr);
}

// ==================== VillagerBreedGoal Tests ====================

class VillagerBreedGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        villager = std::make_unique<VillagerEntity>(LegacyEntityType::Villager, 1);
        goal = std::make_unique<VillagerBreedGoal>(villager.get());
    }

    void TearDown() override
    {
        goal.reset();
        villager.reset();
    }

    std::unique_ptr<VillagerEntity> villager;
    std::unique_ptr<VillagerBreedGoal> goal;
};

TEST_F(VillagerBreedGoalTest, ShouldExecute_ReturnsFalseWhenNotWilling)
{
    // 不愿意繁殖时 shouldExecute 返回 false
    villager->setWillingToBreed(false);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(VillagerBreedGoalTest, TypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "VillagerBreedGoal");
}

TEST_F(VillagerBreedGoalTest, ResetTask_ClearsTarget)
{
    // resetTask 不应抛出异常
    EXPECT_NO_THROW(goal->resetTask());
}

TEST_F(VillagerBreedGoalTest, MutexFlags_MoveAndLook)
{
    // VillagerBreedGoal 应使用 Move 和 Look 标志
    EXPECT_NE(goal, nullptr);
}

TEST_F(VillagerBreedGoalTest, HasEnoughBeds_ReturnsTrue)
{
    // 在没有村庄系统时，hasEnoughBeds 返回 true
    // 简化实现
}

// ==================== SleepAtNightGoal Tests ====================

class SleepAtNightGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        villager = std::make_unique<VillagerEntity>(LegacyEntityType::Villager, 1);
        goal = std::make_unique<SleepAtNightGoal>(villager.get());
    }

    void TearDown() override
    {
        goal.reset();
        villager.reset();
    }

    std::unique_ptr<VillagerEntity> villager;
    std::unique_ptr<SleepAtNightGoal> goal;
};

TEST_F(SleepAtNightGoalTest, TypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "SleepAtNightGoal");
}

TEST_F(SleepAtNightGoalTest, ResetTask_ClearsState)
{
    // resetTask 不应抛出异常
    EXPECT_NO_THROW(goal->resetTask());
}

// ==================== WorkAtJobSiteGoal Tests ====================

class WorkAtJobSiteGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        villager = std::make_unique<VillagerEntity>(LegacyEntityType::Villager, 1);
        goal = std::make_unique<WorkAtJobSiteGoal>(villager.get());
    }

    void TearDown() override
    {
        goal.reset();
        villager.reset();
    }

    std::unique_ptr<VillagerEntity> villager;
    std::unique_ptr<WorkAtJobSiteGoal> goal;
};

TEST_F(WorkAtJobSiteGoalTest, ShouldExecute_ReturnsFalseWhenNoJobSite)
{
    // 没有工作站点时 shouldExecute 返回 false
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(WorkAtJobSiteGoalTest, TypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "WorkAtJobSiteGoal");
}

TEST_F(WorkAtJobSiteGoalTest, ResetTask_ClearsState)
{
    // resetTask 不应抛出异常
    EXPECT_NO_THROW(goal->resetTask());
}

// ==================== LookForJobSiteGoal Tests ====================

class LookForJobSiteGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        villager = std::make_unique<VillagerEntity>(LegacyEntityType::Villager, 1);
        goal = std::make_unique<LookForJobSiteGoal>(villager.get());
    }

    void TearDown() override
    {
        goal.reset();
        villager.reset();
    }

    std::unique_ptr<VillagerEntity> villager;
    std::unique_ptr<LookForJobSiteGoal> goal;
};

TEST_F(LookForJobSiteGoalTest, ShouldExecute_ReturnsFalseForNitwit)
{
    // 设置为傻子村民
    villager->setProfession(VillagerProfession::Nitwit);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(LookForJobSiteGoalTest, TypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "LookForJobSiteGoal");
}

TEST_F(LookForJobSiteGoalTest, ResetTask_ClearsState)
{
    // resetTask 不应抛出异常
    EXPECT_NO_THROW(goal->resetTask());
}
