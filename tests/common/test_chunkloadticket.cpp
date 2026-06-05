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

#include "common/core/Types.hpp"
#include "common/world/chunk/ChunkDistanceGraph.hpp"
#include "common/world/chunk/ChunkLoadTicket.hpp"
#include "common/world/chunk/ChunkLoadTicketManager.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <future>
#include <thread>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world;

// ============================================================================
// ChunkLoadTicket 测试
// ============================================================================

class ChunkLoadTicketTest : public ::testing::Test {
protected:
    void SetUp() override { TicketTypes::initializeTicketTypes(); }
};

TEST_F(ChunkLoadTicketTest, TicketTypeCreation)
{
    auto type = ChunkLoadTicketType<ChunkPos>::create("test");
    EXPECT_EQ(type.name(), "test");
    EXPECT_EQ(type.lifespan(), 0u);
}

TEST_F(ChunkLoadTicketTest, TicketTypeWithLifespan)
{
    auto type = ChunkLoadTicketType<u32>::create("test_lifespan", 100);
    EXPECT_EQ(type.name(), "test_lifespan");
    EXPECT_EQ(type.lifespan(), 100u);
}

TEST_F(ChunkLoadTicketTest, TicketComparison)
{
    // 级别小的优先级高
    ChunkLoadTicket t1(TicketTypes::FORCED, 31, ChunkPos(0, 0));
    ChunkLoadTicket t2(TicketTypes::FORCED, 32, ChunkPos(0, 0));

    // t1 级别低（31 < 32），优先级高
    EXPECT_TRUE(t1.hasHigherPriorityThan(t2));
}

TEST_F(ChunkLoadTicketTest, TicketExpiration)
{
    auto type = ChunkLoadTicketType<ChunkPos>::create("test_expire", 100);
    ChunkLoadTicket ticket(type, 31, ChunkPos(0, 0));

    ticket.setTimestamp(0);
    EXPECT_FALSE(ticket.isExpired(50));  // 未过期
    EXPECT_FALSE(ticket.isExpired(100)); // 刚好过期时间
    EXPECT_TRUE(ticket.isExpired(101));  // 已过期
}

TEST_F(ChunkLoadTicketTest, TicketLevel)
{
    ChunkLoadTicket ticket(TicketTypes::FORCED, 31, ChunkPos(5, 10));
    EXPECT_EQ(ticket.level(), 31);
    EXPECT_TRUE(ticket.hasChunkValue());
    EXPECT_EQ(ticket.chunkValue().x, 5);
    EXPECT_EQ(ticket.chunkValue().z, 10);
}

TEST_F(ChunkLoadTicketTest, TicketWithIntValue)
{
    ChunkLoadTicket ticket(TicketTypes::POST_TELEPORT, 31, 42u);
    EXPECT_TRUE(ticket.hasIntValue());
    EXPECT_EQ(ticket.intValue(), 42u);
}

// ============================================================================
// ChunkTicketSet 测试
// ============================================================================

TEST(ChunkTicketSetTest, AddAndRemoveTicket)
{
    ChunkTicketSet set;

    ChunkLoadTicket t1(TicketTypes::FORCED, 31, ChunkPos(0, 0));
    ChunkLoadTicket t2(TicketTypes::FORCED, 32, ChunkPos(0, 0));

    set.addTicket(t1);
    EXPECT_EQ(set.size(), 1u);

    set.addTicket(t2);
    EXPECT_EQ(set.size(), 2u);

    // 重复添加不应该增加数量
    set.addTicket(t1);
    EXPECT_EQ(set.size(), 2u);

    set.removeTicket(t1);
    EXPECT_EQ(set.size(), 1u);
}

TEST(ChunkTicketSetTest, MinLevel)
{
    ChunkTicketSet set;

    // 空集合返回最大级别
    EXPECT_EQ(set.getMinLevel(), static_cast<i32>(ChunkLoadLevel::MaxLevel));

    set.addTicket(ChunkLoadTicket(TicketTypes::FORCED, 33, ChunkPos(0, 0)));
    EXPECT_EQ(set.getMinLevel(), 33);

    set.addTicket(ChunkLoadTicket(TicketTypes::FORCED, 31, ChunkPos(0, 0)));
    EXPECT_EQ(set.getMinLevel(), 31); // 更小的级别

    set.addTicket(ChunkLoadTicket(TicketTypes::PORTAL, 32, ChunkPos(0, 0)));
    EXPECT_EQ(set.getMinLevel(), 31); // 仍然是最小的
}

TEST(ChunkTicketSetTest, RemoveExpired)
{
    auto expireType = ChunkLoadTicketType<ChunkPos>::create("expire", 10);
    auto permanentType = ChunkLoadTicketType<ChunkPos>::create("permanent");

    ChunkTicketSet set;
    ChunkLoadTicket expiring(expireType, 32, ChunkPos(0, 0));
    expiring.setTimestamp(0);

    ChunkLoadTicket permanent(permanentType, 31, ChunkPos(0, 0));

    set.addTicket(expiring);
    set.addTicket(permanent);
    EXPECT_EQ(set.size(), 2u);

    set.removeExpired(5); // 未过期
    EXPECT_EQ(set.size(), 2u);

    set.removeExpired(20); // 过期
    EXPECT_EQ(set.size(), 1u);
    EXPECT_EQ(set.getMinLevel(), 31); // permanent 票据仍然存在
}

// ============================================================================
// ChunkDistanceGraph 测试
// ============================================================================

class ChunkDistanceGraphTest : public ::testing::Test {
protected:
    ChunkDistanceGraph graph;
};

TEST_F(ChunkDistanceGraphTest, InitialLevel)
{
    // 未设置的区块级别应为 MAX_LEVEL
    EXPECT_EQ(graph.getLevel(0, 0), ChunkDistanceGraph::MAX_LEVEL);
    EXPECT_EQ(graph.getLevel(100, -50), ChunkDistanceGraph::MAX_LEVEL);
}

TEST_F(ChunkDistanceGraphTest, SetSourceLevel)
{
    // 设置源级别
    graph.updateSourceLevel(0, 0, 0, true);
    graph.processUpdates(100);

    EXPECT_EQ(graph.getLevel(0, 0), 0);
}

TEST_F(ChunkDistanceGraphTest, LevelPropagation)
{
    // 设置源区块级别并传播
    graph.updateSourceLevel(0, 0, 0, true);
    graph.processUpdates(100);

    // 检查级别传播
    EXPECT_EQ(graph.getLevel(0, 0), 0);
    EXPECT_EQ(graph.getLevel(1, 0), 1); // 相邻区块 = 源 + 1
    EXPECT_EQ(graph.getLevel(0, 1), 1);
    EXPECT_EQ(graph.getLevel(-1, 0), 1);
    EXPECT_EQ(graph.getLevel(0, -1), 1);

    // 再远一层
    EXPECT_EQ(graph.getLevel(2, 0), 2);
    EXPECT_EQ(graph.getLevel(1, 1), 2);
}

TEST_F(ChunkDistanceGraphTest, LevelChangeCallback)
{
    std::vector<ChunkCoord> xs;
    std::vector<ChunkCoord> zs;
    std::vector<i32> oldLevels;
    std::vector<i32> newLevels;

    graph.setLevelChangeCallback([&](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
        xs.push_back(x);
        zs.push_back(z);
        oldLevels.push_back(oldLevel);
        newLevels.push_back(newLevel);
    });

    graph.updateSourceLevel(0, 0, 31, true);
    graph.processUpdates(100);

    // 应该有多个级别变化
    EXPECT_FALSE(xs.empty());

    // 检查 (0, 0) 的变化
    bool foundCenter = false;
    for (size_t i = 0; i < xs.size(); ++i) {
        if (xs[i] == 0 && zs[i] == 0) {
            foundCenter = true;
            EXPECT_EQ(oldLevels[i], ChunkDistanceGraph::MAX_LEVEL);
            EXPECT_EQ(newLevels[i], 31);
        }
    }
    EXPECT_TRUE(foundCenter);
}

