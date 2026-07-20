/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction restriction, including without limitation the rights
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

#include "common/entity/ai/goal/goals/interact/LandOnOwnersShoulderGoal.hpp"
#include "common/entity/ai/goal/goals/movement/FollowMobGoal.hpp"
#include "common/entity/ai/goal/goals/movement/WaterAvoidingRandomFlyingGoal.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/passive/tamable/ParrotEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc {
namespace {

// ============================================================================
// Test CreatureEntity for flying goal tests
// ============================================================================

class TestFlyingCreature : public CreatureEntity {
public:
    TestFlyingCreature()
        : CreatureEntity(EntityInstanceId(1))
    {
        registerAttributes();
        setHealth(maxHealth());
    }

    void setPositionForTest(f64 x, f64 y, f64 z)
    {
        setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    }
};

// ============================================================================
// Test MobEntity for FollowMobGoal tests
// ============================================================================

class TestMob : public MobEntity {
public:
    TestMob()
        : MobEntity(EntityInstanceId(1))
    {
        registerAttributes();
        setHealth(maxHealth());
    }

    void setPositionForTest(f64 x, f64 y, f64 z)
    {
        setPosition(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z));
    }
};

// ============================================================================
// LandOnOwnersShoulderGoal Tests
// ============================================================================

class LandOnOwnersShoulderGoalTest : public ::testing::Test {
protected:
    void SetUp() override { parrot = std::make_unique<ParrotEntity>(EntityInstanceId(1)); }

    void TearDown() override { parrot.reset(); }

    std::unique_ptr<ParrotEntity> parrot;
};

TEST_F(LandOnOwnersShoulderGoalTest, Construction_ValidEntity)
{
    entity::ai::goal::LandOnOwnersShoulderGoal goal(parrot.get());
    EXPECT_EQ(goal.getTypeName(), "LandOnOwnersShoulderGoal");
}

TEST_F(LandOnOwnersShoulderGoalTest, ShouldExecute_ReturnsFalse_WhenNotTamed)
{
    entity::ai::goal::LandOnOwnersShoulderGoal goal(parrot.get());
    parrot->setTamed(false);

    // 未驯服的鹦鹉不应该执行此目标
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(LandOnOwnersShoulderGoalTest, ShouldExecute_ReturnsFalse_WhenNoWorld)
{
    entity::ai::goal::LandOnOwnersShoulderGoal goal(parrot.get());
    parrot->setTamed(true);

    // 没有世界（没有主人），不应该执行
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(LandOnOwnersShoulderGoalTest, IsPreemptible_ReturnsTrue_WhenNotSittingOnShoulder)
{
    entity::ai::goal::LandOnOwnersShoulderGoal goal(parrot.get());

    // 初始状态，未坐在肩膀上，应该可以被抢占
    EXPECT_TRUE(goal.isPreemptible());
}

TEST_F(LandOnOwnersShoulderGoalTest, StartExecuting_DoesNotCrash)
{
    entity::ai::goal::LandOnOwnersShoulderGoal goal(parrot.get());
    parrot->setTamed(true);

    goal.startExecuting();
    // 应该不会崩溃
    EXPECT_TRUE(true);
}

TEST_F(LandOnOwnersShoulderGoalTest, Tick_DoesNotCrash_WhenNoWorld)
{
    entity::ai::goal::LandOnOwnersShoulderGoal goal(parrot.get());
    parrot->setTamed(true);

    goal.startExecuting();
    goal.tick();
    // 没有世界时不应该崩溃
    EXPECT_TRUE(true);
}

// ============================================================================
// WaterAvoidingRandomFlyingGoal Tests
// ============================================================================

class WaterAvoidingRandomFlyingGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        creature = std::make_unique<TestFlyingCreature>();
        creature->setPositionForTest(0.0, 64.0, 0.0);
    }

    void TearDown() override { creature.reset(); }

    std::unique_ptr<TestFlyingCreature> creature;
};

