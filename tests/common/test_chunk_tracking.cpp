#include <gtest/gtest.h>
#include "common/world/chunk/PlayerChunkTracker.hpp"
#include "common/world/chunk/ChunkTrackingManager.hpp"
#include "common/core/Types.hpp"
#include <tuple>

using namespace mc;
using namespace mc::world;

// ============================================================================
// PlayerChunkTracker 测试
// ============================================================================

class PlayerChunkTrackerTest : public ::testing::Test {
protected:
    PlayerChunkTracker tracker{10};  // 默认视距 10
};

TEST_F(PlayerChunkTrackerTest, InitialState) {
    EXPECT_FALSE(tracker.hasPosition());
    EXPECT_EQ(tracker.viewDistance(), 10);
    EXPECT_EQ(tracker.playerX(), 0);
    EXPECT_EQ(tracker.playerZ(), 0);
    EXPECT_TRUE(tracker.chunksInRange().empty());
}

TEST_F(PlayerChunkTrackerTest, SetPlayerPosition) {
    tracker.setPlayerPosition(5, 10);

    EXPECT_TRUE(tracker.hasPosition());
    EXPECT_EQ(tracker.playerX(), 5);
    EXPECT_EQ(tracker.playerZ(), 10);

    // 检查视距范围内的区块数量
    // 视距 10 意味着 21x21 的区块范围
    EXPECT_EQ(tracker.chunksInRange().size(), 21u * 21u);
}

TEST_F(PlayerChunkTrackerTest, IsChunkInRange) {
    tracker.setPlayerPosition(0, 0);

    // 在视距范围内
    EXPECT_TRUE(tracker.isChunkInRange(0, 0));
    EXPECT_TRUE(tracker.isChunkInRange(10, 0));
    EXPECT_TRUE(tracker.isChunkInRange(0, 10));
    EXPECT_TRUE(tracker.isChunkInRange(-10, -10));

    // 在视距范围外
    EXPECT_FALSE(tracker.isChunkInRange(11, 0));
    EXPECT_FALSE(tracker.isChunkInRange(0, 11));
    EXPECT_FALSE(tracker.isChunkInRange(11, 11));
}

TEST_F(PlayerChunkTrackerTest, SetViewDistance) {
    tracker.setPlayerPosition(0, 0);
    EXPECT_EQ(tracker.chunksInRange().size(), 21u * 21u);

    tracker.setViewDistance(5);
    EXPECT_EQ(tracker.viewDistance(), 5);
    EXPECT_EQ(tracker.chunksInRange().size(), 11u * 11u);

    tracker.setViewDistance(15);
    EXPECT_EQ(tracker.viewDistance(), 15);
    EXPECT_EQ(tracker.chunksInRange().size(), 31u * 31u);
}

TEST_F(PlayerChunkTrackerTest, ViewDistanceClamp) {
    PlayerChunkTracker t1{1};   // 低于最小值
    EXPECT_EQ(t1.viewDistance(), 2);

    PlayerChunkTracker t2{50};  // 高于最大值
    EXPECT_EQ(t2.viewDistance(), 32);
}

TEST_F(PlayerChunkTrackerTest, PositionChangeCallback) {
    std::vector<std::tuple<ChunkCoord, ChunkCoord, bool>> entered;
    std::vector<std::tuple<ChunkCoord, ChunkCoord, bool>> left;

    tracker.setPlayerPosition(0, 0,
        [&](ChunkCoord x, ChunkCoord z, bool isTracking) {
            entered.emplace_back(x, z, isTracking);
        },
        [&](ChunkCoord x, ChunkCoord z, bool isTracking) {
            left.emplace_back(x, z, isTracking);
        });

    EXPECT_FALSE(entered.empty());
    EXPECT_TRUE(left.empty());

    entered.clear();
    left.clear();

    // 移动到新位置
    tracker.setPlayerPosition(20, 0,
        [&](ChunkCoord x, ChunkCoord z, bool isTracking) {
            entered.emplace_back(x, z, isTracking);
        },
        [&](ChunkCoord x, ChunkCoord z, bool isTracking) {
            left.emplace_back(x, z, isTracking);
        });

    EXPECT_FALSE(entered.empty());
    EXPECT_FALSE(left.empty());
}

TEST_F(PlayerChunkTrackerTest, ViewDistanceChangeCallback) {
    tracker.setPlayerPosition(0, 0);

    std::vector<std::tuple<ChunkCoord, ChunkCoord, bool>> entered;
    std::vector<std::tuple<ChunkCoord, ChunkCoord, bool>> left;

    // 增大视距
    tracker.setViewDistance(15,
        [&](ChunkCoord x, ChunkCoord z, bool isTracking) {
            entered.emplace_back(x, z, isTracking);
        },
        [&](ChunkCoord x, ChunkCoord z, bool isTracking) {
            left.emplace_back(x, z, isTracking);
        });

    EXPECT_FALSE(entered.empty());
    EXPECT_TRUE(left.empty());

    entered.clear();
    left.clear();

    // 减小视距
    tracker.setViewDistance(5,
        [&](ChunkCoord x, ChunkCoord z, bool isTracking) {
            entered.emplace_back(x, z, isTracking);
        },
        [&](ChunkCoord x, ChunkCoord z, bool isTracking) {
            left.emplace_back(x, z, isTracking);
        });

    EXPECT_TRUE(entered.empty());
    EXPECT_FALSE(left.empty());
}