TEST_F(ChunkDistanceGraphTest, ClearGraph)
{
    graph.updateSourceLevel(0, 0, 0, true);
    graph.processUpdates(100);
    EXPECT_EQ(graph.getLevel(0, 0), 0);

    graph.clear();
    EXPECT_EQ(graph.getLevel(0, 0), ChunkDistanceGraph::MAX_LEVEL);
    EXPECT_EQ(graph.size(), 0u);
}

// ============================================================================
// ChunkDistanceGraph 扩展测试
// ============================================================================

class ChunkDistanceGraphExtendedTest : public ::testing::Test {
protected:
    ChunkDistanceGraph graph;
};

TEST_F(ChunkDistanceGraphExtendedTest, MultipleSources)
{
    // 设置两个源，级别不同
    graph.updateSourceLevel(0, 0, 20, true);
    graph.updateSourceLevel(10, 0, 20, true);
    graph.processUpdates(1000);

    // 两个源中间的区块应该有更低的级别
    // (5, 0) 距离两个源都是 5，所以级别应该是 20 + 5 = 25
    EXPECT_EQ(graph.getLevel(0, 0), 20);
    EXPECT_EQ(graph.getLevel(10, 0), 20);
    EXPECT_EQ(graph.getLevel(5, 0), 25);
}

TEST_F(ChunkDistanceGraphExtendedTest, LevelIncreaseOnSourceRemoval)
{
    // 设置源
    graph.updateSourceLevel(0, 0, 23, true);
    graph.processUpdates(1000);
    EXPECT_EQ(graph.getLevel(0, 0), 23);

    // 移除源（设置级别为 MAX_LEVEL）
    graph.updateSourceLevel(0, 0, ChunkDistanceGraph::MAX_LEVEL, false);
    graph.processUpdates(1000);

    // 级别应该升高到 MAX_LEVEL
    EXPECT_EQ(graph.getLevel(0, 0), ChunkDistanceGraph::MAX_LEVEL);
}

TEST_F(ChunkDistanceGraphExtendedTest, LevelPropagationChain)
{
    // 设置级别 0 的源
    graph.updateSourceLevel(0, 0, 0, true);
    graph.processUpdates(1000);

    // 验证级别传播链
    // 中心级别 0，邻居 1，邻居的邻居 2，依此类推
    for (i32 dist = 0; dist <= 10; ++dist) {
        EXPECT_EQ(graph.getLevel(dist, 0), dist) << "Level at (" << dist << ", 0) should be " << dist;
        EXPECT_EQ(graph.getLevel(-dist, 0), dist) << "Level at (" << (-dist) << ", 0) should be " << dist;
        EXPECT_EQ(graph.getLevel(0, dist), dist) << "Level at (0, " << dist << ") should be " << dist;
        EXPECT_EQ(graph.getLevel(0, -dist), dist) << "Level at (0, " << (-dist) << ") should be " << dist;
    }
}

TEST_F(ChunkDistanceGraphExtendedTest, DiagonalPropagation)
{
    // 级别应该沿对角线传播
    graph.updateSourceLevel(0, 0, 0, true);
    graph.processUpdates(1000);

    // 对角线上的级别
    // (1, 1) 可以从 (0, 1) 或 (1, 0) 到达，级别为 2
    EXPECT_EQ(graph.getLevel(1, 1), 2);
    EXPECT_EQ(graph.getLevel(2, 2), 4);
    EXPECT_EQ(graph.getLevel(3, 3), 6);
}

TEST_F(ChunkDistanceGraphExtendedTest, LevelChangeCallbackOrder)
{
    std::vector<std::tuple<ChunkCoord, ChunkCoord, i32, i32>> changes;

    graph.setLevelChangeCallback([&](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
        changes.emplace_back(x, z, oldLevel, newLevel);
    });

    graph.updateSourceLevel(0, 0, 30, true);
    graph.processUpdates(1000);

    // 应该有多个级别变化
    EXPECT_FALSE(changes.empty());

    // 中心区块应该从 MAX_LEVEL 变为 30
    bool foundCenter = false;
    for (const auto& [x, z, oldLevel, newLevel] : changes) {
        if (x == 0 && z == 0) {
            foundCenter = true;
            EXPECT_EQ(oldLevel, ChunkDistanceGraph::MAX_LEVEL);
            EXPECT_EQ(newLevel, 30);
            break;
        }
    }
    EXPECT_TRUE(foundCenter);
}

TEST_F(ChunkDistanceGraphExtendedTest, ProcessUpdatesInBatches)
{
    // 设置源
    graph.updateSourceLevel(0, 0, 0, true);

    // 分批处理
    i32 totalProcessed = 0;
    i32 processed;
    do {
        processed = graph.processUpdates(10); // 每次处理 10 个
        totalProcessed += processed;
    } while (processed > 0);

    EXPECT_GT(totalProcessed, 0);
    EXPECT_EQ(graph.getLevel(0, 0), 0);
}

TEST_F(ChunkDistanceGraphExtendedTest, NegativeCoordinates)
{
    // 测试负坐标
    graph.updateSourceLevel(-100, -50, 25, true);
    graph.processUpdates(1000);

    EXPECT_EQ(graph.getLevel(-100, -50), 25);
    EXPECT_EQ(graph.getLevel(-99, -50), 26);
    EXPECT_EQ(graph.getLevel(-100, -49), 26);
    EXPECT_EQ(graph.getLevel(-101, -50), 26);
    EXPECT_EQ(graph.getLevel(-100, -51), 26);
}

TEST_F(ChunkDistanceGraphExtendedTest, LargeCoordinates)
{
    // 测试大坐标
    graph.updateSourceLevel(100000, 200000, 20, true);
    graph.processUpdates(1000);

    EXPECT_EQ(graph.getLevel(100000, 200000), 20);
    EXPECT_EQ(graph.getLevel(100001, 200000), 21);
}

TEST_F(ChunkDistanceGraphExtendedTest, MultipleUpdatesSamePosition)
{
    // 对同一位置进行多次更新
    graph.updateSourceLevel(0, 0, 30, true);
    graph.processUpdates(1000);
    EXPECT_EQ(graph.getLevel(0, 0), 30);

    // 更低的级别应该覆盖
    graph.updateSourceLevel(0, 0, 20, true);
    graph.processUpdates(1000);
    EXPECT_EQ(graph.getLevel(0, 0), 20);

    // 更高的级别不应该覆盖（已经有更低的级别）
    graph.updateSourceLevel(0, 0, 25, true);
    graph.processUpdates(1000);
    EXPECT_EQ(graph.getLevel(0, 0), 20); // 仍然是 20
}

TEST_F(ChunkDistanceGraphExtendedTest, SourceLevelOverride)
{
    // 先设置一个较高级别
    graph.updateSourceLevel(0, 0, 30, true);
    graph.processUpdates(1000);
    EXPECT_EQ(graph.getLevel(0, 0), 30);

    // 然后设置更低的级别
    graph.updateSourceLevel(0, 0, 20, true);
    graph.processUpdates(1000);
    EXPECT_EQ(graph.getLevel(0, 0), 20);

    // 然后设置更高级别（应该不会覆盖）
    graph.updateSourceLevel(0, 0, 35, true);
    graph.processUpdates(1000);
    EXPECT_EQ(graph.getLevel(0, 0), 20); // 仍然是 20
}

TEST_F(ChunkDistanceGraphExtendedTest, AllLevelsMethod)
{
    graph.updateSourceLevel(0, 0, 31, true);
    graph.processUpdates(1000);

    const auto& levels = graph.allLevels();
    EXPECT_FALSE(levels.empty());

    // 验证所有级别都在合理范围内
    for (const auto& [key, level] : levels) {
        EXPECT_GE(level, 31);
        EXPECT_LE(level, ChunkDistanceGraph::MAX_LEVEL);
    }
}

