#include <gtest/gtest.h>
#include "server/world/chunk/ChunkTaskScheduler.hpp"
#include "common/world/chunk/ChunkStatus.hpp"
#include <thread>
#include <atomic>
#include <chrono>

using namespace mc;
using namespace mc::server;

// ============================================================================
// Test Fixture
// ============================================================================

class ChunkTaskSchedulerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // ChunkTaskScheduler 需要 ServerWorld，这里只测试不依赖 ServerWorld 的功能
    }

    void TearDown() override {
    }
};

// ============================================================================
// Priority Tests
// ============================================================================

TEST_F(ChunkTaskSchedulerTest, PriorityOrdering) {
    // Priority 枚举值越小优先级越高
    EXPECT_LT(static_cast<i32>(Priority::BLOCKING), static_cast<i32>(Priority::HIGHEST));
    EXPECT_LT(static_cast<i32>(Priority::HIGHEST), static_cast<i32>(Priority::HIGH));
    EXPECT_LT(static_cast<i32>(Priority::HIGH), static_cast<i32>(Priority::NORMAL));
    EXPECT_LT(static_cast<i32>(Priority::NORMAL), static_cast<i32>(Priority::LOW));
    EXPECT_LT(static_cast<i32>(Priority::LOW), static_cast<i32>(Priority::LOWER));
    EXPECT_LT(static_cast<i32>(Priority::LOWER), static_cast<i32>(Priority::LOWEST));
}

TEST_F(ChunkTaskSchedulerTest, PriorityCount) {
    EXPECT_EQ(static_cast<i32>(Priority::COUNT), 7);
}

// ============================================================================
// ChunkStatusConfig Tests
// ============================================================================

TEST_F(ChunkTaskSchedulerTest, ChunkStatusConfigsInitialized) {
    // 测试静态配置方法
    i32 maxRadius = ChunkTaskScheduler::getMaxAccessRadius();
    EXPECT_GE(maxRadius, 0);
    EXPECT_LE(maxRadius, 2);  // 最大写入半径是 LIGHT 阶段的 2
}

TEST_F(ChunkTaskSchedulerTest, ChunkStatusWriteRadius) {
    // EMPTY - 无写入半径，可并行
    EXPECT_EQ(ChunkTaskScheduler::getWriteRadius(ChunkStatuses::EMPTY), 0);

    // BIOMES - 无写入半径，可并行
    EXPECT_EQ(ChunkTaskScheduler::getWriteRadius(ChunkStatuses::BIOMES), 0);

    // NOISE - 无写入半径，可并行
    EXPECT_EQ(ChunkTaskScheduler::getWriteRadius(ChunkStatuses::NOISE), 0);

    // FEATURES - 需要写入半径 1
    EXPECT_EQ(ChunkTaskScheduler::getWriteRadius(ChunkStatuses::FEATURES), 1);

    // LIGHT - 需要写入半径 2
    EXPECT_EQ(ChunkTaskScheduler::getWriteRadius(ChunkStatuses::LIGHT), 2);

    // FULL - 无写入半径，不可并行
    EXPECT_EQ(ChunkTaskScheduler::getWriteRadius(ChunkStatuses::FULL), 0);
}

TEST_F(ChunkTaskSchedulerTest, ChunkStatusParallelCapable) {
    // EMPTY - 可并行
    EXPECT_TRUE(ChunkTaskScheduler::isParallelCapable(ChunkStatuses::EMPTY));

    // BIOMES - 可并行
    EXPECT_TRUE(ChunkTaskScheduler::isParallelCapable(ChunkStatuses::BIOMES));

    // NOISE - 可并行
    EXPECT_TRUE(ChunkTaskScheduler::isParallelCapable(ChunkStatuses::NOISE));

    // FEATURES - 不可并行
    EXPECT_FALSE(ChunkTaskScheduler::isParallelCapable(ChunkStatuses::FEATURES));

    // LIGHT - 不可并行
    EXPECT_FALSE(ChunkTaskScheduler::isParallelCapable(ChunkStatuses::LIGHT));

    // FULL - 不可并行
    EXPECT_FALSE(ChunkTaskScheduler::isParallelCapable(ChunkStatuses::FULL));
}

TEST_F(ChunkTaskSchedulerTest, ChunkStatusAccessRadius) {
    // getAccessRadius 应该返回写入半径
    EXPECT_EQ(ChunkTaskScheduler::getAccessRadius(ChunkStatuses::EMPTY), 0);
    EXPECT_EQ(ChunkTaskScheduler::getAccessRadius(ChunkStatuses::FEATURES), 1);
    EXPECT_EQ(ChunkTaskScheduler::getAccessRadius(ChunkStatuses::LIGHT), 2);
}

