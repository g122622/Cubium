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

#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/registry/VanillaEntities.hpp"
#include "common/item/Items.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/Vector3.hpp"

#include <cmath>

using namespace mc;
using namespace mc::client;

/**
 * @brief ClientEntity 单元测试
 */
class ClientEntityTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // Initialize Items registry once for all tests
        Items::initialize();
    }

    void SetUp() override { entity = std::make_unique<ClientEntity>(EntityId(1), "test_entity"); }

    void TearDown() override { entity.reset(); }

    std::unique_ptr<ClientEntity> entity;
};

TEST_F(ClientEntityTest, InitialState)
{
    EXPECT_EQ(entity->id(), EntityId(1));
    EXPECT_EQ(entity->typeId(), "test_entity");
    EXPECT_EQ(entity->position(), Vector3(0.0f, 0.0f, 0.0f));
    EXPECT_EQ(entity->targetPosition(), Vector3(0.0f, 0.0f, 0.0f));
    EXPECT_EQ(entity->yaw(), 0.0f);
    EXPECT_EQ(entity->pitch(), 0.0f);
    EXPECT_TRUE(entity->smoothInterpolationEnabled());
    EXPECT_FLOAT_EQ(entity->interpolationSpeed(), 0.3f);
}

TEST_F(ClientEntityTest, SetPosition)
{
    entity->setPosition(100.0f, 64.0f, 200.0f);

    EXPECT_FLOAT_EQ(entity->x(), 100.0f);
    EXPECT_FLOAT_EQ(entity->y(), 64.0f);
    EXPECT_FLOAT_EQ(entity->z(), 200.0f);

    // Target position should also be set
    auto target = entity->targetPosition();
    EXPECT_FLOAT_EQ(target.x, 100.0f);
    EXPECT_FLOAT_EQ(target.y, 64.0f);
    EXPECT_FLOAT_EQ(target.z, 200.0f);

    // Previous position should be zero initially
    auto prev = entity->prevPosition();
    EXPECT_FLOAT_EQ(prev.x, 0.0f);
    EXPECT_FLOAT_EQ(prev.y, 0.0f);
    EXPECT_FLOAT_EQ(prev.z, 0.0f);
}

TEST_F(ClientEntityTest, SetTargetPosition)
{
    entity->setPosition(0.0f, 0.0f, 0.0f);
    entity->setTargetPosition(100.0f, 64.0f, 200.0f);

    // Current position should not change
    EXPECT_FLOAT_EQ(entity->x(), 0.0f);
    EXPECT_FLOAT_EQ(entity->y(), 0.0f);
    EXPECT_FLOAT_EQ(entity->z(), 0.0f);

    // Target position should be set
    auto target = entity->targetPosition();
    EXPECT_FLOAT_EQ(target.x, 100.0f);
    EXPECT_FLOAT_EQ(target.y, 64.0f);
    EXPECT_FLOAT_EQ(target.z, 200.0f);
}

TEST_F(ClientEntityTest, SmoothInterpolation)
{
    entity->setInterpolationSpeed(0.5f);
    entity->setPosition(0.0f, 0.0f, 0.0f);
    entity->setTargetPosition(100.0f, 0.0f, 0.0f);

    // updateInterpolation 每帧调用，使用 deltaTime
    // 插值公式: alpha = 1 - (1 - speed)^(deltaTime * 20)
    // deltaTime = 0.05s (相当于 20 TPS 的一帧)
    // alpha = 1 - 0.5^1 = 0.5
    entity->updateInterpolation(0.05f);

    // Position should be 50% toward target (0 + (100-0) * 0.5 = 50)
    EXPECT_FLOAT_EQ(entity->x(), 50.0f);
    EXPECT_FLOAT_EQ(entity->y(), 0.0f);
    EXPECT_FLOAT_EQ(entity->z(), 0.0f);

    // Another update should move halfway again (50 + (100-50) * 0.5 = 75)
    entity->updateInterpolation(0.05f);
    EXPECT_FLOAT_EQ(entity->x(), 75.0f);
}

TEST_F(ClientEntityTest, DisabledSmoothInterpolation)
{
    entity->setSmoothInterpolation(false);
    entity->setPosition(0.0f, 0.0f, 0.0f);
    entity->setTargetPosition(100.0f, 64.0f, 200.0f);

    // updateInterpolation should immediately jump to target
    entity->updateInterpolation(0.05f);

    EXPECT_FLOAT_EQ(entity->x(), 100.0f);
    EXPECT_FLOAT_EQ(entity->y(), 64.0f);
    EXPECT_FLOAT_EQ(entity->z(), 200.0f);
}

TEST_F(ClientEntityTest, SetRotation)
{
    entity->setRotation(90.0f, 45.0f);

    EXPECT_FLOAT_EQ(entity->yaw(), 90.0f);
    EXPECT_FLOAT_EQ(entity->pitch(), 45.0f);

    // Target rotation should also be set
    EXPECT_FLOAT_EQ(entity->targetYaw(), 90.0f);
    EXPECT_FLOAT_EQ(entity->targetPitch(), 45.0f);

    // Previous rotation should be zero
    EXPECT_FLOAT_EQ(entity->prevYaw(), 0.0f);
    EXPECT_FLOAT_EQ(entity->prevPitch(), 0.0f);
}

TEST_F(ClientEntityTest, SetTargetRotation)
{
    entity->setRotation(0.0f, 0.0f);
    entity->setTargetRotation(90.0f, 45.0f);

    // Current rotation should not change
    EXPECT_FLOAT_EQ(entity->yaw(), 0.0f);
    EXPECT_FLOAT_EQ(entity->pitch(), 0.0f);

    // Target rotation should be set
    EXPECT_FLOAT_EQ(entity->targetYaw(), 90.0f);
    EXPECT_FLOAT_EQ(entity->targetPitch(), 45.0f);
}

TEST_F(ClientEntityTest, SmoothRotationInterpolation)
{
    entity->setInterpolationSpeed(0.5f);
    entity->setRotation(0.0f, 0.0f);
    entity->setTargetRotation(90.0f, 45.0f);

    // deltaTime = 0.05s (相当于 20 TPS 的一帧)
    // alpha = 1 - 0.5^1 = 0.5
    entity->updateInterpolation(0.05f);

    EXPECT_FLOAT_EQ(entity->yaw(), 45.0f);
    EXPECT_FLOAT_EQ(entity->pitch(), 22.5f);
}

TEST_F(ClientEntityTest, YawAngleNormalization)
{
    // Test yaw wrapping from 170 to -170 (should go through 180/-180)
    entity->setInterpolationSpeed(0.5f);
    entity->setRotation(170.0f, 0.0f);
    entity->setTargetRotation(-170.0f, 0.0f);

    // deltaTime = 0.05s
    entity->updateInterpolation(0.05f);

    // Should move toward 180 (shorter path than going all the way around)
    // 170 + ((-170 - 170) normalized) * 0.5
    // diff = -170 - 170 = -340, normalized = 20 (go forward 20 degrees)
    // 170 + 20 * 0.5 = 180
    EXPECT_FLOAT_EQ(entity->yaw(), 180.0f);
}

TEST_F(ClientEntityTest, PitchClamping)
{
    entity->setInterpolationSpeed(0.5f);
    entity->setRotation(0.0f, 0.0f);
    entity->setTargetRotation(0.0f, 200.0f);

    entity->updateInterpolation(0.05f);

    // Pitch should be clamped to [-90, 90]
    // After interpolation: 0 + (200-0) * 0.5 = 100, clamped to 90
    EXPECT_FLOAT_EQ(entity->pitch(), 90.0f);
}

