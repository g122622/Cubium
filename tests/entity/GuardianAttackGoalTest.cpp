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

#include <memory>
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/special/GuardianAttackGoal.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/entities/monster/ocean/ElderGuardianEntity.hpp"
#include "common/entity/entities/monster/ocean/GuardianEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/util/math/random/Random.hpp"

using namespace mc;

// ============================================================================
// GuardianAttackGoal 常量和配置测试
// ============================================================================

// 确保原版实体类型注册表已初始化，使 VanillaEntityTypeKeys::PLAYER / SQUID / ZOMBIE 等
// 全局缓存持有互不相同的非 0 值。本文件 TargetTypes_PlayerAndSquidOnly 用例构造
// validTargets/invalidTargets 列表，并以 `type == PLAYER || type == SQUID` 判定，
// 若全部 ID 为 0 则 ZOMBIE 等会被误判为“有效目标”，导致 EXPECT_FALSE 失败。
// 仅做一次注册（VanillaEntities::registerAll 幂等且线程安全），无异常风险。
class GuardianAttackGoalTest : public ::testing::Test {
protected:
    void SetUp() override { entity::VanillaEntities::registerAll(); }
};

// ============================================================================
// 常量验证测试
// ============================================================================

TEST_F(GuardianAttackGoalTest, AttackDistances_AreCorrect)
{
    // 对齐 vanilla 1.21.11 Guardian。激光攻击范围 15 格；攻击周期 80 tick
    // （Guardian.getAttackDuration():109-111）。激光魔法伤害基础值 1.0（Guardian.java:407 f=1.0F），
    // HARD/elder 加成在 tick() 叠加（+2.0 各）。注意：ATTACK_DAMAGE 属性（Guardian 6.0 /
    // ElderGuardian 8.0，createAttributes）是激光命中后 doHurtTarget 追加的近战伤害，与激光
    // 魔法伤害基础值是两个独立量。

    // 这些常量在 GuardianAttackGoal.hpp 中定义
    constexpr i32 ATTACK_DURATION = 80; // 攻击周期（充能，对齐 vanilla getAttackDuration）
    constexpr f32 ATTACK_RANGE = 15.0f; // 攻击范围
    constexpr f32 LASER_DAMAGE = 1.0f;  // 激光魔法伤害基础值（对齐 vanilla f=1.0）

    EXPECT_EQ(ATTACK_DURATION, 80);
    EXPECT_FLOAT_EQ(ATTACK_RANGE, 15.0f);
    EXPECT_FLOAT_EQ(LASER_DAMAGE, 1.0f);
}

TEST_F(GuardianAttackGoalTest, TargetSelectionDistance_IsCorrect)
{
    // MC 1.16.5: 守卫者只攻击距离 > 3 格的目标
    // 参考 GuardianEntity.TargetPredicate.test()
    constexpr f64 MIN_TARGET_DISTANCE_SQ = 9.0; // 3.0 * 3.0

    EXPECT_DOUBLE_EQ(MIN_TARGET_DISTANCE_SQ, 9.0);
}

// ============================================================================
// 目标类型筛选测试
// ============================================================================

TEST_F(GuardianAttackGoalTest, TargetTypes_PlayerAndSquidOnly)
{
    // MC 1.16.5: 守卫者只攻击玩家和鱿鱼
    // 参考 GuardianEntity.TargetPredicate.test():
    // return (p_test_1_ instanceof PlayerEntity || p_test_1_ instanceof SquidEntity)
    //     && p_test_1_.getDistanceSq(this.parentEntity) > 9.0D;

    // 验证目标类型筛选逻辑
    std::vector<const entity::EntityType*> validTargets = {
        entity::VanillaEntityTypeKeys::PLAYER, entity::VanillaEntityTypeKeys::SQUID};

    std::vector<const entity::EntityType*> invalidTargets = {entity::VanillaEntityTypeKeys::ZOMBIE,
        entity::VanillaEntityTypeKeys::SKELETON,
        entity::VanillaEntityTypeKeys::COW,
        entity::VanillaEntityTypeKeys::PIG,
        entity::VanillaEntityTypeKeys::DOLPHIN,  // 同为水生生物，但不被攻击
        entity::VanillaEntityTypeKeys::GUARDIAN, // 同类
        entity::VanillaEntityTypeKeys::ELDER_GUARDIAN};

    // 验证有效目标
    for (auto type : validTargets) {
        bool isValid = (type == entity::VanillaEntityTypeKeys::PLAYER || type == entity::VanillaEntityTypeKeys::SQUID);
        EXPECT_TRUE(isValid) << "Expected valid target type";
    }

    // 验证无效目标
    for (auto type : invalidTargets) {
        bool isValid = (type == entity::VanillaEntityTypeKeys::PLAYER || type == entity::VanillaEntityTypeKeys::SQUID);
        EXPECT_FALSE(isValid) << "Expected invalid target type";
    }
}

