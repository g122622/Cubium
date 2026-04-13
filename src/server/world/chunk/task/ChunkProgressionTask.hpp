#pragma once

#include "server/world/chunk/ChunkTaskScheduler.hpp"
#include "common/world/chunk/ChunkStatus.hpp"
#include "common/core/Types.hpp"
#include <functional>
#include <memory>
#include <atomic>
#include <vector>
#include <mutex>

namespace mc {

// 前向声明
class ChunkPrimer;
class IChunk;
class ChunkData;

namespace server {
class ServerWorld;
}

// ============================================================================
// ChunkProgressionTask - 区块进度任务基类
// ============================================================================

/**
 * @brief 区块进度任务基类
 *
 * 参考 Moonrise 的 ChunkProgressionTask 设计。
 * 所有区块生成/加载任务的基类，提供：
 * - 状态追踪
 * - 优先级管理
 * - 完成回调
 * - 取消支持
 */
class ChunkProgressionTask {
public:
    using CompleteCallback = std::function<void(ChunkPrimer*, const std::string&)>;

    ChunkProgressionTask(ChunkTaskScheduler& scheduler, server::ServerWorld& world, i32 chunkX, i32 chunkZ);
    virtual ~ChunkProgressionTask() = default;

    // 禁止拷贝
    ChunkProgressionTask(const ChunkProgressionTask&) = delete;
    ChunkProgressionTask& operator=(const ChunkProgressionTask&) = delete;

    // ============================================================================
    // 核心接口
    // ============================================================================

    /**
     * @brief 调度任务执行
     */
    virtual void schedule() = 0;

    /**
     * @brief 取消任务
     */
    virtual void cancel() = 0;

    // ============================================================================
    // 属性访问
    // ============================================================================

    [[nodiscard]] i32 getChunkX() const { return m_chunkX; }
    [[nodiscard]] i32 getChunkZ() const { return m_chunkZ; }
    [[nodiscard]] Priority getPriority() const { return m_priority; }
    [[nodiscard]] bool isScheduled() const { return m_scheduled.load(std::memory_order_acquire); }
    [[nodiscard]] bool isCompleted() const { return m_completed.load(std::memory_order_acquire); }
    [[nodiscard]] bool isCancelled() const { return m_cancelled.load(std::memory_order_acquire); }

    // ============================================================================
    // 优先级操作
    // ============================================================================

    /**
     * @brief 提升优先级
     * @param priority 新的优先级（如果更高）
     */
    virtual void raisePriority(Priority priority);

    /**
     * @brief 设置优先级
     * @param priority 新的优先级
     */
    virtual void setPriority(Priority priority);

    /**
     * @brief 降低优先级
     * @param priority 新的优先级（如果更低）
     */
    virtual void lowerPriority(Priority priority);

    // ============================================================================
    // 完成回调
    // ============================================================================

    /**
     * @brief 添加完成回调
     */
    void addCompleteCallback(CompleteCallback callback);

    // ============================================================================
    // 状态
    // ============================================================================

    [[nodiscard]] virtual ChunkStatus getTargetStatus() const = 0;

protected:
    /**
     * @brief 标记任务完成并调用回调
     * @param primer 生成的区块（可能为空）
     * @param error 错误信息（为空表示成功）
     */
    void complete(ChunkPrimer* primer, const std::string& error);

    /**
     * @brief 标记任务已调度
     */
    void markScheduled();

    /**
     * @brief 标记任务已取消
     */
    void markCancelled();

    ChunkTaskScheduler& m_scheduler;
    server::ServerWorld& m_world;
    i32 m_chunkX;
    i32 m_chunkZ;
    Priority m_priority;
    std::atomic<bool> m_scheduled{false};
    std::atomic<bool> m_completed{false};
    std::atomic<bool> m_cancelled{false};

    std::vector<CompleteCallback> m_callbacks;
    std::mutex m_callbackMutex;
};

} // namespace mc