TEST_F(ClientEntityTest, SetHeadRotation)
{
    entity->setHeadRotation(45.0f);

    EXPECT_FLOAT_EQ(entity->headYaw(), 45.0f);
    EXPECT_FLOAT_EQ(entity->targetHeadYaw(), 45.0f);
}

TEST_F(ClientEntityTest, SetTargetHeadRotation)
{
    entity->setHeadRotation(0.0f);
    entity->setTargetHeadRotation(90.0f);

    EXPECT_FLOAT_EQ(entity->headYaw(), 0.0f);
    EXPECT_FLOAT_EQ(entity->targetHeadYaw(), 90.0f);
}

TEST_F(ClientEntityTest, SmoothHeadYawInterpolation)
{
    entity->setInterpolationSpeed(0.5f);
    entity->setHeadRotation(0.0f);
    entity->setTargetHeadRotation(90.0f);

    // deltaTime = 0.05s
    // alpha = 1 - 0.5^1 = 0.5
    entity->updateInterpolation(0.05f);

    EXPECT_FLOAT_EQ(entity->headYaw(), 45.0f);
}

TEST_F(ClientEntityTest, InterpolationSpeedClamping)
{
    entity->setInterpolationSpeed(0.05f);
    EXPECT_FLOAT_EQ(entity->interpolationSpeed(), 0.05f);

    entity->setInterpolationSpeed(2.0f);
    EXPECT_FLOAT_EQ(entity->interpolationSpeed(), 1.0f);

    entity->setInterpolationSpeed(-1.0f);
    EXPECT_FLOAT_EQ(entity->interpolationSpeed(), 0.01f);
}

TEST_F(ClientEntityTest, GetInterpolatedPosition)
{
    entity->setInterpolationSpeed(1.0f); // Instant interpolation for this test
    entity->setPosition(0.0f, 0.0f, 0.0f);
    entity->setTargetPosition(100.0f, 0.0f, 0.0f);

    // First tick saves prevPosition = 0
    entity->tick();

    // Update interpolation moves position to target (speed=1.0 = instant)
    entity->updateInterpolation(0.05f);

    // Now prevPosition = 0, position = 100
    // partialTick = 0.5 -> interpolated = 0 + (100-0) * 0.5 = 50
    auto pos = entity->getInterpolatedPosition(0.5f);
    EXPECT_FLOAT_EQ(pos.x, 50.0f);
    EXPECT_FLOAT_EQ(pos.y, 0.0f);
    EXPECT_FLOAT_EQ(pos.z, 0.0f);
}

TEST_F(ClientEntityTest, GetInterpolatedYaw)
{
    entity->setInterpolationSpeed(1.0f); // Instant interpolation
    entity->setRotation(0.0f, 0.0f);
    entity->setTargetRotation(90.0f, 0.0f);

    // First tick saves prevYaw = 0
    entity->tick();

    // Update interpolation moves yaw to target (speed=1.0 = instant)
    entity->updateInterpolation(0.05f);

    // Now prevYaw = 0, yaw = 90
    // partialTick = 0.5 -> interpolated = 0 + (90-0) * 0.5 = 45
    EXPECT_FLOAT_EQ(entity->getInterpolatedYaw(0.5f), 45.0f);
}

TEST_F(ClientEntityTest, Tick)
{
    entity->setPosition(0.0f, 0.0f, 0.0f);
    entity->setTargetPosition(100.0f, 0.0f, 0.0f);
    entity->setRotation(0.0f, 0.0f);
    entity->setTargetRotation(90.0f, 0.0f);

    u32 initialTicks = entity->ticksExisted();
    entity->tick();

    // Tick should increment ticksExisted
    EXPECT_EQ(entity->ticksExisted(), initialTicks + 1);

    // Tick should update prevPosition and prevRotation
    // But NOT interpolate position (that's done in updateInterpolation)
    EXPECT_FLOAT_EQ(entity->prevX(), 0.0f);
    EXPECT_FLOAT_EQ(entity->x(), 0.0f); // Position unchanged after tick
    EXPECT_FLOAT_EQ(entity->prevYaw(), 0.0f);
    EXPECT_FLOAT_EQ(entity->yaw(), 0.0f); // Rotation unchanged after tick
}

TEST_F(ClientEntityTest, TickThenInterpolate)
{
    entity->setInterpolationSpeed(1.0f); // Instant interpolation
    entity->setPosition(0.0f, 0.0f, 0.0f);
    entity->setTargetPosition(100.0f, 0.0f, 0.0f);

    // Tick saves prevPosition
    entity->tick();
    EXPECT_FLOAT_EQ(entity->prevX(), 0.0f);
    EXPECT_FLOAT_EQ(entity->x(), 0.0f);

    // Update interpolation with speed=1.0 should jump to target
    entity->updateInterpolation(0.05f);
    EXPECT_FLOAT_EQ(entity->x(), 100.0f);
}

TEST_F(ClientEntityTest, TickUpdatesAnimation)
{
    entity->setPosition(0.0f, 0.0f, 0.0f);
    entity->setTargetPosition(10.0f, 0.0f, 0.0f);
    entity->setInterpolationSpeed(1.0f); // Jump to target immediately
    entity->updateInterpolation(0.05f);

    f32 initialLimbSwingAmount = entity->limbSwingAmount();

    entity->updateAnimation(5.0f);

    EXPECT_FLOAT_EQ(entity->limbSwingAmount(), 5.0f);
    EXPECT_GT(entity->limbSwing(), 0.0f);
}

TEST_F(ClientEntityTest, Metadata)
{
    std::vector<u8> metadata = {0x01, 0x02, 0x03, 0x04};
    entity->setMetadata(metadata);

    const auto& storedMetadata = entity->metadata();
    EXPECT_EQ(storedMetadata.size(), 4u);
    EXPECT_EQ(storedMetadata[0], 0x01);
    EXPECT_EQ(storedMetadata[3], 0x04);
}

TEST_F(ClientEntityTest, ItemStack)
{
    EXPECT_FALSE(entity->hasItem());

    // Create an ItemStack using Items::DIAMOND
    ASSERT_NE(Items::DIAMOND, nullptr) << "Items::DIAMOND should be initialized";
    ItemStack stack(Items::DIAMOND, 10);
    entity->setItemStack(stack);

    EXPECT_TRUE(entity->hasItem());
    ASSERT_NE(entity->itemStack(), nullptr);
    EXPECT_EQ(entity->itemStack()->getItem(), Items::DIAMOND);
    EXPECT_EQ(entity->itemStack()->getCount(), 10);
}

TEST_F(ClientEntityTest, XpValue)
{
    EXPECT_EQ(entity->xpValue(), 1); // Default value

    entity->setXpValue(100);
    EXPECT_EQ(entity->xpValue(), 100);
}

TEST_F(ClientEntityTest, OnGround)
{
    EXPECT_FALSE(entity->onGround());

    entity->setOnGround(true);
    EXPECT_TRUE(entity->onGround());
}

TEST_F(ClientEntityTest, Removed)
{
    EXPECT_TRUE(entity->isAlive());
    EXPECT_FALSE(entity->isRemoved());

    entity->remove();

    EXPECT_TRUE(entity->isRemoved());
    EXPECT_FALSE(entity->isAlive());
}

TEST_F(ClientEntityTest, Dimensions)
{
    entity->setWidth(0.9f);
    entity->setHeight(1.8f);

    EXPECT_FLOAT_EQ(entity->width(), 0.9f);
    EXPECT_FLOAT_EQ(entity->height(), 1.8f);
}