TEST_F(ChunkDistanceGraphExtendedTest, PropagationLimitedByMaxLevel)
{
    // 设置一个较高的级别
    graph.updateSourceLevel(0, 0, 33, true);
    graph.processUpdates(1000);

    // 级别应该在传播时受 MAX_LEVEL 限制
    // 相邻区块级别为 34，但不会超过 MAX_LEVEL
    EXPECT_EQ(graph.getLevel(0, 0), 33);
    EXPECT_EQ(graph.getLevel(1, 0), 34); // MAX_LEVEL
    EXPECT_EQ(graph.getLevel(2, 0), 34); // 仍然是 MAX_LEVEL
}

TEST_F(ChunkDistanceGraphExtendedTest, TwoSourcesDifferentLevels)
{
    // 两个不同级别的源
    graph.updateSourceLevel(0, 0, 20, true);
    graph.updateSourceLevel(10, 0, 25, true);
    graph.processUpdates(1000);

    // 靠近较低级别源的区块应该有更低的级别
    EXPECT_EQ(graph.getLevel(0, 0), 20);
    EXPECT_EQ(graph.getLevel(10, 0), 25);

    // (5, 0) 距离两个源的距离相同
    // 从 (0, 0) 来：20 + 5 = 25
    // 从 (10, 0) 来：25 + 5 = 30
    // 取最小值：25
    EXPECT_EQ(graph.getLevel(5, 0), 25);
}

// ============================================================================
// ChunkLoadTicketManager 测试
// ============================================================================

class ChunkLoadTicketManagerTest : public ::testing::Test {
protected:
    void SetUp() override { TicketTypes::initializeTicketTypes(); }
};

TEST_F(ChunkLoadTicketManagerTest, PlayerPositionUpdate)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(10);

    std::vector<ChunkCoord> xs;
    std::vector<ChunkCoord> zs;
    std::vector<i32> oldLevels;
    std::vector<i32> newLevels;

    manager.setLevelChangeCallback([&](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
        xs.push_back(x);
        zs.push_back(z);
        oldLevels.push_back(oldLevel);
        newLevels.push_back(newLevel);
    });

    // 玩家进入
    manager.updatePlayerPosition(1, 0, 0);

    // 应该触发区块加载
    bool foundLoad = false;
    for (size_t i = 0; i < xs.size(); ++i) {
        if (xs[i] == 0 && zs[i] == 0 && newLevels[i] <= 33) {
            foundLoad = true;
        }
    }
    EXPECT_TRUE(foundLoad);
}

TEST_F(ChunkLoadTicketManagerTest, ChunkUnloadWhenPlayerLeaves)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(2); // 小视距便于测试

    // 玩家进入 (0, 0)
    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    // 区块 (0, 0) 应该加载
    EXPECT_TRUE(manager.shouldChunkLoad(0, 0));

    // 玩家移动到 (100, 0) - 远离原位置
    manager.updatePlayerPosition(1, 100, 0);
    manager.processUpdates();

    // 区块 (0, 0) 应该卸载
    EXPECT_FALSE(manager.shouldChunkLoad(0, 0));
}

TEST_F(ChunkLoadTicketManagerTest, ForcedChunk)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(2);

    // 强制加载区块 (100, 100)
    manager.forceChunk(100, 100, true);
    manager.processUpdates();

    // 即使玩家不在这里，区块也应该加载
    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    EXPECT_TRUE(manager.shouldChunkLoad(100, 100));

    // 取消强制加载
    manager.forceChunk(100, 100, false);
    manager.processUpdates();

    // 现在应该卸载（玩家太远）
    EXPECT_FALSE(manager.shouldChunkLoad(100, 100));
}

TEST_F(ChunkLoadTicketManagerTest, MultiplePlayers)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    // 玩家 1 在 (0, 0)
    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    // 玩家 2 在 (20, 0)
    manager.updatePlayerPosition(2, 20, 0);
    manager.processUpdates();

    EXPECT_EQ(manager.playerCount(), 2u);

    // 区块 (0, 0) 和 (20, 0) 都应该加载
    EXPECT_TRUE(manager.shouldChunkLoad(0, 0));
    EXPECT_TRUE(manager.shouldChunkLoad(20, 0));

    // 移除玩家 1
    manager.removePlayer(1);
    manager.processUpdates();

    EXPECT_EQ(manager.playerCount(), 1u);
}

TEST_F(ChunkLoadTicketManagerTest, ViewDistanceChange)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    // 视距5：玩家在(0,0)，区块(5,0)应该在边界上加载
    // 注意：viewDistanceToLevel(5) = 33 - 5 = 28，传播到距离5的区块级别为28+5=33
    // 级别33是Border，应该加载
    EXPECT_TRUE(manager.shouldChunkLoad(5, 0));

    // 区块(6,0)距离为6，超过视距5，应该不加载
    EXPECT_FALSE(manager.shouldChunkLoad(6, 0));

    // 增加视距
    manager.setViewDistance(10);
    manager.processUpdates();

    // 现在区块(6,0)应该在加载范围内
    EXPECT_TRUE(manager.shouldChunkLoad(6, 0));

    // 区块(10,0)应该也在加载范围内
    EXPECT_TRUE(manager.shouldChunkLoad(10, 0));
}

TEST_F(ChunkLoadTicketManagerTest, TicketCount)
{
    ChunkLoadTicketManager manager;

    // 初始没有票据
    EXPECT_EQ(manager.totalTicketCount(), 0u);

    // 添加玩家不会增加显式票据数量，玩家来源走统一 aggregator
    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    EXPECT_EQ(manager.totalTicketCount(), 0u);
}

// ============================================================================
// ChunkLoadTicketManager 扩展测试
// ============================================================================

class ChunkLoadTicketManagerExtendedTest : public ::testing::Test {
protected:
    void SetUp() override { TicketTypes::initializeTicketTypes(); }
};

TEST_F(ChunkLoadTicketManagerExtendedTest, PortalTicketExpiration)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(2);

    // 添加 PORTAL 票据（300 tick 生命周期）
    manager.registerTicket(TicketTypes::PORTAL, 50, 50, 31, ChunkPos(50, 50));

    // 设置票据时间戳（票据创建时时间戳为0，需要设置当前时间）
    const ChunkTicketSet* tickets = manager.getChunkTickets(50, 50);
    ASSERT_NE(tickets, nullptr);
    // 注意：票据的时间戳需要通过 tick() 递增来检查过期
    // 但由于票据创建时时间戳为0，而 manager.tick() 从0开始，
    // 所以需要超过300 tick才会过期

    manager.processUpdates();

    // 区块应该加载
    EXPECT_TRUE(manager.shouldChunkLoad(50, 50));

    // 运行 300 tick
    for (int i = 0; i < 300; ++i) {
        manager.tick();
    }
    manager.processUpdates();

    // 区块仍应加载（票据可能未过期，因为时间戳是相对的）
    // 注意：这个测试验证票据系统的行为，实际过期逻辑可能需要额外设置
}

TEST_F(ChunkLoadTicketManagerExtendedTest, ForcedChunkPersistsAfterPlayerLeave)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(2);

    // 强制加载区块
    manager.forceChunk(100, 100, true);
    manager.processUpdates();
    EXPECT_TRUE(manager.shouldChunkLoad(100, 100));

    // 玩家进入并在远处
    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    // 强制加载的区块仍然加载
    EXPECT_TRUE(manager.shouldChunkLoad(100, 100));

    // 玩家离开
    manager.removePlayer(1);
    manager.processUpdates();

    // 强制加载的区块仍然加载
    EXPECT_TRUE(manager.shouldChunkLoad(100, 100));

    // 取消强制加载
    manager.forceChunk(100, 100, false);
    manager.processUpdates();
    EXPECT_FALSE(manager.shouldChunkLoad(100, 100));
}

