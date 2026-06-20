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

#include "ServerChunkManager.hpp"
#include "ServerWorld.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/gen/ChunkPyramid.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "server/sync/ChunkSendManager.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::server {

using mc::world::chunk::ChunkDependencies;
using mc::world::chunk::ChunkPyramid;
using mc::world::chunk::ChunkStatus;
namespace ChunkStatuses = mc::world::chunk::ChunkStatuses;
using mc::world::chunk::SingleChunkLifecycleManager;

namespace {

/**
 * @brief 统一完成等待者集合
 *
 * @param waiters 待完成等待者
 * @param success 是否成功
 * @param chunk 返回区块
 */
void fulfillWaiters(std::vector<SingleChunkLifecycleManager::Waiter> waiters, bool success, ChunkData* chunk)
{
    for (auto& waiter : waiters) {
        if (waiter.promise) {
            waiter.promise->set_value(success ? chunk : nullptr);
        }
        if (waiter.callback) {
            waiter.callback(success, success ? chunk : nullptr);
        }
    }
}

[[nodiscard]] bool chunkHasCompletedStatus(const IChunk& chunk, const ChunkStatus& status)
{
    if (const auto* primer = dynamic_cast<const ChunkPrimer*>(&chunk)) {
        return primer->hasCompletedStatus(status);
    }
    return dynamic_cast<const ChunkData*>(&chunk) != nullptr && ChunkStatuses::FULL.isAtLeast(status);
}

void assertRegionSatisfiesDirectDependencies(
    const std::vector<IChunk*>& neighbors, ChunkCoord centerX, ChunkCoord centerZ, i32 radius, const ChunkStep& step)
{
    const i32 diameter = radius * 2 + 1;
    MC_ASSERT_RELEASE(static_cast<i32>(neighbors.size()) == diameter * diameter);

    const ChunkDependencies& deps = step.directDependencies();
    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            const i32 distance = std::max(std::abs(dx), std::abs(dz));
            const ChunkStatus* requiredStatus = deps.get(distance);
            if (requiredStatus == nullptr) {
                continue;
            }

            const size_t index = static_cast<size_t>((dz + radius) * diameter + (dx + radius));
            const IChunk* chunk = neighbors[index];
            MC_ASSERT_RELEASE_MSG(chunk != nullptr, "WorldGenRegion direct dependency chunk is missing");
            MC_ASSERT_RELEASE(chunk->x() == centerX + dx);
            MC_ASSERT_RELEASE(chunk->z() == centerZ + dz);
            MC_ASSERT_RELEASE_MSG(
                chunkHasCompletedStatus(*chunk, *requiredStatus), "WorldGenRegion direct dependency status is too low");
        }
    }
}

} // namespace

// ============================================================================
// 构造与析构
// ============================================================================

ServerChunkManager::ServerChunkManager(ServerWorld& world, std::unique_ptr<IChunkGenerator> generator)
    : m_world(&world)
    , m_generator(std::move(generator))
{
    m_ticketManager.setLevelChangeCallback([this](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
        _onTicketLevelChanged(x, z, oldLevel, newLevel);
    });
}

ServerChunkManager::ServerChunkManager(std::unique_ptr<IChunkGenerator> generator)
    : m_world(nullptr)
    , m_generator(std::move(generator))
{
    m_ticketManager.setLevelChangeCallback([this](ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel) {
        _onTicketLevelChanged(x, z, oldLevel, newLevel);
    });
}

ServerChunkManager::~ServerChunkManager()
{
    shutdown();
}

// ============================================================================
// 生命周期
// ============================================================================

Result<void> ServerChunkManager::initialize()
{
    MC_TRACE_EVENT("server.initialization", "ServerChunkManager::initialize");
    return {};
}

void ServerChunkManager::shutdown()
{
    std::vector<std::unique_ptr<SingleChunkLifecycleManager>> lifecycleManagers;
    {
        std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
        lifecycleManagers.reserve(m_lifecycleManagers.size());
        for (auto& [key, lifecycleManager] : m_lifecycleManagers) {
            MC_UNUSED(key);
            if (lifecycleManager) {
                lifecycleManager->cancelActiveWork();
                lifecycleManagers.push_back(std::move(lifecycleManager));
            }
        }
        m_lifecycleManagers.clear();
    }

    for (auto& lifecycleManager : lifecycleManagers) {
        if (lifecycleManager) {
            _failWaiters(lifecycleManager->takeAllWaiters());
        }
    }

    std::lock_guard<std::mutex> lock(m_chunksMutex);
    m_chunks.clear();
}

// ============================================================================
// 内存查询
// ============================================================================

ChunkData* ServerChunkManager::tryToGetChunkInMem(ChunkCoord x, ChunkCoord z)
{
    return tryToGetChunkSharedInMem(x, z).get();
}

const ChunkData* ServerChunkManager::tryToGetChunkInMem(ChunkCoord x, ChunkCoord z) const
{
    return tryToGetChunkSharedInMem(x, z).get();
}

std::shared_ptr<ChunkData> ServerChunkManager::tryToGetChunkSharedInMem(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);
    std::lock_guard<std::mutex> lock(m_chunksMutex);
    auto it = m_chunks.find(key);
    return it != m_chunks.end() ? it->second : std::shared_ptr<ChunkData>{};
}

std::shared_ptr<const ChunkData> ServerChunkManager::tryToGetChunkSharedInMem(ChunkCoord x, ChunkCoord z) const
{
    const u64 key = posToKey(x, z);
    std::lock_guard<std::mutex> lock(m_chunksMutex);
    auto it = m_chunks.find(key);
    return it != m_chunks.end() ? it->second : std::shared_ptr<const ChunkData>{};
}

