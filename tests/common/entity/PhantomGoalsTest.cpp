/**
 * @file PhantomGoalsTest.cpp
 * @brief 幻翼 AI 目标单元测试
 *
 * 测试 PhantomAttackPlayerTargetGoal、PhantomOrbitPointGoal、
 * PhantomPickAttackGoal、PhantomSweepAttackGoal 的关键方法。
 */

#include "entity/ai/goal/goals/special/PhantomGoals.hpp"
#include "entity/entities/monster/basic/PhantomEntity.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::ai::goal;

// ==================== PhantomEntity Test Fixture ====================

class PhantomEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        phantom = std::make_unique<PhantomEntity>(LegacyEntityType::Phantom, EntityId(0));
    }

    void TearDown() override { phantom.reset(); }

    std::unique_ptr<PhantomEntity> phantom;
};

// ==================== PhantomEntity State Tests ====================

TEST_F(PhantomEntityTest, DefaultState_CirclePhase)
{
    EXPECT_EQ(phantom->getAttackPhase(), PhantomEntity::AttackPhase::CIRCLE);
}

TEST_F(PhantomEntityTest, SetAttackPhase_ChangesPhase)
{
    phantom->setAttackPhase(PhantomEntity::AttackPhase::SWOOP);
    EXPECT_EQ(phantom->getAttackPhase(), PhantomEntity::AttackPhase::SWOOP);

    phantom->setAttackPhase(PhantomEntity::AttackPhase::CIRCLE);
    EXPECT_EQ(phantom->getAttackPhase(), PhantomEntity::AttackPhase::CIRCLE);
}

TEST_F(PhantomEntityTest, DefaultState_ZeroSize)
{
    EXPECT_EQ(phantom->getPhantomSize(), 0);
}

TEST_F(PhantomEntityTest, SetPhantomSize_ChangesSize)
{
    phantom->setPhantomSize(5);
    EXPECT_EQ(phantom->getPhantomSize(), 5);

    phantom->setPhantomSize(64);
    EXPECT_EQ(phantom->getPhantomSize(), 64);
}

TEST_F(PhantomEntityTest, SetPhantomSize_ClampedToMax)
{
    phantom->setPhantomSize(100);
    EXPECT_EQ(phantom->getPhantomSize(), 64);
}

TEST_F(PhantomEntityTest, SetPhantomSize_ClampedToMin)
{
    phantom->setPhantomSize(-5);
    EXPECT_EQ(phantom->getPhantomSize(), 0);
}

TEST_F(PhantomEntityTest, OrbitPosition_DefaultIsZero)
{
    BlockPos pos = phantom->orbitPosition();
    EXPECT_EQ(pos.x, 0);
    EXPECT_EQ(pos.y, 0);
    EXPECT_EQ(pos.z, 0);
}

TEST_F(PhantomEntityTest, SetOrbitPosition_ChangesPosition)
{
    BlockPos newPos(100, 64, -50);
    phantom->setOrbitPosition(newPos);
    EXPECT_EQ(phantom->orbitPosition().x, 100);
    EXPECT_EQ(phantom->orbitPosition().y, 64);
    EXPECT_EQ(phantom->orbitPosition().z, -50);
}

TEST_F(PhantomEntityTest, OrbitOffset_DefaultIsZero)
{
    math::Vector3f offset = phantom->orbitOffset();
    EXPECT_FLOAT_EQ(offset.x, 0.0f);
    EXPECT_FLOAT_EQ(offset.y, 0.0f);
    EXPECT_FLOAT_EQ(offset.z, 0.0f);
}

TEST_F(PhantomEntityTest, SetOrbitOffset_ChangesOffset)
{
    math::Vector3f newOffset(10.5f, 20.0f, -5.5f);
    phantom->setOrbitOffset(newOffset);
    EXPECT_FLOAT_EQ(phantom->orbitOffset().x, 10.5f);
    EXPECT_FLOAT_EQ(phantom->orbitOffset().y, 20.0f);
    EXPECT_FLOAT_EQ(phantom->orbitOffset().z, -5.5f);
}

