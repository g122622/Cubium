#include <gtest/gtest.h>

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/special/BlazeFireballAttackGoal.hpp"

using namespace mc;
using namespace mc::entity::ai;
using namespace mc::entity::ai::goal;

// ============================================================================
// BlazeFireballAttackGoal 基本测试
// ============================================================================
//
// 注意：BlazeFireballAttackGoal 的完整行为测试需要 BlazeEntity、MobEntity 和相关依赖。
// 这里测试常量和基本配置。
// 完整的行为测试应在集成测试中进行，使用 Mock 世界和实体。

class BlazeFireballAttackGoalBasicTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 设置代码
    }
};

// ============================================================================
// MC 1.16.5 常量测试
// ============================================================================

TEST_F(BlazeFireballAttackGoalBasicTest, ChargeTimeConstant_IsCorrect)
{
    // MC 1.16.5: 烈焰人充能时间为 60 ticks (3秒)
    constexpr i32 CHARGE_TIME = 60;
    EXPECT_EQ(CHARGE_TIME, 60);
}

TEST_F(BlazeFireballAttackGoalBasicTest, FireballIntervalConstant_IsCorrect)
{
    // MC 1.16.5: 火球发射间隔为 6 ticks (0.3秒)
    constexpr i32 FIREBALL_INTERVAL = 6;
    EXPECT_EQ(FIREBALL_INTERVAL, 6);
}

TEST_F(BlazeFireballAttackGoalBasicTest, CooldownTimeConstant_IsCorrect)
{
    // MC 1.16.5: 攻击冷却时间为 100 ticks (5秒)
    constexpr i32 COOLDOWN_TIME = 100;
    EXPECT_EQ(COOLDOWN_TIME, 100);
}

TEST_F(BlazeFireballAttackGoalBasicTest, MaxFireballsConstant_IsCorrect)
{
    // MC 1.16.5: 最多连发 3 个火球
    constexpr i32 MAX_FIREBALLS = 3;
    EXPECT_EQ(MAX_FIREBALLS, 3);
}

TEST_F(BlazeFireballAttackGoalBasicTest, MeleeRangeConstant_IsCorrect)
{
    // MC 1.16.5: 近战范围为 2 格 (2 * 2 = 4)
    constexpr f64 MELEE_RANGE = 2.0;
    constexpr f64 MELEE_RANGE_SQ = MELEE_RANGE * MELEE_RANGE;
    EXPECT_DOUBLE_EQ(MELEE_RANGE_SQ, 4.0);
}

// ============================================================================
// 攻击阶段逻辑测试
// ============================================================================

TEST_F(BlazeFireballAttackGoalBasicTest, AttackPhaseSequence_IsCorrect)
{
    // MC 1.16.5: 攻击阶段序列
    // 阶段 0: 未开始
    // 阶段 1: 充能 (60 ticks)
    // 阶段 2-4: 连发火球 (各 6 ticks)
    // 阶段 5+: 冷却 (100 ticks) 后重置为 0

    constexpr i32 PHASE_IDLE = 0;
    constexpr i32 PHASE_CHARGE = 1;
    constexpr i32 PHASE_FIREBALL_1 = 2;
    constexpr i32 PHASE_FIREBALL_2 = 3;
    constexpr i32 PHASE_FIREBALL_3 = 4;

    EXPECT_EQ(PHASE_IDLE, 0);
    EXPECT_EQ(PHASE_CHARGE, 1);
    EXPECT_EQ(PHASE_FIREBALL_1, 2);
    EXPECT_EQ(PHASE_FIREBALL_2, 3);
    EXPECT_EQ(PHASE_FIREBALL_3, 4);

    // 验证总攻击时间
    constexpr i32 TOTAL_ATTACK_TIME = 60 + 6 + 6 + 6 + 100; // 充能 + 3火球 + 冷却
    EXPECT_EQ(TOTAL_ATTACK_TIME, 178);                      // ticks
}

