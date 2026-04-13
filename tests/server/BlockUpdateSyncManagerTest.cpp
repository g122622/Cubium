#include <gtest/gtest.h>

#include "server/sync/BlockUpdateSyncManager.hpp"
#include "common/world/chunk/ChunkLoadTicketManager.hpp"
#include "common/world/block/BlockPos.hpp"

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
    void SetUp() override {
        m_ticketManager.setViewDistance(8);
        m_manager = std::make_unique<BlockUpdateSyncManager>(m_ticketManager);
        m_manager->setOnBlockUpdate([this](PlayerId playerId, i32 x, i32 y, i32 z, u32 blockStateId) {
            m_sentUpdates.push_back(SentBlockUpdate{playerId, BlockPos(x, y, z), blockStateId});
        });
    }

    void TearDown() override {
        m_manager.reset();
        m_sentUpdates.clear();
    }

    void addTrackingPlayer(PlayerId playerId, ChunkCoord chunkX, ChunkCoord chunkZ) {
        m_ticketManager.updatePlayerPosition(playerId, chunkX, chunkZ);
    }

protected:
    world::ChunkLoadTicketManager m_ticketManager;
    std::unique_ptr<BlockUpdateSyncManager> m_manager;
    std::vector<SentBlockUpdate> m_sentUpdates;
};

} // namespace


TEST_F(BlockUpdateSyncManagerTest, DeduplicatesSameBlockWithinTick) {
    addTrackingPlayer(1, 0, 0);

    m_manager->queueBlockUpdate(1, 64, 1, 5u);
    m_manager->queueBlockUpdate(1, 64, 1, 7u);
    m_manager->flushPendingUpdates();

    ASSERT_EQ(m_sentUpdates.size(), 1u);
    EXPECT_EQ(m_sentUpdates[0].playerId, 1u);
    EXPECT_EQ(m_sentUpdates[0].pos, BlockPos(1, 64, 1));
    EXPECT_EQ(m_sentUpdates[0].blockStateId, 7u);
}

TEST_F(BlockUpdateSyncManagerTest, SendsDistinctPositionsSeparately) {
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

TEST_F(BlockUpdateSyncManagerTest, SendsToAllTrackingPlayers) {
    addTrackingPlayer(1, 0, 0);
    addTrackingPlayer(2, 0, 0);

    m_manager->queueBlockUpdate(1, 64, 1, 5u);
    m_manager->flushPendingUpdates();

    ASSERT_EQ(m_sentUpdates.size(), 2u);
    std::sort(m_sentUpdates.begin(), m_sentUpdates.end(), [](const SentBlockUpdate& left, const SentBlockUpdate& right) {
        return left.playerId < right.playerId;
    });

    EXPECT_EQ(m_sentUpdates[0].playerId, 1u);
    EXPECT_EQ(m_sentUpdates[0].pos, BlockPos(1, 64, 1));
    EXPECT_EQ(m_sentUpdates[0].blockStateId, 5u);
    EXPECT_EQ(m_sentUpdates[1].playerId, 2u);
    EXPECT_EQ(m_sentUpdates[1].pos, BlockPos(1, 64, 1));
    EXPECT_EQ(m_sentUpdates[1].blockStateId, 5u);
}

TEST_F(BlockUpdateSyncManagerTest, SkipsPlayersWhoStopTrackingBeforeFlush) {
    addTrackingPlayer(1, 0, 0);

    m_manager->queueBlockUpdate(1, 64, 1, 5u);
    m_ticketManager.removePlayer(1);
    m_manager->flushPendingUpdates();

    EXPECT_TRUE(m_sentUpdates.empty());
}