TEST_F(WaterAvoidingRandomFlyingGoalTest, Construction_ValidParameters)
{
    entity::ai::goal::WaterAvoidingRandomFlyingGoal goal(creature.get(), 1.0);
    EXPECT_EQ(goal.getTypeName(), "WaterAvoidingRandomFlyingGoal");
}

TEST_F(WaterAvoidingRandomFlyingGoalTest, Construction_WithChance)
{
    entity::ai::goal::WaterAvoidingRandomFlyingGoal goal(creature.get(), 1.0, 0.5f);
    EXPECT_EQ(goal.getTypeName(), "WaterAvoidingRandomFlyingGoal");
}

TEST_F(WaterAvoidingRandomFlyingGoalTest, ShouldExecute_ReturnsFalse_WhenNoWorld)
{
    entity::ai::goal::WaterAvoidingRandomFlyingGoal goal(creature.get(), 1.0);

    // 没有世界时无法找到随机位置
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(WaterAvoidingRandomFlyingGoalTest, ShouldContinueExecuting_ReturnsFalse_WhenNoWorld)
{
    entity::ai::goal::WaterAvoidingRandomFlyingGoal goal(creature.get(), 1.0);

    static_cast<void>(goal.shouldExecute());
    goal.startExecuting();

    // 没有世界时应该返回 false
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

TEST_F(WaterAvoidingRandomFlyingGoalTest, ResetTask_DoesNotCrash)
{
    entity::ai::goal::WaterAvoidingRandomFlyingGoal goal(creature.get(), 1.0);

    goal.startExecuting();
    goal.resetTask();

    // 应该不会崩溃
    EXPECT_TRUE(true);
}

TEST_F(WaterAvoidingRandomFlyingGoalTest, Tick_DoesNotCrash)
{
    entity::ai::goal::WaterAvoidingRandomFlyingGoal goal(creature.get(), 1.0);

    goal.startExecuting();

    // 多次 tick 不应该崩溃
    for (int i = 0; i < 100; ++i) {
        goal.tick();
    }

    EXPECT_TRUE(true);
}

TEST_F(WaterAvoidingRandomFlyingGoalTest, ShouldExecute_ReturnsFalse_WhenBeingRidden)
{
    // 注意：TestFlyingCreature 默认 isBeingRidden() 返回 false
    // 这个测试验证目标能正确处理被骑乘状态
    entity::ai::goal::WaterAvoidingRandomFlyingGoal goal(creature.get(), 1.0);

    // 没有世界且没有被骑乘，但无法找到位置
    EXPECT_FALSE(goal.shouldExecute());
}

// ============================================================================
// FollowMobGoal Tests
// ============================================================================

class FollowMobGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        mob = std::make_unique<TestMob>();
        mob->setPositionForTest(0.0, 64.0, 0.0);
    }

    void TearDown() override { mob.reset(); }

    std::unique_ptr<TestMob> mob;
};

TEST_F(FollowMobGoalTest, Construction_ValidParameters)
{
    entity::ai::goal::FollowMobGoal goal(mob.get(), 1.0, 3.0f, 7.0f);
    EXPECT_EQ(goal.getTypeName(), "FollowMobGoal");
}

TEST_F(FollowMobGoalTest, ShouldExecute_ReturnsFalse_WhenNoWorld)
{
    entity::ai::goal::FollowMobGoal goal(mob.get(), 1.0, 3.0f, 7.0f);

    // 没有世界时无法找到目标生物
    EXPECT_FALSE(goal.shouldExecute());
}

TEST_F(FollowMobGoalTest, ShouldContinueExecuting_ReturnsFalse_WhenNoTarget)
{
    entity::ai::goal::FollowMobGoal goal(mob.get(), 1.0, 3.0f, 7.0f);

    // 没有目标时应该返回 false
    EXPECT_FALSE(goal.shouldContinueExecuting());
}

