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

#include "common/entity/utils/AnimationState.hpp"

#include <limits>

using mc::i32;
using mc::entity::AnimationState;

// ============================================================================
// AnimationState 单元测试
// ============================================================================
//
// 验证 AnimationState 工具类的行为，对齐 MC 1.21.11 AnimationState 设计：
// - STOPPED 哨兵值标记未启动状态
// - start/startIfStopped/stop/animateWhen 状态转换
// - isStarted/startTick/copyFrom 查询与复制

class AnimationStateTest : public ::testing::Test {
protected:
    AnimationState m_anim;

    void SetUp() override
    {
        // 默认构造的 AnimationState 应处于未启动状态
        ASSERT_FALSE(m_anim.isStarted());
    }
};

// ==================== 默认状态测试 ====================

TEST_F(AnimationStateTest, DefaultConstructor_IsNotStarted)
{
    EXPECT_FALSE(m_anim.isStarted());
}

TEST_F(AnimationStateTest, DefaultConstructor_StartTickIsStoppedSentinel)
{
    EXPECT_EQ(m_anim.startTick(), AnimationState::STOPPED);
}

TEST_F(AnimationStateTest, StoppedSentinel_IsI32Min)
{
    // STOPPED 取 i32 最小值，确保任何有效的 tickCount 都不会与之相等
    EXPECT_EQ(AnimationState::STOPPED, std::numeric_limits<i32>::min());
}

// ==================== start 方法测试 ====================

TEST_F(AnimationStateTest, Start_SetsStartTick)
{
    m_anim.start(100);
    EXPECT_TRUE(m_anim.isStarted());
    EXPECT_EQ(m_anim.startTick(), 100);
}

TEST_F(AnimationStateTest, Start_OverwritesExistingStartTick)
{
    m_anim.start(100);
    m_anim.start(200);
    EXPECT_EQ(m_anim.startTick(), 200);
}

TEST_F(AnimationStateTest, Start_WithZeroTick)
{
    // tickCount 为 0 是合法值（实体刚生成时）
    m_anim.start(0);
    EXPECT_TRUE(m_anim.isStarted());
    EXPECT_EQ(m_anim.startTick(), 0);
}

TEST_F(AnimationStateTest, Start_WithNegativeTick)
{
    // 负值理论上不应出现，但 AnimationState 不防御
    m_anim.start(-1);
    EXPECT_TRUE(m_anim.isStarted());
    EXPECT_EQ(m_anim.startTick(), -1);
}

TEST_F(AnimationStateTest, Start_WithMaxI32)
{
    m_anim.start(std::numeric_limits<i32>::max());
    EXPECT_TRUE(m_anim.isStarted());
    EXPECT_EQ(m_anim.startTick(), std::numeric_limits<i32>::max());
}

// ==================== startIfStopped 方法测试 ====================

TEST_F(AnimationStateTest, StartIfStopped_StartsWhenNotStarted)
{
    m_anim.startIfStopped(100);
    EXPECT_TRUE(m_anim.isStarted());
    EXPECT_EQ(m_anim.startTick(), 100);
}

TEST_F(AnimationStateTest, StartIfStopped_DoesNotOverwriteExistingStartTick)
{
    m_anim.start(100);
    m_anim.startIfStopped(200);
    EXPECT_EQ(m_anim.startTick(), 100);
}

TEST_F(AnimationStateTest, StartIfStopped_AfterStop_Restarts)
{
    m_anim.start(100);
    m_anim.stop();
    m_anim.startIfStopped(200);
    EXPECT_TRUE(m_anim.isStarted());
    EXPECT_EQ(m_anim.startTick(), 200);
}

// ==================== stop 方法测试 ====================

TEST_F(AnimationStateTest, Stop_OnNotStarted_StateRemainsNotStarted)
{
    m_anim.stop();
    EXPECT_FALSE(m_anim.isStarted());
    EXPECT_EQ(m_anim.startTick(), AnimationState::STOPPED);
}

TEST_F(AnimationStateTest, Stop_OnStarted_StateBecomesNotStarted)
{
    m_anim.start(100);
    m_anim.stop();
    EXPECT_FALSE(m_anim.isStarted());
    EXPECT_EQ(m_anim.startTick(), AnimationState::STOPPED);
}

TEST_F(AnimationStateTest, Stop_Twice_StateRemainsNotStarted)
{
    m_anim.start(100);
    m_anim.stop();
    m_anim.stop();
    EXPECT_FALSE(m_anim.isStarted());
}

// ==================== animateWhen 方法测试 ====================

TEST_F(AnimationStateTest, AnimateWhen_TrueCondition_StartsIfStopped)
{
    m_anim.animateWhen(true, 100);
    EXPECT_TRUE(m_anim.isStarted());
    EXPECT_EQ(m_anim.startTick(), 100);
}

TEST_F(AnimationStateTest, AnimateWhen_FalseCondition_StopsIfStarted)
{
    m_anim.start(100);
    m_anim.animateWhen(false, 200);
    EXPECT_FALSE(m_anim.isStarted());
}

TEST_F(AnimationStateTest, AnimateWhen_TrueCondition_DoesNotOverwrite)
{
    m_anim.start(100);
    m_anim.animateWhen(true, 200);
    EXPECT_EQ(m_anim.startTick(), 100);
}

