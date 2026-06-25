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
#include "ChunkTaskScheduler.hpp"
#include "ServerWorld.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkPyramid.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "server/sync/ChunkSendManager.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::server {

using mc::world::chunk::ChunkPrimer;
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
    // 调度器延迟到 setWorkerPool 时创建（需要 worker 池）；独立模式（无 world/无 pool）由
    // requestChunkSync 等入口按需创建。
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
    // 第一步：通知所有活跃生成任务取消（设置 cancel token）。正在执行的 ChunkProgressionTask
    // 会在下一个取消检查点（execute 开头/executeStatusTask 执行后）检测到取消并返回 false，
    // 其回调转而调用 onChunkGenFailed 完成清理。holder 仍在 m_lifecycleManagers 中（未移除），
    // 故回调中的 findHolder 仍能找到 holder，安全完成失败路径。
    {
        std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
        for (auto& [key, lifecycleManager] : m_lifecycleManagers) {
            MC_UNUSED(key);
            if (lifecycleManager) {
                lifecycleManager->cancelActiveWork();
            }
        }
    }

    // 第二步：排空 worker 池，确保所有正在执行/排队的生成任务完成后再销毁 holder 与 chunk 数据。
    // 这一步至关重要：ChunkProgressionTask::execute 持有 holder 原始指针（findHolder）并引用 m_manager，
    // 若在任务执行期间销毁 holder/chunk 会导致 use-after-free。worker 池由外部持有（测试或 ServerWorld），
    // 此处仅等待其空闲，不停止它。被取消的任务会快速返回，被取消令牌阻止的任务在回调中走失败路径。
    // waitForCompletion 可能因 onChunkGenComplete 的自重调度短暂波动，但取消已使所有 holder 的
    // cancelToken 失效，新调度产生的任务也会立刻检测到取消（holder 已 cancelActiveWork），
    // 最终队列收敛为空。
    if (m_workerPool != nullptr && m_workerPool->isRunning()) {
        m_workerPool->waitForCompletion();
    }

    // 第三步：所有生成任务已结束，安全移除 holder。移到本地 vector 在锁外完成等待者失败通知。
    std::vector<std::unique_ptr<SingleChunkLifecycleManager>> lifecycleManagers;
    {
        std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
        lifecycleManagers.reserve(m_lifecycleManagers.size());
        for (auto& [key, lifecycleManager] : m_lifecycleManagers) {
            MC_UNUSED(key);
            if (lifecycleManager) {
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

    // 同步请求：提交一个带 promise 的请求，阻塞等待生成完成。
    // ChunkTaskScheduler 在 worker 池上推进生成，完成后 _completeReadyWaiters 完成 promise。
    // 无 worker 池时 ChunkTaskScheduler 内部在线执行任务（submitTask 降级），仍会走完成回调。
    auto promise = std::make_shared<std::promise<ChunkData*>>();
    auto future = promise->get_future();
    _submitChunkRequest(x, z, targetStatus, {}, promise);
    return future.get();
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

    if (decision.shouldScheduleGeneration) {
        // 持有调度区域锁后委托给 ChunkTaskScheduler 推进生成
        _scheduleGeneration(lifecycleManager, *decision.targetStatus);
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
        // 存档命中：存入内存缓存。_storeChunkInMemorySync 内部会调用
        // markLoadedFromStorageReady(FULL) + _completeReadyWaiters 唤醒等待者。
        (void)_storeChunkInMemorySync(x, z, std::move(loadedChunk));
        return;
    }

    // 存档缺失：走生成链路（由 ChunkTaskScheduler 调度）
    _advanceChunkState(lifecycleManager, decision);
}

void ServerChunkManager::_scheduleGeneration(
    SingleChunkLifecycleManager& lifecycleManager, const ChunkStatus& targetStatus)
{
    if (m_taskScheduler == nullptr) {
        // 无调度器（独立/测试模式未调用 setWorkerPool）：创建一个共享同一 worker 池的调度器。
        // worker 池可能为 nullptr，ChunkTaskScheduler 会在线执行任务（submitTask 降级）。
        m_taskScheduler = std::make_unique<ChunkTaskScheduler>(*this, m_world, m_workerPool, m_workerPool);
    }

    const ChunkCoord x = lifecycleManager.x();
    const ChunkCoord z = lifecycleManager.z();
    // 持有 2 * maxAccessRadius 的锁，覆盖递归 schedule/checkNeighbour 的邻居范围。
    // 同步执行模式（无 worker 池）下，checkNeighbour 递归 schedule 邻居时，
    // 邻居的 schedule 断言要求 [邻居 ± maxAccessRadius] 被持有，邻居可达 maxAccessRadius 远，
    // 故外层锁需 2 * maxAccessRadius 才能覆盖（与 onChunkGenComplete 的锁范围一致）。
    const i32 lockRadius = 2 * ChunkTaskScheduler::getMaxAccessRadius();
    auto lock = m_taskScheduler->schedulingLockArea().lock(x, z, lockRadius);
    m_taskScheduler->schedule(x, z, targetStatus, lifecycleManager);
    // lock 释放时释放区域锁
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

// ============================================================================
// 生成步骤分派（由 ChunkProgressionTask 调用）
// ============================================================================

void ServerChunkManager::_executeStepTask(ChunkPrimer& chunk, const ChunkStatus& status, WorldGenRegion& region)
{
    MC_TRACE_EVENT("server.chunk",
        "ServerChunkManager::executeStepTask",
        "chunkX",
        chunk.x(),
        "chunkZ",
        chunk.z(),
        "status",
        status.name());

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

i32 ServerChunkManager::_computeSchedulePriority(
    ChunkCoord x, ChunkCoord z, const ChunkStatus& targetStatus, i32 ticketLevel) const
{
    const i32 normalizedLevel = std::clamp(ticketLevel, 0, world::chunk::ChunkDistanceGraph::MAX_LEVEL);
    const i32 statusPenalty = std::max(0, ChunkStatuses::FULL.ordinal() - targetStatus.ordinal());
    const i32 spatialPenalty = static_cast<i32>((std::abs(x) + std::abs(z)) & 0xFF);
    return normalizedLevel * 1024 + statusPenalty * 32 + spatialPenalty;
}

// ============================================================================
// 存储与发布
// ============================================================================

ChunkData* ServerChunkManager::_storeChunkInMemorySync(ChunkCoord x, ChunkCoord z, std::unique_ptr<ChunkData> data)
{
    MC_ASSERT_RELEASE(data != nullptr);
    return _storeChunkInMemorySync(x, z, std::shared_ptr<ChunkData>(std::move(data)));
}

ChunkData* ServerChunkManager::_storeChunkInMemorySync(ChunkCoord x, ChunkCoord z, std::shared_ptr<ChunkData> data)
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
        // 区块已发布到内存缓存：标记 sourceState=Ready + currentGenStatus=FULL，并唤醒等待者。
        // markLoadedFromStorageReady 推进 currentGenStatus 到 FULL（若尚未）并设置 m_sourceState=Ready，
        // 使 _buildDecisionLocked 返回 shouldWakeReadyWaiters=true。
        lifecycleManager->markLoadedFromStorageReady(ChunkStatuses::FULL);
        _completeReadyWaiters(*lifecycleManager);
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

    // toChunkData 非破坏性：返回 shared_ptr 共享同一份 ChunkData，primer 仍持有 m_data。
    // 对齐 Moonrise：FULL 完成后 currentChunk（primer）仍存活供邻居引用，直到 holder 卸载。
    std::shared_ptr<ChunkData> data = primer.toChunkData();
    if (!data) {
        return nullptr;
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

void ServerChunkManager::_publishGeneratedChunk(SingleChunkLifecycleManager& holder, const ChunkStatus& completedStatus)
{
    // FULL 已由 _finalizeGeneratedChunkSync 处理（存入 m_chunks + markLoadedFromStorageReady +
    // _completeReadyWaiters），此处只处理非 FULL 目标状态完成。
    if (completedStatus == ChunkStatuses::FULL) {
        return;
    }

    // 仅当达到请求目标状态时才唤醒等待者（onChunkGenComplete 每步都调用，但中间步骤不应唤醒）。
    if (!holder.hasCompletedStatus(holder.requestedGenStatus())) {
        return;
    }

    // 非 FULL 目标：不设置 sourceState=Ready（Ready 表示 FULL 完成可发布到 m_chunks，
    // 中间状态不应阻止后续更高状态请求调度）。不存入 m_chunks（m_chunks 仅保留 FULL 区块，
    // 保证 tryToGetChunkInMem 快速路径不返回中间状态区块）。
    // 直接用 primer 的 ChunkData 完成等待者（promise/callback），请求者获得 primer 当前状态的 ChunkData。
    // takeAllWaiters 取出所有等待者（无条件），fulfillWaiters 完成 promise。
    ChunkPrimer* primer = holder.getCurrentChunk();
    ChunkData* chunkData = (primer != nullptr) ? primer->getChunkData() : nullptr;

    fulfillWaiters(holder.takeAllWaiters(), chunkData != nullptr, chunkData);
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
            if (!blockState->isLiquid()) {
                BlockState updated = Block::updateFromNeighbourShapes(*blockState, *m_world, pos);
                if (updated != *blockState) {
                    m_world->setBlockState(pos, &updated, 276);
                }
            }
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
                lifecycleManager->isSafeToUnload()) {
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