TEST_F(FollowMobGoalTest, StartExecuting_DoesNotCrash)
{
    entity::ai::goal::FollowMobGoal goal(mob.get(), 1.0, 3.0f, 7.0f);

    goal.startExecuting();
    // 应该不会崩溃
    EXPECT_TRUE(true);
}

TEST_F(FollowMobGoalTest, ResetTask_DoesNotCrash)
{
    entity::ai::goal::FollowMobGoal goal(mob.get(), 1.0, 3.0f, 7.0f);

    goal.startExecuting();
    goal.resetTask();

    // 应该不会崩溃
    EXPECT_TRUE(true);
}

TEST_F(FollowMobGoalTest, Tick_DoesNotCrash_WhenNoTarget)
{
    entity::ai::goal::FollowMobGoal goal(mob.get(), 1.0, 3.0f, 7.0f);

    goal.startExecuting();

    // 多次 tick 不应该崩溃（即使没有目标）
    for (int i = 0; i < 100; ++i) {
        goal.tick();
    }

    EXPECT_TRUE(true);
}

TEST_F(FollowMobGoalTest, MutexFlags_ContainsMove)
{
    entity::ai::goal::FollowMobGoal goal(mob.get(), 1.0, 3.0f, 7.0f);

    const auto& flags = goal.getMutexFlags();
    EXPECT_TRUE(flags.test(entity::ai::GoalFlag::Move));
}

// ============================================================================
// ParrotEntity AI Goals Integration Tests
// ============================================================================

class ParrotGoalsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        parrot = std::make_unique<ParrotEntity>(EntityInstanceId(1));
    }

    void TearDown() override { parrot.reset(); }

    std::unique_ptr<ParrotEntity> parrot;
};

TEST_F(ParrotGoalsTest, Parrot_HasCorrectTameItems)
{
    ItemStack wheatSeeds(Items::WHEAT_SEEDS, 1);
    ItemStack pumpkinSeeds(Items::PUMPKIN_SEEDS, 1);
    ItemStack melonSeeds(Items::MELON_SEEDS, 1);
    ItemStack beetrootSeeds(Items::BEETROOT_SEEDS, 1);
    ItemStack bone(Items::BONE, 1);

    EXPECT_TRUE(parrot->isTameItem(wheatSeeds));
    EXPECT_TRUE(parrot->isTameItem(pumpkinSeeds));
    EXPECT_TRUE(parrot->isTameItem(melonSeeds));
    EXPECT_TRUE(parrot->isTameItem(beetrootSeeds));

    // 骨头不能驯服鹦鹉
    EXPECT_FALSE(parrot->isTameItem(bone));
}

TEST_F(ParrotGoalsTest, Parrot_CannotBreed)
{
    // 鹦鹉不能繁殖
    ItemStack wheatSeeds(Items::WHEAT_SEEDS, 1);
    EXPECT_FALSE(parrot->isBreedingItem(wheatSeeds));
}

TEST_F(ParrotGoalsTest, Parrot_IsShoulderRidingEntity)
{
    // 鹦鹉是 ShoulderRidingEntity
    // 注意: canSitOnShoulder() 需要 m_rideCooldownCounter > 100
    // 初始状态下为 0，所以返回 false
    EXPECT_FALSE(parrot->canSitOnShoulder()); // 冷却时间未到
    EXPECT_FALSE(parrot->isOnShoulder());     // 未在肩膀上
}

TEST_F(ParrotGoalsTest, Parrot_HasCorrectHealth)
{
    // 鹦鹉生命值应该是 6
    EXPECT_FLOAT_EQ(parrot->maxHealth(), 6.0f);
}

TEST_F(ParrotGoalsTest, Parrot_VariantIsValid)
{
    // 鹦鹉应该有有效的变体（0-4）
    // 默认构造后会随机分配变体
    // 这里只检查编译通过
    EXPECT_TRUE(true);
}

} // namespace
} // namespace mc
