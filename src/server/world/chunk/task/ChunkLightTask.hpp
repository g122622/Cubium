#pragma once

#include "server/world/chunk/task/ChunkProgressionTask.hpp"
#include "common/world/chunk/ChunkStatus.hpp"

namespace mc {

// 前向声明
class ChunkData;

// ============================================================================
// ChunkLightTask - 光照计算任务
// ============================================================================

/**
 * @brief 光照计算任务
 *
 * 异步执行光照计算，需要写入半径 2。
 * 参考 Moonrise 的 ChunkLightTask 设计，区分已光照和未光照区块：
 * - 已光照区块：使用 forceLoadInChunk + checkChunkEdges
 * - 未光照区块：使用完整 lightChunk
 */
class ChunkLightTask : public ChunkProgressionTask {
public:
    ChunkLightTask(ChunkTaskScheduler& scheduler, server::ServerWorld& world,
                   i32 chunkX, i32 chunkZ);

    ~ChunkLightTask() override = default;

    void schedule() override;
    void cancel() override;

    [[nodiscard]] ChunkStatus getTargetStatus() const override { return ChunkStatuses::LIGHT; }

    /**
     * @brief 设置区块数据（用于光照计算）
     */
    void setChunkData(ChunkData* data);

private:
    void executeLight();

    ChunkData* m_chunkData{nullptr};
};

} // namespace mc
