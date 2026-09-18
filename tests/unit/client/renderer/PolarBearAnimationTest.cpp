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

#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::client::renderer::entity::core;

class AnimationContextTest : public ::testing::Test {
protected:
    AnimationContext context;
};

// ========== standingProgress Hash ==========

TEST_F(AnimationContextTest, StandingProgressIncludedInHash)
{
    context.standingProgress = 0.0f;
    context.computeHash();
    const u32 hash0 = context.stateHash;

    context.standingProgress = 1.0f;
    context.computeHash();
    const u32 hash1 = context.stateHash;

    EXPECT_NE(hash0, hash1);
}

TEST_F(AnimationContextTest, StandingProgressDifferentValues_HashDiffers)
{
    context.standingProgress = 0.25f;
    context.computeHash();
    const u32 hashA = context.stateHash;

    context.standingProgress = 0.75f;
    context.computeHash();
    const u32 hashB = context.stateHash;

    EXPECT_NE(hashA, hashB);
}

TEST_F(AnimationContextTest, StandingProgressSameValues_HashEqual)
{
    context.standingProgress = 0.5f;
    context.limbSwing = 3.0;
    context.computeHash();
    const u32 hashA = context.stateHash;

    AnimationContext other;
    other.standingProgress = 0.5f;
    other.limbSwing = 3.0;
    other.computeHash();
    const u32 hashB = other.stateHash;

    EXPECT_EQ(hashA, hashB);
}

// ========== standingProgress Significant Change ==========

TEST_F(AnimationContextTest, StandingProgressChange_IsSignificant)
{
    context.standingProgress = 0.0f;
    context.computeHash();

    AnimationContext other;
    other.standingProgress = 1.0f;
    other.computeHash();

    EXPECT_TRUE(context.hasSignificantChange(other, 0.01));
}

TEST_F(AnimationContextTest, StandingProgressSmallChange_NotSignificant)
{
    context.standingProgress = 0.5f;
    context.computeHash();

    AnimationContext other;
    other.standingProgress = 0.5001f;
    other.computeHash();

    EXPECT_FALSE(context.hasSignificantChange(other, 0.01));
}

TEST_F(AnimationContextTest, StandingProgressZeroChange_NotSignificant)
{
    context.standingProgress = 0.5f;
    context.computeHash();

    AnimationContext other;
    other.standingProgress = 0.5f;
    other.computeHash();

    EXPECT_FALSE(context.hasSignificantChange(other, 0.001));
}

// ========== puffState Hash ==========

TEST_F(AnimationContextTest, PuffStateIncludedInHash)
{
    context.puffState = 0;
    context.computeHash();
    const u32 hash0 = context.stateHash;

    context.puffState = 2;
    context.computeHash();
    const u32 hash2 = context.stateHash;

    EXPECT_NE(hash0, hash2);
}

TEST_F(AnimationContextTest, PuffStateChange_IsSignificant)
{
    context.puffState = 0;
    context.computeHash();

    AnimationContext other;
    other.puffState = 1;
    other.computeHash();

    EXPECT_TRUE(context.hasSignificantChange(other, 0.01));
}

// ========== Standing Animation Scale Simulation ==========

TEST_F(AnimationContextTest, StandingProgressIncreasesFromZero)
{
    context.standingProgress = 0.0f;
    context.computeHash();
    const u32 startHash = context.stateHash;

    context.standingProgress = 0.5f;
    context.computeHash();
    const u32 midHash = context.stateHash;

    context.standingProgress = 1.0f;
    context.computeHash();
    const u32 endHash = context.stateHash;

    EXPECT_NE(startHash, midHash);
    EXPECT_NE(midHash, endHash);
    EXPECT_NE(startHash, endHash);
}

TEST_F(AnimationContextTest, StandingProgressDecreasesFromOne)
{
    context.standingProgress = 1.0f;
    context.computeHash();
    const u32 standingHash = context.stateHash;

    context.standingProgress = 0.5f;
    context.computeHash();
    const u32 midHash = context.stateHash;

    context.standingProgress = 0.0f;
    context.computeHash();
    const u32 idleHash = context.stateHash;

    EXPECT_NE(standingHash, midHash);
    EXPECT_NE(midHash, idleHash);
    EXPECT_NE(standingHash, idleHash);
}

// ========== Equality Operators ==========

TEST_F(AnimationContextTest, EqualContexts_CompareEqual)
{
    AnimationContext a;
    a.standingProgress = 0.75f;
    a.puffState = 1;
    a.limbSwing = 2.0;
    a.computeHash();

    AnimationContext b;
    b.standingProgress = 0.75f;
    b.puffState = 1;
    b.limbSwing = 2.0;
    b.computeHash();

    EXPECT_EQ(a, b);
}

TEST_F(AnimationContextTest, DifferentStandingProgress_CompareNotEqual)
{
    AnimationContext a;
    a.standingProgress = 0.0f;
    a.computeHash();

    AnimationContext b;
    b.standingProgress = 1.0f;
    b.computeHash();

    EXPECT_NE(a, b);
}
