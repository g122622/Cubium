#pragma once

#include "common/world/chunk/SingleChunkLifecycleManager.hpp"
#include "common/world/chunk/ChunkStatus.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/core/Types.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <functional>
#include <atomic>
#include <unordered_map>
#include <memory>
#include <unordered_set>

namespace mc::server {

/**
 * @brief 区块 Worker 线程池
 *
 * 管理多个 Worker 线程，异步执行区块生成任务。
 * 不阻塞服务端主循环。
 *
 * 使用方法：
 * @code
 * ChunkWorkerPool pool(4);  // 4 个 Worker 线程
 * pool.start();
 *
 * // 提交任务
 * pool.submit(ChunkTask::Generate, x, z, &ChunkStatuses::FULL, [](ChunkPrimer* chunk) {
 *     // 任务完成回调
 * });
 *
 * // 关闭
 * pool.shutdown();
 * @endcode
 */
class ChunkWorkerPool {
public:
    /**
     * @brief 任务完成回调类型
     * @param success 是否成功
     * @param chunk 生成的区块（如果成功）
     */
    using CompletionCallback = std::function<void(bool success, ChunkPrimer* chunk)>;

    /**
     * @brief 生成器函数类型
     *
     * @param chunk 区块中间态
     * @param targetStatus 目标生成阶段
     * @param cancelSignal 协作取消信号（true 表示应尽快停止）
     */
    using GeneratorFunc = std::function<void(ChunkPrimer& chunk,
                                             const ChunkStatus& targetStatus,
                                             const std::atomic<bool>& cancelSignal)>;

    // ============================================================================
    // 构造与析构
    // ============================================================================

    /**
     * @brief 创建 Worker 线程池
     * @param threadCount 线程数量（-1 表示自动检测）
     */
    explicit ChunkWorkerPool(i32 threadCount = -1);

    ~ChunkWorkerPool();

    // 禁止拷贝
    ChunkWorkerPool(const ChunkWorkerPool&) = delete;
    ChunkWorkerPool& operator=(const ChunkWorkerPool&) = delete;

    // ============================================================================
    // 生命周期
    // ============================================================================

    /**
     * @brief 启动 Worker 线程
     */
    void start();

    /**
     * @brief 关闭 Worker 线程
     *
     * 会等待所有任务完成
     */
    void shutdown();

    /**
     * @brief 检查是否正在运行
     */
    [[nodiscard]] bool isRunning() const { return m_running.load(std::memory_order_acquire); }

    // ============================================================================
    // 任务提交
    // ============================================================================

    /**
     * @brief 提交生成任务
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param targetStatus 目标状态
     * @param callback 完成回调
     * @param priority 优先级（越小越高）
     */
    void submitGenerate(ChunkCoord x, ChunkCoord z,
                        const ChunkStatus& targetStatus,
                        CompletionCallback callback,
                        i32 priority = 0);

    /**
     * @brief 提交可取消的生成任务
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param targetStatus 目标状态
     * @param callback 完成回调
     * @param cancelToken 取消令牌（可为空）
     * @param priority 优先级（越小越高）
     */
    void submitGenerate(ChunkCoord x, ChunkCoord z,
                        const ChunkStatus& targetStatus,
                        CompletionCallback callback,
                        std::shared_ptr<std::atomic<bool>> cancelToken,
                        i32 priority);

    // ============================================================================
    // 统计
    // ============================================================================

    /**
     * @brief 获取待处理任务数量
     */
    [[nodiscard]] size_t pendingTaskCount() const;

    /**
     * @brief 裁剪已取消的排队任务
     *
     * @note 该操作会重建优先队列，适合在大量取消后调用。
     */
    void pruneCancelledTasks();

    /**
     * @brief 获取线程数量
     */
    [[nodiscard]] i32 threadCount() const { return m_threadCount; }

    /**
     * @brief 设置生成器函数
     */
    void setGenerator(GeneratorFunc generator) { m_generator = std::move(generator); }

private:
    struct CoalescedCallbacks {
        std::mutex mutex;
        std::vector<CompletionCallback> callbacks;
    };

    /**
     * @brief 内部任务结构
     */
    struct InternalTask {
        ChunkTask task;
        GeneratorFunc generator;
        CompletionCallback callback;
        std::shared_ptr<std::atomic<bool>> cancelToken;
        u64 coalesceKey = 0;
        std::shared_ptr<CoalescedCallbacks> coalescedCallbacks;
    };

    /**
     * @brief Worker 线程函数
     */
    void workerThread(i32 workerId);

    /**
     * @brief 执行任务
     */
    void executeTask(InternalTask& task);

    /**
     * @brief 获取最优线程数
     */
    static i32 getOptimalThreadCount();

    /**
     * @brief 检查任务是否已被取消
     */
    [[nodiscard]] static bool isTaskCancelled(const InternalTask& task);

    /**
     * @brief 在持锁状态下裁剪已取消任务
     */
    void pruneCancelledTasksLocked();

    /**
     * @brief 任务比较器（用于优先队列）
     *
     * C++17 不支持 lambda 作为模板参数，使用仿函数
     */
    struct TaskComparator {
        bool operator()(const std::shared_ptr<InternalTask>& a,
                        const std::shared_ptr<InternalTask>& b) const {
            return a->task < b->task;
        }
    };

    // 线程
    std::vector<std::thread> m_workers;
    i32 m_threadCount;

    // 任务队列
    std::priority_queue<std::shared_ptr<InternalTask>,
                        std::vector<std::shared_ptr<InternalTask>>,
                        TaskComparator> m_taskQueue;
    std::vector<std::unique_ptr<ChunkPrimer>> m_completedChunks;

    mutable std::mutex m_queueMutex;
    mutable std::mutex m_completedMutex;
    std::condition_variable m_condition;

    // 状态
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stop{false};

    // 生成器
    GeneratorFunc m_generator;
};

} // namespace mc::server
