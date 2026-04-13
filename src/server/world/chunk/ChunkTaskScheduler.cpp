#include "server/world/chunk/ChunkTaskScheduler.hpp"
#include "common/world/chunk/ChunkStatus.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <algorithm>
#include <cassert>

namespace mc {

// ============================================================================
// ChunkStatus 静态配置初始化
// ============================================================================

// ChunkStatus 枚举值参考 ChunkStatus.hpp:
// EMPTY = 0, STRUCTURE_STARTS = 1, STRUCTURE_REFERENCES = 2, BIOMES = 3,
// NOISE = 4, SURFACE = 5, CARVERS = 6, LIQUID_CARVERS = 7, FEATURES = 8,
// LIGHT = 9, SPAWN = 10, HEIGHTMAPS = 11, FULL = 12

std::array<ChunkStatusConfig, 13> ChunkTaskScheduler::s_statusConfigs = {};
i32 ChunkTaskScheduler::s_maxAccessRadius = 0;
bool ChunkTaskScheduler::s_configsInitialized = false;

void ChunkTaskScheduler::initializeConfigs() {
    if (s_configsInitialized) {
        return;
    }

    // EMPTY (0) - 空状态，无写入，可并行
    s_statusConfigs[0] = {0, true, true, true};

    // STRUCTURE_STARTS (1) - 结构起点，可并行
    s_statusConfigs[1] = {0, true, false, false};

    // STRUCTURE_REFERENCES (2) - 结构引用，可并行
    s_statusConfigs[2] = {0, true, false, false};

    // BIOMES (3) - 生物群系，可并行
    s_statusConfigs[3] = {0, true, false, false};

    // NOISE (4) - 噪声生成，可并行
    s_statusConfigs[4] = {0, true, false, false};

    // SURFACE (5) - 地表生成，可并行
    s_statusConfigs[5] = {0, true, false, false};

    // CARVERS (6) - 雕刻器，可并行
    s_statusConfigs[6] = {0, true, false, false};

    // LIQUID_CARVERS (7) - 液体雕刻器，可并行
    s_statusConfigs[7] = {0, true, false, false};

    // FEATURES (8) - 特性，需要写入半径 1，不可并行
    s_statusConfigs[8] = {1, false, false, false};

    // LIGHT (9) - 光照，需要写入半径 2，不可并行
    s_statusConfigs[9] = {2, false, false, false};

    // SPAWN (10) - 生成点，可并行
    s_statusConfigs[10] = {0, true, false, false};

    // HEIGHTMAPS (11) - 高度图，可并行
    s_statusConfigs[11] = {0, true, false, false};

    // FULL (12) - 完成，不可并行
    s_statusConfigs[12] = {0, false, false, false};

    // 计算最大访问半径
    s_maxAccessRadius = 0;
    for (const auto& config : s_statusConfigs) {
        s_maxAccessRadius = std::max(s_maxAccessRadius, config.writeRadius);
    }

    s_configsInitialized = true;
}

// ============================================================================
// BalancedPrioritisedThreadPool::OrderedStreamGroup::Queue
// ============================================================================

BalancedPrioritisedThreadPool::OrderedStreamGroup::Queue::Queue(BalancedPrioritisedThreadPool* pool)
    : m_pool(pool) {
}

BalancedPrioritisedThreadPool::OrderedStreamGroup::Queue::Queue(Queue&& other) noexcept
    : m_pool(other.m_pool)
    , m_localSize(other.m_localSize.load(std::memory_order_relaxed)) {
    other.m_pool = nullptr;
    other.m_localSize.store(0, std::memory_order_relaxed);
}

BalancedPrioritisedThreadPool::OrderedStreamGroup::Queue&
BalancedPrioritisedThreadPool::OrderedStreamGroup::Queue::operator=(Queue&& other) noexcept {
    if (this != &other) {
        m_pool = other.m_pool;
        m_localSize.store(other.m_localSize.load(std::memory_order_relaxed), std::memory_order_relaxed);

        other.m_pool = nullptr;
        other.m_localSize.store(0, std::memory_order_relaxed);
    }
    return *this;
}

void BalancedPrioritisedThreadPool::OrderedStreamGroup::Queue::push(std::unique_ptr<PrioritisedTask> task) {
    if (!m_pool) {
        return;  // 没有关联的线程池
    }

    // 推送到线程池的共享队列
    {
        std::lock_guard<std::mutex> lock(m_pool->m_queueMutex);
        m_pool->m_taskQueue.push(std::move(task));
        m_pool->m_queueSize.fetch_add(1, std::memory_order_relaxed);
    }
    m_localSize.fetch_add(1, std::memory_order_relaxed);

    m_pool->notifyWorker();
}

std::unique_ptr<PrioritisedTask> BalancedPrioritisedThreadPool::OrderedStreamGroup::Queue::poll() {
    if (!m_pool) {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(m_pool->m_queueMutex);
    if (m_pool->m_taskQueue.empty()) {
        return nullptr;
    }
    auto task = std::move(const_cast<std::unique_ptr<PrioritisedTask>&>(m_pool->m_taskQueue.top()));
    m_pool->m_taskQueue.pop();
    m_pool->m_queueSize.fetch_sub(1, std::memory_order_relaxed);
    if (m_localSize.load(std::memory_order_relaxed) > 0) {
        m_localSize.fetch_sub(1, std::memory_order_relaxed);
    }
    return task;
}

bool BalancedPrioritisedThreadPool::OrderedStreamGroup::Queue::empty() const {
    if (!m_pool) {
        return true;
    }
    std::lock_guard<std::mutex> lock(m_pool->m_queueMutex);
    return m_pool->m_taskQueue.empty();
}

size_t BalancedPrioritisedThreadPool::OrderedStreamGroup::Queue::size() const {
    return m_localSize.load(std::memory_order_relaxed);
}

// ============================================================================
// BalancedPrioritisedThreadPool
// ============================================================================

BalancedPrioritisedThreadPool::BalancedPrioritisedThreadPool(i32 threadCount, const String& name)
    : m_threadCount(threadCount)
    , m_name(name)
    , m_running(false)
    , m_stop(false) {
    if (m_threadCount <= 0) {
        // 默认使用硬件并发数
        m_threadCount = static_cast<i32>(std::thread::hardware_concurrency());
        if (m_threadCount <= 0) {
            m_threadCount = 4;
        }
    }
}

BalancedPrioritisedThreadPool::~BalancedPrioritisedThreadPool() {
    shutdown();
}

BalancedPrioritisedThreadPool::OrderedStreamGroup::Queue BalancedPrioritisedThreadPool::createExecutor() {
    return OrderedStreamGroup::Queue(this);
}

void BalancedPrioritisedThreadPool::start() {
    if (m_running.exchange(true, std::memory_order_acq_rel)) {
        return;  // 已经运行
    }

    m_stop.store(false, std::memory_order_release);

    // 创建工作线程
    for (i32 i = 0; i < m_threadCount; ++i) {
        m_workers.emplace_back(&BalancedPrioritisedThreadPool::workerThread, this, i);
    }
}

void BalancedPrioritisedThreadPool::shutdown() {
    if (!m_running.exchange(false, std::memory_order_acq_rel)) {
        return;
    }

    m_stop.store(true, std::memory_order_release);
    m_condition.notify_all();

    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    m_workers.clear();
}

void BalancedPrioritisedThreadPool::notifyWorker() {
    m_condition.notify_one();
}

bool BalancedPrioritisedThreadPool::executeOneTask() {
    std::unique_ptr<PrioritisedTask> task;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (m_taskQueue.empty()) {
            return false;
        }
        task = std::move(const_cast<std::unique_ptr<PrioritisedTask>&>(m_taskQueue.top()));
        m_taskQueue.pop();
        m_queueSize.fetch_sub(1, std::memory_order_relaxed);
    }

    if (task) {
        task->execute();
        return true;
    }
    return false;
}

void BalancedPrioritisedThreadPool::workerThread(i32 workerId) {
    MC_UNUSED(workerId);

    while (!m_stop.load(std::memory_order_acquire)) {
        // 尝试从共享队列获取任务
        std::unique_ptr<PrioritisedTask> task;
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (!m_taskQueue.empty()) {
                task = std::move(const_cast<std::unique_ptr<PrioritisedTask>&>(m_taskQueue.top()));
                m_taskQueue.pop();
                m_queueSize.fetch_sub(1, std::memory_order_relaxed);
            }
        }

        if (task) {
            task->execute();
            continue;
        }

        // 没有任务，等待通知或超时
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_condition.wait_for(lock, std::chrono::milliseconds(10), [this] {
                return m_stop.load(std::memory_order_acquire) || !m_taskQueue.empty();
            });
        }
    }
}

