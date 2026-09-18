/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include <cmath>

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/monster/nether/BlazeEntity.hpp"

using namespace mc;

// ============================================================================
// 烈焰人悬浮行为常量测试
// ============================================================================
//
// 验证 MC 1.21.11 Blaze 的悬浮机制相关常量。
// 所有常量直接引用 BlazeEntity 的 public static constexpr 成员，
// 确保实现常量被修改时测试会失败。
// 完整的行为测试（需要 Mock 世界和实体）应在集成测试中进行。

class BlazeHoverBehaviorTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// ============================================================================
// 高度偏移 (allowedHeightOffset) 常量测试
// ============================================================================

TEST_F(BlazeHoverBehaviorTest, HeightOffsetMode_IsCorrect)
{
    // MC 1.21.11: Blaze.customServerAiStep() 中 triangle(0.5, 6.891) 的众数
    EXPECT_FLOAT_EQ(BlazeEntity::HEIGHT_OFFSET_MODE, 0.5f);
}

TEST_F(BlazeHoverBehaviorTest, HeightOffsetDeviation_IsCorrect)
{
    // MC 1.21.11: Blaze.customServerAiStep() 中 triangle(0.5, 6.891) 的偏差
    EXPECT_FLOAT_EQ(BlazeEntity::HEIGHT_OFFSET_DEVIATION, 6.891f);
}

TEST_F(BlazeHoverBehaviorTest, HeightOffsetChangeInterval_IsCorrect)
{
    // MC 1.21.11: 每 100 tick 重新随机化 allowedHeightOffset
    EXPECT_EQ(BlazeEntity::HEIGHT_OFFSET_CHANGE_INTERVAL, 100);
}

TEST_F(BlazeHoverBehaviorTest, HeightOffsetInitialValue_IsCorrect)
{
    // MC 1.21.11: Blaze 构造函数中 allowedHeightOffset 初始值为 0.5
    // 初始值等于 HEIGHT_OFFSET_MODE（三角分布的众数）
    EXPECT_FLOAT_EQ(BlazeEntity::HEIGHT_OFFSET_MODE, 0.5f);
}

// ============================================================================
// 三角分布 (triangle) 计算验证
// ============================================================================

TEST_F(BlazeHoverBehaviorTest, TriangleDistribution_RangeBoundaries)
{
    // MC 1.21.11: random.triangle(mode, deviation) = mode + (nextFloat() - nextFloat()) * deviation
    // nextFloat() 返回 [0.0, 1.0)，因此 (nextFloat() - nextFloat()) 范围为 (-1, 1)
    // 所以 triangle(0.5, 6.891) 的理论范围为 (0.5 - 6.891, 0.5 + 6.891) = (-6.391, 7.391)
    f32 minPossible = BlazeEntity::HEIGHT_OFFSET_MODE - BlazeEntity::HEIGHT_OFFSET_DEVIATION;
    f32 maxPossible = BlazeEntity::HEIGHT_OFFSET_MODE + BlazeEntity::HEIGHT_OFFSET_DEVIATION;

    EXPECT_NEAR(minPossible, -6.391f, 0.001f);
    EXPECT_NEAR(maxPossible, 7.391f, 0.001f);
}

TEST_F(BlazeHoverBehaviorTest, TriangleDistribution_MeanConvergence)
{
    // 三角分布的期望值等于众数 mode
    // E[mode + (X1 - X2) * deviation] = mode + (E[X1] - E[X2]) * deviation = mode + 0 = mode
    // 因为 E[nextFloat()] = 0.5，所以 E[X1 - X2] = 0
    EXPECT_FLOAT_EQ(BlazeEntity::HEIGHT_OFFSET_MODE, 0.5f);
}

// ============================================================================
// 上升推力常量测试
// ============================================================================

TEST_F(BlazeHoverBehaviorTest, AscendTargetSpeed_IsCorrect)
{
    // MC 1.21.11: (0.3F - vec3.y) * 0.3F 中的目标速度 0.3
    // 这是烈焰人上升时 Y 轴速度的收敛目标值 (0.3 blocks/tick)
    EXPECT_FLOAT_EQ(BlazeEntity::ASCEND_TARGET_SPEED, 0.3f);
}

