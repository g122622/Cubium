#pragma once

#include "common/core/Types.hpp"
#include "common/util/concurrent/ReentrantAreaLock.hpp"
#include "common/world/chunk/ChunkStatus.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <unordered_map>

namespace mc {

// Forward declarations
class ChunkPrimer;
class IChunkGenerator;

namespace server {
class ServerWorld;
}

// ============================================================================
// 优先级
// ============================================================================

/**
 * @brief 任务优先级
 *
 * 参考 Moonrise 的 Priority 设计
 */
enum class Priority : i32 {
    BLOCKING = 0,       // 阻塞优先级（最高）
    HIGHEST = 1,
    HIGH = 2,
    NORMAL = 3,
    LOW = 4,
    LOWER = 5,
    LOWEST = 6,
    COUNT = 7
};

// ============================================================================
// ChunkStatus 配置
// ============================================================================

/**
 * @brief ChunkStatus 调度配置
 *
 * 定义每个 ChunkStatus 的调度行为
 */
struct ChunkStatusConfig {
    i32 writeRadius = 0;        // 写入半径（需要锁定的邻居范围）
    bool parallelCapable = false; // 是否可并行执行
    bool emptyLoadStatus = false; // 加载时是否为空操作
    bool emptyGenStatus = false;  // 生成时是否为空操作
};

// ============================================================================
// PrioritisedTask
// ============================================================================

/**
 * @brief 优先级任务
 */
class PrioritisedTask {
public:
    using TaskFunc = std::function<void()>;

    PrioritisedTask(TaskFunc func, Priority priority, i32 x, i32 z)
        : m_func(std::move(func))
        , m_priority(priority)
        , m_x(x)
        , m_z(z)
        , m_subOrder(s_nextSubOrder.fetch_add(1, std::memory_order_relaxed))
    {}

    void execute() {
        if (m_func) {
            m_func();
        }
    }

    [[nodiscard]] Priority getPriority() const { return m_priority; }
    [[nodiscard]] i32 getX() const { return m_x; }
    [[nodiscard]] i32 getZ() const { return m_z; }
    [[nodiscard]] u64 getSubOrder() const { return m_subOrder; }

    // 比较器：优先级高的先执行，同优先级按 subOrder 排序
    bool operator<(const PrioritisedTask& other) const {
        if (m_priority != other.m_priority) {
            return static_cast<i32>(m_priority) > static_cast<i32>(other.m_priority);
        }
        return m_subOrder > other.m_subOrder;
    }

private:
    TaskFunc m_func;
    Priority m_priority;
    i32 m_x;
    i32 m_z;
    u64 m_subOrder;

    static inline std::atomic<u64> s_nextSubOrder{0};
};

// ============================================================================
// PrioritisedTaskQueue
// ============================================================================

/**
 * @brief 优先级任务队列
 *
 * 线程安全的优先级队列，用于主线程执行器
 */
class PrioritisedTaskQueue {
public:
    PrioritisedTaskQueue() = default;

    void push(std::unique_ptr<PrioritisedTask> task) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(task));
        m_totalScheduled.fetch_add(1, std::memory_order_relaxed);
    }

    std::unique_ptr<PrioritisedTask> poll() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) {
            return nullptr;
        }
        auto task = std::move(const_cast<std::unique_ptr<PrioritisedTask>&>(m_queue.top()));
        m_queue.pop();
        m_totalExecuted.fetch_add(1, std::memory_order_relaxed);
        return task;
    }

    bool executeTask() {
        auto task = poll();
        if (task) {
            task->execute();
            return true;
        }
        return false;
    }

    [[nodiscard]] bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    [[nodiscard]] size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    [[nodiscard]] u64 getTotalTasksScheduled() const {
        return m_totalScheduled.load(std::memory_order_relaxed);
    }

    [[nodiscard]] u64 getTotalTasksExecuted() const {
        return m_totalExecuted.load(std::memory_order_relaxed);
    }

private:
    mutable std::mutex m_mutex;
    std::priority_queue<std::unique_ptr<PrioritisedTask>> m_queue;
    std::atomic<u64> m_totalScheduled{0};
    std::atomic<u64> m_totalExecuted{0};
};

// ============================================================================
// BalancedPrioritisedThreadPool
// ============================================================================

