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

#include "server/world/chunk/lifecycle/ChunkLifecycleController.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/chunk/base/completion/ChunkRequestCompletion.hpp"
#include "server/world/chunk/base/pipeline/ChunkPyramid.hpp"
#include "server/world/chunk/base/request/ChunkRequestPriority.hpp"
#include "server/world/chunk/lifecycle/generation/ChunkProgressionTask.hpp"
#include "server/world/chunk/lifecycle/state/ChunkRegistry.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <utility>
#include <spdlog/spdlog.h>
using namespace mc::trace;

namespace mc::server {
using namespace mc::world::chunk;
namespace ChunkStatuses = mc::world::chunk::ChunkStatuses;

ChunkLifecycleController::ChunkLifecycleController(ServerWorld* world, std::unique_ptr<IChunkGenerator> generator)
    : demand()
    , registry(
          [this](ChunkCoord x, ChunkCoord z) {
              return demand.getChunkLevel(x, z);
          },
          [this](Holder& holder) {
              unloader.updateCandidate(holder);
          })
    , integration(world, registry, demand)
    , steps(
          std::move(generator),
          integration.dimension(),
          [this](WorldGenRegion& region) {
              integration.configureRegion(region);
          },
          [this](ChunkPrimer& chunk, WorldGenRegion& region) {
              integration.lightGeneration(chunk, region);
          })
    , standaloneArea(world ? nullptr : std::make_unique<AreaTaskScheduler>(nullptr))
    , area(world ? world->areaTasks() : *standaloneArea)
    , scheduler(registry,
          steps,
          {[this](Holder& holder) {
               _resolveStorageForScheduling(holder);
           },
              [this](Holder& holder, ChunkPrimer& primer) {
                  _publishFull(holder, primer);
              }},
          nullptr,
          area)
    , loader(
          registry,
          integration,
          [world]() {
              return world && world->isStorageOpen() ? &world->storage() : nullptr;
          },
          integration.dimension(),
          [this](Holder& holder, const Holder::EnqueueDecision& decision) {
              _advanceChunkState(holder, decision);
          },
          [this](Holder& holder) {
              _notifyStorageReady(holder);
          },
          [this](Holder& holder) {
              scheduler.onChunkGenFailed(holder);
          })
    , unloader(
          registry,
          demand,
          scheduler,
          steps,
          loader,
          integration,
          [world]() {
              return world && world->isStorageOpen() ? &world->storage() : nullptr;
          },
          integration.dimension(),
          [this](u64 key) {
              return m_deferredTicketLevels.contains(key);
          })
    , m_world(world)
{
    demand.setLevelChangeCallback([this](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
        onTicketLevelChanged(x, z, oldLevel, newLevel);
    });
}

ChunkLifecycleController::~ChunkLifecycleController()
{
    shutdown();
}

void ChunkLifecycleController::setWorkerPool(util::UniversalWorkerPool* pool)
{
    if (m_world) {
        m_world->setComputeExecutor(pool);
    } else {
        area.setExecutor(pool);
    }
    m_workerPool.store(pool, std::memory_order::release);
    scheduler.setExecutor(pool);
}

void ChunkLifecycleController::stopScheduling()
{
    m_shuttingDown.store(true, std::memory_order::release);
    scheduler.setShuttingDown();
    loader.stop();
    unloader.stop();
}

void ChunkLifecycleController::shutdown()
{
    if (m_shuttingDown.exchange(true, std::memory_order::acq_rel) && registry.holderCount() == 0) {
        return;
    }
    stopScheduling();
    m_deferredTicketLevels.clear();
    auto holders = registry.holders();
    for (auto& holder : holders) {
        auto lock = scheduler.schedulingLockArea().lock(holder->x(), holder->z(), 0);
        holder->cancelActiveWork();
    }
    area.shutdown();
    if (auto* pool = computeExecutor(); pool && pool->isRunning()) {
        pool->waitForCompletion();
    }
    for (auto& holder : holders) {
        scheduler.cancelGeneration(*holder);
    }
    registry.clear();
    integration.clear();
    loader.clear();
    unloader.clear();
}

ChunkData* ChunkLifecycleController::requestChunkSync(ChunkCoord x, ChunkCoord z, const ChunkStatus& targetStatus)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk,
        "ChunkLifecycleController::requestChunkSync",
        "x",
        x,
        "z",
        z,
        "targetStatus",
        targetStatus.name(),
        [flow = ::perfetto::Flow::ProcessScoped(ChunkPos(x, z).toId())](::perfetto::EventContext ctx) {
            flow(ctx);
        });

    if (ChunkData* chunk = registry.chunk(x, z).get()) {
        return chunk;
    }
    auto promise = std::make_shared<std::promise<ChunkData*>>();
    auto future = promise->get_future();
    submit(x, z, targetStatus, {}, promise);
    while (future.wait_for(std::chrono::milliseconds(1)) != std::future_status::ready) {
        loader.drain();
    }
    return future.get();
}

