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
 * @file ServerPlayerClientMovementTest.cpp
 * @brief 客户端 tick 运动量记账（knownMovement / receivedMovementThisTick）单元测试
 *
 * 覆盖 ServerPlayer 的跨 tick 运动量状态机：
 * - recordClientMovement：移动包被采纳后记账（最近一次覆盖前一次），并置位"本 tick 已上报"
 * - endClientTick（client_tick_end 收口）：本 tick 无上报则清零运动量，随后复位标记
 * - getKnownMovement：经 Player* 基类指针取到的是客户端上报值（虚分派）
 *
 * 上游调用点：MovementHandler::handlePlayerMovePacket / handleMoveVehiclePacket（记账）与
 * handleClientTickEndPacket（收口）。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/util/math/Vector3.hpp"
#include "entity/entities/player/Player.hpp"
#include "server/player/ServerPlayer.hpp"

using namespace mc;

namespace {

[[nodiscard]] Vector3 vec(f32 x, f32 y, f32 z)
{
    return Vector3(x, y, z);
}

} // namespace

TEST(ServerPlayerClientMovementTest, InitialKnownMovementIsZero)
{
    ServerPlayer player(EntityInstanceId(1), "TestServerPlayer", mc::test::testEcsRegistry());

    EXPECT_EQ(player.getKnownMovement(), vec(0.0f, 0.0f, 0.0f));
}

TEST(ServerPlayerClientMovementTest, RecordClientMovementStoresDelta)
{
    ServerPlayer player(EntityInstanceId(2), "TestServerPlayer", mc::test::testEcsRegistry());

    player.recordClientMovement(vec(0.3f, -0.1f, 0.4f));

    EXPECT_EQ(player.getKnownMovement(), vec(0.3f, -0.1f, 0.4f));
}

TEST(ServerPlayerClientMovementTest, LastRecordInSameTickWins)
{
    // 一个客户端 tick 内可到达多个移动包，vanilla 以最后一次的位移为已知运动量。
    ServerPlayer player(EntityInstanceId(3), "TestServerPlayer", mc::test::testEcsRegistry());

    player.recordClientMovement(vec(0.3f, 0.0f, 0.0f));
    player.recordClientMovement(vec(0.0f, 0.2f, 0.0f));
    player.recordClientMovement(vec(0.0f, 0.0f, 0.5f));

    EXPECT_EQ(player.getKnownMovement(), vec(0.0f, 0.0f, 0.5f));
}

TEST(ServerPlayerClientMovementTest, EndClientTickWithoutReportZeroesMovement)
{
    ServerPlayer player(EntityInstanceId(4), "TestServerPlayer", mc::test::testEcsRegistry());

    player.recordClientMovement(vec(0.5f, 0.0f, 0.0f));
    player.endClientTick(); // 本 tick 有上报：保留

    player.endClientTick(); // 本 tick 无上报：玩家静止，清零
    EXPECT_EQ(player.getKnownMovement(), vec(0.0f, 0.0f, 0.0f));
}

TEST(ServerPlayerClientMovementTest, ReportFlagResetsAfterEachEndClientTick)
{
    // 收口后标记必须复位，否则下一 tick 的静止状态会误判为"移动中"。
    ServerPlayer player(EntityInstanceId(5), "TestServerPlayer", mc::test::testEcsRegistry());

    player.recordClientMovement(vec(0.5f, 0.0f, 0.0f));
    player.endClientTick();
    EXPECT_EQ(player.getKnownMovement(), vec(0.5f, 0.0f, 0.0f)) << "有上报的 tick 不应清零";

    player.endClientTick();
    EXPECT_EQ(player.getKnownMovement(), vec(0.0f, 0.0f, 0.0f)) << "无上报的 tick 应清零";

    // 清零后再次上报，仍能正常记账（状态机可重复使用）。
    player.recordClientMovement(vec(0.0f, 0.0f, -0.25f));
    player.endClientTick();
    EXPECT_EQ(player.getKnownMovement(), vec(0.0f, 0.0f, -0.25f));
}

TEST(ServerPlayerClientMovementTest, ZeroLengthReportCountsAsReceived)
{
    // 纯朝向/着地包位移为零，但确实是一次客户端上报：收口不应把它当成"没收到"。
    ServerPlayer player(EntityInstanceId(6), "TestServerPlayer", mc::test::testEcsRegistry());

    player.recordClientMovement(vec(0.4f, 0.0f, 0.0f));
    player.endClientTick();

    player.recordClientMovement(vec(0.0f, 0.0f, 0.0f));
    player.endClientTick();
    EXPECT_EQ(player.getKnownMovement(), vec(0.0f, 0.0f, 0.0f));
}

TEST(ServerPlayerClientMovementTest, BasePointerUsesClientReportedMovement)
{
    ServerPlayer player(EntityInstanceId(7), "TestServerPlayer", mc::test::testEcsRegistry());
    player.setVelocity(vec(0.9f, 0.0f, 0.0f)); // 服务端自身速度不应参与

    player.recordClientMovement(vec(0.2f, 0.0f, 0.1f));

    Player* base = &player;
    EXPECT_EQ(base->getKnownMovement(), vec(0.2f, 0.0f, 0.1f));
}
