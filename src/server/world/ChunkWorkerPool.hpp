#pragma once

#include "../../common/world/chunk/SingleChunkLifecycleManager.hpp"
#include "../../common/world/chunk/ChunkStatus.hpp"
#include "../../common/world/gen/chunk/IChunkGenerator.hpp"
#include "../../common/core/Types.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <functional>
#include <atomic>

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
     */
    using GeneratorFunc = std::function<void(ChunkPrimer& chunk, const ChunkStatus& targetStatus)>;

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
     * @brief 提交自定义任务
     * @param task 任务
     * @param generator 生成器函数
     * @param callback 完成回调
     */
    void submitTask(ChunkTask task,
                    GeneratorFunc generator,
                    CompletionCallback callback);

    // ============================================================================
    // 统计
    // ============================================================================

    /**
     * @brief 获取待处理任务数量
     */
    [[nodiscard]] size_t pendingTaskCount() const;

    /**
     * @brief 获取线程数量
     */
    [[nodiscard]] i32 threadCount() const { return m_threadCount; }

    /**
     * @brief 设置生成器函数
     */
    void setGenerator(GeneratorFunc generator) { m_generator = std::move(generator); }

private:
    /**
     * @brief 内部任务结构
     */
    struct InternalTask {
        ChunkTask task;
        GeneratorFunc generator;
        CompletionCallback callback;
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
     * @brief 任务比较器（用于优先队列）
     *
     * C++17 不支持 lambda 作为模板参数，使用仿函数
     */
    struct TaskComparator {
        bool operator()(const InternalTask& a, const InternalTask& b) const {
            return a.task < b.task;
        }
    };

    // 线程
    std::vector<std::thread> m_workers;
    i32 m_threadCount;

    // 任务队列
    std::priority_queue<InternalTask, std::vector<InternalTask>, TaskComparator> m_taskQueue;
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
