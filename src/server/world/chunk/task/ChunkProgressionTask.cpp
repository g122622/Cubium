#include "server/world/chunk/task/ChunkProgressionTask.hpp"
#include "server/world/ServerWorld.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/chunk/IChunk.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <algorithm>

namespace mc {

// ============================================================================
// ChunkProgressionTask
// ============================================================================

ChunkProgressionTask::ChunkProgressionTask(ChunkTaskScheduler& scheduler, server::ServerWorld& world, i32 chunkX, i32 chunkZ)
    : m_scheduler(scheduler)
    , m_world(world)
    , m_chunkX(chunkX)
    , m_chunkZ(chunkZ)
    , m_priority(Priority::NORMAL) {
}

void ChunkProgressionTask::raisePriority(Priority priority) {
    Priority current = m_priority;
    // 只有新优先级更高（数值更小）时才更新
    if (static_cast<i32>(priority) < static_cast<i32>(current)) {
        m_priority = priority;
    }
}

void ChunkProgressionTask::setPriority(Priority priority) {
    m_priority = priority;
}

void ChunkProgressionTask::lowerPriority(Priority priority) {
    Priority current = m_priority;
    // 只有新优先级更低（数值更大）时才更新
    if (static_cast<i32>(priority) > static_cast<i32>(current)) {
        m_priority = priority;
    }
}

void ChunkProgressionTask::addCompleteCallback(CompleteCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_callbacks.push_back(std::move(callback));
}

void ChunkProgressionTask::complete(ChunkPrimer* primer, const std::string& error) {
    if (m_completed.exchange(true, std::memory_order_acq_rel)) {
        return;  // 已经完成
    }

    // 调用所有回调
    std::vector<CompleteCallback> callbacksCopy;
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        callbacksCopy = std::move(m_callbacks);
    }

    for (auto& callback : callbacksCopy) {
        if (callback) {
            callback(primer, error);
        }
    }
}

void ChunkProgressionTask::markScheduled() {
    m_scheduled.store(true, std::memory_order_release);
}

void ChunkProgressionTask::markCancelled() {
    m_cancelled.store(true, std::memory_order_release);
}

// ============================================================================
// ChunkUpgradeStatusTask
// ============================================================================

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

// ============================================================================
// ChunkLightTask
// ============================================================================

ChunkLightTask::ChunkLightTask(ChunkTaskScheduler& scheduler, server::ServerWorld& world,
                                 i32 chunkX, i32 chunkZ)
    : ChunkProgressionTask(scheduler, world, chunkX, chunkZ) {
}

void ChunkLightTask::schedule() {
    if (m_scheduled.exchange(true, std::memory_order_acq_rel)) {
        return;  // 已经调度
    }

    if (m_cancelled.load(std::memory_order_acquire)) {
        complete(nullptr, "Task cancelled");
        return;
    }

    // 光照需要写入半径 2，使用半径感知调度器
    auto* radiusAware = &m_scheduler.getRadiusAwareScheduler();
    if (radiusAware) {
        radiusAware->addTask(
            [this]() { executeLight(); },
            m_chunkX, m_chunkZ, 2, m_priority);
    }
}

void ChunkLightTask::cancel() {
    markCancelled();
}

void ChunkLightTask::setChunkData(ChunkData* data) {
    m_chunkData = data;
}

void ChunkLightTask::executeLight() {
    if (m_cancelled.load(std::memory_order_acquire)) {
        complete(nullptr, "Task cancelled");
        return;
    }

    if (m_completed.load(std::memory_order_acquire)) {
        return;  // 已经完成
    }

    MC_TRACE_EVENT("world.chunk_task", "LightTask");

    // 实际的光照计算逻辑
    if (m_chunkData) {
        auto* lightManager = m_world.lightManager();
        if (lightManager) {
            // 光照计算由 WorldLightManager 处理
            // lightManager->lightChunk(m_chunkData);
        }
    }

    complete(nullptr, "");
}

// ============================================================================
// ChunkFullTask
// ============================================================================

ChunkFullTask::ChunkFullTask(ChunkTaskScheduler& scheduler, server::ServerWorld& world,
                               i32 chunkX, i32 chunkZ)
    : ChunkProgressionTask(scheduler, world, chunkX, chunkZ) {
}

void ChunkFullTask::schedule() {
    if (m_scheduled.exchange(true, std::memory_order_acq_rel)) {
        return;  // 已经调度
    }

    if (m_cancelled.load(std::memory_order_acquire)) {
        complete(nullptr, "Task cancelled");
        return;
    }

    // FULL 任务必须在主线程执行
    m_scheduler.executeOnMainThread(
        [this]() { executeFull(); }, m_priority);
}

void ChunkFullTask::cancel() {
    markCancelled();
}

void ChunkFullTask::setChunkData(ChunkData* data) {
    m_chunkData = data;
}

void ChunkFullTask::executeFull() {
    if (m_cancelled.load(std::memory_order_acquire)) {
        complete(nullptr, "Task cancelled");
        return;
    }

    if (m_completed.load(std::memory_order_acquire)) {
        return;  // 已经完成
    }

    MC_TRACE_EVENT("world.chunk_task", "FullTask");

    // 完成区块的最后步骤
    // - Entity 加载（如果有持久化）
    // - 最终验证
    // - 标记区块为完成状态

    complete(nullptr, "");
}

} // namespace mc