// ============================================================================
// AreaDependentQueue
// ============================================================================

AreaDependentQueue::AreaDependentQueue(BalancedPrioritisedThreadPool::OrderedStreamGroup::Queue* executor, i32 chunkShift)
    : m_executor(executor)
    , m_chunkShift(chunkShift) {
}

void AreaDependentQueue::addTask(std::function<void()> task, i32 centerX, i32 centerZ, i32 radius, Priority priority) {
    std::lock_guard<std::mutex> lock(m_mutex);

    AreaTask areaTask;
    areaTask.task = std::move(task);
    areaTask.centerX = centerX;
    areaTask.centerZ = centerZ;
    areaTask.radius = radius;
    areaTask.priority = priority;
    areaTask.subOrder = m_nextSubOrder.fetch_add(1, std::memory_order_relaxed);

    // 计算区域键
    i32 minChunkX = centerX - radius;
    i32 minChunkZ = centerZ - radius;
    i32 maxChunkX = centerX + radius;
    i32 maxChunkZ = centerZ + radius;

    i32 minSectionX = minChunkX >> m_chunkShift;
    i32 minSectionZ = minChunkZ >> m_chunkShift;
    i32 maxSectionX = maxChunkX >> m_chunkShift;
    i32 maxSectionZ = maxChunkZ >> m_chunkShift;

    // 收集所有依赖的区块键
    std::vector<u64> dependencies;

    for (i32 sectionZ = minSectionZ; sectionZ <= maxSectionZ; ++sectionZ) {
        for (i32 sectionX = minSectionX; sectionX <= maxSectionX; ++sectionX) {
            u64 key = (static_cast<u64>(static_cast<u32>(sectionX)) << 32) | static_cast<u32>(sectionZ);
            dependencies.push_back(key);
        }
    }

    // 检查所有依赖是否可用
    bool canExecute = true;
    for (u64 key : dependencies) {
        // 暂时简化：假设所有区块都可用
        // 实际实现需要检查区块持有者状态
        MC_UNUSED(key);
    }

    if (canExecute) {
        // 直接提交到执行器
        if (m_executor) {
            m_executor->push(std::make_unique<PrioritisedTask>(areaTask.task, priority, centerX, centerZ));
        }
    } else {
        // 添加到等待队列
        m_pendingTasks.push(areaTask);

        // 记录等待关系
        for (u64 key : dependencies) {
            m_waitingByChunk[key].push_back(areaTask);
        }
    }
}

