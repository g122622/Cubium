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
#include "ChunkLightingProvider.hpp"
#include "ChunkTaskScheduler.hpp"
#include "ServerWorld.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/base/SectionPos.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkPyramid.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "server/sync/ChunkSendManager.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <future>
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
    // 标记关闭：异步存档加载完成回调（ServerCompute 线程）检测到此标志后不再入队
    // m_pendingLoadCompletes，避免析构后回调访问悬空 this。置位必须在 cancelActiveWork 之前，
    // 使在途回调快速丢弃结果。
    m_shuttingDown.store(true, std::memory_order::release);

    // 第一步：标记调度器关闭，通知所有活跃生成任务取消（设置 cancel token）。
    // setShuttingDown 使 cancelGeneration 不重新调度被解除阻塞的邻居（避免无限循环）：
    // 关闭时所有 holder 的 abortSignal 已失效，重调度的任务无法被取消，waitForCompletion 会无限等待。
    // 正在执行的 ChunkProgressionTask 会在取消检查点检测到取消并返回 false，onCancel→cancelGeneration
    // 清理依赖图（不重调度）。holder 仍在 m_lifecycleManagers 中（未移除），回调中的 findHolder 仍能找到。
    if (m_taskScheduler != nullptr) {
        m_taskScheduler->setShuttingDown();
    }

    // 收集所有 holder 的 shared_ptr（保持存活），在 m_lifecycleManagersMutex 锁外执行 cancelGeneration。
    // cancelGeneration 持调度锁并调用 findHolder（获取 m_lifecycleManagersMutex），若在遍历 m_lifecycleManagers
    // 时持锁调用会死锁。shared_ptr 保证 worker 线程的 ChunkProgressionTask（持有 holder 引用）期间 holder 不被销毁。
    std::vector<std::shared_ptr<SingleChunkLifecycleManager>> lifecycleManagers;
    {
        std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
        lifecycleManagers.reserve(m_lifecycleManagers.size());
        for (auto& [key, lifecycleManager] : m_lifecycleManagers) {
            MC_UNUSED(key);
            if (lifecycleManager) {
                lifecycleManagers.push_back(lifecycleManager);
            }
        }
    }

    // 对每个 holder 执行 cancelGeneration + cancelActiveWork（顺序与 unloadChunkSync 一致）：
    //   - cancelGeneration 持调度锁清理依赖图、补偿释放邻居引用计数、通知等待者失败。
    //     isShuttingDown=true 时不重新调度被解除阻塞的邻居（避免无限循环）。
    //   - cancelActiveWork 设置 abortSignal、清空 m_generationTask/m_scheduledStatus、重置请求优先级。
    // 旧实现仅调用 cancelActiveWork：m_generationTask 被清空后 onCancel 的 cancelGeneration(holder, task)
    // 因 generationTask()!=task 为 no-op，依赖图与邻居引用计数永不清理 → holder 泄漏 → waitForCompletion 卡死。
    for (auto& lifecycleManager : lifecycleManagers) {
        if (lifecycleManager == nullptr) {
            continue;
        }
        if (m_taskScheduler != nullptr) {
            m_taskScheduler->cancelGeneration(*lifecycleManager);
        }
        lifecycleManager->cancelActiveWork();
    }

    // 第二步：排空 worker 池，确保所有正在执行/排队的生成任务完成后再销毁 holder 与 chunk 数据。
    // 这一步至关重要：ChunkProgressionTask::execute 及其回调持有 holder 的 shared_ptr（m_holder），
    // 并引用 m_manager；若在任务执行期间销毁 holder/chunk 会导致 use-after-free。worker 池由外部持有
    // （测试或 ServerWorld），此处仅等待其空闲，不停止它。被取消的任务会快速返回，被取消令牌阻止的
    // 任务在回调中走失败路径。waitForCompletion 可能因 onChunkGenComplete 的自重调度短暂波动，
    // 但取消已使所有 holder 的 abortSignal 失效，新调度产生的任务也会立刻检测到取消
    // （holder 已 cancelActiveWork），最终队列收敛为空。
    if (m_workerPool != nullptr && m_workerPool->isRunning()) {
        m_workerPool->waitForCompletion();
    }

    // 第三步：所有生成任务已结束，安全移除 holder。移到本地 vector 在锁外完成等待者失败通知。
    std::vector<std::shared_ptr<SingleChunkLifecycleManager>> remainingManagers;
    {
        std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
        remainingManagers.reserve(m_lifecycleManagers.size());
        for (auto& [key, lifecycleManager] : m_lifecycleManagers) {
            MC_UNUSED(key);
            if (lifecycleManager) {
                remainingManagers.push_back(std::move(lifecycleManager));
            }
        }
        m_lifecycleManagers.clear();
    }

    for (auto& lifecycleManager : remainingManagers) {
        if (lifecycleManager) {
            _failWaiters(lifecycleManager->takeAllWaiters());
        }
    }

    std::lock_guard<std::mutex> lock(m_chunksMutex);
    m_chunks.clear();

    // 清理后处理队列与去重标记，避免 shutdown 后残留状态影响下次 initialize
    {
        std::lock_guard<std::mutex> ppLock(m_pendingPostProcessMutex);
        m_pendingPostProcess.clear();
        m_postProcessedChunks.clear();
    }

    // 清理异步加载完成队列与追踪表（此时线程池已 shutdown，无新入队）
    {
        std::lock_guard<std::mutex> lcLock(m_pendingLoadCompletesMutex);
        m_pendingLoadCompletes.clear();
    }
    {
        std::lock_guard<std::mutex> ltLock(m_pendingLoadTasksMutex);
        m_pendingLoadTasks.clear();
    }
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

    // 异步存档加载路径（_resolveChunkSourceSync → loadChunkAsyncCallback）把完成回调入队
    // m_pendingLoadCompletes，需主线程 _drainPendingLoadCompletes 出队处理（_onChunkLoadComplete
    // 存档命中→_completeReadyWaiters fulfill promise）。requestChunkSync 在主线程阻塞等待期间
    // 主动 pump 该队列，否则存档命中时 promise 永不 fulfill（主线程卡在 future.get 不进 tick()）。
    // 仅 pump m_pendingLoadCompletes：_onChunkLoadComplete 内联调 onChunkLoaded/callback，不依赖
    // _drainPendingPostProcess；生成路径 onChunkGenComplete 在 worker 线程直接 fulfill promise。
    // 此方法仅可在主线程调用（worker 线程请用 requestChunkAsync）。
    while (future.wait_for(std::chrono::milliseconds(1)) != std::future_status::ready) {
        _drainPendingLoadCompletes();
    }
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
        "server.chunk", "ServerChunkManager::_submitChunkRequest", "x", x, "z", z, "targetStatus", targetStatus.name());

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

    // 持有 SCLM 共享所有权，防止异步加载期间被 unloadChunkSync 销毁。
    // _onChunkLoadComplete 通过实例指针一致性校验防止 SCLM 被 unload 重建后的误用。
    auto lifecycleHolder = _findLifecycleManagerShared(x, z);
    // 正常路径下 lifecycleHolder 非空（_getOrCreateLifecycleManager 刚创建并入表）。
    // 若极端情况下为空（并发 unload），直接 noteStorageResolved(false) 走生成链路。
    if (lifecycleHolder == nullptr) {
        auto decision = lifecycleManager.noteStorageResolved(false);
        _advanceChunkState(lifecycleManager, decision);
        return;
    }

    // 无存档（测试/独立模式未配置存储，或存档未打开）：视同存档缺失，直接走生成链路。
    // 与旧 _tryToLoadChunkFromStorageSync 的 !isStorageOpen() 守卫等价，避免 storage() 解引用空指针。
    if (!m_world || !m_world->isStorageOpen()) {
        auto decision = lifecycleManager.noteStorageResolved(false);
        _advanceChunkState(lifecycleManager, decision);
        return;
    }

    // 追踪进行中的异步加载，供 unloadChunkSync 取消与 _onChunkLoadComplete 移除。
    {
        std::lock_guard<std::mutex> lock(m_pendingLoadTasksMutex);
        m_pendingLoadTasks[posToKey(x, z)] = lifecycleHolder;
    }

    const auto dimension = m_world->dimension();

    // 异步发起：ServerIO 读盘（section+blockEntity 两路并行）→ ServerCompute 反序列化组装。
    // 完成回调在 ServerCompute 线程执行，仅把结果入队 m_pendingLoadCompletes，不触及主线程独占状态。
    // abortSignal 来自 SCLM，unloadChunkSync→cancelActiveWork 会置位，使 ServerIO/ServerCompute 任务协作取消。
    auto abortSignal = lifecycleManager.abortSignal();
    auto* self = this;
    m_world->storage().loadChunkAsyncCallback(
        x,
        z,
        dimension,
        [self, x, z, dimension, lifecycleHolder](ChunkCoord /*cbX*/,
            ChunkCoord /*cbZ*/,
            mc::DimensionId /*cbDim*/,
            mc::Result<std::optional<mc::ChunkData>> result) {
            // shutdown 后不再入队：ServerChunkManager 可能已析构或正在析构。
            if (self->m_shuttingDown.load(std::memory_order::acquire)) {
                return;
            }
            PendingLoadComplete item;
            item.x = x;
            item.z = z;
            item.dimension = dimension;
            item.result = std::move(result);
            item.lifecycleHolder = lifecycleHolder;
            {
                std::lock_guard<std::mutex> lock(self->m_pendingLoadCompletesMutex);
                self->m_pendingLoadCompletes.push_back(std::move(item));
            }
        },
        abortSignal);
}