bool ServerChunkManager::hasChunkInMem(ChunkCoord x, ChunkCoord z) const
{
    return tryToGetChunkInMem(x, z) != nullptr;
}

// ============================================================================
// 请求入口
// ============================================================================

ChunkData* ServerChunkManager::requestChunkSync(ChunkCoord x, ChunkCoord z, const ChunkStatus& targetStatus)
{
    if (ChunkData* chunk = tryToGetChunkInMem(x, z)) {
        return chunk;
    }

    std::lock_guard<std::mutex> generationLock(m_syncGenerationMutex);

    if (ChunkData* chunk = tryToGetChunkInMem(x, z)) {
        return chunk;
    }

    if (m_world && m_world->isStorageOpen()) {
        if (auto loadedChunk = _tryToLoadChunkFromStorageSync(x, z)) {
            SingleChunkLifecycleManager& lifecycleManager = _getOrCreateLifecycleManager(x, z);
            lifecycleManager.markLoadedFromStorageReady();
            return _storeChunkInMemorySync(x, z, std::move(loadedChunk));
        }
    }

    SingleChunkLifecycleManager& lifecycleManager = _getOrCreateLifecycleManager(x, z);
    const i32 priority = _computeSchedulePriority(x, z, targetStatus, m_ticketManager.getChunkLevel(x, z));
    MC_UNUSED(priority);
    _executeGenerationSync(lifecycleManager, targetStatus);
    return tryToGetChunkInMem(x, z);
}

ChunkData* ServerChunkManager::requestFullChunkSync(ChunkCoord x, ChunkCoord z)
{
    return requestChunkSync(x, z, ChunkStatuses::FULL);
}

std::future<ChunkData*> ServerChunkManager::requestChunkAsync(
    ChunkCoord x, ChunkCoord z, const ChunkStatus& targetStatus)
{
    auto promise = std::make_shared<std::promise<ChunkData*>>();
    auto future = promise->get_future();
    _submitChunkRequest(x, z, targetStatus, {}, promise);
    return future;
}

void ServerChunkManager::requestChunkAsync(
    ChunkCoord x, ChunkCoord z, const ChunkStatus& targetStatus, ChunkCallback callback)
{
    _submitChunkRequest(x, z, targetStatus, std::move(callback), {});
}

void ServerChunkManager::_submitChunkRequest(ChunkCoord x,
    ChunkCoord z,
    const ChunkStatus& targetStatus,
    ChunkCallback callback,
    std::shared_ptr<std::promise<ChunkData*>> promise)
{
    MC_TRACE_EVENT(
        "server.chunk", "ServerChunkManager::submitChunkRequest", "x", x, "z", z, "targetStatus", targetStatus.name());

    if (ChunkData* chunk = tryToGetChunkInMem(x, z)) {
        std::vector<SingleChunkLifecycleManager::Waiter> waiters;
        waiters.emplace_back(SingleChunkLifecycleManager::Waiter{std::move(callback), std::move(promise)});
        fulfillWaiters(std::move(waiters), true, chunk);
        return;
    }

    SingleChunkLifecycleManager& lifecycleManager = _getOrCreateLifecycleManager(x, z);
    const i32 priority = _computeSchedulePriority(x, z, targetStatus, m_ticketManager.getChunkLevel(x, z));
    auto decision = lifecycleManager.submitRequest(targetStatus, priority, std::move(callback), std::move(promise));
    _advanceChunkState(lifecycleManager, decision);
}

// ============================================================================
// 状态机推进
// ============================================================================

void ServerChunkManager::_advanceChunkState(
    SingleChunkLifecycleManager& lifecycleManager, const SingleChunkLifecycleManager::EnqueueDecision& decision)
{
    if (decision.shouldWakeReadyWaiters) {
        _completeReadyWaiters(lifecycleManager);
        return;
    }

    if (decision.shouldResolveStorage) {
        _resolveChunkSourceSync(lifecycleManager);
        return;
    }

    if (decision.shouldQueueGeneration) {
        _enqueueChunkGenerationAsync(lifecycleManager, decision);
    }
}

void ServerChunkManager::_resolveChunkSourceSync(SingleChunkLifecycleManager& lifecycleManager)
{
    const ChunkCoord x = lifecycleManager.x();
    const ChunkCoord z = lifecycleManager.z();

    std::unique_ptr<ChunkData> loadedChunk;
    if (m_world && m_world->isStorageOpen()) {
        loadedChunk = _tryToLoadChunkFromStorageSync(x, z);
    }

    auto decision = lifecycleManager.noteStorageResolved(loadedChunk != nullptr);
    if (loadedChunk) {
        lifecycleManager.markLoadedFromStorageReady();
        ChunkData* stored = _storeChunkInMemorySync(x, z, std::move(loadedChunk));
        MC_UNUSED(stored);
        _completeReadyWaiters(lifecycleManager);
        _wakeBlockedNeighborsAsync(x, z);
        return;
    }

    // 使用 ChunkPyramid 的直接依赖模型检查邻居是否就绪
    const ChunkPyramid& pyramid = ChunkPyramid::generationPyramid();
    const ChunkStep& step = pyramid.getStepTo(lifecycleManager.requestedStatus());
    ChunkStepDependencyInfo depInfo = _getDirectDependencyInfo(lifecycleManager.requestedStatus());

    if (depInfo.hasDependencies) {
        decision = lifecycleManager.noteNeighborProgress(_areNeighborsReady(x, z, step));
    } else {
        decision = lifecycleManager.noteNeighborProgress(true);
    }

    _advanceChunkState(lifecycleManager, decision);
}

