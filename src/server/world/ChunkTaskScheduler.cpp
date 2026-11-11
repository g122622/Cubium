/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING LIMITATION TO THE NOTICE BELOW.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING LIMITATION TO THE NOTICE BELOW.
 *
 * Github: https://github.com/guoyi22
 */

#include "ChunkTaskScheduler.hpp"
#include "ChunkProgressionTask.hpp"
#include "ServerChunkManager.hpp"
#include "ServerWorld.hpp"
#include "SingleChunkLifecycleManager.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/thread/ITask.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include "server/world/StaticChunkCache2D.hpp"
#include "server/world/chunk/pipeline/ChunkPyramid.hpp"
#include "server/world/chunk/pipeline/ChunkStatus.hpp"
#include "server/world/chunk/pipeline/ChunkStep.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/scheduling/AreaTaskScheduler.hpp"
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <memory>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::server {

using mc::world::chunk::ChunkPrimer;
using mc::world::chunk::ChunkPyramid;
using mc::world::chunk::ChunkStatus;
using mc::world::chunk::ChunkStep;
namespace ChunkStatuses = mc::world::chunk::ChunkStatuses;
using mc::world::chunk::SingleChunkLifecycleManager;

// ============================================================================
// 构造与访问半径
// ============================================================================

ChunkTaskScheduler::ChunkTaskScheduler(ServerChunkManager& manager,
    ServerWorld* world,
    util::UniversalWorkerPool* computeExecutor,
    AreaTaskScheduler& areaTasks)
    : m_manager(manager)
    , m_world(world)
    , m_computeExecutor(computeExecutor)
    , m_areaTasks(areaTasks)
    , m_schedulingLockArea(
          6) // coordinateShift=6：对齐 Moonrise getChunkSystemLockShift()=max(regionChunkShift,SECTION_SHIFT)=6，每
             // section 覆盖 64×64 区块。2*maxAccessRadius(22) 的锁从 shift=0 的 2025 sections 降到 1~4 sections，消除
             // ReentrantAreaLock::lock 获取争用热点。
{}

i32 ChunkTaskScheduler::getAccessRadius(const ChunkStatus& status)
{
    // 对齐 Moonrise：accessRadius = step.accumulatedRadius()
    const ChunkPyramid& pyramid = ChunkPyramid::generationPyramid();
    return pyramid.getStepTo(status).accumulatedRadius();
}

i32 ChunkTaskScheduler::getMaxAccessRadius()
{
    // 最大访问半径 = FULL 的 accumulatedRadius（当前为 11）
    return getAccessRadius(ChunkStatuses::FULL);
}

SingleChunkLifecycleManager& ChunkTaskScheduler::getOrCreateHolder(ChunkCoord x, ChunkCoord z)
{
    return m_manager._getOrCreateLifecycleManager(x, z);
}

SingleChunkLifecycleManager* ChunkTaskScheduler::findHolder(ChunkCoord x, ChunkCoord z)
{
    return m_manager._findLifecycleManager(x, z);
}

// ============================================================================
// schedule：把 (x, z) 推进到 targetStatus（一次一步）
// ============================================================================

ChunkProgressionTask* ChunkTaskScheduler::schedule(ChunkCoord x, ChunkCoord z, SingleChunkLifecycleManager& holder)
{
    // 调用者必须持有覆盖 [x±maxAccessRadius, z±maxAccessRadius] 的区域锁。
    // 使用 maxAccessRadius（而非 getAccessRadius(targetStatus)）保证递归 schedule 邻居时
    // （邻居距离可达 maxAccessRadius，其锁区域可达 center±2*maxAccessRadius）被外层锁覆盖。
    const i32 accessRadius = getMaxAccessRadius();
    MC_ASSERT_RELEASE_MSG(m_schedulingLockArea.isHeldByCurrentThread(x, z, accessRadius),
        "ChunkTaskScheduler::schedule must be called holding the scheduling lock area");

    // shared_ptr 只保证存活，不能保证仍属于当前世界；取指针后等待区域锁期间可能已卸载。
    // 必须在锁内核对身份，否则中心状态来自旧 holder，任务缓存却会取到同坐标的新 holder。
    if (m_manager._findLifecycleManagerShared(x, z).get() != &holder) {
        return nullptr;
    }

    // 一次性原子读出本函数用到的全部 holder 状态。分多次加锁读取（尤其"区块对象"与"存档来源"两次
    // 独立读取）会在两次读取之间跨越一次状态发布——publishStorageLoaded 在同一临界区内安装 primer
    // 并推进来源状态，先读到"区块为空"、后读到"来源已解析"的组合在真实状态中并不存在，据此判定
    // 会把已发布区块的 holder 误当作待加载区块。
    const SingleChunkLifecycleManager::GenerationSnapshot snapshot = holder.generationSnapshot();

    if (isShuttingDown() || snapshot.hasFailedGeneration || snapshot.requestedGenStatus == nullptr ||
        snapshot.hasGenerationTask) {
        return nullptr;
    }
    const ChunkStatus& targetStatus = *snapshot.requestedGenStatus;
    if (snapshot.currentChunk != nullptr && snapshot.currentGenStatus->isAtLeast(targetStatus)) {
        // 取消高阶段任务后，新请求可能只要求已经完成的低阶段；没有下一次任务完成
        // 可以唤醒这些请求，必须在这次重调度时交付。
        m_manager._completeReadyWaiters(holder);
        return nullptr;
    }
    // 重新进入只发生在旧任务已经完成后，不复位旧任务捕获的令牌。
    holder.reviveForScheduling();

    // currentChunk 为空：需要先加载 EMPTY（从存档加载或新建空 Primer）。
    // 对齐 Moonrise：pristine holder（currentGenStatus==null，对应 Cubium currentChunk 为空）必须经
    // ChunkLoadTask 先试磁盘读取，磁盘缺失/失败才回落到空 ProtoChunk（getEmptyChunk）。Moonrise 中
    // 二者是同一任务对象，结构上不可能并发。Cubium 把"磁盘读取"拆为异步存档解析（_resolveChunkSourceSync），
    // "空 Primer 创建"拆为 scheduleEmptyLoad/executeEmptyLoad，故必须用 sourceState 严格区分二者，
    // 杜绝对同一 holder 并发：
    //
    // - Unknown：holder 由 checkNeighbour 按需创建、存档来源尚未判定。绝不可直接 scheduleEmptyLoad——
    //   否则 EMPTY 任务运行期间主线程 submitRequest 把 sourceState 从 Unknown 推进到 ResolvingStorage，
    //   executeEmptyLoad 命中 ResolvingStorage 分支返回 false → onChunkGenFailed → markFailed → 永久阻塞
    //   （依赖图泄漏，checkNeighbour 把它当永久阻塞，所有依赖该区块的邻居级联失败）。
    //   正确做法：发起存档解析（_resolveStorageForScheduling），返回 nullptr 挂起。调用方（checkNeighbour）
    //   已建立双向依赖（addBlockingNeighbour/addWaitingNeighbour），存档解析完成后由现有管线重新驱动：
    //     命中→markLoadedFromStorageReady(FULL)+onLoadedFromStorageReady（解除依赖邻居阻塞）
    //     缺失→_scheduleGeneration→schedule（sourceState=StorageMissing）→scheduleEmptyLoad 创建空 Primer
    //           （对齐 getEmptyChunk）→onChunkGenComplete→notifyWaitingNeighbours
    // - ResolvingStorage：异步存档读取在途，不创建任务，返回 nullptr 等待存档解析完成路径重新调度。
    // - StorageMissing：存档已确认不存在，创建空 Primer（对齐 Moonrise getEmptyChunk）。
    //
    // LoadedFromStorage/Ready 不在此列：二者蕴含 holder 已持有区块（publishStorageLoaded 在同一临界区内
    // 安装 primer 并推进来源状态），故 currentChunk 为空时 sourceState 只可能是 StorageMissing。
    // 绝不可对 LoadedFromStorage/Ready 兜底创建空 Primer 任务——那会把该 holder 送进 executeEmptyLoad
    // 的"已加载"分支，在区块尚未安装时就把状态宣告为 FULL 就绪，依赖它的邻居随即在 buildNeighbourCache
    // 取到空 primer。
    if (snapshot.currentChunk == nullptr) {
        const auto sourceState = snapshot.sourceState;
        if (sourceState == mc::world::chunk::SingleChunkLifecycleManager::SourceState::Unknown) {
            // pristine holder：发起存档解析，挂起等待。不创建生成任务。
            m_manager._resolveStorageForScheduling(holder);
            return nullptr;
        }
        if (sourceState == mc::world::chunk::SingleChunkLifecycleManager::SourceState::ResolvingStorage) {
            // 异步存档读取在途：不创建任务，等待 _onChunkLoadComplete 完成路径重新调度。
            return nullptr;
        }
        MC_ASSERT_RELEASE_MSG(sourceState == mc::world::chunk::SingleChunkLifecycleManager::SourceState::StorageMissing,
            "ChunkTaskScheduler::schedule: a holder that resolved storage must already hold its chunk");
        return scheduleEmptyLoad(x, z, holder);
    }

    // 推进一步：toStatus = currentGenStatus 的下一个状态
    const ChunkStatus& currentStatus = *snapshot.currentGenStatus;
    const auto& allStatuses = ChunkStatus::getAll();
    const i32 nextOrdinal = currentStatus.ordinal() + 1;
    if (nextOrdinal >= static_cast<i32>(allStatuses.size())) {
        return nullptr; // 已是最高状态
    }
    const ChunkStatus& toStatus = allStatuses[static_cast<size_t>(nextOrdinal)];
    if (toStatus.ordinal() > targetStatus.ordinal()) {
        return nullptr; // 目标已达到（防御性）
    }

    return scheduleStatusStep(x, z, holder, toStatus);
}

ChunkProgressionTask* ChunkTaskScheduler::scheduleStatusStep(
    ChunkCoord x, ChunkCoord z, SingleChunkLifecycleManager& holder, const ChunkStatus& toStatus)
{
    const ChunkPyramid& pyramid = ChunkPyramid::generationPyramid();
    const ChunkStep& step = pyramid.getStepTo(toStatus);
    const i32 neighbourReadRadius = step.neighbourReadRadius();
    MC_ASSERT_RELEASE(neighbourReadRadius <= getMaxAccessRadius());

    // 遍历邻居读取半径范围，检查每个邻居是否达到 requiredStatus
    bool hasUnreadyNeighbours = false;
    for (i32 dz = -neighbourReadRadius; dz <= neighbourReadRadius; ++dz) {
        for (i32 dx = -neighbourReadRadius; dx <= neighbourReadRadius; ++dx) {
            const i32 distance = std::max(std::abs(dx), std::abs(dz));
            if (distance == 0) {
                continue; // 中心区块由 holder 保证
            }
            const ChunkStatus* requiredStatus = step.getRequiredStatusAtRadius(distance);
            if (requiredStatus == nullptr) {
                continue; // 该距离无依赖
            }
            if (checkNeighbour(x + dx, z + dz, *requiredStatus, holder)) {
                hasUnreadyNeighbours = true;
                if (holder.hasFailedGeneration()) {
                    return nullptr;
                }
            }
        }
    }

    if (hasUnreadyNeighbours) {
        // 有邻居未就绪：任务挂起，等邻居 onChunkGenComplete 重新调度
        return nullptr;
    }

    // 全部邻居就绪：构建邻居 holder 共享所有权缓存（StaticChunkCache2D<shared_ptr<SCLM>>）
    auto neighbours = buildNeighbourCache(x, z, step);
    const i32 writeRadius = step.blockStateWriteRadius();

    // 获取中心 holder 的共享所有权，交给 ChunkProgressionTask。
    // 任务在 worker 线程执行 execute（含 onChunkGenComplete）及回调期间持有此 shared_ptr，
    // 保证 holder 不被主线程 unloadChunkSync 销毁（消除 use-after-free 竞态）。
    // 调度阶段 holder 必然已存在（schedule 由 _scheduleGeneration 持锁调用，holder 已在
    // m_lifecycleManagers 中），用 _findLifecycleManagerShared 而非 _getOrCreateLifecycleManager。
    auto holderShared = m_manager._findLifecycleManagerShared(x, z);
    MC_ASSERT_RELEASE_MSG(holderShared != nullptr, "ChunkTaskScheduler::scheduleStatusStep: center holder missing");

    auto task = std::make_unique<ChunkProgressionTask>(
        *this, m_manager, holderShared, x, z, toStatus, std::move(neighbours), writeRadius);
    ChunkProgressionTask* taskPtr = task.get();

    holder.setGenerationTask(taskPtr, toStatus);

    // 选择执行器并提交（worker 池为空时在线执行）
    submitTask(std::move(task), toStatus, x, z, writeRadius, holder.abortSignal(), holderShared);

    return taskPtr;
}

// ============================================================================
// submitTask：提交任务到执行器（无 worker 池时在线执行）
// ============================================================================

void ChunkTaskScheduler::submitTask(std::unique_ptr<ChunkProgressionTask> task,
    const ChunkStatus& status,
    ChunkCoord x,
    ChunkCoord z,
    i32 writeRadius,
    std::shared_ptr<std::atomic<bool>> abortSignal,
    std::shared_ptr<SingleChunkLifecycleManager> holder)
{
    util::UniversalWorkerPool* pool = m_computeExecutor.load(std::memory_order::acquire);
    MC_UNUSED(status);
    const ChunkCoord centerX = x;
    const ChunkCoord centerZ = z;

    // 回调期间任务体仍存活，裸指针只用于锁内身份比对；回调持有 holder 保证其存活。
    auto callback = [this, holder](bool success, util::ITask* rawTask) {
        if (!success) {
            onChunkGenFailed(*holder, static_cast<ChunkProgressionTask*>(rawTask));
        }
    };

    if (pool == nullptr || !pool->isRunning()) {
        // 无 worker 池：在线执行任务。
        // 任务执行（execute）内部会调用 onChunkGenComplete，其重调度通过 rescheduleChunk 入队，
        // 由最外层 runInlineAndDrain 统一 drain。这避免 onChunkGenComplete 的重调度在
        // scheduleStatusStep 邻居扫描期间重入（重入会导致中心区块被过早重调度，产生指数级重复扫描）。
        static const std::atomic<bool> neverAbort{false};
        runInlineAndDrain([&]() {
            const auto& signal = abortSignal ? *abortSignal : neverAbort;
            const bool success = task->execute(signal);
            if (!success && signal.load(std::memory_order::acquire)) {
                task->onCancel();
            }
            // task 的所有权在 lambda 内仍由外层 unique_ptr 持有，execute 不消费它。
            callback(success, task.get());
        });
        return;
    }

    m_areaTasks.submit(std::move(task),
        std::move(callback),
        ChunkPos(centerX, centerZ),
        writeRadius,
        util::TaskPriority::Normal,
        std::move(abortSignal));
}

// ============================================================================
// checkNeighbour：验证邻居是否达到 requiredStatus
// ============================================================================

bool ChunkTaskScheduler::checkNeighbour(
    ChunkCoord x, ChunkCoord z, const ChunkStatus& requiredStatus, SingleChunkLifecycleManager& center)
{
    SingleChunkLifecycleManager& neighbour = getOrCreateHolder(x, z);

    // 从首次扫描就保活，不能等全部邻居就绪后才加引用：先完成的邻居若在等待
    // 其他邻居时被卸载，重扫会反复重建邻居，依赖集合永远无法同时就绪。
    if (center.pinGenerationNeighbour(neighbour)) {
        neighbour.addNeighbourUsingChunk();
    }

    // 已达到所需状态：就绪
    if (neighbour.hasCompletedStatus(requiredStatus)) {
        return false;
    }

    // 失败向上游传播，不能留下永远无法满足的依赖。
    if (neighbour.hasFailedGeneration()) {
        center.markFailed();
        auto owner = m_manager._findLifecycleManagerShared(center.x(), center.z());
        MC_ASSERT_RELEASE(owner.get() == &center);
        if (auto* pool = m_computeExecutor.load(std::memory_order::acquire); pool && pool->isRunning()) {
            pool->submit(std::make_unique<util::FunctionTask>(
                             util::TaskType::ChunkGenerate,
                             "Propagate chunk generation failure",
                             [this, owner](const std::atomic<bool>&) {
                                 onChunkGenFailed(*owner);
                                 return true;
                             },
                             "server.chunk"),
                {},
                util::TaskPriority::High,
                nullptr);
        } else {
            onChunkGenFailed(center);
        }
        return true;
    }

    // 建立双向依赖：center 等待 neighbour，neighbour 完成时通知 center
    center.addBlockingNeighbour(&neighbour);
    neighbour.addWaitingNeighbour(&center, &requiredStatus);

    // 目标已由依赖边汇总；旧任务完成前不能提交第二个任务。
    if (neighbour.hasGenerationTask()) {
        return true;
    }

    // 若 neighbour 的 currentChunk 为空，先推进到 EMPTY（递归 schedule）
    // 否则递归推进 neighbour 到 requiredStatus
    schedule(x, z, neighbour);
    return true;
}

// ============================================================================
// onChunkGenComplete：任务完成回调
// ============================================================================

void ChunkTaskScheduler::onChunkGenComplete(
    SingleChunkLifecycleManager& holder, const ChunkStatus& completedStatus, const ChunkProgressionTask& task)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk,
        "ChunkTaskScheduler::onChunkGenComplete",
        "x",
        holder.x(),
        "z",
        holder.z(),
        "completedStatus",
        completedStatus.name());

    // 持有 maxAccessRadius 的锁，覆盖依赖图操作（notifyWaitingNeighbours/releaseNeighbourRefCounts
    // 访问的邻居均在 maxAccessRadius 内）。
    //
    // 历史原因：曾用 2*maxAccessRadius 覆盖持锁期间递归 schedule→checkNeighbour→schedule 的邻居链
    // （最大触达 2*maxAccessRadius）。现 rescheduleChunk 移到释放锁后执行（对齐 Moonrise needsScheduling
    // 列表 + post-unlock schedule），持锁期间不再递归 schedule，maxAccessRadius 足够。
    // 该锁与 unloadChunkSync 互斥：worker 持锁期间修改依赖图，isSafeToUnload() 必为 false
    // （m_generationTask 在 clearGenerationTask 前非空，或 m_waitingNeighbours/m_blockingNeighbours 非空），
    // unloadChunkSync 不会在此窗口销毁 holder 或其邻居。
    const i32 lockRadius = getMaxAccessRadius();
    auto lock = m_schedulingLockArea.lock(holder.x(), holder.z(), lockRadius);

    // 活跃任务在执行器确认结束之前始终保留身份，卸载不能提前清除或替换它。
    MC_ASSERT_RELEASE(holder.generationTask() == &task);
    const ChunkStatus* scheduledStatus = holder.scheduledStatus();
    MC_ASSERT_RELEASE(scheduledStatus != nullptr);

    // 推进 currentGenStatus（Cubium ChunkPrimer 累积式，primer 同一对象）
    holder.onChunkGenComplete(completedStatus);
    holder.clearGenerationTask();

    // 待重调度列表：持锁期间收集（notifyWaitingNeighbours 的就绪邻居 + 自推进），
    // 释放锁后统一 rescheduleChunk。对齐 Moonrise needsScheduling 列表 + post-unlock schedule，
    // 避免持锁期间嵌套获取邻居的 2*maxAccessRadius 锁（ReentrantAreaLock::lock 竞争的主要放大器）。
    std::vector<PendingReschedule> pending;

    // 先通知等待者，再释放邻居引用计数。
    // 顺序至关重要：notifyWaitingNeighbours 通过 raw pointer 访问等待者 holder
    // （removeBlockingNeighbour/requestedGenStatus 等）。若先 releaseNeighbourRefCounts，
    // 邻居的 m_neighboursUsingThisChunk 归零 → isSafeToUnload() 可能为 true →
    // unloadChunkSync（持同一把调度锁等待）在 notifyWaitingNeighbours 完成前销毁邻居 holder，
    // 造成 use-after-free。先 notify（依赖图清理 + 收集 pending）再 release，保证通知期间
    // 邻居引用计数 > 0，邻居不可卸载。
    notifyWaitingNeighbours(holder, completedStatus, pending);

    // 释放邻居引用计数（中心区块不再阻塞邻居卸载）
    // 包括依赖扫描期间已就绪的邻居，均按实际登记集合释放。
    releaseNeighbourRefCounts(holder);

    // 自推进：若区块尚未达到请求的目标状态，继续调度推进下一步。
    // 对齐 Moonrise onChunkGenComplete 后重新 schedule 同一区块直至达到 requestedGenStatus。
    // schedule 会检查 hasGenerationTask（此时已 clear，无任务）并推进到 currentGenStatus.next()。
    // 若仍有邻居未就绪，schedule 会注册依赖并返回 nullptr（等待邻居 onChunkGenComplete 重新调度）。
    // 通过 rescheduleChunk 走延迟队列（同步模式下），避免在 scheduleStatusStep 邻居扫描期间重入。
    // 关闭期间（isShuttingDown）不自推进：所有 holder 已 cancelActiveWork，重调度的任务无法被取消，
    // 会导致 waitForCompletion 无限循环。
    // isShuttingDown() 在持锁时求值（与原行为一致），收集到 pending，释放锁后 rescheduleChunk。
    if (!isShuttingDown() && !holder.hasFailedGeneration() && !holder.hasGenerationTask()) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk, "ChunkTaskScheduler::onChunkGenComplete_rescheduleIfNeeded");
        const ChunkStatus& target = holder.requestedGenStatus();
        if (holder.getCurrentGenStatus().isBefore(target)) {
            pending.push_back({holder.x(), holder.z()});
        }
    }

    // 达到请求目标状态时发布区块并唤醒等待者（非 FULL 路径；FULL 已由 _finalizeGeneratedChunkSync 处理）。
    // 保持在锁内执行：读 holder.requestedGenStatus()，与 unloadChunkSync 串行，避免 holder 在发布期间被卸载。
    // 顺序在自推进收集之后、释放锁之前：自推进收集只读 requestedGenStatus，publish 唤醒等待者，
    // 两者无依赖（自推进是异步 submit，publish 是同步 fulfillWaiters），顺序调整无功能影响。
    if (!holder.hasFailedGeneration()) {
        m_manager._publishGeneratedChunk(holder, completedStatus);
    }

    // 释放区域锁后再 rescheduleChunk：每项 rescheduleChunk 自己获取 2*maxAccessRadius 锁
    // （覆盖其内部 schedule→checkNeighbour→schedule 递归的邻居链），不再嵌套在 onChunkGenComplete 的锁内。
    // rescheduleChunk 内部 findHolder 重新查找 holder（释放锁后 holder 可能被 unload——返回 nullptr 安全返回），
    // schedule 有 abortSignal/hasGenerationTask 守卫防 TOCTOU（释放锁后 holder 可能被 cancel/unload）。
    lock.reset();
    for (const auto& item : pending) {
        rescheduleChunk(item.x, item.z);
    }
}