TEST_F(PlayerChunkTrackerTest, Clear) {
    tracker.setPlayerPosition(0, 0);
    EXPECT_FALSE(tracker.chunksInRange().empty());

    std::vector<std::tuple<ChunkCoord, ChunkCoord, bool>> left;
    tracker.clear([&](ChunkCoord x, ChunkCoord z, bool isTracking) {
        left.emplace_back(x, z, isTracking);
    });

    EXPECT_TRUE(tracker.chunksInRange().empty());
    EXPECT_FALSE(tracker.hasPosition());
    EXPECT_FALSE(left.empty());
}

TEST_F(PlayerChunkTrackerTest, GetDistanceToPlayer) {
    tracker.setPlayerPosition(0, 0);

    EXPECT_EQ(tracker.getDistanceToPlayer(0, 0), 0);
    EXPECT_EQ(tracker.getDistanceToPlayer(5, 0), 5);
    EXPECT_EQ(tracker.getDistanceToPlayer(0, 5), 5);
    EXPECT_EQ(tracker.getDistanceToPlayer(3, 4), 4);  // 切比雪夫距离
    EXPECT_EQ(tracker.getDistanceToPlayer(10, 10), 10);
    EXPECT_EQ(tracker.getDistanceToPlayer(11, 0), -1);  // 超出视距
}

TEST_F(PlayerChunkTrackerTest, SamePositionNoChange) {
    tracker.setPlayerPosition(5, 5);

    size_t chunksBefore = tracker.chunksInRange().size();

    // 再次设置相同位置
    int callbackCount = 0;
    tracker.setPlayerPosition(5, 5,
        [&](ChunkCoord, ChunkCoord, bool) { ++callbackCount; },
        [&](ChunkCoord, ChunkCoord, bool) { ++callbackCount; });

    EXPECT_EQ(tracker.chunksInRange().size(), chunksBefore);
    EXPECT_EQ(callbackCount, 0);
}

TEST_F(PlayerChunkTrackerTest, NegativeCoordinates) {
    tracker.setPlayerPosition(-100, -200);

    EXPECT_TRUE(tracker.isChunkInRange(-100, -200));
    EXPECT_TRUE(tracker.isChunkInRange(-110, -200));
    EXPECT_FALSE(tracker.isChunkInRange(-111, -200));
}

// ============================================================================
// ChunkTrackingManager 测试
// ============================================================================

class ChunkTrackingManagerTest : public ::testing::Test {
protected:
    ChunkTrackingManager manager;
};

TEST_F(ChunkTrackingManagerTest, InitialState) {
    EXPECT_EQ(manager.playerCount(), 0u);
    EXPECT_FALSE(manager.hasPlayer(1));
}

TEST_F(ChunkTrackingManagerTest, AddPlayer) {
    manager.updatePlayerPosition(1, 0, 0);

    EXPECT_EQ(manager.playerCount(), 1u);
    EXPECT_TRUE(manager.hasPlayer(1));
    EXPECT_TRUE(manager.isPlayerTracking(1, 0, 0));
    EXPECT_TRUE(manager.isPlayerTracking(1, 5, 5));
    EXPECT_FALSE(manager.isPlayerTracking(1, 20, 0));
}

TEST_F(ChunkTrackingManagerTest, RemovePlayer) {
    manager.updatePlayerPosition(1, 0, 0);
    EXPECT_EQ(manager.playerCount(), 1u);

    manager.removePlayer(1);
    EXPECT_EQ(manager.playerCount(), 0u);
    EXPECT_FALSE(manager.hasPlayer(1));
    EXPECT_FALSE(manager.isPlayerTracking(1, 0, 0));
}

TEST_F(ChunkTrackingManagerTest, MultiplePlayers) {
    manager.updatePlayerPosition(1, 0, 0);
    manager.updatePlayerPosition(2, 10, 0);
    manager.updatePlayerPosition(3, 20, 0);

    EXPECT_EQ(manager.playerCount(), 3u);

    // 玩家 1 和 2 有重叠区域
    auto players5_0 = manager.getTrackingPlayers(5, 0);
    EXPECT_EQ(players5_0.size(), 2u);

    // 区块 (20, 0) 被玩家 2 和玩家 3 追踪
    // 玩家 2 在位置 10，视距 10，能看到 x 范围 0-20
    // 玩家 3 在位置 20，视距 10，能看到 x 范围 10-30
    auto players20_0 = manager.getTrackingPlayers(20, 0);
    EXPECT_EQ(players20_0.size(), 2u);

    // 区块 (25, 0) 只被玩家 3 追踪
    auto players25_0 = manager.getTrackingPlayers(25, 0);
    EXPECT_EQ(players25_0.size(), 1u);
    EXPECT_EQ(players25_0[0], 3);

    // 区块 (-5, 0) 只被玩家 1 追踪
    auto playersNeg5_0 = manager.getTrackingPlayers(-5, 0);
    EXPECT_EQ(playersNeg5_0.size(), 1u);
    EXPECT_EQ(playersNeg5_0[0], 1);
}

