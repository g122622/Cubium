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
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkPyramid.hpp"
#include "common/world/chunk/gen/ChunkStep.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include <algorithm>

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
    util::UniversalWorkerPool* parallelGenExecutor,
    util::UniversalWorkerPool* radiusAwareExecutor)
    : m_manager(manager)
    , m_world(world)
    , m_parallelGenExecutor(parallelGenExecutor)
    , m_radiusAwareExecutor(radiusAwareExecutor)
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

util::UniversalWorkerPool* ChunkTaskScheduler::selectExecutor(const ChunkStatus& status)
{
    const ChunkPyramid& pyramid = ChunkPyramid::generationPyramid();
    const ChunkStep& step = pyramid.getStepTo(status);
    // writeRadius <= 0 的状态不写方块状态（或只写中心），可完全并行；
    // writeRadius > 0 的状态（FEATURES=1, LIGHT=2）走区域互斥池。
    const i32 writeRadius = step.blockStateWriteRadius();
    if (writeRadius <= 0) {
        return m_parallelGenExecutor;
    }
    return m_radiusAwareExecutor;
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

ChunkProgressionTask* ChunkTaskScheduler::schedule(
    ChunkCoord x, ChunkCoord z, const ChunkStatus& targetStatus, SingleChunkLifecycleManager& holder)
{
    // 调用者必须持有覆盖 [x±maxAccessRadius, z±maxAccessRadius] 的区域锁。
    // 使用 maxAccessRadius（而非 getAccessRadius(targetStatus)）保证递归 schedule 邻居时
    // （邻居距离可达 maxAccessRadius，其锁区域可达 center±2*maxAccessRadius）被外层锁覆盖。
    const i32 accessRadius = getMaxAccessRadius();
    MC_ASSERT_RELEASE_MSG(m_schedulingLockArea.isHeldByCurrentThread(x, z, accessRadius),
        "ChunkTaskScheduler::schedule must be called holding the scheduling lock area");

    // 已完成到目标状态：无需推进
    if (holder.hasCompletedStatus(targetStatus)) {
        return nullptr;
    }

    // 取消守卫（安全网）：holder 的 abortSignal 已被 cancelActiveWork 置位（卸载/票据级别下降），
    // 说明该 holder 已被取消，不应再调度生成任务。返回 nullptr 不提交任务。
    //
    // 正常路径不会到达此处：
    //   - _scheduleGeneration（经 submitRequest）submitRequest 已分配新 false 令牌（abortSignal=false）。
    //   - checkNeighbour 遇到取消态邻居时先 reviveForScheduling 复活（分配新 false 令牌）再 schedule。
    //   - onChunkGenComplete/cancelGeneration/notifyWaitingNeighbours 的 rescheduleChunk 只重调度未取消的
    //     holder（完成者或被取消者的邻居，邻居未被取消）。
    //
    // 此守卫仅作为防御：若因竞态（submitRequest 复位令牌后、schedule 持锁前另一线程 cancelActiveWork
    // 置位）到达此处，返回 nullptr 安全跳过——该 holder 的 waiters 已被 _onTicketLevelChanged/
    // unloadChunkSync 的 _failWaiters(takeAllWaiters()) 失败通知，不会泄漏。
    if (auto sig = holder.abortSignal(); sig && sig->load(std::memory_order::acquire)) {
        return nullptr;
    }

    // 提升请求目标状态（对齐 Moonrise upgradeGenTarget）。
    // onChunkGenComplete 自推进依赖 requestedGenStatus 判断是否继续调度下一步，
    // 故 schedule 必须先提升目标，否则自推进会在低于 targetStatus 时提前停止。
    holder.upgradeGenTarget(targetStatus);

    // 已有进行中的生成任务：仅提升目标，不重复调度（对齐 Moonrise upgradeGenTarget）
    if (holder.hasGenerationTask()) {
        return nullptr;
    }

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
    if (holder.getCurrentChunk() == nullptr) {
        const auto sourceState = holder.sourceState();
        if (sourceState == mc::world::chunk::SingleChunkLifecycleManager::SourceState::Unknown) {
            // pristine holder：发起存档解析，挂起等待。不创建生成任务。
            m_manager._resolveStorageForScheduling(holder, targetStatus);
            return nullptr;
        }
        if (sourceState == mc::world::chunk::SingleChunkLifecycleManager::SourceState::ResolvingStorage) {
            // 异步存档读取在途：不创建任务，等待 _onChunkLoadComplete 完成路径重新调度。
            return nullptr;
        }
        // StorageMissing（存档已确认不存在）或 LoadedFromStorage/Ready（极端竞态 currentChunk 为空）：
        // 创建空 Primer（对齐 getEmptyChunk）。LoadedFromStorage/Ready 的 currentChunk 由 _onChunkLoadComplete
        // 设置，理论不应为空；若为空则当作存档缺失处理（创建空 Primer 后走生成），不会数据损坏。
        return scheduleEmptyLoad(x, z, holder);
    }

    // 推进一步：toStatus = currentGenStatus 的下一个状态
    const ChunkStatus& currentStatus = holder.getCurrentGenStatus();
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

    // 增加邻居引用计数，防止生成期间邻居被卸载
    for (i32 dz = -neighbourReadRadius; dz <= neighbourReadRadius; ++dz) {
        for (i32 dx = -neighbourReadRadius; dx <= neighbourReadRadius; ++dx) {
            const i32 distance = std::max(std::abs(dx), std::abs(dz));
            if (distance == 0) {
                continue;
            }
            const ChunkStatus* requiredStatus = step.getRequiredStatusAtRadius(distance);
            if (requiredStatus == nullptr) {
                continue;
            }
            if (SingleChunkLifecycleManager* neighbour = findHolder(x + dx, z + dz)) {
                neighbour->addNeighbourUsingChunk();
            }
        }
    }

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
    util::UniversalWorkerPool* pool = selectExecutor(status);
    const ChunkCoord centerX = x;
    const ChunkCoord centerZ = z;

    // 回调持有 holder 的 shared_ptr（与 ChunkProgressionTask::m_holder 一致），
    // 保证 execute 返回后的回调期间 holder 不被 unloadChunkSync 销毁。
    auto callback = [this, x, z, holder](bool success, util::ITask* rawTask) {
        if (holder == nullptr) {
            return;
        }
        // onChunkGenFailed 仅在「本任务真正失败（execute 返回 false 且未被取消）」时触发。
        // 用任务身份比对（generationTask()==rawTask）而非 hasGenerationTask()：
        //
        // 取消场景（abortSignal=true）：executeTask 已调用 onCancel→cancelGeneration(holder, this)。
        //   - 若本任务仍是当前任务（generationTask()==rawTask）：cancelGeneration 清理依赖图、
        //     释放邻居引用计数、clearGenerationTask。清理后 generationTask()==nullptr ≠ rawTask。
        //   - 若本任务已被取代（cancelActiveWork 清空 m_generationTask 后，新 submitRequest 调度了
        //     新任务 B 并 setGenerationTask(B)）：cancelGeneration(holder, this) 是 no-op
        //     （generationTask()==B ≠ this），m_generationTask 仍指向 B。此时 hasGenerationTask()=true
        //     但本任务已被取代，不应触发 onChunkGenFailed（否则会 markFailed 一个仍有活跃任务 B 的 holder，
        //     并 clearGenerationTask 清掉 B，导致 B 在已失败的 holder 上运行 → 依赖图永久阻塞 → 测试挂起）。
        //     用 generationTask()==rawTask 比对：B ≠ rawTask → no-op。
        //
        // 真正失败场景（execute 返回 false 且未被取消，如 primer 为空）：onCancel 未调用，
        // m_generationTask 仍指向本任务 → generationTask()==rawTask → 触发 onChunkGenFailed。
        //
        // 成功场景：execute 返回 true，execute 内部已调用 onChunkGenComplete→clearGenerationTask，
        // generationTask()==nullptr ≠ rawTask → no-op。
        //
        // 身份比对在 onChunkGenFailed(holder, task) 内部持调度锁进行，消除 execute 返回与回调之间的
        // TOCTOU 窗口（另一线程可能在回调前 setGenerationTask(B)）。
        if (!success) {
            onChunkGenFailed(*holder, static_cast<ChunkProgressionTask*>(rawTask));
        }
        // 成功时 onChunkGenComplete 已由 ChunkProgressionTask::execute 内部调用
    };

    if (pool == nullptr || !pool->isRunning()) {
        // 无 worker 池：在线执行任务。
        // 任务执行（execute）内部会调用 onChunkGenComplete，其重调度通过 rescheduleChunk 入队，
        // 由最外层 runInlineAndDrain 统一 drain。这避免 onChunkGenComplete 的重调度在
        // scheduleStatusStep 邻居扫描期间重入（重入会导致中心区块被过早重调度，产生指数级重复扫描）。
        std::atomic<bool> dummyCancel(false);
        runInlineAndDrain([&]() {
            const bool success = task->execute(dummyCancel);
            // task 的所有权在 lambda 内仍由外层 unique_ptr 持有，execute 不消费它。
            callback(success, task.get());
        });
        return;
    }

    pool->submit(std::move(task),
        std::move(callback),
        centerX,
        centerZ,
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

    // 已达到所需状态：就绪
    if (neighbour.hasCompletedStatus(requiredStatus)) {
        return false;
    }

    // 邻居生成失败：永久阻塞（对齐 Moonrise 失败即不可恢复）
    if (neighbour.hasFailedGeneration()) {
        return true;
    }

    // 复活取消态邻居：cancelActiveWork 置位 abortSignal 后不清空（保留 true），
    // 仅 submitRequest 分配新令牌。但 checkNeighbour→schedule 是邻居驱动的重新生成路径，
    // 不经 submitRequest。若不复活，schedule 的取消守卫返回 nullptr，建立的双向依赖
    // （下方 addBlockingNeighbour/addWaitingNeighbour）永不解除——邻居永不被驱动到 requiredStatus，
    // 中心 holder 永久阻塞（依赖图泄漏，测试表现为 holders 不降、pending 冻结）。
    // 复活分配新 false 令牌，使下方 schedule 正常提交任务，邻居完成后 onChunkGenComplete
    // 通知中心解除依赖。复活对齐 Moonrise 邻居按需生成语义（邻居无论 ticket 级别，被需要即生成）。
    // 安全性：调用者持有调度锁，cancelActiveWork 也持有同一把锁，故 abortSignal 稳定；
    // cancelActiveWork 已清空 m_generationTask，旧运行任务自取消（见 reviveForScheduling 注释）。
    neighbour.reviveForScheduling();

    // 建立双向依赖：center 等待 neighbour，neighbour 完成时通知 center
    center.addBlockingNeighbour(&neighbour);
    neighbour.addWaitingNeighbour(&center, &requiredStatus);

    // 若 neighbour 已有进行中任务，只提升目标
    if (neighbour.hasGenerationTask()) {
        neighbour.upgradeGenTarget(requiredStatus);
        return true;
    }

    // 若 neighbour 的 currentChunk 为空，先推进到 EMPTY（递归 schedule）
    // 否则递归推进 neighbour 到 requiredStatus
    schedule(x, z, requiredStatus, neighbour);
    return true;
}

// ============================================================================
// onChunkGenComplete：任务完成回调
// ============================================================================

void ChunkTaskScheduler::onChunkGenComplete(SingleChunkLifecycleManager& holder, const ChunkStatus& completedStatus)
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

    // 检测并发取消：cancelGeneration（unloadChunkSync/_onTicketLevelChanged/shutdown 路径）在任务执行期间
    // 调用时会 clearGenerationTask 并 releaseNeighbourRefCounts、清理依赖图。若 onChunkGenComplete 在
    // cancelGeneration 之后运行（持同一把调度锁，串行化），holder.generationTask() 已为 nullptr，
    // 表示取消路径已完成清理，onChunkGenComplete 不应重复释放邻居引用计数或重新调度（否则 double-free
    // 邻居引用计数、重调度已被取消的 holder）。
    // 仅推进 currentGenStatus（生成工作已完成，状态推进无害）并 clearGenerationTask（幂等）。
    const bool wasCancelled = (holder.generationTask() == nullptr);

    // 推进 currentGenStatus（Cubium ChunkPrimer 累积式，primer 同一对象）
    holder.onChunkGenComplete(completedStatus);
    holder.clearGenerationTask();

    if (wasCancelled) {
        // 取消路径（cancelGeneration）已清理依赖图、释放邻居引用计数、通知等待者失败。
        // onChunkGenComplete 仅推进 currentGenStatus（上面已完成），不重复清理、不重新调度。
        // _publishGeneratedChunk 跳过：取消路径已 _failWaiters，不应再唤醒等待者。
        return;
    }

    // 待重调度列表：持锁期间收集（notifyWaitingNeighbours 的就绪邻居 + 自推进），
    // 释放锁后统一 rescheduleChunk。对齐 Moonrise needsScheduling 列表 + post-unlock schedule，
    // 避免持锁期间嵌套获取邻居的 2*maxAccessRadius 锁（ReentrantAreaLock::lock 竞争的主要放大器）。
    // target 为 requestedGenStatus() 的指针（ChunkStatus 静态单例，释放锁后仍有效）。
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
    // 注：引用计数在 scheduleStatusStep 中增加，这里对应释放
    releaseNeighbourRefCounts(holder, completedStatus);

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
            pending.push_back({holder.x(), holder.z(), &target});
        }
    }

    // 达到请求目标状态时发布区块并唤醒等待者（非 FULL 路径；FULL 已由 _finalizeGeneratedChunkSync 处理）。
    // _publishGeneratedChunk 内部判断 completedStatus != FULL 且 hasCompletedStatus(requestedGenStatus) 才发布。
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
        if (item.target == nullptr) {
            continue;
        }
        rescheduleChunk(item.x, item.z, *item.target);
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
        if (item.target == nullptr) {
            continue;
        }
        rescheduleChunk(item.x, item.z, *item.target);
    }
}

