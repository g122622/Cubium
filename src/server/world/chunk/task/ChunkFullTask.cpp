#include "server/world/chunk/task/ChunkFullTask.hpp"
#include "server/world/chunk/ChunkTaskScheduler.hpp"
#include "server/world/ServerWorld.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/perfetto/TraceEvents.hpp"

namespace mc {

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