void ChunkTaskScheduler::onLoadedFromStorageReady(SingleChunkLifecycleManager& holder)
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Server.Chunk, "ChunkTaskScheduler::onLoadedFromStorageReady", "x", holder.x(), "z", holder.z());

    // 持有 maxAccessRadius 的锁（与 onChunkGenComplete 一致）：notifyWaitingNeighbours 访问的
    // 等待邻居均在 maxAccessRadius 内（checkNeighbour 建立依赖时邻居就在该范围内）。
    const i32 lockRadius = getMaxAccessRadius();
    auto lock = m_schedulingLockArea.lock(holder.x(), holder.z(), lockRadius);

    if (m_manager._findLifecycleManagerShared(holder.x(), holder.z()).get() != &holder) {
        return;
    }

    // holder 已达 FULL（markLoadedFromStorageReady），通知等待该 holder 的依赖邻居解除阻塞，
    // 收集到 pending 释放锁后 rescheduleChunk 重新推进。对齐 onChunkGenComplete 的通知部分，
    // 但跳过生成任务相关清理（无 generationTask、无邻居引用计数补偿、不自推进、不发布）。
    std::vector<PendingReschedule> pending;
    notifyWaitingNeighbours(holder, ChunkStatuses::FULL, pending);

    // 释放区域锁后再 rescheduleChunk，避免持锁期间嵌套获取邻居的 2*maxAccessRadius 锁。
    // rescheduleChunk 内部 findHolder 重新查找邻居（释放锁后可能被 unload——返回 nullptr 安全返回），
    // schedule 有 abortSignal/hasGenerationTask 守卫防 TOCTOU。
    // 关闭期间（isShuttingDown）notifyWaitingNeighbours 已不收集 pending（见其内部 break），
    // 此处自然不重调度。
    lock.reset();
    for (const auto& item : pending) {
        rescheduleChunk(item.x, item.z);
    }
}