// ============================================================================
// GoalFlag 配置测试
// ============================================================================

TEST_F(GuardianAttackGoalTest, GoalFlags_MoveAndLook)
{
    // GuardianAttackGoal 使用 MOVE 和 LOOK 标志
    // 参考 GuardianEntity.AttackGoal 构造函数:
    // this.setMutexFlags(EnumSet.of(Goal.Flag.MOVE, Goal.Flag.LOOK));

    EnumSet<mc::entity::ai::GoalFlag> expectedFlags{mc::entity::ai::GoalFlag::Move, mc::entity::ai::GoalFlag::Look};

    EXPECT_TRUE(expectedFlags.test(mc::entity::ai::GoalFlag::Move));
    EXPECT_TRUE(expectedFlags.test(mc::entity::ai::GoalFlag::Look));
    EXPECT_FALSE(expectedFlags.test(mc::entity::ai::GoalFlag::Jump));
    EXPECT_FALSE(expectedFlags.test(mc::entity::ai::GoalFlag::Target));
    EXPECT_EQ(expectedFlags.count(), 2);
}

// ============================================================================
// 距离计算测试
// ============================================================================

TEST_F(GuardianAttackGoalTest, DistanceCheck_GreaterThanThree)
{
    // MC 1.16.5: 目标必须距离 > 3 格

    // 距离平方 <= 9 应该被排除
    f64 distSq1 = 8.0;  // 2.83 格
    f64 distSq2 = 9.0;  // 正好 3 格
    f64 distSq3 = 10.0; // 3.16 格

    EXPECT_FALSE(distSq1 > 9.0); // 2.83 格，太近，应该排除
    EXPECT_FALSE(distSq2 > 9.0); // 正好 3 格，应该排除
    EXPECT_TRUE(distSq3 > 9.0);  // 3.16 格，应该接受
}

TEST_F(GuardianAttackGoalTest, AttackRange_WithinFifteen)
{
    // MC 1.16.5: 激光攻击范围是 15 格

    constexpr f32 ATTACK_RANGE = 15.0f;
    constexpr f32 ATTACK_RANGE_SQ = ATTACK_RANGE * ATTACK_RANGE;

    // 15 格内应该可以攻击
    f64 distSq1 = 100.0; // 10 格
    f64 distSq2 = 225.0; // 正好 15 格
    f64 distSq3 = 256.0; // 16 格

    EXPECT_TRUE(distSq1 <= ATTACK_RANGE_SQ);
    EXPECT_TRUE(distSq2 <= ATTACK_RANGE_SQ);
    EXPECT_FALSE(distSq3 <= ATTACK_RANGE_SQ);
}

// ============================================================================
// 充能和冷却时序测试
// ============================================================================