/**
 * @brief 平衡优先级线程池
 *
 * 参考 Moonrise 的 BalancedPrioritisedThreadPool
 */
class BalancedPrioritisedThreadPool {
public:
    class OrderedStreamGroup {
    public:
        /**
         * @brief 任务队列（共享队列的句柄）
         *
         * 推送到此队列的任务会进入线程池的共享队列。
         */
        class Queue {
        public:
            explicit Queue(BalancedPrioritisedThreadPool* pool = nullptr);
            ~Queue() = default;

            Queue(const Queue&) = delete;
            Queue& operator=(const Queue&) = delete;
            Queue(Queue&& other) noexcept;
            Queue& operator=(Queue&& other) noexcept;

            void push(std::unique_ptr<PrioritisedTask> task);
            std::unique_ptr<PrioritisedTask> poll();
            [[nodiscard]] bool empty() const;
            [[nodiscard]] size_t size() const;

        private:
            friend class BalancedPrioritisedThreadPool;

            BalancedPrioritisedThreadPool* m_pool;
            std::atomic<size_t> m_localSize{0};  // 本地计数器，用于跟踪推送到此队列的任务数
        };
    };

    explicit BalancedPrioritisedThreadPool(i32 threadCount, const String& name = "WorkerPool");
    ~BalancedPrioritisedThreadPool();

    void start();
    OrderedStreamGroup::Queue createExecutor();
    void shutdown();

    [[nodiscard]] i32 getThreadCount() const { return m_threadCount; }
    [[nodiscard]] bool isRunning() const { return m_running.load(std::memory_order_acquire); }

    /**
     * @brief 执行一个任务（用于主线程执行）
     * @return 是否执行了任务
     */
    bool executeOneTask();

private:
    void workerThread(i32 workerId);
    void notifyWorker();

    i32 m_threadCount;
    String m_name;
    std::vector<std::thread> m_workers;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stop{false};

    mutable std::mutex m_queueMutex;
    std::condition_variable m_condition;

    // 共享任务队列（工作线程从此拉取任务）
    std::priority_queue<std::unique_ptr<PrioritisedTask>> m_taskQueue;
    std::atomic<size_t> m_queueSize{0};
};

// ============================================================================
// AreaDependentQueue
// ============================================================================

/**
 * @brief 区域依赖队列
 *
 * 用于调度需要邻居区块访问权限的任务。
 * 任务会被延迟直到其依赖的区块可用。
 */
class AreaDependentQueue {
public:
    /**
     * @brief 区域依赖任务
     */
    struct AreaTask {
        std::function<void()> task;
        i32 centerX;
        i32 centerZ;
        i32 radius;
        Priority priority;
        u64 subOrder;

        bool operator<(const AreaTask& other) const {
            if (priority != other.priority) {
                return static_cast<i32>(priority) > static_cast<i32>(other.priority);
            }
            return subOrder > other.subOrder;
        }
    };

    explicit AreaDependentQueue(BalancedPrioritisedThreadPool::OrderedStreamGroup::Queue* executor, i32 chunkShift = 2);

    /**
     * @brief 添加区域依赖任务
     * @param task 任务函数
     * @param centerX 中心区块 X
     * @param centerZ 中心区块 Z
     * @param radius 需要的区块半径
     * @param priority 优先级
     */
    void addTask(std::function<void()> task, i32 centerX, i32 centerZ, i32 radius, Priority priority);

    /**
     * @brief 释放区块，触发依赖任务
     * @param x 区块 X
     * @param z 区块 Z
     */
    void releaseChunk(i32 x, i32 z);

    /**
     * @brief 检查是否有任务在等待指定区块
     */
    [[nodiscard]] bool hasTasksWaitingFor(i32 x, i32 z) const;

    /**
     * @brief 获取队列大小
     */
    [[nodiscard]] size_t size() const;

private:
    void tryExecuteTasks();

    BalancedPrioritisedThreadPool::OrderedStreamGroup::Queue* m_executor;
    i32 m_chunkShift;
    mutable std::mutex m_mutex;
    std::priority_queue<AreaTask> m_pendingTasks;
    std::unordered_map<u64, std::vector<AreaTask>> m_waitingByChunk;
    std::atomic<u64> m_nextSubOrder{0};
};

// ============================================================================
// ChunkTaskScheduler
// ============================================================================