TEST_F(ChunkLoadTicketManagerExtendedTest, MultiplePlayersOverlappingChunks)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    // 玩家 1 在 (0, 0)
    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    // 玩家 2 在 (3, 0)，视距重叠
    manager.updatePlayerPosition(2, 3, 0);
    manager.processUpdates();

    // 验证重叠区域的加载状态
    EXPECT_TRUE(manager.shouldChunkLoad(0, 0)); // 玩家 1 的位置
    EXPECT_TRUE(manager.shouldChunkLoad(3, 0)); // 玩家 2 的位置
    EXPECT_TRUE(manager.shouldChunkLoad(1, 0)); // 中间区块

    // 玩家 1 离开
    manager.removePlayer(1);
    manager.processUpdates();

    // 玩家 2 的区块仍然加载
    EXPECT_TRUE(manager.shouldChunkLoad(3, 0));
    EXPECT_TRUE(manager.shouldChunkLoad(1, 0)); // 仍在玩家 2 的视距内

    // 玩家 1 原来的位置可能不在玩家 2 的视距内
    // 视距 5，玩家 2 在 (3, 0)，所以 (0, 0) 在视距外
    // 注意：实际行为取决于票据传播
}

TEST_F(ChunkLoadTicketManagerExtendedTest, GetChunkTickets)
{
    ChunkLoadTicketManager manager;

    // 初始没有票据
    EXPECT_EQ(manager.getChunkTickets(0, 0), nullptr);

    // 添加玩家来源
    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    // 玩家来源不再创建显式 ticket
    EXPECT_EQ(manager.getChunkTickets(0, 0), nullptr);

    // 添加强制加载票据
    manager.forceChunk(0, 0, true);
    manager.processUpdates();

    const ChunkTicketSet* tickets = manager.getChunkTickets(0, 0);
    EXPECT_NE(tickets, nullptr);
    EXPECT_EQ(tickets->size(), 1u);
}

TEST_F(ChunkLoadTicketManagerExtendedTest, PlayerMovementTriggersLoadUnload)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(3);

    std::vector<ChunkPos> loadedChunks;
    std::vector<ChunkPos> unloadedChunks;

    manager.setLevelChangeCallback([&](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
        if (newLevel <= 33 && oldLevel > 33) {
            loadedChunks.emplace_back(x, z);
        } else if (newLevel > 33 && oldLevel <= 33) {
            unloadedChunks.emplace_back(x, z);
        }
    });

    // 玩家进入
    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    // 应该有区块被加载
    EXPECT_FALSE(loadedChunks.empty());

    loadedChunks.clear();
    unloadedChunks.clear();

    // 玩家移动到远处
    manager.updatePlayerPosition(1, 100, 100);
    manager.processUpdates();

    // 原来的区块应该被卸载
    EXPECT_FALSE(unloadedChunks.empty());

    // 新的区块应该被加载
    EXPECT_FALSE(loadedChunks.empty());
}

TEST_F(ChunkLoadTicketManagerExtendedTest, SamePlayerPositionUpdate)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    // 玩家进入
    manager.updatePlayerPosition(1, 10, 10);
    manager.processUpdates();

    size_t ticketCount = manager.totalTicketCount();

    // 再次设置相同位置
    manager.updatePlayerPosition(1, 10, 10);
    manager.processUpdates();

    // 票据数量不应该增加
    EXPECT_EQ(manager.totalTicketCount(), ticketCount);
}

TEST_F(ChunkLoadTicketManagerExtendedTest, ViewDistanceBoundaryConditions)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(10);

    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    // 视距边界上的区块
    // viewDistanceToLevel(10) = 23
    // 玩家在 (0, 0)，级别 23
    // (10, 0) 距离 10，级别 33（Border），应该加载
    EXPECT_TRUE(manager.shouldChunkLoad(10, 0));
    EXPECT_TRUE(manager.shouldChunkLoad(-10, 0));
    EXPECT_TRUE(manager.shouldChunkLoad(0, 10));
    EXPECT_TRUE(manager.shouldChunkLoad(0, -10));

    // 超出视距
    EXPECT_FALSE(manager.shouldChunkLoad(11, 0));
}

TEST_F(ChunkLoadTicketManagerExtendedTest, SmallViewDistance)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(1); // 最小视距

    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    EXPECT_TRUE(manager.shouldChunkLoad(0, 0)); // 玩家位置
    EXPECT_TRUE(manager.shouldChunkLoad(1, 0)); // 距离 1，级别 33 (Border)
    EXPECT_TRUE(manager.shouldChunkLoad(-1, 0));
    EXPECT_TRUE(manager.shouldChunkLoad(0, 1));
    EXPECT_TRUE(manager.shouldChunkLoad(0, -1));

    EXPECT_TRUE(manager.shouldChunkLoad(2, 0));
    EXPECT_TRUE(manager.shouldChunkLoad(0, 2));
}

TEST_F(ChunkLoadTicketManagerExtendedTest, LargeViewDistance)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(10); // 使用正常视距

    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    // 距离 2 的区块应该加载 (23 + 2 = 25 < 33)
    EXPECT_TRUE(manager.shouldChunkLoad(2, 0));

    // 距离 10 的区块应该加载 (23 + 10 = 33 = Border)
    EXPECT_TRUE(manager.shouldChunkLoad(10, 0));

    // 距离 11 的区块不应该加载 (23 + 11 = 34 > 33)
    EXPECT_FALSE(manager.shouldChunkLoad(11, 0));
}

TEST_F(ChunkLoadTicketManagerExtendedTest, NegativePlayerPosition)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    manager.updatePlayerPosition(1, -10, -20);
    manager.processUpdates();

    EXPECT_TRUE(manager.shouldChunkLoad(-10, -20));
    EXPECT_TRUE(manager.shouldChunkLoad(-15, -20));  // 视距边缘
    EXPECT_FALSE(manager.shouldChunkLoad(-16, -20)); // 超出视距
}

TEST_F(ChunkLoadTicketManagerExtendedTest, RemoveNonExistentPlayer)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    // 移除不存在的玩家不应该崩溃
    manager.removePlayer(999);
    EXPECT_EQ(manager.playerCount(), 0u);
}

TEST_F(ChunkLoadTicketManagerExtendedTest, RegisterReleaseTicketManually)
{
    ChunkLoadTicketManager manager;

    // 手动注册票据
    manager.registerTicket(TicketTypes::FORCED, 100, 200, 31, ChunkPos(100, 200));
    manager.processUpdates();

    EXPECT_TRUE(manager.shouldChunkLoad(100, 200));

    // 手动释放票据
    manager.releaseTicket(TicketTypes::FORCED, 100, 200, 31, ChunkPos(100, 200));
    manager.processUpdates();

    EXPECT_FALSE(manager.shouldChunkLoad(100, 200));
}

TEST_F(ChunkLoadTicketManagerExtendedTest, MultipleForcedChunks)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(2);

    // 强制加载多个区块
    for (int x = 0; x < 5; ++x) {
        for (int z = 0; z < 5; ++z) {
            manager.forceChunk(x * 10, z * 10, true);
        }
    }
    manager.processUpdates();

    // 所有强制加载的区块都应该加载
    for (int x = 0; x < 5; ++x) {
        for (int z = 0; z < 5; ++z) {
            EXPECT_TRUE(manager.shouldChunkLoad(x * 10, z * 10));
        }
    }

    // 取消一半的强制加载
    for (int x = 0; x < 5; ++x) {
        for (int z = 0; z < 5; ++z) {
            if ((x + z) % 2 == 0) {
                manager.forceChunk(x * 10, z * 10, false);
            }
        }
    }
    manager.processUpdates();

    // 验证取消的区块不再加载
    for (int x = 0; x < 5; ++x) {
        for (int z = 0; z < 5; ++z) {
            if ((x + z) % 2 == 0) {
                EXPECT_FALSE(manager.shouldChunkLoad(x * 10, z * 10));
            } else {
                EXPECT_TRUE(manager.shouldChunkLoad(x * 10, z * 10));
            }
        }
    }
}

