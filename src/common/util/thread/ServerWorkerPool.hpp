#pragma once

#include "ITask.hpp"
#include "common/core/Types.hpp"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace mc::util {

/**
 * @brief 服务端通用任务池
 *
 * 支持优先级调度、协作取消、Perfetto追踪的多线程任务池。
 *
 * 特性：
 * - 优先级队列调度（数值越小优先级越高）
 * - 协作取消机制（通过 atomic<bool> 取消令牌）
 * - Perfetto追踪集成
 * - 线程命名（用于调试）
 *
 * 使用方法：
 * @code
 * ServerWorkerPool pool(4, "ServerWorker");
 * pool.start();
 *
 * // 提交任务
 * auto task = std::make_unique<MyTask>();
 * pool.submit(std::move(task),
 *             [](bool success) {  },
 *             TaskPriority::Normal);
 *
 * // 关闭
 * pool.shutdown();
 * @endcode
 */
class ServerWorkerPool {
public:
    // ============================================================================
    // 构造与析构
    // ============================================================================

    /**
     * @brief 创建任务池
     *
     * @param threadCount 线程数量，-1 表示自动检测（硬件并发数的一半）
     * @param name 线程名称前缀（用于调试和追踪）
     */
    explicit ServerWorkerPool(i32 threadCount = -1, std::string name = "ServerWorker");

    ~ServerWorkerPool();

    // 禁止拷贝
    ServerWorkerPool(const ServerWorkerPool&) = delete;
    ServerWorkerPool& operator=(const ServerWorkerPool&) = delete;

    // ============================================================================
    // 生命周期
    // ============================================================================

    /**
     * @brief 启动工作线程
     *
     * 如果已经运行，则不执行任何操作。
     */
    void start();

    /**
     * @brief 关闭工作线程
     *
     * 会等待所有正在执行的任务完成，然后停止线程。
     * 排队中的任务不会被执行。
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
     * @brief 提交任务
     *
     * @param task 任务对象
     * @param callback 完成回调（可为空）
     * @param priority 优先级
     * @param cancelToken 取消令牌（可为空）
     * @return 任务ID
     */
    u64 submit(std::unique_ptr<ITask> task,
               TaskCallback callback,
               TaskPriority priority = TaskPriority::Normal,
               std::shared_ptr<std::atomic<bool>> cancelToken = nullptr);

    // ============================================================================
    // 任务管理
    // ============================================================================

    /**
     * @brief 取消指定任务
     *
     * 设置取消令牌为 true。如果任务已经开始执行，则无法中断。
     *
     * @param taskId 任务ID
     * @return true 如果找到并取消了任务
     */
    bool cancel(u64 taskId);

    /**
     * @brief 裁剪已取消的排队任务
     *
     * 遍历任务队列，移除已取消的任务。
     * 适合在大量取消后调用，释放内存。
     */
    void pruneCancelledTasks();

    /**
     * @brief 等待所有任务完成
     *
     * 阻塞直到任务队列为空且没有正在执行的任务。
     */
    void waitForCompletion();

    // ============================================================================
    // 统计
    // ============================================================================

    /**
     * @brief 获取待处理任务数量
     */
    [[nodiscard]] size_t pendingTaskCount() const;

    /**
     * @brief 获取正在执行的任务数量
     */
    [[nodiscard]] size_t runningTaskCount() const;

    /**
     * @brief 获取线程数量
     */
    [[nodiscard]] i32 threadCount() const { return m_threadCount; }

private:
    /**
     * @brief 内部任务结构
     */
    struct InternalTask {
        u64 id;
        TaskPriority priority;
        u64 timestamp;
        std::shared_ptr<ITask> task;
        TaskCallback callback;
        std::shared_ptr<std::atomic<bool>> cancelToken;
    };

    /**
     * @brief 任务比较器（用于优先队列）
     *
     * 优先级小的在前，同优先级时间早的在前。
     */
    struct TaskComparator {
        bool operator()(const std::shared_ptr<InternalTask>& a,
                        const std::shared_ptr<InternalTask>& b) const {
            if (a->priority != b->priority) {
                return static_cast<i8>(a->priority) > static_cast<i8>(b->priority);
            }
            return a->timestamp > b->timestamp;
        }
    };

    /**
     * @brief 工作线程函数
     */
    void workerThread(i32 workerId);

    /**
     * @brief 执行任务
     */
    void executeTask(std::shared_ptr<InternalTask> task);

    /**
     * @brief 获取最优线程数
     */
    [[nodiscard]] static i32 getOptimalThreadCount();

    /**
     * @brief 检查任务是否已被取消
     */
    [[nodiscard]] static bool isTaskCancelled(const InternalTask& task);

    // ============================================================================
    // 成员变量
    // ============================================================================

    // 工作线程
    std::vector<std::thread> m_workers;
    std::string m_poolName;
    i32 m_threadCount;

    // 任务队列
    std::priority_queue<std::shared_ptr<InternalTask>,
                        std::vector<std::shared_ptr<InternalTask>>,
                        TaskComparator> m_taskQueue;
    mutable std::mutex m_queueMutex;
    std::condition_variable m_condition;

    // 任务ID生成
    std::atomic<u64> m_nextTaskId{1};

    // 正在执行的任务数量
    std::atomic<size_t> m_runningTaskCount{0};

    // 运行状态
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stop{false};

    // 等待完成的条件变量
    std::mutex m_completionMutex;
    std::condition_variable m_completionCondition;
};

} // namespace mc::util