void ChunkTaskScheduler::onChunkGenFailed(SingleChunkLifecycleManager& holder)
{
    const i32 lockRadius = 2 * getMaxAccessRadius();
    auto lock = m_schedulingLockArea.lock(holder.x(), holder.z(), lockRadius);

    // 对齐 Moonrise：失败即不可恢复
    holder.markFailed();
    holder.clearGenerationTask();

    // 通知所有等待者失败
    m_manager._failWaiters(holder.takeAllWaiters());
}

void ChunkTaskScheduler::onChunkGenFailed(SingleChunkLifecycleManager& holder, mc::server::ChunkProgressionTask* task)
{
    const i32 lockRadius = 2 * getMaxAccessRadius();
    auto lock = m_schedulingLockArea.lock(holder.x(), holder.z(), lockRadius);

    // 任务身份感知：仅当 holder.m_generationTask 仍指向 task 时才执行失败处理。
    // 消除 execute 返回与回调之间的 TOCTOU 窗口：另一线程可能在回调前 cancelActiveWork
    // 清空 m_generationTask 并 setGenerationTask(B)，此时本任务已被取代，不应 markFailed holder
    // （否则 B 在已失败的 holder 上运行，依赖图永久阻塞）。
    // - 真正失败（execute 返回 false 且未被取消）：onCancel 未调用，m_generationTask 仍指向 task → 执行。
    // - 取消且未被取代：onCancel→cancelGeneration(holder, task) 已 clearGenerationTask，
    //   m_generationTask==nullptr ≠ task → no-op。
    // - 取消且已被取代：cancelGeneration(holder, task) 是 no-op，m_generationTask==B ≠ task → no-op。
    // - 成功：execute 内部 onChunkGenComplete 已 clearGenerationTask，m_generationTask==nullptr ≠ task → no-op。
    if (holder.generationTask() != task) {
        return;
    }

    spdlog::warn("[ChunkTaskScheduler] onChunkGenFailed triggered for ({}, {}): genStatus={}, "
                 "scheduledStatus={}, hasGenerationTask=true",
        holder.x(),
        holder.z(),
        holder.getCurrentGenStatus().name(),
        holder.scheduledStatus() ? holder.scheduledStatus()->name() : "null");

    // 瞬态失败：EMPTY 任务在 sourceState==ResolvingStorage 时运行（TOCTOU）。
    // schedule 的 ResolvingStorage 守卫仅在调度时刻检查 sourceState，任务排队到 worker 执行
    // executeEmptyLoad 期间，主线程可能 submitRequest（玩家重新靠近）把 sourceState 从 Unknown
    // 推进到 ResolvingStorage，使 executeEmptyLoad 命中 ResolvingStorage 分支返回 false。
    // 这是"存档解析在途、不应生成"的瞬态条件，不是真正的生成失败：
    //   - 存档命中：_onChunkLoadComplete→markLoadedFromStorageReady(FULL)→onLoadedFromStorageReady
    //     （notifyWaitingNeighbours 解除依赖邻居阻塞）+ _completeReadyWaiters（fulfill 请求 promise）。
    //   - 存档缺失：_onChunkLoadComplete→noteStorageResolved(false)→_scheduleGeneration→schedule
    //     （sourceState=StorageMissing，非 ResolvingStorage）→scheduleEmptyLoad 成功→onChunkGenComplete
    //     →notifyWaitingNeighbours。
    // 两条路径都会推进 holder 状态并解除依赖邻居阻塞。故此处仅 clearGenerationTask（使 holder 可被
    // 存档完成路径重新调度），不 markFailed（保持可恢复），不 _failWaiters（保留 m_waiters 由存档完成
    // 路径 fulfill，而非让请求 future 收到 nullptr）。
    // 对齐 Moonrise：EMPTY 不真正失败（getEmptyChunk 总成功），失败模型只对非 EMPTY 的真正生成错误生效。
    if (holder.sourceState() == mc::world::chunk::SingleChunkLifecycleManager::SourceState::ResolvingStorage) {
        spdlog::info("[ChunkTaskScheduler] onChunkGenFailed transient (ResolvingStorage) for ({}, {}): "
                     "skipping markFailed, deferring to storage-resolve path",
            holder.x(),
            holder.z());
        holder.clearGenerationTask();
        return;
    }

    // 对齐 Moonrise：失败即不可恢复
    holder.markFailed();
    holder.clearGenerationTask();

    // 通知所有等待者失败
    m_manager._failWaiters(holder.takeAllWaiters());
}