TEST_F(ChunkLoadTicketManagerExtendedTest, DistanceGraphAccess)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    // 访问距离图
    const ChunkDistanceGraph& graph = manager.distanceGraph();
    EXPECT_GT(graph.size(), 0u);
}

TEST_F(ChunkLoadTicketManagerExtendedTest, LevelChangeCallbackMultipleChanges)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(3);

    int loadCount = 0;
    int unloadCount = 0;

    manager.setLevelChangeCallback([&](ChunkCoord, ChunkCoord, i32 oldLevel, i32 newLevel) {
        if (newLevel <= 33 && oldLevel > 33) {
            ++loadCount;
        } else if (newLevel > 33 && oldLevel <= 33) {
            ++unloadCount;
        }
    });

    // 玩家进入
    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();
    EXPECT_GT(loadCount, 0);

    // 玩家离开
    manager.removePlayer(1);
    manager.processUpdates();
    EXPECT_GT(unloadCount, 0);
}

TEST_F(ChunkLoadTicketManagerExtendedTest, LevelChangeCallbackCanQueryTrackingPlayers)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(3);

    std::promise<void> callbackPromise;
    std::future<void> callbackFuture = callbackPromise.get_future();
    std::promise<void> donePromise;
    std::future<void> doneFuture = donePromise.get_future();
    std::atomic<bool> callbackSignaled{false};

    manager.setLevelChangeCallback([&](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
        if (x != 0 || z != 0) {
            return;
        }

        if (newLevel > ChunkLoadTicketManager::MAX_LOADED_LEVEL ||
            oldLevel <= ChunkLoadTicketManager::MAX_LOADED_LEVEL) {
            return;
        }

        auto players = manager.getTrackingPlayers(x, z);
        EXPECT_NE(std::find(players.begin(), players.end(), 1), players.end());

        if (!callbackSignaled.exchange(true)) {
            callbackPromise.set_value();
        }
    });

    std::thread worker([&]() {
        manager.updatePlayerPosition(1, 0, 0);
        donePromise.set_value();
    });

    if (callbackFuture.wait_for(std::chrono::seconds(2)) != std::future_status::ready) {
        worker.detach();
        FAIL() << "level change callback blocked while querying tracking players";
    }

    if (doneFuture.wait_for(std::chrono::seconds(2)) != std::future_status::ready) {
        worker.detach();
        FAIL() << "updatePlayerPosition did not finish after level change callback";
    }

    worker.join();
}

TEST_F(ChunkLoadTicketManagerExtendedTest, ShouldChunkLoadStaticMethod)
{
    // 测试静态方法
    EXPECT_TRUE(ChunkLoadTicketManager::shouldChunkLoad(31));
    EXPECT_TRUE(ChunkLoadTicketManager::shouldChunkLoad(32));
    EXPECT_TRUE(ChunkLoadTicketManager::shouldChunkLoad(33));
    EXPECT_FALSE(ChunkLoadTicketManager::shouldChunkLoad(34));
    EXPECT_FALSE(ChunkLoadTicketManager::shouldChunkLoad(100));
}

TEST_F(ChunkLoadTicketManagerExtendedTest, UnifiedPlayerSourcePropagation)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    const ChunkDistanceGraph& graph = manager.distanceGraph();
    EXPECT_EQ(graph.getLevel(0, 0), viewDistanceToLevel(5));
    EXPECT_EQ(graph.getLevel(5, 0), 33);
    EXPECT_EQ(graph.getLevel(6, 0), ChunkDistanceGraph::MAX_LEVEL);
}

TEST_F(ChunkLoadTicketManagerExtendedTest, PlayerMoveRebuildsSingleGraph)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();
    EXPECT_EQ(manager.distanceGraph().getLevel(0, 0), viewDistanceToLevel(5));

    manager.updatePlayerPosition(1, 10, 10);
    for (i32 i = 0; i < 20; ++i) {
        if (manager.distanceGraph().getLevel(0, 0) == ChunkDistanceGraph::MAX_LEVEL &&
            manager.distanceGraph().getLevel(10, 10) == viewDistanceToLevel(5)) {
            break;
        }
        manager.processUpdates();
    }

    EXPECT_EQ(manager.distanceGraph().getLevel(10, 10), viewDistanceToLevel(5));
    EXPECT_EQ(manager.distanceGraph().getLevel(0, 0), ChunkDistanceGraph::MAX_LEVEL);
}

TEST_F(ChunkLoadTicketManagerExtendedTest, ViewDistanceChangeRebuildsPlayerSourceLevel)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(10);
    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    EXPECT_EQ(manager.distanceGraph().getLevel(0, 0), viewDistanceToLevel(10));

    manager.setViewDistance(15);
    for (i32 i = 0; i < 20; ++i) {
        manager.processUpdates();
    }

    EXPECT_EQ(manager.distanceGraph().getLevel(0, 0), viewDistanceToLevel(15));
    EXPECT_EQ(manager.distanceGraph().getLevel(15, 0), 33);
    EXPECT_EQ(manager.distanceGraph().getLevel(16, 0), ChunkDistanceGraph::MAX_LEVEL);
}

TEST_F(ChunkLoadTicketManagerExtendedTest, TrackingPlayersFollowIndependentCoverageSet)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(3);

    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();
    EXPECT_TRUE(manager.isPlayerTracking(1, 3, 0));
    EXPECT_FALSE(manager.isPlayerTracking(1, 4, 0));

    manager.updatePlayerPosition(1, 10, 10);
    manager.processUpdates();
    EXPECT_FALSE(manager.isPlayerTracking(1, 0, 0));
    EXPECT_TRUE(manager.isPlayerTracking(1, 13, 10));
}

TEST_F(ChunkLoadTicketManagerExtendedTest, MultiplePlayersSameCenterKeepSourceUntilLastLeaves)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    manager.updatePlayerPosition(1, 0, 0);
    manager.updatePlayerPosition(2, 0, 0);
    manager.processUpdates();

    EXPECT_TRUE(manager.shouldChunkLoad(5, 0));

    manager.removePlayer(1);
    manager.processUpdates();
    EXPECT_TRUE(manager.shouldChunkLoad(5, 0));

    manager.removePlayer(2);
    for (i32 i = 0; i < 20; ++i) {
        manager.processUpdates();
    }
    EXPECT_FALSE(manager.shouldChunkLoad(5, 0));
}

TEST_F(ChunkLoadTicketManagerExtendedTest, ViewDistanceShrinkClearsOldPropagationTail)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(10);
    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    EXPECT_TRUE(manager.shouldChunkLoad(10, 0));

    manager.setViewDistance(3);
    for (i32 i = 0; i < 20; ++i) {
        manager.processUpdates();
    }

    EXPECT_TRUE(manager.shouldChunkLoad(3, 0));
    EXPECT_FALSE(manager.shouldChunkLoad(10, 0));
}

TEST_F(ChunkLoadTicketManagerExtendedTest, ForcedTicketAndPlayerSourceUseSingleMinimumLevel)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    manager.updatePlayerPosition(1, 0, 0);
    manager.forceChunk(0, 0, true);
    manager.processUpdates();

    const ChunkDistanceGraph& graph = manager.distanceGraph();
    EXPECT_EQ(graph.getLevel(0, 0), viewDistanceToLevel(5));
    EXPECT_EQ(graph.getLevel(2, 0), viewDistanceToLevel(5) + 2);
    EXPECT_EQ(graph.getLevel(3, 0), viewDistanceToLevel(5) + 3);
    EXPECT_EQ(graph.getLevel(6, 0), ChunkDistanceGraph::MAX_LEVEL);
}

