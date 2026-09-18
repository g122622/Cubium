/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the
 * following conditions:
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

#include "server/core/PlayerManager.hpp"
#include "common/core/Types.hpp"
#include "common/util/UuidUtils.hpp"
#include <algorithm>
#include <gtest/gtest.h>

using namespace mc::server::core;

/**
 * @brief PlayerManager 单元测试
 *
 * 新网络层 addPlayer 第4参为 mc::server::net::ServerClientConnection*（裸指针）。
 * 本测试只验证 PlayerManager 的玩家生命周期/查询/映射数据维护，不依赖连接真发包，
 * 故统一传 nullptr（与 BaseTestServer::addTestPlayer 一致，Step5 删旧体系后
 * 统一重构测试桩）。
 */
class PlayerManagerTest : public ::testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(PlayerManagerTest, DefaultConstruction)
{
    PlayerManager manager;
    EXPECT_EQ(manager.playerCount(), 0u);
    EXPECT_EQ(manager.maxPlayers(), 20); // default
}

TEST_F(PlayerManagerTest, ConstructionWithConfig)
{
    PlayerManager manager(50);
    EXPECT_EQ(manager.maxPlayers(), 50);
}

TEST_F(PlayerManagerTest, AddPlayer)
{
    PlayerManager manager;

    auto* player =
        manager.addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);
    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->playerId, 1u);
    EXPECT_EQ(player->username, "Steve");
    EXPECT_TRUE(player->loggedIn);
    EXPECT_EQ(manager.playerCount(), 1u);
    EXPECT_TRUE(manager.hasPlayer(1));
}

TEST_F(PlayerManagerTest, AddPlayerDuplicate)
{
    PlayerManager manager;

    auto* player1 =
        manager.addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);
    ASSERT_NE(player1, nullptr);

    // 添加重复ID应返回 nullptr
    auto* player2 =
        manager.addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Alex")), "Alex", nullptr);
    EXPECT_EQ(player2, nullptr);
    EXPECT_EQ(manager.playerCount(), 1u);
}

TEST_F(PlayerManagerTest, AddPlayerWhenFull)
{
    PlayerManager manager(2);

    ASSERT_NE(
        manager.addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Player1")), "Player1", nullptr),
        nullptr);
    ASSERT_NE(
        manager.addPlayer(2, mc::util::uuidToString(mc::util::generateOfflineUuid("Player2")), "Player2", nullptr),
        nullptr);

    // 已满时应返回 nullptr
    EXPECT_EQ(
        manager.addPlayer(3, mc::util::uuidToString(mc::util::generateOfflineUuid("Player3")), "Player3", nullptr),
        nullptr);
    EXPECT_TRUE(manager.isFull());
}

TEST_F(PlayerManagerTest, RemovePlayer)
{
    PlayerManager manager;

    manager.addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);
    EXPECT_EQ(manager.playerCount(), 1u);

    manager.removePlayer(1);
    EXPECT_EQ(manager.playerCount(), 0u);
    EXPECT_FALSE(manager.hasPlayer(1));
}

TEST_F(PlayerManagerTest, RemoveNonexistentPlayer)
{
    PlayerManager manager;
    // 移除不存在的玩家应该安全
    manager.removePlayer(999);
    EXPECT_EQ(manager.playerCount(), 0u);
}

TEST_F(PlayerManagerTest, GetPlayer)
{
    PlayerManager manager;

    manager.addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    auto* player = manager.getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->username, "Steve");

    auto* playerConst = static_cast<const PlayerManager&>(manager).getPlayer(1);
    ASSERT_NE(playerConst, nullptr);
    EXPECT_EQ(playerConst->username, "Steve");

    EXPECT_EQ(manager.getPlayer(999), nullptr);
}

