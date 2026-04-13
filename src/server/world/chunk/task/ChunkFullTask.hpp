#pragma once

#include "server/world/chunk/task/ChunkProgressionTask.hpp"
#include "common/world/chunk/ChunkStatus.hpp"

namespace mc {

// 前向声明
class ChunkData;

// ============================================================================
// ChunkFullTask - 区块完成任务
// ============================================================================

/**
 * @brief 区块完成任务
 *
 * 在主线程执行区块完成操作（entity 加载等）。
 */
class ChunkFullTask : public ChunkProgressionTask {
public:
    ChunkFullTask(ChunkTaskScheduler& scheduler, server::ServerWorld& world,
                  i32 chunkX, i32 chunkZ);

    ~ChunkFullTask() override = default;

    void schedule() override;
    void cancel() override;

    [[nodiscard]] ChunkStatus getTargetStatus() const override { return ChunkStatuses::FULL; }

    /**
     * @brief 设置区块数据
     */
    void setChunkData(ChunkData* data);

private:
    void executeFull();

    ChunkData* m_chunkData{nullptr};
};

} // namespace mc