void ChunkTaskScheduler::cancelGeneration(SingleChunkLifecycleManager& holder)
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Server.Chunk, "ChunkTaskScheduler::cancelGeneration", "x", holder.x(), "z", holder.z());

    const i32 lockRadius = 2 * getMaxAccessRadius();
    auto lock = m_schedulingLockArea.lock(holder.x(), holder.z(), lockRadius);

    // Plan A：锁内收集待重调度的邻居到 pending，释放锁后再 rescheduleChunk
    // （对齐 Moonrise needsScheduling + post-unlock schedule，避免持锁期间嵌套获取邻居的 2*maxAccessRadius 锁）。
    std::vector<PendingReschedule> pending;

    // 1. 捕获 scheduledStatus（clearGenerationTask 会清空它），用于补偿释放邻居引用计数。
    //    若 scheduledStatus 为空（任务未真正调度或已清理），跳过引用计数释放。
    const ChunkStatus* scheduledStatus = holder.scheduledStatus();

    // 2. 清除生成任务（abortSignal 已由 cancelActiveWork 设置，运行中的任务检测到后 onCancel 调用本方法）
    holder.clearGenerationTask();

    // 3. 补偿释放邻居引用计数：任务取消时 onChunkGenComplete 未运行，releaseNeighbourRefCounts 未执行。
    //    对齐 releaseNeighbourRefCounts，但基于 scheduledStatus。
    if (scheduledStatus != nullptr) {
        const ChunkPyramid& pyramid = ChunkPyramid::generationPyramid();
        const ChunkStep& step = pyramid.getStepTo(*scheduledStatus);
        const i32 neighbourReadRadius = step.neighbourReadRadius();
        const ChunkCoord x = holder.x();
        const ChunkCoord z = holder.z();
        for (i32 dz = -neighbourReadRadius; dz <= neighbourReadRadius; ++dz) {
            for (i32 dx = -neighbourReadRadius; dx <= neighbourReadRadius; ++dx) {
                const i32 distance = std::max(std::abs(dx), std::abs(dz));
                if (distance == 0) {
                    continue;
                }
                const ChunkStatus* requiredStatus = step.getRequiredStatusAtRadius(distance);
                if (requiredStatus == nullptr) {
                    continue;
                }
                if (SingleChunkLifecycleManager* neighbour = findHolder(x + dx, z + dz)) {
                    neighbour->removeNeighbourUsingChunk();
                }
            }
        }
    }

    // 4. 出向依赖：本 holder 等待的邻居（m_blockingNeighbours）。
    //    从每个邻居的 m_waitingNeighbours 移除本 holder，并清空本 holder 的 m_blockingNeighbours。
    //    邻居的 m_waitingNeighbours 移除本 holder 后，若邻居不再被任何 holder 等待，
    //    邻居的 isSafeToUnload 可能为 true（可卸载）。不在此重新调度出向邻居——
    //    它们原本就在等待本 holder，本 holder 取消后它们通过 _onTicketLevelChanged 重新评估。
    {
        const auto blocking = holder.blockingNeighbours();
        holder.clearBlockingNeighbours();
        for (SingleChunkLifecycleManager* neighbour : blocking) {
            if (neighbour != nullptr) {
                neighbour->removeWaitingNeighbour(&holder);
            }
        }
    }

    // 5. 入向依赖：等本 holder 的邻居（m_waitingNeighbours）。
    //    从每个邻居的 m_blockingNeighbours 移除本 holder。若邻居不再阻塞且有请求目标，
    //    重新调度邻居（邻居的 schedule→checkNeighbour 会通过 getOrCreateHolder 重建本 holder）。
    //    takeWaitingNeighbours 取出并清空本 holder 的 m_waitingNeighbours。
    //    关闭期间（isShuttingDown）不重新调度：所有 holder 的 abortSignal 已失效，
    //    重调度的任务无法被取消，会导致 waitForCompletion 无限循环。
    auto waiting = holder.takeWaitingNeighbours();
    for (auto& [neighbour, requiredStatus] : waiting) {
        if (neighbour == nullptr) {
            continue;
        }
        neighbour->removeBlockingNeighbour(&holder);
        // 邻居不再阻塞且无进行中任务且有请求目标：收集到 pending，释放锁后 rescheduleChunk（关闭期间跳过）。
        // isShuttingDown 在持锁时求值（与原行为一致）。target 为 requestedGenStatus()
        // 的指针（静态单例，释放锁后仍有效）。
        if (!isShuttingDown() && !neighbour->hasGenerationTask() && !neighbour->isWaitingForNeighbors()) {
            const ChunkStatus& target = neighbour->requestedGenStatus();
            if (neighbour->getCurrentGenStatus().isBefore(target)) {
                pending.push_back({neighbour->x(), neighbour->z(), &target});
            }
        }
    }

    // 6. 通知请求等待者失败（promise 完成 nullptr）
    m_manager._failWaiters(holder.takeAllWaiters());

    // Plan A：释放区域锁后再 rescheduleChunk，避免持锁期间嵌套获取邻居的 2*maxAccessRadius 锁。
    // rescheduleChunk 内部 findHolder 重新查找 holder（释放锁后 holder 可能被 unload——返回 nullptr 安全返回），
    // schedule 有 abortSignal/hasGenerationTask 守卫防 TOCTOU。
    lock.reset();
    for (const auto& item : pending) {
        if (item.target == nullptr) {
            continue;
        }
        rescheduleChunk(item.x, item.z, *item.target);
    }
}