TEST_F(PlayerManagerTest, SessionMapping)
{
    PlayerManager manager;

    manager.addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);
    manager.mapSessionToPlayer(100, 1);

    EXPECT_EQ(manager.getPlayerIdBySession(100), 1u);
    EXPECT_EQ(manager.findBySessionId(100)->playerId, 1u);

    manager.unmapSession(100);
    EXPECT_EQ(manager.getPlayerIdBySession(100), 0u);
}

TEST_F(PlayerManagerTest, RemovePlayerBySessionId)
{
    PlayerManager manager;

    manager.addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);
    manager.mapSessionToPlayer(100, 1);
    EXPECT_EQ(manager.playerCount(), 1u);

    manager.removePlayerBySessionId(100);
    EXPECT_EQ(manager.playerCount(), 0u);
    EXPECT_EQ(manager.getPlayerIdBySession(100), 0u);
}

TEST_F(PlayerManagerTest, ForEachPlayer)
{
    PlayerManager manager;

    manager.addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);
    manager.addPlayer(2, mc::util::uuidToString(mc::util::generateOfflineUuid("Alex")), "Alex", nullptr);

    std::vector<mc::PlayerId> ids;
    manager.forEachPlayer([&ids](mc::server::ServerPlayerData& player) { ids.push_back(player.playerId); });

    EXPECT_EQ(ids.size(), 2u);
    EXPECT_NE(std::find(ids.begin(), ids.end(), mc::PlayerId(1)), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), mc::PlayerId(2)), ids.end());
}

TEST_F(PlayerManagerTest, ForEachPlayerCanNestGetPlayerCall)
{
    PlayerManager manager;

    manager.addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);
    manager.addPlayer(2, mc::util::uuidToString(mc::util::generateOfflineUuid("Alex")), "Alex", nullptr);

    size_t nestedLookupSuccess = 0;
    manager.forEachPlayer([&](mc::server::ServerPlayerData& player) {
        auto* found = manager.getPlayer(player.playerId);
        if (found != nullptr && found->playerId == player.playerId) {
            ++nestedLookupSuccess;
        }
    });

    EXPECT_EQ(nestedLookupSuccess, 2u);
}

TEST_F(PlayerManagerTest, ForEachPlayerSupportsRemovalDuringIteration)
{
    PlayerManager manager;

    manager.addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);
    manager.addPlayer(2, mc::util::uuidToString(mc::util::generateOfflineUuid("Alex")), "Alex", nullptr);
    manager.addPlayer(3, mc::util::uuidToString(mc::util::generateOfflineUuid("Eve")), "Eve", nullptr);

    manager.forEachPlayer([&](mc::server::ServerPlayerData& player) {
        if (player.playerId == 2) {
            manager.removePlayer(2);
        }
    });

    EXPECT_FALSE(manager.hasPlayer(2));
    EXPECT_EQ(manager.playerCount(), 2u);
}

TEST_F(PlayerManagerTest, GetPlayerIds)
{
    PlayerManager manager;

    manager.addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);
    manager.addPlayer(2, mc::util::uuidToString(mc::util::generateOfflineUuid("Alex")), "Alex", nullptr);

    auto ids = manager.getPlayerIds();
    EXPECT_EQ(ids.size(), 2u);
}

TEST_F(PlayerManagerTest, NextPlayerId)
{
    PlayerManager manager;

    auto id1 = manager.nextPlayerId();
    auto id2 = manager.nextPlayerId();
    auto id3 = manager.nextPlayerId();

    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
}

TEST_F(PlayerManagerTest, ChunkSyncManager)
{
    PlayerManager manager;
    auto& chunkSync = manager.chunkSyncManager();
    EXPECT_EQ(chunkSync.defaultViewDistance(), 10);

    chunkSync.setDefaultViewDistance(12);
    EXPECT_EQ(chunkSync.defaultViewDistance(), 12);
}

TEST_F(PlayerManagerTest, SetMaxPlayers)
{
    PlayerManager manager;
    EXPECT_EQ(manager.maxPlayers(), 20);

    manager.setMaxPlayers(50);
    EXPECT_EQ(manager.maxPlayers(), 50);
}