void ServerChunkManager::_enqueueChunkGenerationAsync(
    SingleChunkLifecycleManager& lifecycleManager, const SingleChunkLifecycleManager::EnqueueDecision& decision)
{
    const ChunkCoord x = lifecycleManager.x();
    const ChunkCoord z = lifecycleManager.z();

    // 同步路径直接生成，避免在没有 worker 池时把请求永久挂起。
    if (m_workerPool == nullptr || !m_workerPool->isRunning()) {
        std::lock_guard<std::mutex> generationLock(m_syncGenerationMutex);
        lifecycleManager.noteGenerationStarted(decision.generation);
        _executeGenerationSync(lifecycleManager, *decision.targetStatus);
        auto completionDecision = lifecycleManager.noteGenerationFinished(
            decision.generation, SingleChunkLifecycleManager::CompletionState::Succeeded);
        MC_UNUSED(completionDecision);
        _completeReadyWaiters(lifecycleManager);
        _wakeBlockedNeighborsAsync(x, z);
        return;
    }

    lifecycleManager.noteGenerationQueued(decision.generation);

    // 计算生成缓存半径
    const ChunkPyramid& pyramid = ChunkPyramid::generationPyramid();
    const ChunkStep& targetStep = pyramid.getStepTo(*decision.targetStatus);
    const i32 cacheRadius = targetStep.accumulatedRadius();

    auto task = std::make_unique<ChunkGenerateTask>(x,
        z,
        *decision.targetStatus,
        [this, cacheRadius](
            ChunkPrimer& chunk, const ChunkStatus& targetStatus, const std::atomic<bool>& cancelSignal) {
            // 注册到生成中 Primer 缓存
            {
                std::lock_guard<std::mutex> lock(m_generatingPrimersMutex);
                m_generatingPrimers[posToKey(chunk.x(), chunk.z())] = &chunk;
            }

            // 创建生成缓存
            GenerationChunkCache cache(chunk.x(), chunk.z(), cacheRadius);
            cache.set(chunk.x(), chunk.z(), &chunk);

            _doGenerateChunkToTargetStatus(chunk, targetStatus, cache);
            if (!cancelSignal.load(std::memory_order_acquire)) {
                _doSpawnInitialMobs(chunk);
            }

            // 从生成中缓存移除
            {
                std::lock_guard<std::mutex> lock(m_generatingPrimersMutex);
                m_generatingPrimers.erase(posToKey(chunk.x(), chunk.z()));
            }
        });

    m_workerPool->submit(
        std::move(task),
        [this, x, z, generation = decision.generation](bool success, util::ITask* task) {
            SingleChunkLifecycleManager* lifecycleManager = _findLifecycleManager(x, z);
            if (!lifecycleManager || !lifecycleManager->isGenerationCurrent(generation)) {
                return;
            }

            lifecycleManager->noteGenerationStarted(generation);

            ChunkData* storedChunk = nullptr;
            auto completionState = SingleChunkLifecycleManager::CompletionState::Failed;

            if (success && task) {
                auto* generationTask = static_cast<ChunkGenerateTask*>(task);
                auto result = generationTask->takeResult();
                if (result) {
                    storedChunk = _finalizeGeneratedChunkSync(x, z, *result);
                    if (storedChunk != nullptr) {
                        completionState = SingleChunkLifecycleManager::CompletionState::Succeeded;
                    }
                }
            }

            auto completionDecision = lifecycleManager->noteGenerationFinished(generation, completionState);
            if (completionState == SingleChunkLifecycleManager::CompletionState::Succeeded) {
                _completeReadyWaiters(*lifecycleManager);
                _wakeBlockedNeighborsAsync(x, z);
            } else {
                MC_UNUSED(completionDecision);
            }
        },
        static_cast<util::TaskPriority>(decision.priority),
        decision.cancelToken);
}

void ServerChunkManager::_completeReadyWaiters(SingleChunkLifecycleManager& lifecycleManager)
{
    ChunkData* chunk = tryToGetChunkInMem(lifecycleManager.x(), lifecycleManager.z());
    fulfillWaiters(lifecycleManager.takeReadyWaiters(), chunk != nullptr, chunk);
}

void ServerChunkManager::_failWaiters(std::vector<SingleChunkLifecycleManager::Waiter> waiters)
{
    fulfillWaiters(std::move(waiters), false, nullptr);
}

// ============================================================================
// 生命周期管理器访问
// ============================================================================

SingleChunkLifecycleManager& ServerChunkManager::_getOrCreateLifecycleManager(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);
    std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
    auto it = m_lifecycleManagers.find(key);
    if (it != m_lifecycleManagers.end()) {
        return *it->second;
    }

    auto lifecycleManager = std::make_unique<SingleChunkLifecycleManager>(x, z);
    auto* ptr = lifecycleManager.get();
    m_lifecycleManagers[key] = std::move(lifecycleManager);
    return *ptr;
}

SingleChunkLifecycleManager* ServerChunkManager::_findLifecycleManager(ChunkCoord x, ChunkCoord z)
{
    return _doFindLifecycleManager(x, z);
}

const SingleChunkLifecycleManager* ServerChunkManager::_findLifecycleManager(ChunkCoord x, ChunkCoord z) const
{
    return _doFindLifecycleManager(x, z);
}

SingleChunkLifecycleManager* ServerChunkManager::_doFindLifecycleManager(ChunkCoord x, ChunkCoord z) const
{
    const u64 key = posToKey(x, z);
    std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
    auto it = m_lifecycleManagers.find(key);
    return it != m_lifecycleManagers.end() ? it->second.get() : nullptr;
}

ChunkPrimer* ServerChunkManager::getGeneratingPrimer(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);
    std::lock_guard<std::mutex> lock(m_generatingPrimersMutex);
    auto it = m_generatingPrimers.find(key);
    return it != m_generatingPrimers.end() ? it->second : nullptr;
}