void ChunkTaskScheduler::cancelGeneration(SingleChunkLifecycleManager& holder, mc::server::ChunkProgressionTask* task)
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Server.Chunk, "ChunkTaskScheduler::cancelGeneration(task)", "x", holder.x(), "z", holder.z());

    const i32 lockRadius = 2 * getMaxAccessRadius();
    auto lock = m_schedulingLockArea.lock(holder.x(), holder.z(), lockRadius);

    // 任务身份感知：仅当 m_generationTask 仍指向本任务时才清理。
    // 旧任务 A 的 onCancel 可能在新任务 B 已 setGenerationTask(B) 后才执行（cancelActiveWork 清空
    // m_generationTask 后 submitRequest 调度 B）。此时 A 的 onCancel 不应干扰 B 的依赖图/状态。
    // A 的邻居引用计数与依赖图条目已在 A 真正完成（onChunkGenComplete）或 holder 卸载
    // （cancelGeneration(holder) 无 task 参数版本）时清理，此处 no-op 是安全的。
    if (holder.generationTask() != task) {
        return;
    }

    // Plan A：锁内收集待重调度的邻居到 pending，释放锁后再 rescheduleChunk
    // （对齐 Moonrise needsScheduling + post-unlock schedule，避免持锁期间嵌套获取邻居的 2*maxAccessRadius 锁）。
    std::vector<PendingReschedule> pending;

    // m_generationTask 仍指向本任务：执行与 cancelGeneration(holder) 相同的清理。
    // 复用无 task 参数版本的逻辑：clearGenerationTask + releaseNeighbourRefCounts +
    // 依赖图清理 + _failWaiters。
    // 注意：不能直接调用 cancelGeneration(holder)（会重复加锁），此处持锁后委托内联逻辑。
    const ChunkStatus* scheduledStatus = holder.scheduledStatus();
    holder.clearGenerationTask();

    if (scheduledStatus != nullptr) {
        const ChunkPyramid& pyramid = ChunkPyramid::generationPyramid();
        const ChunkStep& step = pyramid.getStepTo(*scheduledStatus);
        const i32 neighbourReadRadius = step.neighbourReadRadius();
        const ChunkCoord x = holder.x();
        const ChunkCoord z = holder.z();
        for (i32 dz = -neighbourReadRadius; dz <= neighbourReadRadius; ++dz) {
            for (i32 dx = -neighbourReadRadius; dx <= neighbourReadRadius; ++dx) {
                const i32 distance = std::max(std::abs(dx), std::abs(dz));
                if (distance == 0) {
                    continue;
                }
                const ChunkStatus* requiredStatus = step.getRequiredStatusAtRadius(distance);
                if (requiredStatus == nullptr) {
                    continue;
                }
                if (SingleChunkLifecycleManager* neighbour = findHolder(x + dx, z + dz)) {
                    neighbour->removeNeighbourUsingChunk();
                }
            }
        }
    }

    {
        const auto blocking = holder.blockingNeighbours();
        holder.clearBlockingNeighbours();
        for (SingleChunkLifecycleManager* neighbour : blocking) {
            if (neighbour != nullptr) {
                neighbour->removeWaitingNeighbour(&holder);
            }
        }
    }

    auto waiting = holder.takeWaitingNeighbours();
    for (auto& [neighbour, requiredStatus] : waiting) {
        if (neighbour == nullptr) {
            continue;
        }
        neighbour->removeBlockingNeighbour(&holder);
        // 邻居不再阻塞且无进行中任务且有请求目标：收集到 pending，释放锁后 rescheduleChunk（关闭期间跳过）。
        // isShuttingDown 在持锁时求值（与原行为一致）。target 为 requestedGenStatus()
        // 的指针（静态单例，释放锁后仍有效）。
        if (!isShuttingDown() && !neighbour->hasGenerationTask() && !neighbour->isWaitingForNeighbors()) {
            const ChunkStatus& target = neighbour->requestedGenStatus();
            if (neighbour->getCurrentGenStatus().isBefore(target)) {
                pending.push_back({neighbour->x(), neighbour->z(), &target});
            }
        }
    }

    m_manager._failWaiters(holder.takeAllWaiters());

    // Plan A：释放区域锁后再 rescheduleChunk，避免持锁期间嵌套获取邻居的 2*maxAccessRadius 锁。
    // rescheduleChunk 内部 findHolder 重新查找 holder（释放锁后 holder 可能被 unload——返回 nullptr 安全返回），
    // schedule 有 abortSignal/hasGenerationTask 守卫防 TOCTOU。
    lock.reset();
    for (const auto& item : pending) {
        if (item.target == nullptr) {
            continue;
        }
        rescheduleChunk(item.x, item.z, *item.target);
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
        const ChunkStatus& target = neighbour->requestedGenStatus();
        pending.push_back({neighbour->x(), neighbour->z(), &target});
    }
}