// ========== IP 地址相关测试 ==========

TEST_F(PlayerManagerTest, LocalConnectionHasEmptyIpAddress)
{
    PlayerManager manager;

    auto* player =
        manager.addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);
    ASSERT_NE(player, nullptr);

    // 新网络层：Local 模式下 addPlayer 始终将 ipAddress 置空（PlayerManager.cpp 注释明示）。
    // 旧 LocalServerConnection::getAddress()/type() 断言随旧连接体系删除而移除。
    EXPECT_EQ(player->ipAddress, "");
}

TEST_F(PlayerManagerTest, GetPlayerIdsByAddress)
{
    PlayerManager manager;

    auto* player1 =
        manager.addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);
    auto* player2 =
        manager.addPlayer(2, mc::util::uuidToString(mc::util::generateOfflineUuid("Alex")), "Alex", nullptr);
    auto* player3 = manager.addPlayer(3, mc::util::uuidToString(mc::util::generateOfflineUuid("Eve")), "Eve", nullptr);

    ASSERT_NE(player1, nullptr);
    ASSERT_NE(player2, nullptr);
    ASSERT_NE(player3, nullptr);

    // 手动设置 IP 地址（模拟 TCP 连接）
    player1->ipAddress = "192.168.1.100";
    player2->ipAddress = "192.168.1.100"; // 同一 IP
    player3->ipAddress = "192.168.1.200"; // 不同 IP

    // 查找同一 IP 的玩家
    auto players100 = manager.getPlayerIdsByAddress("192.168.1.100");
    EXPECT_EQ(players100.size(), 2u);
    EXPECT_NE(std::find(players100.begin(), players100.end(), mc::PlayerId(1)), players100.end());
    EXPECT_NE(std::find(players100.begin(), players100.end(), mc::PlayerId(2)), players100.end());

    // 查找另一个 IP 的玩家
    auto players200 = manager.getPlayerIdsByAddress("192.168.1.200");
    EXPECT_EQ(players200.size(), 1u);
    EXPECT_EQ(players200[0], mc::PlayerId(3));

    // 查找不存在的 IP
    auto playersEmpty = manager.getPlayerIdsByAddress("10.0.0.1");
    EXPECT_EQ(playersEmpty.size(), 0u);

    // 查找空 IP（本地连接）
    auto localPlayers = manager.getPlayerIdsByAddress("");
    EXPECT_EQ(localPlayers.size(), 0u); // 没有空 IP 的玩家（都设置了 IP）
}

TEST_F(PlayerManagerTest, FindByUsername)
{
    PlayerManager manager;

    manager.addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);
    manager.addPlayer(2, mc::util::uuidToString(mc::util::generateOfflineUuid("Alex")), "Alex", nullptr);

    // 精确匹配
    auto* player = manager.findByUsername("Steve");
    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->playerId, 1u);

    // 不区分大小写
    auto* playerLower = manager.findByUsername("steve");
    ASSERT_NE(playerLower, nullptr);
    EXPECT_EQ(playerLower->playerId, 1u);

    auto* playerUpper = manager.findByUsername("STEVE");
    ASSERT_NE(playerUpper, nullptr);
    EXPECT_EQ(playerUpper->playerId, 1u);

    // 混合大小写
    auto* playerMixed = manager.findByUsername("AlEx");
    ASSERT_NE(playerMixed, nullptr);
    EXPECT_EQ(playerMixed->playerId, 2u);

    // 不存在的玩家
    EXPECT_EQ(manager.findByUsername("NotExists"), nullptr);

    // const 版本
    const auto& constManager = manager;
    auto* playerConst = constManager.findByUsername("Steve");
    ASSERT_NE(playerConst, nullptr);
    EXPECT_EQ(playerConst->playerId, 1u);
}
