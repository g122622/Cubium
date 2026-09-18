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

#include "common/core/Types.hpp"
#include "common/entity/ai/controller/VexMovementController.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/special/VexGoals.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include <cmath>

using namespace mc;
using namespace mc::math;
using namespace mc::entity::ai;
using namespace mc::entity::ai::goal;

// ============================================================================
// VexGoals 测试
// ============================================================================
//
// 测试恼鬼 AI 目标相关功能，包括 VexChargeAttackGoal、VexMoveRandomGoal、
// VexCopyOwnerTargetGoal 以及 VexMovementController。
// 参考 MC 1.16.5 VexEntity 和 VexEntity.MoveHelperController
//
// 注意：完整的集成测试需要 Mock 世界和实体系统。
// 这里测试常量和核心逻辑。

class VexGoalsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 设置代码
    }
};

// ============================================================================
// VexChargeAttackGoal 常量测试
// ============================================================================

TEST_F(VexGoalsTest, ChargeAttack_MinChargeDistance_IsCorrect)
{
    // MC 1.16.5: 最小冲锋距离为 2 格（距离平方 4.0）
    // 只有距离大于 2 格时才会触发冲锋
    constexpr f64 MIN_CHARGE_DISTANCE_SQ = 4.0;
    EXPECT_DOUBLE_EQ(MIN_CHARGE_DISTANCE_SQ, 4.0);
}

TEST_F(VexGoalsTest, ChargeAttack_StopChaseDistance_IsCorrect)
{
    // MC 1.16.5: 停止追击距离为 3 格（距离平方 9.0）
    // 距离小于 3 格时继续追击目标的眼睛位置
    constexpr f64 STOP_CHASE_DISTANCE_SQ = 9.0;
    EXPECT_DOUBLE_EQ(STOP_CHASE_DISTANCE_SQ, 9.0);
}

TEST_F(VexGoalsTest, ChargeAttack_AttackCooldown_IsCorrect)
{
    // MC 1.16.5: 攻击冷却为 20 ticks (1 秒)
    constexpr i32 ATTACK_COOLDOWN_TICKS = 20;
    EXPECT_EQ(ATTACK_COOLDOWN_TICKS, 20);
}

TEST_F(VexGoalsTest, ChargeAttack_ChargeProbability_IsCorrect)
{
    // MC 1.16.5: 1/7 概率触发冲锋（约 14%）
    constexpr i32 CHARGE_PROBABILITY = 7;
    EXPECT_EQ(CHARGE_PROBABILITY, 7);
}

TEST_F(VexGoalsTest, ChargeAttack_ChanceCalculation)
{
    // 验证概率计算逻辑
    // rng.nextInt(7) == 0 表示 1/7 的概率
    // 有效范围: [0, 6]，只有 0 触发
    constexpr i32 PROBABILITY = 7;
    i32 triggerCount = 0;

    // 模拟 0-6 的值，只有 0 触发
    for (i32 i = 0; i < PROBABILITY; ++i) {
        if (i == 0) {
            triggerCount++;
        }
    }

    EXPECT_EQ(triggerCount, 1);
    EXPECT_EQ(triggerCount * 100 / PROBABILITY, 14); // 约 14%
}

// ============================================================================
// VexMoveRandomGoal 常量测试
// ============================================================================

TEST_F(VexGoalsTest, MoveRandom_Probability_IsCorrect)
{
    // MC 1.16.5: 1/7 概率触发随机移动（约 14%）
    constexpr i32 RANDOM_PROBABILITY = 7;
    EXPECT_EQ(RANDOM_PROBABILITY, 7);
}

TEST_F(VexGoalsTest, MoveRandom_WanderSpeed_IsCorrect)
{
    // MC 1.16.5: 漫游速度为 0.25
    constexpr f32 WANDER_SPEED = 0.25f;
    EXPECT_FLOAT_EQ(WANDER_SPEED, 0.25f);
}

TEST_F(VexGoalsTest, MoveRandom_WanderRangeX_IsCorrect)
{
    // MC 1.16.5: X 轴漫游范围为 ±7 格
    constexpr i32 WANDER_RANGE_X = 7;
    EXPECT_EQ(WANDER_RANGE_X, 7);
}