void ServerChunkManager::_onChunkLoadComplete(ChunkCoord x,
    ChunkCoord z,
    mc::DimensionId dimension,
    mc::Result<std::optional<mc::ChunkData>> result,
    std::shared_ptr<SingleChunkLifecycleManager> lifecycleHolder)
{
    const u64 key = posToKey(x, z);

    // 从追踪表移除（无论命中/缺失/错误/取消）。
    {
        std::lock_guard<std::mutex> lock(m_pendingLoadTasksMutex);
        m_pendingLoadTasks.erase(key);
    }

    // 实例一致性校验：异步期间 SCLM 可能被 unload 重建。
    // 仅当当前 m_lifecycleManagers 中的实例仍是 lifecycleHolder 指向的同一实例时才推进。
    auto currentHolder = _findLifecycleManagerShared(x, z);
    if (currentHolder == nullptr || currentHolder.get() != lifecycleHolder.get()) {
        // SCLM 已被卸载或重建，丢弃本次加载结果（避免在错误实例上 noteStorageResolved）。
        return;
    }

    SingleChunkLifecycleManager& lifecycleManager = *lifecycleHolder;

    // 加载失败：视同存档缺失，走生成链路（与同步路径 loadChunk 失败行为一致）。
    if (result.failed()) {
        spdlog::warn("Async load chunk failed ({}, {}): {}", x, z, result.error().message());
        auto decision = lifecycleManager.noteStorageResolved(false);
        _advanceChunkState(lifecycleManager, decision);
        return;
    }

    std::optional<ChunkData> chunkOpt = std::move(result).value();
    if (chunkOpt.has_value()) {
        // 存档命中：存入内存缓存。_storeChunkInMemorySync 内部 markLoadedFromStorageReady(FULL) +
        // _completeReadyWaiters 唤醒等待者。noteStorageResolved(true) 推进 SourceState→LoadedFromStorage。
        auto decision = lifecycleManager.noteStorageResolved(true);
        std::unique_ptr<ChunkData> loadedChunk = std::make_unique<ChunkData>(std::move(chunkOpt.value()));
        ChunkData* stored = _storeChunkInMemorySync(x, z, std::move(loadedChunk));
        if (stored && m_world) {
            // onChunkLoaded/m_chunkLoadedCallback 触及主线程独占状态，本方法在主线程 tick() 调用，
            // 可直接内联。去重通过 m_postProcessedChunks（与 _resolveChunkSourceSync 同步路径、
            // _drainPendingPostProcess 共享同一集合，避免 worker 存档加载路径重复执行）。
            bool alreadyProcessed;
            {
                std::lock_guard<std::mutex> lock(m_pendingPostProcessMutex);
                alreadyProcessed = !m_postProcessedChunks.insert(key).second;
            }
            if (!alreadyProcessed) {
                m_world->onChunkLoaded(x, z);
                if (m_chunkLoadedCallback) {
                    m_chunkLoadedCallback(x, z);
                }
            }
        }
        // noteStorageResolved(true) 后 decision 通常 shouldWakeReadyWaiters（LoadedFromStorage→Ready
        // 由 _storeChunkInMemorySync 的 markLoadedFromStorageReady 完成）。若有等待者未唤醒则补唤醒。
        if (decision.shouldWakeReadyWaiters) {
            _completeReadyWaiters(lifecycleManager);
        }
        return;
    }

    // 存档缺失：走生成链路（由 ChunkTaskScheduler 调度）。
    auto decision = lifecycleManager.noteStorageResolved(false);
    _advanceChunkState(lifecycleManager, decision);
}