bool ServerChunkManager::hasGeneratingPrimer(ChunkCoord x, ChunkCoord z) const
{
    const u64 key = posToKey(x, z);
    std::lock_guard<std::mutex> lock(m_generatingPrimersMutex);
    return m_generatingPrimers.find(key) != m_generatingPrimers.end();
}

// ============================================================================
// 票据与唤醒
// ============================================================================

void ServerChunkManager::_onTicketLevelChanged(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel)
{
    MC_UNUSED(oldLevel);
    MC_TRACE_EVENT("server.chunk",
        "ServerChunkManager::onTicketLevelChanged",
        "x",
        x,
        "z",
        z,
        "oldLevel",
        oldLevel,
        "newLevel",
        newLevel);

    SingleChunkLifecycleManager& lifecycleManager = _getOrCreateLifecycleManager(x, z);
    lifecycleManager.setLevel(newLevel);

    if (newLevel <= world::chunk::ChunkLoadTicketManager::MAX_LOADED_LEVEL) {
        // 根据票据级别推导目标生成状态
        const ChunkStatus* levelStatus = ChunkPyramid::generationStatus(newLevel);
        const ChunkStatus& targetStatus = (levelStatus && levelStatus->ordinal() > ChunkStatuses::EMPTY.ordinal())
            ? *levelStatus
            : ChunkStatuses::FULL;

        // 如果已请求更高状态，保持更高状态
        const ChunkStatus& existingRequested = lifecycleManager.requestedStatus();
        const ChunkStatus& effectiveTarget =
            (existingRequested.ordinal() > targetStatus.ordinal()) ? existingRequested : targetStatus;
        _submitChunkRequest(x, z, effectiveTarget, {}, {});
        return;
    }

    lifecycleManager.cancelActiveWork();
    _failWaiters(lifecycleManager.takeAllWaiters());
}

void ServerChunkManager::_wakeBlockedNeighborsAsync(ChunkCoord x, ChunkCoord z)
{
    MC_TRACE_EVENT("server.chunk", "ServerChunkManager::wakeBlockedNeighborsAsync");

    // 唤醒范围使用 ChunkPyramid 的最大累积依赖半径
    const ChunkPyramid& pyramid = ChunkPyramid::generationPyramid();
    i32 maxRadius = 0;
    for (const auto& step : pyramid.steps()) {
        maxRadius = std::max(maxRadius, step.accumulatedRadius());
    }
    const i32 retryRadius = maxRadius;

    std::vector<SingleChunkLifecycleManager*> neighbors;

    {
        std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
        neighbors.reserve(static_cast<size_t>((retryRadius * 2 + 1) * (retryRadius * 2 + 1)));
        for (i32 dz = -retryRadius; dz <= retryRadius; ++dz) {
            for (i32 dx = -retryRadius; dx <= retryRadius; ++dx) {
                if (dx == 0 && dz == 0) {
                    continue;
                }
                auto it = m_lifecycleManagers.find(posToKey(x + dx, z + dz));
                if (it != m_lifecycleManagers.end()) {
                    neighbors.push_back(it->second.get());
                }
            }
        }
    }

    for (SingleChunkLifecycleManager* neighbor : neighbors) {
        if (!neighbor || !neighbor->isWaitingForNeighbors()) {
            continue;
        }

        const ChunkStep& step = pyramid.getStepTo(neighbor->requestedStatus());
        ChunkStepDependencyInfo depInfo = _getDirectDependencyInfo(neighbor->requestedStatus());
        const bool neighborsReady =
            depInfo.hasDependencies ? _areNeighborsReady(neighbor->x(), neighbor->z(), step) : true;
        _advanceChunkState(*neighbor, neighbor->noteNeighborProgress(neighborsReady));
    }
}

// ============================================================================
// 同步生成与邻居依赖
// ============================================================================

void ServerChunkManager::_executeGenerationSync(
    SingleChunkLifecycleManager& lifecycleManager, const ChunkStatus& targetStatus)
{
    ChunkPrimer* primer = lifecycleManager.createGeneratingChunk();
    MC_ASSERT_RELEASE(primer != nullptr);

    // 注册到生成中 Primer 缓存，供邻居区块访问
    {
        std::lock_guard<std::mutex> lock(m_generatingPrimersMutex);
        m_generatingPrimers[posToKey(lifecycleManager.x(), lifecycleManager.z())] = primer;
    }

    // 创建生成缓存，半径为目标步骤的累积依赖半径
    const ChunkPyramid& pyramid = ChunkPyramid::generationPyramid();
    const ChunkStep& targetStep = pyramid.getStepTo(targetStatus);
    const i32 cacheRadius = targetStep.accumulatedRadius();
    GenerationChunkCache cache(lifecycleManager.x(), lifecycleManager.z(), cacheRadius);

    cache.set(lifecycleManager.x(), lifecycleManager.z(), primer);

    _doGenerateChunkToTargetStatus(*primer, targetStatus, cache);
    _doSpawnInitialMobs(*primer);

    // 从生成中缓存移除
    {
        std::lock_guard<std::mutex> lock(m_generatingPrimersMutex);
        m_generatingPrimers.erase(posToKey(lifecycleManager.x(), lifecycleManager.z()));
    }

    auto data = lifecycleManager.completeGeneration();
    MC_ASSERT_RELEASE(data != nullptr);
    ChunkData* stored = _storeChunkInMemorySync(lifecycleManager.x(), lifecycleManager.z(), std::move(data));
    MC_ASSERT_RELEASE(stored != nullptr);
}