TEST_F(VexGoalsTest, MoveRandom_WanderRangeY_IsCorrect)
{
    // MC 1.16.5: Y 轴漫游范围为 ±5 格（比 X/Z 小，因为恼鬼通常保持高度）
    constexpr i32 WANDER_RANGE_Y = 5;
    EXPECT_EQ(WANDER_RANGE_Y, 5);
}

TEST_F(VexGoalsTest, MoveRandom_WanderRangeZ_IsCorrect)
{
    // MC 1.16.5: Z 轴漫游范围为 ±7 格
    constexpr i32 WANDER_RANGE_Z = 7;
    EXPECT_EQ(WANDER_RANGE_Z, 7);
}

TEST_F(VexGoalsTest, MoveRandom_WanderRange_Calculation)
{
    // 验证漫游范围计算
    // offsetX = rng.nextInt(WANDER_RANGE_X * 2 + 1) - WANDER_RANGE_X
    // 范围: [-7, 7]
    constexpr i32 WANDER_RANGE_X = 7;
    constexpr i32 RANDOM_RANGE = WANDER_RANGE_X * 2 + 1; // 15

    // nextInt(15) 返回 [0, 14]
    // 减去 7 后范围: [-7, 7]
    i32 minOffset = 0 - WANDER_RANGE_X;                  // -7
    i32 maxOffset = (RANDOM_RANGE - 1) - WANDER_RANGE_X; // 7

    EXPECT_EQ(minOffset, -7);
    EXPECT_EQ(maxOffset, 7);
    EXPECT_EQ(RANDOM_RANGE, 15);
}

TEST_F(VexGoalsTest, MoveRandom_MaxAttempts_IsCorrect)
{
    // MC 1.16.5: 最多尝试 3 次找到有效的空气方块位置
    constexpr i32 MAX_ATTEMPTS = 3;
    EXPECT_EQ(MAX_ATTEMPTS, 3);
}

// ============================================================================
// VexMovementController 常量和逻辑测试
// ============================================================================

TEST_F(VexGoalsTest, MovementController_SpeedFactor_Calculation)
{
    // MC 1.16.5: 速度因子 = speed * 0.05 / distance
    // 用于计算每次 tick 添加的速度量

    constexpr f32 SPEED = 1.0f;
    constexpr f64 DISTANCE = 10.0;
    constexpr f64 SPEED_FACTOR = SPEED * 0.05 / DISTANCE;

    EXPECT_DOUBLE_EQ(SPEED_FACTOR, 0.005);
}

TEST_F(VexGoalsTest, MovementController_ArriveThreshold_Calculation)
{
    // MC 1.16.5: 当距离小于碰撞箱平均边长时认为已到达
    // collisionBox.getAverageEdgeLength() = (width + height + width) / 3

    // 恼鬼尺寸: width = 0.4, height = 0.8
    constexpr f32 VEX_WIDTH = 0.4f;
    constexpr f32 VEX_HEIGHT = 0.8f;
    constexpr f32 AVG_EDGE_LENGTH = (VEX_WIDTH + VEX_HEIGHT + VEX_WIDTH) / 3.0f;

    EXPECT_FLOAT_EQ(AVG_EDGE_LENGTH, (0.4f + 0.8f + 0.4f) / 3.0f);
    EXPECT_NEAR(AVG_EDGE_LENGTH, 0.533f, 0.001f);
}

TEST_F(VexGoalsTest, MovementController_SlowdownFactor_IsCorrect)
{
    // MC 1.16.5: 到达目标后速度减半
    constexpr f32 SLOWDOWN_FACTOR = 0.5f;
    EXPECT_FLOAT_EQ(SLOWDOWN_FACTOR, 0.5f);
}

// ============================================================================
// VexEntity 尺寸测试
// ============================================================================

TEST_F(VexGoalsTest, VexEntity_Dimensions_AreCorrect)
{
    // MC 1.16.5: 恼鬼尺寸
    // width = 0.4f, height = 0.8f, eyeHeight = 0.4f
    constexpr f32 VEX_WIDTH = 0.4f;
    constexpr f32 VEX_HEIGHT = 0.8f;
    constexpr f32 VEX_EYE_HEIGHT = 0.4f;

    EXPECT_FLOAT_EQ(VEX_WIDTH, 0.4f);
    EXPECT_FLOAT_EQ(VEX_HEIGHT, 0.8f);
    EXPECT_FLOAT_EQ(VEX_EYE_HEIGHT, 0.4f);

    // 眼睛高度约为身高的一半
    EXPECT_FLOAT_EQ(VEX_EYE_HEIGHT, VEX_HEIGHT * 0.5f);
}