void ServerChunkManager::_drainPendingLoadCompletes()
{
    std::vector<PendingLoadComplete> pending;
    {
        std::lock_guard<std::mutex> lock(m_pendingLoadCompletesMutex);
        pending.swap(m_pendingLoadCompletes);
    }

    for (auto& item : pending) {
        _onChunkLoadComplete(item.x, item.z, item.dimension, std::move(item.result), std::move(item.lifecycleHolder));
    }
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

    auto lifecycleManager = std::make_shared<SingleChunkLifecycleManager>(x, z);
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

std::shared_ptr<SingleChunkLifecycleManager> ServerChunkManager::_findLifecycleManagerShared(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);
    std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
    auto it = m_lifecycleManagers.find(key);
    if (it != m_lifecycleManagers.end()) {
        return it->second;
    }
    return nullptr;
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

    // 票据级别降至卸载阈值以下：标记卸载意图，由 _checkChunkUnloading 安全卸载。
    //
    // 语义：
    //   - 标记卸载意图时取消生成任务，但 holder 仅在 isSafeToUnload
    //     （totalNeighboursUsingThisChunk==0 且无依赖图）时才真正卸载。
    //     被其他 holder 当作邻居使用或正在生成中的 holder 不会被取消/卸载，
    //     其生成由邻居的 checkNeighbour 按需驱动，完成后 _checkChunkUnloading 卸载。
    //
    // Cubium 之前的实现对所有 out-of-range holder 都调用 cancelGeneration + cancelActiveWork，
    // 即使该 holder 正被邻居使用或正在生成中。这导致 cancel-revive 抖动：
    //   1. _onTicketLevelChanged 取消 N（cancelGeneration + cancelActiveWork 置 abortSignal）。
    //   2. cancelGeneration 重新调度等待 N 的活跃邻居 C（rescheduleChunk）。
    //   3. C 的 schedule→checkNeighbour 遇到已取消的 N，reviveForScheduling 复活 N 并重新生成。
    //   4. mover 下次移动再次取消 N → 回到步骤 2，产生大量重复生成工作，2 worker 无法消化。
    //
    // 修复：仅当 holder 已安全可卸载（isSafeToUnload）时才立即取消生成并清理依赖图。
    //   - isSafeToUnload 为 false（holder 正被邻居使用/正在生成/有依赖图）：不取消生成，
    //     不失败等待者。holder 的生成继续由邻居 checkNeighbour 驱动，请求等待者
    //     （getChunkAsync）在 holder 完成 FULL 后由 _publishGeneratedChunk/_finalizeGeneratedChunkSync 唤醒。
    //     holder 的 abortSignal 保持 false，运行中的任务正常完成。当依赖图清空
    //     （邻居完成/取消释放引用、等待者解除），_checkChunkUnloading 通过 isSafeToUnload 卸载 holder。
    //   - isSafeToUnload 为 true（holder 无依赖）：安全取消生成并清理依赖图，_checkChunkUnloading 卸载。
    //
    // 注意：isSafeToUnload 在此处的检查不持调度锁（_onTicketLevelChanged 由 tick 主线程调用，
    // 非 worker 线程的 onChunkGenComplete/schedule 持锁路径）。isSafeToUnload 的最终一致性由
    // unloadChunkSync 持调度锁重新检查保证。此处仅作为快速过滤：若 isSafeToUnload 为 false，
    // 立即取消会导致 cancel-revive 抖动；若为 true，cancelGeneration 持锁清理后 unloadChunkSync
    // 再次检查（持锁）确认安全后移除 holder。
    if (m_taskScheduler != nullptr && lifecycleManager.isSafeToUnload()) {
        m_taskScheduler->cancelGeneration(lifecycleManager);
        lifecycleManager.cancelActiveWork();
        _failWaiters(lifecycleManager.takeAllWaiters());
    }
    // isSafeToUnload 为 false 时：不取消、不失败等待者。holder 保留在 m_lifecycleManagers 中，
    // 由 _checkChunkUnloading（每 UNLOAD_CHECK_INTERVAL_TICKS tick）在安全后卸载。
    // holder 的 shouldLoad() 已为 false（newLevel > MAX_LOADED_LEVEL），_checkChunkUnloading 会尝试卸载。
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
        // INITIALIZE_LIGHT 为空操作。
        // 光源收集由 BlockStarLightEngine::_getSources 在 LIGHT 阶段完成，
        // initializeLightSources 写入的 ChunkSection nibble 不被 StarLight 引擎读取（冗余）。
    } else if (status == ChunkStatuses::LIGHT) {
        // 在 worker 线程执行光照计算（LIGHT 阶段管线内完成，区块推进到 SPAWN 前光照就绪）。
        // 光照针对 primer 底层 ChunkData（ChunkPrimer 未重写光照 nibble 接口）。
        // markLightChanged 由 ChunkLightingProvider 吞掉（区块尚未发布，网络回写无效且非线程安全）；
        // light() 已通过 setNibbles 将结果写入 ChunkData 的 SWMRNibbleArray。
        ServerWorld* world = m_world;
        WorldLightManager* lightManager = (world != nullptr) ? world->lightManager() : nullptr;
        if (lightManager == nullptr) {
            return;
        }

        ChunkData* chunkData = chunk.getChunkData();
        const ChunkCoord chunkX = chunk.x();
        const ChunkCoord chunkZ = chunk.z();

        // 计算空区块段标记，更新空映射与区块段状态（与 LightSyncManager::initializeChunkLighting 的 else 分支一致）。
        // updateEmptinessMap / updateSectionStatus / lightChunk 均经 WorldLightManager::m_mutex 串行化，
        // 保证 worker 线程与主线程 tick/checkBlock 不并发修改引擎状态。
        const ChunkSection* const* sections = chunkData->getSections();
        constexpr i32 sectionCount = world::CHUNK_SECTIONS;
        for (i32 sectionY = 0; sectionY < sectionCount; ++sectionY) {
            const ChunkSection* section = (sections != nullptr) ? sections[static_cast<size_t>(sectionY)] : nullptr;
            const bool isEmpty = (section == nullptr || section->isEmpty());
            SectionPos sectionPos(chunkX, world::sectionIndexToCoord(sectionY), chunkZ);
            lightManager->updateSectionStatus(sectionPos, isEmpty);
        }

        // 更新方块光引擎空映射（经 WorldLightManager 加锁；仅方块光有空映射）
        lightManager->updateEmptinessMap(chunkX, chunkZ, chunkData);

        // 构造光照提供者：中心+半径1邻居经 WorldGenRegion 取 ChunkPrimer 底层 ChunkData，
        // 半径2邻居 fallback 到 ServerWorld::getChunkForLight。
        ChunkLightingProvider provider(*world, region, chunkX, chunkZ);

        chunkData->setLightCorrect(false);
        lightManager->lightChunk(&provider, chunkData, /*needsEdgeChecks=*/true);
        chunkData->setLightCorrect(true);
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

    // onChunkLoaded / m_chunkLoadedCallback 不在此调用：它们触及主线程独占的世界状态
    // （EntityManager、POI、光照、ChunkSendManager 等），_storeChunkInMemorySync 可能在 worker 线程
    // （生成/存档加载）被调用。由调用方负责：worker 路径经 _enqueuePostProcess 延迟到主线程 tick()，
    // 主线程路径（_resolveChunkSourceSync）直接调用。

    return stored;
}

