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

#include "server/core/KeepAliveManager.hpp"
#include "common/network/connection/LocalConnection.hpp"
#include "common/network/connection/LocalServerConnection.hpp"
#include "common/network/packet/Packet.hpp"
#include "common/util/UuidUtils.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PacketHandler.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/PositionTracker.hpp"
#include "server/core/ServerCoreConfig.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/core/TimeManager.hpp"
#include <gtest/gtest.h>

using namespace mc::server::core;
using namespace mc::network;
using mc::server::ServerCoreConfig;

/**
 * @brief KeepAliveManager 单元测试
 */
class KeepAliveManagerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_connectionPair = std::make_unique<LocalConnectionPair>();
        m_connectionPair->connect();
        m_playerManager = std::make_unique<PlayerManager>();
        m_config.keepAliveInterval = 1000;
        m_config.keepAliveTimeout = 5000;
        m_keepAliveManager = std::make_unique<KeepAliveManager>(*m_playerManager, m_config);
    }

    void TearDown() override
    {
        m_keepAliveManager.reset();
        m_playerManager.reset();
        m_connectionPair.reset();
    }

    ConnectionPtr createConnection()
    {
        return std::make_shared<LocalServerConnection>(&m_connectionPair->serverEndpoint());
    }

    ServerCoreConfig m_config;
    std::unique_ptr<LocalConnectionPair> m_connectionPair;
    std::unique_ptr<PlayerManager> m_playerManager;
    std::unique_ptr<KeepAliveManager> m_keepAliveManager;
};

TEST_F(KeepAliveManagerTest, NeedsKeepAlive)
{
    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

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
    auto conn1 = createConnection();
    auto conn2 = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn1);
    m_playerManager->addPlayer(2, mc::util::uuidToString(mc::util::generateOfflineUuid("Alex")), "Alex", conn2);

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
    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

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
    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

    m_keepAliveManager->recordKeepAliveSent(1, 1000, 20);

    // 使用错误的时间戳
    m_keepAliveManager->handleKeepAliveResponse(1, 2000, 1050);

    // ping 应该还是 0（时间戳不匹配）
    mc::u32 ping = m_keepAliveManager->getPlayerPing(1);
    EXPECT_EQ(ping, 0u);
}

TEST_F(KeepAliveManagerTest, IsTimedOut)
{
    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

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
    auto conn1 = createConnection();
    auto conn2 = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn1);
    m_playerManager->addPlayer(2, mc::util::uuidToString(mc::util::generateOfflineUuid("Alex")), "Alex", conn2);

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
    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

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

TEST(KeepAlivePacketHandler, HandleFullPacket)
{
    mc::server::ServerCoreConfig config;
    config.viewDistance = 6;
    config.keepAliveInterval = 1000;
    config.keepAliveTimeout = 5000;

    mc::server::core::PlayerManager playerManager;
    mc::server::core::ConnectionManager connectionManager(playerManager);
    mc::server::core::TimeManager timeManager(0, 0);
    mc::server::core::TeleportManager teleportManager(playerManager);
    mc::server::core::KeepAliveManager keepAliveManager(playerManager, config);
    mc::server::core::PositionTracker positionTracker(playerManager, config);
    mc::server::core::PacketHandler packetHandler(
        playerManager, connectionManager, teleportManager, keepAliveManager, positionTracker, timeManager, config);

    auto connectionPair = std::make_unique<mc::network::LocalConnectionPair>();
    connectionPair->connect();
    auto connection = std::make_shared<mc::network::LocalServerConnection>(&connectionPair->serverEndpoint());

    auto* player =
        playerManager.addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", connection);
    ASSERT_NE(player, nullptr);
    playerManager.mapSessionToPlayer(1, 1);

    const mc::u64 timestamp = 1000;
    keepAliveManager.recordKeepAliveSent(1, timestamp, 20);

    mc::network::KeepAlivePacket packet;
    packet.setTimestamp(timestamp);
    auto serializeResult = packet.serialize();
    ASSERT_TRUE(serializeResult.success());

    const auto& data = serializeResult.value();
    auto result = packetHandler.handleKeepAlive(1, data.data(), data.size(), 1050);

    EXPECT_EQ(result, mc::server::core::PacketHandleResult::Success);
    EXPECT_EQ(keepAliveManager.getLastKeepAliveReceived(1), 1050u);
    EXPECT_EQ(keepAliveManager.getPlayerPing(1), 50u);
}