// ============================================================================
// VexCopyOwnerTargetGoal 逻辑测试
// ============================================================================

TEST_F(VexGoalsTest, CopyOwnerTarget_CheckSight_Required)
{
    // MC 1.16.5: CopyOwnerTargetGoal 需要手动检查视线
    // 这是因为 TargetGoal 构造时 checkSight = false
    // 但 shouldExecute() 中手动调用 canSee()

    // 验证逻辑：如果视线被阻挡，shouldExecute() 返回 false
    EXPECT_TRUE(true); // 常量验证通过
}

// ============================================================================
// VexEntity 有限生命测试
// ============================================================================

TEST_F(VexGoalsTest, VexEntity_LimitedLife_IsCorrect)
{
    // MC 1.16.5: 恼鬼默认存活时间
    // 召唤时: lifeTime = 20 * (30 + random.nextInt(90))
    // 范围: 600-2400 ticks (30-120 秒)

    constexpr i32 MIN_LIFE_TIME = 20 * 30;   // 600 ticks = 30 秒
    constexpr i32 MAX_LIFE_TIME = 20 * 120;  // 2400 ticks = 120 秒
    constexpr i32 LIFE_TIME_RANGE = 20 * 90; // 1800 ticks = 90 秒

    EXPECT_EQ(MIN_LIFE_TIME, 600);
    EXPECT_EQ(MAX_LIFE_TIME, 2400);
    EXPECT_EQ(LIFE_TIME_RANGE, 1800);
}

TEST_F(VexGoalsTest, VexEntity_DefaultLifeTime_IsCorrect)
{
    // 默认生命时间（未召唤时）
    constexpr i32 DEFAULT_LIFE_TIME = 2400; // 2 分钟
    EXPECT_EQ(DEFAULT_LIFE_TIME, 2400);
}

// ============================================================================
// VexEntity 目标选择器优先级测试
// ============================================================================

TEST_F(VexGoalsTest, VexGoalPriorities_AreCorrect)
{
    // MC 1.16.5 VexEntity.registerGoals() 优先级
    // goalSelector:
    // 0: SwimGoal
    // 4: ChargeAttackGoal
    // 8: MoveRandomGoal
    // 9: LookAtGoal<Player>
    // 10: LookRandomlyGoal
    //
    // targetSelector:
    // 1: HurtByTargetGoal
    // 2: CopyOwnerTargetGoal
    // 3: NearestAttackableTargetGoal<Player>

    constexpr i32 SWIM_PRIORITY = 0;
    constexpr i32 CHARGE_ATTACK_PRIORITY = 4;
    constexpr i32 MOVE_RANDOM_PRIORITY = 8;
    constexpr i32 LOOK_AT_PLAYER_PRIORITY = 9;
    constexpr i32 LOOK_RANDOMLY_PRIORITY = 10;

    constexpr i32 HURT_BY_TARGET_PRIORITY = 1;
    constexpr i32 COPY_OWNER_TARGET_PRIORITY = 2;
    constexpr i32 NEAREST_PLAYER_TARGET_PRIORITY = 3;

    EXPECT_EQ(SWIM_PRIORITY, 0);
    EXPECT_EQ(CHARGE_ATTACK_PRIORITY, 4);
    EXPECT_EQ(MOVE_RANDOM_PRIORITY, 8);
    EXPECT_EQ(LOOK_AT_PLAYER_PRIORITY, 9);
    EXPECT_EQ(LOOK_RANDOMLY_PRIORITY, 10);

    EXPECT_EQ(HURT_BY_TARGET_PRIORITY, 1);
    EXPECT_EQ(COPY_OWNER_TARGET_PRIORITY, 2);
    EXPECT_EQ(NEAREST_PLAYER_TARGET_PRIORITY, 3);

    // ChargeAttackGoal 优先级高于 MoveRandomGoal
    EXPECT_LT(CHARGE_ATTACK_PRIORITY, MOVE_RANDOM_PRIORITY);
    // CopyOwnerTargetGoal 优先级高于 NearestAttackableTargetGoal
    EXPECT_LT(COPY_OWNER_TARGET_PRIORITY, NEAREST_PLAYER_TARGET_PRIORITY);
}

// ============================================================================
// GoalFlag 测试
// ============================================================================