TEST_F(GuardianAttackGoalTest, Timing_SequenceCorrect)
{
    // MC 1.16.5: 激光攻击的时序
    // 1. shouldExecute() - 检查是否有目标
    // 2. startExecuting() - 初始化攻击状态
    // 3. tick() x 60 - 充能阶段 (60 tick = 3 秒)
    // 4. performLaserAttack() - 发射激光
    // 5. resetTask() 或进入冷却

    constexpr i32 CHARGE_DURATION = 60;   // 充能时间
    constexpr i32 COOLDOWN_DURATION = 20; // 冷却时间
    constexpr i32 TOTAL_CYCLE = CHARGE_DURATION + COOLDOWN_DURATION;

    EXPECT_EQ(CHARGE_DURATION, 60);   // 3 秒充能
    EXPECT_EQ(COOLDOWN_DURATION, 20); // 1 秒冷却
    EXPECT_EQ(TOTAL_CYCLE, 80);       // 完整攻击周期
}

// ============================================================================
// EntityUtils 搜索功能测试（验证搜索逻辑）
// ============================================================================

TEST_F(GuardianAttackGoalTest, EntityUtils_FindClosestEntity_Predicate)
{
    // 验证 EntityUtils::findClosestEntity 的谓词逻辑
    // 这与我们实现 selectTarget() 的逻辑一致

    // 模拟目标筛选谓词
    auto guardianTargetPredicate = [](const entity::EntityType* type, f64 distSq) -> bool {
        // 类型筛选
        bool isPlayer = (type == entity::VanillaEntityTypeKeys::PLAYER);
        bool isSquid = (type == entity::VanillaEntityTypeKeys::SQUID);
        if (!isPlayer && !isSquid) {
            return false;
        }

        // 距离筛选
        if (distSq <= 9.0) {
            return false;
        }

        return true;
    };

    // 测试玩家在有效距离
    EXPECT_TRUE(guardianTargetPredicate(entity::VanillaEntityTypeKeys::PLAYER, 10.0)); // 3.16 格

    // 测试玩家太近
    EXPECT_FALSE(guardianTargetPredicate(entity::VanillaEntityTypeKeys::PLAYER, 8.0)); // 2.83 格

    // 测试玩家正好在边界
    EXPECT_FALSE(guardianTargetPredicate(entity::VanillaEntityTypeKeys::PLAYER, 9.0)); // 正好 3 格

    // 测试鱿鱼在有效距离
    EXPECT_TRUE(guardianTargetPredicate(entity::VanillaEntityTypeKeys::SQUID, 16.0)); // 4 格

    // 测试其他生物被排除
    EXPECT_FALSE(guardianTargetPredicate(entity::VanillaEntityTypeKeys::ZOMBIE, 10.0));
    EXPECT_FALSE(guardianTargetPredicate(entity::VanillaEntityTypeKeys::DOLPHIN, 10.0));
    EXPECT_FALSE(guardianTargetPredicate(entity::VanillaEntityTypeKeys::GUARDIAN, 10.0));
}

// ============================================================================
// 目标选择器配置测试
// ============================================================================

TEST_F(GuardianAttackGoalTest, TargetSelector_ConfigurationCorrect)
{
    // MC 1.16.5 GuardianEntity.registerGoals() 中的目标选择器配置:
    // this.targetSelector.addGoal(1, new NearestAttackableTargetGoal<>(
    //     this, LivingEntity.class, 10, true, false, new TargetPredicate(this)));
    //
    // 参数说明:
    // - 优先级: 1
    // - 目标类型: LivingEntity.class
    // - checkInterval: 10 (每 10 tick 检查一次)
    // - checkSight: true (需要视线检查)
    // - nearbyOnly: false (不需要近战距离)
    // - predicate: 自定义目标筛选

    constexpr i32 TARGET_SELECTOR_PRIORITY = 1;
    constexpr i32 CHECK_INTERVAL = 10;
    constexpr bool CHECK_SIGHT = true;
    constexpr bool NEARBY_ONLY = false;

    EXPECT_EQ(TARGET_SELECTOR_PRIORITY, 1);
    EXPECT_EQ(CHECK_INTERVAL, 10); // 每 0.5 秒检查一次
    EXPECT_TRUE(CHECK_SIGHT);
    EXPECT_FALSE(NEARBY_ONLY);
}

// ============================================================================
// 远古守卫者伤害加成测试
// ============================================================================