void ChunkTaskScheduler::onChunkGenFailed(SingleChunkLifecycleManager& holder)
{
    const i32 lockRadius = 2 * getMaxAccessRadius();
    auto lock = m_schedulingLockArea.lock(holder.x(), holder.z(), lockRadius);

    MC_ASSERT_RELEASE(!holder.hasGenerationTask());
    holder.markFailed();

    lock.reset();
    _releaseDependencyEdgesOnFailure(holder);
}

void ChunkTaskScheduler::_releaseDependencyEdgesOnFailure(SingleChunkLifecycleManager& holder)
{
    // 失败沿入向依赖传播，确保所有依赖已失败邻居的显式请求也完成为失败。
    // 逐持有者加锁，避免沿依赖链递归持锁扩大访问范围。
    std::vector<std::shared_ptr<SingleChunkLifecycleManager>> pending;
    auto root = m_manager._findLifecycleManagerShared(holder.x(), holder.z());
    if (root.get() != &holder) {
        return;
    }
    pending.push_back(std::move(root));
    while (!pending.empty()) {
        auto failed = std::move(pending.back());
        pending.pop_back();
        auto lock = m_schedulingLockArea.lock(failed->x(), failed->z(), 2 * getMaxAccessRadius());
        MC_ASSERT_RELEASE(!failed->hasGenerationTask());
        for (auto& [neighbour, requiredStatus] : clearAllDependencyEdges(*failed)) {
            MC_UNUSED(requiredStatus);
            if (!neighbour->hasFailedGeneration()) {
                auto waiter = m_manager._findLifecycleManagerShared(neighbour->x(), neighbour->z());
                MC_ASSERT_RELEASE(waiter.get() == neighbour);
                neighbour->markFailed();
                pending.push_back(std::move(waiter));
            }
        }
        m_manager._failWaiters(failed->takeAllWaiters());
    }
}