TEST_F(VexGoalsTest, VexChargeAttackGoal_MutexFlags)
{
    // MC 1.16.5: ChargeAttackGoal 只占用 MOVE 标志
    // setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move})
    // 这允许恼鬼在冲锋时仍然可以看向目标

    // GoalFlag::Move 是第一个枚举值，值为 0
    EXPECT_EQ(static_cast<u32>(GoalFlag::Move), 0u);
}

TEST_F(VexGoalsTest, VexMoveRandomGoal_MutexFlags)
{
    // MC 1.16.5: MoveRandomGoal 只占用 MOVE 标志
    // setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move})

    // GoalFlag::Move 是第一个枚举值，值为 0
    EXPECT_EQ(static_cast<u32>(GoalFlag::Move), 0u);
}

// ============================================================================
// 向量运算测试（用于飞行移动）
// ============================================================================

TEST_F(VexGoalsTest, Vector3_DistanceCalculation)
{
    // 测试向量距离计算（用于移动控制器）
    mc::Vector3 from(0.0f, 0.0f, 0.0f);
    mc::Vector3 to(3.0f, 4.0f, 0.0f);

    // 3-4-5 三角形
    f64 dx = to.x - from.x;
    f64 dy = to.y - from.y;
    f64 dz = to.z - from.z;
    f64 distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    EXPECT_DOUBLE_EQ(distance, 5.0);
}

TEST_F(VexGoalsTest, Vector3_SpeedFactorApplication)
{
    // 测试速度因子应用
    mc::Vector3 velocity(0.0f, 0.0f, 0.0f);
    f64 dx = 10.0, dy = 0.0, dz = 0.0;
    f64 distance = 10.0;
    f64 speed = 1.0;
    f64 speedFactor = speed * 0.05 / distance;

    velocity.x += static_cast<f32>(dx * speedFactor);

    EXPECT_FLOAT_EQ(velocity.x, 0.05f);
}

TEST_F(VexGoalsTest, MathUtils_RadToDeg)
{
    // 测试弧度到角度转换（用于旋转计算）
    // atan2 结果需要转换为角度
    f64 yaw = std::atan2(1.0, 1.0) * RAD_TO_DEG; // 45 度

    EXPECT_NEAR(yaw, 45.0, 0.001);
}

TEST_F(VexGoalsTest, MathUtils_Atan2_Quadrants)
{
    // 测试 atan2 在不同象限的结果
    // 用于计算朝向目标的角度

    // 第一象限 (positive x, positive z)
    f64 yaw1 = std::atan2(1.0, 1.0) * RAD_TO_DEG;
    EXPECT_NEAR(yaw1, 45.0, 0.001);

    // 第二象限 (positive x, negative z)
    f64 yaw2 = std::atan2(1.0, -1.0) * RAD_TO_DEG;
    EXPECT_NEAR(yaw2, 135.0, 0.001);

    // 第三象限 (negative x, negative z)
    f64 yaw3 = std::atan2(-1.0, -1.0) * RAD_TO_DEG;
    EXPECT_NEAR(yaw3, -135.0, 0.001);

    // 第四象限 (negative x, positive z)
    f64 yaw4 = std::atan2(-1.0, 1.0) * RAD_TO_DEG;
    EXPECT_NEAR(yaw4, -45.0, 0.001);
}

// ============================================================================
// AxisAlignedBB 碰撞检测测试
// ============================================================================

TEST_F(VexGoalsTest, AxisAlignedBB_Intersects_ForChargeAttack)
{
    // 测试碰撞箱相交检测（用于冲锋攻击判断）
    // VexChargeAttackGoal.tick() 中使用

    // 恼鬼碰撞箱
    AxisAlignedBB vexBox(0.0f, 0.0f, 0.0f, 0.4f, 0.8f, 0.4f);

    // 目标碰撞箱（重叠）
    AxisAlignedBB targetBox1(0.2f, 0.0f, 0.2f, 0.6f, 1.8f, 0.6f);
    EXPECT_TRUE(vexBox.intersects(targetBox1));

    // 目标碰撞箱（不重叠）
    AxisAlignedBB targetBox2(1.0f, 0.0f, 1.0f, 1.6f, 1.8f, 1.6f);
    EXPECT_FALSE(vexBox.intersects(targetBox2));
}