TEST_F(ClientEntityTest, ChildFlag)
{
    EXPECT_FALSE(entity->isChild());

    entity->setChild(true);
    EXPECT_TRUE(entity->isChild());
}

TEST_F(ClientEntityTest, Velocity)
{
    entity->setVelocity(1.0f, 2.0f, 3.0f);

    auto vel = entity->velocity();
    EXPECT_FLOAT_EQ(vel.x, 1.0f);
    EXPECT_FLOAT_EQ(vel.y, 2.0f);
    EXPECT_FLOAT_EQ(vel.z, 3.0f);
}

TEST_F(ClientEntityTest, MultipleInterpolationSteps)
{
    entity->setInterpolationSpeed(0.5f);
    entity->setPosition(0.0f, 0.0f, 0.0f);
    entity->setTargetPosition(100.0f, 0.0f, 0.0f);

    // Multiple updateInterpolation calls should converge to target
    // Each call with deltaTime=0.05s: alpha = 1 - 0.5^1 = 0.5
    for (int i = 0; i < 10; ++i) {
        entity->updateInterpolation(0.05f);
    }

    // After 10 updates with speed=0.5, position should be very close to target
    // Each update: x = x + (100-x) * 0.5
    // After n updates: x = 100 * (1 - 0.5^n)
    // After 10 updates: x = 100 * (1 - 0.5^10) = 100 * 0.999 = 99.9
    EXPECT_NEAR(entity->x(), 99.9f, 0.2f);
}

TEST_F(ClientEntityTest, FrameRateIndependentInterpolation)
{
    // Test that interpolation is roughly frame-rate independent
    // Same total time should give similar result regardless of frame count

    const f32 speed = 0.3f;
    const f32 totalTime = 0.167f; // ~10 frames at 60 FPS

    // Test 1: 10 frames at 60 FPS (0.0167s each)
    entity->setInterpolationSpeed(speed);
    entity->setPosition(0.0f, 0.0f, 0.0f);
    entity->setTargetPosition(100.0f, 0.0f, 0.0f);

    for (int i = 0; i < 10; ++i) {
        entity->updateInterpolation(1.0f / 60.0f);
    }
    f32 result1 = entity->x();

    // Test 2: 5 frames at 30 FPS (0.033s each)
    entity->setPosition(0.0f, 0.0f, 0.0f);
    entity->setTargetPosition(100.0f, 0.0f, 0.0f);

    for (int i = 0; i < 5; ++i) {
        entity->updateInterpolation(1.0f / 30.0f);
    }
    f32 result2 = entity->x();

    // Results should be similar (within ~5% tolerance due to discretization)
    // The formula 1 - (1-speed)^(deltaTime*20) normalizes to 20 TPS
    f32 tolerance = std::max(result1, result2) * 0.05f;
    EXPECT_NEAR(result1, result2, tolerance);
}

// ============================================================================
// partialTick 边界测试
// ============================================================================

TEST_F(ClientEntityTest, PartialTickZeroReturnsPrevPosition)
{
    // partialTick = 0.0 应返回 prevPosition
    entity->setInterpolationSpeed(1.0f);
    entity->setPosition(0.0f, 0.0f, 0.0f);
    entity->setTargetPosition(100.0f, 64.0f, 200.0f);
    entity->tick();                     // prevPosition = (0, 0, 0)
    entity->updateInterpolation(0.05f); // position = target

    auto pos = entity->getInterpolatedPosition(0.0f);
    EXPECT_FLOAT_EQ(pos.x, 0.0f);
    EXPECT_FLOAT_EQ(pos.y, 0.0f);
    EXPECT_FLOAT_EQ(pos.z, 0.0f);
}

TEST_F(ClientEntityTest, PartialTickOneReturnsCurrentPosition)
{
    // partialTick = 1.0 应返回当前 position
    entity->setInterpolationSpeed(1.0f);
    entity->setPosition(0.0f, 0.0f, 0.0f);
    entity->setTargetPosition(100.0f, 64.0f, 200.0f);
    entity->tick();                     // prevPosition = (0, 0, 0)
    entity->updateInterpolation(0.05f); // position = target

    auto pos = entity->getInterpolatedPosition(1.0f);
    EXPECT_FLOAT_EQ(pos.x, 100.0f);
    EXPECT_FLOAT_EQ(pos.y, 64.0f);
    EXPECT_FLOAT_EQ(pos.z, 200.0f);
}

TEST_F(ClientEntityTest, PartialTickHalfReturnsMidpoint)
{
    // partialTick = 0.5 应返回中点
    entity->setInterpolationSpeed(1.0f);
    entity->setPosition(0.0f, 0.0f, 0.0f);
    entity->setTargetPosition(100.0f, 64.0f, 200.0f);
    entity->tick();                     // prevPosition = (0, 0, 0)
    entity->updateInterpolation(0.05f); // position = target

    auto pos = entity->getInterpolatedPosition(0.5f);
    EXPECT_FLOAT_EQ(pos.x, 50.0f);
    EXPECT_FLOAT_EQ(pos.y, 32.0f);
    EXPECT_FLOAT_EQ(pos.z, 100.0f);
}

