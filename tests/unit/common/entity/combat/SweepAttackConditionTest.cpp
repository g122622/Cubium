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

// 横扫攻击"几乎静止"判定（Player::isSweepStationary）测试。
//
// 判定条件（vanilla Player.isSweepAttack 的静止项）：已知水平位移² < (移动速度属性 × 2.5)²，
// 玩家移动速度为 0.1 时阈值 = 0.25²  = 0.0625。
// 运动量来源分两条路径：
//   - 客户端驱动的服务端玩家：取客户端上报的每 tick 位移（ServerPlayer 覆写 getKnownMovement，
//     由 client_tick_end 收口；本 tick 无上报则清零）——服务端自身速度不参与判定；
//   - 其余实体（无客户端连接，如 SimulatedPlayer）：回退到自身速度（每 tick 位移，量纲一致）。
//
// 注意：本文件替代了此前的 1.16.5 版 distanceWalkedModified < aiMoveSpeed 条件测试——该条件
// 已不是项目的实现（其量纲不匹配，且在服务端玩家位置上根本取不到客户端行走）。

#include <gtest/gtest.h>

#include <memory>

#include "common/TestWorldHelper.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/item/Items.hpp"
#include "common/item/enchantment/EnchantmentRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "entity/entities/player/Player.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/test/simulated/SimulatedPlayer.hpp"
#include "world/fluid/FluidRegistry.hpp"

using namespace mc;

namespace {

/// 默认移动速度 0.1 对应的横扫阈值：(0.1 × 2.5)² = 0.0625。
/// 取 0.2 / 0.3 这类远离阈值的位移做断言，避免 f32/f64 在边界上的精度抖动。
constexpr f32 kBelowThresholdDisplacement = 0.2f; // 0.04 < 0.0625 → 几乎静止
constexpr f32 kAboveThresholdDisplacement = 0.3f; // 0.09 > 0.0625 → 不算静止

} // namespace

class SweepAttackConditionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        fluid::FluidRegistry::instance().initialize();
        Items::initialize();
        item::enchant::EnchantmentRegistry::initialize();
    }

    void TearDown() override { item::enchant::EnchantmentRegistry::clear(); }
};

// ============================================================================
// 基类 Player：运动量回退到实体速度
// ============================================================================

TEST_F(SweepAttackConditionTest, BasePlayer_InitialVelocityIsZero_IsStationary)
{
    Player player(EntityInstanceId(1), "TestPlayer", mc::test::testEcsRegistry());

    EXPECT_EQ(player.getKnownMovement(), Vector3(0.0f, 0.0f, 0.0f));
    EXPECT_TRUE(player.isSweepStationary());
}

TEST_F(SweepAttackConditionTest, BasePlayer_MovingHorizontally_IsNotStationary)
{
    Player player(EntityInstanceId(2), "TestPlayer", mc::test::testEcsRegistry());

    player.setVelocity(kAboveThresholdDisplacement, 0.0f, 0.0f);
    EXPECT_FALSE(player.isSweepStationary()) << "X 方向位移超过阈值不应判为几乎静止";

    player.setVelocity(0.0f, 0.0f, kAboveThresholdDisplacement);
    EXPECT_FALSE(player.isSweepStationary()) << "Z 方向位移同样计入水平位移";
}

TEST_F(SweepAttackConditionTest, BasePlayer_SlowHorizontalMovement_IsStationary)
{
    Player player(EntityInstanceId(3), "TestPlayer", mc::test::testEcsRegistry());

    player.setVelocity(kBelowThresholdDisplacement, 0.0f, kBelowThresholdDisplacement);
    // 0.2² + 0.2² = 0.08 > 0.0625：两轴叠加后超阈值，故分别构造单轴与对角两种场景。
    EXPECT_FALSE(player.isSweepStationary()) << "两轴叠加位移超阈值时不应判为静止";

    player.setVelocity(kBelowThresholdDisplacement, 0.0f, 0.0f);
    EXPECT_TRUE(player.isSweepStationary()) << "单轴慢速位移仍在阈值内";
}

TEST_F(SweepAttackConditionTest, BasePlayer_VerticalVelocity_IsIgnored)
{
    Player player(EntityInstanceId(4), "TestPlayer", mc::test::testEcsRegistry());

    // 判定只看水平位移：纯垂直速度（自由落体/跳跃）不影响横扫静止判定。
    player.setVelocity(0.0f, 1.0f, 0.0f);
    EXPECT_TRUE(player.isSweepStationary());
}

// ============================================================================
// ServerPlayer：运动量取客户端上报的每 tick 位移
// ============================================================================

TEST_F(SweepAttackConditionTest, ServerPlayer_NoClientReport_IsStationary)
{
    ServerPlayer player(EntityInstanceId(5), "ServerPlayer", mc::test::testEcsRegistry());

    EXPECT_EQ(player.getKnownMovement(), Vector3(0.0f, 0.0f, 0.0f));
    EXPECT_TRUE(player.isSweepStationary());
}

