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

// MobEntity::updateBodyRotation 对齐 vanilla BodyRotationControl.clientTick 测试。
//
// vanilla（BodyRotationControl.java:21-58）：
//   - 移动中：yBodyRot = yRot；yHeadRot 用 rotateIfNecessary 限制在身体最大头转范围内；
//     记录 lastStableYHeadRot = yHeadRot、headStableTime = 0。
//   - 静止（且首个乘客不是 Mob）：
//       |yHeadRot - lastStableYHeadRot| > 15 → headStableTime = 0、更新稳定基准、
//           yBodyRot 向 yHeadRot 旋转（限制 maxHeadYRot）；
//       否则 headStableTime++；超过 10 后 yBodyRot 按 (stableTime-10)/10 收缩的
//           头转上限逐步回正。
// 此前项目只实现移动分支，静止时身体朝向恒不更新。

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <memory>

using namespace mc;

namespace {

class BodyRotationTestWorld final : public mc::test::BaseTestWorld {
public:
    BodyRotationTestWorld() = default;
};

} // namespace

class MobBodyRotationTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }

    BodyRotationTestWorld m_world;
};

// 移动中：身体对齐朝向，头部被限制在最大头转范围内。
TEST_F(MobBodyRotationTest, MovingSnapsBodyToYawAndClampsHead)
{
    PigEntity pig(EntityInstanceId(1), mc::test::testEcsRegistry());
    pig.setWorld(&m_world);
    pig.setRotation(90.0f, 0.0f);
    pig.setRotationYawHead(300.0f); // 与身体相差 150°，远超默认 75° 头转上限

    pig.setVelocity(1.0f, 0.0f, 0.0f); // 水平速度平方 = 1 > 2.5e-7
    pig.updateBodyRotation();

    EXPECT_FLOAT_EQ(pig.renderYawOffset(), 90.0f);
    // rotateIfNecessary(head=300, body=90, max=75)：差值 wrap(90-300)=wrap(-210)=150，
    // 钳到 75 → 90-75=15。即头部只能落后身体最多 75°。
    EXPECT_FLOAT_EQ(pig.rotationYawHead(), 15.0f);
    EXPECT_EQ(pig.headStableTime(), 0);
    EXPECT_FLOAT_EQ(pig.lastStableYHeadRot(), 15.0f);
}

// 静止且头偏转超过 15°：身体追击头部（限制在最大头转范围）。
TEST_F(MobBodyRotationTest, StationaryLargeHeadOffsetRotatesBodyTowardHead)
{
    PigEntity pig(EntityInstanceId(2), mc::test::testEcsRegistry());
    pig.setWorld(&m_world);
    pig.setRenderYawOffset(165.0f);
    pig.setRotationYawHead(90.0f);
    pig.setHeadStableState(5, 30.0f); // |90-30|=60 > 15

    pig.updateBodyRotation();

    // rotateIfNecessary(body=165, head=90, max=75)：差值 wrap(90-165)=-75 → 钳到 -75 → 90+75=165。
    EXPECT_FLOAT_EQ(pig.renderYawOffset(), 165.0f);
    EXPECT_EQ(pig.headStableTime(), 0) << "大幅偏转应重置稳定计时";
    EXPECT_FLOAT_EQ(pig.lastStableYHeadRot(), 90.0f);
}

// 静止且头稳定：10 tick 内保持，超过后身体开始逐步回正。
TEST_F(MobBodyRotationTest, StationaryStableHeadSlowlyRealignsBody)
{
    PigEntity pig(EntityInstanceId(3), mc::test::testEcsRegistry());
    pig.setWorld(&m_world);
    pig.setRenderYawOffset(0.0f);
    pig.setRotationYawHead(10.0f);
    pig.setHeadStableState(10, 10.0f); // 头未偏转，稳定 10 tick

    pig.updateBodyRotation();
    EXPECT_EQ(pig.headStableTime(), 11);
    // progress = (11-10)/10 = 0.1 → maxOffset = 75 * 0.9 = 67.5；
    // rotateIfNecessary(body=0, head=10, 67.5)：差值 wrap(10-0)=10 未超限 → 结果 = 10-10 = 0。
    EXPECT_FLOAT_EQ(pig.renderYawOffset(), 0.0f);

    // 头偏转 97.5°、稳定时间推进到 15：progress = 0.5 → maxOffset = 37.5，
    // 差值 wrap(120-22.5) = 97.5 被钳到 37.5 → 结果 = 120 - 37.5 = 82.5，
    // 身体仍落后头部 37.5°，说明在"逐步"回正而非一步对齐。
    PigEntity pig2(EntityInstanceId(4), mc::test::testEcsRegistry());
    pig2.setWorld(&m_world);
    pig2.setRenderYawOffset(22.5f);
    pig2.setRotationYawHead(120.0f);
    pig2.setHeadStableState(14, 120.0f);
    pig2.updateBodyRotation();
    EXPECT_FLOAT_EQ(pig2.renderYawOffset(), 82.5f);
    EXPECT_EQ(pig2.headStableTime(), 15);
}

// 静止但背着 Mob 乘客时不转身体（vanilla notCarryingMobPassengers）。
TEST_F(MobBodyRotationTest, StationaryWithMobPassengerDoesNotRotateBody)
{
    PigEntity vehicle(EntityInstanceId(5), mc::test::testEcsRegistry());
    vehicle.setWorld(&m_world);
    vehicle.setPosition(0.0f, 64.0f, 0.0f);
    vehicle.setRenderYawOffset(165.0f);
    vehicle.setRotationYawHead(90.0f);
    vehicle.setHeadStableState(0, 0.0f);

    auto passenger = std::make_unique<PigEntity>(EntityInstanceId(6), mc::test::testEcsRegistry());
    passenger->setWorld(&m_world);
    passenger->setPosition(0.0f, 64.0f, 0.0f);
    m_world.registerEntityForLookup(&vehicle);
    m_world.registerEntityForLookup(passenger.get());
    ASSERT_TRUE(passenger->startRiding(vehicle));

    vehicle.updateBodyRotation();

    EXPECT_FLOAT_EQ(vehicle.renderYawOffset(), 165.0f) << "背着 Mob 乘客时身体不应转向";

    passenger->stopRiding();
    vehicle.updateBodyRotation();
    // 乘客下车后恢复追击头部：rotateIfNecessary(body=165, head=90, max=75) = 165。
    EXPECT_FLOAT_EQ(vehicle.renderYawOffset(), 165.0f) << "乘客下车后恢复追击头部";
}