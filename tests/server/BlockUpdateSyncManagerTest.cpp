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

#include <gtest/gtest.h>

#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/load/ChunkLoadTicketManager.hpp"
#include "server/sync/BlockUpdateSyncManager.hpp"

#include <algorithm>
#include <memory>
#include <vector>

using namespace mc;
using namespace mc::server::sync;

namespace {

struct SentBlockUpdate {
    PlayerId playerId = 0;
    BlockPos pos;
    u32 blockStateId = 0;
};

class BlockUpdateSyncManagerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_ticketManager.setViewDistance(8);
        m_manager = std::make_unique<BlockUpdateSyncManager>(m_ticketManager);
        m_manager->setOnBlockUpdate([this](PlayerId playerId, i32 x, i32 y, i32 z, u32 blockStateId) {
            m_sentUpdates.push_back(SentBlockUpdate{playerId, BlockPos(x, y, z), blockStateId});
        });
    }

    void TearDown() override
    {
        m_manager.reset();
        m_sentUpdates.clear();
    }

    void addTrackingPlayer(PlayerId playerId, ChunkCoord chunkX, ChunkCoord chunkZ)
    {
        m_ticketManager.updatePlayerPosition(playerId, chunkX, chunkZ);
    }

protected:
    mc::world::chunk::ChunkLoadTicketManager m_ticketManager;
    std::unique_ptr<BlockUpdateSyncManager> m_manager;
    std::vector<SentBlockUpdate> m_sentUpdates;
};

} // namespace

TEST_F(BlockUpdateSyncManagerTest, DeduplicatesSameBlockWithinTick)
{
    addTrackingPlayer(1, 0, 0);

    m_manager->queueBlockUpdate(1, 64, 1, 5u);
    m_manager->queueBlockUpdate(1, 64, 1, 7u);
    m_manager->flushPendingUpdates();

    ASSERT_EQ(m_sentUpdates.size(), 1u);
    EXPECT_EQ(m_sentUpdates[0].playerId, 1u);
    EXPECT_EQ(m_sentUpdates[0].pos, BlockPos(1, 64, 1));
    EXPECT_EQ(m_sentUpdates[0].blockStateId, 7u);
}

TEST_F(BlockUpdateSyncManagerTest, SendsDistinctPositionsSeparately)
{
    addTrackingPlayer(1, 0, 0);

    m_manager->queueBlockUpdate(1, 64, 1, 5u);
    m_manager->queueBlockUpdate(2, 64, 1, 6u);
    m_manager->flushPendingUpdates();

    ASSERT_EQ(m_sentUpdates.size(), 2u);
    EXPECT_EQ(m_sentUpdates[0].playerId, 1u);
    EXPECT_EQ(m_sentUpdates[0].pos, BlockPos(1, 64, 1));
    EXPECT_EQ(m_sentUpdates[0].blockStateId, 5u);
    EXPECT_EQ(m_sentUpdates[1].playerId, 1u);
    EXPECT_EQ(m_sentUpdates[1].pos, BlockPos(2, 64, 1));
    EXPECT_EQ(m_sentUpdates[1].blockStateId, 6u);
}

TEST_F(BlockUpdateSyncManagerTest, SendsToAllTrackingPlayers)
{
    addTrackingPlayer(1, 0, 0);
    addTrackingPlayer(2, 0, 0);

    m_manager->queueBlockUpdate(1, 64, 1, 5u);
    m_manager->flushPendingUpdates();

    ASSERT_EQ(m_sentUpdates.size(), 2u);
    std::sort(m_sentUpdates.begin(),
        m_sentUpdates.end(),
        [](const SentBlockUpdate& left, const SentBlockUpdate& right) { return left.playerId < right.playerId; });

    EXPECT_EQ(m_sentUpdates[0].playerId, 1u);
    EXPECT_EQ(m_sentUpdates[0].pos, BlockPos(1, 64, 1));
    EXPECT_EQ(m_sentUpdates[0].blockStateId, 5u);
    EXPECT_EQ(m_sentUpdates[1].playerId, 2u);
    EXPECT_EQ(m_sentUpdates[1].pos, BlockPos(1, 64, 1));
    EXPECT_EQ(m_sentUpdates[1].blockStateId, 5u);
}

TEST_F(BlockUpdateSyncManagerTest, SkipsPlayersWhoStopTrackingBeforeFlush)
{
    addTrackingPlayer(1, 0, 0);

    m_manager->queueBlockUpdate(1, 64, 1, 5u);
    m_ticketManager.removePlayer(1);
    m_manager->flushPendingUpdates();

    EXPECT_TRUE(m_sentUpdates.empty());
}