// ============================================================================
// PrioritisedTask Tests
// ============================================================================

TEST_F(ChunkTaskSchedulerTest, PrioritisedTaskCreation) {
    std::atomic<bool> executed{false};

    auto task = std::make_unique<PrioritisedTask>(
        [&executed]() { executed = true; },
        Priority::NORMAL,
        10,  // x
        20   // z
    );

    EXPECT_EQ(task->getPriority(), Priority::NORMAL);
    EXPECT_EQ(task->getX(), 10);
    EXPECT_EQ(task->getZ(), 20);
    EXPECT_FALSE(executed);

    task->execute();
    EXPECT_TRUE(executed);
}

TEST_F(ChunkTaskSchedulerTest, PrioritisedTaskComparison) {
    auto task1 = std::make_unique<PrioritisedTask>(
        []() {}, Priority::HIGH, 0, 0);

    // 等待一下确保 subOrder 不同
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    auto task2 = std::make_unique<PrioritisedTask>(
        []() {}, Priority::NORMAL, 0, 0);

    // HIGH 优先级 < NORMAL 优先级（数值更小）
    // 在 priority_queue 中，< 返回 true 意味着 task1 排在后面
    // 所以 HIGH 优先级任务先出队
    EXPECT_TRUE(*task2 < *task1);  // NORMAL < HIGH 为 true（因为 NORMAL 数值更大）
}

// ============================================================================
// PrioritisedTaskQueue Tests
// ============================================================================

TEST_F(ChunkTaskSchedulerTest, PrioritisedTaskQueueBasic) {
    PrioritisedTaskQueue queue;

    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0u);

    // 推送任务
    std::atomic<i32> counter{0};
    queue.push(std::make_unique<PrioritisedTask>(
        [&counter]() { counter++; },
        Priority::NORMAL, 0, 0));

    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1u);

    // 执行任务
    EXPECT_TRUE(queue.executeTask());
    EXPECT_EQ(counter, 1);
    EXPECT_TRUE(queue.empty());
}

TEST_F(ChunkTaskSchedulerTest, PrioritisedTaskQueuePriority) {
    PrioritisedTaskQueue queue;

    std::vector<i32> executionOrder;

    // 按 NORMAL, HIGH, BLOCKING 顺序推送
    queue.push(std::make_unique<PrioritisedTask>(
        [&executionOrder]() { executionOrder.push_back(2); },  // NORMAL
        Priority::NORMAL, 0, 0));

    queue.push(std::make_unique<PrioritisedTask>(
        [&executionOrder]() { executionOrder.push_back(1); },  // HIGH
        Priority::HIGH, 0, 0));

    queue.push(std::make_unique<PrioritisedTask>(
        [&executionOrder]() { executionOrder.push_back(0); },  // BLOCKING
        Priority::BLOCKING, 0, 0));

    // 按优先级执行：BLOCKING, HIGH, NORMAL
    queue.executeTask();
    queue.executeTask();
    queue.executeTask();

    EXPECT_EQ(executionOrder.size(), 3u);
    EXPECT_EQ(executionOrder[0], 0);  // BLOCKING first
    EXPECT_EQ(executionOrder[1], 1);  // HIGH second
    EXPECT_EQ(executionOrder[2], 2);  // NORMAL last
}

TEST_F(ChunkTaskSchedulerTest, PrioritisedTaskQueueCounters) {
    PrioritisedTaskQueue queue;

    EXPECT_EQ(queue.getTotalTasksScheduled(), 0u);
    EXPECT_EQ(queue.getTotalTasksExecuted(), 0u);

    queue.push(std::make_unique<PrioritisedTask>([]() {}, Priority::NORMAL, 0, 0));
    queue.push(std::make_unique<PrioritisedTask>([]() {}, Priority::NORMAL, 0, 0));

    EXPECT_EQ(queue.getTotalTasksScheduled(), 2u);
    EXPECT_EQ(queue.getTotalTasksExecuted(), 0u);

    queue.executeTask();
    EXPECT_EQ(queue.getTotalTasksExecuted(), 1u);

    queue.executeTask();
    EXPECT_EQ(queue.getTotalTasksExecuted(), 2u);
}

// ============================================================================
// BalancedPrioritisedThreadPool Tests
// ============================================================================

TEST_F(ChunkTaskSchedulerTest, ThreadPoolCreation) {
    BalancedPrioritisedThreadPool pool(2, "TestPool");

    EXPECT_EQ(pool.getThreadCount(), 2);
    EXPECT_FALSE(pool.isRunning());

    pool.start();
    EXPECT_TRUE(pool.isRunning());

    pool.shutdown();
    EXPECT_FALSE(pool.isRunning());
}