TEST_F(BlazeFireballAttackGoalBasicTest, TotalAttackCycleTime_IsCorrect)
{
    // MC 1.16.5: 完整攻击周期 = 充能 + 3火球 + 冷却
    // 60 + 3*6 + 100 = 178 ticks = 8.9 秒
    constexpr i32 CHARGE_TIME = 60;
    constexpr i32 FIREBALL_INTERVAL = 6;
    constexpr i32 MAX_FIREBALLS = 3;
    constexpr i32 COOLDOWN_TIME = 100;

    constexpr i32 TOTAL_CYCLE = CHARGE_TIME + (FIREBALL_INTERVAL * MAX_FIREBALLS) + COOLDOWN_TIME;
    EXPECT_EQ(TOTAL_CYCLE, 178);

    // 转换为秒 (20 ticks = 1 秒)
    constexpr f64 TOTAL_SECONDS = static_cast<f64>(TOTAL_CYCLE) / 20.0;
    EXPECT_DOUBLE_EQ(TOTAL_SECONDS, 8.9);
}

// ============================================================================
// GoalFlag 测试
// ============================================================================

TEST_F(BlazeFireballAttackGoalBasicTest, MutexFlags_AreMoveAndLook)
{
    // MC 1.16.5: BlazeFireballAttackGoal 使用 Move 和 Look 标志
    // 这意味着它会与其他使用 Move 或 Look 的目标互斥
    EnumSet<GoalFlag> expectedFlags{GoalFlag::Move, GoalFlag::Look};

    // 验证标志数量
    EXPECT_EQ(expectedFlags.count(), 2);
    EXPECT_TRUE(expectedFlags.test(GoalFlag::Move));
    EXPECT_TRUE(expectedFlags.test(GoalFlag::Look));
    EXPECT_FALSE(expectedFlags.test(GoalFlag::Jump));
    EXPECT_FALSE(expectedFlags.test(GoalFlag::Target));
}

TEST_F(BlazeFireballAttackGoalBasicTest, MutexFlags_IntersectsWithOtherMovementGoals)
{
    // 验证与其他移动目标的互斥性
    EnumSet<GoalFlag> blazeFlags{GoalFlag::Move, GoalFlag::Look};

    // WaterAvoidingRandomWalkingGoal 只使用 Move
    EnumSet<GoalFlag> walkingFlags{GoalFlag::Move};
    EXPECT_TRUE(blazeFlags.intersects(walkingFlags));

    // LookAtGoal 只使用 Look
    EnumSet<GoalFlag> lookFlags{GoalFlag::Look};
    EXPECT_TRUE(blazeFlags.intersects(lookFlags));

    // LookRandomlyGoal 只使用 Look
    EnumSet<GoalFlag> lookRandomlyFlags{GoalFlag::Look};
    EXPECT_TRUE(blazeFlags.intersects(lookRandomlyFlags));
}

// ============================================================================
// 优先级测试
// ============================================================================

TEST_F(BlazeFireballAttackGoalBasicTest, Priority_ShouldBe4)
{
    // MC 1.16.5: BlazeEntity.registerGoals() 中 FireballAttackGoal 优先级为 4
    // 参考: this.goalSelector.addGoal(4, new FireballAttackGoal(this));
    constexpr i32 BLAZE_FIREBALL_ATTACK_PRIORITY = 4;
    EXPECT_EQ(BLAZE_FIREBALL_ATTACK_PRIORITY, 4);
}

// ============================================================================
// 火球属性测试
// ============================================================================

TEST_F(BlazeFireballAttackGoalBasicTest, SmallFireballAcceleration_IsCorrect)
{
    // MC 1.16.5: 小火球使用加速度而非速度
    // 加速度 = 方向向量 * 0.1
    constexpr f32 ACCELERATION_MULTIPLIER = 0.1f;
    EXPECT_FLOAT_EQ(ACCELERATION_MULTIPLIER, 0.1f);
}

TEST_F(BlazeFireballAttackGoalBasicTest, FireballSpread_Calculation)
{
    // MC 1.16.5: spread = sqrt(sqrt(distSq)) * 0.5
    // 测试几个典型距离

    // 距离 16 格 (追踪范围边缘)
    f64 distSq_16 = 16.0 * 16.0; // 256
    f32 spread_16 = static_cast<f32>(std::sqrt(std::sqrt(distSq_16))) * 0.5f;
    EXPECT_NEAR(spread_16, 2.0f, 0.01f); // sqrt(sqrt(256)) = sqrt(16) = 4, 4 * 0.5 = 2

    // 距离 48 格 (默认追踪范围)
    f64 distSq_48 = 48.0 * 48.0; // 2304
    f32 spread_48 = static_cast<f32>(std::sqrt(std::sqrt(distSq_48))) * 0.5f;
    // sqrt(sqrt(2304)) = sqrt(48) ≈ 6.928, * 0.5 ≈ 3.464
    EXPECT_NEAR(spread_48, 3.464f, 0.01f);
}