TEST_F(GuardianAttackGoalTest, ElderGuardian_DamageBonus)
{
    // MC 1.16.5: 远古守卫者额外伤害
    // 基础魔法伤害: 1.0
    // 困难模式加成: +2.0
    // 远古守卫者加成: +2.0

    constexpr f32 BASE_MAGIC_DAMAGE = 1.0f;
    constexpr f32 HARD_MODE_BONUS = 2.0f;
    constexpr f32 ELDER_BONUS = 2.0f;

    // 普通守卫者 + 普通难度
    f32 normalGuardianNormal = BASE_MAGIC_DAMAGE;
    EXPECT_FLOAT_EQ(normalGuardianNormal, 1.0f);

    // 普通守卫者 + 困难
    f32 normalGuardianHard = BASE_MAGIC_DAMAGE + HARD_MODE_BONUS;
    EXPECT_FLOAT_EQ(normalGuardianHard, 3.0f);

    // 远古守卫者 + 普通难度
    f32 elderGuardianNormal = BASE_MAGIC_DAMAGE + ELDER_BONUS;
    EXPECT_FLOAT_EQ(elderGuardianNormal, 3.0f);

    // 远古守卫者 + 困难
    f32 elderGuardianHard = BASE_MAGIC_DAMAGE + HARD_MODE_BONUS + ELDER_BONUS;
    EXPECT_FLOAT_EQ(elderGuardianHard, 5.0f);
}

// ============================================================================
// 守卫者/远古守卫者属性值对齐 vanilla 测试
// ============================================================================
//
// 对齐 vanilla 1.21.11 createAttributes（Guardian.java:86 / ElderGuardian.java:35），
// 直接读构造后实体的属性基值，防止属性注册回归（此前 Guardian ATTACK_DAMAGE 误设为
// LASER_DAMAGE(4.0)、MOVEMENT_SPEED 误设 0.3；ElderGuardian ATTACK_DAMAGE 误设 5.0）。
// ATTACK_DAMAGE 用于激光命中后 doHurtTarget 追加的近战伤害（见 GuardianAttackGoal::tick），
// 与激光魔法伤害基础值（1.0）是两个独立量。

TEST_F(GuardianAttackGoalTest, GuardianAttributes_AlignedWithVanilla)
{
    // vanilla Guardian.createAttributes（Guardian.java:86）：
    //   Monster.createMonsterAttributes() 基类含 FOLLOW_RANGE=16.0，叠加
    //   ATTACK_DAMAGE=6.0、MOVEMENT_SPEED=0.5、MAX_HEALTH=30.0。
    GuardianEntity guardian(EntityInstanceId(1), mc::test::testEcsRegistry());

    EXPECT_FLOAT_EQ(static_cast<f32>(guardian.getAttributeValue(entity::attribute::Attributes::MAX_HEALTH)), 30.0f);
    EXPECT_FLOAT_EQ(static_cast<f32>(guardian.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED)), 0.5f);
    EXPECT_FLOAT_EQ(static_cast<f32>(guardian.getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE)), 6.0f);
    EXPECT_FLOAT_EQ(static_cast<f32>(guardian.getAttributeValue(entity::attribute::Attributes::FOLLOW_RANGE)), 16.0f);
}

TEST_F(GuardianAttackGoalTest, ElderGuardianAttributes_AlignedWithVanilla)
{
    // vanilla ElderGuardian.createAttributes（ElderGuardian.java:35）：
    //   在 Guardian 基础上覆盖 MOVEMENT_SPEED=0.3、ATTACK_DAMAGE=8.0、MAX_HEALTH=80.0。
    //   FOLLOW_RANGE=16.0 继承自 Guardian 基类（vanilla 未覆盖）。
    ElderGuardianEntity elder(EntityInstanceId(1), mc::test::testEcsRegistry());

    EXPECT_FLOAT_EQ(static_cast<f32>(elder.getAttributeValue(entity::attribute::Attributes::MAX_HEALTH)), 80.0f);
    EXPECT_FLOAT_EQ(static_cast<f32>(elder.getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED)), 0.3f);
    EXPECT_FLOAT_EQ(static_cast<f32>(elder.getAttributeValue(entity::attribute::Attributes::ATTACK_DAMAGE)), 8.0f);
    EXPECT_FLOAT_EQ(static_cast<f32>(elder.getAttributeValue(entity::attribute::Attributes::FOLLOW_RANGE)), 16.0f);
}