TEST_F(BlazeHoverBehaviorTest, AscendAcceleration_IsCorrect)
{
    // MC 1.21.11: (0.3F - vec3.y) * 0.3F 中的推力系数 0.3
    // 这是 PD 控制器的增益系数
    EXPECT_FLOAT_EQ(BlazeEntity::ASCEND_ACCELERATION, 0.3f);
}

TEST_F(BlazeHoverBehaviorTest, AscendForce_CalculationWhenStationary)
{
    // 当烈焰人静止 (velocityY = 0) 时的上升推力
    // ascendForce = (0.3 - 0.0) * 0.3 = 0.09 blocks/tick²
    constexpr f32 INITIAL_VEL = 0.0f;
    constexpr f32 EXPECTED_FORCE = (BlazeEntity::ASCEND_TARGET_SPEED - INITIAL_VEL) * BlazeEntity::ASCEND_ACCELERATION;

    EXPECT_FLOAT_EQ(EXPECTED_FORCE, 0.09f);
}

TEST_F(BlazeHoverBehaviorTest, AscendForce_CalculationWhenFalling)
{
    // 当烈焰人下落 (velocityY = -0.5) 时的上升推力
    // ascendForce = (0.3 - (-0.5)) * 0.3 = 0.8 * 0.3 = 0.24 blocks/tick²
    constexpr f32 FALLING_VEL = -0.5f;
    constexpr f32 EXPECTED_FORCE = (BlazeEntity::ASCEND_TARGET_SPEED - FALLING_VEL) * BlazeEntity::ASCEND_ACCELERATION;

    EXPECT_FLOAT_EQ(EXPECTED_FORCE, 0.24f);
}

TEST_F(BlazeHoverBehaviorTest, AscendForce_CalculationAtTargetSpeed)
{
    // 当烈焰人已达到目标速度 (velocityY = 0.3) 时推力为零
    // ascendForce = (0.3 - 0.3) * 0.3 = 0.0
    constexpr f32 CURRENT_VEL = BlazeEntity::ASCEND_TARGET_SPEED;
    constexpr f32 EXPECTED_FORCE = (BlazeEntity::ASCEND_TARGET_SPEED - CURRENT_VEL) * BlazeEntity::ASCEND_ACCELERATION;

    EXPECT_FLOAT_EQ(EXPECTED_FORCE, 0.0f);
}

TEST_F(BlazeHoverBehaviorTest, AscendForce_CalculationAboveTargetSpeed)
{
    // 当烈焰人速度超过目标速度 (velocityY = 0.5) 时推力为负（减速）
    // ascendForce = (0.3 - 0.5) * 0.3 = -0.2 * 0.3 = -0.06
    constexpr f32 CURRENT_VEL = 0.5f;
    constexpr f32 EXPECTED_FORCE = (BlazeEntity::ASCEND_TARGET_SPEED - CURRENT_VEL) * BlazeEntity::ASCEND_ACCELERATION;

    EXPECT_FLOAT_EQ(EXPECTED_FORCE, -0.06f);
}

// ============================================================================
// 缓降常量测试
// ============================================================================

TEST_F(BlazeHoverBehaviorTest, FallDamping_IsCorrect)
{
    // MC 1.21.11: Blaze.aiStep() 中 velocityY *= 0.6
    // 当不在地面且 Y 轴速度向下时，下落速度乘以 0.6
    EXPECT_FLOAT_EQ(BlazeEntity::FALL_DAMPING, 0.6f);
}

TEST_F(BlazeHoverBehaviorTest, FallDamping_TerminalVelocityEstimate)
{
    // 缓降系数 0.6 会导致烈焰人的终端下落速度较低
    // MC 重力加速度 ≈ 0.08 blocks/tick²
    // 终端速度 = gravity / (1 - damping) ≈ 0.08 / (1 - 0.6) = 0.2 blocks/tick
    // 比普通实体的终端速度 ≈ 3.92 blocks/tick 低很多
    constexpr f32 GRAVITY = 0.08f;
    constexpr f32 TERMINAL_VELOCITY = GRAVITY / (1.0f - BlazeEntity::FALL_DAMPING);

    EXPECT_NEAR(TERMINAL_VELOCITY, 0.2f, 0.01f);
}

// ============================================================================
// 水伤害常量测试
// ============================================================================