void ServerChunkManager::_doGenerateChunkToTargetStatus(
    ChunkPrimer& chunk, const ChunkStatus& targetStatus, GenerationChunkCache& cache)
{
    const ChunkPyramid& pyramid = ChunkPyramid::generationPyramid();
    const auto& allStatuses = ChunkStatus::getAll();
    for (const auto& status : allStatuses) {
        if (status.ordinal() > targetStatus.ordinal()) {
            break;
        }
        if (chunk.hasCompletedStatus(status)) {
            continue;
        }

        const ChunkStep& step = pyramid.getStepTo(status);
        _prepareStepDependencies(chunk, step, cache);

        const i32 regionRadius = step.accumulatedRadius() > 0 ? step.accumulatedRadius() : 0;
        auto context = _doCreateWorldGenRegion(chunk, regionRadius, &cache, &step);

        _executeStepTask(chunk, status, *context.region);

        // 设置 persistedStatus 和 chunkStatus
        chunk.setPersistedStatus(status);
        chunk.setChunkStatus(status);
    }
}

void ServerChunkManager::_prepareStepDependencies(
    ChunkPrimer& chunk, const ChunkStep& step, GenerationChunkCache& cache)
{
    const ChunkDependencies& deps = step.directDependencies();
    for (i32 radius = 0; radius < deps.size(); ++radius) {
        const ChunkStatus* requiredStatus = deps.get(radius);
        if (requiredStatus == nullptr) {
            continue;
        }

        for (i32 dz = -radius; dz <= radius; ++dz) {
            for (i32 dx = -radius; dx <= radius; ++dx) {
                if (dx == 0 && dz == 0) {
                    continue;
                }

                const ChunkCoord nx = chunk.x() + dx;
                const ChunkCoord nz = chunk.z() + dz;
                MC_ASSERT_RELEASE(cache.contains(nx, nz));

                ChunkPrimer* dependency = cache.get(nx, nz);
                if (dependency == nullptr) {
                    if (auto loadedChunk = tryToGetChunkSharedInMem(nx, nz)) {
                        MC_ASSERT_RELEASE(chunkHasCompletedStatus(*loadedChunk, *requiredStatus));
                        continue;
                    }
                    dependency = &cache.getOrCreateOwned(nx, nz);
                    MC_ASSERT_RELEASE(cache.owns(nx, nz));
                }

                if (!dependency->hasCompletedStatus(*requiredStatus)) {
                    MC_ASSERT_RELEASE_MSG(cache.owns(nx, nz), "Generation dependency is not owned by this task");
                    _doGenerateChunkToTargetStatus(*dependency, *requiredStatus, cache);
                }
                MC_ASSERT_RELEASE(dependency->hasCompletedStatus(*requiredStatus));
            }
        }
    }
}

void ServerChunkManager::_executeStepTask(ChunkPrimer& chunk, const ChunkStatus& status, WorldGenRegion& region)
{
    if (status == ChunkStatuses::STRUCTURE_STARTS) {
        m_generator->generateStructureStarts(region, chunk);
    } else if (status == ChunkStatuses::STRUCTURE_REFERENCES) {
        m_generator->generateStructureReferences(region, chunk);
    } else if (status == ChunkStatuses::BIOMES) {
        m_generator->generateBiomes(region, chunk);
    } else if (status == ChunkStatuses::NOISE) {
        m_generator->generateNoise(region, chunk);
    } else if (status == ChunkStatuses::SURFACE) {
        m_generator->buildSurface(region, chunk);
    } else if (status == ChunkStatuses::CARVERS) {
        m_generator->applyCarvers(region, chunk);
    } else if (status == ChunkStatuses::FEATURES) {
        // 在特性放置前 prime POST_FEATURES 高度图
        // CARVERS 完成后方块数据已就绪，但 POST_FEATURES 高度图尚未创建
        chunk.primeHeightmaps(HeightmapFlag::POST_FEATURES);
        m_generator->placeFeatures(region, chunk);
    } else if (status == ChunkStatuses::INITIALIZE_LIGHT) {
        chunk.initializeLightSources();
    } else if (status == ChunkStatuses::LIGHT) {
        // 光照传播由光照引擎异步处理，此处标记状态
    } else if (status == ChunkStatuses::SPAWN) {
        std::vector<SpawnedEntityData> entities;
        m_generator->spawnInitialMobs(region, chunk, entities);
        for (auto& entityData : entities) {
            chunk.addSpawnedEntity(std::move(entityData));
        }
    }
}

void ServerChunkManager::_doSpawnInitialMobs(ChunkPrimer& chunk)
{
    // SPAWN 阶段已移入 _doGenerateChunkToTargetStatus 循环中
    // 此方法保留为兼容性入口，不再执行实际生成逻辑
}

ChunkStepDependencyInfo ServerChunkManager::_getDirectDependencyInfo(const ChunkStatus& targetStatus) const
{
    const ChunkPyramid& pyramid = ChunkPyramid::generationPyramid();
    const ChunkStep& step = pyramid.getStepTo(targetStatus);
    const ChunkDependencies& deps = step.directDependencies();

    ChunkStepDependencyInfo info;
    info.maxDirectRadius = deps.getRadius();
    info.hasDependencies = info.maxDirectRadius > 0;
    return info;
}

bool ServerChunkManager::_areNeighborsReady(ChunkCoord x, ChunkCoord z, const ChunkStep& step) const
{
    const ChunkDependencies& deps = step.directDependencies();

    // 对每个半径级别检查对应邻居是否达到所需状态
    for (i32 radius = 0; radius < deps.size(); ++radius) {
        const ChunkStatus* requiredStatus = deps.get(radius);
        if (requiredStatus == nullptr) {
            continue;
        }

        // 遍历此半径级别的所有邻居位置
        for (i32 dz = -radius; dz <= radius; ++dz) {
            for (i32 dx = -radius; dx <= radius; ++dx) {
                if (dx == 0 && dz == 0) {
                    continue; // 跳过中心区块自身
                }

                const SingleChunkLifecycleManager* neighbor = _findLifecycleManager(x + dx, z + dz);
                if (!neighbor || !neighbor->hasCompletedStatus(*requiredStatus)) {
                    // 额外检查：生成中的 Primer 可能已完成所需状态
                    // 但 lifecycle manager 尚未更新
                    {
                        std::lock_guard<std::mutex> lock(m_generatingPrimersMutex);
                        auto it = m_generatingPrimers.find(posToKey(x + dx, z + dz));
                        if (it != m_generatingPrimers.end() && it->second != nullptr) {
                            if (it->second->hasCompletedStatus(*requiredStatus)) {
                                continue;
                            }
                        }
                    }
                    return false;
                }
            }
        }
    }

    return true;
}