ChunkData* ServerChunkManager::_finalizeGeneratedChunkSync(ChunkCoord x, ChunkCoord z, ChunkPrimer& primer)
{
    MC_TRACE_EVENT("server.chunk", "ServerChunkManager::finalizeGeneratedChunkSync", "x", x, "z", z);

    std::vector<SpawnedEntityData> spawnedEntities;
    if (primer.spawnedEntityCount() > 0) {
        spawnedEntities = std::move(primer.spawnedEntities());
    }

    // toChunkData 非破坏性：返回 shared_ptr 共享同一份 ChunkData，primer 仍持有 m_data。
    // FULL 完成后 currentChunk（primer）仍存活供邻居引用，直到 holder 卸载。
    std::shared_ptr<ChunkData> data = primer.toChunkData();
    if (!data) {
        return nullptr;
    }

    ChunkData* stored = _storeChunkInMemorySync(x, z, std::move(data));

    // spawnEntitiesFromChunkGeneration / _postProcessChunk / onChunkLoaded / m_chunkLoadedCallback 触及
    // 主线程独占状态（ServerTickList、EntityManager、setBlockState、POI、光照），不能在 worker 线程调用。
    // 入队后由主线程 tick() 的 _drainPendingPostProcess 出队执行（worker 线程入队、主线程出队）。
    // _storeChunkInMemorySync 已不再调用 onChunkLoaded/callback，
    // 故此处单个 PendingPostProcess 条目覆盖全部四项主线程工作。
    if (stored && m_world) {
        _enqueuePostProcess(x, z, std::move(spawnedEntities), /*needsPostProcess=*/true);
    } else if (m_entitySpawnCallback && !spawnedEntities.empty()) {
        // 无 world（独立/测试模式）：实体生成回调不触及世界状态，直接在 worker 线程调用。
        m_entitySpawnCallback(spawnedEntities);
    }

    return stored;
}