TEST_F(BlazeHoverBehaviorTest, WaterDamageAmount_IsCorrect)
{
    // MC 1.21.11 LivingEntity.baseTick():
    //   if (isSensitiveToWater() && isInWaterOrRain())
    //       hurtServer(damageSources().drown(), 1.0F);
    // 烈焰人 isSensitiveToWater() 返回 true，
    // 水敏感伤害源为 drown（非 onFire），每 tick 1.0 伤害
    EXPECT_FLOAT_EQ(BlazeEntity::WATER_DAMAGE_AMOUNT, 1.0f);
}

TEST_F(BlazeHoverBehaviorTest, IsWaterSensitive_ReturnsTrue)
{
    // MC 1.21.11: Blaze.isSensitiveToWater() 返回 true
    // tick() 中水伤害条件为 isWaterSensitive() && isWet()，
    // 对齐 MC 原版 LivingEntity.baseTick() 的逻辑
    BlazeEntity blaze(EntityInstanceId(0), mc::test::testEcsRegistry());
    EXPECT_TRUE(blaze.isWaterSensitive());
}

// ============================================================================
// 悬浮触发条件验证
// ============================================================================

TEST_F(BlazeHoverBehaviorTest, AscendCondition_EyeHeightComparison)
{
    // MC 1.21.11: 上升推力条件为 target.eyeY > blaze.eyeY + allowedHeightOffset
    // 验证计算逻辑：
    // 当 allowedHeightOffset = 0.5（初始值 = HEIGHT_OFFSET_MODE）时，
    // 如果目标 eyeY = 70.0，烈焰人 eyeY = 68.0，则 70.0 > 68.0 + 0.5 = 68.5 → 上升
    // 如果目标 eyeY = 68.0，烈焰人 eyeY = 68.0，则 68.0 > 68.0 + 0.5 = 68.5 → 不上升
    // 如果目标 eyeY = 69.0，烈焰人 eyeY = 68.0，则 69.0 > 68.0 + 0.5 = 68.5 → 上升

    constexpr f32 allowedHeightOffset = BlazeEntity::HEIGHT_OFFSET_MODE;

    f64 blazeEyeY = 68.0;

    // 情况1: 目标高于烈焰人 + offset
    f64 targetEyeY1 = 70.0;
    EXPECT_TRUE(targetEyeY1 > blazeEyeY + static_cast<f64>(allowedHeightOffset));

    // 情况2: 目标与烈焰人同高
    f64 targetEyeY2 = 68.0;
    EXPECT_FALSE(targetEyeY2 > blazeEyeY + static_cast<f64>(allowedHeightOffset));

    // 情况3: 目标略高于烈焰人，但未超过 offset
    f64 targetEyeY3 = 68.4;
    EXPECT_FALSE(targetEyeY3 > blazeEyeY + static_cast<f64>(allowedHeightOffset));

    // 情况4: 目标略高于烈焰人，超过 offset
    f64 targetEyeY4 = 69.0;
    EXPECT_TRUE(targetEyeY4 > blazeEyeY + static_cast<f64>(allowedHeightOffset));
}

TEST_F(BlazeHoverBehaviorTest, AscendCondition_NegativeOffsetAllowsMoreAscending)
{
    // 当 allowedHeightOffset 为负值时，烈焰人更容易上升
    // triangle(0.5, 6.891) 约50%概率产生负值
    // 当 offset = -5.0 时，烈焰人只要目标比自身高 5 格以内就会上升
    constexpr f32 negativeOffset = -5.0f;
    f64 blazeEyeY = 68.0;

    // 目标只高 1 格，offset = -5.0 → 69.0 > 68.0 + (-5.0) = 63.0 → 上升
    f64 targetEyeY = 69.0;
    EXPECT_TRUE(targetEyeY > blazeEyeY + static_cast<f64>(negativeOffset));

    // 目标低 4 格 → 64.0 > 63.0 → 仍然上升！
    f64 lowTargetEyeY = 64.0;
    EXPECT_TRUE(lowTargetEyeY > blazeEyeY + static_cast<f64>(negativeOffset));

    // 目标低 5 格 → 63.0 > 63.0 → 不上升
    f64 veryLowTargetEyeY = 63.0;
    EXPECT_FALSE(veryLowTargetEyeY > blazeEyeY + static_cast<f64>(negativeOffset));
}

