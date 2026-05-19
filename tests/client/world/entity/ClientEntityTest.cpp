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
#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/util/math/Vector3.hpp"

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