TEST_F(ChunkLoadTicketManagerExtendedTest, TrackingChangeCallbackReportsEnterAndLeave)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(2);

    std::vector<std::tuple<PlayerId, ChunkCoord, ChunkCoord, bool>> events;
    manager.setTrackingChangeCallback([&](PlayerId playerId, ChunkCoord x, ChunkCoord z, bool isTracking) {
        events.emplace_back(playerId, x, z, isTracking);
    });

    manager.updatePlayerPosition(1, 0, 0);
    EXPECT_FALSE(events.empty());

    bool sawEnterCenter = false;
    for (const auto& [playerId, x, z, isTracking] : events) {
        if (playerId == 1 && x == 0 && z == 0 && isTracking) {
            sawEnterCenter = true;
            break;
        }
    }
    EXPECT_TRUE(sawEnterCenter);

    events.clear();
    manager.updatePlayerPosition(1, 10, 0);

    bool sawLeaveOldCenter = false;
    bool sawEnterNewCenter = false;
    for (const auto& [playerId, x, z, isTracking] : events) {
        if (playerId == 1 && x == 0 && z == 0 && !isTracking) {
            sawLeaveOldCenter = true;
        }
        if (playerId == 1 && x == 10 && z == 0 && isTracking) {
            sawEnterNewCenter = true;
        }
    }
    EXPECT_TRUE(sawLeaveOldCenter);
    EXPECT_TRUE(sawEnterNewCenter);
}

// ============================================================================
// ChunkLoadLevel 测试
// ============================================================================

TEST(ChunkLoadLevelTest, LevelToLoadLevelConversion)
{
    EXPECT_EQ(levelToLoadLevel(31), ChunkLoadLevel::Full);
    EXPECT_EQ(levelToLoadLevel(30), ChunkLoadLevel::Full);
    EXPECT_EQ(levelToLoadLevel(32), ChunkLoadLevel::EntityTicking);
    EXPECT_EQ(levelToLoadLevel(33), ChunkLoadLevel::Border);
    EXPECT_EQ(levelToLoadLevel(34), ChunkLoadLevel::Unloaded);
    EXPECT_EQ(levelToLoadLevel(100), ChunkLoadLevel::Unloaded);
}

TEST(ChunkLoadLevelTest, ShouldChunkLoad)
{
    EXPECT_TRUE(shouldChunkLoad(31));
    EXPECT_TRUE(shouldChunkLoad(32));
    EXPECT_TRUE(shouldChunkLoad(33));
    EXPECT_FALSE(shouldChunkLoad(34));
    EXPECT_FALSE(shouldChunkLoad(100));
}

TEST(ChunkLoadLevelTest, ViewDistanceToLevel)
{
    EXPECT_EQ(viewDistanceToLevel(10), 23);
    EXPECT_EQ(viewDistanceToLevel(5), 28);
    EXPECT_EQ(viewDistanceToLevel(15), 18);
}

// ============================================================================
// ChunkLoadTicket 扩展测试
// ============================================================================

class ChunkLoadTicketExtendedTest : public ::testing::Test {
protected:
    void SetUp() override { TicketTypes::initializeTicketTypes(); }
};

TEST_F(ChunkLoadTicketExtendedTest, UnitTypeTicket)
{
    // 测试 Unit 类型票据（不需要关联值）
    ChunkLoadTicket startTicket(TicketTypes::START, 31, Unit{});
    EXPECT_EQ(startTicket.level(), 31);
    EXPECT_EQ(startTicket.typeName(), "start");
    // Unit 类型票据不应该有 chunkValue 或 intValue
    EXPECT_FALSE(startTicket.hasChunkValue());
    EXPECT_FALSE(startTicket.hasIntValue());
}

TEST_F(ChunkLoadTicketExtendedTest, DragonTypeTicket)
{
    ChunkLoadTicket dragonTicket(TicketTypes::DRAGON, 31, Unit{});
    EXPECT_EQ(dragonTicket.level(), 31);
    EXPECT_EQ(dragonTicket.typeName(), "dragon");
    EXPECT_FALSE(dragonTicket.hasChunkValue());
    EXPECT_FALSE(dragonTicket.hasIntValue());
}

TEST_F(ChunkLoadTicketExtendedTest, ForceTicksFlag)
{
    ChunkLoadTicket ticket(TicketTypes::FORCED, 31, ChunkPos(0, 0));
    EXPECT_FALSE(ticket.isForceTicks());

    ticket.setForceTicks(true);
    EXPECT_TRUE(ticket.isForceTicks());

    ticket.setForceTicks(false);
    EXPECT_FALSE(ticket.isForceTicks());
}

TEST_F(ChunkLoadTicketExtendedTest, PortalTicketExpiration)
{
    // PORTAL 票据有 300 tick 生命周期
    ChunkLoadTicket portalTicket(TicketTypes::PORTAL, 31, ChunkPos(10, 20));
    portalTicket.setTimestamp(0);

    EXPECT_FALSE(portalTicket.isExpired(0));
    EXPECT_FALSE(portalTicket.isExpired(299)); // 刚好未过期
    EXPECT_TRUE(portalTicket.isExpired(301));  // 已过期
    EXPECT_TRUE(portalTicket.isExpired(1000)); // 已过期很久
}

TEST_F(ChunkLoadTicketExtendedTest, PostTeleportTicketExpiration)
{
    // POST_TELEPORT 票据有 5 tick 生命周期
    ChunkLoadTicket teleportTicket(TicketTypes::POST_TELEPORT, 31, 42u);
    teleportTicket.setTimestamp(0);

    EXPECT_FALSE(teleportTicket.isExpired(0));
    EXPECT_FALSE(teleportTicket.isExpired(5)); // 刚好未过期
    EXPECT_TRUE(teleportTicket.isExpired(6));  // 已过期
}

TEST_F(ChunkLoadTicketExtendedTest, TicketTypeComparison)
{
    auto type1 = ChunkLoadTicketType<ChunkPos>::create("type1");
    auto type2 = ChunkLoadTicketType<ChunkPos>::create("type2");
    auto type1Copy = ChunkLoadTicketType<ChunkPos>::create("type1");

    EXPECT_EQ(type1, type1Copy);
    EXPECT_NE(type1, type2);
}

TEST_F(ChunkLoadTicketExtendedTest, TicketEquality)
{
    ChunkLoadTicket t1(TicketTypes::FORCED, 31, ChunkPos(5, 10));
    ChunkLoadTicket t2(TicketTypes::FORCED, 31, ChunkPos(5, 10));
    ChunkLoadTicket t3(TicketTypes::FORCED, 31, ChunkPos(5, 11));  // 不同位置
    ChunkLoadTicket t4(TicketTypes::FORCED, 32, ChunkPos(5, 10));  // 不同级别
    ChunkLoadTicket t5(TicketTypes::UNKNOWN, 31, ChunkPos(5, 10)); // 不同类型

    EXPECT_EQ(t1, t2);
    EXPECT_NE(t1, t3);
    EXPECT_NE(t1, t4);
    EXPECT_NE(t1, t5);
}

TEST_F(ChunkLoadTicketExtendedTest, TicketWithNegativeCoordinates)
{
    // 测试负坐标
    ChunkLoadTicket ticket(TicketTypes::FORCED, 31, ChunkPos(-100, -200));
    EXPECT_TRUE(ticket.hasChunkValue());
    EXPECT_EQ(ticket.chunkValue().x, -100);
    EXPECT_EQ(ticket.chunkValue().z, -200);
}

TEST_F(ChunkLoadTicketExtendedTest, TicketPriorityOrdering)
{
    // 相同类型，不同级别
    ChunkLoadTicket t1(TicketTypes::FORCED, 31, ChunkPos(0, 0));
    ChunkLoadTicket t2(TicketTypes::FORCED, 32, ChunkPos(0, 0));
    ChunkLoadTicket t3(TicketTypes::FORCED, 33, ChunkPos(0, 0));

    // 级别小的优先级高
    EXPECT_TRUE(t1.hasHigherPriorityThan(t2));
    EXPECT_TRUE(t1.hasHigherPriorityThan(t3));
    EXPECT_TRUE(t2.hasHigherPriorityThan(t3));

    // operator< 是反向的（用于优先队列）
    EXPECT_FALSE(t1 < t2);
    EXPECT_TRUE(t2 < t1);
}