i32 ServerChunkManager::_computeSchedulePriority(
    ChunkCoord x, ChunkCoord z, const ChunkStatus& targetStatus, i32 ticketLevel) const
{
    const i32 normalizedLevel = std::clamp(ticketLevel, 0, world::chunk::ChunkDistanceGraph::MAX_LEVEL);
    const i32 statusPenalty = std::max(0, ChunkStatuses::FULL.ordinal() - targetStatus.ordinal());
    const i32 spatialPenalty = static_cast<i32>((std::abs(x) + std::abs(z)) & 0xFF);
    return normalizedLevel * 1024 + statusPenalty * 32 + spatialPenalty;
}

// ============================================================================
// 邻居窗口
// ============================================================================

void ServerChunkManager::_collectNeighborChunks(ChunkCoord x,
    ChunkCoord z,
    i32 radius,
    IChunk* centerChunk,
    std::vector<IChunk*>& neighbors,
    std::vector<std::shared_ptr<ChunkData>>& loadedNeighbors,
    std::vector<std::unique_ptr<ChunkPrimer>>& missingNeighbors,
    GenerationChunkCache* cache)
{
    const i32 diameter = radius * 2 + 1;
    MC_ASSERT_RELEASE(centerChunk != nullptr);
    MC_ASSERT_RELEASE(static_cast<i32>(neighbors.size()) == diameter * diameter);
    MC_ASSERT_RELEASE(static_cast<i32>(loadedNeighbors.size()) == diameter * diameter);
    MC_ASSERT_RELEASE(static_cast<i32>(missingNeighbors.size()) == diameter * diameter);

    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            const size_t index = static_cast<size_t>((dz + radius) * diameter + (dx + radius));

            if (dx == 0 && dz == 0) {
                neighbors[index] = centerChunk;
                continue;
            }

            const ChunkCoord nx = x + dx;
            const ChunkCoord nz = z + dz;

            // 优先从生成缓存获取
            if (cache != nullptr) {
                ChunkPrimer* cachedPrimer = cache->get(nx, nz);
                if (cachedPrimer != nullptr) {
                    neighbors[index] = cachedPrimer;
                    continue;
                }
            }

            // 其次从生成中 Primer 缓存获取
            ChunkPrimer* generatingPrimer = getGeneratingPrimer(nx, nz);
            if (generatingPrimer != nullptr) {
                neighbors[index] = generatingPrimer;
                continue;
            }

            // 再从内存缓存获取
            if (auto loadedChunk = tryToGetChunkSharedInMem(nx, nz)) {
                loadedNeighbors[index] = std::move(loadedChunk);
                neighbors[index] = loadedNeighbors[index].get();
                continue;
            }

            neighbors[index] = nullptr;
        }
    }
}

ServerChunkManager::NeighborRegionContext ServerChunkManager::_doCreateWorldGenRegion(
    IChunk& centerChunk, i32 radius, GenerationChunkCache* cache, const ChunkStep* step)
{
    const size_t chunkCount = static_cast<size_t>((radius * 2 + 1) * (radius * 2 + 1));

    NeighborRegionContext context{std::vector<IChunk*>(chunkCount, nullptr),
        std::vector<std::shared_ptr<ChunkData>>(chunkCount),
        std::vector<std::unique_ptr<ChunkPrimer>>(chunkCount),
        nullptr};
    _collectNeighborChunks(centerChunk.x(),
        centerChunk.z(),
        radius,
        &centerChunk,
        context.neighbors,
        context.loadedNeighbors,
        context.missingNeighbors,
        cache);
    const DimensionId dimId = m_world != nullptr ? m_world->dimension() : 0;
    if (step != nullptr) {
        assertRegionSatisfiesDirectDependencies(context.neighbors, centerChunk.x(), centerChunk.z(), radius, *step);
        context.region = std::make_unique<WorldGenRegion>(
            centerChunk.x(), centerChunk.z(), *step, std::move(context.neighbors), dimId);
    } else {
        context.region = std::make_unique<WorldGenRegion>(
            centerChunk.x(), centerChunk.z(), radius, std::move(context.neighbors), dimId);
    }

    // MC 1.21.11: WorldGenRegion 需要从世界获取种子、时间、难度等信息
    // 这些字段在生成过程中被 WorldGenSpawner、Carver 等使用
    if (m_world != nullptr) {
        context.region->setSeed(m_world->seed());
        context.region->setCurrentTick(m_world->currentTick());
        context.region->setDayTime(m_world->dayTime());
        context.region->setHardcore(m_world->isHardcore());
        context.region->setDifficulty(m_world->difficulty());
    }

    return context;
}

// ============================================================================
// 存储与发布
// ============================================================================

