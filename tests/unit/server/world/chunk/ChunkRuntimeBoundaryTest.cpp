#include "server/ChunkSystemTestAccess.hpp"
#include "server/world/chunk/base/completion/ChunkCompletionInbox.hpp"
#include "server/world/chunk/base/pipeline/ChunkPyramid.hpp"
#include "server/world/chunk/integration/WorldChunkLifecycle.hpp"
#include "server/world/chunk/lifecycle/state/ChunkRegistry.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/FlatChunkGenerator.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::server;

TEST(ChunkRuntimeBoundaryTest, LateStorageCompletionOutlivesClosedReceiver)
{
    auto inbox = std::make_shared<ChunkCompletionInbox<std::shared_ptr<i32>>>();
    auto completion = [inbox](std::shared_ptr<i32> value) {
        inbox->push(std::move(value));
    };
    inbox->close();
    inbox.reset();

    auto payload = std::make_shared<i32>(42);
    std::weak_ptr<i32> lifetime = payload;
    completion(std::move(payload));
    EXPECT_TRUE(lifetime.expired());
}

TEST(ChunkRuntimeBoundaryTest, BudgetPreservesOldCompletionsBeforeReentrantSubmission)
{
    ChunkCompletionInbox<i32> inbox;
    inbox.push(1);
    inbox.push(2);
    std::vector<i32> delivered;
    inbox.drain(std::chrono::milliseconds(0), [&](i32 value) {
        delivered.push_back(value);
        inbox.push(3);
    });
    EXPECT_EQ(delivered, (std::vector<i32>{1}));
    inbox.drain(std::chrono::milliseconds(100), [&](i32 value) {
        delivered.push_back(value);
    });
    EXPECT_EQ(delivered, (std::vector<i32>{1, 2, 3}));
}

TEST(ChunkRuntimeBoundaryTest, RetiredPublisherCannotOverwriteCurrentInstance)
{
    ChunkRegistry registry(
        [](ChunkCoord, ChunkCoord) {
            return 46;
        },
        [](world::chunk::ChunkHolder&) {});
    auto& original = registry.getOrCreate(0, 0);
    original.setCurrentChunk(std::make_unique<world::chunk::ChunkPrimer>(0, 0));
    auto old = registry.findShared(0, 0);
    ASSERT_TRUE(registry.retire(0, 0, old));
    auto& current = registry.getOrCreate(0, 0);

    EXPECT_EQ(registry.publish(0, 0, old->getCurrentChunk()->shareChunkData(), *old), nullptr);
    EXPECT_EQ(registry.chunk(0, 0), nullptr);
    EXPECT_EQ(registry.find(0, 0), &current);
    EXPECT_EQ(current.getCurrentChunk(), nullptr);
}

TEST(ChunkRuntimeBoundaryTest, OldActivationCannotConsumeNewInstancesRegistration)
{
    world::chunk::ChunkDemandController demand;
    ChunkRegistry registry(
        [](ChunkCoord, ChunkCoord) {
            return 46;
        },
        [](world::chunk::ChunkHolder&) {});
    WorldChunkLifecycle integration(nullptr, registry, demand);
    i32 registered = 0;
    integration.setLoadedCallback([&](ChunkCoord, ChunkCoord) {
        ++registered;
    });

    (void)registry.getOrCreate(0, 0);
    auto old = registry.findShared(0, 0);
    integration.enqueue(0, 0, {}, false);
    ASSERT_TRUE(registry.retire(0, 0, old));
    integration.forget(0, 0);
    (void)registry.getOrCreate(0, 0);
    integration.enqueue(0, 0, {}, false);
    integration.drain();
    integration.drain();
    EXPECT_EQ(registered, 1);
}

TEST(ChunkRuntimeBoundaryTest, NonOriginUnloadCandidatesUseWorldLocalCoordinates)
{
    ServerChunkManager manager(std::make_unique<FlatChunkGenerator>(0, FlatLevelGeneratorSettings::createDefault()));
    auto& lifecycle = ChunkSystemTestAccess::lifecycle(manager);
    auto& holder = lifecycle.registry.getOrCreate(-3, 7);
    holder.setCurrentChunk(std::make_unique<world::chunk::ChunkPrimer>(-3, 7));
    auto data = holder.getCurrentChunk()->toChunkData();
    data->setDirty(false);
    ASSERT_NE(lifecycle.registry.publish(-3, 7, std::move(data), holder), nullptr);
    manager.tick();
    manager.tick();
    EXPECT_FALSE(manager.hasChunkInMem(-3, 7));
    EXPECT_EQ(lifecycle.registry.find(-3, 7), nullptr);
}

TEST(ChunkRuntimeBoundaryTest, InvalidGenerationStepIsUnreachable)
{
    ChunkStepExecutor executor(
        std::make_unique<FlatChunkGenerator>(0, FlatLevelGeneratorSettings::createDefault()),
        0,
        [](WorldGenRegion&) {},
        [](world::chunk::ChunkPrimer&, WorldGenRegion&) {});
    world::chunk::ChunkPrimer primer(0, 0);
    WorldGenRegion region(0, 0, 0, {&primer}, 0);
    world::chunk::ChunkStatus invalid("invalid",
        world::chunk::ChunkStatuses::FULL_ORDINAL + 1,
        &world::chunk::ChunkStatuses::FULL,
        world::chunk::HeightmapFlag::NONE,
        world::chunk::ChunkType::PROTOCHUNK);
    EXPECT_DEATH(executor.execute(primer, invalid, region), "Unexpected chunk generation step");
}