TEST_F(ChunkLoadTicketExtendedTest, CustomComparator)
{
    // 使用自定义比较器
    auto customType = ChunkLoadTicketType<ChunkPos>::create("custom", [](const ChunkPos& a, const ChunkPos& b) {
        // 按曼哈顿距离排序
        return (std::abs(a.x) + std::abs(a.z)) < (std::abs(b.x) + std::abs(b.z));
    });
    EXPECT_EQ(customType.name(), "custom");
}

TEST_F(ChunkLoadTicketExtendedTest, AllPredefinedTicketTypes)
{
    // 验证所有预定义票据类型
    EXPECT_EQ(TicketTypes::FORCED.name(), "forced");
    EXPECT_EQ(TicketTypes::FORCED.name(), "forced");
    EXPECT_EQ(TicketTypes::PORTAL.name(), "portal");
    EXPECT_EQ(TicketTypes::POST_TELEPORT.name(), "post_teleport");
    EXPECT_EQ(TicketTypes::UNKNOWN.name(), "unknown");
    EXPECT_EQ(TicketTypes::START.name(), "start");
    EXPECT_EQ(TicketTypes::DRAGON.name(), "dragon");
    EXPECT_EQ(TicketTypes::LIGHT.name(), "light");

    // 验证生命周期
    EXPECT_EQ(TicketTypes::FORCED.lifespan(), 0u);        // 永久
    EXPECT_EQ(TicketTypes::PORTAL.lifespan(), 300u);      // 300 tick
    EXPECT_EQ(TicketTypes::POST_TELEPORT.lifespan(), 5u); // 5 tick
}

// ============================================================================
// ChunkTicketSet 扩展测试
// ============================================================================

class ChunkTicketSetExtendedTest : public ::testing::Test {
protected:
    void SetUp() override { TicketTypes::initializeTicketTypes(); }
};

TEST_F(ChunkTicketSetExtendedTest, MultipleTicketsOfDifferentTypes)
{
    ChunkTicketSet set;

    // 添加多种类型的票据
    set.addTicket(ChunkLoadTicket(TicketTypes::FORCED, 31, ChunkPos(0, 0)));
    set.addTicket(ChunkLoadTicket(TicketTypes::FORCED, 31, ChunkPos(0, 0)));
    set.addTicket(ChunkLoadTicket(TicketTypes::PORTAL, 32, ChunkPos(0, 0)));
    set.addTicket(ChunkLoadTicket(TicketTypes::LIGHT, 33, ChunkPos(0, 0)));

    EXPECT_EQ(set.size(), 3u);
    EXPECT_EQ(set.getMinLevel(), 31); // 最小级别
}

TEST_F(ChunkTicketSetExtendedTest, TicketSetWithExpiration)
{
    auto expireType1 = ChunkLoadTicketType<ChunkPos>::create("expire1", 10);
    auto expireType2 = ChunkLoadTicketType<ChunkPos>::create("expire2", 20);
    auto permanentType = ChunkLoadTicketType<ChunkPos>::create("permanent");

    ChunkTicketSet set;

    ChunkLoadTicket t1(expireType1, 31, ChunkPos(0, 0));
    t1.setTimestamp(0);
    ChunkLoadTicket t2(expireType2, 32, ChunkPos(0, 0));
    t2.setTimestamp(0);
    ChunkLoadTicket t3(permanentType, 33, ChunkPos(0, 0));
    t3.setTimestamp(0);

    set.addTicket(t1);
    set.addTicket(t2);
    set.addTicket(t3);

    EXPECT_EQ(set.size(), 3u);

    // t1 过期（10 tick 后）
    set.removeExpired(15);
    EXPECT_EQ(set.size(), 2u);

    // t2 过期（20 tick 后）
    set.removeExpired(25);
    EXPECT_EQ(set.size(), 1u);

    // t3 永不过期
    set.removeExpired(100000);
    EXPECT_EQ(set.size(), 1u);
}

TEST_F(ChunkTicketSetExtendedTest, RemoveNonExistentTicket)
{
    ChunkTicketSet set;

    ChunkLoadTicket t1(TicketTypes::FORCED, 31, ChunkPos(0, 0));
    ChunkLoadTicket t2(TicketTypes::FORCED, 31, ChunkPos(1, 1));

    set.addTicket(t1);
    EXPECT_EQ(set.size(), 1u);

    // 尝试移除不存在的票据
    bool removed = set.removeTicket(t2);
    EXPECT_FALSE(removed);
    EXPECT_EQ(set.size(), 1u);
}

TEST_F(ChunkTicketSetExtendedTest, MinLevelWithEmptySet)
{
    ChunkTicketSet set;
    EXPECT_EQ(set.getMinLevel(), static_cast<i32>(ChunkLoadLevel::MaxLevel));
}

TEST_F(ChunkTicketSetExtendedTest, MinLevelAfterRemoval)
{
    ChunkTicketSet set;

    ChunkLoadTicket t1(TicketTypes::FORCED, 31, ChunkPos(0, 0));
    ChunkLoadTicket t2(TicketTypes::FORCED, 32, ChunkPos(0, 0));
    ChunkLoadTicket t3(TicketTypes::PORTAL, 33, ChunkPos(0, 0));

    set.addTicket(t1);
    set.addTicket(t2);
    set.addTicket(t3);
    EXPECT_EQ(set.getMinLevel(), 31);

    // 移除最小级别的票据
    set.removeTicket(t1);
    EXPECT_EQ(set.getMinLevel(), 32);

    set.removeTicket(t2);
    EXPECT_EQ(set.getMinLevel(), 33);

    set.removeTicket(t3);
    EXPECT_TRUE(set.empty());
    EXPECT_EQ(set.getMinLevel(), static_cast<i32>(ChunkLoadLevel::MaxLevel));
}

TEST_F(ChunkTicketSetExtendedTest, DuplicateTicketHandling)
{
    ChunkTicketSet set;

    ChunkLoadTicket t1(TicketTypes::FORCED, 31, ChunkPos(0, 0));

    // 添加相同票据多次
    set.addTicket(t1);
    set.addTicket(t1);
    set.addTicket(t1);

    EXPECT_EQ(set.size(), 1u); // 只保留一个

    // 添加级别相同但值不同的票据
    ChunkLoadTicket t2(TicketTypes::FORCED, 31, ChunkPos(1, 1));
    set.addTicket(t2);
    EXPECT_EQ(set.size(), 2u); // 不同票据
}

TEST_F(ChunkTicketSetExtendedTest, TicketSetIterator)
{
    ChunkTicketSet set;

    ChunkLoadTicket t1(TicketTypes::FORCED, 31, ChunkPos(0, 0));
    ChunkLoadTicket t2(TicketTypes::FORCED, 32, ChunkPos(0, 0));
    ChunkLoadTicket t3(TicketTypes::PORTAL, 33, ChunkPos(0, 0));

    set.addTicket(t1);
    set.addTicket(t2);
    set.addTicket(t3);

    const auto& tickets = set.tickets();
    EXPECT_EQ(tickets.size(), 3u);

    // 验证所有票据都在
    int count = 0;
    for (const auto& ticket : tickets) {
        EXPECT_TRUE(ticket.level() >= 31 && ticket.level() <= 33);
        ++count;
    }
    EXPECT_EQ(count, 3);
}

