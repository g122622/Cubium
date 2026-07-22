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
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
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
 * - 区域互斥（对齐 Moonrise 区域锁执行器，见带坐标的 submit 重载）
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
    [[nodiscard]] bool isRunning() const { return m_running.load(std::memory_order::acquire); }

    // ============================================================================
    // 任务提交
    // ============================================================================

    /**
     * @brief 提交任务
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

    /**
     * @brief 提交带区域互斥的任务（对齐 Moonrise 区域锁执行器）
     *
     * 任务携带一个矩形写入区域 `[centerX-writeRadius, centerX+writeRadius] ×
     * [centerZ-writeRadius, centerZ+writeRadius]`。调度器保证：同一时刻不存在
     * 两个写入区域**重叠**的区域互斥任务同时执行。区域不重叠的任务可并行执行。
     *
     * 用途：FEATURES/LIGHT/SPAWN/FULL 等会写方块的状态通过此重载提交，
     * 避免并发生成时同一区块被两个任务同时写入。writeRadius 来源为
     * `ChunkStep::blockStateWriteRadius()`（FEATURES=1，LIGHT=2，其他≤0）。
     *
     * writeRadius ≤ 0 的任务视作仅写中心区块（writeRadius=0），区域为单个区块。
     * 无区域互斥需求的任务应使用无坐标的 submit 重载（可完全并行）。
     *
     * @param task 任务对象
     * @param callback 完成回调（可为空）
     * @param centerX 写入区域中心 X 坐标（区块坐标）
     * @param centerZ 写入区域中心 Z 坐标（区块坐标）
     * @param writeRadius 写入半径（切比雪夫距离），≥0
     * @param priority 优先级
     * @param abortSignal 取消令牌（可为空）
     * @return 任务ID
     */
    u64 submit(std::unique_ptr<ITask> task,
        TaskCallback callback,
        ChunkCoord centerX,
        ChunkCoord centerZ,
        i32 writeRadius,
        TaskPriority priority = TaskPriority::Normal,
        std::shared_ptr<std::atomic<bool>> abortSignal = nullptr);

    /**
     * @brief 查询某写入区域是否可立即执行（无冲突）
     *
     * 检查 `[centerX±writeRadius, centerZ±writeRadius]` 是否与任何正在执行的
     * 区域互斥任务重叠。无重叠返回 true。
     *
     * 注意：返回值仅反映调用瞬间的状态，调用者仍需通过带坐标的 submit 提交，
     * 调度器会在执行前再次检查（TOCTOU 由内部锁保护）。
     */
    [[nodiscard]] bool canExecuteNow(ChunkCoord centerX, ChunkCoord centerZ, i32 writeRadius) const;

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

    /**
     * @brief 诊断：转储工作线程池状态（运行区域、队列任务区域分布）
     *
     * 用于死锁诊断：当 pendingTaskCount > 0 但 runningTaskCount == 0 时，
     * 转储 m_runningRegions 和队列中任务的区域信息，判断是否区域互斥活锁。
     */
    void debugDumpState();

    /**
     * @brief 诊断：转储每个 worker 当前正在执行的任务（描述与已执行时长）
     *
     * 用于死锁定位：当 runningTaskCount > 0 但 pending 冻结时，转储各 worker 的执行任务，
     * 判断是否卡在 execute（生成器死循环/死锁）还是 onChunkGenComplete（调度锁等待）。
     */
    void debugDumpRunningTasks();

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
        std::shared_ptr<std::atomic<bool>> abortSignal;

        // 区域互斥信息（hasArea=false 时表示无区域互斥，可完全并行）
        bool hasArea = false;
        ChunkCoord areaCenterX = 0;
        ChunkCoord areaCenterZ = 0;
        i32 areaWriteRadius = 0;
    };

    /**
     * @brief 任务比较器（用于优先队列）
     *
     * 优先级小的在前，同优先级时间早的在前。
     */
    struct TaskComparator {
        bool operator()(const std::shared_ptr<InternalTask>& a, const std::shared_ptr<InternalTask>& b) const
        {
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
    // 区域互斥（对齐 Moonrise 区域锁执行器）
    // ============================================================================

    /**
     * @brief 将区块坐标打包为 64 位键（内部使用，不依赖 chunk 模块）
     *
     * 高 32 位为 X（转 u32），低 32 位为 Z（转 u32）。
     */
    [[nodiscard]] static u64 packChunkKey(ChunkCoord x, ChunkCoord z) noexcept;

    /**
     * @brief 检查任务的写入区域是否与正在执行的区域互斥任务冲突
     *
     * 调用者必须持有 m_runningRegionsMutex。
     *
     * @param task 待检查的任务（必须有区域信息）
     * @return true 表示冲突（不能执行），false 表示无冲突（可以执行）
     */
    [[nodiscard]] bool hasAreaConflictLocked(const InternalTask& task) const;

    /**
     * @brief 标记任务的写入区域为正在执行（加入 m_runningRegions）
     *
     * 调用者必须持有 m_runningRegionsMutex。
     */
    void markAreaRunningLocked(const InternalTask& task);

    /**
     * @brief 清除任务的写入区域标记（从 m_runningRegions 移除）
     *
     * 调用者必须持有 m_runningRegionsMutex。
     */
    void unmarkAreaRunningLocked(const InternalTask& task);

    /**
     * @brief 计算任务写入区域覆盖的所有区块键
     *
     * @param task 有区域信息的任务
     * @return 区块键集合（(2*writeRadius+1)² 个）
     */
    [[nodiscard]] static std::vector<u64> computeAreaKeys(const InternalTask& task);

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

    // 正在执行的任务数量
    std::atomic<size_t> m_runningTaskCount{0};

    // 诊断：正在执行的任务信息（用于死锁定位）。
    // m_runningTaskInfo 在 m_runningTaskMutex 下读写，记录每个 worker 当前执行任务的描述与开始时间。
    struct RunningTaskInfo {
        std::string description;
        std::chrono::steady_clock::time_point startTime;
        bool hasArea = false;
        ChunkCoord areaCenterX = 0;
        ChunkCoord areaCenterZ = 0;
        i32 areaWriteRadius = 0;
    };
    std::mutex m_runningTaskMutex;
    std::unordered_map<i32, RunningTaskInfo> m_runningTaskInfo; // key = workerId

    // 运行状态
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stop{false};

    // 等待完成的条件变量
    std::mutex m_completionMutex;
    std::condition_variable m_completionCondition;

    // ============================================================================
    // 区域互斥状态（对齐 Moonrise 区域锁执行器）
    // ============================================================================

    // 正在执行的区域互斥任务所占据的区块键集合。
    // 任务开始执行时把其写入区域（(2*writeRadius+1)² 个区块键）全部加入，
    // 执行完成时移除。无区域互斥的任务（hasArea=false）不参与此集合。
    //
    // 冲突判定：新任务的任一区块键已在集合中 → 冲突，必须等待。
    // 这保证两个写入区域重叠的任务不会同时执行（方块写入互斥），
    // 而区域不重叠的任务可完全并行。
    std::unordered_set<u64> m_runningRegions;
    mutable std::mutex m_runningRegionsMutex;

    // 区域任务释放时通知等待冲突的工作线程重试
    std::condition_variable m_areaReleasedCondition;
};

} // namespace mc::util