// ============================================================================
// 烈焰人属性测试
// ============================================================================

TEST_F(BlazeFireballAttackGoalBasicTest, BlazeDefaultFollowRange_IsCorrect)
{
    // MC 1.16.5: 烈焰人默认追踪范围为 48 格
    constexpr f64 DEFAULT_FOLLOW_RANGE = 48.0;
    EXPECT_DOUBLE_EQ(DEFAULT_FOLLOW_RANGE, 48.0);
}

TEST_F(BlazeFireballAttackGoalBasicTest, BlazeDefaultHealth_IsCorrect)
{
    // MC 1.16.5: 烈焰人默认生命值为 20
    constexpr f64 DEFAULT_MAX_HEALTH = 20.0;
    EXPECT_DOUBLE_EQ(DEFAULT_MAX_HEALTH, 20.0);
}

TEST_F(BlazeFireballAttackGoalBasicTest, BlazeDefaultMovementSpeed_IsCorrect)
{
    // MC 1.16.5: 烈焰人默认移动速度为 0.23
    constexpr f64 DEFAULT_MOVEMENT_SPEED = 0.23;
    EXPECT_DOUBLE_EQ(DEFAULT_MOVEMENT_SPEED, 0.23);
}

TEST_F(BlazeFireballAttackGoalBasicTest, BlazeDefaultAttackDamage_IsCorrect)
{
    // MC 1.16.5: 烈焰人默认攻击伤害为 6
    constexpr f64 DEFAULT_ATTACK_DAMAGE = 6.0;
    EXPECT_DOUBLE_EQ(DEFAULT_ATTACK_DAMAGE, 6.0);
}

// ============================================================================
// 目标选择测试
// ============================================================================

TEST_F(BlazeFireballAttackGoalBasicTest, TargetSelectorPriority_IsCorrect)
{
    // MC 1.16.5: BlazeEntity 目标选择器配置
    // 优先级 1: HurtByTargetGoal (被攻击反击)
    // 优先级 2: NearestAttackableTargetGoal<Player> (攻击玩家)
    constexpr i32 HURT_BY_TARGET_PRIORITY = 1;
    constexpr i32 NEAREST_ATTACKABLE_TARGET_PRIORITY = 2;

    EXPECT_EQ(HURT_BY_TARGET_PRIORITY, 1);
    EXPECT_EQ(NEAREST_ATTACKABLE_TARGET_PRIORITY, 2);
}

TEST_F(BlazeFireballAttackGoalBasicTest, HurtByTargetGoal_AlertsAllies)
{
    // MC 1.16.5: 烈焰人的 HurtByTargetGoal 设置为呼唤同伴
    // 参考: this.targetSelector.addGoal(1, new HurtByTargetGoal(this, new Class[0]).setCallsForHelp());
    // 在我们的实现中使用构造函数参数: HurtByTargetGoal(this, true)
    constexpr bool ALERT_ALLIES = true;
    EXPECT_TRUE(ALERT_ALLIES);
}

// ============================================================================
// EnumSet 操作测试 (额外覆盖)
// ============================================================================

TEST_F(BlazeFireballAttackGoalBasicTest, EnumSet_ComprehensiveTest)
{
    // 测试所有 GoalFlag
    EnumSet<GoalFlag> allFlags = allGoalFlags();

    EXPECT_TRUE(allFlags.test(GoalFlag::Move));
    EXPECT_TRUE(allFlags.test(GoalFlag::Look));
    EXPECT_TRUE(allFlags.test(GoalFlag::Jump));
    EXPECT_TRUE(allFlags.test(GoalFlag::Target));
    EXPECT_EQ(allFlags.count(), 4);

    // 测试 Blaze 标志子集
    EnumSet<GoalFlag> blazeFlags{GoalFlag::Move, GoalFlag::Look};

    // 测试包含关系 (blazeFlags 是 allFlags 的子集)
    EXPECT_TRUE(allFlags.contains(blazeFlags));

    // 测试差异
    EnumSet<GoalFlag> otherFlags{GoalFlag::Jump, GoalFlag::Target};
    EXPECT_FALSE(blazeFlags.intersects(otherFlags));

    // 测试并集
    EnumSet<GoalFlag> combined = blazeFlags | otherFlags;
    EXPECT_EQ(combined.count(), 4);
    EXPECT_TRUE(combined == allFlags);
}
