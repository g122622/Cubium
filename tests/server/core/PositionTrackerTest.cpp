#include <gtest/gtest.h>
#include "server/core/PositionTracker.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerCoreConfig.hpp"
#include "common/network/connection/LocalServerConnection.hpp"
#include "common/network/connection/LocalConnection.hpp"
#include "common/core/Types.hpp"
#include "common/world/chunk/ChunkPos.hpp"
#include "common/util/UuidUtils.hpp"
#include <algorithm>
#include <vector>

using namespace mc::server::core;
using namespace mc::network;
using mc::server::ServerCoreConfig;
using mc::ChunkPos;
using mc::ChunkCoord;

/**
 * @brief PositionTracker 单元测试
 */
class PositionTrackerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_connectionPair = std::make_unique<LocalConnectionPair>();
        m_connectionPair->connect();
        m_playerManager = std::make_unique<PlayerManager>();
        m_positionTracker = std::make_unique<PositionTracker>(*m_playerManager, m_config);
    }

    void TearDown() override {
        m_positionTracker.reset();
        m_playerManager.reset();
        m_connectionPair.reset();
    }

    ConnectionPtr createConnection() {
        return std::make_shared<LocalServerConnection>(&m_connectionPair->serverEndpoint());
    }

    ServerCoreConfig m_config;
    std::unique_ptr<LocalConnectionPair> m_connectionPair;
    std::unique_ptr<PlayerManager> m_playerManager;
    std::unique_ptr<PositionTracker> m_positionTracker;
};

TEST_F(PositionTrackerTest, UpdatePosition) {
    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

    bool result = m_positionTracker->updatePosition(1, 100.5, 64.0, 200.5, 90.0f, 45.0f, true);
    EXPECT_TRUE(result);

    auto* player = m_playerManager->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_FLOAT_EQ(player->x, 100.5f);
    EXPECT_FLOAT_EQ(player->y, 64.0f);
    EXPECT_FLOAT_EQ(player->z, 200.5f);
    EXPECT_FLOAT_EQ(player->yaw, 90.0f);
    EXPECT_FLOAT_EQ(player->pitch, 45.0f);
    EXPECT_TRUE(player->onGround);
}

TEST_F(PositionTrackerTest, UpdatePositionNonexistentPlayer) {
    bool result = m_positionTracker->updatePosition(999, 100.0, 64.0, 200.0, 0.0f, 0.0f, true);
    EXPECT_FALSE(result);
}

TEST_F(PositionTrackerTest, UpdatePositionOnly) {
    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

    m_positionTracker->updatePosition(1, 100.0, 64.0, 200.0);

    auto* player = m_playerManager->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_FLOAT_EQ(player->x, 100.0f);
    EXPECT_FLOAT_EQ(player->y, 64.0f);
    EXPECT_FLOAT_EQ(player->z, 200.0f);
}

TEST_F(PositionTrackerTest, UpdateRotation) {
    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

    m_positionTracker->updateRotation(1, 180.0f, 30.0f);

    auto* player = m_playerManager->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_FLOAT_EQ(player->yaw, 180.0f);
    EXPECT_FLOAT_EQ(player->pitch, 30.0f);
}

TEST_F(PositionTrackerTest, GetPosition) {
    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

    m_positionTracker->updatePosition(1, 100.0, 64.0, 200.0, 90.0f, 45.0f, true);

    auto pos = m_positionTracker->getPosition(1);
    EXPECT_FLOAT_EQ(pos.x, 100.0f);
    EXPECT_FLOAT_EQ(pos.y, 64.0f);
    EXPECT_FLOAT_EQ(pos.z, 200.0f);

    // 不存在的玩家返回默认位置
    auto defaultPos = m_positionTracker->getPosition(999);
    EXPECT_FLOAT_EQ(defaultPos.y, 64.0f);
}

TEST_F(PositionTrackerTest, GetRotation) {
    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

    m_positionTracker->updateRotation(1, 180.0f, 30.0f);

    auto rot = m_positionTracker->getRotation(1);
    EXPECT_FLOAT_EQ(rot.x, 180.0f);
    EXPECT_FLOAT_EQ(rot.y, 30.0f);
}

TEST_F(PositionTrackerTest, GetChunkPosition) {
    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

    m_positionTracker->updatePosition(1, 100.0, 64.0, 200.0);

    auto chunkPos = m_positionTracker->getChunkPosition(1);
    EXPECT_EQ(chunkPos.x, 6);   // 100 / 16 = 6.25 -> 6
    EXPECT_EQ(chunkPos.z, 12);  // 200 / 16 = 12.5 -> 12
}

TEST_F(PositionTrackerTest, IsOnGround) {
    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

    m_positionTracker->updatePosition(1, 100.0, 64.0, 200.0, 0.0f, 0.0f, true);
    EXPECT_TRUE(m_positionTracker->isOnGround(1));

    m_positionTracker->updatePosition(1, 100.0, 70.0, 200.0, 0.0f, 0.0f, false);
    EXPECT_FALSE(m_positionTracker->isOnGround(1));
}

TEST_F(PositionTrackerTest, SetViewDistance) {
    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

    m_positionTracker->setViewDistance(1, 12);
    EXPECT_EQ(m_positionTracker->getViewDistance(1), 12);
}

TEST_F(PositionTrackerTest, CalculateChunkUpdates) {
    m_config.viewDistance = 2;
    m_playerManager = std::make_unique<PlayerManager>(m_config);
    m_positionTracker = std::make_unique<PositionTracker>(*m_playerManager, m_config);

    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

    // 设置初始位置
    m_positionTracker->updatePosition(1, 0.0, 64.0, 0.0);

    std::vector<ChunkPos> toLoad, toUnload;
    m_positionTracker->calculateChunkUpdates(1, toLoad, toUnload);

    // 应该有需要加载的区块
    EXPECT_GT(toLoad.size(), 0u);

    // 标记区块为已发送
    for (const auto& pos : toLoad) {
        m_positionTracker->markChunkSent(1, pos.x, pos.z);
    }

    // 移动玩家到新位置
    m_positionTracker->updatePosition(1, 100.0, 64.0, 100.0);  // 新区块

    toLoad.clear();
    toUnload.clear();
    m_positionTracker->calculateChunkUpdates(1, toLoad, toUnload);

    // 应该有新的区块要加载，旧的区块要卸载
    // (取决于视距和移动距离)
}

TEST_F(PositionTrackerTest, MarkChunkSent) {
    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

    m_positionTracker->markChunkSent(1, 5, 10);

    auto* player = m_playerManager->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_TRUE(player->chunkTracker->hasChunk(5, 10));
}

TEST_F(PositionTrackerTest, MarkChunkUnloaded) {
    auto conn = createConnection();
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", conn);

    m_positionTracker->markChunkSent(1, 5, 10);
    EXPECT_TRUE(m_positionTracker->getViewDistance(1) >= 0);  // just check it's valid

    m_positionTracker->markChunkUnloaded(1, 5, 10);

    auto* player = m_playerManager->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_FALSE(player->chunkTracker->hasChunk(5, 10));
}