void ChunkTaskScheduler::onChunkGenFailed(SingleChunkLifecycleManager& holder, const ChunkProgressionTask* task)
{
    const i32 lockRadius = 2 * getMaxAccessRadius();
    auto lock = m_schedulingLockArea.lock(holder.x(), holder.z(), lockRadius);

    if (holder.generationTask() != task) {
        return;
    }
    // 取消可能发生在计算池决定调用失败回调之后；同样必须结清身份，不能静默忽略。
    const auto signal = holder.abortSignal();
    if (signal && signal->load(std::memory_order::acquire)) {
        lock.reset();
        cancelGeneration(holder, task);
        return;
    }

    spdlog::warn("[ChunkTaskScheduler] onChunkGenFailed triggered for ({}, {}): genStatus={}, "
                 "scheduledStatus={}, hasGenerationTask=true",
        holder.x(),
        holder.z(),
        holder.getCurrentGenStatus().name(),
        holder.scheduledStatus() ? holder.scheduledStatus()->name() : "null");

    // 真正失败也必须结清该步骤的邻居引用，不能只清除任务指针而把邻居永久钉住。
    const ChunkStatus* scheduledStatus = holder.scheduledStatus();
    MC_ASSERT_RELEASE(scheduledStatus != nullptr);
    releaseNeighbourRefCounts(holder);
    holder.markFailed();
    holder.clearGenerationTask();

    lock.reset();
    _releaseDependencyEdgesOnFailure(holder);
}

