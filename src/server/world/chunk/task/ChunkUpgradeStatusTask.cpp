#include "server/world/chunk/task/ChunkUpgradeStatusTask.hpp"
#include "server/world/chunk/ChunkTaskScheduler.hpp"
#include "server/world/ServerWorld.hpp"
#include "common/world/chunk/IChunk.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <algorithm>

namespace mc {

ChunkUpgradeStatusTask::ChunkUpgradeStatusTask(ChunkTaskScheduler& scheduler, server::ServerWorld& world,
                                                 i32 chunkX, i32 chunkZ,
                                                 ChunkStatus targetStatus,
                                                 ChunkStatus currentStatus)
    : ChunkProgressionTask(scheduler, world, chunkX, chunkZ)
    , m_targetStatus(targetStatus)
    , m_currentStatus(currentStatus) {
    m_neighbors.fill(nullptr);
}

void ChunkUpgradeStatusTask::schedule() {
    if (m_scheduled.exchange(true, std::memory_order_acq_rel)) {
        return;  // 已经调度
    }

    if (m_cancelled.load(std::memory_order_acquire)) {
        complete(nullptr, "Task cancelled");
        return;
    }

    // 根据状态配置选择执行器
    i32 writeRadius = ChunkTaskScheduler::getWriteRadius(m_targetStatus);
    bool parallelCapable = ChunkTaskScheduler::isParallelCapable(m_targetStatus);

    if (writeRadius > 0) {
        // 使用半径感知调度器
        auto* radiusAware = &m_scheduler.getRadiusAwareScheduler();
        if (radiusAware) {
            radiusAware->addTask(
                [this]() { executeUpgrade(); },
                m_chunkX, m_chunkZ, writeRadius, m_priority);
        }
    } else if (parallelCapable) {
        // 使用并行执行器
        m_scheduler.scheduleChunkTask(m_chunkX, m_chunkZ,
            [this]() { executeUpgrade(); }, m_priority);
    } else {
        // 使用主线程执行器
        m_scheduler.executeOnMainThread(
            [this]() { executeUpgrade(); }, m_priority);
    }
}

void ChunkUpgradeStatusTask::cancel() {
    markCancelled();
}

bool ChunkUpgradeStatusTask::upgradeTarget(ChunkStatus newTarget) {
    if (newTarget.ordinal() > m_targetStatus.ordinal()) {
        m_targetStatus = newTarget;
        return true;
    }
    return false;
}

void ChunkUpgradeStatusTask::setNeighbors(const std::array<IChunk*, 8>& neighbors) {
    m_neighbors = neighbors;
    m_hasNeighbors = true;
}

void ChunkUpgradeStatusTask::executeUpgrade() {
    if (m_cancelled.load(std::memory_order_acquire)) {
        complete(nullptr, "Task cancelled");
        return;
    }

    if (m_completed.load(std::memory_order_acquire)) {
        return;  // 已经完成
    }

    MC_TRACE_EVENT("world.chunk_task", "UpgradeStatusTask");

    // 实际的状态升级逻辑将在 ServerChunkManager 中实现
    // 这里只是框架，具体的生成逻辑需要访问区块管理器

    // 完成任务
    complete(nullptr, "");
}

} // namespace mc