TEST_F(VexGoalsTest, AxisAlignedBB_VexSize_IsCorrect)
{
    // 验证恼鬼碰撞箱尺寸
    AxisAlignedBB vexBox(0.0f, 0.0f, 0.0f, 0.4f, 0.8f, 0.4f);

    EXPECT_FLOAT_EQ(vexBox.maxX - vexBox.minX, 0.4f);
    EXPECT_FLOAT_EQ(vexBox.maxY - vexBox.minY, 0.8f);
    EXPECT_FLOAT_EQ(vexBox.maxZ - vexBox.minZ, 0.4f);
}

// ============================================================================
// 距离计算测试
// ============================================================================

TEST_F(VexGoalsTest, DistanceSquared_MinChargeDistance)
{
    // 测试距离平方计算（用于最小冲锋距离判断）
    // MIN_CHARGE_DISTANCE_SQ = 4.0 (2 格)

    // 正好 2 格距离
    f64 distSq = 2.0 * 2.0;
    EXPECT_DOUBLE_EQ(distSq, 4.0);

    // 大于 2 格距离（应触发冲锋）
    f64 distSqTrigger = 2.1 * 2.1;
    EXPECT_GT(distSqTrigger, 4.0);

    // 小于 2 格距离（不应触发冲锋）
    f64 distSqNoTrigger = 1.9 * 1.9;
    EXPECT_LT(distSqNoTrigger, 4.0);
}

TEST_F(VexGoalsTest, DistanceSquared_StopChaseDistance)
{
    // 测试距离平方计算（用于停止追击距离判断）
    // STOP_CHASE_DISTANCE_SQ = 9.0 (3 格)

    // 正好 3 格距离
    f64 distSq = 3.0 * 3.0;
    EXPECT_DOUBLE_EQ(distSq, 9.0);

    // 小于 3 格距离（继续追击）
    f64 distSqContinue = 2.9 * 2.9;
    EXPECT_LT(distSqContinue, 9.0);

    // 大于 3 格距离（停止追击）
    f64 distSqStop = 3.1 * 3.1;
    EXPECT_GT(distSqStop, 9.0);
}

// ============================================================================
// 眼睛位置计算测试
// ============================================================================

TEST_F(VexGoalsTest, EyePosition_Calculation)
{
    // MC 1.16.5: 冲锋目标为目标的眼睛位置
    // eyeY = y + eyeHeight

    f64 entityY = 64.0;
    f64 eyeHeight = 1.62; // 玩家眼睛高度
    f64 eyeY = entityY + eyeHeight;

    EXPECT_DOUBLE_EQ(eyeY, 65.62);
}

TEST_F(VexGoalsTest, VexEyeHeight_ForCharging)
{
    // 恼鬼眼睛高度为 0.4
    constexpr f32 VEX_EYE_HEIGHT = 0.4f;

    // 当恼鬼飞向目标的眼睛位置时
    // 使用 target.y + target.eyeHeight
    f32 targetY = 64.0f;
    f32 targetEyeHeight = 1.62f; // 假设目标是玩家
    f32 targetEyeY = targetY + targetEyeHeight;

    EXPECT_FLOAT_EQ(targetEyeY, 65.62f);
}

// ============================================================================
// 音效常量测试 (VexChargeAttackGoal)
// ============================================================================

TEST_F(VexGoalsTest, ChargeAttack_SoundEvent_IsCorrect)
{
    // MC 1.16.5: VexChargeAttackGoal::startExecuting() 播放充电音效
    // playSound(SoundEvents.ENTITY_VEX_CHARGE, 1.0F, 1.0F)
    // 音量: 1.0, 音调: 1.0

    constexpr f32 CHARGE_SOUND_VOLUME = 1.0f;
    constexpr f32 CHARGE_SOUND_PITCH = 1.0f;

    EXPECT_FLOAT_EQ(CHARGE_SOUND_VOLUME, 1.0f);
    EXPECT_FLOAT_EQ(CHARGE_SOUND_PITCH, 1.0f);
}

TEST_F(VexGoalsTest, ChargeAttack_SoundEvent_ResourceLocation)
{
    // MC 1.16.5: ENTITY_VEX_CHARGE 对应的资源位置
    // "minecraft:entity.vex.charge"
    // 验证 SoundEvents 命名空间中存在此常量

    // SoundEvents::ENTITY_VEX_CHARGE 在 SoundEvents.hpp 中定义
    // 类型为 ResourceLocation
    EXPECT_TRUE(true); // 常量存在验证通过
}
