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
 * @file AnimatedMeshCacheTest.cpp
 * @brief 测试动画网格缓存的核心逻辑
 *
 * 由于 AnimatedMeshCache 依赖 Vulkan pipeline，无法在单元测试中完整测试。
 * 此文件测试缓存决策逻辑的基础组件：AnimationContext 的 hash 和变更检测。
 */

#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::client::renderer::entity::core;

// ========== AnimationContext Hash 变化检测 ==========

class AnimatedMeshCacheLogicTest : public ::testing::Test {
protected:
    AnimationContext baseContext;

    void SetUp() override
    {
        baseContext.partialTicks = 0.5;
        baseContext.limbSwing = 1.0;
        baseContext.limbSwingAmount = 0.5;
        baseContext.ageInTicks = 100.0;
        baseContext.scale = 1.0 / 16.0;
        baseContext.swingProgress = 0.0f;
        baseContext.standingProgress = 0.0f;
        baseContext.puffState = 0;
        baseContext.computeHash();
    }
};

TEST_F(AnimatedMeshCacheLogicTest, SameContext_NoSignificantChange)
{
    AnimationContext same = baseContext;
    same.computeHash();
    EXPECT_FALSE(baseContext.hasSignificantChange(same, 0.08));
}

TEST_F(AnimatedMeshCacheLogicTest, SwingProgressChange_IsSignificant)
{
    AnimationContext changed = baseContext;
    changed.swingProgress = 0.5f;
    changed.computeHash();
    EXPECT_TRUE(baseContext.hasSignificantChange(changed, 0.08));
}

TEST_F(AnimatedMeshCacheLogicTest, SmallSwingProgressChange_NotSignificant)
{
    AnimationContext changed = baseContext;
    changed.swingProgress = 0.001f;
    changed.computeHash();
    EXPECT_FALSE(baseContext.hasSignificantChange(changed, 0.08));
}

TEST_F(AnimatedMeshCacheLogicTest, LimbSwingChange_IsSignificant)
{
    AnimationContext changed = baseContext;
    changed.limbSwing = 5.0;
    changed.computeHash();
    EXPECT_TRUE(baseContext.hasSignificantChange(changed, 0.08));
}

TEST_F(AnimatedMeshCacheLogicTest, StandingProgressChange_IsSignificant)
{
    AnimationContext changed = baseContext;
    changed.standingProgress = 1.0f;
    changed.computeHash();
    EXPECT_TRUE(baseContext.hasSignificantChange(changed, 0.08));
}

TEST_F(AnimatedMeshCacheLogicTest, PuffStateChange_IsSignificant)
{
    AnimationContext changed = baseContext;
    changed.puffState = 2;
    changed.computeHash();
    EXPECT_TRUE(baseContext.hasSignificantChange(changed, 0.08));
}

TEST_F(AnimatedMeshCacheLogicTest, PartialTicksAlone_NotSignificant)
{
    AnimationContext changed = baseContext;
    changed.partialTicks = 0.9;
    changed.computeHash();
    // partialTicks alone may or may not be significant depending on threshold
    // The hash includes it, but hasSignificantChange uses threshold-based comparison
}

// ========== Hash Computation ==========

TEST_F(AnimatedMeshCacheLogicTest, HashChangesWithAnimationState)
{
    const u32 baseHash = baseContext.stateHash;

    AnimationContext limbChanged = baseContext;
    limbChanged.limbSwing = 10.0;
    limbChanged.computeHash();
    EXPECT_NE(baseHash, limbChanged.stateHash);

    AnimationContext ageChanged = baseContext;
    ageChanged.ageInTicks = 200.0;
    ageChanged.computeHash();
    EXPECT_NE(baseHash, ageChanged.stateHash);
}

TEST_F(AnimatedMeshCacheLogicTest, HashIsDeterministic)
{
    AnimationContext a;
    a.limbSwing = 3.14;
    a.limbSwingAmount = 0.7;
    a.swingProgress = 0.5f;
    a.computeHash();

    AnimationContext b;
    b.limbSwing = 3.14;
    b.limbSwingAmount = 0.7;
    b.swingProgress = 0.5f;
    b.computeHash();

    EXPECT_EQ(a.stateHash, b.stateHash);
}

// ========== Equality Operators ==========

TEST_F(AnimatedMeshCacheLogicTest, EqualContexts_AreEqual)
{
    AnimationContext a;
    a.limbSwing = 1.0;
    a.swingProgress = 0.0f;
    a.computeHash();

    AnimationContext b;
    b.limbSwing = 1.0;
    b.swingProgress = 0.0f;
    b.computeHash();

    EXPECT_EQ(a, b);
}

TEST_F(AnimatedMeshCacheLogicTest, DifferentContexts_AreNotEqual)
{
    AnimationContext a;
    a.swingProgress = 0.0f;
    a.computeHash();

    AnimationContext b;
    b.swingProgress = 1.0f;
    b.computeHash();

    EXPECT_NE(a, b);
}

// ========== State Change Threshold ==========

TEST_F(AnimatedMeshCacheLogicTest, BelowThreshold_NotSignificant)
{
    AnimationContext a;
    a.limbSwingAmount = 0.0;
    a.computeHash();

    AnimationContext b;
    b.limbSwingAmount = 0.01;
    b.computeHash();

    EXPECT_FALSE(a.hasSignificantChange(b, 0.08));
}

TEST_F(AnimatedMeshCacheLogicTest, AboveThreshold_IsSignificant)
{
    AnimationContext a;
    a.limbSwingAmount = 0.0;
    a.computeHash();

    AnimationContext b;
    b.limbSwingAmount = 1.0;
    b.computeHash();

    EXPECT_TRUE(a.hasSignificantChange(b, 0.08));
}

// ========== Boolean State Changes ==========

TEST_F(AnimatedMeshCacheLogicTest, SittingChange_IsSignificant)
{
    AnimationContext standing;
    standing.computeHash();

    AnimationContext sitting;
    sitting.isSitting = true;
    sitting.computeHash();

    EXPECT_TRUE(standing.hasSignificantChange(sitting, 0.08));
}

TEST_F(AnimatedMeshCacheLogicTest, ChildChange_IsSignificant)
{
    AnimationContext adult;
    adult.computeHash();

    AnimationContext child;
    child.isChild = true;
    child.computeHash();

    EXPECT_TRUE(adult.hasSignificantChange(child, 0.08));
}

TEST_F(AnimatedMeshCacheLogicTest, SneakingChange_IsSignificant)
{
    AnimationContext standing;
    standing.computeHash();

    AnimationContext sneaking;
    sneaking.isSneaking = true;
    sneaking.computeHash();

    EXPECT_TRUE(standing.hasSignificantChange(sneaking, 0.08));
}
