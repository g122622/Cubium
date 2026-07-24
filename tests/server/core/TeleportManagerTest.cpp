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

#include "server/core/TeleportManager.hpp"
#include "common/core/Types.hpp"
#include "common/util/UuidUtils.hpp"
#include "server/core/PlayerManager.hpp"
#include <gtest/gtest.h>

using namespace mc::server::core;

/**
 * @brief TeleportManager 单元测试
 *
 * 新网络层 addPlayer 第4参为 mc::server::net::ServerClientConnection*（裸指针）。
 * 本测试只验证 TeleportManager 的传送请求/确认/状态数据维护，不依赖连接真发包，
 * 故统一传 nullptr（与 BaseTestServer::addTestPlayer 一致）。
 */
class TeleportManagerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_playerManager = std::make_unique<PlayerManager>();
        m_teleportManager = std::make_unique<TeleportManager>(*m_playerManager);
    }

    void TearDown() override
    {
        m_teleportManager.reset();
        m_playerManager.reset();
    }

    std::unique_ptr<PlayerManager> m_playerManager;
    std::unique_ptr<TeleportManager> m_teleportManager;
};

TEST_F(TeleportManagerTest, RequestTeleport)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    mc::u32 teleportId = m_teleportManager->requestTeleport(1, 100.0, 64.0, 200.0, 90.0f, 45.0f);
    EXPECT_NE(teleportId, 0u);

    // 验证玩家位置已更新
    auto* player = m_playerManager->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_FLOAT_EQ(player->x, 100.0f);
    EXPECT_FLOAT_EQ(player->y, 64.0f);
    EXPECT_FLOAT_EQ(player->z, 200.0f);
    EXPECT_FLOAT_EQ(player->yaw, 90.0f);
    EXPECT_FLOAT_EQ(player->pitch, 45.0f);

    // 验证玩家正在等待传送确认
    EXPECT_TRUE(m_teleportManager->isWaitingForConfirm(1));
    EXPECT_EQ(m_teleportManager->getPendingTeleportId(1), teleportId);
}

TEST_F(TeleportManagerTest, RequestTeleportNonexistentPlayer)
{
    mc::u32 teleportId = m_teleportManager->requestTeleport(999, 100.0, 64.0, 200.0);
    EXPECT_EQ(teleportId, 0u);
}

TEST_F(TeleportManagerTest, ConfirmTeleport)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    mc::u32 teleportId = m_teleportManager->requestTeleport(1, 100.0, 64.0, 200.0);

    // 确认传送
    bool result = m_teleportManager->confirmTeleport(1, teleportId);
    EXPECT_TRUE(result);
    EXPECT_FALSE(m_teleportManager->isWaitingForConfirm(1));
}

TEST_F(TeleportManagerTest, ConfirmTeleportWrongId)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    m_teleportManager->requestTeleport(1, 100.0, 64.0, 200.0);

    // 使用错误的ID确认
    bool result = m_teleportManager->confirmTeleport(1, 999);
    EXPECT_FALSE(result);
    EXPECT_TRUE(m_teleportManager->isWaitingForConfirm(1));
}

TEST_F(TeleportManagerTest, ConfirmTeleportWithoutRequest)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    // 没有传送请求时确认
    bool result = m_teleportManager->confirmTeleport(1, 1);
    EXPECT_FALSE(result);
}

TEST_F(TeleportManagerTest, ConfirmTeleportNonexistentPlayer)
{
    bool result = m_teleportManager->confirmTeleport(999, 1);
    EXPECT_FALSE(result);
}

TEST_F(TeleportManagerTest, MultipleTeleports)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    mc::u32 id1 = m_teleportManager->requestTeleport(1, 100.0, 64.0, 200.0);
    mc::u32 id2 = m_teleportManager->requestTeleport(1, 200.0, 64.0, 300.0);

    EXPECT_NE(id1, id2);

    // 只有最后一次传送有效
    EXPECT_EQ(m_teleportManager->getPendingTeleportId(1), id2);

    // 确认第一次传送应该失败
    EXPECT_FALSE(m_teleportManager->confirmTeleport(1, id1));

    // 确认第二次传送应该成功
    EXPECT_TRUE(m_teleportManager->confirmTeleport(1, id2));
}

TEST_F(TeleportManagerTest, IsWaitingForConfirmNonexistentPlayer)
{
    EXPECT_FALSE(m_teleportManager->isWaitingForConfirm(999));
}

TEST_F(TeleportManagerTest, GetPendingTeleportIdNonexistentPlayer)
{
    EXPECT_EQ(m_teleportManager->getPendingTeleportId(999), 0u);
}