TEST_F(SweepAttackConditionTest, ServerPlayer_ClientReportedMovement_IsNotStationary)
{
    ServerPlayer player(EntityInstanceId(6), "ServerPlayer", mc::test::testEcsRegistry());

    player.recordClientMovement(Vector3(kAboveThresholdDisplacement, 0.0f, 0.0f));
    EXPECT_EQ(player.getKnownMovement(), Vector3(kAboveThresholdDisplacement, 0.0f, 0.0f));
    EXPECT_FALSE(player.isSweepStationary());
}

TEST_F(SweepAttackConditionTest, ServerPlayer_OwnVelocityIsNotUsedForSweep)
{
    ServerPlayer player(EntityInstanceId(7), "ServerPlayer", mc::test::testEcsRegistry());

    // 服务端玩家的位置由客户端权威申报，服务端自身速度（击退/物理残留）不代表其在行走：
    // 速度很大但客户端未上报位移时，仍应判为几乎静止。
    player.setVelocity(kAboveThresholdDisplacement, 0.0f, 0.0f);
    EXPECT_TRUE(player.isSweepStationary()) << "ServerPlayer 的判定必须用客户端上报运动量而非自身速度";

    // 反之，客户端上报了位移时，即便自身速度为零也不算静止。
    player.setVelocity(0.0f, 0.0f, 0.0f);
    player.recordClientMovement(Vector3(kAboveThresholdDisplacement, 0.0f, 0.0f));
    EXPECT_FALSE(player.isSweepStationary());
}

TEST_F(SweepAttackConditionTest, ServerPlayer_ReportedMovementSurvivesClientTickEnd)
{
    ServerPlayer player(EntityInstanceId(8), "ServerPlayer", mc::test::testEcsRegistry());

    player.recordClientMovement(Vector3(kAboveThresholdDisplacement, 0.0f, 0.0f));
    player.endClientTick(); // 本 tick 有上报 → 收口时不清零

    EXPECT_EQ(player.getKnownMovement(), Vector3(kAboveThresholdDisplacement, 0.0f, 0.0f));
    EXPECT_FALSE(player.isSweepStationary());
}

TEST_F(SweepAttackConditionTest, ServerPlayer_IdleClientTickZeroesMovement)
{
    ServerPlayer player(EntityInstanceId(9), "ServerPlayer", mc::test::testEcsRegistry());

    player.recordClientMovement(Vector3(kAboveThresholdDisplacement, 0.0f, 0.0f));
    player.endClientTick();

    // 下一个客户端 tick 没有收到任何移动上报：玩家静止，运动量清零。
    player.endClientTick();
    EXPECT_EQ(player.getKnownMovement(), Vector3(0.0f, 0.0f, 0.0f));
    EXPECT_TRUE(player.isSweepStationary());
}

// ============================================================================
// SimulatedPlayer：无客户端连接，运动量取自身速度
// ============================================================================

TEST_F(SweepAttackConditionTest, SimulatedPlayer_UsesOwnVelocity)
{
    mc::test::SimulatedPlayer bot(EntityInstanceId(10), "SimulatedPlayer", mc::test::testEcsRegistry());

    EXPECT_TRUE(bot.isSweepStationary());

    bot.setVelocity(kAboveThresholdDisplacement, 0.0f, 0.0f);
    EXPECT_FALSE(bot.isSweepStationary()) << "模拟玩家在移动时不应被判为几乎静止";

    // 模拟玩家不参与客户端 tick 收口：收口不会改变其判定依据。
    bot.endClientTick();
    EXPECT_FALSE(bot.isSweepStationary());
}

// ============================================================================
// 虚分派：经 Player* 调用取到的是派生类实现
// ============================================================================

TEST_F(SweepAttackConditionTest, VirtualDispatch_UsesDerivedKnownMovement)
{
    ServerPlayer serverPlayer(EntityInstanceId(11), "ServerPlayer", mc::test::testEcsRegistry());
    mc::test::SimulatedPlayer bot(EntityInstanceId(12), "SimulatedPlayer", mc::test::testEcsRegistry());
    Player base(EntityInstanceId(13), "BasePlayer", mc::test::testEcsRegistry());

    base.setVelocity(kAboveThresholdDisplacement, 0.0f, 0.0f);

    serverPlayer.recordClientMovement(Vector3(kAboveThresholdDisplacement, 0.0f, 0.0f));

    Player* asBase = &serverPlayer;
    EXPECT_EQ(asBase->getKnownMovement(), Vector3(kAboveThresholdDisplacement, 0.0f, 0.0f));
    EXPECT_FALSE(asBase->isSweepStationary());

    Player* botAsBase = &bot;
    EXPECT_EQ(botAsBase->getKnownMovement(), bot.velocity());

    Player* plainAsBase = &base;
    EXPECT_EQ(plainAsBase->getKnownMovement(), base.velocity());
    EXPECT_FALSE(plainAsBase->isSweepStationary());
}
