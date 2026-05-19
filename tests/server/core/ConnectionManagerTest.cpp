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

#include "server/core/ConnectionManager.hpp"
#include "common/core/Types.hpp"
#include "common/network/connection/LocalConnection.hpp"
#include "common/network/connection/LocalServerConnection.hpp"
#include "common/network/packet/Packet.hpp"
#include "common/util/UuidUtils.hpp"
#include "server/core/PlayerManager.hpp"
#include <gtest/gtest.h>

using namespace mc::server::core;
using namespace mc::network;

/**
 * @brief ConnectionManager 单元测试
 */
class ConnectionManagerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_connectionPair = std::make_unique<LocalConnectionPair>();
        m_connectionPair->connect();
        m_playerManager = std::make_unique<PlayerManager>();
        m_connectionManager = std::make_unique<ConnectionManager>(*m_playerManager);
    }

    void TearDown() override
    {
        m_connectionManager.reset();
        m_playerManager.reset();
        m_connectionPair.reset();
    }

    ConnectionPtr createConnection()
    {
        return std::make_shared<LocalServerConnection>(&m_connectionPair->serverEndpoint());
    }

    std::unique_ptr<LocalConnectionPair> m_connectionPair;
    std::unique_ptr<PlayerManager> m_playerManager;
    std::unique_ptr<ConnectionManager> m_connectionManager;
};

TEST_F(ConnectionManagerTest, SendToPlayer)
{
    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

    std::vector<mc::u8> data = {1, 2, 3, 4, 5};
    EXPECT_TRUE(m_connectionManager->sendToPlayer(1, data.data(), data.size()));

    // 发送给不存在的玩家应返回 false
    EXPECT_FALSE(m_connectionManager->sendToPlayer(999, data.data(), data.size()));
}

TEST_F(ConnectionManagerTest, SendPacketToPlayer)
{
    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

    std::vector<mc::u8> payload = {1, 2, 3};
    EXPECT_TRUE(m_connectionManager->sendPacketToPlayer(1, PacketType::KeepAlive, payload));
}

TEST_F(ConnectionManagerTest, Broadcast)
{
    auto conn1 = createConnection();
    auto conn2 = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn1);
    m_playerManager->addPlayer(2, mc::util::uuidToString(mc::util::generateOfflineUuid("Alex")), "Alex", conn2);

    std::vector<mc::u8> data = {1, 2, 3};
    // 广播应不会崩溃
    m_connectionManager->broadcast(data.data(), data.size());
}

TEST_F(ConnectionManagerTest, BroadcastExcept)
{
    auto conn1 = createConnection();
    auto conn2 = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn1);
    m_playerManager->addPlayer(2, mc::util::uuidToString(mc::util::generateOfflineUuid("Alex")), "Alex", conn2);

    std::vector<mc::u8> data = {1, 2, 3};
    // 广播给除玩家1以外的所有玩家
    m_connectionManager->broadcastExcept(1, data.data(), data.size());
}

TEST_F(ConnectionManagerTest, BroadcastPacket)
{
    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

    std::vector<mc::u8> payload = {1, 2, 3};
    m_connectionManager->broadcastPacket(PacketType::KeepAlive, payload);
}

TEST_F(ConnectionManagerTest, DisconnectPlayer)
{
    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

    EXPECT_TRUE(m_playerManager->hasPlayer(1));

    m_connectionManager->disconnectPlayer(1, "Test disconnect");

    EXPECT_FALSE(m_playerManager->hasPlayer(1));
}

TEST_F(ConnectionManagerTest, DisconnectAll)
{
    auto conn1 = createConnection();
    auto conn2 = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn1);
    m_playerManager->addPlayer(2, mc::util::uuidToString(mc::util::generateOfflineUuid("Alex")), "Alex", conn2);

    EXPECT_EQ(m_playerManager->playerCount(), 2u);

    m_connectionManager->disconnectAll("Server shutdown");

    EXPECT_EQ(m_playerManager->playerCount(), 0u);
}

TEST_F(ConnectionManagerTest, CleanupDisconnectedPlayers)
{
    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

    // 手动断开连接
    conn->disconnect("Test");

    // 清理断开连接的玩家
    size_t cleaned = m_connectionManager->cleanupDisconnectedPlayers();
    EXPECT_EQ(cleaned, 1u);
    EXPECT_EQ(m_playerManager->playerCount(), 0u);
}

TEST_F(ConnectionManagerTest, EncapsulatePacket)
{
    std::vector<mc::u8> payload = {1, 2, 3, 4, 5};
    auto packet = ConnectionManager::encapsulatePacket(PacketType::KeepAlive, payload);

    // 验证包头
    EXPECT_GE(packet.size(), PACKET_HEADER_SIZE);

    // 解析包头
    PacketDeserializer deser(packet.data(), packet.size());
    auto sizeResult = deser.readU32();
    auto typeResult = deser.readU16();

    ASSERT_TRUE(sizeResult.success());
    ASSERT_TRUE(typeResult.success());

    EXPECT_EQ(sizeResult.value(), PACKET_HEADER_SIZE + payload.size());
    EXPECT_EQ(typeResult.value(), static_cast<mc::u16>(PacketType::KeepAlive));
}