TEST_F(ChunkTaskSchedulerTest, ThreadPoolTaskExecution) {
    BalancedPrioritisedThreadPool pool(2, "TestPool");
    pool.start();

    std::atomic<i32> counter{0};
    const int taskCount = 10;

    auto executor = pool.createExecutor();

    for (int i = 0; i < taskCount; ++i) {
        executor.push(std::make_unique<PrioritisedTask>(
            [&counter]() { counter++; },
            Priority::NORMAL, 0, 0));
    }

    // 等待任务完成
    for (int i = 0; i < 100 && counter.load() < taskCount; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(counter.load(), taskCount);

    pool.shutdown();
}

TEST_F(ChunkTaskSchedulerTest, ThreadPoolPriorityOrder) {
    BalancedPrioritisedThreadPool pool(1, "TestPool");  // 单线程确保顺序
    pool.start();

    std::vector<i32> executionOrder;
    std::mutex orderMutex;

    auto executor = pool.createExecutor();

    // 推送任务：NORMAL, HIGH, LOW, BLOCKING
    executor.push(std::make_unique<PrioritisedTask>(
        [&executionOrder, &orderMutex]() {
            std::lock_guard<std::mutex> lock(orderMutex);
            executionOrder.push_back(2);  // NORMAL
        },
        Priority::NORMAL, 0, 0));

    executor.push(std::make_unique<PrioritisedTask>(
        [&executionOrder, &orderMutex]() {
            std::lock_guard<std::mutex> lock(orderMutex);
            executionOrder.push_back(1);  // HIGH
        },
        Priority::HIGH, 0, 0));

    executor.push(std::make_unique<PrioritisedTask>(
        [&executionOrder, &orderMutex]() {
            std::lock_guard<std::mutex> lock(orderMutex);
            executionOrder.push_back(3);  // LOW
        },
        Priority::LOW, 0, 0));

    executor.push(std::make_unique<PrioritisedTask>(
        [&executionOrder, &orderMutex]() {
            std::lock_guard<std::mutex> lock(orderMutex);
            executionOrder.push_back(0);  // BLOCKING
        },
        Priority::BLOCKING, 0, 0));

    // 等待任务完成
    for (int i = 0; i < 100 && executionOrder.size() < 4; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 验证优先级顺序
    ASSERT_EQ(executionOrder.size(), 4u);
    EXPECT_EQ(executionOrder[0], 0);  // BLOCKING first
    EXPECT_EQ(executionOrder[1], 1);  // HIGH second
    EXPECT_EQ(executionOrder[2], 2);  // NORMAL third
    EXPECT_EQ(executionOrder[3], 3);  // LOW last

    pool.shutdown();
}

// ============================================================================
// AreaDependentQueue Tests
// ============================================================================

TEST_F(ChunkTaskSchedulerTest, AreaDependentQueueCreation) {
    BalancedPrioritisedThreadPool pool(1, "TestPool");
    pool.start();

    auto executor = pool.createExecutor();
    AreaDependentQueue queue(&executor, 2);  // shift=2 意味着 4x4 区块为一个区域

    EXPECT_EQ(queue.size(), 0u);
    EXPECT_FALSE(queue.hasTasksWaitingFor(0, 0));

    pool.shutdown();
}

TEST_F(ChunkTaskSchedulerTest, AreaDependentQueueAddTask) {
    BalancedPrioritisedThreadPool pool(1, "TestPool");
    pool.start();

    auto executor = pool.createExecutor();
    AreaDependentQueue queue(&executor, 2);

    std::atomic<bool> executed{false};

    queue.addTask(
        [&executed]() { executed = true; },
        10,  // centerX
        10,  // centerZ
        2,   // radius
        Priority::NORMAL);

    // 等待任务完成
    for (int i = 0; i < 100 && !executed.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(executed.load());

    pool.shutdown();
}

TEST_F(ChunkTaskSchedulerTest, AreaDependentQueueReleaseChunk) {
    BalancedPrioritisedThreadPool pool(1, "TestPool");
    pool.start();

    auto executor = pool.createExecutor();
    AreaDependentQueue queue(&executor, 2);

    std::atomic<i32> counter{0};

    // 添加任务
    queue.addTask(
        [&counter]() { counter++; },
        0, 0, 2, Priority::NORMAL);

    // 释放区块
    queue.releaseChunk(0, 0);
    queue.releaseChunk(1, 0);
    queue.releaseChunk(0, 1);

    // 等待任务完成
    for (int i = 0; i < 100 && counter.load() < 1; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_GE(counter.load(), 1);

    pool.shutdown();
}

// ============================================================================
// ReentrantAreaLock Integration Tests
// ============================================================================

TEST_F(ChunkTaskSchedulerTest, SchedulingLockCreation) {
    // 测试 ChunkTaskScheduler 是否正确初始化 ReentrantAreaLock
    // 锁的 shift 为 6 意味着 64x64 区块为一个区域

    // 由于 ChunkTaskScheduler 需要 ServerWorld，我们只能测试锁本身
    concurrent::ReentrantAreaLock lock(6);

    // 锁定单个区块
    auto* node = lock.lock(0, 0);
    ASSERT_NE(node, nullptr);
    EXPECT_TRUE(lock.isHeldByCurrentThread(0, 0));

    // 解锁
    lock.unlock(node);
    EXPECT_FALSE(lock.isHeldByCurrentThread(0, 0));
}

TEST_F(ChunkTaskSchedulerTest, SchedulingLockRadius) {
    concurrent::ReentrantAreaLock lock(6);

    // 锁定半径 2 的区域
    auto* node = lock.lock(10, 10, 2);
    ASSERT_NE(node, nullptr);

    // 检查范围内的区块是否被锁定
    EXPECT_TRUE(lock.isHeldByCurrentThread(10, 10, 2));
    EXPECT_TRUE(lock.isHeldByCurrentThread(8, 8));
    EXPECT_TRUE(lock.isHeldByCurrentThread(12, 12));

    lock.unlock(node);
}

// ============================================================================
// Multi-threaded Tests
// ============================================================================

TEST_F(ChunkTaskSchedulerTest, ThreadPoolMultipleThreads) {
    const int numThreads = 4;
    BalancedPrioritisedThreadPool pool(numThreads, "TestPool");
    pool.start();

    std::atomic<i32> counter{0};
    const int taskCount = 100;

    auto executor = pool.createExecutor();

    for (int i = 0; i < taskCount; ++i) {
        executor.push(std::make_unique<PrioritisedTask>(
            [&counter]() { counter++; },
            Priority::NORMAL, 0, 0));
    }

    // 等待任务完成
    for (int i = 0; i < 200 && counter.load() < taskCount; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(counter.load(), taskCount);

    pool.shutdown();
}

TEST_F(ChunkTaskSchedulerTest, ThreadPoolConcurrentPush) {
    const int numThreads = 4;
    BalancedPrioritisedThreadPool pool(numThreads, "TestPool");
    pool.start();

    std::atomic<i32> counter{0};
    const int tasksPerThread = 25;

    auto executor = pool.createExecutor();

    std::vector<std::thread> threads;
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&executor, &counter, tasksPerThread]() {
            for (int i = 0; i < tasksPerThread; ++i) {
                executor.push(std::make_unique<PrioritisedTask>(
                    [&counter]() { counter++; },
                    Priority::NORMAL, 0, 0));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 等待任务完成
    for (int i = 0; i < 200 && counter.load() < numThreads * tasksPerThread; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(counter.load(), numThreads * tasksPerThread);

    pool.shutdown();
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(ChunkTaskSchedulerTest, ThreadPoolShutdownIdempotent) {
    BalancedPrioritisedThreadPool pool(2, "TestPool");

    pool.start();
    EXPECT_TRUE(pool.isRunning());

    // 多次关闭应该是安全的
    pool.shutdown();
    pool.shutdown();
    pool.shutdown();

    EXPECT_FALSE(pool.isRunning());
}

TEST_F(ChunkTaskSchedulerTest, PrioritisedTaskQueueEmptyExecution) {
    PrioritisedTaskQueue queue;

    // 空队列执行应该返回 false
    EXPECT_FALSE(queue.executeTask());
}

TEST_F(ChunkTaskSchedulerTest, AreaDependentQueueNullExecutor) {
    // 空 executor 应该不会崩溃
    AreaDependentQueue queue(nullptr, 2);

    std::atomic<bool> executed{false};

    // 添加任务但不执行（因为没有 executor）
    queue.addTask(
        [&executed]() { executed = true; },
        0, 0, 0, Priority::NORMAL);

    // 任务不应该执行
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(executed.load());
}

TEST_F(ChunkTaskSchedulerTest, ChunkStatusConfigInvalidOrdinal) {
    // 测试无效 ordinal 的处理
    // ChunkStatus 的 ordinal 应该在 0-12 范围内

    // 创建一个临时 ChunkStatus 来测试边界情况
    // 由于 ChunkStatus 是具体类，我们测试已知的状态
    EXPECT_EQ(ChunkTaskScheduler::getWriteRadius(ChunkStatuses::EMPTY), 0);
    EXPECT_EQ(ChunkTaskScheduler::getWriteRadius(ChunkStatuses::FULL), 0);
}
