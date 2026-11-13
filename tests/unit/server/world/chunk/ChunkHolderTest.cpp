#include "server/world/chunk/lifecycle/state/ChunkHolder.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::chunk;

TEST(ChunkHolderTest, CreationHasNoAvailableChunk)
{
    ChunkHolder holder(5, -10);
    EXPECT_EQ(holder.pos(), ChunkPos(5, -10));
    EXPECT_EQ(holder.id(), ChunkPos(5, -10).toId());
    EXPECT_EQ(holder.getCurrentGenStatus(), ChunkStatuses::EMPTY);
    EXPECT_FALSE(holder.hasCompletedStatus(ChunkStatuses::EMPTY));
    EXPECT_EQ(holder.level(), static_cast<i32>(ChunkLoadLevel::MaxLevel));
}

TEST(ChunkHolderTest, CompletedGenerationNeverRegresses)
{
    ChunkHolder holder(0, 0);
    holder.setCurrentChunk(std::make_unique<ChunkPrimer>(0, 0));
    holder.onChunkGenComplete(ChunkStatuses::NOISE);
    holder.onChunkGenComplete(ChunkStatuses::BIOMES);
    EXPECT_EQ(holder.getCurrentGenStatus(), ChunkStatuses::NOISE);
    EXPECT_TRUE(holder.hasCompletedStatus(ChunkStatuses::BIOMES));
    EXPECT_FALSE(holder.hasCompletedStatus(ChunkStatuses::SURFACE));
}

TEST(ChunkHolderTest, StorageInstallsDataAndPhaseTogether)
{
    ChunkHolder holder(-2, 7);
    holder.setLevel(static_cast<i32>(ChunkLoadLevel::Full));
    const auto decision = holder.prepareGeneration(0);
    ASSERT_TRUE(decision.shouldResolveStorage);
    ASSERT_EQ(holder.sourceState(), ChunkHolder::SourceState::ResolvingStorage);

    holder.publishStorageLoaded(std::make_unique<ChunkPrimer>(std::make_unique<ChunkData>(-2, 7)));
    const auto snapshot = holder.generationSnapshot();
    EXPECT_NE(snapshot.currentChunk, nullptr);
    EXPECT_EQ(*snapshot.currentGenStatus, ChunkStatuses::FULL);
    EXPECT_EQ(snapshot.sourceState, ChunkHolder::SourceState::LoadedFromStorage);

    holder.markDataPublished(ChunkStatuses::FULL);
    EXPECT_EQ(holder.sourceState(), ChunkHolder::SourceState::LoadedFromStorage);
    EXPECT_TRUE(holder.hasCompletedStatus(ChunkStatuses::FULL));
}

TEST(ChunkHolderTest, GeneratedPublicationKeepsItsSourceIdentity)
{
    ChunkHolder holder(0, 0);
    holder.setLevel(static_cast<i32>(ChunkLoadLevel::Full));
    ASSERT_TRUE(holder.prepareGeneration(0).shouldResolveStorage);
    holder.noteStorageMissing();
    holder.setCurrentChunk(std::make_unique<ChunkPrimer>(0, 0));
    holder.markDataPublished(ChunkStatuses::FULL);
    EXPECT_EQ(holder.sourceState(), ChunkHolder::SourceState::StorageMissing);
    EXPECT_EQ(holder.getCurrentGenStatus(), ChunkStatuses::FULL);
}