void ServerChunkManager::_publishGeneratedChunk(SingleChunkLifecycleManager& holder, const ChunkStatus& completedStatus)
{
    MC_TRACE_EVENT("server.chunk", "ServerChunkManager::publishGeneratedChunk");

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
                    const auto& fluid = fluidState->getFluid();
                    m_world->tickManager().scheduleFluidTick(pos, fluid, fluid.getTickDelay(*m_world));
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

    // 持有调度区域锁（覆盖 2 * maxAccessRadius，与 onChunkGenComplete/schedule 一致）后移除 holder。
    // 这把锁与 worker 线程的 onChunkGenComplete/schedule/checkNeighbour 互斥：
    //   - worker 持锁期间修改依赖图（m_waitingNeighbours/m_blockingNeighbours/m_generationTask/
    //     m_neighboursUsingThisChunk），isSafeToUnload() 必然返回 false（依赖图非空或有进行中任务），
    //     unloadChunkSync 不会在此窗口销毁 holder。
    //   - 若 worker 刚 clearGenerationTask 且 notifyWaitingNeighbours 未完成，m_waitingNeighbours 非空，
    //     isSafeToUnload() 仍为 false。
    //   - 锁保证 unloadChunkSync 读取 isSafeToUnload 与 worker 的依赖图修改不会交错。
    // 若 isSafeToUnload() 为 false（仍有进行中生成或邻居引用），先 cancelGeneration 清理依赖图
    // （取消任务、释放邻居引用、解除双向依赖、通知等待者），使 holder 可安全卸载。
    std::shared_ptr<SingleChunkLifecycleManager> lifecycleManager;
    {
        std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
        auto it = m_lifecycleManagers.find(key);
        if (it != m_lifecycleManagers.end()) {
            lifecycleManager = it->second;
        }
    }

    if (lifecycleManager == nullptr) {
        // holder 已不存在（可能被并发 unload 或从未创建），仅清理 m_chunks
        std::lock_guard<std::mutex> lock(m_chunksMutex);
        m_chunks.erase(key);
        // 清理后处理去重标记，使重新加载可重新执行 onChunkLoaded/callback/postProcess
        std::lock_guard<std::mutex> ppLock(m_pendingPostProcessMutex);
        m_postProcessedChunks.erase(key);
        return;
    }

    if (m_taskScheduler != nullptr) {
        const i32 lockRadius = 2 * ChunkTaskScheduler::getMaxAccessRadius();
        auto schedLock = m_taskScheduler->schedulingLockArea().lock(x, z, lockRadius);

        // cancelGeneration 清理依赖图：取消生成任务（abortSignal 由 cancelActiveWork 设置）、
        // 补偿释放邻居引用计数、解除双向依赖（m_blockingNeighbours/m_waitingNeighbours）、
        // 重新调度被解除阻塞的邻居、通知请求等待者失败。
        // 清理后 isSafeToUnload 应返回 true（依赖图空、无生成任务）。
        // 唯一例外：m_neighboursUsingThisChunk > 0（其他 holder 的任务正在使用本 holder 作为邻居），
        // 此时跳过卸载，下个 tick 重试（isSafeToUnload 的 neighbours_generating 检查）。
        m_taskScheduler->cancelGeneration(*lifecycleManager);
        lifecycleManager->cancelActiveWork();

        if (!lifecycleManager->isSafeToUnload()) {
            // 仍有邻居引用（其他 holder 的任务正在使用本 holder）：跳过本次卸载，下个 tick 重试。
            // cancelGeneration 已清理依赖图与生成任务，下个 tick 的 cancelGeneration 多为空操作。
            // holder 保留在 m_lifecycleManagers 中，shared_ptr 在此作用域释放。
            return;
        }

        // 持锁下安全：从 m_lifecycleManagers 移除（释放 map 的引用），lifecycleManager shared_ptr 仍持有。
        // 注意：cancelGeneration 可能已重新调度被解除阻塞的邻居，邻居的 checkNeighbour→getOrCreateHolder
        // 可能重建本 holder（若邻居仍需要它）。此处移除的是"旧的、已取消的"holder 实例；
        // 若被重建，getOrCreateHolder 会创建新实例，与旧实例无关。
        {
            std::lock_guard<std::mutex> lmLock(m_lifecycleManagersMutex);
            // 重新检查：cancelGeneration 重新调度的邻居可能已通过 getOrCreateHolder 重建了同坐标 holder。
            // 若已重建（map 中的实例不再是 lifecycleManager），只移除与 lifecycleManager 匹配的实例。
            auto it = m_lifecycleManagers.find(key);
            if (it != m_lifecycleManagers.end() && it->second.get() == lifecycleManager.get()) {
                m_lifecycleManagers.erase(it);
            }
        }
        _failWaiters(lifecycleManager->takeAllWaiters());
        // schedLock 释放时释放调度锁
    } else {
        // 无调度器（独立/测试模式）：直接检查并移除
        lifecycleManager->cancelActiveWork();
        if (!lifecycleManager->isSafeToUnload()) {
            return;
        }
        {
            std::lock_guard<std::mutex> lmLock(m_lifecycleManagersMutex);
            auto it = m_lifecycleManagers.find(key);
            if (it != m_lifecycleManagers.end() && it->second.get() == lifecycleManager.get()) {
                m_lifecycleManagers.erase(it);
            }
        }
        _failWaiters(lifecycleManager->takeAllWaiters());
    }

    // lifecycleManager（shared_ptr）在此作用域结束时释放。
    // 若仍有 ChunkProgressionTask 持有该 holder 的 shared_ptr（worker 线程正在执行 onChunkGenComplete），
    // holder 不会被销毁，直到任务完成回调释放 shared_ptr。这消除了 worker 线程访问已释放 holder 的
    // use-after-free 竞态（unloadChunkSync 与 onChunkGenComplete 并发）。

    {
        std::lock_guard<std::mutex> lock(m_chunksMutex);
        m_chunks.erase(key);
    }

    // 清理后处理去重标记，使重新加载可重新执行 onChunkLoaded/callback/postProcess
    {
        std::lock_guard<std::mutex> ppLock(m_pendingPostProcessMutex);
        m_postProcessedChunks.erase(key);
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

    // 出队并处理异步存档加载完成回调（ServerCompute 线程入队的结果）。
    // 必须在 _drainPendingPostProcess 之前：加载完成把区块存入内存缓存并推进状态机，
    // 后处理（onChunkLoaded/callback）依赖区块已在缓存中。
    _drainPendingLoadCompletes();

    // 出队并执行 worker 线程入队的主线程后处理任务（onChunkLoaded / m_chunkLoadedCallback /
    // spawnEntitiesFromChunkGeneration / _postProcessChunk）。这些触及主线程独占状态
    // （ServerTickList、EntityManager、setBlockState、POI、光照），必须在主线程执行。
    _drainPendingPostProcess();

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

void ServerChunkManager::_enqueuePostProcess(
    ChunkCoord x, ChunkCoord z, std::vector<SpawnedEntityData>&& spawnedEntities, bool needsPostProcess)
{
    PendingPostProcess item;
    item.x = x;
    item.z = z;
    item.spawnedEntities = std::move(spawnedEntities);
    item.needsPostProcess = needsPostProcess;
    {
        std::lock_guard<std::mutex> lock(m_pendingPostProcessMutex);
        m_pendingPostProcess.push_back(std::move(item));
    }
}

void ServerChunkManager::_drainPendingPostProcess()
{
    std::vector<PendingPostProcess> pending;
    {
        std::lock_guard<std::mutex> lock(m_pendingPostProcessMutex);
        pending.swap(m_pendingPostProcess);
    }

    // TODO: 当前后处理在 FULL-complete 时触发；Moonrise 在 BLOCK_TICKING（onChunkTicking）阶段触发。
    // 待引入 FullChunkStatus 状态机（onChunkBorder/onChunkTicking/onChunkEntityTicking + ticking-chunk
    // 列表）后对齐触发时机。当前 Cubium 无该状态机，FULL-complete 是唯一合理触发点。
    for (auto& item : pending) {
        const u64 key = posToKey(item.x, item.z);

        // 去重：同一区块的 onChunkLoaded/callback/spawn/postProcess 至多执行一次。
        // 重复入队（worker 存档加载入队 + 主线程存档解析直接调用竞态，或同一区块多次入队）
        // 的条目被丢弃，避免实体重复生成、区块重复发送、光照重复初始化。
        // 卸载时移除 key（unloadChunkSync），使重新加载可重新执行后处理。
        {
            std::lock_guard<std::mutex> lock(m_pendingPostProcessMutex);
            if (!m_postProcessedChunks.insert(key).second) {
                continue; // 已处理，丢弃重复项（spawnedEntities 随 item 析构，不 spawn）
            }
        }

        // onChunkLoaded：加载区块内实体（存档恢复）+ POI 通知
        if (m_world) {
            m_world->onChunkLoaded(item.x, item.z);
        }

        // m_chunkLoadedCallback：光照初始化 + 区块发送等（MinecraftServer 设置）
        if (m_chunkLoadedCallback) {
            m_chunkLoadedCallback(item.x, item.z);
        }

        // spawnEntitiesFromChunkGeneration：区块生成产生的被动实体
        if (!item.spawnedEntities.empty() && m_world) {
            m_world->spawnEntitiesFromChunkGeneration(item.spawnedEntities);
        }

        // _postProcessChunk：含水层流体 tick 调度 + 方块形状更新（生成路径才需要）
        // isPostProcessingDone 双重保护：即使 needsPostProcess 重复入队，也只执行一次。
        if (item.needsPostProcess && m_world) {
            ChunkData* chunk = tryToGetChunkInMem(item.x, item.z);
            if (chunk != nullptr && !chunk->isPostProcessingDone()) {
                _postProcessChunk(*chunk);
                chunk->setPostProcessingDone(true);
            }
        }
    }
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

void ServerChunkManager::_debugDumpStuckHolders() const
{
    std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
    size_t stuckCount = 0;
    size_t waitingForNeighbors = 0;
    size_t hasGenTask = 0;
    size_t neighboursUsingCount = 0;
    for (const auto& [key, lifecycleManager] : m_lifecycleManagers) {
        if (!lifecycleManager) {
            continue;
        }
        if (lifecycleManager->isSafeToUnload()) {
            continue;
        }
        ++stuckCount;
        const bool isWaiting = lifecycleManager->isWaitingForNeighbors();
        const bool genTask = lifecycleManager->hasGenerationTask();
        const i32 neighboursUsing = lifecycleManager->neighboursUsingThisChunkCount();
        if (isWaiting) {
            ++waitingForNeighbors;
        }
        if (genTask) {
            ++hasGenTask;
        }
        if (neighboursUsing > 0) {
            ++neighboursUsingCount;
        }
        // 仅打印前 20 个卡住的 holder，避免日志爆炸
        if (stuckCount <= 20) {
            spdlog::info(
                "[stuck] ({}, {}) genStatus={} reqStatus={} genTask={} blocking={} waiting={} neighboursUsing={} "
                "failed={} source={}",
                lifecycleManager->x(),
                lifecycleManager->z(),
                lifecycleManager->getCurrentGenStatus().name(),
                lifecycleManager->requestedGenStatus().name(),
                genTask,
                lifecycleManager->blockingNeighbourCount(),
                isWaiting,
                neighboursUsing,
                lifecycleManager->hasFailedGeneration(),
                static_cast<int>(lifecycleManager->sourceState()));
        }
    }
    spdlog::info("[stuck] 总计卡住的 holder={}，其中 waitingForNeighbors={} hasGenTask={} neighboursUsing={}",
        stuckCount,
        waitingForNeighbors,
        hasGenTask,
        neighboursUsingCount);
}

} // namespace mc::server
