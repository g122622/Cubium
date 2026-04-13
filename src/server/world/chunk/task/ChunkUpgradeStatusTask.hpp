#pragma once

#include "server/world/chunk/task/ChunkProgressionTask.hpp"
#include "common/world/chunk/ChunkStatus.hpp"
#include <array>

namespace mc {

// 前向声明
class IChunk;

// ============================================================================
// ChunkUpgradeStatusTask - 区块状态升级任务
// ============================================================================

/**
 * @brief 区块状态升级任务
 *
 * 负责将区块从一个状态升级到另一个状态。
 * 根据 ChunkStatus 的配置选择并行执行器或半径感知调度器。
 */
class ChunkUpgradeStatusTask : public ChunkProgressionTask {
public:
    ChunkUpgradeStatusTask(ChunkTaskScheduler& scheduler, server::ServerWorld& world,
                            i32 chunkX, i32 chunkZ,
                            ChunkStatus targetStatus,
                            ChunkStatus currentStatus);

    ~ChunkUpgradeStatusTask() override = default;

    void schedule() override;
    void cancel() override;

    [[nodiscard]] ChunkStatus getTargetStatus() const override { return m_targetStatus; }
    [[nodiscard]] ChunkStatus getCurrentStatus() const { return m_currentStatus; }

    /**
     * @brief 更新目标状态（如果新目标更高）
     * @return 是否更新了目标
     */
    bool upgradeTarget(ChunkStatus newTarget);

    /**
     * @brief 设置邻居区块（用于状态升级）
     */
    void setNeighbors(const std::array<IChunk*, 8>& neighbors);

private:
    void executeUpgrade();

    ChunkStatus m_targetStatus;
    ChunkStatus m_currentStatus;
    std::array<IChunk*, 8> m_neighbors{};
    bool m_hasNeighbors{false};
};

} // namespace mc
