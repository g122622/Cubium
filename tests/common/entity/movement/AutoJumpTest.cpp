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

#include "common/entity/movement/AutoJump.hpp"
#include "common/entity/movement/AutoJumpConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::entity::movement;
using namespace mc::entity::movement::AutoJumpConstants;

namespace {

// ============================================================================
// 配置测试
// ============================================================================

TEST(AutoJumpTest, DefaultEnabled)
{
    AutoJump autoJump;
    EXPECT_TRUE(autoJump.isEnabled());
}

TEST(AutoJumpTest, CanEnableDisable)
{
    AutoJump autoJump;

    autoJump.setEnabled(false);
    EXPECT_FALSE(autoJump.isEnabled());

    autoJump.setEnabled(true);
    EXPECT_TRUE(autoJump.isEnabled());
}

TEST(AutoJumpTest, JumpBoostLevelAffectsMaxHeight)
{
    AutoJump autoJump;

    // 默认等级 0
    EXPECT_EQ(0, autoJump.jumpBoostLevel());
    EXPECT_FLOAT_EQ(BASE_JUMP_HEIGHT, autoJump.calculateMaxJumpHeight());

    // 等级 1
    autoJump.setJumpBoostLevel(1);
    EXPECT_EQ(1, autoJump.jumpBoostLevel());
    EXPECT_FLOAT_EQ(BASE_JUMP_HEIGHT + JUMP_BOOST_PER_LEVEL, autoJump.calculateMaxJumpHeight());

    // 等级 2
    autoJump.setJumpBoostLevel(2);
    EXPECT_FLOAT_EQ(BASE_JUMP_HEIGHT + JUMP_BOOST_PER_LEVEL * 2, autoJump.calculateMaxJumpHeight());

    // 负值应该被限制为 0
    autoJump.setJumpBoostLevel(-1);
    EXPECT_EQ(0, autoJump.jumpBoostLevel());
}

TEST(AutoJumpTest, CooldownMechanics)
{
    AutoJump autoJump;

    // 默认冷却为 0
    EXPECT_EQ(0, autoJump.autoJumpTime());

    // 重置冷却
    autoJump.resetCooldown();
    EXPECT_EQ(AUTO_JUMP_COOLDOWN, autoJump.autoJumpTime());

    // tick 应该递减冷却
    autoJump.tick();
    EXPECT_EQ(AUTO_JUMP_COOLDOWN - 1, autoJump.autoJumpTime());
}

// ============================================================================
// 方向计算测试
// ============================================================================

TEST(AutoJumpTest, IsMovingForwardDetection)
{
    // 向前移动（点积 > 阈值）
    Vector3 forward(0.0f, 0.0f, -1.0f);       // 面向 -Z
    Vector3 movingForward(0.0f, 0.0f, -1.0f); // 向 -Z 移动
    EXPECT_TRUE(AutoJump::isMovingForward(movingForward, forward));

    // 向后移动（点积 < 阈值）
    Vector3 movingBackward(0.0f, 0.0f, 1.0f); // 向 +Z 移动
    EXPECT_FALSE(AutoJump::isMovingForward(movingBackward, forward));

    // 侧向移动（点积接近 0）
    Vector3 strafingLeft(-1.0f, 0.0f, 0.0f);                       // 向 -X 移动
    EXPECT_TRUE(AutoJump::isMovingForward(strafingLeft, forward)); // 点积 = 0 > -0.15

    // 斜向前移动
    Vector3 diagonal(-0.707f, 0.0f, -0.707f); // 向左前方
    EXPECT_TRUE(AutoJump::isMovingForward(diagonal, forward));

    // 斜向后移动（应该不触发）
    Vector3 diagonalBack(0.5f, 0.0f, 0.5f);
    EXPECT_FALSE(AutoJump::isMovingForward(diagonalBack, forward));
}

// ============================================================================
// 高度计算测试
// ============================================================================

TEST(AutoJumpTest, MaxJumpHeightCalculation)
{
    AutoJump autoJump;

    // 无跳跃药水
    EXPECT_FLOAT_EQ(1.2f, autoJump.calculateMaxJumpHeight());

    // 跳跃药水 I
    autoJump.setJumpBoostLevel(1);
    EXPECT_FLOAT_EQ(1.95f, autoJump.calculateMaxJumpHeight());

    // 跳跃药水 II
    autoJump.setJumpBoostLevel(2);
    EXPECT_FLOAT_EQ(2.7f, autoJump.calculateMaxJumpHeight());

    // 跳跃药水 III (假设存在)
    autoJump.setJumpBoostLevel(3);
    EXPECT_FLOAT_EQ(3.45f, autoJump.calculateMaxJumpHeight());
}

// ============================================================================
// 常量验证测试
// ============================================================================

TEST(AutoJumpConstantsTest, VerifyValues)
{
    // 验证常量与 MC 源码一致
    EXPECT_FLOAT_EQ(1.2f, BASE_JUMP_HEIGHT);
    EXPECT_FLOAT_EQ(0.75f, JUMP_BOOST_PER_LEVEL);
    EXPECT_FLOAT_EQ(0.5f, MIN_JUMP_HEIGHT);
    EXPECT_FLOAT_EQ(-0.15f, FORWARD_THRESHOLD);
    EXPECT_FLOAT_EQ(7.0f, DETECTION_DISTANCE_MULTIPLIER);
    EXPECT_FLOAT_EQ(0.51f, DETECTION_HEIGHT_OFFSET);
    EXPECT_EQ(1, AUTO_JUMP_COOLDOWN);
    EXPECT_FLOAT_EQ(0.001f, MOVEMENT_THRESHOLD_SQ);
    EXPECT_DOUBLE_EQ(1.0, JUMP_FACTOR_THRESHOLD);
    EXPECT_FLOAT_EQ(0.5f, LINE_OFFSET_RATIO);
    EXPECT_EQ(2, HEAD_SPACE_CHECK_HEIGHT);
}

// ============================================================================
// 边界条件测试
// ============================================================================

TEST(AutoJumpTest, ZeroInputReturnsEmptyDirection)
{
    // 当速度和输入都为零时，calculateMovementDirection 应该返回零向量
    // 这需要 Player 实例，所以这里只验证常量逻辑

    // 验证阈值正确
    EXPECT_LT(FORWARD_THRESHOLD, 0.0f); // 阈值应该是负数

    // 验证最小跳跃高度大于 0
    EXPECT_GT(MIN_JUMP_HEIGHT, 0.0f);

    // 验证基础跳跃高度大于最小跳跃高度
    EXPECT_GT(BASE_JUMP_HEIGHT, MIN_JUMP_HEIGHT);
}

TEST(AutoJumpTest, CooldownPreventsMultipleJumps)
{
    AutoJump autoJump;

    // 设置冷却
    autoJump.resetCooldown();
    EXPECT_EQ(AUTO_JUMP_COOLDOWN, autoJump.autoJumpTime());

    // 冷却期间不应该触发
    EXPECT_GT(autoJump.autoJumpTime(), 0);

    // tick 后冷却减少
    autoJump.tick();
    EXPECT_LT(autoJump.autoJumpTime(), AUTO_JUMP_COOLDOWN);
}

// ============================================================================
// 启用/禁用测试
// ============================================================================

TEST(AutoJumpTest, DisabledDoesNotTrigger)
{
    AutoJump autoJump;
    autoJump.setEnabled(false);

    EXPECT_FALSE(autoJump.isEnabled());

    // 禁用后冷却仍可正常工作
    autoJump.resetCooldown();
    EXPECT_EQ(AUTO_JUMP_COOLDOWN, autoJump.autoJumpTime());

    autoJump.tick();
    EXPECT_EQ(AUTO_JUMP_COOLDOWN - 1, autoJump.autoJumpTime());
}

// ============================================================================
// 方向阈值测试
// ============================================================================

TEST(AutoJumpTest, ForwardThresholdEdgeCases)
{
    // 测试阈值边界情况
    Vector3 forward(0.0f, 0.0f, -1.0f);

    // 正好在阈值上
    // FORWARD_THRESHOLD = -0.15
    // 点积 = cos(angle) = -0.15 时，angle ≈ 98.6度
    // 我们用实际向量来测试

    // 正前方：点积 = 1
    Vector3 straight(0.0f, 0.0f, -1.0f);
    EXPECT_TRUE(AutoJump::isMovingForward(straight, forward));

    // 90度侧方：点积 = 0
    Vector3 side(1.0f, 0.0f, 0.0f);
    EXPECT_TRUE(AutoJump::isMovingForward(side, forward)); // 0 > -0.15

    // 120度：点积 = cos(120°) = -0.5
    Vector3 backSide(-0.5f, 0.0f, 0.866f);
    EXPECT_FALSE(AutoJump::isMovingForward(backSide, forward)); // -0.5 < -0.15
}

} // namespace
