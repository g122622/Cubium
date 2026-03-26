#include <gtest/gtest.h>
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerCoreConfig.hpp"
#include "common/network/connection/LocalServerConnection.hpp"
#include "common/network/connection/LocalConnection.hpp"
#include "common/core/Types.hpp"
#include <algorithm>

using namespace mc::server::core;
using namespace mc::network;
using mc::server::ServerCoreConfig;

/**
 * @brief PlayerManager 单元测试
 */
class PlayerManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建本地连接对用于测试
        m_connectionPair = std::make_unique<LocalConnectionPair>();
        m_connectionPair->connect();
    }

    void TearDown() override {
        m_connectionPair.reset();
    }

    ConnectionPtr createConnection() {
        return std::make_shared<LocalServerConnection>(&m_connectionPair->serverEndpoint());
    }

    ServerCoreConfig m_config;
    std::unique_ptr<LocalConnectionPair> m_connectionPair;
};

TEST_F(PlayerManagerTest, DefaultConstruction) {
    PlayerManager manager;
    EXPECT_EQ(manager.playerCount(), 0u);
    EXPECT_EQ(manager.maxPlayers(), 20); // default
}

TEST_F(PlayerManagerTest, ConstructionWithConfig) {
    m_config.maxPlayers = 50;
    m_config.viewDistance = 12;
    PlayerManager manager(m_config);
    EXPECT_EQ(manager.maxPlayers(), 50);
}

TEST_F(PlayerManagerTest, AddPlayer) {
    PlayerManager manager;
    auto conn = createConnection();

    auto* player = manager.addPlayer(1, "Steve", conn);
    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->playerId, 1u);
    EXPECT_EQ(player->username, "Steve");
    EXPECT_TRUE(player->loggedIn);
    EXPECT_EQ(manager.playerCount(), 1u);
    EXPECT_TRUE(manager.hasPlayer(1));
}

TEST_F(PlayerManagerTest, AddPlayerDuplicate) {
    PlayerManager manager;
    auto conn = createConnection();

    auto* player1 = manager.addPlayer(1, "Steve", conn);
    ASSERT_NE(player1, nullptr);

    // 添加重复ID应返回 nullptr
    auto* player2 = manager.addPlayer(1, "Alex", conn);
    EXPECT_EQ(player2, nullptr);
    EXPECT_EQ(manager.playerCount(), 1u);
}

TEST_F(PlayerManagerTest, AddPlayerWhenFull) {
    m_config.maxPlayers = 2;
    PlayerManager manager(m_config);

    auto conn1 = createConnection();
    auto conn2 = createConnection();
    auto conn3 = createConnection();

    ASSERT_NE(manager.addPlayer(1, "Player1", conn1), nullptr);
    ASSERT_NE(manager.addPlayer(2, "Player2", conn2), nullptr);

    // 已满时应返回 nullptr
    EXPECT_EQ(manager.addPlayer(3, "Player3", conn3), nullptr);
    EXPECT_TRUE(manager.isFull());
}

TEST_F(PlayerManagerTest, RemovePlayer) {
    PlayerManager manager;
    auto conn = createConnection();

    manager.addPlayer(1, "Steve", conn);
    EXPECT_EQ(manager.playerCount(), 1u);

    manager.removePlayer(1);
    EXPECT_EQ(manager.playerCount(), 0u);
    EXPECT_FALSE(manager.hasPlayer(1));
}

TEST_F(PlayerManagerTest, RemoveNonexistentPlayer) {
    PlayerManager manager;
    // 移除不存在的玩家应该安全
    manager.removePlayer(999);
    EXPECT_EQ(manager.playerCount(), 0u);
}

TEST_F(PlayerManagerTest, GetPlayer) {
    PlayerManager manager;
    auto conn = createConnection();

    manager.addPlayer(1, "Steve", conn);

    auto* player = manager.getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->username, "Steve");

    auto* playerConst = static_cast<const PlayerManager&>(manager).getPlayer(1);
    ASSERT_NE(playerConst, nullptr);
    EXPECT_EQ(playerConst->username, "Steve");

    EXPECT_EQ(manager.getPlayer(999), nullptr);
}

