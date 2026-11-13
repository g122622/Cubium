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

#include "server/world/chunk/lifecycle/unloading/ChunkUnloadController.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "server/world/chunk/base/completion/ChunkCompletionQueue.hpp"
#include "server/world/chunk/base/completion/ChunkRequestCompletion.hpp"
#include "server/world/chunk/base/pipeline/ChunkPyramid.hpp"
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

ChunkUnloadController::ChunkUnloadController(ChunkRegistry& registry,
    world::chunk::ChunkDemandController& demand,
    ChunkGenerationScheduler& scheduler,
    ChunkStepExecutor& steps,
    ChunkSourceLoader& loader,
    IChunkWorldLifecycle& integration,
    std::function<world::storage::SingleLevelStorageManager*()> storage,
    DimensionId dimension,
    std::function<bool(u64)> deferred)
    : m_registry(registry)
    , m_ticketManager(demand)
    , m_scheduler(scheduler)
    , m_steps(steps)
    , m_loader(loader)
    , m_integration(integration)
    , m_storage(std::move(storage))
    , m_dimension(dimension)
    , m_deferred(std::move(deferred))
{}

void ChunkUnloadController::request(ChunkCoord x, ChunkCoord z)
{
    const auto key = posToKey(x, z);
    if (m_deferred(key)) {
        return;
    }
    auto holder = m_registry.findShared(x, z);
    {
        std::lock_guard lock(m_unloadStateMutex);
        if (!m_unloadSaveInProgress.insert(key).second) {
            return;
        }
    }
    auto chunk = m_registry.chunk(x, z);
    auto* storage = m_storage();
    if (storage && holder && chunk && chunk->isDirty()) {
        chunk->setDirty(false);
        storage->saveChunkAsyncCallback(std::move(chunk),
            m_dimension,
            [completion = m_finished, dimension = m_dimension, x, z, holder](
                std::optional<ChunkPos>, const Result<void>& result) {
                if (result.failed()) {
                    spdlog::error("Async save chunk failed ({}, {}): {}", x, z, result.error().message());
                }
                completion->push({x, z, dimension, holder, result.success()});
            });
        return;
    }
    m_finished->push({x, z, m_dimension, std::move(holder), true});
}

void ChunkUnloadController::_finalizeUnloadAfterSave(
    ChunkCoord x, ChunkCoord z, DimensionId dimension, std::shared_ptr<Holder> holder, bool saveSucceeded)
{
    (void)dimension;
    const auto key = posToKey(x, z);
    const auto releaseSave = [&] {
        std::lock_guard lock(m_unloadStateMutex);
        m_unloadSaveInProgress.erase(key);
    };
    const auto cooldown = [&] {
        {
            std::lock_guard lock(m_unloadStateMutex);
            m_unloadSaveInProgress.erase(key);
            m_unloadCooldownChunks.insert(key);
        }
        m_ticketManager.registerTicket(TicketTypes::UNLOAD_COOLDOWN, x, z, ChunkPyramid::maxLevel(), Unit{});
    };
    const auto retry = [&] {
        m_finished->push({x, z, dimension, holder, saveSucceeded});
    };
    // 旧保存结果仅影响其原生命周期；不能恢复新实例脏标记或移除新实例。
    if (m_registry.findShared(x, z) != holder) {
        releaseSave();
        return;
    }
    if (!saveSucceeded) {
        if (auto chunk = m_registry.chunk(x, z)) {
            chunk->setDirty(true);
        }
        cooldown();
        return;
    }
    if (m_deferred(key)) {
        retry();
        return;
    }
    if (holder && _isUnloadSuppressed(*holder)) {
        releaseSave();
        return;
    }
    auto lock = m_scheduler.schedulingLockArea().tryLock(x, z, 2 * ChunkGenerationScheduler::getMaxAccessRadius());
    if (!lock) {
        retry();
        return;
    }
    if (m_registry.findShared(x, z) != holder || (holder && _isUnloadSuppressed(*holder))) {
        releaseSave();
        return;
    }
    if (auto chunk = m_registry.chunk(x, z); chunk && chunk->isDirty()) {
        releaseSave();
        if (holder) {
            updateCandidate(*holder);
        }
        return;
    }
    if (holder && !holder->isSafeToUnload()) {
        cooldown();
        return;
    }
    if (holder) {
        m_scheduler.cancelGeneration(*holder);
        holder->cancelActiveWork();
        completeChunkRequests(holder->takeAllWaiters(), false, nullptr);
    }
    // 世界退场仍能访问区块；最终移除与请求登记共用调度区域锁。
    m_integration.notifyUnload(x, z);
    MC_ASSERT_RELEASE(m_registry.retire(x, z, holder));
    m_integration.forget(x, z);
    m_loader.forget(x, z, holder);
    releaseSave();
}

void ChunkUnloadController::drain()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk, "ChunkUnloadController::drainPendingUnloadFinishes");
    m_finished->drain(std::chrono::milliseconds(1), [this](auto& item) {
        _finalizeUnloadAfterSave(item.x, item.z, item.dimension, std::move(item.lifecycleHolder), item.saveSucceeded);
    });
}

