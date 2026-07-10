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
#include "common/profiler/TraceEvents.hpp"
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

using namespace mc::trace;

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
void _fulfillWaiters(std::vector<SingleChunkLifecycleManager::Waiter> waiters, bool success, ChunkData* chunk)
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
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "ServerChunkManager::initialize");
    return {};
}

void ServerChunkManager::shutdown()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Initialization, "ServerChunkManager::shutdown");

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
        MC_TRACE_SCOPED_EVENT(
            TraceEvents.Server.Initialization, "ServerChunkManager::shutdown::WaitForWorkerCompletion");
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
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk,
        "ServerChunkManager::_submitChunkRequest",
        "x",
        x,
        "z",
        z,
        "targetStatus",
        targetStatus.name());

    if (ChunkData* chunk = tryToGetChunkInMem(x, z)) {
        std::vector<SingleChunkLifecycleManager::Waiter> waiters;
        waiters.emplace_back(SingleChunkLifecycleManager::Waiter{std::move(callback), std::move(promise)});
        _fulfillWaiters(std::move(waiters), true, chunk);
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
    // SCM 层去重：若该区块已有在途加载（cancel-revive 重建场景），附加当前 SCLM 为等待者，
    // 不重新发起 loadChunkAsyncCallback。所有者完成后 _onChunkLoadComplete 遍历 attachedWaiters
    // 推进其状态机（命中→markLoadedFromStorageReady，缺失→生成），避免重复 RocksDB 读取。
    {
        std::lock_guard<std::mutex> lock(m_pendingLoadTasksMutex);
        auto it = m_pendingLoadTasks.find(posToKey(x, z));
        if (it != m_pendingLoadTasks.end()) {
            // 已有在途加载：附加当前 SCLM 为等待者，不重新发起加载。
            // 所有者的 abortSignal 独立于本 SCLM；本 SCLM 的 sourceState 已是 ResolvingStorage
            // （submitRequest 设置），所有者完成后 noteStorageResolved 推进。
            it->second.attachedWaiters.push_back({lifecycleHolder});
            return;
        }
        // 无在途加载：插入 owner 条目，下方继续发起 loadChunkAsyncCallback。
        PendingLoadEntry entry;
        entry.ownerLifecycle = lifecycleHolder;
        m_pendingLoadTasks[posToKey(x, z)] = std::move(entry);
    }

    const auto dimension = m_world->dimension();

    // 异步发起：ServerIO 读盘（section+blockEntity 两路并行）→ ServerCompute 反序列化组装。
    // 完成回调在 ServerCompute 线程执行，仅把结果入队 m_pendingLoadCompletes，不触及主线程独占状态。
    // abortSignal 来自 SCLM，unloadChunkSync→cancelActiveWork 会置位，使 ServerIO/ServerCompute 任务协作取消。
    // priority 来自 SCLM 请求优先级（玩家附近 High、远处 Low），传播到 ServerIO/ServerCompute 任务队列。
    auto abortSignal = lifecycleManager.abortSignal();
    const util::TaskPriority taskPriority = mapSclmPriorityToTaskPriority(lifecycleManager.requestPriority());
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
                if (self->m_pendingLoadCompletes.size() > PENDING_LOAD_COMPLETES_WARN_THRESHOLD) {
                    spdlog::warn("Pending load-completes backlog reached {} (threshold {}); main tick may be lagging",
                        self->m_pendingLoadCompletes.size(),
                        PENDING_LOAD_COMPLETES_WARN_THRESHOLD);
                }
            }
        },
        abortSignal,
        taskPriority);
}

