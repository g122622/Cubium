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
 * @file EntityRendererCoverageTest.cpp
 * @brief 验证渲染路径核心组件
 *
 * 测试 AnimationContext 的 hash/变更检测机制，
 * 确保所有关键动画参数（swingProgress, standingProgress, puffState 等）
 * 被正确纳入缓存失效判断。
 */

#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::client::renderer::entity::core;

class AnimationContextHashTest : public ::testing::Test {
protected:
    AnimationContext context;
};

// ========== swingProgress ==========

TEST_F(AnimationContextHashTest, SwingProgressAffectsHash)
{
    context.swingProgress = 0.0f;
    context.computeHash();
    const u32 hash0 = context.stateHash;

    context.swingProgress = 0.5f;
    context.computeHash();
    const u32 hash1 = context.stateHash;

    EXPECT_NE(hash0, hash1);
}

TEST_F(AnimationContextHashTest, SwingProgressSignificantChange)
{
    AnimationContext idle;
    idle.swingProgress = 0.0f;
    idle.computeHash();

    AnimationContext swinging;
    swinging.swingProgress = 0.5f;
    swinging.computeHash();

    EXPECT_TRUE(idle.hasSignificantChange(swinging, 0.08));
}

// ========== standingProgress (Polar Bear regression) ==========

TEST_F(AnimationContextHashTest, StandingProgressAffectsHash)
{
    context.standingProgress = 0.0f;
    context.computeHash();
    const u32 hash0 = context.stateHash;

    context.standingProgress = 1.0f;
    context.computeHash();
    const u32 hash1 = context.stateHash;

    EXPECT_NE(hash0, hash1);
}

TEST_F(AnimationContextHashTest, StandingProgressSignificantChange)
{
    AnimationContext onAllFours;
    onAllFours.standingProgress = 0.0f;
    onAllFours.computeHash();

    AnimationContext standing;
    standing.standingProgress = 1.0f;
    standing.computeHash();

    EXPECT_TRUE(onAllFours.hasSignificantChange(standing, 0.08));
}

TEST_F(AnimationContextHashTest, StandingProgressSmallChange_NotSignificant)
{
    AnimationContext a;
    a.standingProgress = 0.5f;
    a.computeHash();

    AnimationContext b;
    b.standingProgress = 0.5001f;
    b.computeHash();

    EXPECT_FALSE(a.hasSignificantChange(b, 0.08));
}

// ========== puffState (Pufferfish) ==========

TEST_F(AnimationContextHashTest, PuffStateAffectsHash)
{
    context.puffState = 0;
    context.computeHash();
    const u32 hash0 = context.stateHash;

    context.puffState = 2;
    context.computeHash();
    const u32 hash2 = context.stateHash;

    EXPECT_NE(hash0, hash2);
}

TEST_F(AnimationContextHashTest, PuffStateSignificantChange)
{
    AnimationContext deflated;
    deflated.puffState = 0;
    deflated.computeHash();

    AnimationContext inflated;
    inflated.puffState = 2;
    inflated.computeHash();

    EXPECT_TRUE(deflated.hasSignificantChange(inflated, 0.08));
}

// ========== Boolean states ==========

TEST_F(AnimationContextHashTest, IsSittingSignificantChange)
{
    AnimationContext standing;
    standing.computeHash();

    AnimationContext sitting;
    sitting.isSitting = true;
    sitting.computeHash();

    EXPECT_TRUE(standing.hasSignificantChange(sitting, 0.08));
}

TEST_F(AnimationContextHashTest, IsChildSignificantChange)
{
    AnimationContext adult;
    adult.computeHash();

    AnimationContext child;
    child.isChild = true;
    child.computeHash();

    EXPECT_TRUE(adult.hasSignificantChange(child, 0.08));
}

TEST_F(AnimationContextHashTest, IsSneakingSignificantChange)
{
    AnimationContext standing;
    standing.computeHash();

    AnimationContext sneaking;
    sneaking.isSneaking = true;
    sneaking.computeHash();

    EXPECT_TRUE(standing.hasSignificantChange(sneaking, 0.08));
}

// ========== Hash determinism ==========

TEST_F(AnimationContextHashTest, HashIsDeterministic)
{
    AnimationContext a;
    a.limbSwing = 3.14;
    a.swingProgress = 0.5f;
    a.standingProgress = 0.75f;
    a.puffState = 1;
    a.computeHash();

    AnimationContext b;
    b.limbSwing = 3.14;
    b.swingProgress = 0.5f;
    b.standingProgress = 0.75f;
    b.puffState = 1;
    b.computeHash();

    EXPECT_EQ(a.stateHash, b.stateHash);
}