void ChunkUnloadController::processCandidates()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk, "ChunkUnloadController::processChunkUnloads");
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(1);
    const bool generationIdle = _isGenerationIdle();
    const bool wasArmed = m_idleHaloReclaimArmed;
    m_idleHaloReclaimArmed = generationIdle;
    if (m_idleHaloReclaimArmed && !wasArmed) {
        _enqueueIdleHaloReclaim();
    }

    size_t pending = 0;
    {
        std::lock_guard<std::mutex> lock(m_unloadQueueMutex);
        pending = m_inUnloadQueue.size();
    }
    if (pending == 0) {
        return;
    }
    const size_t batch = std::max<size_t>(
        MIN_CHUNK_UNLOAD_COUNT, static_cast<size_t>(std::ceil(static_cast<f64>(pending) * MIN_CHUNK_UNLOAD_FRACTION)));

    size_t unloaded = 0;
    while (unloaded < batch) {
        if (std::chrono::steady_clock::now() >= deadline) {
            break;
        }
        u64 key = 0;
        if (!_tryPopUnloadCandidate(key)) {
            break;
        }
        const ChunkPos chunkId = ChunkPos::fromId(key);
        auto lifecycleHolder = m_registry.findShared(chunkId.x, chunkId.z);
        if (lifecycleHolder == nullptr || !_isUnloadCandidate(*lifecycleHolder)) {
            continue;
        }

        request(chunkId.x, chunkId.z);
        ++unloaded;
    }
}

void ChunkUnloadController::updateCandidate(ChunkHolder& holder)
{
    if (m_shuttingDown.load(std::memory_order::acquire)) {
        return;
    }

    const u64 key = posToKey(holder.x(), holder.z());
    const bool isCandidate = _isUnloadCandidate(holder);

    std::lock_guard<std::mutex> lock(m_unloadQueueMutex);
    if (isCandidate) {
        if (m_inUnloadQueue.insert(key).second) {
            m_unloadQueue.push_back(key);
        }
    } else {
        m_inUnloadQueue.erase(key);
    }
}

bool ChunkUnloadController::_isUnloadSuppressed(const ChunkHolder& holder) const
{
    {
        std::lock_guard<std::mutex> lock(m_unloadStateMutex);
        if (m_unloadCooldownChunks.contains(posToKey(holder.x(), holder.z()))) {
            return true;
        }
    }
    const i32 level = holder.level();
    if (level <= world::chunk::ChunkDemandController::MAX_LOADED_LEVEL) {
        return true; // 加载环内：必须保留
    }
    if (level < static_cast<i32>(world::chunk::ChunkLoadLevel::Unloaded) && !m_idleHaloReclaimArmed) {
        return true;
    }
    return false;
}

void ChunkUnloadController::expireCooldowns()
{
    std::vector<u64> cooldownKeys;
    {
        std::lock_guard<std::mutex> lock(m_unloadStateMutex);
        cooldownKeys.assign(m_unloadCooldownChunks.begin(), m_unloadCooldownChunks.end());
    }
    for (const u64 key : cooldownKeys) {
        const auto chunkId = ChunkPos::fromId(key);
        const ChunkCoord x = chunkId.x;
        const ChunkCoord z = chunkId.z;
        if (m_ticketManager.hasTicket(
                world::chunk::TicketTypes::UNLOAD_COOLDOWN, x, z, ChunkPyramid::maxLevel(), world::chunk::Unit{})) {
            continue;
        }
        {
            std::lock_guard<std::mutex> lock(m_unloadStateMutex);
            m_unloadCooldownChunks.erase(key);
        }
        if (auto holder = m_registry.findShared(x, z)) {
            updateCandidate(*holder);
        }
    }
}

bool ChunkUnloadController::_isUnloadCandidate(const ChunkHolder& holder)
{
    if (m_suppressAutoUnload) {
        return false;
    }
    if (_isUnloadSuppressed(holder) || !holder.isSafeToUnload()) {
        return false;
    }

    const u64 key = posToKey(holder.x(), holder.z());
    if (m_ticketManager.hasTrackingPlayers(key)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_unloadStateMutex);
    return m_unloadSaveInProgress.count(key) == 0;
}

bool ChunkUnloadController::_tryPopUnloadCandidate(u64& key)
{
    std::lock_guard<std::mutex> lock(m_unloadQueueMutex);
    while (!m_unloadQueue.empty()) {
        key = m_unloadQueue.front();
        m_unloadQueue.pop_front();
        if (m_inUnloadQueue.erase(key) > 0) {
            return true;
        }
    }
    return false;
}

bool ChunkUnloadController::_isGenerationIdle()
{
    const auto last = m_steps.lastActivity();
    const auto now = util::TimeUtils::getCurrentTimeMs();
    if (last != 0 && now - last < UNLOAD_IDLE_DEBOUNCE_MS) {
        return false;
    }
    if (m_loader.pending() || m_integration.pending()) {
        return false;
    }
    return !m_finished->pending();
}

void ChunkUnloadController::_enqueueIdleHaloReclaim()
{
    for (auto& holder : m_registry.holders()) {
        const auto level = holder->level();
        if (level > world::chunk::ChunkDemandController::MAX_LOADED_LEVEL &&
            level < static_cast<i32>(ChunkLoadLevel::Unloaded)) {
            updateCandidate(*holder);
        }
    }
}

void ChunkUnloadController::noteCooldown(ChunkCoord x, ChunkCoord z)
{
    std::lock_guard lock(m_unloadStateMutex);
    m_unloadCooldownChunks.insert(posToKey(x, z));
}

void ChunkUnloadController::stop()
{
    m_shuttingDown.store(true, std::memory_order::release);
    m_finished->close();
}

void ChunkUnloadController::clear()
{
    {
        std::lock_guard lock(m_unloadStateMutex);
        m_finished->close();
        m_unloadSaveInProgress.clear();
        m_unloadCooldownChunks.clear();
    }
    {
        std::lock_guard lock(m_unloadQueueMutex);
        m_unloadQueue.clear();
        m_inUnloadQueue.clear();
    }
}

} // namespace mc::server