void AreaDependentQueue::releaseChunk(i32 x, i32 z) {
    std::lock_guard<std::mutex> lock(m_mutex);

    u64 key = (static_cast<u64>(static_cast<u32>(x >> m_chunkShift)) << 32) |
              static_cast<u32>(z >> m_chunkShift);

    auto it = m_waitingByChunk.find(key);
    if (it == m_waitingByChunk.end()) {
        return;
    }

    // 移除等待映射
    m_waitingByChunk.erase(it);

    // 尝试执行任务
    tryExecuteTasks();
}

bool AreaDependentQueue::hasTasksWaitingFor(i32 x, i32 z) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    u64 key = (static_cast<u64>(static_cast<u32>(x >> m_chunkShift)) << 32) |
              static_cast<u32>(z >> m_chunkShift);

    return m_waitingByChunk.find(key) != m_waitingByChunk.end();
}

size_t AreaDependentQueue::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pendingTasks.size();
}

void AreaDependentQueue::tryExecuteTasks() {
    // 注意：调用者必须持有 m_mutex

    // 尝试执行等待中的任务
    std::vector<AreaTask> readyTasks;

    // 创建临时队列来检查任务
    std::priority_queue<AreaTask> tempQueue;
    while (!m_pendingTasks.empty()) {
        auto task = std::move(const_cast<AreaTask&>(m_pendingTasks.top()));
        m_pendingTasks.pop();

        // 检查任务是否可以执行
        i32 minSectionX = (task.centerX - task.radius) >> m_chunkShift;
        i32 minSectionZ = (task.centerZ - task.radius) >> m_chunkShift;
        i32 maxSectionX = (task.centerX + task.radius) >> m_chunkShift;
        i32 maxSectionZ = (task.centerZ + task.radius) >> m_chunkShift;

        bool canExecute = true;
        for (i32 sectionZ = minSectionZ; sectionZ <= maxSectionZ && canExecute; ++sectionZ) {
            for (i32 sectionX = minSectionX; sectionX <= maxSectionX && canExecute; ++sectionX) {
                u64 key = (static_cast<u64>(static_cast<u32>(sectionX)) << 32) |
                          static_cast<u32>(sectionZ);
                if (m_waitingByChunk.find(key) != m_waitingByChunk.end()) {
                    canExecute = false;
                }
            }
        }

        if (canExecute) {
            readyTasks.push_back(std::move(task));
        } else {
            tempQueue.push(std::move(task));
        }
    }

    // 恢复未就绪的任务
    m_pendingTasks = std::move(tempQueue);

    // 执行就绪的任务
    for (const auto& task : readyTasks) {
        if (m_executor) {
            m_executor->push(std::make_unique<PrioritisedTask>(task.task, task.priority, task.centerX, task.centerZ));
        }
    }
}

// ============================================================================
// ChunkTaskScheduler
// ============================================================================