std::vector<std::pair<SingleChunkLifecycleManager*, const ChunkStatus*>> ChunkTaskScheduler::clearAllDependencyEdges(
    SingleChunkLifecycleManager& holder)
{
    // 出向：本 holder 正在等待的邻居。从每个邻居的等待表中摘除本 holder，
    // 使邻居在不再被任何 holder 等待时可以卸载。
    {
        const auto blocking = holder.blockingNeighbours();
        holder.clearBlockingNeighbours();
        for (SingleChunkLifecycleManager* neighbour : blocking) {
            if (neighbour != nullptr) {
                neighbour->removeWaitingNeighbour(&holder);
            }
        }
    }

    // 入向：正在等待本 holder 的邻居。从每个等待者的阻塞表中摘除本 holder，
    // 否则它们会永久阻塞在一条永远不会被满足的依赖边上，导致其 isSafeToUnload() 恒为 false。
    auto waiting = holder.takeWaitingNeighbours();
    for (auto& [neighbour, requiredStatus] : waiting) {
        if (neighbour != nullptr) {
            neighbour->removeBlockingNeighbour(&holder);
            continue;
        }
        // 空指针条目不应存在于依赖图中：添加点（checkNeighbour）始终传入有效 holder。
        MC_ASSERT_RELEASE_MSG(false, "ChunkTaskScheduler: null neighbour in waiting-neighbour map");
    }
    releaseNeighbourRefCounts(holder);
    return waiting;
}

