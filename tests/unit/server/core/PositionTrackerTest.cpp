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

#include "server/core/PositionTracker.hpp"
#include "common/core/Types.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "server/core/PlayerManager.hpp"
#include <algorithm>
#include <vector>
#include <gtest/gtest.h>

using namespace mc::server::core;
using mc::ChunkCoord;
using mc::ChunkPos;

/**
 * @brief PositionTracker 单元测试
 *
 * 新网络层 addPlayer 第4参为 mc::server::net::ServerClientConnection*（裸指针）。
 * 本测试只验证 PositionTracker/PlayerManager 的位置数据维护，不依赖连接真发包，
 * 故统一传 nullptr（与 BaseTestServer::addTestPlayer 一致，Step5 删旧体系后
 * 统一重构测试桩）。
 */
class PositionTrackerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_playerManager = std::make_unique<PlayerManager>();
        m_positionTracker = std::make_unique<PositionTracker>(*m_playerManager, 10);
    }

    void TearDown() override
    {
        m_positionTracker.reset();
        m_playerManager.reset();
    }

    std::unique_ptr<PlayerManager> m_playerManager;
    std::unique_ptr<PositionTracker> m_positionTracker;
};

TEST_F(PositionTrackerTest, UpdatePosition)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

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

TEST_F(PositionTrackerTest, UpdatePositionNonexistentPlayer)
{
    bool result = m_positionTracker->updatePosition(999, 100.0, 64.0, 200.0, 0.0f, 0.0f, true);
    EXPECT_FALSE(result);
}

TEST_F(PositionTrackerTest, UpdatePositionOnly)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    m_positionTracker->updatePosition(1, 100.0, 64.0, 200.0);

    auto* player = m_playerManager->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_FLOAT_EQ(player->x, 100.0f);
    EXPECT_FLOAT_EQ(player->y, 64.0f);
    EXPECT_FLOAT_EQ(player->z, 200.0f);
}

TEST_F(PositionTrackerTest, UpdateRotation)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    m_positionTracker->updateRotation(1, 180.0f, 30.0f);

    auto* player = m_playerManager->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_FLOAT_EQ(player->yaw, 180.0f);
    EXPECT_FLOAT_EQ(player->pitch, 30.0f);
}

TEST_F(PositionTrackerTest, GetPosition)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    m_positionTracker->updatePosition(1, 100.0, 64.0, 200.0, 90.0f, 45.0f, true);

    auto pos = m_positionTracker->getPosition(1);
    EXPECT_FLOAT_EQ(pos.x, 100.0f);
    EXPECT_FLOAT_EQ(pos.y, 64.0f);
    EXPECT_FLOAT_EQ(pos.z, 200.0f);

    // 不存在的玩家返回默认位置
    auto defaultPos = m_positionTracker->getPosition(999);
    EXPECT_FLOAT_EQ(defaultPos.y, 64.0f);
}

TEST_F(PositionTrackerTest, GetRotation)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    m_positionTracker->updateRotation(1, 180.0f, 30.0f);

    auto rot = m_positionTracker->getRotation(1);
    EXPECT_FLOAT_EQ(rot.x, 180.0f);
    EXPECT_FLOAT_EQ(rot.y, 30.0f);
}

TEST_F(PositionTrackerTest, GetChunkPosition)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    m_positionTracker->updatePosition(1, 100.0, 64.0, 200.0);

    auto chunkPos = m_positionTracker->getChunkPosition(1);
    EXPECT_EQ(chunkPos.x, 6);  // 100 / 16 = 6.25 -> 6
    EXPECT_EQ(chunkPos.z, 12); // 200 / 16 = 12.5 -> 12
}

TEST_F(PositionTrackerTest, IsOnGround)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    m_positionTracker->updatePosition(1, 100.0, 64.0, 200.0, 0.0f, 0.0f, true);
    EXPECT_TRUE(m_positionTracker->isOnGround(1));

    m_positionTracker->updatePosition(1, 100.0, 70.0, 200.0, 0.0f, 0.0f, false);
    EXPECT_FALSE(m_positionTracker->isOnGround(1));
}

TEST_F(PositionTrackerTest, SetViewDistance)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    m_positionTracker->setViewDistance(1, 12);
    EXPECT_EQ(m_positionTracker->getViewDistance(1), 12);
}

TEST_F(PositionTrackerTest, CalculateChunkUpdates)
{
    m_playerManager = std::make_unique<PlayerManager>(20);
    m_positionTracker = std::make_unique<PositionTracker>(*m_playerManager, 2);

    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

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
    m_positionTracker->updatePosition(1, 100.0, 64.0, 100.0); // 新区块

    toLoad.clear();
    toUnload.clear();
    m_positionTracker->calculateChunkUpdates(1, toLoad, toUnload);

    // 应该有新的区块要加载，旧的区块要卸载
    // (取决于视距和移动距离)
}

TEST_F(PositionTrackerTest, MarkChunkSent)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    m_positionTracker->markChunkSent(1, 5, 10);

    auto* player = m_playerManager->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_TRUE(player->chunkTracker->hasChunk(5, 10));
}

TEST_F(PositionTrackerTest, MarkChunkUnloaded)
{
    m_playerManager->addPlayer(1, mc::util::uuidToString(mc::util::generateOfflineUuid("Steve")), "Steve", nullptr);

    m_positionTracker->markChunkSent(1, 5, 10);
    EXPECT_TRUE(m_positionTracker->getViewDistance(1) >= 0); // just check it's valid

    m_positionTracker->markChunkUnloaded(1, 5, 10);

    auto* player = m_playerManager->getPlayer(1);
    ASSERT_NE(player, nullptr);
    EXPECT_FALSE(player->chunkTracker->hasChunk(5, 10));
}