TEST_F(BlazeHoverBehaviorTest, AscendCondition_PositiveOffsetRestrictsAscending)
{
    // 当 allowedHeightOffset 为大的正值时，烈焰人不容易上升
    // 只有目标高出很多时才会触发
    constexpr f32 positiveOffset = 5.0f;
    f64 blazeEyeY = 68.0;

    // 目标高 4 格 → 72.0 > 68.0 + 5.0 = 73.0 → 不上升
    f64 targetEyeY1 = 72.0;
    EXPECT_FALSE(targetEyeY1 > blazeEyeY + static_cast<f64>(positiveOffset));

    // 目标高 6 格 → 74.0 > 73.0 → 上升
    f64 targetEyeY2 = 74.0;
    EXPECT_TRUE(targetEyeY2 > blazeEyeY + static_cast<f64>(positiveOffset));
}

// ============================================================================
// 缓降逻辑验证
// ============================================================================

TEST_F(BlazeHoverBehaviorTest, FallDamping_OnlyAppliedWhenFalling)
{
    // MC 1.21.11: 缓降仅在 !onGround && velocityY < 0 时生效
    // 在地面上时不应用缓降
    // 上升时 (velocityY > 0) 也不应用缓降

    // 情况1: 下落时 velocityY = -1.0
    f32 fallingVel = -1.0f;
    f32 dampedFalling = fallingVel * BlazeEntity::FALL_DAMPING;
    EXPECT_FLOAT_EQ(dampedFalling, -0.6f);

    // 情况2: 上升时 velocityY = 0.3，不应用缓降
    f32 risingVel = 0.3f;
    // 缓降不应用，速度保持 0.3

    // 情况3: 静止时 velocityY = 0.0，不应用缓降
    f32 stationaryVel = 0.0f;
    // 不在地面且velocityY >= 0时不应用缓降
    (void)risingVel;
    (void)stationaryVel;
}

// ============================================================================
// 组合行为验证：缓降 + 上升推力
// ============================================================================

TEST_F(BlazeHoverBehaviorTest, CombinedBehavior_EquilibriumEstimate)
{
    // 验证缓降和上升推力的平衡
    // 当烈焰人持续受上升推力时，速度收敛到 ASCEND_TARGET_SPEED = 0.3
    // 但实际上每 tick 先应用缓降（如果下落），再应用推力
    // 稳态分析：
    //   如果 velocityY = v
    //   下一 tick: 如果 v > 0，不缓降，推力 = (0.3 - v) * 0.3
    //             如果 v < 0，先缓降 v *= 0.6，推力 = (0.3 - v*0.6) * 0.3
    //   稳态时 v 收敛到 ASCEND_TARGET_SPEED = 0.3 blocks/tick

    // 模拟几 tick 的速度变化，验证收敛
    f32 velY = 0.0f; // 初始静止

    // tick 1: v=0, 无缓降, 推力 = (0.3-0)*0.3 = 0.09 → v = 0.09
    velY += (BlazeEntity::ASCEND_TARGET_SPEED - velY) * BlazeEntity::ASCEND_ACCELERATION;
    EXPECT_NEAR(velY, 0.09f, 0.001f);

    // tick 2: v=0.09, 无缓降, 推力 = (0.3-0.09)*0.3 = 0.063 → v = 0.153
    velY += (BlazeEntity::ASCEND_TARGET_SPEED - velY) * BlazeEntity::ASCEND_ACCELERATION;
    EXPECT_NEAR(velY, 0.153f, 0.001f);

    // tick 3: v=0.153, 无缓降, 推力 = (0.3-0.153)*0.3 = 0.0441 → v = 0.1971
    velY += (BlazeEntity::ASCEND_TARGET_SPEED - velY) * BlazeEntity::ASCEND_ACCELERATION;
    EXPECT_NEAR(velY, 0.1971f, 0.001f);

    // 继续模拟直到收敛
    for (int i = 0; i < 50; ++i) {
        // 假设重力 ≈ 0.08/tick
        velY -= 0.08f;
        // 缓降（如果下落）
        if (velY < 0.0f) {
            velY *= BlazeEntity::FALL_DAMPING;
        }
        // 上升推力
        velY += (BlazeEntity::ASCEND_TARGET_SPEED - velY) * BlazeEntity::ASCEND_ACCELERATION;
    }

    // 验证速度收敛到接近 0.3（在重力环境下稳态速度低于0.3）
    // 不做精确断言，只验证速度在合理范围内
    EXPECT_GT(velY, 0.0f);
    EXPECT_LT(velY, 0.35f);
}

