/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "ITask.hpp"
#include "common/core/Types.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace mc::util {

/**
 * @brief 通用任务池
 *
 * 服务端与客户端共用的通用多线程任务池。支持优先级调度、协作取消、Perfetto追踪。
 * 由各宿主（MinecraftServer 的 ServerCompute/ServerIO、ClientApplication 的 ClientCompute 等）
 * 以值成员持有，生命周期绑定宿主。
 *
 * 特性：
 * - 优先级队列调度（数值越小优先级越高）
 * - 协作取消机制（通过 atomic<bool> 取消令牌）
 * - Perfetto追踪集成
 * - 线程命名（用于调试；rankBase 决定 profiler 中线程排序分组）
 *
 * 使用方法：
 * @code
 * UniversalWorkerPool pool(4, "ServerWorker", 200);
 * pool.start();
 *
 * // 提交任务
 * auto task = std::make_unique<MyTask>();
 * pool.submit(std::move(task),
 *             [](bool success, ITask*) { },
 *             TaskPriority::Normal);
 *
 * // 关闭
 * pool.shutdown();
 * @endcode
 */
class UniversalWorkerPool {
public:
    // ============================================================================
    // 构造与析构
    // ============================================================================

    /**
     * @brief 创建任务池
     *
     * @param threadCount 线程数量，-1 表示自动检测（硬件并发数的一半）
     * @param name 线程名称前缀（用于调试和追踪）
     * @param rankBase profiler 线程排序基准（rankBase + workerId），各组间隔 100。
     *                  约定：ServerCompute=100、ServerIO=200、ClientCompute=300。
     */
    explicit UniversalWorkerPool(i32 threadCount, std::string name, i32 rankBase);

    ~UniversalWorkerPool();

    // 禁止拷贝
    UniversalWorkerPool(const UniversalWorkerPool&) = delete;
    UniversalWorkerPool& operator=(const UniversalWorkerPool&) = delete;

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
    [[nodiscard]] bool isRunning() const
    {
        return m_running.load(std::memory_order::acquire);
    }

    // ============================================================================
    // 任务提交
    // ============================================================================

    /**
     * @brief 提交任务
     *
     * 【任务对象所有权】任务对象的所有权随 unique_ptr 一并转移给任务池：池在任务执行完毕
     * （含完成回调）后立即销毁它。调用方不得在提交后通过裸指针访问任务对象（例如把它当作
     * 等待/通知的同步句柄），否则会访问到已销毁的对象；需要观察任务状态请在 callback 内进行，
     * 或使用独立于任务对象、由调用方自己持有的同步量。
     *
     * @param task 任务对象
     * @param callback 完成回调（可为空）
     * @param priority 优先级
     * @param abortSignal 取消令牌（可为空）
     * @return 任务ID
     */
    u64 submit(std::unique_ptr<ITask> task,
        TaskCallback callback,
        TaskPriority priority = TaskPriority::Normal,
        std::shared_ptr<std::atomic<bool>> abortSignal = nullptr);

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
    [[nodiscard]] i32 threadCount() const
    {
        return m_threadCount;
    }

    /**
     * @brief 诊断：转储排队与运行任务数量
     *
     * 用于死锁诊断：当 pendingTaskCount > 0 但 runningTaskCount == 0 时，
     * 转储队列与运行数量，检查线程池推进状态。
     */
    void debugDumpState();

    /**
     * @brief 诊断：转储每个 worker 当前正在执行的任务（描述与已执行时长）
     *
     * 用于死锁定位：当 runningTaskCount > 0 但 pending 冻结时，转储各 worker 的执行任务，
     * 定位长时间未完成的任务。
     */
    void debugDumpRunningTasks();

private:
    /**
     * @brief 内部任务结构
     */
    struct InternalTask {
        u64 id;
        TaskPriority priority;
        std::shared_ptr<ITask> task;
        TaskCallback callback;
        std::shared_ptr<std::atomic<bool>> abortSignal;
    };