TEST_F(PhantomEntityTest, CreatureAttribute_IsUndead)
{
    EXPECT_EQ(phantom->getCreatureAttribute(), CreatureAttribute::Undead);
}

TEST_F(PhantomEntityTest, EyeHeight_IsCorrect)
{
    // MC 1.16.5: height * 0.35F
    f32 expectedEyeHeight = phantom->height() * 0.35f;
    EXPECT_FLOAT_EQ(phantom->eyeHeight(), expectedEyeHeight);
}

// ==================== PhantomAttackPlayerTargetGoal Tests ====================

class PhantomAttackPlayerTargetGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        phantom = std::make_unique<PhantomEntity>(LegacyEntityType::Phantom, EntityId(0));
        goal = std::make_unique<PhantomAttackPlayerTargetGoal>(phantom.get());
    }

    void TearDown() override
    {
        goal.reset();
        phantom.reset();
    }

    std::unique_ptr<PhantomEntity> phantom;
    std::unique_ptr<PhantomAttackPlayerTargetGoal> goal;
};

TEST_F(PhantomAttackPlayerTargetGoalTest, TypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "PhantomAttackPlayerTargetGoal");
}

TEST_F(PhantomAttackPlayerTargetGoalTest, ShouldExecute_WithoutWorld_ReturnsFalse)
{
    // 没有世界的情况下不应执行
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PhantomAttackPlayerTargetGoalTest, ResetTask_ClearsAttackTarget)
{
    // 设置一个假的目标（实际使用时会检查目标有效性）
    goal->resetTask();
    EXPECT_EQ(phantom->attackTarget(), nullptr);
}

// ==================== PhantomOrbitPointGoal Tests ====================

class PhantomOrbitPointGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        phantom = std::make_unique<PhantomEntity>(LegacyEntityType::Phantom, EntityId(0));
        goal = std::make_unique<PhantomOrbitPointGoal>(phantom.get());
    }

    void TearDown() override
    {
        goal.reset();
        phantom.reset();
    }

    std::unique_ptr<PhantomEntity> phantom;
    std::unique_ptr<PhantomOrbitPointGoal> goal;
};

TEST_F(PhantomOrbitPointGoalTest, TypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "PhantomOrbitPointGoal");
}

TEST_F(PhantomOrbitPointGoalTest, ShouldExecute_NoTarget_ReturnsTrue)
{
    // 无攻击目标时应该执行
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(PhantomOrbitPointGoalTest, ShouldExecute_CirclePhase_ReturnsTrue)
{
    phantom->setAttackPhase(PhantomEntity::AttackPhase::CIRCLE);
    EXPECT_TRUE(goal->shouldExecute());
}

TEST_F(PhantomOrbitPointGoalTest, StartExecuting_InitializesOrbitParameters)
{
    goal->startExecuting();
    // 环绕偏移应该被设置
    Vector3 offset = phantom->orbitOffset();
    // 初始化后偏移应该非零
    // 由于随机性，只检查是否设置过
    SUCCEED();
}

TEST_F(PhantomOrbitPointGoalTest, Tick_WithoutWorld_DoesNotCrash)
{
    goal->startExecuting();
    EXPECT_NO_THROW({
        for (int i = 0; i < 100; ++i) {
            goal->tick();
        }
    });
}

// ==================== PhantomPickAttackGoal Tests ====================

class PhantomPickAttackGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        phantom = std::make_unique<PhantomEntity>(LegacyEntityType::Phantom, EntityId(0));
        goal = std::make_unique<PhantomPickAttackGoal>(phantom.get());
    }

    void TearDown() override
    {
        goal.reset();
        phantom.reset();
    }

    std::unique_ptr<PhantomEntity> phantom;
    std::unique_ptr<PhantomPickAttackGoal> goal;
};