void ChunkTaskScheduler::releaseNeighbourRefCounts(
    SingleChunkLifecycleManager& holder, const ChunkStatus& completedStatus)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk, "ChunkTaskScheduler::releaseNeighbourRefCounts");

    // 遍历该步的邻居读取半径范围，对每个使用的邻居释放引用计数
    // 与 scheduleStatusStep 中的 addNeighbourUsingChunk 一一对应
    const ChunkPyramid& pyramid = ChunkPyramid::generationPyramid();
    const ChunkStep& step = pyramid.getStepTo(completedStatus);
    const i32 neighbourReadRadius = step.neighbourReadRadius();
    const ChunkCoord x = holder.x();
    const ChunkCoord z = holder.z();

    for (i32 dz = -neighbourReadRadius; dz <= neighbourReadRadius; ++dz) {
        for (i32 dx = -neighbourReadRadius; dx <= neighbourReadRadius; ++dx) {
            const i32 distance = std::max(std::abs(dx), std::abs(dz));
            if (distance == 0) {
                continue; // 中心区块自身
            }
            const ChunkStatus* requiredStatus = step.getRequiredStatusAtRadius(distance);
            if (requiredStatus == nullptr) {
                continue;
            }
            if (SingleChunkLifecycleManager* neighbour = findHolder(x + dx, z + dz)) {
                neighbour->removeNeighbourUsingChunk();
            }
        }
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
        ChunkPrimer* primer = holder->getCurrentChunk();
        if (nx == x && nz == z) {
            // 中心区块：primer 由 schedule 调用前保证非空（currentChunk 为空走 scheduleEmptyLoad）
            MC_ASSERT_RELEASE_MSG(primer != nullptr, "buildNeighbourCache: center primer missing");
        } else {
            // 邻居：checkNeighbour 已确认达到 requiredStatus，getChunkIfPresentUnchecked 必非空
            const i32 dx = nx - x;
            const i32 dz = nz - z;
            const i32 distance = std::max(std::abs(dx), std::abs(dz));
            const ChunkStatus* requiredStatus = step.getRequiredStatusAtRadius(distance);
            MC_ASSERT_RELEASE_MSG(requiredStatus != nullptr, "buildNeighbourCache: requiredStatus is null");
            (void)holder->getChunkIfPresentUnchecked(*requiredStatus); // 状态就绪性断言（与旧逻辑一致）
            MC_ASSERT_RELEASE_MSG(primer != nullptr,
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

void ChunkTaskScheduler::rescheduleChunk(ChunkCoord x, ChunkCoord z, const ChunkStatus& target)
{
    SyncSchedulingContext& ctx = currentSyncContext();
    if (ctx.depth > 0) {
        // 处于在线执行模式：入队，由最外层 runInlineAndDrain 统一调度
        ctx.pending.push_back({x, z, &target});
        return;
    }
    // 异步模式（worker 池运行）或最外层调用：立即调度
    SingleChunkLifecycleManager* holder = findHolder(x, z);
    if (holder == nullptr) {
        return;
    }
    // 持有 2 * maxAccessRadius 锁，覆盖递归 schedule/checkNeighbour 的邻居范围
    const i32 lockRadius = 2 * getMaxAccessRadius();
    auto lock = m_schedulingLockArea.lock(x, z, lockRadius);
    schedule(x, z, target, *holder);
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
            SingleChunkLifecycleManager* holder = findHolder(item.x, item.z);
            if (holder == nullptr) {
                continue;
            }
            // 持有 2 * maxAccessRadius 锁，覆盖递归 schedule/checkNeighbour 的邻居范围
            const i32 lockRadius = 2 * getMaxAccessRadius();
            auto lock = m_schedulingLockArea.lock(item.x, item.z, lockRadius);
            schedule(item.x, item.z, *item.target, *holder);
        }
        --ctx.depth;
    }
}

} // namespace mc::server