void ChunkTaskScheduler::cancelGeneration(SingleChunkLifecycleManager& holder)
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Server.Chunk, "ChunkTaskScheduler::cancelGeneration", "x", holder.x(), "z", holder.z());

    const i32 lockRadius = 2 * getMaxAccessRadius();
    auto lock = m_schedulingLockArea.lock(holder.x(), holder.z(), lockRadius);

    // 锁内只收集受影响邻居，释放后按最新需求重新调度。
    std::vector<PendingReschedule> pending;

    // 无任务身份的清理只允许在执行器已经排空，或 holder 已安全可卸载时调用。
    // 活跃任务只能由带身份的完成/取消回调释放，不能在这里提前释放邻居引用。
    MC_ASSERT_RELEASE(!holder.hasGenerationTask());

    // 双向摘除依赖边，释放依赖扫描期间登记的全部邻居保活。
    //    出向邻居不在此重新调度——它们原本就在等待本 holder，本 holder 取消后通过
    //    _onTicketLevelChanged 重新评估。入向等待者若不再阻塞且有请求目标，收集到 pending，
    //    释放锁后 rescheduleChunk（邻居的 schedule→checkNeighbour 会通过 getOrCreateHolder 重建本 holder）。
    //    关闭期间（isShuttingDown）不重新调度：所有 holder 的 abortSignal 已失效，
    //    重调度的任务无法被取消，会导致 waitForCompletion 无限循环。
    for (auto& [neighbour, requiredStatus] : clearAllDependencyEdges(holder)) {
        if (neighbour == nullptr) {
            continue;
        }
        // 邻居不再阻塞且无进行中任务且有请求目标：收集到 pending，释放锁后 rescheduleChunk（关闭期间跳过）。
        // isShuttingDown 在持锁时求值（与原行为一致）。target 为 requestedGenStatus()
        // 的指针（静态单例，释放锁后仍有效）。
        if (!isShuttingDown() && !neighbour->hasGenerationTask() && !neighbour->isWaitingForNeighbors()) {
            const ChunkStatus& target = neighbour->requestedGenStatus();
            if (neighbour->getCurrentGenStatus().isBefore(target)) {
                pending.push_back({neighbour->x(), neighbour->z()});
            }
        }
    }

    // 通知尚未完成的请求失败。
    m_manager._failWaiters(holder.takeAllWaiters());

    // 释放区域锁后再重新调度邻居，避免嵌套扩大锁范围。
    // rescheduleChunk 内部 findHolder 重新查找 holder（释放锁后 holder 可能被 unload——返回 nullptr 安全返回），
    // schedule 有 abortSignal/hasGenerationTask 守卫防 TOCTOU。
    lock.reset();
    for (const auto& item : pending) {
        rescheduleChunk(item.x, item.z);
    }
}

void ChunkTaskScheduler::cancelGeneration(SingleChunkLifecycleManager& holder, const ChunkProgressionTask* task)
{
    auto lock = m_schedulingLockArea.lock(holder.x(), holder.z(), 2 * getMaxAccessRadius());
    if (holder.generationTask() != task) {
        return;
    }
    const ChunkStatus* scheduledStatus = holder.scheduledStatus();
    MC_ASSERT_RELEASE(scheduledStatus != nullptr);
    // 执行器已确认任务不再访问邻居，此时才释放引用并清除任务身份。
    releaseNeighbourRefCounts(holder);
    holder.clearGenerationTask();
    lock.reset();
    if (!isShuttingDown()) {
        reconcileGenerationDemand(holder.x(), holder.z());
        rescheduleChunk(holder.x(), holder.z());
    }
}

void ChunkTaskScheduler::reconcileGenerationDemand(ChunkCoord x, ChunkCoord z)
{
    // 逐持有者取得区域锁，避免沿依赖链递归持锁，锁范围随路径无限扩大。
    std::vector<PendingReschedule> pending{{x, z}};
    while (!pending.empty()) {
        const auto item = pending.back();
        pending.pop_back();
        auto holder = m_manager._findLifecycleManagerShared(item.x, item.z);
        if (holder == nullptr) {
            continue;
        }
        auto lock = m_schedulingLockArea.lock(item.x, item.z, 2 * getMaxAccessRadius());
        if (m_manager._findLifecycleManagerShared(item.x, item.z) != holder) {
            continue;
        }
        const auto snapshot = holder->generationSnapshot();
        const bool reachedTarget = snapshot.requestedGenStatus == nullptr ||
            (snapshot.currentChunk != nullptr && snapshot.currentGenStatus->isAtLeast(*snapshot.requestedGenStatus));
        if (auto* task = holder->generationTask()) {
            if (reachedTarget && !task->hasStarted()) {
                holder->signalGenerationCancellation();
            }
            // 已开始的阶段完整执行，引用计数和任务身份由完成回调结清。
            continue;
        }
        if (!reachedTarget) {
            continue;
        }
        for (auto* neighbour : holder->blockingNeighbours()) {
            pending.push_back({neighbour->x(), neighbour->z()});
            neighbour->removeWaitingNeighbour(holder.get());
        }
        holder->clearBlockingNeighbours();
        releaseNeighbourRefCounts(*holder);
    }
}

// ============================================================================
// notifyWaitingNeighbours：通知等待该区块的邻居重新调度
// ============================================================================

