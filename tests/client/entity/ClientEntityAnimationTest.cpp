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

#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::client;

class ClientEntityAnimationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { Items::initialize(); }

    void SetUp() override { entity = std::make_unique<ClientEntity>(EntityId(1), "test_entity"); }

    void TearDown() override { entity.reset(); }

    std::unique_ptr<ClientEntity> entity;
};

// ========== Swing Animation ==========

TEST_F(ClientEntityAnimationTest, SwingProgressStartsAtZero)
{
    EXPECT_FLOAT_EQ(entity->swingProgress(), 0.0f);
    EXPECT_FLOAT_EQ(entity->prevSwingProgress(), 0.0f);
}

TEST_F(ClientEntityAnimationTest, TriggerSwingMainHand_SetsSwingInProgress)
{
    entity->triggerSwingAnimation(0);
    EXPECT_TRUE(entity->isSwingInProgress());
    EXPECT_EQ(entity->swingHand(), 0);
}

TEST_F(ClientEntityAnimationTest, TriggerSwingOffHand_SetsSwingInProgress)
{
    entity->triggerSwingAnimation(1);
    EXPECT_TRUE(entity->isSwingInProgress());
    EXPECT_EQ(entity->swingHand(), 1);
}

TEST_F(ClientEntityAnimationTest, SwingProgressAdvancesOverTicks)
{
    entity->triggerSwingAnimation(0);
    EXPECT_TRUE(entity->isSwingInProgress());

    for (int i = 0; i < 3; ++i) {
        entity->tick();
    }
    EXPECT_TRUE(entity->isSwingInProgress());
    EXPECT_GT(entity->swingProgress(), 0.0f);
    EXPECT_LT(entity->swingProgress(), 1.0f);
}

TEST_F(ClientEntityAnimationTest, SwingProgressCompletesAfterDuration)
{
    entity->triggerSwingAnimation(0);

    for (int i = 0; i < 6; ++i) {
        entity->tick();
    }
    EXPECT_FALSE(entity->isSwingInProgress());
    EXPECT_FLOAT_EQ(entity->swingProgress(), 0.0f);
}

TEST_F(ClientEntityAnimationTest, SwingProgressResetsOnNewTrigger)
{
    entity->triggerSwingAnimation(0);
    for (int i = 0; i < 3; ++i) {
        entity->tick();
    }
    EXPECT_GT(entity->swingProgress(), 0.0f);

    entity->triggerSwingAnimation(0);
    EXPECT_TRUE(entity->isSwingInProgress());
}

// ========== Hurt Animation ==========

TEST_F(ClientEntityAnimationTest, TriggerHurtAnimation_SetsHurtTime)
{
    entity->triggerHurtAnimation();
    EXPECT_GT(entity->hurtTime(), 0);
}

TEST_F(ClientEntityAnimationTest, HurtTimeDecrementsOnTick)
{
    entity->triggerHurtAnimation();
    const i32 initialHurtTime = entity->hurtTime();
    entity->tick();
    EXPECT_EQ(entity->hurtTime(), initialHurtTime - 1);
}

TEST_F(ClientEntityAnimationTest, HurtTimeReachesZero)
{
    entity->triggerHurtAnimation();
    for (int i = 0; i < 20; ++i) {
        entity->tick();
        if (entity->hurtTime() <= 0) {
            break;
        }
    }
    EXPECT_EQ(entity->hurtTime(), 0);
}

// ========== PuffState ==========

TEST_F(ClientEntityAnimationTest, PuffState_DefaultIsZero)
{
    EXPECT_EQ(entity->puffState(), 0);
}

TEST_F(ClientEntityAnimationTest, PuffState_ClampedToRange)
{
    entity->setPuffState(-1);
    EXPECT_EQ(entity->puffState(), 0);

    entity->setPuffState(5);
    EXPECT_EQ(entity->puffState(), 2);

    entity->setPuffState(1);
    EXPECT_EQ(entity->puffState(), 1);

    entity->setPuffState(2);
    EXPECT_EQ(entity->puffState(), 2);
}

// ========== Interpolated Swing Progress ==========

TEST_F(ClientEntityAnimationTest, InterpolatedSwingProgress_NoSwing)
{
    EXPECT_FLOAT_EQ(entity->getInterpolatedSwingProgress(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(entity->getInterpolatedSwingProgress(0.5f), 0.0f);
    EXPECT_FLOAT_EQ(entity->getInterpolatedSwingProgress(1.0f), 0.0f);
}

TEST_F(ClientEntityAnimationTest, InterpolatedSwingProgress_AtZero)
{
    entity->triggerSwingAnimation(0);
    EXPECT_FLOAT_EQ(entity->getInterpolatedSwingProgress(0.0f), 0.0f);
}

TEST_F(ClientEntityAnimationTest, InterpolatedSwingProgress_MonotonicWithPartialTick)
{
    entity->triggerSwingAnimation(0);
    for (int i = 0; i < 3; ++i) {
        entity->tick();
    }
    const f32 atZero = entity->getInterpolatedSwingProgress(0.0f);
    const f32 atHalf = entity->getInterpolatedSwingProgress(0.5f);
    const f32 atOne = entity->getInterpolatedSwingProgress(1.0f);

    EXPECT_LE(atZero, atHalf);
    EXPECT_LE(atHalf, atOne);
}
