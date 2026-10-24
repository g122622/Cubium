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
 * @file CamelEntityTest.cpp
 * @brief 骆驼实体测试
 *
 * 覆盖 vanilla Camel 的关键语义：
 * 1. 默认状态：站立、未冲刺、恒为已驯服
 * 2. 坐下/站起：状态位（LAST_POSE_CHANGE_TICK 负值）与姿态切换
 * 3. 冲刺：冷却 55、首次冲刺重置冷却
 * 4. NBT：LastPoseTick 往返（负值恢复坐姿）
 * 5. 食物：仙人掌
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EntityPose.hpp"
#include "common/entity/entities/passive/special/CamelEntity.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <memory>

namespace mc {
namespace {

/**
 * @brief 可推进时间的世界桩
 *
 * BaseTestWorld::currentTick() 恒为 0，而骆驼坐姿/站起判定依赖游戏时间（getPoseTime），
 * 故这里提供可设置的 tick 以便验证姿态过渡窗口。
 */
class CamelTestWorld final : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] u64 currentTick() const override
    {
        return m_tick;
    }

    void setTick(u64 tick)
    {
        m_tick = tick;
    }

    void advance(u64 ticks)
    {
        m_tick += ticks;
    }

private:
    u64 m_tick = 0;
};

class CamelEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::ensureInitialized();
    }

    CamelTestWorld m_world;
};

TEST_F(CamelEntityTest, DefaultsToStandingAndTame)
{
    CamelEntity camel(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(camel.isCamelSitting());
    EXPECT_FALSE(camel.isDashing());
    EXPECT_EQ(camel.dashCooldown(), 0);
    EXPECT_TRUE(camel.isTame());
    EXPECT_EQ(camel.lastPoseChangeTick(), 0);
}

TEST_F(CamelEntityTest, SitDownAndStandUp_TogglePose)
{
    CamelEntity camel(EntityInstanceId(1), mc::test::testEcsRegistry());
    camel.setWorld(&m_world);

    camel.sitDown();
    EXPECT_TRUE(camel.isCamelSitting());
    EXPECT_LT(camel.lastPoseChangeTick(), 0);
    EXPECT_EQ(camel.pose(), EntityPose::Sitting);
    EXPECT_TRUE(camel.refuseToMove());

    camel.standUp();
    EXPECT_FALSE(camel.isCamelSitting());
    EXPECT_GE(camel.lastPoseChangeTick(), 0);
    EXPECT_EQ(camel.pose(), EntityPose::Standing);
}

TEST_F(CamelEntityTest, PoseTime_AdvancesWithGameTime)
{
    m_world.setTick(100);
    CamelEntity camel(EntityInstanceId(1), mc::test::testEcsRegistry());
    camel.setWorld(&m_world);
    camel.sitDown();
    EXPECT_EQ(camel.getPoseTime(), 0);
    EXPECT_TRUE(camel.isInPoseTransition());

    // 坐姿动画 40 tick：过了就该结束过渡（但仍在 sitting 状态）
    m_world.advance(40);
    EXPECT_FALSE(camel.isInPoseTransition());
    EXPECT_TRUE(camel.isCamelSitting());
}

TEST_F(CamelEntityTest, SitDown_WhenAlreadySitting_IsNoop)
{
    CamelEntity camel(EntityInstanceId(1), mc::test::testEcsRegistry());
    camel.setWorld(&m_world);
    camel.resetLastPoseChangeTick(-500);

    camel.sitDown();
    // 时间戳不被覆盖（仍为原负值）
    EXPECT_EQ(camel.lastPoseChangeTick(), -500);
}

TEST_F(CamelEntityTest, StandUp_EndsSitting)
{
    m_world.setTick(1000);
    CamelEntity camel(EntityInstanceId(1), mc::test::testEcsRegistry());
    camel.setWorld(&m_world);
    camel.sitDown();
    EXPECT_TRUE(camel.isInPoseTransition());

    camel.standUp();
    EXPECT_FALSE(camel.isCamelSitting());
    EXPECT_EQ(camel.pose(), EntityPose::Standing);
    // 站起动画 52 tick 内仍处于过渡（拒绝移动），越过后恢复
    EXPECT_TRUE(camel.isInPoseTransition());
    m_world.advance(CamelEntity::STANDUP_DURATION_TICKS);
    EXPECT_FALSE(camel.isInPoseTransition());
    EXPECT_FALSE(camel.refuseToMove());
}

TEST_F(CamelEntityTest, Dash_SetsCooldownAndFlag)
{
    CamelEntity camel(EntityInstanceId(1), mc::test::testEcsRegistry());
    camel.setWorld(&m_world);

    camel.setDashing(true);
    EXPECT_TRUE(camel.isDashing());
    EXPECT_EQ(camel.dashCooldown(), CamelEntity::DASH_COOLDOWN_TICKS);

    camel.setDashing(false);
    EXPECT_FALSE(camel.isDashing());
}

TEST_F(CamelEntityTest, Food_IsCactus)
{
    CamelEntity camel(EntityInstanceId(1), mc::test::testEcsRegistry());
    ASSERT_NE(Items::CACTUS, nullptr);
    EXPECT_TRUE(camel.isFoodItem(ItemStack(*Items::CACTUS, 1)));
    ASSERT_NE(Items::WHEAT, nullptr);
    EXPECT_FALSE(camel.isFoodItem(ItemStack(*Items::WHEAT, 1)));
}

TEST_F(CamelEntityTest, NbtRoundTrip_PreservesPoseTick)
{
    m_world.setTick(1000);
    CamelEntity camel(EntityInstanceId(1), mc::test::testEcsRegistry());
    camel.setWorld(&m_world);
    camel.sitDown();

    nbt::tags::compound_tag tag;
    camel.addAdditionalSaveData(tag);

    CamelEntity restored(EntityInstanceId(2), mc::test::testEcsRegistry());
    ASSERT_TRUE(restored.readAdditionalSaveData(tag).success());
    EXPECT_TRUE(restored.isCamelSitting());
    EXPECT_EQ(restored.lastPoseChangeTick(), camel.lastPoseChangeTick());
    EXPECT_EQ(restored.pose(), EntityPose::Sitting);
}

TEST_F(CamelEntityTest, AgeScale_BabyIsFortyFivePercent)
{
    CamelEntity camel(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(camel.getAgeScaleValue(), 1.0f);
    camel.setChild(true);
    EXPECT_FLOAT_EQ(camel.getAgeScaleValue(), CamelEntity::BABY_SCALE);
}

} // namespace
} // namespace mc