/**
 * @brief 区块任务调度器
 *
 * 参考 Moonrise 的 ChunkTaskScheduler，负责：
 * 1. 管理多个执行器（并行生成、主线程、半径感知）
 * 2. 协调区块生成任务的调度
 * 3. 与 ThreadedTicketLevelPropagator 集成
 */
class ChunkTaskScheduler {
public:
    /**
     * @brief 构造函数
     * @param world 服务端世界
     * @param threadCount 线程数
     */
    ChunkTaskScheduler(server::ServerWorld& world, i32 threadCount = -1);
    ~ChunkTaskScheduler();

    // 禁止拷贝
    ChunkTaskScheduler(const ChunkTaskScheduler&) = delete;
    ChunkTaskScheduler& operator=(const ChunkTaskScheduler&) = delete;

    // ============================================================================
    // 生命周期
    // ============================================================================

    void start();
    void shutdown();

    // ============================================================================
    // 执行器访问
    // ============================================================================

    [[nodiscard]] BalancedPrioritisedThreadPool::OrderedStreamGroup::Queue* getParallelGenExecutor() { return &m_parallelGenExecutor; }
    [[nodiscard]] PrioritisedTaskQueue& getMainThreadExecutor() { return m_mainThreadExecutor; }
    [[nodiscard]] AreaDependentQueue& getRadiusAwareScheduler() { return *m_radiusAwareScheduler; }
    [[nodiscard]] concurrent::ReentrantAreaLock& getSchedulingLock() { return m_schedulingLock; }

    // ============================================================================
    // 调度方法
    // ============================================================================

    /**
     * @brief 调度区块任务
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @param task 任务函数
     * @param priority 优先级
     */
    void scheduleChunkTask(i32 x, i32 z, std::function<void()> task, Priority priority);

    /**
     * @brief 在主线程执行任务
     */
    void executeOnMainThread(std::function<void()> task, Priority priority = Priority::NORMAL);

    /**
     * @brief 执行主线程任务
     * @return 是否执行了任务
     */
    bool executeMainThreadTask();

    /**
     * @brief 执行所有最近排队的主线程任务
     */
    void executeAllRecentlyQueuedMainThreadTasks();

    // ============================================================================
    // 优先级操作
    // ============================================================================

    void raisePriority(i32 x, i32 z, Priority priority);
    void setPriority(i32 x, i32 z, Priority priority);
    void lowerPriority(i32 x, i32 z, Priority priority);

    // ============================================================================
    // ChunkStatus 访问半径
    // ============================================================================

    /**
     * @brief 获取 ChunkStatus 的访问半径
     */
    [[nodiscard]] static i32 getAccessRadius(const ChunkStatus& status);

    /**
     * @brief 获取最大访问半径
     */
    [[nodiscard]] static i32 getMaxAccessRadius();

    /**
     * @brief 获取 ChunkStatus 的写入半径
     */
    [[nodiscard]] static i32 getWriteRadius(const ChunkStatus& status);

    /**
     * @brief 检查 ChunkStatus 是否可并行执行
     */
    [[nodiscard]] static bool isParallelCapable(const ChunkStatus& status);

    // ============================================================================
    // 配置
    // ============================================================================

    [[nodiscard]] i32 getLockShift() const { return m_lockShift; }
    [[nodiscard]] server::ServerWorld& getWorld() { return m_world; }
    [[nodiscard]] bool hasShutdown() const { return m_shutdown.load(std::memory_order_acquire); }

private:
    server::ServerWorld& m_world;
    i32 m_threadCount;
    i32 m_lockShift;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_shutdown{false};

    // 线程池
    std::unique_ptr<BalancedPrioritisedThreadPool> m_threadPool;

    // 执行器（指向线程池的共享队列）
    BalancedPrioritisedThreadPool::OrderedStreamGroup::Queue m_parallelGenExecutor;
    std::unique_ptr<AreaDependentQueue> m_radiusAwareScheduler;
    PrioritisedTaskQueue m_mainThreadExecutor;

    // 区域锁
    concurrent::ReentrantAreaLock m_schedulingLock;

    // ChunkStatus 配置
    static std::array<ChunkStatusConfig, 13> s_statusConfigs;
    static i32 s_maxAccessRadius;
    static bool s_configsInitialized;

    static void initializeConfigs();
};

} // namespace mc