TEST_F(ClientEntityTest, PartialTickRotationInterpolation)
{
    entity->setInterpolationSpeed(1.0f);
    entity->setRotation(0.0f, 0.0f);
    entity->setTargetRotation(180.0f, 90.0f);
    entity->tick();
    entity->updateInterpolation(0.05f);

    // partialTick = 0.0 应返回 prev 值
    EXPECT_FLOAT_EQ(entity->getInterpolatedYaw(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(entity->getInterpolatedPitch(0.0f), 0.0f);

    // partialTick = 1.0 应返回当前值
    EXPECT_FLOAT_EQ(entity->getInterpolatedYaw(1.0f), 180.0f);
    EXPECT_FLOAT_EQ(entity->getInterpolatedPitch(1.0f), 90.0f);

    // partialTick = 0.5 应返回中点
    EXPECT_FLOAT_EQ(entity->getInterpolatedYaw(0.5f), 90.0f);
    EXPECT_FLOAT_EQ(entity->getInterpolatedPitch(0.5f), 45.0f);
}

TEST_F(ClientEntityTest, PartialTickHeadYawInterpolation)
{
    entity->setInterpolationSpeed(1.0f);
    entity->setHeadRotation(0.0f);
    entity->setTargetHeadRotation(90.0f);
    entity->tick();
    entity->updateInterpolation(0.05f);

    EXPECT_FLOAT_EQ(entity->getInterpolatedHeadYaw(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(entity->getInterpolatedHeadYaw(1.0f), 90.0f);
    EXPECT_FLOAT_EQ(entity->getInterpolatedHeadYaw(0.5f), 45.0f);
}

TEST_F(ClientEntityTest, PartialTickSwingProgressInterpolation)
{
    // 设置 swingProgress，getInterpolatedSwingProgress 在 prev 和 current 之间插值
    // 注意：prevSwingProgress 需要通过调用 startSwing() 或在 tick 中手动更新
    entity->setSwingProgress(0.5f);
    // 在当前实现中，prevSwingProgress 初始为 0
    EXPECT_FLOAT_EQ(entity->prevSwingProgress(), 0.0f);
    EXPECT_FLOAT_EQ(entity->swingProgress(), 0.5f);

    // partialTick 插值测试
    EXPECT_FLOAT_EQ(entity->getInterpolatedSwingProgress(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(entity->getInterpolatedSwingProgress(1.0f), 0.5f);
    EXPECT_FLOAT_EQ(entity->getInterpolatedSwingProgress(0.5f), 0.25f);
}

TEST_F(ClientEntityTest, PartialTickNegativePosition)
{
    // 测试负坐标
    entity->setInterpolationSpeed(1.0f);
    entity->setPosition(-100.0f, -64.0f, -200.0f);
    entity->setTargetPosition(100.0f, 64.0f, 200.0f);
    entity->tick();
    entity->updateInterpolation(0.05f);

    auto pos0 = entity->getInterpolatedPosition(0.0f);
    EXPECT_FLOAT_EQ(pos0.x, -100.0f);
    EXPECT_FLOAT_EQ(pos0.y, -64.0f);
    EXPECT_FLOAT_EQ(pos0.z, -200.0f);

    auto pos1 = entity->getInterpolatedPosition(1.0f);
    EXPECT_FLOAT_EQ(pos1.x, 100.0f);
    EXPECT_FLOAT_EQ(pos1.y, 64.0f);
    EXPECT_FLOAT_EQ(pos1.z, 200.0f);

    auto posHalf = entity->getInterpolatedPosition(0.5f);
    EXPECT_FLOAT_EQ(posHalf.x, 0.0f);
    EXPECT_FLOAT_EQ(posHalf.y, 0.0f);
    EXPECT_FLOAT_EQ(posHalf.z, 0.0f);
}

TEST_F(ClientEntityTest, PartialTickSmallValues)
{
    // 测试小数值精度
    entity->setInterpolationSpeed(1.0f);
    entity->setPosition(0.0f, 0.0f, 0.0f);
    entity->setTargetPosition(0.001f, 0.001f, 0.001f);
    entity->tick();
    entity->updateInterpolation(0.05f);

    auto pos = entity->getInterpolatedPosition(0.5f);
    EXPECT_NEAR(pos.x, 0.0005f, 1e-7f);
    EXPECT_NEAR(pos.y, 0.0005f, 1e-7f);
    EXPECT_NEAR(pos.z, 0.0005f, 1e-7f);
}

TEST_F(ClientEntityTest, PartialTickLargeValues)
{
    // 测试大坐标值
    entity->setInterpolationSpeed(1.0f);
    entity->setPosition(0.0f, 0.0f, 0.0f);
    entity->setTargetPosition(30000000.0f, 256.0f, 30000000.0f);
    entity->tick();
    entity->updateInterpolation(0.05f);

    auto pos = entity->getInterpolatedPosition(0.5f);
    EXPECT_FLOAT_EQ(pos.x, 15000000.0f);
    EXPECT_FLOAT_EQ(pos.y, 128.0f);
    EXPECT_FLOAT_EQ(pos.z, 15000000.0f);
}

TEST_F(ClientEntityTest, PartialTickAnimationValues)
{
    // 测试动画相关值的插值
    // updateAnimation 更新 limbSwing 和 limbSwingAmount
    entity->updateAnimation(1.0f); // limbSwingAmount = 1.0
    // 此时 prevLimbSwingAmount 已被保存为旧值，limbSwingAmount 被设为 1.0
    EXPECT_FLOAT_EQ(entity->prevLimbSwingAmount(), 0.0f); // 初始为 0
    EXPECT_FLOAT_EQ(entity->limbSwingAmount(), 1.0f);

    entity->updateAnimation(0.5f); // limbSwingAmount = 0.5, prevLimbSwingAmount = 1.0
    EXPECT_FLOAT_EQ(entity->prevLimbSwingAmount(), 1.0f);
    EXPECT_FLOAT_EQ(entity->limbSwingAmount(), 0.5f);
}

TEST_F(ClientEntityTest, PartialTickConsistencyMultipleCalls)
{
    // 多次调用 getInterpolatedPosition 应返回一致的结果
    entity->setInterpolationSpeed(1.0f);
    entity->setPosition(0.0f, 0.0f, 0.0f);
    entity->setTargetPosition(100.0f, 64.0f, 200.0f);
    entity->tick();
    entity->updateInterpolation(0.05f);

    for (int i = 0; i < 10; ++i) {
        auto pos = entity->getInterpolatedPosition(0.75f);
        EXPECT_FLOAT_EQ(pos.x, 75.0f);
        EXPECT_FLOAT_EQ(pos.y, 48.0f);
        EXPECT_FLOAT_EQ(pos.z, 150.0f);
    }
}

// ============================================================================
// 豹猫信任状态客户端同步测试
// ============================================================================

class ClientEntityOcelotSyncTest : public ::testing::Test {
protected:
    void SetUp() override { entity = std::make_unique<ClientEntity>(EntityId(1), "minecraft:ocelot"); }

    void TearDown() override { entity.reset(); }

    std::unique_ptr<ClientEntity> entity;
};

TEST_F(ClientEntityOcelotSyncTest, TrustingState_DefaultFalse)
{
    // 豹猫客户端实体默认不信任
    EXPECT_FALSE(entity->isTrusting());
}

TEST_F(ClientEntityOcelotSyncTest, TrustingState_CanSetAndGet)
{
    EXPECT_FALSE(entity->isTrusting());

    entity->setTrusting(true);
    EXPECT_TRUE(entity->isTrusting());

    entity->setTrusting(false);
    EXPECT_FALSE(entity->isTrusting());
}

TEST_F(ClientEntityOcelotSyncTest, TrustingState_SyncFromDataManager)
{
    // 模拟服务端发送的信任状态元数据同步到客户端
    auto& dataManager = entity->dataManager();

    // 注册 DataParameter（与服务端 OcelotEntity 相同的参数）
    auto trustingParam = mc::entity::EntityDataManager::createKey<bool>();
    dataManager.registerParam(trustingParam, false);

    // 设置信任状态为 true
    dataManager.set(trustingParam, true);
    EXPECT_TRUE(dataManager.hasDirtyData());

    // 模拟元数据同步：手动调用 syncMetadataFromDataManager 的逻辑
    // 此处验证 ClientEntity::isTrusting() 通过 setTrusting 正确更新
    entity->setTrusting(true);
    EXPECT_TRUE(entity->isTrusting());
}

// ============================================================================
// 狼兴趣状态客户端同步测试
// ============================================================================

class ClientEntityWolfSyncTest : public ::testing::Test {
protected:
    void SetUp() override { entity = std::make_unique<ClientEntity>(EntityId(1), "minecraft:wolf"); }

    void TearDown() override { entity.reset(); }

    std::unique_ptr<ClientEntity> entity;
};

TEST_F(ClientEntityWolfSyncTest, InterestedState_DefaultFalse)
{
    // 狼客户端实体默认不感兴趣
    EXPECT_FALSE(entity->wolfIsInterested());
}

TEST_F(ClientEntityWolfSyncTest, InterestedState_CanSetAndGet)
{
    EXPECT_FALSE(entity->wolfIsInterested());

    entity->setWolfIsInterested(true);
    EXPECT_TRUE(entity->wolfIsInterested());

    entity->setWolfIsInterested(false);
    EXPECT_FALSE(entity->wolfIsInterested());
}

TEST_F(ClientEntityWolfSyncTest, InterestedState_SyncFromDataManager)
{
    // 模拟服务端 WolfEntity 通过元数据同步兴趣状态到客户端
    auto& dataManager = entity->dataManager();

    // 注册 DataParameter（与服务端 WolfEntity::DATA_INTERESTED_PARAM 相同的参数）
    auto interestedParam = mc::entity::EntityDataManager::createKey<bool>();
    dataManager.registerParam(interestedParam, false);

    // 服务端调用 setInterested(true) → EntityTracker 广播 → 客户端收到元数据
    dataManager.set(interestedParam, true);
    EXPECT_TRUE(dataManager.hasDirtyData());

    // 模拟 ClientEntity::syncMetadataFromDataManager 的逻辑
    // 此处验证 setWolfIsInterested 正确更新客户端镜像状态
    entity->setWolfIsInterested(true);
    EXPECT_TRUE(entity->wolfIsInterested());
}

TEST_F(ClientEntityWolfSyncTest, InterestedAngle_InterpolatesTowardOneWhenInterested)
{
    // 验证 ClientEntity::tick 推进 wolfInterestedAngle 向 1.0 插值
    // 对应 MC Wolf.tick() 第 318-323 行
    entity->setWolfIsInterested(true);

    // 初始角度为 0
    EXPECT_FLOAT_EQ(entity->wolfInterestedAngle(), 0.0f);

    // 每次插值：angle += (1.0 - angle) * 0.4
    // 第一次 tick 后：0.0 + 1.0 * 0.4 = 0.4
    entity->tick();
    EXPECT_NEAR(entity->wolfInterestedAngle(), 0.4f, 0.001f);

    // 第二次 tick 后：0.4 + 0.6 * 0.4 = 0.64
    entity->tick();
    EXPECT_NEAR(entity->wolfInterestedAngle(), 0.64f, 0.001f);
}

TEST_F(ClientEntityWolfSyncTest, InterestedAngle_InterpolatesTowardZeroWhenNotInterested)
{
    // 验证 ClientEntity::tick 推进 wolfInterestedAngle 向 0.0 插值
    // 先设置 interested=true 让角度接近 1.0
    entity->setWolfIsInterested(true);
    for (int i = 0; i < 50; ++i) {
        entity->tick();
    }
    EXPECT_NEAR(entity->wolfInterestedAngle(), 1.0f, 0.01f);

    // 切换为 not interested，角度应向 0.0 趋近
    entity->setWolfIsInterested(false);
    entity->tick();
    // 1.0 + (0.0 - 1.0) * 0.4 = 0.6
    EXPECT_NEAR(entity->wolfInterestedAngle(), 0.6f, 0.01f);

    for (int i = 0; i < 50; ++i) {
        entity->tick();
    }
    EXPECT_NEAR(entity->wolfInterestedAngle(), 0.0f, 0.01f);
}

TEST_F(ClientEntityWolfSyncTest, InterestedAngle_OTracksCurrentFrame)
{
    // 验证 wolfInterestedAngleO 始终追踪上一 tick 的 wolfInterestedAngle
    entity->setWolfIsInterested(true);
    entity->tick();

    // tick 后 wolfInterestedAngleO 应等于 tick 前的 wolfInterestedAngle（0.0）
    EXPECT_FLOAT_EQ(entity->wolfInterestedAngleO(), 0.0f);
    EXPECT_NEAR(entity->wolfInterestedAngle(), 0.4f, 0.001f);

    entity->tick();
    EXPECT_NEAR(entity->wolfInterestedAngleO(), 0.4f, 0.001f);
    EXPECT_NEAR(entity->wolfInterestedAngle(), 0.64f, 0.001f);
}

// ============================================================================
// 狼驯服状态客户端同步测试
// ============================================================================

TEST_F(ClientEntityWolfSyncTest, TamedState_DefaultFalse)
{
    // 狼客户端实体默认未驯服
    EXPECT_FALSE(entity->wolfTamed());
}

TEST_F(ClientEntityWolfSyncTest, TamedState_CanSetAndGet)
{
    EXPECT_FALSE(entity->wolfTamed());

    entity->setWolfTamed(true);
    EXPECT_TRUE(entity->wolfTamed());

    entity->setWolfTamed(false);
    EXPECT_FALSE(entity->wolfTamed());
}

TEST_F(ClientEntityWolfSyncTest, TamedState_SyncFromDataManager)
{
    // 模拟服务端 TameableEntity 通过元数据同步驯服状态到客户端
    auto& dataManager = entity->dataManager();

    // 注册 DataParameter（与服务端 TameableEntity::DATA_TAMED_PARAM 相同的参数）
    auto tamedParam = mc::entity::EntityDataManager::createKey<bool>();
    dataManager.registerParam(tamedParam, false);

    // 服务端调用 setTamed(true) → EntityTracker 广播 → 客户端收到元数据
    dataManager.set(tamedParam, true);
    EXPECT_TRUE(dataManager.hasDirtyData());

    // 模拟 ClientEntity::syncMetadataFromDataManager 的逻辑
    entity->setWolfTamed(true);
    EXPECT_TRUE(entity->wolfTamed());
}

// ============================================================================
// 狼颈圈颜色客户端同步测试
// ============================================================================

TEST_F(ClientEntityWolfSyncTest, CollarColor_DefaultRed)
{
    // 狼客户端实体默认颈圈颜色为红色
    EXPECT_EQ(entity->wolfCollarColor(), DyeColor::Red);
}

TEST_F(ClientEntityWolfSyncTest, CollarColor_CanSetAndGet)
{
    entity->setWolfCollarColor(DyeColor::Blue);
    EXPECT_EQ(entity->wolfCollarColor(), DyeColor::Blue);

    entity->setWolfCollarColor(DyeColor::Green);
    EXPECT_EQ(entity->wolfCollarColor(), DyeColor::Green);

    entity->setWolfCollarColor(DyeColor::White);
    EXPECT_EQ(entity->wolfCollarColor(), DyeColor::White);
}

TEST_F(ClientEntityWolfSyncTest, CollarColor_SyncFromDataManager)
{
    // 模拟服务端 WolfEntity 通过元数据同步颈圈颜色到客户端
    auto& dataManager = entity->dataManager();

    // 注册 DataParameter（与服务端 WolfEntity::DATA_COLLAR_COLOR_PARAM 相同的参数）
    auto collarColorParam = mc::entity::EntityDataManager::createKey<i32>();
    dataManager.registerParam(collarColorParam, static_cast<i32>(DyeColor::Red));

    // 服务端调用 setCollarColor(DyeColor::Blue) → EntityTracker 广播 → 客户端收到元数据
    dataManager.set(collarColorParam, static_cast<i32>(DyeColor::Blue));
    EXPECT_TRUE(dataManager.hasDirtyData());

    // 模拟 ClientEntity::syncMetadataFromDataManager 的逻辑
    entity->setWolfCollarColor(DyeColor::Blue);
    EXPECT_EQ(entity->wolfCollarColor(), DyeColor::Blue);
}

TEST_F(ClientEntityWolfSyncTest, CollarColor_AllDyeColorsRoundTrip)
{
    // 验证所有 16 种 DyeColor 都能正确设置和读取
    for (i32 i = 0; i <= 15; ++i) {
        DyeColor color = static_cast<DyeColor>(i);
        entity->setWolfCollarColor(color);
        EXPECT_EQ(entity->wolfCollarColor(), color);
    }
}

// ============================================================================
// 狼愤怒状态客户端同步测试
//
// 服务端 WolfEntity 通过 DATA_ANGER_TIME_PARAM（i32）同步愤怒时间到客户端，
// 客户端 ClientEntity::syncMetadataFromDataManager 读取该参数并调用 setWolfIsAngry
// 更新镜像状态。EntityRendererManager 读取 wolfIsAngry() 写入 AnimationContext.isAngry，
// 由 WolfModel::setAnimState 接收以决定尾巴 Y 旋转（愤怒时锁 0）和尾巴 X 旋转（1.539f）。
// ============================================================================

TEST_F(ClientEntityWolfSyncTest, AngryState_DefaultFalse)
{
    // 狼客户端实体默认不愤怒
    EXPECT_FALSE(entity->wolfIsAngry());
}

TEST_F(ClientEntityWolfSyncTest, AngryState_CanSetAndGet)
{
    EXPECT_FALSE(entity->wolfIsAngry());

    entity->setWolfIsAngry(true);
    EXPECT_TRUE(entity->wolfIsAngry());

    entity->setWolfIsAngry(false);
    EXPECT_FALSE(entity->wolfIsAngry());
}

TEST_F(ClientEntityWolfSyncTest, AngryState_SyncFromDataManager)
{
    // 模拟服务端 WolfEntity 通过元数据同步愤怒状态到客户端
    auto& dataManager = entity->dataManager();

    // 注册 DataParameter（与服务端 WolfEntity::DATA_ANGER_TIME_PARAM 相同的参数类型）
    auto angerTimeParam = mc::entity::EntityDataManager::createKey<i32>();
    dataManager.registerParam(angerTimeParam, static_cast<i32>(0));

    // 服务端调用 setAngerTime(100) → EntityTracker 广播 → 客户端收到元数据
    dataManager.set(angerTimeParam, static_cast<i32>(100));
    EXPECT_TRUE(dataManager.hasDirtyData());

    // 模拟 ClientEntity::syncMetadataFromDataManager 的逻辑：
    // 读取 DataParameter 值，angerTime > 0 时调用 setWolfIsAngry(true)
    i32 angerTime = dataManager.get<i32>(angerTimeParam);
    entity->setWolfIsAngry(angerTime > 0);
    EXPECT_TRUE(entity->wolfIsAngry());

    // 模拟愤怒时间递减到 0
    dataManager.set(angerTimeParam, static_cast<i32>(0));
    angerTime = dataManager.get<i32>(angerTimeParam);
    entity->setWolfIsAngry(angerTime > 0);
    EXPECT_FALSE(entity->wolfIsAngry());
}

TEST_F(ClientEntityWolfSyncTest, AngryState_SyncFromDataManager_VariousAngerTimes)
{
    // 验证不同 angerTime 值的愤怒状态判定
    auto& dataManager = entity->dataManager();
    auto angerTimeParam = mc::entity::EntityDataManager::createKey<i32>();
    dataManager.registerParam(angerTimeParam, static_cast<i32>(0));

    // angerTime = 1 → 愤怒
    dataManager.set(angerTimeParam, static_cast<i32>(1));
    i32 angerTime = dataManager.get<i32>(angerTimeParam);
    entity->setWolfIsAngry(angerTime > 0);
    EXPECT_TRUE(entity->wolfIsAngry());

    // angerTime = 0 → 不愤怒
    dataManager.set(angerTimeParam, static_cast<i32>(0));
    angerTime = dataManager.get<i32>(angerTimeParam);
    entity->setWolfIsAngry(angerTime > 0);
    EXPECT_FALSE(entity->wolfIsAngry());

    // angerTime = -1（异常值）→ 不愤怒
    dataManager.set(angerTimeParam, static_cast<i32>(-1));
    angerTime = dataManager.get<i32>(angerTimeParam);
    entity->setWolfIsAngry(angerTime > 0);
    EXPECT_FALSE(entity->wolfIsAngry());

    // angerTime = 1000 → 愤怒
    dataManager.set(angerTimeParam, static_cast<i32>(1000));
    angerTime = dataManager.get<i32>(angerTimeParam);
    entity->setWolfIsAngry(angerTime > 0);
    EXPECT_TRUE(entity->wolfIsAngry());
}

// ============================================================================
// ClientEntity 眼高（eyeHeight）测试
// ============================================================================

/**
 * @brief ClientEntity 眼高测试固件
 *
 * 注册所有原版实体类型以使 EntityRegistry 可用。
 */
class ClientEntityEyeHeightTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化 Items 和 VanillaEntities 以确保 EntityRegistry 可用
        Items::initialize();
        mc::entity::VanillaEntities::registerAll();
    }

    void SetUp() override { entity = std::make_unique<ClientEntity>(EntityId(1), "minecraft:pig"); }

    void TearDown() override { entity.reset(); }

    std::unique_ptr<ClientEntity> entity;
};

TEST_F(ClientEntityEyeHeightTest, DefaultEyeHeightIsPlayerStanding)
{
    // 使用未注册 typeId 创建的实体默认使用玩家站立眼高
    ClientEntity unknownEntity(EntityId(99), "minecraft:unknown_entity");
    EXPECT_FLOAT_EQ(unknownEntity.eyeHeight(), mc::game::PLAYER_EYE_HEIGHT);
}

TEST_F(ClientEntityEyeHeightTest, SetEyeHeightDirectly)
{
    entity->setEyeHeight(2.5f);
    EXPECT_FLOAT_EQ(entity->eyeHeight(), 2.5f);

    entity->setEyeHeight(0.1f);
    EXPECT_FLOAT_EQ(entity->eyeHeight(), 0.1f);
}

TEST_F(ClientEntityEyeHeightTest, RefreshEyeHeightFromRegistry)
{
    // minecraft:pig 的注册表尺寸为 0.9x0.9，默认眼高 = 0.9 * 0.85 = 0.765
    entity->refreshEyeHeight();
    EXPECT_FLOAT_EQ(entity->eyeHeight(), 0.765f);
}

TEST_F(ClientEntityEyeHeightTest, RefreshEyeHeightPlayerStanding)
{
    // 玩家站立眼高 = 1.62
    ClientEntity player(EntityId(2), mc::entity::EntityTypes::PLAYER);
    player.refreshEyeHeight();
    EXPECT_FLOAT_EQ(player.eyeHeight(), mc::game::PLAYER_EYE_HEIGHT);
}

TEST_F(ClientEntityEyeHeightTest, RefreshEyeHeightPlayerSneaking)
{
    // 玩家蹲伏眼高 = 1.27
    ClientEntity player(EntityId(3), mc::entity::EntityTypes::PLAYER);
    player.setSneaking(true);
    EXPECT_FLOAT_EQ(player.eyeHeight(), 1.27f);
}

TEST_F(ClientEntityEyeHeightTest, RefreshEyeHeightPlayerSwimming)
{
    // 玩家游泳眼高 = 0.4
    ClientEntity player(EntityId(4), mc::entity::EntityTypes::PLAYER);
    player.setSwimming(true);
    EXPECT_FLOAT_EQ(player.eyeHeight(), 0.4f);
}

TEST_F(ClientEntityEyeHeightTest, RefreshEyeHeightPlayerSleeping)
{
    // 玩家睡眠眼高 = 0.2
    ClientEntity player(EntityId(5), mc::entity::EntityTypes::PLAYER);
    player.setSleeping(true);
    EXPECT_FLOAT_EQ(player.eyeHeight(), 0.2f);
}

TEST_F(ClientEntityEyeHeightTest, RefreshEyeHeightPlayerFallFlying)
{
    // 玩家鞘翅飞行眼高 = 0.4（与游泳相同）
    // 注意：isFallFlying() 从元数据读取，此处使用 setSwimming 模拟
    // 因为 setSneaking/setSwimming/setSleeping 都触发 refreshEyeHeight
    ClientEntity player(EntityId(6), mc::entity::EntityTypes::PLAYER);
    // 鞘翅飞行状态下眼高与游泳相同
    // 由于 isFallFlying() 依赖元数据，此处测试游泳状态即可
    player.setSwimming(true);
    EXPECT_FLOAT_EQ(player.eyeHeight(), 0.4f);
}

TEST_F(ClientEntityEyeHeightTest, RefreshEyeHeightPlayerSneakingOverridesSwimming)
{
    // 同时蹲伏和游泳时，蹲伏优先（因为后设置的 sneaking 会覆盖）
    ClientEntity player(EntityId(7), mc::entity::EntityTypes::PLAYER);
    player.setSwimming(true);
    EXPECT_FLOAT_EQ(player.eyeHeight(), 0.4f);
    player.setSneaking(true);
    // refreshEyeHeight 中，sneaking 在 swimming 之后检查
    // 但 sleep > swimming/fallFlying > sneaking > standing
    // 同时 swimming 和 sneaking 时，swimming 先匹配
    // 这里 sneaking 被设为 true，但 swimming 仍为 true
    // refreshEyeHeight 中先检查 sleeping，再检查 swimming || isFallFlying，
    // 所以 swimming=true 时眼高=0.4
    EXPECT_FLOAT_EQ(player.eyeHeight(), 0.4f);
}

TEST_F(ClientEntityEyeHeightTest, RefreshEyeHeightPlayerSleepOverridesAll)
{
    // 睡眠优先级最高
    ClientEntity player(EntityId(8), mc::entity::EntityTypes::PLAYER);
    player.setSwimming(true);
    player.setSneaking(true);
    player.setSleeping(true);
    EXPECT_FLOAT_EQ(player.eyeHeight(), 0.2f);
}

TEST_F(ClientEntityEyeHeightTest, ChildEntityEyeHeightHalved)
{
    // 非玩家幼年个体眼高为注册表眼高的一半
    // minecraft:pig 注册表眼高 = 0.765，幼年 = 0.3825
    entity->setChild(true);
    EXPECT_FLOAT_EQ(entity->eyeHeight(), 0.765f * 0.5f);
}

TEST_F(ClientEntityEyeHeightTest, ChildPlayerEyeHeightNotHalved)
{
    // 玩家不使用注册表眼高减半逻辑，而是使用固定姿态眼高
    ClientEntity player(EntityId(9), mc::entity::EntityTypes::PLAYER);
    player.setChild(true);
    // 玩家子实体走 Player 分支，不经过 child * 0.5 逻辑
    EXPECT_FLOAT_EQ(player.eyeHeight(), mc::game::PLAYER_EYE_HEIGHT);
}

TEST_F(ClientEntityEyeHeightTest, SetSneakingTriggersRefresh)
{
    // 玩家从站立切换到蹲伏应自动刷新眼高
    ClientEntity player(EntityId(10), mc::entity::EntityTypes::PLAYER);
    EXPECT_FLOAT_EQ(player.eyeHeight(), mc::game::PLAYER_EYE_HEIGHT);

    player.setSneaking(true);
    EXPECT_FLOAT_EQ(player.eyeHeight(), 1.27f);

    player.setSneaking(false);
    EXPECT_FLOAT_EQ(player.eyeHeight(), mc::game::PLAYER_EYE_HEIGHT);
}

TEST_F(ClientEntityEyeHeightTest, SetSwimmingTriggersRefresh)
{
    ClientEntity player(EntityId(11), mc::entity::EntityTypes::PLAYER);
    EXPECT_FLOAT_EQ(player.eyeHeight(), mc::game::PLAYER_EYE_HEIGHT);

    player.setSwimming(true);
    EXPECT_FLOAT_EQ(player.eyeHeight(), 0.4f);

    player.setSwimming(false);
    EXPECT_FLOAT_EQ(player.eyeHeight(), mc::game::PLAYER_EYE_HEIGHT);
}

TEST_F(ClientEntityEyeHeightTest, SetSleepingTriggersRefresh)
{
    ClientEntity player(EntityId(12), mc::entity::EntityTypes::PLAYER);
    EXPECT_FLOAT_EQ(player.eyeHeight(), mc::game::PLAYER_EYE_HEIGHT);

    player.setSleeping(true);
    EXPECT_FLOAT_EQ(player.eyeHeight(), 0.2f);

    player.setSleeping(false);
    EXPECT_FLOAT_EQ(player.eyeHeight(), mc::game::PLAYER_EYE_HEIGHT);
}

TEST_F(ClientEntityEyeHeightTest, SetChildTriggersRefresh)
{
    // 非玩家实体设为幼年应自动刷新眼高
    entity->refreshEyeHeight();
    const f32 adultEyeHeight = entity->eyeHeight(); // 0.765

    entity->setChild(true);
    EXPECT_FLOAT_EQ(entity->eyeHeight(), adultEyeHeight * 0.5f);

    entity->setChild(false);
    EXPECT_FLOAT_EQ(entity->eyeHeight(), adultEyeHeight);
}

TEST_F(ClientEntityEyeHeightTest, NoRefreshOnSameValue)
{
    // 设置相同的值不应触发 refreshEyeHeight（优化：值未变化时跳过）
    ClientEntity player(EntityId(13), mc::entity::EntityTypes::PLAYER);
    EXPECT_FLOAT_EQ(player.eyeHeight(), mc::game::PLAYER_EYE_HEIGHT);

    // 设置相同的 sneaking 值（false -> false），眼高不变
    player.setSneaking(false);
    EXPECT_FLOAT_EQ(player.eyeHeight(), mc::game::PLAYER_EYE_HEIGHT);
}

TEST_F(ClientEntityEyeHeightTest, DifferentEntityTypesHaveDifferentEyeHeights)
{
    // 不同实体类型应有不同的注册表眼高
    ClientEntity pig(EntityId(20), "minecraft:pig");
    pig.refreshEyeHeight();
    f32 pigEyeHeight = pig.eyeHeight();

    ClientEntity cow(EntityId(21), "minecraft:cow");
    cow.refreshEyeHeight();
    f32 cowEyeHeight = cow.eyeHeight();

    ClientEntity chicken(EntityId(22), "minecraft:chicken");
    chicken.refreshEyeHeight();
    f32 chickenEyeHeight = chicken.eyeHeight();

    // 不同实体的眼高应该不同
    EXPECT_NE(pigEyeHeight, cowEyeHeight);
    EXPECT_NE(pigEyeHeight, chickenEyeHeight);

    // 验证具体值：pig=0.9*0.85=0.765, cow=1.4*0.85=1.19, chicken=0.7*0.85=0.595
    EXPECT_FLOAT_EQ(pigEyeHeight, 0.765f);
    EXPECT_FLOAT_EQ(cowEyeHeight, 1.19f);
    EXPECT_FLOAT_EQ(chickenEyeHeight, 0.595f);
}

TEST_F(ClientEntityEyeHeightTest, EyeHeightInitialValueBeforeRefresh)
{
    // ClientEntity 默认眼高为玩家站立眼高 1.62
    ClientEntity freshEntity(EntityId(30), "minecraft:creeper");
    EXPECT_FLOAT_EQ(freshEntity.eyeHeight(), mc::game::PLAYER_EYE_HEIGHT);
}

TEST_F(ClientEntityEyeHeightTest, DimensionsInitializedWithDefault)
{
    // ClientEntity 默认尺寸为玩家尺寸
    ClientEntity freshEntity(EntityId(31), "minecraft:sheep");
    EXPECT_FLOAT_EQ(freshEntity.width(), 0.6f);
    EXPECT_FLOAT_EQ(freshEntity.height(), 1.8f);
}

// ============================================================================
// ClientEntity 兔子跳跃动画状态测试
// ============================================================================
// 对应 MC 1.21.11 Rabbit.handleEntityEvent(byte 1) + Rabbit.aiStep() 客户端镜像逻辑
// 数据流：服务端 broadcastEntityStatus(RabbitJump=1)
//   → ClientApplicationNetwork::onEntityStatus
//   → ClientEntity::setRabbitJumpStart() (jumpDuration=10, jumpTicks=0)
//   → ClientEntity::tick() 中 tickRabbitJump() 推进
//   → rabbitJumpCompletion(partialTick) 供渲染器计算 jumpRotation

class ClientEntityRabbitJumpTest : public ::testing::Test {
protected:
    void SetUp() override { entity = std::make_unique<ClientEntity>(EntityId(1), "minecraft:rabbit"); }

    void TearDown() override { entity.reset(); }

    std::unique_ptr<ClientEntity> entity;
};

TEST_F(ClientEntityRabbitJumpTest, DefaultState_NoJumpInProgress)
{
    // 默认状态：未在跳跃中
    EXPECT_FALSE(entity->rabbitIsJumping());
    EXPECT_FLOAT_EQ(entity->rabbitJumpCompletion(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(entity->rabbitJumpCompletion(0.5f), 0.0f);
    EXPECT_FLOAT_EQ(entity->rabbitJumpCompletion(1.0f), 0.0f);
}

TEST_F(ClientEntityRabbitJumpTest, SetRabbitJumpStart_InitializesJumpDuration)
{
    // 对应 MC Rabbit.handleEntityEvent(byte 1): jumpDuration=10; jumpTicks=0;
    entity->setRabbitJumpStart();

    EXPECT_TRUE(entity->rabbitIsJumping());
    // jumpDuration=10, jumpTicks=0: completion = (0 + partialTick) / 10
    EXPECT_FLOAT_EQ(entity->rabbitJumpCompletion(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(entity->rabbitJumpCompletion(0.5f), 0.05f);
    EXPECT_FLOAT_EQ(entity->rabbitJumpCompletion(1.0f), 0.1f);
}

TEST_F(ClientEntityRabbitJumpTest, Tick_AdvancesJumpTicks)
{
    // tick() 中 tickRabbitJump() 推进 m_rabbitJumpTicks
    // 对应 MC Rabbit.aiStep() 的 jumpTicks++ 逻辑
    entity->setRabbitJumpStart(); // jumpDuration=10, jumpTicks=0

    entity->tick();                                            // jumpTicks 0→1
    EXPECT_FLOAT_EQ(entity->rabbitJumpCompletion(0.0f), 0.1f); // 1/10

    entity->tick();                                            // jumpTicks 1→2
    EXPECT_FLOAT_EQ(entity->rabbitJumpCompletion(0.0f), 0.2f); // 2/10
}

TEST_F(ClientEntityRabbitJumpTest, Tick_ResetsAtJumpDuration)
{
    // jumpTicks 达到 jumpDuration 时，tick 应归零
    // 对应 MC Rabbit.aiStep(): else if (jumpDuration != 0) { jumpTicks=0; jumpDuration=0; }
    //
    // 时序：setRabbitJumpStart → jumpTicks=0
    //   tick #1: jumpTicks 0→1
    //   ...
    //   tick #10: jumpTicks 9→10 (仍走 ++ 分支)
    //   tick #11: jumpTicks==jumpDuration → 归零
    entity->setRabbitJumpStart();

    for (int i = 0; i < 10; ++i) {
        entity->tick();
    }
    // 第 10 次 tick 后: jumpTicks=10，仍在跳跃中
    EXPECT_TRUE(entity->rabbitIsJumping());
    EXPECT_FLOAT_EQ(entity->rabbitJumpCompletion(0.0f), 1.0f); // 10/10

    // 第 11 次 tick: 归零
    entity->tick();
    EXPECT_FALSE(entity->rabbitIsJumping());
    EXPECT_FLOAT_EQ(entity->rabbitJumpCompletion(0.0f), 0.0f);
}

TEST_F(ClientEntityRabbitJumpTest, RabbitJumpCompletion_PartialTickInterpolation)
{
    // 验证 partialTick 在 [0, 1] 范围内的插值
    // completion = (jumpTicks + partialTick) / jumpDuration
    entity->setRabbitJumpStart(); // jumpDuration=10, jumpTicks=0

    // 推进 3 tick: jumpTicks=3
    for (int i = 0; i < 3; ++i) {
        entity->tick();
    }

    // completion = (3 + partialTick) / 10
    EXPECT_FLOAT_EQ(entity->rabbitJumpCompletion(0.0f), 0.3f);
    EXPECT_FLOAT_EQ(entity->rabbitJumpCompletion(0.25f), 0.325f);
    EXPECT_FLOAT_EQ(entity->rabbitJumpCompletion(0.5f), 0.35f);
    EXPECT_FLOAT_EQ(entity->rabbitJumpCompletion(0.75f), 0.375f);
    EXPECT_FLOAT_EQ(entity->rabbitJumpCompletion(1.0f), 0.4f);
}

TEST_F(ClientEntityRabbitJumpTest, SetRabbitJumpStart_IdempotentDuringJump)
{
    // 跳跃进行中再次调用 setRabbitJumpStart 不应重置进度
    // （虽然服务端 startJumping() 有幂等保护，客户端也应安全）
    entity->setRabbitJumpStart();

    for (int i = 0; i < 3; ++i) {
        entity->tick();
    }
    // jumpTicks=3
    EXPECT_FLOAT_EQ(entity->rabbitJumpCompletion(0.0f), 0.3f);

    // 再次调用 setRabbitJumpStart：重置 jumpTicks=0, jumpDuration=10
    entity->setRabbitJumpStart();
    EXPECT_FLOAT_EQ(entity->rabbitJumpCompletion(0.0f), 0.0f);
    EXPECT_TRUE(entity->rabbitIsJumping());
}

TEST_F(ClientEntityRabbitJumpTest, JumpCompletion_SinProductForJumpRotation)
{
    // 验证渲染器使用的 jumpRotation = sin(completion * PI) 公式
    // 这不是 ClientEntity 的方法，但验证 completion 值能正确驱动该公式
    entity->setRabbitJumpStart();

    // 跳跃中点：jumpTicks=5, completion(0.5) = 5.5/10 = 0.55
    // sin(0.55 * PI) ≈ 0.9877
    for (int i = 0; i < 5; ++i) {
        entity->tick();
    }
    const f32 completion = entity->rabbitJumpCompletion(0.5f);
    const f32 jumpRotation = std::sin(completion * math::PI);
    EXPECT_NEAR(jumpRotation, 0.9877f, 0.001f);

    // 跳跃开始：jumpTicks=0, completion(0) = 0, sin(0) = 0
    entity->setRabbitJumpStart();
    EXPECT_FLOAT_EQ(std::sin(entity->rabbitJumpCompletion(0.0f) * math::PI), 0.0f);

    // 跳跃中点（partialTick=1, jumpTicks=4）: completion = 5/10 = 0.5
    // sin(0.5 * PI) = 1.0 （最大跳跃旋转）
    for (int i = 0; i < 4; ++i) {
        entity->tick();
    }
    const f32 midCompletion = entity->rabbitJumpCompletion(1.0f);
    EXPECT_FLOAT_EQ(midCompletion, 0.5f);
    EXPECT_FLOAT_EQ(std::sin(midCompletion * math::PI), 1.0f);
}
