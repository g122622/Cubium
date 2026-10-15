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

#include "common/util/thread/ITask.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include "common/world/chunk/gen/ChunkPyramid.hpp"
#include "common/world/chunk/gen/ChunkStep.hpp"
#include "server/world/ChunkTaskScheduler.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/SingleChunkLifecycleManager.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/FlatChunkGenerator.hpp"
#include "server/world/gen/settings/FlatLevelGeneratorSettings.hpp"
#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <gtest/gtest.h>

namespace mc::server {
namespace demand_test {

using world::chunk::SingleChunkLifecycleManager;
namespace ChunkStatuses = world::chunk::ChunkStatuses;
constexpr i32 UNLOADED = static_cast<i32>(world::chunk::ChunkLoadLevel::Unloaded);

class DemandGenerator final : public FlatChunkGenerator {
public:
    DemandGenerator()
        : FlatChunkGenerator(0, FlatLevelGeneratorSettings::createDefault())
    {}

    void generateStructureStarts(WorldGenRegion&, world::chunk::ChunkPrimer&) override
    {
        ++starts;
        if (onStart) {
            onStart();
        }
    }

    std::atomic<i32> starts{0};
    std::function<void()> onStart;
};

class ChunkGenerationDemandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        auto generator = std::make_unique<DemandGenerator>();
        m_generator = generator.get();
        m_manager = std::make_unique<ServerChunkManager>(std::move(generator));
        m_manager->setWorkerPool(&m_pool);
        m_scheduler = m_manager->ensureTaskScheduler();
        m_releaseFuture = m_release.get_future().share();
        m_pool.start();
    }

    void TearDown() override
    {
        release();
        m_manager->shutdown();
        m_pool.shutdown();
        m_manager.reset();
    }

    void release()
    {
        if (!m_released) {
            m_released = true;
            m_release.set_value();
        }
    }