TEST_F(AnimationStateTest, AnimateWhen_FalseCondition_OnNotStarted_RemainsNotStarted)
{
    m_anim.animateWhen(false, 200);
    EXPECT_FALSE(m_anim.isStarted());
}

// ==================== copyFrom 方法测试 ====================

TEST_F(AnimationStateTest, CopyFrom_StartedState)
{
    AnimationState src;
    src.start(100);

    m_anim.copyFrom(src);
    EXPECT_TRUE(m_anim.isStarted());
    EXPECT_EQ(m_anim.startTick(), 100);
}

TEST_F(AnimationStateTest, CopyFrom_NotStartedState)
{
    AnimationState src; // 未启动

    m_anim.start(100); // m_anim 已启动
    m_anim.copyFrom(src);
    EXPECT_FALSE(m_anim.isStarted());
    EXPECT_EQ(m_anim.startTick(), AnimationState::STOPPED);
}

TEST_F(AnimationStateTest, CopyFrom_SelfCopy)
{
    m_anim.start(100);
    m_anim.copyFrom(m_anim);
    EXPECT_TRUE(m_anim.isStarted());
    EXPECT_EQ(m_anim.startTick(), 100);
}

// ==================== 完整生命周期测试 ====================

TEST_F(AnimationStateTest, FullLifecycle_StartStopRestart)
{
    // 初始未启动
    EXPECT_FALSE(m_anim.isStarted());

    // 启动
    m_anim.start(10);
    EXPECT_TRUE(m_anim.isStarted());
    EXPECT_EQ(m_anim.startTick(), 10);

    // 跳过相同 tick 的 startIfStopped 不影响
    m_anim.startIfStopped(10);
    EXPECT_EQ(m_anim.startTick(), 10);

    // 跳过不同 tick 的 startIfStopped 也不影响
    m_anim.startIfStopped(20);
    EXPECT_EQ(m_anim.startTick(), 10);

    // 停止
    m_anim.stop();
    EXPECT_FALSE(m_anim.isStarted());

    // 重新启动
    m_anim.startIfStopped(30);
    EXPECT_TRUE(m_anim.isStarted());
    EXPECT_EQ(m_anim.startTick(), 30);
}

// ==================== 旋风人使用场景模拟测试 ====================

TEST_F(AnimationStateTest, BreezeSlideToSlideBackTransitionScenario)
{
    // 模拟旋风人 slide→slideBack 过渡场景：
    // 1. 进入 Sliding Pose 时，slide.startIfStopped 被调用
    // 2. 离开 Sliding Pose 时，slideBack.start 并 slide.stop
    AnimationState slide;
    AnimationState slideBack;

    // 进入 Sliding
    slide.startIfStopped(100);
    EXPECT_TRUE(slide.isStarted());
    EXPECT_EQ(slide.startTick(), 100);
    EXPECT_FALSE(slideBack.isStarted());

    // 离开 Sliding，触发过渡
    if (slide.isStarted()) {
        slideBack.start(120);
        slide.stop();
    }

    EXPECT_FALSE(slide.isStarted());
    EXPECT_TRUE(slideBack.isStarted());
    EXPECT_EQ(slideBack.startTick(), 120);
}

TEST_F(AnimationStateTest, BreezeIdleStartIfStoppedEveryTickScenario)
{
    // 模拟旋风人 idle 动画每 tick 调用 startIfStopped
    // 应只在首次启动，后续 tick 不覆盖 startTick
    m_anim.startIfStopped(0);
    EXPECT_EQ(m_anim.startTick(), 0);

    for (i32 tick = 1; tick < 100; ++tick) {
        m_anim.startIfStopped(tick);
        EXPECT_EQ(m_anim.startTick(), 0) << "Idle startTick should not be overwritten at tick " << tick;
    }
}

TEST_F(AnimationStateTest, BreezeLongJumpStartIfStoppedScenario)
{
    // 模拟旋风人 longJump 动画在 LongJumping Pose 时每 tick 调用 startIfStopped
    m_anim.startIfStopped(50);
    EXPECT_EQ(m_anim.startTick(), 50);

    // 后续 tick 不应覆盖
    m_anim.startIfStopped(51);
    m_anim.startIfStopped(52);
    EXPECT_EQ(m_anim.startTick(), 50);
}

// ==================== 边界值测试 ====================

TEST_F(AnimationStateTest, StartWithStoppedSentinel_DoesNotMarkAsStarted)
{
    // 即使显式传入 STOPPED，start 也会设置 startTick 为 STOPPED
    // 此时 isStarted() 返回 false（因为 startTick == STOPPED）
    m_anim.start(AnimationState::STOPPED);
    EXPECT_FALSE(m_anim.isStarted());
    EXPECT_EQ(m_anim.startTick(), AnimationState::STOPPED);
}

TEST_F(AnimationStateTest, StartIfStoppedWithStoppedSentinel_OnNotStarted_RemainsNotStarted)
{
    m_anim.startIfStopped(AnimationState::STOPPED);
    // startIfStopped 调用 start(STOPPED)，isStarted 仍为 false
    EXPECT_FALSE(m_anim.isStarted());
}