ChunkData* ServerChunkManager::_storeChunkInMemorySync(ChunkCoord x, ChunkCoord z, std::unique_ptr<ChunkData> data)
{
    MC_ASSERT_RELEASE(data != nullptr);
    std::shared_ptr<ChunkData> sharedChunk(std::move(data));
    MC_ASSERT_RELEASE(sharedChunk != nullptr);

    ChunkData* stored = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_chunksMutex);
        auto& slot = m_chunks[posToKey(x, z)];
        if (slot && slot.get() == sharedChunk.get()) {
            stored = slot.get();
        } else {
            slot = std::move(sharedChunk);
            stored = slot.get();
        }
    }

    if (SingleChunkLifecycleManager* lifecycleManager = _findLifecycleManager(x, z)) {
        lifecycleManager->setStatus(ChunkStatuses::FULL);
    }

    if (stored && m_world) {
        m_world->onChunkLoaded(x, z);
    }

    if (m_chunkLoadedCallback && stored) {
        m_chunkLoadedCallback(x, z);
    }

    return stored;
}

ChunkData* ServerChunkManager::_finalizeGeneratedChunkSync(ChunkCoord x, ChunkCoord z, ChunkPrimer& primer)
{
    std::vector<SpawnedEntityData> spawnedEntities;
    if (primer.spawnedEntityCount() > 0) {
        spawnedEntities = std::move(primer.spawnedEntities());
    }

    auto data = primer.toChunkData();
    if (!data) {
        return nullptr;
    }

    if (SingleChunkLifecycleManager* lifecycleManager = _findLifecycleManager(x, z)) {
        lifecycleManager->markGenerationReady();
    }

    // 确保从生成中缓存移除（可能在异步路径中已被清理）
    {
        std::lock_guard<std::mutex> lock(m_generatingPrimersMutex);
        m_generatingPrimers.erase(posToKey(x, z));
    }

    ChunkData* stored = _storeChunkInMemorySync(x, z, std::move(data));
    if (stored && !spawnedEntities.empty()) {
        if (m_world) {
            m_world->spawnEntitiesFromChunkGeneration(spawnedEntities);
        } else if (m_entitySpawnCallback) {
            m_entitySpawnCallback(spawnedEntities);
        }
    }

    // LevelChunk.postProcessGeneration — 处理含水层流体更新和方块形状更新
    if (stored && m_world) {
        _postProcessChunk(*stored);
    }

    return stored;
}

void ServerChunkManager::_postProcessChunk(ChunkData& chunk)
{
    // LevelChunk.postProcessGeneration(ServerLevel)
    // 遍历所有后处理位置，调度流体 tick 和方块形状更新
    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();
    const i32 startX = chunkX * world::CHUNK_WIDTH;
    const i32 startZ = chunkZ * world::CHUNK_WIDTH;

    for (i32 sectionIdx = 0; sectionIdx < world::CHUNK_SECTIONS; ++sectionIdx) {
        const auto& packedPositions = chunk.postProcessingSections()[sectionIdx];
        if (packedPositions.empty()) {
            continue;
        }

        for (u16 packed : packedPositions) {
            // 解包段内本地坐标
            const i32 localX = packed & world::CHUNK_MASK;
            const i32 localY = (packed >> world::SECTION_SHIFT) & world::CHUNK_MASK;
            const i32 localZ = (packed >> (world::SECTION_SHIFT * 2)) & world::CHUNK_MASK;

            // 重建世界坐标
            const i32 worldX = localX + startX;
            const i32 worldY = localY + (sectionIdx * world::CHUNK_SECTION_HEIGHT + world::MIN_BUILD_HEIGHT);
            const i32 worldZ = localZ + startZ;

            const BlockPos pos(worldX, worldY, worldZ);
            const BlockState* blockState = chunk.getBlockState(localX, worldY, localZ);
            if (blockState == nullptr) {
                continue;
            }

            // 流体方块 → 调度流体 tick
            if (blockState->isLiquid()) {
                const auto* fluidState = blockState->getFluidState();
                if (fluidState != nullptr && !fluidState->isEmpty()) {
                    // scheduleFluidTick 需要 Fluid&，但 Fluid 是注册表单例，const_cast 安全
                    auto& fluid = const_cast<fluid::Fluid&>(fluidState->getFluid());
                    m_world->tickManager().scheduleFluidTick(pos, fluid, fluidState->getFluid().getTickDelay(*m_world));
                }
            }

            // 非液体方块 → updateFromNeighbourShapes
            // TODO 当 Block.updateFromNeighbourShapes 实现后启用此分支
            // if (!blockState->getBlock().isLiquidBlock()) {
            //     BlockState updated = Block::updateFromNeighbourShapes(...);
            //     if (updated != *blockState) {
            //         m_world->setBlock(pos, updated, 276);
            //     }
            // }
        }

        chunk.clearPostProcessingForSection(sectionIdx);
    }
}

void ServerChunkManager::_saveChunkSectionsSync(const ChunkData& chunk)
{
    if (!m_world || !m_world->isStorageOpen()) {
        return;
    }

    auto saveResult = m_world->storage().saveChunk(chunk, m_world->dimension());
    if (saveResult.failed()) {
        spdlog::error("Save chunk failed ({}, {}): {}", chunk.x(), chunk.z(), saveResult.error().message());
    }
}

std::unique_ptr<ChunkData> ServerChunkManager::_tryToLoadChunkFromStorageSync(ChunkCoord x, ChunkCoord z)
{
    MC_TRACE_EVENT("server.chunk", "ServerChunkManager::tryToLoadChunkFromStorageSync");

    if (!m_world || !m_world->isStorageOpen()) {
        return nullptr;
    }

    auto loadResult = m_world->storage().loadChunk(x, z, m_world->dimension());
    if (loadResult.failed()) {
        spdlog::error("Load chunk failed ({}, {}): {}", x, z, loadResult.error().message());
        return nullptr;
    }
    if (!loadResult.value().has_value()) {
        return nullptr;
    }
    return std::make_unique<ChunkData>(std::move(loadResult.value().value()));
}

// ============================================================================
// 卸载
// ============================================================================