TEST_F(ChunkTrackingManagerTest, TrackingChangeCallback) {
    std::vector<std::tuple<PlayerId, ChunkCoord, ChunkCoord, bool>> changes;

    manager.setTrackingChangeCallback(
        [&](PlayerId player, ChunkCoord x, ChunkCoord z, bool isTracking) {
            changes.emplace_back(player, x, z, isTracking);
        });

    manager.updatePlayerPosition(1, 0, 0);

    // 视距 10，21x21 区块
    EXPECT_EQ(changes.size(), 21u * 21u);
    for (const auto& change : changes) {
        EXPECT_EQ(std::get<0>(change), 1);
        EXPECT_TRUE(std::get<3>(change));
    }
}

TEST_F(ChunkTrackingManagerTest, SetPlayerViewDistance) {
    manager.updatePlayerPosition(1, 0, 0);

    auto players = manager.getTrackingPlayers(0, 0);
    EXPECT_EQ(players.size(), 1u);

    // 减小视距
    manager.setPlayerViewDistance(1, 5);

    const PlayerChunkTracker* tracker = manager.getPlayerTracker(1);
    ASSERT_NE(tracker, nullptr);
    EXPECT_EQ(tracker->viewDistance(), 5);
}

TEST_F(ChunkTrackingManagerTest, HasTrackingPlayers) {
    EXPECT_FALSE(manager.hasTrackingPlayers(
        ChunkTrackingManager::posToKey(0, 0)));

    manager.updatePlayerPosition(1, 0, 0);

    EXPECT_TRUE(manager.hasTrackingPlayers(
        ChunkTrackingManager::posToKey(0, 0)));
    EXPECT_TRUE(manager.hasTrackingPlayers(
        ChunkTrackingManager::posToKey(5, 5)));
    EXPECT_FALSE(manager.hasTrackingPlayers(
        ChunkTrackingManager::posToKey(20, 20)));
}

TEST_F(ChunkTrackingManagerTest, DefaultViewDistance) {
    EXPECT_EQ(manager.defaultViewDistance(), 10);

    manager.setDefaultViewDistance(15);
    EXPECT_EQ(manager.defaultViewDistance(), 15);

    // 新玩家使用默认视距
    manager.updatePlayerPosition(1, 0, 0);
    const PlayerChunkTracker* tracker = manager.getPlayerTracker(1);
    ASSERT_NE(tracker, nullptr);
    EXPECT_EQ(tracker->viewDistance(), 15);
}

TEST_F(ChunkTrackingManagerTest, PlayerMovementTriggersCallbacks) {
    std::vector<std::tuple<PlayerId, ChunkCoord, ChunkCoord, bool>> enters;
    std::vector<std::tuple<PlayerId, ChunkCoord, ChunkCoord, bool>> leaves;

    manager.setTrackingChangeCallback(
        [&](PlayerId player, ChunkCoord x, ChunkCoord z, bool isTracking) {
            if (isTracking) {
                enters.emplace_back(player, x, z, isTracking);
            } else {
                leaves.emplace_back(player, x, z, isTracking);
            }
        });

    manager.updatePlayerPosition(1, 0, 0);
    EXPECT_FALSE(enters.empty());
    enters.clear();
    leaves.clear();

    // 移动到新位置
    manager.updatePlayerPosition(1, 20, 0);

    EXPECT_FALSE(enters.empty());  // 新区块
    EXPECT_FALSE(leaves.empty());  // 离开旧区块
}

TEST_F(ChunkTrackingManagerTest, RemovePlayerClearsTracking) {
    manager.updatePlayerPosition(1, 0, 0);
    manager.updatePlayerPosition(2, 0, 0);

    // 区块 (0, 0) 被两个玩家追踪
    EXPECT_TRUE(manager.hasTrackingPlayers(
        ChunkTrackingManager::posToKey(0, 0)));
    auto players = manager.getTrackingPlayers(0, 0);
    EXPECT_EQ(players.size(), 2u);

    // 移除玩家 1
    manager.removePlayer(1);

    // 区块 (0, 0) 仍被玩家 2 追踪
    EXPECT_TRUE(manager.hasTrackingPlayers(
        ChunkTrackingManager::posToKey(0, 0)));
    players = manager.getTrackingPlayers(0, 0);
    EXPECT_EQ(players.size(), 1u);
    EXPECT_EQ(players[0], 2);
}