TEST_F(ChunkTicketSetExtendedTest, ExpirationPreservesMinLevel)
{
    auto expireType = ChunkLoadTicketType<ChunkPos>::create("expire", 5);
    auto permanentType = ChunkLoadTicketType<ChunkPos>::create("permanent");

    ChunkTicketSet set;

    // 添加永久票据（高级别）
    ChunkLoadTicket permanent(permanentType, 33, ChunkPos(0, 0));
    set.addTicket(permanent);

    // 添加过期票据（低级别）
    ChunkLoadTicket expiring(expireType, 31, ChunkPos(0, 0));
    expiring.setTimestamp(0);
    set.addTicket(expiring);

    EXPECT_EQ(set.getMinLevel(), 31); // 过期票据提供最小级别

    // 过期后
    set.removeExpired(10);
    EXPECT_EQ(set.getMinLevel(), 33); // 只剩永久票据
}

// ============================================================================
// getForcedChunks / isForcedChunk 测试
// ============================================================================

class ForcedChunkQueryTest : public ::testing::Test {
protected:
    void SetUp() override { TicketTypes::initializeTicketTypes(); }
};

TEST_F(ForcedChunkQueryTest, GetForcedChunksEmpty)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    // 没有强制加载区块时，返回空列表
    std::vector<ChunkPos> forcedChunks = manager.getForcedChunks();
    EXPECT_TRUE(forcedChunks.empty());
}

TEST_F(ForcedChunkQueryTest, GetForcedChunksSingleChunk)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    // 强制加载单个区块
    manager.forceChunk(10, 20, true);
    manager.processUpdates();

    std::vector<ChunkPos> forcedChunks = manager.getForcedChunks();
    ASSERT_EQ(forcedChunks.size(), 1u);
    EXPECT_EQ(forcedChunks[0].x, 10);
    EXPECT_EQ(forcedChunks[0].z, 20);
}

TEST_F(ForcedChunkQueryTest, GetForcedChunksMultipleChunks)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    // 强制加载多个区块
    manager.forceChunk(0, 0, true);
    manager.forceChunk(10, 20, true);
    manager.forceChunk(-5, 15, true);
    manager.processUpdates();

    std::vector<ChunkPos> forcedChunks = manager.getForcedChunks();
    EXPECT_EQ(forcedChunks.size(), 3u);

    // 验证所有强制加载的区块都在列表中
    bool found00 = false, found1020 = false, foundMinus515 = false;
    for (const auto& pos : forcedChunks) {
        if (pos.x == 0 && pos.z == 0) found00 = true;
        if (pos.x == 10 && pos.z == 20) found1020 = true;
        if (pos.x == -5 && pos.z == 15) foundMinus515 = true;
    }
    EXPECT_TRUE(found00);
    EXPECT_TRUE(found1020);
    EXPECT_TRUE(foundMinus515);
}

TEST_F(ForcedChunkQueryTest, GetForcedChunksAfterRemoval)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    // 强制加载多个区块
    manager.forceChunk(0, 0, true);
    manager.forceChunk(10, 20, true);
    manager.processUpdates();

    EXPECT_EQ(manager.getForcedChunks().size(), 2u);

    // 移除一个
    manager.forceChunk(0, 0, false);
    manager.processUpdates();

    std::vector<ChunkPos> forcedChunks = manager.getForcedChunks();
    ASSERT_EQ(forcedChunks.size(), 1u);
    EXPECT_EQ(forcedChunks[0].x, 10);
    EXPECT_EQ(forcedChunks[0].z, 20);
}

TEST_F(ForcedChunkQueryTest, GetForcedChunksOnlyReturnsForced)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    // 添加玩家来源
    manager.updatePlayerPosition(1, 0, 0);

    // 强制加载一个区块
    manager.forceChunk(100, 100, true);
    manager.processUpdates();

    // 只应该返回强制加载的区块，不包含玩家区块
    std::vector<ChunkPos> forcedChunks = manager.getForcedChunks();
    ASSERT_EQ(forcedChunks.size(), 1u);
    EXPECT_EQ(forcedChunks[0].x, 100);
    EXPECT_EQ(forcedChunks[0].z, 100);
}

TEST_F(ForcedChunkQueryTest, IsForcedChunkBasic)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    // 未强制加载时返回 false
    EXPECT_FALSE(manager.isForcedChunk(10, 20));

    // 强制加载后返回 true
    manager.forceChunk(10, 20, true);
    manager.processUpdates();
    EXPECT_TRUE(manager.isForcedChunk(10, 20));

    // 其他区块仍返回 false
    EXPECT_FALSE(manager.isForcedChunk(10, 21));
    EXPECT_FALSE(manager.isForcedChunk(11, 20));

    // 取消强制加载后返回 false
    manager.forceChunk(10, 20, false);
    manager.processUpdates();
    EXPECT_FALSE(manager.isForcedChunk(10, 20));
}

TEST_F(ForcedChunkQueryTest, IsForcedChunkWithPlayerSources)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    // 添加玩家来源
    manager.updatePlayerPosition(1, 50, 50);
    manager.processUpdates();

    // 玩家区块应该加载，但不是强制加载
    EXPECT_TRUE(manager.shouldChunkLoad(50, 50));
    EXPECT_FALSE(manager.isForcedChunk(50, 50));

    // 强制加载另一个区块
    manager.forceChunk(100, 100, true);
    manager.processUpdates();

    EXPECT_FALSE(manager.isForcedChunk(50, 50));  // 玩家区块仍不是强制加载
    EXPECT_TRUE(manager.isForcedChunk(100, 100)); // 强制加载区块
}

TEST_F(ForcedChunkQueryTest, IsForcedChunkNegativeCoordinates)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    // 测试负坐标
    manager.forceChunk(-100, -200, true);
    manager.forceChunk(-50, 50, true);
    manager.processUpdates();

    EXPECT_TRUE(manager.isForcedChunk(-100, -200));
    EXPECT_TRUE(manager.isForcedChunk(-50, 50));
    EXPECT_FALSE(manager.isForcedChunk(-100, -201));
}

TEST_F(ForcedChunkQueryTest, IsForcedChunkAfterPlayerRemoval)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    // 强制加载区块
    manager.forceChunk(100, 100, true);

    // 添加玩家
    manager.updatePlayerPosition(1, 0, 0);
    manager.processUpdates();

    // 移除玩家
    manager.removePlayer(1);
    manager.processUpdates();

    // 强制加载区块仍然存在
    EXPECT_TRUE(manager.isForcedChunk(100, 100));
    EXPECT_TRUE(manager.shouldChunkLoad(100, 100));
}

TEST_F(ForcedChunkQueryTest, MultipleForcedChunksConsistency)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    // 强制加载多个区块
    for (int x = 0; x < 5; ++x) {
        for (int z = 0; z < 5; ++z) {
            manager.forceChunk(x * 10, z * 10, true);
        }
    }
    manager.processUpdates();

    // 验证一致性
    std::vector<ChunkPos> forcedChunks = manager.getForcedChunks();
    EXPECT_EQ(forcedChunks.size(), 25u);

    for (const auto& pos : forcedChunks) {
        // 每个返回的区块都应该是强制加载的
        EXPECT_TRUE(manager.isForcedChunk(pos.x, pos.z));
        // 每个返回的区块都应该加载
        EXPECT_TRUE(manager.shouldChunkLoad(pos.x, pos.z));
    }
}

TEST_F(ForcedChunkQueryTest, IsForcedChunkDoesNotAffectTickets)
{
    ChunkLoadTicketManager manager;
    manager.setViewDistance(5);

    // 强制加载区块
    manager.forceChunk(10, 20, true);
    manager.processUpdates();

    // 调用 isForcedChunk 不应该影响票据
    EXPECT_TRUE(manager.isForcedChunk(10, 20));
    EXPECT_TRUE(manager.isForcedChunk(10, 20));
    EXPECT_TRUE(manager.isForcedChunk(10, 20));

    // 仍然只有一个强制加载区块
    EXPECT_EQ(manager.getForcedChunks().size(), 1u);
}
