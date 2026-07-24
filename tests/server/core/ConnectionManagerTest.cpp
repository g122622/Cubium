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
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/util/UuidUtils.hpp"
#include "server/core/PlayerManager.hpp"
#include <gtest/gtest.h>

using namespace mc::server::core;

/**
 * @brief ConnectionManager 单元测试（新网络层 IR 版本）
 *
 * 新 ConnectionManager 是薄门面：sendToPlayer/broadcast/broadcastExcept 委托
 * ServerPlayerData::send(ir::IrPacket) → ServerClientConnection::send。本测试
 * 不构造真实 ServerClientConnection（需 LocalTransportPair + ProtocolTables，
 * 属集成测试范畴），统一传 nullptr 连接：
 * - 发送类用例：nullptr 连接下 ServerPlayerData::send 返回 false，可断言返回值。
 * - 玩家管理类用例（disconnect/cleanup/disconnectAll）：验证 PlayerManager 侧状态。
 *
 * 注意：conn=nullptr 时 ServerPlayerData::hasConnection() 返回 false，故
 * cleanupDisconnectedPlayers 会把所有 nullptr 连接玩家视为已断开并清理。
 */
class ConnectionManagerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_playerManager = std::make_unique<PlayerManager>();
        m_connectionManager = std::make_unique<ConnectionManager>(*m_playerManager);
    }

    void TearDown() override
    {
        m_connectionManager.reset();
        m_playerManager.reset();
    }

    /// 构造一个最小 IR 包（Play 阶段 KeepAlive）用于发送类用例
    static mc::network::ir::IrPacket makeKeepAlivePacket()
    {
        return mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Play,
            mc::network::ir::PlayPacket{mc::network::ir::play::KeepAlive{}}};
    }

    std::unique_ptr<PlayerManager> m_playerManager;
    std::unique_ptr<ConnectionManager> m_connectionManager;
};

TEST_F(ConnectionManagerTest, SendToPlayerReturnsFalseWhenConnectionIsNull)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    // nullptr 连接下 send 返回 false（不崩溃）
    EXPECT_FALSE(m_connectionManager->sendToPlayer(1, makeKeepAlivePacket()));

    // 发送给不存在的玩家应返回 false
    EXPECT_FALSE(m_connectionManager->sendToPlayer(999, makeKeepAlivePacket()));
}

TEST_F(ConnectionManagerTest, BroadcastDoesNotCrashWithNullConnections)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);
    m_playerManager->addPlayer(2, mc::util::uuidToString(mc::util::generateOfflineUuid("Alex")), "Alex", nullptr);

    // 广播应不会崩溃（各玩家 send 返回 false 被忽略）
    m_connectionManager->broadcast(makeKeepAlivePacket());
}

TEST_F(ConnectionManagerTest, BroadcastExceptDoesNotCrashWithNullConnections)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);
    m_playerManager->addPlayer(2, mc::util::uuidToString(mc::util::generateOfflineUuid("Alex")), "Alex", nullptr);

    // 广播给除玩家1以外的所有玩家应不会崩溃
    m_connectionManager->broadcastExcept(1, makeKeepAlivePacket());
}

TEST_F(ConnectionManagerTest, DisconnectPlayer)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    EXPECT_TRUE(m_playerManager->hasPlayer(1));

    m_connectionManager->disconnectPlayer(1, "Test disconnect");

    EXPECT_FALSE(m_playerManager->hasPlayer(1));
}

TEST_F(ConnectionManagerTest, DisconnectAll)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);
    m_playerManager->addPlayer(2, mc::util::uuidToString(mc::util::generateOfflineUuid("Alex")), "Alex", nullptr);

    EXPECT_EQ(m_playerManager->playerCount(), 2u);

    m_connectionManager->disconnectAll("Server shutdown");

    EXPECT_EQ(m_playerManager->playerCount(), 0u);
}

TEST_F(ConnectionManagerTest, CleanupDisconnectedPlayersTreatsNullConnectionAsDisconnected)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    // nullptr 连接 hasConnection() 返回 false，被视为已断开
    size_t cleaned = m_connectionManager->cleanupDisconnectedPlayers();
    EXPECT_EQ(cleaned, 1u);
    EXPECT_EQ(m_playerManager->playerCount(), 0u);
}