void ServerChunkManager::unloadChunkSync(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);

    if (m_world && m_world->isStorageOpen()) {
        std::shared_ptr<ChunkData> chunkToSave;
        {
            std::lock_guard<std::mutex> lock(m_chunksMutex);
            auto it = m_chunks.find(key);
            if (it != m_chunks.end() && it->second && it->second->isDirty()) {
                chunkToSave = it->second;
            }
        }

        if (chunkToSave) {
            _saveChunkSectionsSync(*chunkToSave);
            chunkToSave->setDirty(false);
        }
    }

    if (m_world) {
        m_world->onChunkUnloading(x, z);
    }

    if (m_chunkSendManager) {
        m_chunkSendManager->onChunkPreUnload(x, z);
    }
    if (m_chunkUnloadedCallback) {
        m_chunkUnloadedCallback(x, z);
    }

    std::unique_ptr<SingleChunkLifecycleManager> lifecycleManager;
    {
        std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
        auto it = m_lifecycleManagers.find(key);
        if (it != m_lifecycleManagers.end()) {
            lifecycleManager = std::move(it->second);
            m_lifecycleManagers.erase(it);
        }
    }

    if (lifecycleManager) {
        lifecycleManager->cancelActiveWork();
        _failWaiters(lifecycleManager->takeAllWaiters());
    }

    {
        std::lock_guard<std::mutex> lock(m_chunksMutex);
        m_chunks.erase(key);
    }
}

void ServerChunkManager::_checkChunkUnloading()
{
    std::vector<u64> toUnload;

    {
        std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
        for (const auto& [key, lifecycleManager] : m_lifecycleManagers) {
            if (!lifecycleManager) {
                continue;
            }

            if (!lifecycleManager->shouldLoad() && !m_ticketManager.hasTrackingPlayers(key) &&
                !lifecycleManager->hasGeneratingChunk() &&
                !hasGeneratingPrimer(ChunkId::fromId(key).x, ChunkId::fromId(key).z)) {
                toUnload.push_back(key);
            }
        }
    }

    for (u64 key : toUnload) {
        auto chunkId = ChunkId::fromId(key);
        unloadChunkSync(chunkId.x, chunkId.z);
    }
}

// ============================================================================
// 票据与 tick
// ============================================================================

void ServerChunkManager::updatePlayerPosition(PlayerId player, f64 x, f64 z)
{
    MC_TRACE_EVENT("server.chunk", "ServerChunkManager::updatePlayerPosition", "player", player, "x", x, "z", z);

    const ChunkCoord chunkX = static_cast<ChunkCoord>(std::floor(x / world::CHUNK_WIDTH));
    const ChunkCoord chunkZ = static_cast<ChunkCoord>(std::floor(z / world::CHUNK_WIDTH));
    m_ticketManager.updatePlayerPosition(player, chunkX, chunkZ);
}

void ServerChunkManager::removePlayer(PlayerId player)
{
    m_ticketManager.removePlayer(player);
}

void ServerChunkManager::forceChunk(ChunkCoord x, ChunkCoord z, bool force)
{
    m_ticketManager.forceChunk(x, z, force);
}

void ServerChunkManager::setViewDistance(i32 distance)
{
    m_ticketManager.setViewDistance(distance);
}

void ServerChunkManager::tick()
{
    ++m_currentTick;
    processTicketUpdatesSync();

    // 增加有玩家附近的区块的居住时间（每 tick +1）
    _incrementInhabitedTime();

    if (m_currentTick - m_lastUnloadCheckTick >= UNLOAD_CHECK_INTERVAL_TICKS) {
        _checkChunkUnloading();
        m_lastUnloadCheckTick = m_currentTick;
    }
}

void ServerChunkManager::_incrementInhabitedTime()
{
    // 遍历所有已加载区块，对有玩家追踪的区块增加居住时间
    forEachLoadedChunk([this](ChunkData& chunk) {
        const u64 key = ChunkId(chunk.x(), chunk.z(), 0).toId();
        if (m_ticketManager.hasTrackingPlayers(key)) {
            chunk.incrementInhabitedTime(1);
        }
        return true; // 继续遍历
    });
}

// ============================================================================
// 统计
// ============================================================================

size_t ServerChunkManager::loadedChunkCount() const
{
    std::lock_guard<std::mutex> lock(m_chunksMutex);
    return m_chunks.size();
}

void ServerChunkManager::forEachLoadedChunk(const std::function<bool(ChunkData&)>& callback)
{
    std::vector<std::shared_ptr<ChunkData>> chunks;
    {
        std::lock_guard<std::mutex> lock(m_chunksMutex);
        chunks.reserve(m_chunks.size());
        for (const auto& [key, chunk] : m_chunks) {
            MC_UNUSED(key);
            if (chunk) {
                chunks.push_back(chunk);
            }
        }
    }

    for (auto& chunk : chunks) {
        if (chunk && !callback(*chunk)) {
            break;
        }
    }
}

void ServerChunkManager::forEachLoadedChunk(const std::function<bool(const ChunkData&)>& callback) const
{
    std::vector<std::shared_ptr<const ChunkData>> chunks;
    {
        std::lock_guard<std::mutex> lock(m_chunksMutex);
        chunks.reserve(m_chunks.size());
        for (const auto& [key, chunk] : m_chunks) {
            MC_UNUSED(key);
            if (chunk) {
                chunks.push_back(chunk);
            }
        }
    }

    for (const auto& chunk : chunks) {
        if (chunk && !callback(*chunk)) {
            break;
        }
    }
}

size_t ServerChunkManager::lifecycleManagerCount() const
{
    std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
    return m_lifecycleManagers.size();
}

size_t ServerChunkManager::pendingTaskCount() const
{
    return m_workerPool ? m_workerPool->pendingTaskCount() : 0;
}

} // namespace mc::server