void ChunkLifecycleController::submit(ChunkCoord x,
    ChunkCoord z,
    const ChunkStatus& targetStatus,
    ChunkCallback callback,
    std::shared_ptr<std::promise<ChunkData*>> promise)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk,
        "ChunkLifecycleController::submit",
        "x",
        x,
        "z",
        z,
        "targetStatus",
        targetStatus.name());

    if (ChunkData* chunk = registry.chunk(x, z).get()) {
        std::vector<ChunkRequestWaiter> waiters;
        waiters.emplace_back(ChunkRequestWaiter{std::move(callback), std::move(promise), &targetStatus});
        completeChunkRequests(std::move(waiters), true, chunk);
        return;
    }
    auto* scheduler = &this->scheduler;
    auto lock = scheduler->schedulingLockArea().lock(x, z, 0);
    if (m_shuttingDown.load(std::memory_order::acquire)) {
        lock.reset();
        std::vector<ChunkRequestWaiter> waiters;
        waiters.push_back({std::move(callback), std::move(promise), &targetStatus});
        completeChunkRequests(std::move(waiters), false, nullptr);
        return;
    }
    (void)registry.getOrCreate(x, z);
    auto lifecycleManager = registry.findShared(x, z);
    MC_ASSERT_RELEASE(lifecycleManager != nullptr);
    const i32 priority = chunkRequestPriority(x, z, targetStatus, demand.getChunkLevel(x, z));
    auto decision = lifecycleManager->submitRequest(targetStatus, priority, std::move(callback), std::move(promise));
    lock.reset();
    _advanceChunkState(*lifecycleManager, decision);
}

void ChunkLifecycleController::_advanceChunkState(
    ChunkHolder& lifecycleManager, const ChunkHolder::EnqueueDecision& decision)
{
    if (lifecycleManager.hasFailedGeneration()) {
        completeChunkRequests(lifecycleManager.takeAllWaiters(), false, nullptr);
        return;
    }
    if (decision.shouldWakeReadyWaiters) {
        registry.completeReady(lifecycleManager);
    }

    if (decision.shouldResolveStorage) {
        loader.resolve(lifecycleManager);
        return;
    }

    if (decision.shouldScheduleGeneration) {
        _scheduleGeneration(lifecycleManager);
    }
}

void ChunkLifecycleController::_resolveStorageForScheduling(ChunkHolder& lifecycleManager)
{
    const i32 priority = chunkRequestPriority(
        lifecycleManager.x(), lifecycleManager.z(), lifecycleManager.requestedGenStatus(), lifecycleManager.level());
    _advanceChunkState(lifecycleManager, lifecycleManager.prepareGeneration(priority));
}