ChunkTaskScheduler::ChunkTaskScheduler(server::ServerWorld& world, i32 threadCount)
    : m_world(world)
    , m_threadCount(threadCount)
    , m_lockShift(6)  // 64x64 区块为一个区域
    , m_schedulingLock(m_lockShift) {
    initializeConfigs();
}

ChunkTaskScheduler::~ChunkTaskScheduler() {
    shutdown();
}

void ChunkTaskScheduler::start() {
    if (m_running.exchange(true, std::memory_order_acq_rel)) {
        return;  // 已经运行
    }

    m_shutdown.store(false, std::memory_order_release);

    // 创建线程池
    m_threadPool = std::make_unique<BalancedPrioritisedThreadPool>(m_threadCount, "ChunkWorker");

    // 启动线程池
    m_threadPool->start();

    // 获取共享执行器（任务推送到此队列，工作线程从此队列拉取）
    m_parallelGenExecutor = m_threadPool->createExecutor();

    // 创建半径感知调度器
    m_radiusAwareScheduler = std::make_unique<AreaDependentQueue>(&m_parallelGenExecutor, m_lockShift);
}

void ChunkTaskScheduler::shutdown() {
    if (!m_running.exchange(false, std::memory_order_acq_rel)) {
        return;  // 已经关闭
    }

    m_shutdown.store(true, std::memory_order_release);

    // 关闭线程池
    if (m_threadPool) {
        m_threadPool->shutdown();
    }

    m_threadPool.reset();
    m_radiusAwareScheduler.reset();
}

void ChunkTaskScheduler::scheduleChunkTask(i32 x, i32 z, std::function<void()> task, Priority priority) {
    if (m_shutdown.load(std::memory_order_acquire)) {
        // 调度器已关闭，静默返回
        return;
    }

    if (!task) {
        MC_ASSERT_RELEASE_MSG(false, "Cannot schedule null task");
        return;
    }

    m_parallelGenExecutor.push(std::make_unique<PrioritisedTask>(std::move(task), priority, x, z));
}

void ChunkTaskScheduler::executeOnMainThread(std::function<void()> task, Priority priority) {
    m_mainThreadExecutor.push(std::make_unique<PrioritisedTask>(std::move(task), priority, 0, 0));
}

bool ChunkTaskScheduler::executeMainThreadTask() {
    return m_mainThreadExecutor.executeTask();
}

void ChunkTaskScheduler::executeAllRecentlyQueuedMainThreadTasks() {
    u64 scheduled = m_mainThreadExecutor.getTotalTasksScheduled();
    u64 executed = m_mainThreadExecutor.getTotalTasksExecuted();

    // 执行最近排队的任务
    while (executed < scheduled) {
        if (!m_mainThreadExecutor.executeTask()) {
            break;
        }
        executed = m_mainThreadExecutor.getTotalTasksExecuted();
    }
}

void ChunkTaskScheduler::raisePriority(i32 x, i32 z, Priority priority) {
    // 暂时简化实现
    // 完整实现需要追踪任务并提升优先级
    MC_UNUSED(x);
    MC_UNUSED(z);
    MC_UNUSED(priority);
}

void ChunkTaskScheduler::setPriority(i32 x, i32 z, Priority priority) {
    // 暂时简化实现
    // 完整实现需要追踪任务并设置优先级
    MC_UNUSED(x);
    MC_UNUSED(z);
    MC_UNUSED(priority);
}

void ChunkTaskScheduler::lowerPriority(i32 x, i32 z, Priority priority) {
    // 暂时简化实现
    // 完整实现需要追踪任务并降低优先级
    MC_UNUSED(x);
    MC_UNUSED(z);
    MC_UNUSED(priority);
}

i32 ChunkTaskScheduler::getAccessRadius(const ChunkStatus& status) {
    initializeConfigs();

    i32 index = status.ordinal();
    if (index < 0 || index >= static_cast<i32>(s_statusConfigs.size())) {
        return 0;
    }

    return s_statusConfigs[index].writeRadius;
}

i32 ChunkTaskScheduler::getMaxAccessRadius() {
    initializeConfigs();
    return s_maxAccessRadius;
}

i32 ChunkTaskScheduler::getWriteRadius(const ChunkStatus& status) {
    initializeConfigs();

    i32 index = status.ordinal();
    if (index < 0 || index >= static_cast<i32>(s_statusConfigs.size())) {
        return 0;
    }

    return s_statusConfigs[index].writeRadius;
}

bool ChunkTaskScheduler::isParallelCapable(const ChunkStatus& status) {
    initializeConfigs();

    i32 index = status.ordinal();
    if (index < 0 || index >= static_cast<i32>(s_statusConfigs.size())) {
        return false;
    }

    return s_statusConfigs[index].parallelCapable;
}

} // namespace mc
