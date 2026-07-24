/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
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

#include "server/core/KeepAliveManager.hpp"
#include "common/util/UuidUtils.hpp"
#include "server/core/PlayerManager.hpp"
#include <gtest/gtest.h>

using namespace mc::server::core;

/**
 * @brief KeepAliveManager 单元测试
 *
 * 新网络层 addPlayer 第4参为 mc::server::net::ServerClientConnection*（裸指针）。
 * 本测试只验证 KeepAliveManager 的心跳时序/超时/ping 数据维护，不依赖连接真发包，
 * 故统一传 nullptr。原 KeepAlivePacketHandler.HandleFullPacket 用例依赖旧
 * KeepAlivePacket 字节序列化 + PacketHandler::handleKeepAlive 字节路径，新网络层
 * 该路径已由 ServerPlayRouter 的 KeepAlive 分支覆盖（PacketHandler 已删除），
 * 故移除该集成用例（由 ServerPlayRouter 集成测试替代）。
 */
class KeepAliveManagerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_playerManager = std::make_unique<PlayerManager>();
        m_keepAliveManager = std::make_unique<KeepAliveManager>(*m_playerManager, 1000, 5000);
    }

    void TearDown() override
    {
        m_keepAliveManager.reset();
        m_playerManager.reset();
    }

    std::unique_ptr<PlayerManager> m_playerManager;
    std::unique_ptr<KeepAliveManager> m_keepAliveManager;
};

TEST_F(KeepAliveManagerTest, NeedsKeepAlive)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    // 初始时应该需要发送心跳
    EXPECT_TRUE(m_keepAliveManager->needsKeepAlive(1, 2000));

    // 记录发送时间
    m_keepAliveManager->recordKeepAliveSent(1, 2000, 40); // tick 40 = 2000ms

    // 刚发送后不应该需要
    EXPECT_FALSE(m_keepAliveManager->needsKeepAlive(1, 2500));

    // 超过间隔后应该需要
    EXPECT_TRUE(m_keepAliveManager->needsKeepAlive(1, 3500));
}

TEST_F(KeepAliveManagerTest, NeedsKeepAliveNonexistentPlayer)
{
    EXPECT_FALSE(m_keepAliveManager->needsKeepAlive(999, 0));
}

TEST_F(KeepAliveManagerTest, GetPlayersNeedingKeepAlive)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);
    m_playerManager->addPlayer(2, mc::util::uuidToString(mc::util::generateOfflineUuid("Alex")), "Alex", nullptr);

    // 两个玩家都需要心跳
    auto players = m_keepAliveManager->getPlayersNeedingKeepAlive(2000);
    EXPECT_EQ(players.size(), 2u);

    // 只记录玩家1的发送时间
    m_keepAliveManager->recordKeepAliveSent(1, 2000, 40);

    // 只有玩家2需要心跳
    players = m_keepAliveManager->getPlayersNeedingKeepAlive(2500);
    EXPECT_EQ(players.size(), 1u);
    EXPECT_EQ(players[0], 2u);
}

TEST_F(KeepAliveManagerTest, HandleKeepAliveResponse)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    // 记录发送时间
    m_keepAliveManager->recordKeepAliveSent(1, 1000, 20);

    // 处理响应
    m_keepAliveManager->handleKeepAliveResponse(1, 1000, 1050);

    // ping 应该约为 50ms
    mc::u32 ping = m_keepAliveManager->getPlayerPing(1);
    EXPECT_EQ(ping, 50u);
}

TEST_F(KeepAliveManagerTest, HandleKeepAliveResponseWrongTimestamp)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    m_keepAliveManager->recordKeepAliveSent(1, 1000, 20);

    // 使用错误的时间戳
    m_keepAliveManager->handleKeepAliveResponse(1, 2000, 1050);

    // ping 应该还是 0（时间戳不匹配）
    mc::u32 ping = m_keepAliveManager->getPlayerPing(1);
    EXPECT_EQ(ping, 0u);
}

TEST_F(KeepAliveManagerTest, IsTimedOut)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    // 初始时不应该超时（没有接收过心跳）
    EXPECT_FALSE(m_keepAliveManager->isTimedOut(1, 1000));

    // 记录接收时间
    m_keepAliveManager->updateKeepAlive(1, 1000);

    // 超时时间内不应该超时
    EXPECT_FALSE(m_keepAliveManager->isTimedOut(1, 4000));

    // 超过超时时间应该超时
    EXPECT_TRUE(m_keepAliveManager->isTimedOut(1, 7000));
}

TEST_F(KeepAliveManagerTest, GetTimedOutPlayers)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);
    m_playerManager->addPlayer(2, mc::util::uuidToString(mc::util::generateOfflineUuid("Alex")), "Alex", nullptr);

    // 记录玩家1的接收时间
    m_keepAliveManager->updateKeepAlive(1, 1000);

    // 玩家2没有记录接收时间，不应超时
    auto timedOut = m_keepAliveManager->getTimedOutPlayers(2000);
    EXPECT_EQ(timedOut.size(), 0u);

    // 超过超时时间
    timedOut = m_keepAliveManager->getTimedOutPlayers(7000);
    EXPECT_EQ(timedOut.size(), 1u);
    EXPECT_EQ(timedOut[0], 1u);
}

TEST_F(KeepAliveManagerTest, UpdateKeepAlive)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    m_keepAliveManager->updateKeepAlive(1, 1000);
    EXPECT_EQ(m_keepAliveManager->getLastKeepAliveReceived(1), 1000u);
}

TEST_F(KeepAliveManagerTest, GetPlayerPingNonexistentPlayer)
{
    EXPECT_EQ(m_keepAliveManager->getPlayerPing(999), 0u);
}

TEST_F(KeepAliveManagerTest, GetLastKeepAliveSentNonexistentPlayer)
{
    EXPECT_EQ(m_keepAliveManager->getLastKeepAliveSent(999), 0u);
}

TEST_F(KeepAliveManagerTest, GetLastKeepAliveReceivedNonexistentPlayer)
{
    EXPECT_EQ(m_keepAliveManager->getLastKeepAliveReceived(999), 0u);
}