    void blockWorker()
    {
        auto started = std::make_shared<std::promise<void>>();
        auto ready = started->get_future();
        m_pool.submit(std::make_unique<util::FunctionTask>(
                          util::TaskType::Custom,
                          "DemandTestGate",
                          [this, started](const std::atomic<bool>&) {
                              started->set_value();
                              m_releaseFuture.wait();
                              return true;
                          },
                          "worker_pool"),
            nullptr,
            util::TaskPriority::Normal,
            nullptr);
        ASSERT_EQ(ready.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    }

    void scheduleFromTicket(SingleChunkLifecycleManager& holder)
    {
        auto lock = m_scheduler->schedulingLockArea().lock(
            holder.x(), holder.z(), 2 * ChunkTaskScheduler::getMaxAccessRadius());
        holder.setLevel(static_cast<i32>(world::chunk::ChunkLoadLevel::Full));
        m_scheduler->schedule(holder.x(), holder.z(), holder);
    }

    void leave(SingleChunkLifecycleManager& holder)
    {
        {
            auto lock = m_scheduler->schedulingLockArea().lock(
                holder.x(), holder.z(), 2 * ChunkTaskScheduler::getMaxAccessRadius());
            holder.setLevel(UNLOADED);
        }
        m_scheduler->reconcileGenerationDemand(holder.x(), holder.z());
    }

    util::UniversalWorkerPool m_pool{1, "DemandTest", 4096};
    std::unique_ptr<ServerChunkManager> m_manager;
    ChunkTaskScheduler* m_scheduler = nullptr;
    DemandGenerator* m_generator = nullptr;
    std::promise<void> m_release;
    std::shared_future<void> m_releaseFuture;
    bool m_released = false;
};

TEST_F(ChunkGenerationDemandTest, TicketDowngradeLowersTarget)
{
    SingleChunkLifecycleManager holder(0, 0);
    holder.setLevel(33);
    EXPECT_EQ(holder.requestedGenStatus(), ChunkStatuses::FULL);
    holder.setLevel(40);
    EXPECT_EQ(&holder.requestedGenStatus(), world::chunk::ChunkPyramid::generationStatus(40));
    holder.setLevel(UNLOADED);
    EXPECT_FALSE(holder.hasGenerationDemand());
}

TEST_F(ChunkGenerationDemandTest, ExplicitRequestsCompleteAtTheirOwnStatus)
{
    SingleChunkLifecycleManager holder(0, 0);
    holder.setCurrentChunk(std::make_unique<world::chunk::ChunkPrimer>(0, 0));
    holder.submitRequest(ChunkStatuses::STRUCTURE_STARTS, 0, {}, {});
    holder.submitRequest(ChunkStatuses::FULL, 0, {}, {});
    holder.completeStatusTo(ChunkStatuses::STRUCTURE_STARTS);
    auto ready = holder.takeReadyWaiters();
    ASSERT_EQ(ready.size(), 1);
    EXPECT_EQ(ready.front().targetStatus, &ChunkStatuses::STRUCTURE_STARTS);
    EXPECT_EQ(holder.waiterCount(), 1);
    EXPECT_EQ(holder.requestedGenStatus(), ChunkStatuses::FULL);
    holder.takeAllWaiters();
    EXPECT_FALSE(holder.hasGenerationDemand());
}

TEST_F(ChunkGenerationDemandTest, QueuedCancellationRetainsTaskUntilAcknowledged)
{
    blockWorker();
    auto& holder = m_scheduler->getOrCreateHolder(0, 0);
    scheduleFromTicket(holder);
    auto* task = holder.generationTask();
    ASSERT_NE(task, nullptr);
    auto token = holder.abortSignal();
    leave(holder);
    EXPECT_TRUE(token->load());
    EXPECT_EQ(holder.generationTask(), task);
    EXPECT_FALSE(holder.isSafeToUnload());
    release();
    m_pool.waitForCompletion();
    EXPECT_FALSE(holder.hasGenerationTask());
    EXPECT_TRUE(holder.isSafeToUnload());
    EXPECT_EQ(holder.getCurrentChunk(), nullptr);
    EXPECT_EQ(m_generator->starts.load(), 0);
    EXPECT_FALSE(holder.hasFailedGeneration());
}

TEST_F(ChunkGenerationDemandTest, ReturningDemandWaitsForOldCancellation)
{
    blockWorker();
    auto& holder = m_scheduler->getOrCreateHolder(0, 0);
    scheduleFromTicket(holder);
    auto token = holder.abortSignal();
    auto* task = holder.generationTask();
    leave(holder);
    auto requested = m_manager->requestChunkAsync(0, 0, ChunkStatuses::STRUCTURE_STARTS);
    EXPECT_EQ(holder.generationTask(), task);
    EXPECT_EQ(holder.abortSignal(), token);
    EXPECT_TRUE(token->load());
    release();
    ASSERT_EQ(requested.wait_for(std::chrono::seconds(10)), std::future_status::ready);
    EXPECT_NE(requested.get(), nullptr);
    m_pool.waitForCompletion();
    EXPECT_EQ(m_generator->starts.load(), 1);
    EXPECT_EQ(holder.getCurrentGenStatus(), ChunkStatuses::STRUCTURE_STARTS);
    EXPECT_FALSE(holder.hasFailedGeneration());
    EXPECT_TRUE(holder.isSafeToUnload());
}

TEST_F(ChunkGenerationDemandTest, QueuedCancellationRetainsNeighbourReferences)
{
    blockWorker();
    const auto& step = world::chunk::ChunkPyramid::generationPyramid().getStepTo(ChunkStatuses::STRUCTURE_REFERENCES);
    const i32 radius = step.neighbourReadRadius();
    ASSERT_GT(radius, 0);
    for (i32 z = -radius; z <= radius; ++z) {
        for (i32 x = -radius; x <= radius; ++x) {
            auto& neighbour = m_scheduler->getOrCreateHolder(x, z);
            neighbour.setCurrentChunk(std::make_unique<world::chunk::ChunkPrimer>(x, z));
            neighbour.completeStatusTo(x == 0 && z == 0 ? ChunkStatuses::STRUCTURE_STARTS : ChunkStatuses::FULL);
        }
    }
    auto& holder = m_scheduler->getOrCreateHolder(0, 0);
    {
        auto lock = m_scheduler->schedulingLockArea().lock(0, 0, 2 * ChunkTaskScheduler::getMaxAccessRadius());
        holder.setLevel(33);
        holder.prepareGeneration(0);
        holder.noteStorageMissing();
        m_scheduler->schedule(0, 0, holder);
    }
    auto& neighbour = m_scheduler->getOrCreateHolder(1, 0);
    ASSERT_EQ(neighbour.neighboursUsingThisChunkCount(), 1);
    leave(holder);
    EXPECT_EQ(neighbour.neighboursUsingThisChunkCount(), 1);
    EXPECT_FALSE(neighbour.isSafeToUnload());
    release();
    m_pool.waitForCompletion();
    EXPECT_EQ(neighbour.neighboursUsingThisChunkCount(), 0);
    EXPECT_TRUE(neighbour.isSafeToUnload());
    EXPECT_EQ(holder.getCurrentGenStatus(), ChunkStatuses::STRUCTURE_STARTS);
}

TEST_F(ChunkGenerationDemandTest, StartedStageFinishesWithoutContinuingOldTarget)
{
    auto started = std::make_shared<std::promise<void>>();
    auto ready = started->get_future();
    m_generator->onStart = [this, started]() {
        started->set_value();
        m_releaseFuture.wait();
    };
    auto& holder = m_scheduler->getOrCreateHolder(0, 0);
    scheduleFromTicket(holder);
    ASSERT_EQ(ready.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    auto token = holder.abortSignal();
    leave(holder);
    EXPECT_FALSE(token->load());
    EXPECT_TRUE(holder.hasGenerationTask());
    release();
    m_pool.waitForCompletion();
    EXPECT_EQ(m_generator->starts.load(), 1);
    EXPECT_EQ(holder.getCurrentGenStatus(), ChunkStatuses::STRUCTURE_STARTS);
    EXPECT_FALSE(holder.hasGenerationTask());
    EXPECT_TRUE(holder.isSafeToUnload());
    EXPECT_FALSE(m_manager->hasChunkInMem(0, 0));
}

TEST_F(ChunkGenerationDemandTest, LeavingPreservesExplicitRequest)
{
    blockWorker();
    auto requested = m_manager->requestChunkAsync(0, 0, ChunkStatuses::STRUCTURE_STARTS);
    auto& holder = m_scheduler->getOrCreateHolder(0, 0);
    auto token = holder.abortSignal();
    leave(holder);
    EXPECT_FALSE(token->load());
    release();
    ASSERT_EQ(requested.wait_for(std::chrono::seconds(10)), std::future_status::ready);
    EXPECT_NE(requested.get(), nullptr);
    m_pool.waitForCompletion();
    EXPECT_EQ(m_generator->starts.load(), 1);
}

TEST_F(ChunkGenerationDemandTest, SharedNeighbourKeepsOtherCentresDemand)
{
    auto& expired = m_scheduler->getOrCreateHolder(0, 0);
    auto& active = m_scheduler->getOrCreateHolder(1, 0);
    auto& shared = m_scheduler->getOrCreateHolder(0, 1);
    expired.addBlockingNeighbour(&shared);
    active.addBlockingNeighbour(&shared);
    shared.addWaitingNeighbour(&expired, &ChunkStatuses::BIOMES);
    shared.addWaitingNeighbour(&active, &ChunkStatuses::STRUCTURE_STARTS);
    leave(expired);
    EXPECT_TRUE(expired.isSafeToUnload());
    EXPECT_EQ(shared.requestedGenStatus(), ChunkStatuses::STRUCTURE_STARTS);
    EXPECT_TRUE(active.isWaitingForNeighbors());
    leave(active);
    EXPECT_FALSE(shared.hasGenerationDemand());
    EXPECT_TRUE(shared.isSafeToUnload());
    EXPECT_TRUE(active.isSafeToUnload());
}

} // namespace demand_test
} // namespace mc::server