TEST_F(PlayerManagerTest, SessionMapping) {
    PlayerManager manager;
    auto conn = createConnection();

    manager.addPlayer(1, "Steve", conn);
    manager.mapSessionToPlayer(100, 1);

    EXPECT_EQ(manager.getPlayerIdBySession(100), 1u);
    EXPECT_EQ(manager.findBySessionId(100)->playerId, 1u);

    manager.unmapSession(100);
    EXPECT_EQ(manager.getPlayerIdBySession(100), 0u);
}

TEST_F(PlayerManagerTest, RemovePlayerBySessionId) {
    PlayerManager manager;
    auto conn = createConnection();

    manager.addPlayer(1, "Steve", conn);
    manager.mapSessionToPlayer(100, 1);
    EXPECT_EQ(manager.playerCount(), 1u);

    manager.removePlayerBySessionId(100);
    EXPECT_EQ(manager.playerCount(), 0u);
    EXPECT_EQ(manager.getPlayerIdBySession(100), 0u);
}

TEST_F(PlayerManagerTest, ForEachPlayer) {
    PlayerManager manager;
    auto conn1 = createConnection();
    auto conn2 = createConnection();

    manager.addPlayer(1, "Steve", conn1);
    manager.addPlayer(2, "Alex", conn2);

    std::vector<mc::PlayerId> ids;
    manager.forEachPlayer([&ids](mc::server::ServerPlayerData& player) {
        ids.push_back(player.playerId);
    });

    EXPECT_EQ(ids.size(), 2u);
    EXPECT_NE(std::find(ids.begin(), ids.end(), mc::PlayerId(1)), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), mc::PlayerId(2)), ids.end());
}

TEST_F(PlayerManagerTest, ForEachPlayerCanNestGetPlayerCall) {
    PlayerManager manager;
    auto conn1 = createConnection();
    auto conn2 = createConnection();

    manager.addPlayer(1, "Steve", conn1);
    manager.addPlayer(2, "Alex", conn2);

    size_t nestedLookupSuccess = 0;
    manager.forEachPlayer([&](mc::server::ServerPlayerData& player) {
        auto* found = manager.getPlayer(player.playerId);
        if (found != nullptr && found->playerId == player.playerId) {
            ++nestedLookupSuccess;
        }
    });

    EXPECT_EQ(nestedLookupSuccess, 2u);
}

TEST_F(PlayerManagerTest, ForEachPlayerSupportsRemovalDuringIteration) {
    PlayerManager manager;
    auto conn1 = createConnection();
    auto conn2 = createConnection();
    auto conn3 = createConnection();

    manager.addPlayer(1, "Steve", conn1);
    manager.addPlayer(2, "Alex", conn2);
    manager.addPlayer(3, "Eve", conn3);

    manager.forEachPlayer([&](mc::server::ServerPlayerData& player) {
        if (player.playerId == 2) {
            manager.removePlayer(2);
        }
    });

    EXPECT_FALSE(manager.hasPlayer(2));
    EXPECT_EQ(manager.playerCount(), 2u);
}

TEST_F(PlayerManagerTest, GetPlayerIds) {
    PlayerManager manager;
    auto conn1 = createConnection();
    auto conn2 = createConnection();

    manager.addPlayer(1, "Steve", conn1);
    manager.addPlayer(2, "Alex", conn2);

    auto ids = manager.getPlayerIds();
    EXPECT_EQ(ids.size(), 2u);
}

TEST_F(PlayerManagerTest, NextPlayerId) {
    PlayerManager manager;

    auto id1 = manager.nextPlayerId();
    auto id2 = manager.nextPlayerId();
    auto id3 = manager.nextPlayerId();

    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
}

TEST_F(PlayerManagerTest, ChunkSyncManager) {
    PlayerManager manager;
    auto& chunkSync = manager.chunkSyncManager();
    EXPECT_EQ(chunkSync.defaultViewDistance(), 10);

    chunkSync.setDefaultViewDistance(12);
    EXPECT_EQ(chunkSync.defaultViewDistance(), 12);
}

TEST_F(PlayerManagerTest, SetMaxPlayers) {
    PlayerManager manager;
    EXPECT_EQ(manager.maxPlayers(), 20);

    manager.setMaxPlayers(50);
    EXPECT_EQ(manager.maxPlayers(), 50);
}