void ServerChunkManager::_onChunkLoadComplete(ChunkCoord x,
    ChunkCoord z,
    mc::DimensionId dimension,
    mc::Result<std::optional<mc::ChunkData>> result,
    std::shared_ptr<SingleChunkLifecycleManager> lifecycleHolder)
{
    const u64 key = posToKey(x, z);

    // 取出条目（owner + attachedWaiters），从追踪表移除。
    // 仅当 lifecycleHolder 仍是本条目的 owner 时才处理并扇出（防止 owner 已被 unload 重建后
    // 新 owner 的条目被旧回调误消费）。
    PendingLoadEntry entry;
    bool wasOwner = false;
    {
        std::lock_guard<std::mutex> lock(m_pendingLoadTasksMutex);
        auto it = m_pendingLoadTasks.find(key);
        if (it != m_pendingLoadTasks.end()) {
            if (it->second.ownerLifecycle.get() == lifecycleHolder.get()) {
                wasOwner = true;
                entry = std::move(it->second);
                m_pendingLoadTasks.erase(it);
            }
        }
    }
    if (!wasOwner) {
        // lifecycleHolder 不是 owner（owner 已被 unload 重建且新 owner 发起了新加载，
        // 或回调对应的加载从未入表）。丢弃本次结果。新 owner 的加载独立完成。
        return;
    }

    // 实例一致性校验：owner SCLM 可能已被 unloadChunkSync 从 m_lifecycleManagers 移除。
    // 若已移除，跳过 owner 自身的状态推进（noteStorageResolved 会在已析构/复用的 SCLM 上调用，
    // 但 lifecycleHolder shared_ptr 仍存活，持有旧实例）。附加等待者仍需扇出。
    auto currentHolder = _findLifecycleManagerShared(x, z);
    const bool ownerAlive = (currentHolder != nullptr && currentHolder.get() == lifecycleHolder.get());

    // owner 结果处理（仅在 owner 仍存活时推进 owner SCLM 状态）。
    bool hit = false;
    if (ownerAlive) {
        SingleChunkLifecycleManager& lifecycleManager = *lifecycleHolder;

        if (result.failed()) {
            spdlog::warn("Async load chunk failed ({}, {}): {}", x, z, result.error().message());
            auto decision = lifecycleManager.noteStorageResolved(false);
            _advanceChunkState(lifecycleManager, decision);
            // 扇出附加等待者（同样走生成路径）
            _fanOutAttachedWaiters(x, z, entry.attachedWaiters, false);
            return;
        }

        std::optional<ChunkData> chunkOpt = std::move(result).value();
        hit = chunkOpt.has_value();
        if (hit) {
            auto decision = lifecycleManager.noteStorageResolved(true);
            std::unique_ptr<ChunkData> loadedChunk = std::make_unique<ChunkData>(std::move(chunkOpt.value()));
            // 从存档 ChunkData 创建 primer（对齐 Moonrise ChunkLoadTask 设 currentChunk）。
            // 存档命中 holder 的 currentChunk 必须非空：经 checkNeighbour 注册为生成邻居的依赖者
            // （如中心区块 STRUCTURE_REFERENCES/FEATURES 读邻居）会在 buildNeighbourCache 取
            // getCurrentChunk()，若为空触发 "neighbour primer missing" 断言。primer 与 m_chunks
            // 共享同一份 ChunkData（shareChunkData 非破坏性，不触发 toChunkData 收尾，避免 setBiomes
            // 用 primer 未填充的 m_biomes 清空存档生物群系）。primer 构造置 chunkStatus=FULL、
            // status=Loaded，与 markLoadedFromStorageReady(FULL) 的 currentGenStatus 一致。
            auto primer = std::make_unique<ChunkPrimer>(std::move(loadedChunk));
            std::shared_ptr<ChunkData> sharedData = primer->shareChunkData();
            lifecycleManager.setCurrentChunk(std::move(primer));
            ChunkData* stored = _storeChunkInMemorySync(x, z, std::move(sharedData));
            if (stored && m_world) {
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
            // markLoadedFromStorageReady 在 _storeChunkInMemorySync 内部完成（sourceState→Ready，
            // currentGenStatus→FULL）。存档命中不走生成任务，故不触发 onChunkGenComplete→
            // notifyWaitingNeighbours；经 checkNeighbour 注册到本 holder m_waitingNeighbours 的依赖邻居
            // （如中心区块生成时 checkNeighbour 发现本 holder 在 ResolvingStorage，schedule 返回 nullptr
            // 挂起等待）若不显式通知将永久阻塞——本 holder 已就绪但邻居永不被唤醒（死锁）。
            // 此处对齐 onChunkGenComplete 的通知语义，解除依赖邻居阻塞并重新调度它们。
            if (decision.shouldWakeReadyWaiters) {
                _completeReadyWaiters(lifecycleManager);
            }
            if (m_taskScheduler != nullptr) {
                m_taskScheduler->onLoadedFromStorageReady(lifecycleManager);
            }
        } else {
            // 存档缺失：走生成链路（由 ChunkTaskScheduler 调度）。
            auto decision = lifecycleManager.noteStorageResolved(false);
            _advanceChunkState(lifecycleManager, decision);
        }
    }

    // 扇出附加等待者（cancel-revive 重建的 SCLM）。
    // hit 为 owner 结果的存档命中标志（owner 已 unload 时 hit=false，等待者走生成路径——
    //   owner 卸载意味着其加载被 abort，存档结果不可信，等待者应自行重新解析来源）。
    _fanOutAttachedWaiters(x, z, entry.attachedWaiters, hit);
}

void ServerChunkManager::_fanOutAttachedWaiters(
    ChunkCoord x, ChunkCoord z, std::vector<PendingLoadEntry::AttachedWaiter>& waiters, bool hit)
{
    for (auto& waiter : waiters) {
        if (waiter.lifecycle == nullptr) {
            continue;
        }
        // 实例校验：等待者可能已被 unload 重建（m_lifecycleManagers 中无此实例或已替换）。
        if (_findLifecycleManager(x, z) != waiter.lifecycle.get()) {
            continue;
        }
        SingleChunkLifecycleManager& waiterSclm = *waiter.lifecycle;
        // 等待者 SCLM 仍是 ResolvingStorage（被附加时 submitRequest 设置，未推进）。
        // noteStorageResolved 推进状态机，与 owner 的处理对称。
        auto decision = waiterSclm.noteStorageResolved(hit);
        if (hit) {
            // 命中：区块已在 m_chunks（owner 存入）。推进 SCLM 到 Ready 并唤醒等待者。
            // 不重复 _storeChunkInMemorySync（区块已在 m_chunks）。
            // 与 owner 命中路径对称：从已发布的 ChunkData 创建 primer 设到 waiter SCLM 的
            // currentChunk（共享所有权），否则经 checkNeighbour 注册为生成邻居的依赖者在
            // buildNeighbourCache 取 getCurrentChunk() 为空 → "neighbour primer missing" 断言。
            if (std::shared_ptr<ChunkData> sharedData = tryToGetChunkSharedInMem(x, z)) {
                waiterSclm.setCurrentChunk(std::make_unique<ChunkPrimer>(std::move(sharedData)));
            }
            waiterSclm.markLoadedFromStorageReady(ChunkStatuses::FULL);
            _completeReadyWaiters(waiterSclm);
            // 与 owner 命中路径对称：通知经 checkNeighbour 注册到本等待者 m_waitingNeighbours
            // 的依赖邻居解除阻塞并重新调度，否则它们永久等待一个已就绪的 holder（死锁）。
            if (m_taskScheduler != nullptr) {
                m_taskScheduler->onLoadedFromStorageReady(waiterSclm);
            }
        } else {
            // 缺失/失败：走生成链路。
            _advanceChunkState(waiterSclm, decision);
        }
    }
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

void ServerChunkManager::_resolveStorageForScheduling(
    SingleChunkLifecycleManager& lifecycleManager, const ChunkStatus& targetStatus)
{
    // 把 Unknown holder 推进到 ResolvingStorage 并发起异步存档读取，对齐 Moonrise pristine holder
    // 经 ChunkLoadTask 先试磁盘再回落空 ProtoChunk 的语义。无 callback/promise：纯调度驱动，
    // 不挂请求等待者（依赖邻居已由 checkNeighbour 注册到 m_waitingNeighbours，存档解析完成后由
    // onLoadedFromStorageReady/notifyWaitingNeighbours 解除阻塞；FULL 发布由 _storeChunkInMemorySync
    // 内部 _completeReadyWaiters 处理已挂的请求等待者）。
    // 优先级用 INT_MAX（不提升）：本路径由 checkNeighbour 在 worker 线程的 schedule 内调用，
    // 不可读取 m_ticketManager.getChunkLevel（ChunkDistanceGraph.m_levels 无锁 unordered_map，
    // worker 与主线程 processTicketUpdatesSync 并发读会数据竞争）。Unknown 邻居 holder 无直接请求，
    // 低优先级读盘可接受；持有者后续若被 _onTicketLevelChanged 直接请求，submitRequest 会收敛到
    // 更高优先级（数值更小）。submitRequest 见 m_requestPriority==INT_MAX 分支保持 INT_MAX。
    constexpr i32 kNeighbourResolvePriority = std::numeric_limits<i32>::max();
    auto decision = lifecycleManager.submitRequest(targetStatus, kNeighbourResolvePriority, {}, {});
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
    // FULL 区块已发布到 m_chunks：优先用内存缓存中的 ChunkData 完成等待者（Ready 稳态路径）。
    ChunkData* chunk = tryToGetChunkInMem(lifecycleManager.x(), lifecycleManager.z());
    if (chunk != nullptr) {
        _fulfillWaiters(lifecycleManager.takeReadyWaiters(), true, chunk);
        return;
    }

    // 非 FULL 但已达请求状态（StorageMissing 中间态，如请求 STRUCTURE_REFERENCES 已完成）：
    // 区块数据在 primer 的 m_currentChunk 中，未发布到 m_chunks（仅 FULL 入 m_chunks）。
    // takeAllWaiters 取出所有等待者（不要求 sourceState==Ready，_buildDecisionLocked 的非 Ready
    // 已达请求状态分支已保证仅在数据就绪且无在途任务时触发），从 primer 的 ChunkData 完成它们。
    // 与 _publishGeneratedChunk 同语义，避免 waiter 在中间态永久悬挂（死锁）。
    ChunkPrimer* primer = lifecycleManager.getCurrentChunk();
    ChunkData* primerData = (primer != nullptr) ? primer->getChunkData() : nullptr;
    _fulfillWaiters(lifecycleManager.takeAllWaiters(), primerData != nullptr, primerData);
}

void ServerChunkManager::_failWaiters(std::vector<SingleChunkLifecycleManager::Waiter> waiters)
{
    _fulfillWaiters(std::move(waiters), false, nullptr);
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
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk,
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
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk,
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
        // CARVERS 之后释放不再需要的生成态数据
        // m_noiseChunk（最后在 applyCarvers 中读取）、m_carvingMask（仅 applyCarvers 使用）
        chunk.releaseGenOnlyData(status);
    } else if (status == ChunkStatuses::FEATURES) {
        // 在特性放置前 prime POST_FEATURES 高度图
        // CARVERS 完成后方块数据已就绪，但 POST_FEATURES 高度图尚未创建
        chunk.primeHeightmaps(HeightmapFlag::POST_FEATURES);
        m_generator->placeFeatures(region, chunk);
        // FEATURES 之后释放不再需要的生成态数据
        // m_structureReferences（最后在 placeFeatures 中读取）
        chunk.releaseGenOnlyData(status);
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

util::TaskPriority ServerChunkManager::mapSclmPriorityToTaskPriority(i32 sclmPriority)
{
    // 无优先级（INT_MAX = cancelActiveWork 重置后未重新调度）→ 最低
    if (sclmPriority == std::numeric_limits<i32>::max()) {
        return util::TaskPriority::Low;
    }
    // 恢复 normalizedLevel（票据级别，越小越靠近玩家）。
    // _computeSchedulePriority = normalizedLevel*1024 + statusPenalty*32 + spatialPenalty(0..255)，
    // statusPenalty*32+spatialPenalty < 1024，故 /1024 无损恢复 normalizedLevel。
    const i32 normalizedLevel = sclmPriority / 1024;
    // MAX_LOADED_LEVEL（玩家视距内 FULL 区块）→ High，优先于仍在生成流水线的远处区块。
    if (normalizedLevel <= world::chunk::ChunkLoadTicketManager::MAX_LOADED_LEVEL) {
        return util::TaskPriority::High;
    }
    // 过渡区（刚超出加载边界）→ Normal
    if (normalizedLevel <= world::chunk::ChunkLoadTicketManager::MAX_LOADED_LEVEL + 2) {
        return util::TaskPriority::Normal;
    }
    return util::TaskPriority::Low;
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
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk, "ServerChunkManager::finalizeGeneratedChunkSync", "x", x, "z", z);

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
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk, "ServerChunkManager::publishGeneratedChunk");

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
    // takeAllWaiters 取出所有等待者（无条件），_fulfillWaiters 完成 promise。
    ChunkPrimer* primer = holder.getCurrentChunk();
    ChunkData* chunkData = (primer != nullptr) ? primer->getChunkData() : nullptr;

    _fulfillWaiters(holder.takeAllWaiters(), chunkData != nullptr, chunkData);
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

// ============================================================================
// 卸载
// ============================================================================

void ServerChunkManager::_notifyChunkUnload(ChunkCoord x, ChunkCoord z)
{
    if (m_world) {
        m_world->onChunkUnloading(x, z);
    }
    if (m_chunkSendManager) {
        m_chunkSendManager->onChunkPreUnload(x, z);
    }
    if (m_chunkUnloadedCallback) {
        m_chunkUnloadedCallback(x, z);
    }
}

void ServerChunkManager::unloadChunkSync(ChunkCoord x, ChunkCoord z)
{
    const u64 key = posToKey(x, z);

    // stage1（对齐 Moonrise unloadStage1）：捕获脏区块快照并提交异步保存，不阻塞主 tick。
    // 保存完成（stage2，ServerIO）后回调入队 m_pendingUnloadFinishes，由 stage3 完成卸载收尾。
    // 非脏区块（无存储或未修改）无需异步保存，直接进入 stage3 收尾。
    std::shared_ptr<SingleChunkLifecycleManager> lifecycleHolder = _findLifecycleManagerShared(x, z);

    bool asyncSaveStarted = false;
    if (m_world && m_world->isStorageOpen() && lifecycleHolder != nullptr) {
        std::shared_ptr<ChunkData> chunkToSave;
        {
            std::lock_guard<std::mutex> lock(m_chunksMutex);
            auto it = m_chunks.find(key);
            if (it != m_chunks.end() && it->second && it->second->isDirty()) {
                chunkToSave = it->second;
            }
        }

        if (chunkToSave) {
            // 标记异步卸载保存进行中，使 _checkChunkUnloading 跳过本区块，避免重复发起保存。
            // 持有 m_pendingUnloadFinishesMutex 与现有注释一致（m_unloadSaveInProgress 共用此锁）。
            {
                std::lock_guard<std::mutex> lock(m_pendingUnloadFinishesMutex);
                m_unloadSaveInProgress.insert(key);
            }

            // 立即清除脏标记：保存快照已捕获（saveChunkAsyncCallback 在主线程序列化），后续修改会重新置脏
            // 并在下次卸载时重新保存。避免 stage3 完成前重复保存。
            chunkToSave->setDirty(false);

            const auto dimension = m_world->dimension();
            auto* self = this;
            auto holderCopy = lifecycleHolder;
            // shared_ptr<const ChunkData> 共享所有权，避免 600KB 深拷贝；保存期间区块数据不可变
            // （卸载候选 isSafeToUnload，且序列化在主线程与 setBlockState 串行无数据竞争）。
            std::shared_ptr<const ChunkData> constChunk = std::move(chunkToSave);
            m_world->storage().saveChunkAsyncCallback(std::move(constChunk),
                dimension,
                [self, x, z, dimension, holderCopy = std::move(holderCopy)](
                    ChunkCoord /*cbX*/, ChunkCoord /*cbZ*/, mc::Result<void> result) {
                    // shutdown 后不再入队：ServerChunkManager 可能已析构或正在析构。
                    if (self->m_shuttingDown.load(std::memory_order::acquire)) {
                        return;
                    }
                    if (result.failed()) {
                        spdlog::error("Async save chunk failed ({}, {}): {}", x, z, result.error().message());
                    }
                    PendingUnloadFinish item;
                    item.x = x;
                    item.z = z;
                    item.dimension = dimension;
                    item.lifecycleHolder = std::move(holderCopy);
                    {
                        std::lock_guard<std::mutex> lock(self->m_pendingUnloadFinishesMutex);
                        self->m_pendingUnloadFinishes.push_back(std::move(item));
                    }
                });
            asyncSaveStarted = true;
        }
    }

    if (asyncSaveStarted) {
        // stage3 由 _drainPendingUnloadFinishes 在保存完成回调入队后执行。
        return;
    }

    // 非脏路径：直接进入 stage3 收尾（无异步保存等待）。
    _finalizeUnloadAfterSave(x, z, m_world ? m_world->dimension() : mc::DimensionId{}, std::move(lifecycleHolder));
}

bool ServerChunkManager::_finalizeUnloadAfterSave(
    ChunkCoord x, ChunkCoord z, mc::DimensionId dimension, std::shared_ptr<SingleChunkLifecycleManager> lifecycleHolder)
{
    const u64 key = posToKey(x, z);

    // 清理存储层进行中保存追踪条目（保存已完成，对齐 Moonrise unloadStage3 清空 chunkDataUnload）。
    if (m_world && m_world->isStorageOpen()) {
        m_world->storage()._removePendingChunkSave(x, z, dimension);
    }

    // 复检是否被重新请求（对齐 Moonrise unloadStage3 复检 currentChunk/entityChunk/poiChunk）：
    // 异步保存期间票据可能返回（玩家重新靠近），此时中止卸载，保留区块与 holder。
    // 区块数据已在 m_chunks（异步保存未移除它），重新加载经 _waitPendingChunkSave 读到保存后数据。
    if (lifecycleHolder != nullptr && lifecycleHolder->shouldLoad()) {
        std::lock_guard<std::mutex> lock(m_pendingUnloadFinishesMutex);
        m_unloadSaveInProgress.erase(key);
        return true; // 中止卸载，条目丢弃
    }

    std::shared_ptr<SingleChunkLifecycleManager> lifecycleManager = std::move(lifecycleHolder);

    if (lifecycleManager == nullptr) {
        // holder 已不存在（可能被并发 unload 或从未创建），仅清理 m_chunks。
        // 通知客户端卸载、移除实体（与有 holder 路径一致：仅在确定移除时触发）。
        _notifyChunkUnload(x, z);
        std::lock_guard<std::mutex> lock(m_chunksMutex);
        m_chunks.erase(key);
        std::lock_guard<std::mutex> ppLock(m_pendingPostProcessMutex);
        m_postProcessedChunks.erase(key);
        std::lock_guard<std::mutex> uLock(m_pendingUnloadFinishesMutex);
        m_unloadSaveInProgress.erase(key);
        return true;
    }

    if (m_taskScheduler != nullptr) {
        const i32 lockRadius = 2 * ChunkTaskScheduler::getMaxAccessRadius();
        auto schedLock = m_taskScheduler->schedulingLockArea().lock(x, z, lockRadius);

        m_taskScheduler->cancelGeneration(*lifecycleManager);
        lifecycleManager->cancelActiveWork();

        if (!lifecycleManager->isSafeToUnload()) {
            // 仍有邻居引用（其他 holder 的任务正在使用本 holder）：保留条目，下一 tick stage3 重试。
            // 区块已保存（落盘），holder 与 m_chunks 保留，下个 tick 重试时不再重复保存
            // （_finalizeUnloadAfterSave 不触发保存，仅完成移除）。m_unloadSaveInProgress 保留以阻止
            // _checkChunkUnloading 重复选中。未触发卸载通知，重试时不会重复通知客户端/移除实体。
            return false;
        }

        {
            std::lock_guard<std::mutex> lmLock(m_lifecycleManagersMutex);
            auto it = m_lifecycleManagers.find(key);
            if (it != m_lifecycleManagers.end() && it->second.get() == lifecycleManager.get()) {
                m_lifecycleManagers.erase(it);
            }
        }
        _failWaiters(lifecycleManager->takeAllWaiters());
    } else {
        // 无调度器（独立/测试模式）：直接检查并移除
        lifecycleManager->cancelActiveWork();
        if (!lifecycleManager->isSafeToUnload()) {
            return false;
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

    // 卸载通知（实体保存+移除、卸载发送、callback）。在 isSafeToUnload 通过、确定移除后触发：
    // 确保保存落盘后再通知客户端卸载、移除实体，且每次卸载仅触发一次（重试路径不触发）。
    _notifyChunkUnload(x, z);

    {
        std::lock_guard<std::mutex> lock(m_chunksMutex);
        m_chunks.erase(key);
    }
    {
        std::lock_guard<std::mutex> ppLock(m_pendingPostProcessMutex);
        m_postProcessedChunks.erase(key);
    }
    {
        std::lock_guard<std::mutex> lock(m_pendingLoadTasksMutex);
        auto it = m_pendingLoadTasks.find(key);
        if (it != m_pendingLoadTasks.end()) {
            auto& waiters = it->second.attachedWaiters;
            waiters.erase(std::remove_if(waiters.begin(),
                              waiters.end(),
                              [&lifecycleManager](const PendingLoadEntry::AttachedWaiter& w) {
                                  return w.lifecycle.get() == lifecycleManager.get();
                              }),
                waiters.end());
        }
    }
    {
        std::lock_guard<std::mutex> uLock(m_pendingUnloadFinishesMutex);
        m_unloadSaveInProgress.erase(key);
    }
    return true;
}

void ServerChunkManager::_drainPendingUnloadFinishes()
{
    std::vector<PendingUnloadFinish> pending;
    {
        std::lock_guard<std::mutex> lock(m_pendingUnloadFinishesMutex);
        pending.swap(m_pendingUnloadFinishes);
    }

    std::vector<PendingUnloadFinish> retry;
    for (auto& item : pending) {
        // 持有 holder 副本：_finalizeUnloadAfterSave 返回 false（重试）时 item.lifecycleHolder
        // 已被 move 置空，需用 holderForRetry 重新填入重试条目。
        std::shared_ptr<SingleChunkLifecycleManager> holderForRetry = item.lifecycleHolder;
        const bool done = _finalizeUnloadAfterSave(item.x, item.z, item.dimension, std::move(item.lifecycleHolder));
        if (!done) {
            // isSafeToUnload=false：保留至下一 tick 重试。holder 仍在 m_lifecycleManagers（未移除）。
            PendingUnloadFinish retryItem;
            retryItem.x = item.x;
            retryItem.z = item.z;
            retryItem.dimension = item.dimension;
            retryItem.lifecycleHolder = std::move(holderForRetry);
            retry.push_back(std::move(retryItem));
        }
    }

    if (!retry.empty()) {
        std::lock_guard<std::mutex> lock(m_pendingUnloadFinishesMutex);
        // 重试条目插回队尾，下一 tick 继续处理。
        for (auto& item : retry) {
            m_pendingUnloadFinishes.push_back(std::move(item));
        }
    }
}

void ServerChunkManager::_checkChunkUnloading()
{
    // 候选区块：可卸载的（shouldLoad=false 且无追踪玩家且 isSafeToUnload）
    std::vector<u64> toUnload;

    // 软上限强制卸载候选池：仍加载（shouldLoad=true）的区块及其 level。
    // level 越高 = 越远 = 越应优先卸载。仅当 m_maxLoadedChunks > 0 时收集。
    // pair: {level, key}，升序排序后从末尾（最大 level）取。
    std::vector<std::pair<i32, u64>> forcedCandidates;

    const bool enforceSoftCap = m_maxLoadedChunks > 0;
    size_t loadedCount = 0;

    // 复制一份进行中的卸载保存集合，避免在持锁遍历 m_lifecycleManagers 时
    // 再去加 m_pendingUnloadFinishesMutex（与 m_lifecycleManagersMutex 无固定次序，防死锁）。
    std::unordered_set<u64> saveInProgress;
    {
        std::lock_guard<std::mutex> lock(m_pendingUnloadFinishesMutex);
        saveInProgress = m_unloadSaveInProgress;
    }

    {
        std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
        for (const auto& [key, lifecycleManager] : m_lifecycleManagers) {
            if (!lifecycleManager) {
                continue;
            }

            // 已有异步卸载保存进行中（stage1 已发起、stage3 未完成）：跳过。
            // 由 _drainPendingUnloadFinishes 负责其 stage3 收尾，避免重复发起保存。
            if (saveInProgress.count(key) > 0) {
                continue;
            }

            // 票级高于 Border（shouldLoad=false）且无追踪玩家且可安全卸载 → 常规卸载候选
            if (!lifecycleManager->shouldLoad() && !m_ticketManager.hasTrackingPlayers(key) &&
                lifecycleManager->isSafeToUnload()) {
                toUnload.push_back(key);
                continue;
            }

            // 软上限统计：所有仍加载（shouldLoad=true）的区块计入强制卸载候选池。
            // 超限时按 level 降序（最远优先）强制卸载。
            if (enforceSoftCap && lifecycleManager->shouldLoad()) {
                forcedCandidates.emplace_back(lifecycleManager->level(), key);
                ++loadedCount;
            }
        }
    }

    // 1) 常规卸载：受每 tick 预算限制，平滑卸载尖峰（对齐 Moonrise processUnloads）
    i32 unloadBudget = MAX_UNLOADS_PER_TICK;
    for (u64 key : toUnload) {
        if (unloadBudget <= 0) {
            break;
        }
        auto chunkId = ChunkId::fromId(key);
        unloadChunkSync(chunkId.x, chunkId.z);
        --unloadBudget;
    }

    // 2) 软上限强制卸载：加载区块数超过 m_maxLoadedChunks 时，按最远优先强制卸载。
    //    forcedCandidates 升序排序后从末尾（最大 level = 最远）取，直到不再超限或预算耗尽。
    //    强制卸载也消耗同一预算，避免与常规卸载叠加造成单 tick 尖峰。
    if (enforceSoftCap && loadedCount > static_cast<size_t>(m_maxLoadedChunks)) {
        std::sort(forcedCandidates.begin(), forcedCandidates.end());
        const size_t excess = loadedCount - static_cast<size_t>(m_maxLoadedChunks);
        size_t unloaded = 0;
        for (auto it = forcedCandidates.rbegin();
            it != forcedCandidates.rend() && unloaded < excess && unloadBudget > 0;
            ++it) {
            auto chunkId = ChunkId::fromId(it->second);
            unloadChunkSync(chunkId.x, chunkId.z);
            ++unloaded;
            --unloadBudget;
        }
    }
}

// ============================================================================
// 票据与 tick
// ============================================================================

void ServerChunkManager::updatePlayerPosition(PlayerId player, f64 x, f64 z)
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Server.Chunk, "ServerChunkManager::updatePlayerPosition", "player", player, "x", x, "z", z);

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

    // 出队并执行异步卸载保存完成回调（ServerIO 线程入队的 stage3 收尾）。
    // 必须在 _checkChunkUnloading 之前：先消化进行中的卸载收尾，避免与新发起的卸载相互干扰，
    // 也确保 shouldLoad() 复检（玩家重新靠近）能及时中止卸载。
    _drainPendingUnloadFinishes();

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
        if (m_pendingPostProcess.size() > PENDING_POST_PROCESS_WARN_THRESHOLD) {
            spdlog::warn("Pending post-process backlog reached {} (threshold {}); main tick may be lagging",
                m_pendingPostProcess.size(),
                PENDING_POST_PROCESS_WARN_THRESHOLD);
        }
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

void ServerChunkManager::_debugDumpStuckHolders()
{
    // 诊断转储：无锁读取各队列 size（best-effort，仅用于测试诊断，轻微数据竞争可接受）
    spdlog::info("[stuck] m_pendingLoadCompletes={}", m_pendingLoadCompletes.size());
    spdlog::info("[stuck] m_pendingLoadTasks={}", m_pendingLoadTasks.size());
    spdlog::info("[stuck] m_unloadSaveInProgress={} m_pendingUnloadFinishes={}",
        m_unloadSaveInProgress.size(),
        m_pendingUnloadFinishes.size());
    std::vector<std::shared_ptr<mc::world::chunk::SingleChunkLifecycleManager>> lifecycleManagersCopy;
    {
        std::lock_guard<std::mutex> lock(m_lifecycleManagersMutex);
        spdlog::info("[stuck] total lifecycleManagers={}", m_lifecycleManagers.size());
        // 拷贝一份 shared_ptr 列表，避免持锁遍历时调用 SCLM 方法（其内部另有 m_mutex，且 isWaitingForNeighbors
        // 等可能触发断言）。shared_ptr 拷贝保证 holder 在锁外遍历期间存活。
        lifecycleManagersCopy.reserve(m_lifecycleManagers.size());
        for (const auto& [k, v] : m_lifecycleManagers) {
            MC_UNUSED(k);
            if (v) {
                lifecycleManagersCopy.push_back(v);
            }
        }
    }
    size_t stuckCount = 0;
    size_t waitingForNeighbors = 0;
    size_t hasGenTask = 0;
    size_t neighboursUsingCount = 0;
    size_t resolvingStorage = 0;
    size_t notReadyWithWaiters = 0;
    for (const auto& lifecycleManager : lifecycleManagersCopy) {
        if (!lifecycleManager) {
            continue;
        }
        // 转储所有"未完成请求目标"或"非安全卸载"的 holder，覆盖等待异步加载/等待邻居/未卸载等卡死态
        const bool wantsMore = lifecycleManager->getCurrentGenStatus().isBefore(lifecycleManager->requestedGenStatus());
        const bool stuck = !lifecycleManager->isSafeToUnload() || wantsMore;
        if (!stuck) {
            continue;
        }
        ++stuckCount;
        const bool isWaiting = lifecycleManager->isWaitingForNeighbors();
        const bool genTask = lifecycleManager->hasGenerationTask();
        const i32 neighboursUsing = lifecycleManager->neighboursUsingThisChunkCount();
        const auto source = lifecycleManager->sourceState();
        if (isWaiting) {
            ++waitingForNeighbors;
        }
        if (genTask) {
            ++hasGenTask;
        }
        if (neighboursUsing > 0) {
            ++neighboursUsingCount;
        }
        if (source == mc::world::chunk::SingleChunkLifecycleManager::SourceState::ResolvingStorage) {
            ++resolvingStorage;
        }
        if (wantsMore && !lifecycleManager->isSafeToUnload()) {
            ++notReadyWithWaiters;
        }
        // 仅打印前 30 个卡住的 holder，避免日志爆炸
        if (stuckCount <= 30) {
            spdlog::info("[stuck] ({}, {}) gen={} req={} genTask={} blocking={} waiting={} neighboursUsing={} "
                         "failed={} source={} shouldLoad={} level={}",
                lifecycleManager->x(),
                lifecycleManager->z(),
                lifecycleManager->getCurrentGenStatus().name(),
                lifecycleManager->requestedGenStatus().name(),
                genTask,
                lifecycleManager->blockingNeighbourCount(),
                isWaiting,
                neighboursUsing,
                lifecycleManager->hasFailedGeneration(),
                static_cast<int>(source),
                lifecycleManager->shouldLoad(),
                lifecycleManager->level());
        }
    }
    spdlog::info("[stuck] 总计卡住的 holder={}，其中 waitingForNeighbors={} hasGenTask={} neighboursUsing={} "
                 "resolvingStorage={} notReadyWithWaiters={}",
        stuckCount,
        waitingForNeighbors,
        hasGenTask,
        neighboursUsingCount,
        resolvingStorage,
        notReadyWithWaiters);

    // 额外诊断：按 sourceState 与 shouldLoad 分类全部 holder，定位泄漏/卡卸载的群体。
    // 同时转储处于 m_unloadSaveInProgress 的 holder（stage3 重试中）的 isSafeToUnload 各分量，
    // 揭示 _finalizeUnloadAfterSave 为何持续返回 false（重试）。
    size_t bySource[5] = {0, 0, 0, 0, 0}; // Unknown/ResolvingStorage/LoadedFromStorage/StorageMissing/Ready
    size_t shouldLoadTrue = 0;
    size_t safeToUnloadTrue = 0;
    size_t hasCurrentChunk = 0;
    std::unordered_set<u64> saveInProgressCopy;
    {
        std::lock_guard<std::mutex> lock(m_pendingUnloadFinishesMutex);
        saveInProgressCopy = m_unloadSaveInProgress;
    }
    size_t unloadingDumped = 0;
    size_t holdersWithWaiters = 0;
    size_t totalWaiters = 0;
    size_t waitersDumped = 0;
    for (const auto& lifecycleManager : lifecycleManagersCopy) {
        if (!lifecycleManager) {
            continue;
        }
        const auto src = lifecycleManager->sourceState();
        bySource[static_cast<size_t>(src)]++;
        if (lifecycleManager->shouldLoad()) {
            ++shouldLoadTrue;
        }
        if (lifecycleManager->isSafeToUnload()) {
            ++safeToUnloadTrue;
        }
        if (lifecycleManager->getCurrentChunk() != nullptr) {
            ++hasCurrentChunk;
        }
        const size_t wcount = lifecycleManager->waiterCount();
        if (wcount > 0) {
            ++holdersWithWaiters;
            totalWaiters += wcount;
            if (waitersDumped < 30) {
                ++waitersDumped;
                spdlog::info("[stuck-waiter] ({}, {}) waiters={} gen={} req={} genTask={} safe={} source={} "
                             "shouldLoad={} level={}",
                    lifecycleManager->x(),
                    lifecycleManager->z(),
                    wcount,
                    lifecycleManager->getCurrentGenStatus().name(),
                    lifecycleManager->requestedGenStatus().name(),
                    lifecycleManager->hasGenerationTask(),
                    lifecycleManager->isSafeToUnload(),
                    static_cast<int>(src),
                    lifecycleManager->shouldLoad(),
                    lifecycleManager->level());
            }
        }
        const u64 key = posToKey(lifecycleManager->x(), lifecycleManager->z());
        if (saveInProgressCopy.count(key) > 0 && unloadingDumped < 20) {
            ++unloadingDumped;
            spdlog::info("[stuck-unload] ({}, {}) safe={} genTask={} blocking={} waiting={} neighboursUsing={} "
                         "failed={} source={} shouldLoad={} level={}",
                lifecycleManager->x(),
                lifecycleManager->z(),
                lifecycleManager->isSafeToUnload(),
                lifecycleManager->hasGenerationTask(),
                lifecycleManager->blockingNeighbourCount(),
                lifecycleManager->isWaitingForNeighbors(),
                lifecycleManager->neighboursUsingThisChunkCount(),
                lifecycleManager->hasFailedGeneration(),
                static_cast<int>(src),
                lifecycleManager->shouldLoad(),
                lifecycleManager->level());
        }
    }
    spdlog::info("[stuck] source分布: Unknown={} ResolvingStorage={} StorageMissing={} LoadedFromStorage={} Ready={}",
        bySource[0],
        bySource[1],
        bySource[2],
        bySource[3],
        bySource[4]);
    spdlog::info("[stuck] shouldLoad=true={} isSafeToUnload=true={} hasCurrentChunk={}",
        shouldLoadTrue,
        safeToUnloadTrue,
        hasCurrentChunk);
    spdlog::info("[stuck] holdersWithWaiters={} totalWaiters={}", holdersWithWaiters, totalWaiters);
}

} // namespace mc::server