void ChunkLifecycleController::_scheduleGeneration(ChunkHolder& lifecycleManager)
{
    ChunkGenerationScheduler* scheduler = &this->scheduler;
    const i32 lockRadius = 2 * ChunkGenerationScheduler::getMaxAccessRadius();
    if (scheduler->schedulingLockArea().isHeldByCurrentThread(lifecycleManager.x(), lifecycleManager.z(), lockRadius)) {
        scheduler->schedule(lifecycleManager.x(), lifecycleManager.z(), lifecycleManager);
        return;
    }
    if (auto* pool = computeExecutor(); pool != nullptr && pool->isRunning()) {
        steps.noteActivity();
        auto holder = registry.findShared(lifecycleManager.x(), lifecycleManager.z());
        MC_ASSERT_RELEASE(holder.get() == &lifecycleManager);
        const auto priority = chunkTaskPriority(holder->requestPriority());
        auto task = std::make_unique<util::FunctionTask>(
            util::TaskType::ChunkGenerate,
            "Schedule chunk generation",
            [this, scheduler, holder](const std::atomic<bool>& abortSignal) {
                if (abortSignal.load(std::memory_order::acquire) || scheduler->isShuttingDown()) {
                    return false;
                }
                const i32 radius = 2 * ChunkGenerationScheduler::getMaxAccessRadius();
                auto lock = scheduler->schedulingLockArea().lock(holder->x(), holder->z(), radius);
                if (scheduler->isShuttingDown() || registry.findShared(holder->x(), holder->z()) != holder) {
                    return false;
                }
                scheduler->schedule(holder->x(), holder->z(), *holder);
                return true;
            },
            "server.chunk");
        pool->submit(std::move(task), {}, priority, nullptr);
        return;
    }
    auto lock = scheduler->schedulingLockArea().lock(lifecycleManager.x(), lifecycleManager.z(), lockRadius);
    scheduler->schedule(lifecycleManager.x(), lifecycleManager.z(), lifecycleManager);
}

void ChunkLifecycleController::_notifyStorageReady(ChunkHolder& lifecycleManager)
{
    auto* scheduler = &this->scheduler;
    if (auto* pool = computeExecutor(); pool != nullptr && pool->isRunning()) {
        auto holder = registry.findShared(lifecycleManager.x(), lifecycleManager.z());
        MC_ASSERT_RELEASE(holder.get() == &lifecycleManager);
        auto task = std::make_unique<util::FunctionTask>(
            util::TaskType::ChunkLoad,
            "Notify stored chunk ready",
            [scheduler, holder](const std::atomic<bool>& abortSignal) {
                if (abortSignal.load(std::memory_order::acquire) || scheduler->isShuttingDown()) {
                    return false;
                }
                scheduler->onLoadedFromStorageReady(*holder);
                return true;
            },
            "server.chunk");
        pool->submit(std::move(task), {}, util::TaskPriority::High, nullptr);
        return;
    }
    scheduler->onLoadedFromStorageReady(lifecycleManager);
}