void ChunkTaskScheduler::notifyWaitingNeighbours(
    SingleChunkLifecycleManager& holder, const ChunkStatus& completedStatus, std::vector<PendingReschedule>& pending)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk, "ChunkTaskScheduler::notifyWaitingNeighbours");

    // 取出等待者快照（持锁下操作，避免迭代时修改）
    const auto waiting = holder.waitingNeighbours();
    if (waiting.empty()) {
        return;
    }

    // 第一遍：解除阻塞关系，收集可重新调度的邻居
    std::vector<SingleChunkLifecycleManager*> readyToSchedule;
    for (auto& [neighbour, requiredStatus] : waiting) {
        if (requiredStatus == nullptr || completedStatus.isAtLeast(*requiredStatus)) {
            // holder 已达到 neighbour 所需状态，解除 neighbour 对 holder 的阻塞
            neighbour->removeBlockingNeighbour(&holder);
            // 若 neighbour 不再等待任何邻居且有请求目标，则可重新调度
            if (!neighbour->isWaitingForNeighbors()) {
                readyToSchedule.push_back(neighbour);
            }
        }
    }

    // 清除 holder 中已满足的等待者记录
    holder.clearSatisfiedWaitingNeighbours(completedStatus);

    // 第二遍：收集待重调度的邻居到 pending（不立即 rescheduleChunk）。
    // 调用方（onChunkGenComplete）在释放区域锁后统一 rescheduleChunk，避免持锁期间嵌套获取
    // 邻居的 2*maxAccessRadius 锁（ReentrantAreaLock::lock 竞争的主要放大器）。
    // 通过 rescheduleChunk 走延迟队列（同步模式下），避免在 scheduleStatusStep 邻居扫描尚未完成时
    // 重入会过早重调度中心区块。
    // 关闭期间（isShuttingDown）不收集：避免无限循环。isShuttingDown() 在持锁时求值（与原行为一致）。
    for (SingleChunkLifecycleManager* neighbour : readyToSchedule) {
        if (isShuttingDown()) {
            break;
        }
        pending.push_back({neighbour->x(), neighbour->z()});
    }
}

void ChunkTaskScheduler::releaseNeighbourRefCounts(SingleChunkLifecycleManager& holder)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk, "ChunkTaskScheduler::releaseNeighbourRefCounts");

    for (auto* neighbour : holder.takeGenerationNeighbours()) {
        neighbour->removeNeighbourUsingChunk();
    }
}

// ============================================================================
// buildNeighbourCache：构建邻居可变 ChunkPrimer 缓存
// ============================================================================

StaticChunkCache2D<std::shared_ptr<SingleChunkLifecycleManager>> ChunkTaskScheduler::buildNeighbourCache(
    ChunkCoord x, ChunkCoord z, const ChunkStep& step)
{
    const i32 radius = step.neighbourReadRadius();

    // 加载器：返回各 holder 的 shared_ptr（对齐 Moonrise StaticCache2D<GenerationChunkHolder> 强引用语义）。
    // worker 线程在 executeStatusStep 期间持此 shared_ptr，保证邻居 holder 及其 m_currentChunk（ChunkPrimer）
    // 不被主线程 unloadChunkSync 销毁。调用前已由 checkNeighbour 确认所有邻居就绪，holder 必然存在。
    auto loader = [this, x, z, &step](ChunkCoord nx, ChunkCoord nz) -> std::shared_ptr<SingleChunkLifecycleManager> {
        // 中心与邻居都通过 _findLifecycleManagerShared 取 shared_ptr（与 scheduleStatusStep 的 holderShared 一致）。
        std::shared_ptr<SingleChunkLifecycleManager> holder = m_manager._findLifecycleManagerShared(nx, nz);
        MC_ASSERT_RELEASE_MSG(holder != nullptr, "buildNeighbourCache: holder missing");
        if (nx == x && nz == z) {
            // 中心区块：primer 由 schedule 调用前保证非空（currentChunk 为空走 scheduleEmptyLoad）
            MC_ASSERT_RELEASE_MSG(holder->getCurrentChunk() != nullptr, "buildNeighbourCache: center primer missing");
        } else {
            // 邻居可用性判据必须是"状态 ∧ 对象"的合取：getChunkIfPresentUnchecked 同时校验
            // currentGenStatus 达标与 primer 非空。仅比对状态会漏掉"状态已宣告就绪但区块尚未安装"
            // 的 holder，其空 primer 会直接进入 WorldGenRegion。
            const i32 distance = std::max(std::abs(nx - x), std::abs(nz - z));
            const ChunkStatus* requiredStatus = step.getRequiredStatusAtRadius(distance);
            MC_ASSERT_RELEASE_MSG(requiredStatus != nullptr, "buildNeighbourCache: requiredStatus is null");
            MC_ASSERT_RELEASE_MSG(holder->getChunkIfPresentUnchecked(*requiredStatus) != nullptr,
                "buildNeighbourCache: neighbour primer missing (checkNeighbour should have verified readiness)");
        }
        return holder;
    };

    return StaticChunkCache2D<std::shared_ptr<SingleChunkLifecycleManager>>(x, z, radius, loader);
}

// ============================================================================
// scheduleEmptyLoad：EMPTY 加载任务（从存档或新建空 Primer）
// ============================================================================

