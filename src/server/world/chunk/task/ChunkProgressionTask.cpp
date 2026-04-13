#include "server/world/chunk/task/ChunkProgressionTask.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/util/assert/AssertAll.hpp"

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
    const i32 newPriorityVal = static_cast<i32>(priority);
    MC_ASSERT_RELEASE_MSG(newPriorityVal >= 0 && newPriorityVal < static_cast<i32>(Priority::COUNT),
                          "Invalid priority value");

    // 只有新优先级更高（数值更小）时才更新
    if (newPriorityVal < static_cast<i32>(m_priority)) {
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

} // namespace mc