void ChunkLifecycleController::onTicketLevelChanged(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel)
{
    MC_UNUSED(oldLevel);
    auto holder = registry.findShared(x, z);
    if (holder == nullptr && newLevel > world::chunk::ChunkDemandController::MAX_LOADED_LEVEL) {
        m_deferredTicketLevels.erase(posToKey(x, z));
        return;
    }
    if (holder == nullptr) {
        (void)registry.getOrCreate(x, z);
        holder = registry.findShared(x, z);
    }
    MC_ASSERT_RELEASE(holder != nullptr);
    ChunkGenerationScheduler* scheduler = &this->scheduler;
    const i32 previousLevel = holder->level();
    {
        const i32 lockRadius = 2 * ChunkGenerationScheduler::getMaxAccessRadius();
        auto lock = scheduler->schedulingLockArea().tryLock(x, z, lockRadius);
        if (!lock) {
            m_deferredTicketLevels[posToKey(x, z)] = newLevel;
            return;
        }
        m_deferredTicketLevels.erase(posToKey(x, z));
        holder->setLevel(newLevel);
        const auto snapshot = holder->generationSnapshot();
        const bool reachedTarget = snapshot.requestedGenStatus == nullptr ||
            (snapshot.currentChunk != nullptr && snapshot.currentGenStatus->isAtLeast(*snapshot.requestedGenStatus));
        if (auto* task = holder->generationTask(); task != nullptr && reachedTarget && !task->hasStarted()) {
            holder->signalGenerationCancellation();
        }
    }
    if (demand.hasTicket(
            world::chunk::TicketTypes::UNLOAD_COOLDOWN, x, z, ChunkPyramid::maxLevel(), world::chunk::Unit{})) {
        unloader.noteCooldown(x, z);
    }
    if (newLevel > previousLevel) {
        if (auto* pool = computeExecutor(); pool != nullptr && pool->isRunning()) {
            auto task = std::make_unique<util::FunctionTask>(
                util::TaskType::ChunkGenerate,
                "Reconcile chunk generation demand",
                [scheduler, holder](const std::atomic<bool>& abortSignal) {
                    if (abortSignal.load(std::memory_order::acquire) || scheduler->isShuttingDown()) {
                        return false;
                    }
                    scheduler->reconcileGenerationDemand(holder->x(), holder->z());
                    return true;
                },
                "server.chunk");
            pool->submit(std::move(task), {}, util::TaskPriority::High, nullptr);
        } else {
            scheduler->reconcileGenerationDemand(x, z);
        }
    }
    if (newLevel <= world::chunk::ChunkDemandController::MAX_LOADED_LEVEL) {
        const i32 priority = chunkRequestPriority(x, z, holder->requestedGenStatus(), newLevel);
        _advanceChunkState(*holder, holder->prepareGeneration(priority));
    }
    unloader.updateCandidate(*holder);
}

void ChunkLifecycleController::_drainDeferredTicketLevels()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk, "ChunkLifecycleController::drainDeferredTicketLevels");
    std::unordered_map<u64, i32> pending;
    pending.swap(m_deferredTicketLevels);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(1);
    for (auto it = pending.begin(); it != pending.end();) {
        if (std::chrono::steady_clock::now() >= deadline) {
            break;
        }
        const auto [key, level] = *it;
        it = pending.erase(it);
        if (!m_deferredTicketLevels.contains(key)) {
            const auto pos = ChunkPos::fromId(key);
            onTicketLevelChanged(pos.x, pos.z, level, level);
        }
    }
    for (const auto& [key, level] : pending) {
        m_deferredTicketLevels.try_emplace(key, level);
    }
}

void ChunkLifecycleController::_publishFull(Holder& holder, ChunkPrimer& primer)
{
    auto entities = std::move(primer.spawnedEntities());
    auto data = primer.toChunkData();
    if (!data) {
        return;
    }
    auto* result = registry.publish(holder.x(), holder.z(), std::move(data), holder);
    if (!result) {
        return;
    }
    if (m_world) {
        integration.enqueue(holder.x(), holder.z(), std::move(entities), true);
    } else {
        integration.deliverStandaloneEntities(entities);
    }
}

void ChunkLifecycleController::processTicketUpdates()
{
    demand.processUpdates();
    _drainDeferredTicketLevels();
}

void ChunkLifecycleController::processPendingChunkTasks()
{
    processTicketUpdates();
    loader.drain();
}

void ChunkLifecycleController::tick()
{
    demand.tick();
    unloader.expireCooldowns();
    processPendingChunkTasks();
    integration.drain();
    integration.postProcessTicking();
    unloader.drain();
    integration.incrementInhabitedTime();
    unloader.processCandidates();
}

void ChunkLifecycleController::forEachLoadedChunk(const std::function<bool(ChunkData&)>& callback) const
{
    for (auto& data : registry.chunks()) {
        if (!callback(*data)) {
            break;
        }
    }
}

void ChunkLifecycleController::forEachTickingChunk(const std::function<bool(ChunkData&)>& callback) const
{
    MC_ASSERT_RELEASE(m_world != nullptr);
    for (auto& data : registry.chunks()) {
        if (demand.isTickingChunk(data->x(), data->z(), m_world->entityManager().simulationDistance()) &&
            !callback(*data)) {
            break;
        }
    }
}

} // namespace mc::server