ChunkProgressionTask* ChunkTaskScheduler::scheduleEmptyLoad(
    ChunkCoord x, ChunkCoord z, SingleChunkLifecycleManager& holder)
{
    const ChunkPyramid& pyramid = ChunkPyramid::generationPyramid();
    const ChunkStep& emptyStep = pyramid.getStepTo(ChunkStatuses::EMPTY);

    // EMPTY 推进不需要邻居缓存（neighbourReadRadius=0），构建仅含中心的缓存。
    // 同样存 shared_ptr<holder>（与 scheduleStatusStep 的缓存类型一致），中心 primer 可能为空
    // （由 ChunkProgressionTask::executeEmptyLoad 创建），holder 必然存在。
    auto loader = [this, x, z](ChunkCoord nx, ChunkCoord nz) -> std::shared_ptr<SingleChunkLifecycleManager> {
        MC_ASSERT_RELEASE(nx == x && nz == z);
        std::shared_ptr<SingleChunkLifecycleManager> h = m_manager._findLifecycleManagerShared(nx, nz);
        MC_ASSERT_RELEASE_MSG(h != nullptr, "scheduleEmptyLoad: center holder missing");
        return h;
    };

    StaticChunkCache2D<std::shared_ptr<SingleChunkLifecycleManager>> neighbours(x, z, 0, loader);

    // 获取中心 holder 的共享所有权（与 scheduleStatusStep 一致，防 use-after-free）
    auto holderShared = m_manager._findLifecycleManagerShared(x, z);
    MC_ASSERT_RELEASE_MSG(holderShared != nullptr, "ChunkTaskScheduler::scheduleEmptyLoad: center holder missing");

    auto task = std::make_unique<ChunkProgressionTask>(*this,
        m_manager,
        holderShared,
        x,
        z,
        ChunkStatuses::EMPTY,
        std::move(neighbours),
        emptyStep.blockStateWriteRadius());
    ChunkProgressionTask* taskPtr = task.get();

    holder.setGenerationTask(taskPtr, ChunkStatuses::EMPTY);

    // 选择执行器并提交（worker 池为空时在线执行）
    submitTask(std::move(task),
        ChunkStatuses::EMPTY,
        x,
        z,
        emptyStep.blockStateWriteRadius(),
        holder.abortSignal(),
        holderShared);

    return taskPtr;
}

// ============================================================================
// 同步执行上下文：延迟重调度（避免在线执行模式的重入）
// ============================================================================

ChunkTaskScheduler::SyncSchedulingContext& ChunkTaskScheduler::currentSyncContext()
{
    thread_local SyncSchedulingContext ctx;
    return ctx;
}

void ChunkTaskScheduler::resetThreadLocalContext()
{
    auto& ctx = currentSyncContext();
    ctx.depth = 0;
    ctx.pending.clear();
}

void ChunkTaskScheduler::rescheduleChunk(ChunkCoord x, ChunkCoord z)
{
    SyncSchedulingContext& ctx = currentSyncContext();
    if (ctx.depth > 0) {
        // 处于在线执行模式：入队，由最外层 runInlineAndDrain 统一调度
        ctx.pending.push_back({x, z});
        return;
    }
    // 异步模式（worker 池运行）或最外层调用：立即调度。
    // 必须取 shared_ptr 而非裸指针：findHolder 返回的裸指针在取到之后、加锁之前，
    // 主线程的 unloadChunkSync 可能已把该 holder 从 m_lifecycleManagers 移除并析构，
    // 随后 schedule(x, z, target, *holder) 解引用即 use-after-free（实测报错栈：
    // generationSnapshot ← schedule ← rescheduleChunk ← onChunkGenComplete ← worker 线程）。
    // shared_ptr 保证 holder 在本次调度期间存活。
    std::shared_ptr<SingleChunkLifecycleManager> holder = m_manager._findLifecycleManagerShared(x, z);
    if (holder == nullptr) {
        return;
    }
    // 持有 2 * maxAccessRadius 锁，覆盖递归 schedule/checkNeighbour 的邻居范围
    const i32 lockRadius = 2 * getMaxAccessRadius();
    auto lock = m_schedulingLockArea.lock(x, z, lockRadius);
    schedule(x, z, *holder);
}

void ChunkTaskScheduler::runInlineAndDrain(std::function<void()>&& callable)
{
    SyncSchedulingContext& ctx = currentSyncContext();
    ++ctx.depth;
    callable();
    --ctx.depth;

    // 最外层（depth 归零）：drain 延迟重调度队列。
    // drain 期间产生的 rescheduleChunk 仍会入队（depth 已归零但 pending 非空时
    // rescheduleChunk 会立即调度——此时调度产生的 onChunkGenComplete 仍在线程栈上，
    // 会再次入队）。为避免重入，drain 期间临时把 depth 视为 >0：每个 schedule 在
    // 持锁下执行，其 onChunkGenComplete 的 rescheduleChunk 入队而非立即调度。
    if (ctx.depth != 0) {
        return;
    }

    // drain 期间需要让 rescheduleChunk 入队（而非立即调度），故保持 depth=1 的语义。
    // 用一个标志位区分"drain 中"与"正常在线执行"。
    while (!ctx.pending.empty()) {
        auto batch = std::move(ctx.pending);
        ctx.pending.clear();
        // drain 中的 schedule 调用会触发 onChunkGenComplete → rescheduleChunk，
        // 此时 depth==0 会立即调度而非入队。为保证 drain 产生的重调度也入队迭代处理，
        // 临时提升 depth。
        ++ctx.depth;
        for (const auto& item : batch) {
            // 与上方 rescheduleChunk 同理：必须持 shared_ptr 跨越"查找 → 加锁"这段窗口。
            // 若用裸指针，其它线程的 unloadChunkSync 可能在此期间把 holder 从
            // m_lifecycleManagers 移除并析构，随后 schedule(..., *holder) 即 use-after-free。
            std::shared_ptr<SingleChunkLifecycleManager> holder = m_manager._findLifecycleManagerShared(item.x, item.z);
            if (holder == nullptr) {
                continue;
            }
            // 持有 2 * maxAccessRadius 锁，覆盖递归 schedule/checkNeighbour 的邻居范围
            const i32 lockRadius = 2 * getMaxAccessRadius();
            auto lock = m_schedulingLockArea.lock(item.x, item.z, lockRadius);
            schedule(item.x, item.z, *holder);
        }
        --ctx.depth;
    }
}

} // namespace mc::server