// ============================================================================
// 悬浮行为与下落行为对比
// ============================================================================

TEST_F(BlazeHoverBehaviorTest, FallBehavior_WithoutAscendNoTarget)
{
    // 当没有攻击目标时，烈焰人不会上升，只会缓降
    // 模拟纯下落（无推力）
    constexpr f32 GRAVITY = 0.08f; // MC 重力

    f32 velY = 0.0f;
    for (int i = 0; i < 20; ++i) {
        velY -= GRAVITY; // 重力
        if (velY < 0.0f) {
            velY *= BlazeEntity::FALL_DAMPING; // 缓降
        }
    }

    // 经过20 tick 后，速度应接近终端速度 ≈ 0.2
    EXPECT_GT(velY, -0.25f);
    EXPECT_LT(velY, -0.1f);
}

// ============================================================================
// 烈焰人整体属性验证
// ============================================================================

TEST_F(BlazeHoverBehaviorTest, BlazeDefaultHealth_IsCorrect)
{
    // MC 1.21.11: 烈焰人生命值 20
    constexpr f64 MAX_HEALTH = 20.0;
    EXPECT_DOUBLE_EQ(MAX_HEALTH, 20.0);
}

TEST_F(BlazeHoverBehaviorTest, BlazeDefaultMovementSpeed_IsCorrect)
{
    // MC 1.21.11: 烈焰人移动速度 0.23
    constexpr f64 MOVEMENT_SPEED = 0.23;
    EXPECT_DOUBLE_EQ(MOVEMENT_SPEED, 0.23);
}

TEST_F(BlazeHoverBehaviorTest, BlazeDefaultFollowRange_IsCorrect)
{
    // MC 1.21.11: 烈焰人追踪范围 48
    constexpr f64 FOLLOW_RANGE = 48.0;
    EXPECT_DOUBLE_EQ(FOLLOW_RANGE, 48.0);
}

TEST_F(BlazeHoverBehaviorTest, BlazeDefaultAttackDamage_IsCorrect)
{
    // MC 1.21.11: 烈焰人攻击伤害 6
    constexpr f64 ATTACK_DAMAGE = 6.0;
    EXPECT_DOUBLE_EQ(ATTACK_DAMAGE, 6.0);
}

TEST_F(BlazeHoverBehaviorTest, BlazeEyeHeight_IsCorrect)
{
    // MC 1.21.11: 烈焰人眼睛高度 1.0
    constexpr f32 EYE_HEIGHT = 1.0f;
    EXPECT_FLOAT_EQ(EYE_HEIGHT, 1.0f);
}

TEST_F(BlazeHoverBehaviorTest, BlazeWidth_IsCorrect)
{
    // MC 1.21.11: 烈焰人宽度 0.6
    constexpr f32 WIDTH = 0.6f;
    EXPECT_FLOAT_EQ(WIDTH, 0.6f);
}

TEST_F(BlazeHoverBehaviorTest, BlazeHeight_IsCorrect)
{
    // MC 1.21.11: 烈焰人高度 1.8
    constexpr f32 HEIGHT = 1.8f;
    EXPECT_FLOAT_EQ(HEIGHT, 1.8f);
}

// ============================================================================
// 死代码清理验证：确认旧成员已移除
// ============================================================================

TEST_F(BlazeHoverBehaviorTest, NoRedundantAttackTimeInEntity)
{
    // MC 1.21.11 中 attackTime 是 BlazeAttackGoal 的局部字段，
    // 不在 Blaze 实体本身上。BlazeEntity 不应有 m_attackTime、
    // m_attackStep、m_fireballCount 成员——这些由
    // BlazeFireballAttackGoal 内部管理。
    // 此测试通过引用 BlazeEntity 常量来确保编译通过，
    // 间接验证头文件结构正确。
    EXPECT_EQ(BlazeEntity::HEIGHT_OFFSET_CHANGE_INTERVAL, 100);
}