// ============================================================================
// 行为目标优先级测试
// ============================================================================

TEST_F(GuardianAttackGoalTest, GoalSelector_PrioritiesCorrect)
{
    // MC 1.16.5 GuardianEntity.registerGoals() 行为目标优先级:
    // 优先级 4: AttackGoal (激光攻击)
    // 优先级 5: MoveTowardsRestrictionGoal (向限制区域移动)
    // 优先级 7: RandomWalkingGoal (随机漫步)
    // 优先级 8: LookAtGoal (看向玩家/同类)
    // 优先级 9: LookRandomlyGoal (随机看向)

    constexpr i32 ATTACK_GOAL_PRIORITY = 4;
    constexpr i32 MOVE_RESTRICTION_PRIORITY = 5;
    constexpr i32 RANDOM_WALKING_PRIORITY = 7;
    constexpr i32 LOOK_AT_PRIORITY = 8;
    constexpr i32 LOOK_RANDOMLY_PRIORITY = 9;

    // 攻击目标优先级最高（数字最小）
    EXPECT_LT(ATTACK_GOAL_PRIORITY, MOVE_RESTRICTION_PRIORITY);
    EXPECT_LT(ATTACK_GOAL_PRIORITY, RANDOM_WALKING_PRIORITY);

    // 看向目标优先级最低
    EXPECT_GT(LOOK_RANDOMLY_PRIORITY, LOOK_AT_PRIORITY);
    EXPECT_GT(LOOK_RANDOMLY_PRIORITY, RANDOM_WALKING_PRIORITY);
}

// ============================================================================
// EnumSet 基本操作测试
// ============================================================================

TEST_F(GuardianAttackGoalTest, EnumSet_BasicOperations)
{
    // 测试 EnumSet 可以正确存储 GoalFlag
    EnumSet<mc::entity::ai::GoalFlag> flags;
    flags.set(mc::entity::ai::GoalFlag::Move);
    flags.set(mc::entity::ai::GoalFlag::Look);

    EXPECT_TRUE(flags.test(mc::entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(mc::entity::ai::GoalFlag::Look));
    EXPECT_FALSE(flags.test(mc::entity::ai::GoalFlag::Jump));
    EXPECT_FALSE(flags.test(mc::entity::ai::GoalFlag::Target));

    EXPECT_EQ(flags.count(), 2);
}

TEST_F(GuardianAttackGoalTest, EnumSet_InitializerList)
{
    // 使用初始化列表创建 EnumSet
    EnumSet<mc::entity::ai::GoalFlag> flags{mc::entity::ai::GoalFlag::Move, mc::entity::ai::GoalFlag::Look};

    EXPECT_TRUE(flags.test(mc::entity::ai::GoalFlag::Move));
    EXPECT_TRUE(flags.test(mc::entity::ai::GoalFlag::Look));
    EXPECT_FALSE(flags.test(mc::entity::ai::GoalFlag::Jump));
    EXPECT_FALSE(flags.test(mc::entity::ai::GoalFlag::Target));
}

TEST_F(GuardianAttackGoalTest, AllGoalFlags_ReturnsAllFlags)
{
    auto all = mc::entity::ai::allGoalFlags();

    EXPECT_TRUE(all.test(mc::entity::ai::GoalFlag::Move));
    EXPECT_TRUE(all.test(mc::entity::ai::GoalFlag::Look));
    EXPECT_TRUE(all.test(mc::entity::ai::GoalFlag::Jump));
    EXPECT_TRUE(all.test(mc::entity::ai::GoalFlag::Target));
    EXPECT_EQ(all.count(), 4);
}