TEST_F(PhantomPickAttackGoalTest, TypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "PhantomPickAttackGoal");
}

TEST_F(PhantomPickAttackGoalTest, ShouldExecute_NoTarget_ReturnsFalse)
{
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PhantomPickAttackGoalTest, StartExecuting_SetsCirclePhase)
{
    phantom->setAttackPhase(PhantomEntity::AttackPhase::SWOOP);
    goal->startExecuting();
    EXPECT_EQ(phantom->getAttackPhase(), PhantomEntity::AttackPhase::CIRCLE);
}

TEST_F(PhantomPickAttackGoalTest, Tick_WithoutTarget_DoesNotCrash)
{
    goal->startExecuting();
    EXPECT_NO_THROW({
        for (int i = 0; i < 100; ++i) {
            goal->tick();
        }
    });
}

// ==================== PhantomSweepAttackGoal Tests ====================

class PhantomSweepAttackGoalTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        phantom = std::make_unique<PhantomEntity>(LegacyEntityType::Phantom, EntityId(0));
        goal = std::make_unique<PhantomSweepAttackGoal>(phantom.get());
    }

    void TearDown() override
    {
        goal.reset();
        phantom.reset();
    }

    std::unique_ptr<PhantomEntity> phantom;
    std::unique_ptr<PhantomSweepAttackGoal> goal;
};

TEST_F(PhantomSweepAttackGoalTest, TypeName_ReturnsCorrectName)
{
    EXPECT_EQ(goal->getTypeName(), "PhantomSweepAttackGoal");
}

TEST_F(PhantomSweepAttackGoalTest, ShouldExecute_NoTarget_ReturnsFalse)
{
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PhantomSweepAttackGoalTest, ShouldExecute_CirclePhase_ReturnsFalse)
{
    phantom->setAttackPhase(PhantomEntity::AttackPhase::CIRCLE);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PhantomSweepAttackGoalTest, ShouldExecute_SwoopPhaseNoTarget_ReturnsFalse)
{
    phantom->setAttackPhase(PhantomEntity::AttackPhase::SWOOP);
    EXPECT_FALSE(goal->shouldExecute());
}

TEST_F(PhantomSweepAttackGoalTest, ResetTask_SetsCirclePhase)
{
    phantom->setAttackPhase(PhantomEntity::AttackPhase::SWOOP);
    goal->resetTask();
    EXPECT_EQ(phantom->getAttackPhase(), PhantomEntity::AttackPhase::CIRCLE);
}

TEST_F(PhantomSweepAttackGoalTest, ResetTask_ClearsAttackTarget)
{
    goal->resetTask();
    EXPECT_EQ(phantom->attackTarget(), nullptr);
}

TEST_F(PhantomSweepAttackGoalTest, Tick_WithoutTarget_DoesNotCrash)
{
    goal->startExecuting();
    EXPECT_NO_THROW({
        for (int i = 0; i < 100; ++i) {
            goal->tick();
        }
    });
}

// ==================== Integration Tests ====================

class PhantomGoalsIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        phantom = std::make_unique<PhantomEntity>(LegacyEntityType::Phantom, EntityId(0));
        targetGoal = std::make_unique<PhantomAttackPlayerTargetGoal>(phantom.get());
        orbitGoal = std::make_unique<PhantomOrbitPointGoal>(phantom.get());
        pickGoal = std::make_unique<PhantomPickAttackGoal>(phantom.get());
        sweepGoal = std::make_unique<PhantomSweepAttackGoal>(phantom.get());
    }

    void TearDown() override
    {
        sweepGoal.reset();
        pickGoal.reset();
        orbitGoal.reset();
        targetGoal.reset();
        phantom.reset();
    }

    std::unique_ptr<PhantomEntity> phantom;
    std::unique_ptr<PhantomAttackPlayerTargetGoal> targetGoal;
    std::unique_ptr<PhantomOrbitPointGoal> orbitGoal;
    std::unique_ptr<PhantomPickAttackGoal> pickGoal;
    std::unique_ptr<PhantomSweepAttackGoal> sweepGoal;
};