    /**
     * @brief 任务比较器（用于优先队列）
     *
     * 优先级小的在前，同优先级提交 ID 小的在前。
     */
    struct TaskComparator {
        bool operator()(const std::shared_ptr<InternalTask>& a, const std::shared_ptr<InternalTask>& b) const
        {
            if (a->priority != b->priority) {
                return static_cast<i8>(a->priority) > static_cast<i8>(b->priority);
            }
            return a->id > b->id;
        }
    };

    /**
     * @brief 工作线程函数
     */
    void _workerThread(i32 workerId);

    /**
     * @brief 执行任务
     */
    void _executeTask(const std::shared_ptr<InternalTask>& task);

    /** @brief 在锁外取消任务并通知失败，隔离收尾异常。 */
    void _cancelTask(const std::shared_ptr<InternalTask>& task);

    /** @brief 在锁外调用完成回调，隔离回调异常。 */
    void _callback(const std::shared_ptr<InternalTask>& task, bool success);

    /**
     * @brief 获取最优线程数
     */
    [[nodiscard]] static i32 _getOptimalThreadCount();

    /**
     * @brief 检查任务是否已被取消
     */
    [[nodiscard]] static bool _isTaskCancelled(const InternalTask& task);

    /**
     * @brief 结清一次未完成任务计数
     *
     * 与 submit() 的递增严格成对：任务执行完（含回调）或被丢弃（取消/裁剪/关闭清空）时各调用一次。
     * 计数失衡（尤其下溢）会让 waitForCompletion() 永久挂起，故下溢直接断言暴露。
     */
    void _releaseOutstandingTask();

    /**
     * @brief 若无未完成任务则唤醒所有 waitForCompletion 等待方
     *
     * 谓词状态（m_outstandingTaskCount）变更与原实现 notify_all 均不持 m_completionMutex,worker
     * 可能在等待方"谓词检查=false 与原子 release+block"之间完成状态变更并 notify,该 notify 落在
     * 无人阻塞时丢失,等待方永久挂起(CTest 超时,-j16 抢占下偶发)。修:持 m_completionMutex
     * 重检谓词再 notify,把 worker 的 notify 与等待方的"谓词检查+wait"串行化。
     *
     * fast-path:m_outstandingTaskCount≠0 时必然非空闲,直接返回不抢锁(任务执行期间高频调用,
     * 避免无谓锁竞争);仅计数归零才抢锁做权威判定。与 m_completionMutex 锁序固定。
     */
    void _notifyIfIdle();

    // ============================================================================
    // 成员变量
    // ============================================================================

    // 工作线程
    std::vector<std::thread> m_workers;
    std::string m_poolName;
    i32 m_threadCount;
    i32 m_rankBase;

    // 任务队列
    std::priority_queue<std::shared_ptr<InternalTask>, std::vector<std::shared_ptr<InternalTask>>, TaskComparator>
        m_taskQueue;
    mutable std::mutex m_queueMutex;
    std::condition_variable m_condition;

    // 任务ID生成
    std::atomic<u64> m_nextTaskId{1};

    // 正在执行的任务数量（包含回调）
    std::atomic<size_t> m_runningTaskCount{0};

    // 未完成任务数量：自 submit() 入队起算，到"任务回调执行完毕"或"任务被丢弃
    // （取消 / 裁剪 / 关闭清空队列）"为止。waitForCompletion() 以此为完成谓词。
    //
    // 不能用 pendingTaskCount()==0 && runningTaskCount()==0 做谓词，存在两个提前返回窗口：
    //   1) 排队任务取消后，收尾回调可能仍未结束；
    //   2) worker "出队"与"标记运行"之间，队列已空且运行数为 0，但任务其实还没跑。
    std::atomic<size_t> m_outstandingTaskCount{0};

    // 诊断：正在执行的任务信息（用于死锁定位）。
    // m_runningTaskInfo 在 m_runningTaskMutex 下读写，记录每个 worker 当前执行任务的描述与开始时间。
    struct RunningTaskInfo {
        std::string description;
        std::chrono::steady_clock::time_point startTime;
    };

    std::mutex m_runningTaskMutex;
    std::unordered_map<i32, RunningTaskInfo> m_runningTaskInfo; // key = workerId

    // 运行状态
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stop{false};

    // 等待完成的条件变量
    std::mutex m_completionMutex;
    std::condition_variable m_completionCondition;
};

} // namespace mc::util
