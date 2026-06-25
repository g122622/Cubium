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
#include "common/util/assert/AssertAll.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkPyramid.hpp"
#include "common/world/chunk/gen/ChunkStep.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

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
    util::ServerWorkerPool* parallelGenExecutor,
    util::ServerWorkerPool* radiusAwareExecutor)
    : m_manager(manager)
    , m_world(world)
    , m_parallelGenExecutor(parallelGenExecutor)
    , m_radiusAwareExecutor(radiusAwareExecutor)
    , m_schedulingLockArea(0) // coordinateShift=0：一个区块一个锁条目（区块级粒度）
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

util::ServerWorkerPool* ChunkTaskScheduler::selectExecutor(const ChunkStatus& status)
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

    // 提升请求目标状态（对齐 Moonrise upgradeGenTarget）。
    // onChunkGenComplete 自推进依赖 requestedGenStatus 判断是否继续调度下一步，
    // 故 schedule 必须先提升目标，否则自推进会在低于 targetStatus 时提前停止。
    holder.upgradeGenTarget(targetStatus);

    // 已有进行中的生成任务：仅提升目标，不重复调度（对齐 Moonrise upgradeGenTarget）
    if (holder.hasGenerationTask()) {
        return nullptr;
    }

    // currentChunk 为空：需要先加载 EMPTY（从存档或新建空 Primer）
    if (holder.getCurrentChunk() == nullptr) {
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

    // 全部邻居就绪：构建邻居缓存（StaticChunkCache2D<ChunkPrimer*>）
    StaticChunkCache2D<ChunkPrimer*> neighbours = buildNeighbourCache(x, z, step);
    const i32 writeRadius = step.blockStateWriteRadius();

    auto task =
        std::make_unique<ChunkProgressionTask>(*this, m_manager, x, z, toStatus, std::move(neighbours), writeRadius);
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
    submitTask(std::move(task), toStatus, x, z, writeRadius, holder.cancelToken());

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
    std::shared_ptr<std::atomic<bool>> cancelToken)
{
    util::ServerWorkerPool* pool = selectExecutor(status);
    const ChunkCoord centerX = x;
    const ChunkCoord centerZ = z;

    auto callback = [this, x, z](bool success, util::ITask* rawTask) {
        MC_UNUSED(rawTask);
        SingleChunkLifecycleManager* holder = findHolder(x, z);
        if (holder == nullptr) {
            return;
        }
        // 任务执行失败时（execute 返回 false 或抛异常），标记失败并通知等待者
        if (!success && !holder->hasCompletedStatus(ChunkStatuses::FULL)) {
            onChunkGenFailed(*holder);
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
        std::move(cancelToken));
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
    // 持有 2 * maxAccessRadius 的锁，覆盖邻居的邻居
    const i32 lockRadius = 2 * getMaxAccessRadius();
    auto lock = m_schedulingLockArea.lock(holder.x(), holder.z(), lockRadius);

    // 推进 currentGenStatus（Cubium ChunkPrimer 累积式，primer 同一对象）
    holder.onChunkGenComplete(completedStatus);
    holder.clearGenerationTask();

    // 释放邻居引用计数（中心区块不再阻塞邻居卸载）
    // 注：引用计数在 scheduleStatusStep 中增加，这里对应释放
    releaseNeighbourRefCounts(holder, completedStatus);

    notifyWaitingNeighbours(holder, completedStatus);

    // 自推进：若区块尚未达到请求的目标状态，继续调度推进下一步。
    // 对齐 Moonrise onChunkGenComplete 后重新 schedule 同一区块直至达到 requestedGenStatus。
    // schedule 会检查 hasGenerationTask（此时已 clear，无任务）并推进到 currentGenStatus.next()。
    // 若仍有邻居未就绪，schedule 会注册依赖并返回 nullptr（等待邻居 onChunkGenComplete 重新调度）。
    // 通过 rescheduleChunk 走延迟队列（同步模式下），避免在 scheduleStatusStep 邻居扫描期间重入。
    if (!holder.hasFailedGeneration() && !holder.hasGenerationTask()) {
        const ChunkStatus& target = holder.requestedGenStatus();
        if (holder.getCurrentGenStatus().isBefore(target)) {
            rescheduleChunk(holder.x(), holder.z(), target);
        }
    }

    // 达到请求目标状态时发布区块并唤醒等待者（非 FULL 路径；FULL 已由 _finalizeGeneratedChunkSync 处理）。
    // _publishGeneratedChunk 内部判断 completedStatus != FULL 且 hasCompletedStatus(requestedGenStatus) 才发布。
    // 必须在自推进之后调用：自推进可能继续调度更高状态，但若当前已完成到 requestedGenStatus，
    // 等待者（requestChunkSync 的 promise）应被唤醒。
    if (!holder.hasFailedGeneration()) {
        m_manager._publishGeneratedChunk(holder, completedStatus);
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

// ============================================================================
// notifyWaitingNeighbours：通知等待该区块的邻居重新调度
// ============================================================================

void ChunkTaskScheduler::notifyWaitingNeighbours(
    SingleChunkLifecycleManager& holder, const ChunkStatus& completedStatus)
{
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

    // 第二遍：重新调度可推进的邻居
    // 通过 rescheduleChunk 走延迟队列（同步模式下），避免在 onChunkGenComplete 持锁期间
    // 重入 schedule（scheduleStatusStep 邻居扫描尚未完成时重入会过早重调度中心区块）。
    for (SingleChunkLifecycleManager* neighbour : readyToSchedule) {
        const ChunkStatus& target = neighbour->requestedGenStatus();
        rescheduleChunk(neighbour->x(), neighbour->z(), target);
    }
}

void ChunkTaskScheduler::releaseNeighbourRefCounts(
    SingleChunkLifecycleManager& holder, const ChunkStatus& completedStatus)
{
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

StaticChunkCache2D<ChunkPrimer*> ChunkTaskScheduler::buildNeighbourCache(
    ChunkCoord x, ChunkCoord z, const ChunkStep& step)
{
    const i32 radius = step.neighbourReadRadius();

    // 加载器：从各 holder 的 getChunkIfPresentUnchecked(requiredStatus) 取可变 ChunkPrimer
    // 调用前已由 checkNeighbour 确认所有邻居就绪，故 loader 必返回有效指针
    auto loader = [this, x, z, &step](ChunkCoord nx, ChunkCoord nz) -> ChunkPrimer* {
        if (nx == x && nz == z) {
            // 中心区块
            SingleChunkLifecycleManager* holder = findHolder(nx, nz);
            MC_ASSERT_RELEASE_MSG(holder != nullptr, "buildNeighbourCache: center holder missing");
            ChunkPrimer* primer = holder->getCurrentChunk();
            MC_ASSERT_RELEASE_MSG(primer != nullptr, "buildNeighbourCache: center primer missing");
            return primer;
        }
        const i32 dx = nx - x;
        const i32 dz = nz - z;
        const i32 distance = std::max(std::abs(dx), std::abs(dz));
        const ChunkStatus* requiredStatus = step.getRequiredStatusAtRadius(distance);
        MC_ASSERT_RELEASE_MSG(requiredStatus != nullptr, "buildNeighbourCache: requiredStatus is null");

        SingleChunkLifecycleManager* holder = findHolder(nx, nz);
        MC_ASSERT_RELEASE_MSG(holder != nullptr, "buildNeighbourCache: neighbour holder missing");
        ChunkPrimer* primer = holder->getChunkIfPresentUnchecked(*requiredStatus);
        MC_ASSERT_RELEASE_MSG(primer != nullptr,
            "buildNeighbourCache: neighbour primer missing (checkNeighbour should have verified readiness)");
        return primer;
    };

    return StaticChunkCache2D<ChunkPrimer*>(x, z, radius, loader);
}

// ============================================================================
// scheduleEmptyLoad：EMPTY 加载任务（从存档或新建空 Primer）
// ============================================================================

ChunkProgressionTask* ChunkTaskScheduler::scheduleEmptyLoad(
    ChunkCoord x, ChunkCoord z, SingleChunkLifecycleManager& holder)
{
    // EMPTY 推进不需要邻居缓存（neighbourReadRadius=0），构建仅含中心的缓存
    const ChunkPyramid& pyramid = ChunkPyramid::generationPyramid();
    const ChunkStep& emptyStep = pyramid.getStepTo(ChunkStatuses::EMPTY);

    auto loader = [this, x, z](ChunkCoord nx, ChunkCoord nz) -> ChunkPrimer* {
        MC_ASSERT_RELEASE(nx == x && nz == z);
        SingleChunkLifecycleManager* h = findHolder(nx, nz);
        MC_ASSERT_RELEASE(h != nullptr);
        ChunkPrimer* primer = h->getCurrentChunk();
        // EMPTY 阶段 currentChunk 可能为空，由 ChunkProgressionTask::executeEmptyLoad 创建
        return primer;
    };

    StaticChunkCache2D<ChunkPrimer*> neighbours(x, z, 0, loader);

    auto task = std::make_unique<ChunkProgressionTask>(
        *this, m_manager, x, z, ChunkStatuses::EMPTY, std::move(neighbours), emptyStep.blockStateWriteRadius());
    ChunkProgressionTask* taskPtr = task.get();

    holder.setGenerationTask(taskPtr, ChunkStatuses::EMPTY);

    // 选择执行器并提交（worker 池为空时在线执行）
    submitTask(std::move(task), ChunkStatuses::EMPTY, x, z, emptyStep.blockStateWriteRadius(), holder.cancelToken());

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