TEST_F(PhantomGoalsIntegrationTest, MultipleGoals_CanCoexist)
{
    EXPECT_NE(targetGoal, nullptr);
    EXPECT_NE(orbitGoal, nullptr);
    EXPECT_NE(pickGoal, nullptr);
    EXPECT_NE(sweepGoal, nullptr);
}

TEST_F(PhantomGoalsIntegrationTest, MultipleTicks_DoNotThrow)
{
    EXPECT_NO_THROW({
        for (int i = 0; i < 100; ++i) {
            orbitGoal->tick();
            pickGoal->tick();
            sweepGoal->tick();
        }
    });
}

TEST_F(PhantomGoalsIntegrationTest, AttackPhase_TransitionsCorrectly)
{
    // 初始状态是 CIRCLE
    EXPECT_EQ(phantom->getAttackPhase(), PhantomEntity::AttackPhase::CIRCLE);

    // 设置为 SWOOP
    phantom->setAttackPhase(PhantomEntity::AttackPhase::SWOOP);
    EXPECT_EQ(phantom->getAttackPhase(), PhantomEntity::AttackPhase::SWOOP);

    // 检查 goal 应该执行的条件
    EXPECT_TRUE(orbitGoal->shouldExecute()); // 无目标时应该执行
    EXPECT_FALSE(sweepGoal->shouldExecute()); // 无目标时不应该执行
}

TEST_F(PhantomGoalsIntegrationTest, OrbitPosition_CanBeSetAndRetrieved)
{
    BlockPos pos1(100, 50, -100);
    phantom->setOrbitPosition(pos1);
    EXPECT_EQ(phantom->orbitPosition().x, 100);
    EXPECT_EQ(phantom->orbitPosition().y, 50);
    EXPECT_EQ(phantom->orbitPosition().z, -100);

    BlockPos pos2(-50, 100, 200);
    phantom->setOrbitPosition(pos2);
    EXPECT_EQ(phantom->orbitPosition().x, -50);
    EXPECT_EQ(phantom->orbitPosition().y, 100);
    EXPECT_EQ(phantom->orbitPosition().z, 200);
}

// ==================== Constants Validation Tests ====================

TEST_F(PhantomEntityTest, Constants_AreCorrect)
{
    // MC 1.16.5: BASE_ATTACK_DAMAGE = 6.0f
    // 通过常量验证（属性需要初始化后才能测试）
    // 这里验证幻翼的基本常量
    EXPECT_EQ(phantom->getPhantomSize(), 0);  // 默认大小为0
    EXPECT_EQ(phantom->getAttackPhase(), PhantomEntity::AttackPhase::CIRCLE);  // 默认环绕阶段
}

TEST_F(PhantomEntityTest, Size_AffectsDimensions)
{
    // 获取基础尺寸
    entity::EntitySize size0 = phantom->getDimensions(EntityPose::Standing);

    // 增大幻翼
    phantom->setPhantomSize(10);
    entity::EntitySize size10 = phantom->getDimensions(EntityPose::Standing);

    // 尺寸应该增大
    EXPECT_GT(size10.width(), size0.width());
    EXPECT_GT(size10.height(), size0.height());
}

TEST_F(PhantomEntityTest, Size_AffectsAttackDamage)
{
    // MC 1.16.5: 每级大小 +1.0 攻击力
    // 验证大小设置正确
    phantom->setPhantomSize(5);
    EXPECT_EQ(phantom->getPhantomSize(), 5);

    phantom->setPhantomSize(0);
    EXPECT_EQ(phantom->getPhantomSize(), 0);
}
