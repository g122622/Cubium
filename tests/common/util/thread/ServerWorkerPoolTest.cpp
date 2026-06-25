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

#include "common/util/thread/ServerWorkerPool.hpp"
#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::util;

// ============================================================================
// 测试任务类
// ============================================================================

/**
 * @brief 简单测试任务
 */
class SimpleTestTask : public ITask {
public:
    explicit SimpleTestTask(int value)
        : m_value(value)
        , m_executed(false)
    {}

    bool execute(const std::atomic<bool>& abortSignal) override
    {
        if (abortSignal.load(std::memory_order_acquire)) {
            return false;
        }
        m_executed = true;
        return true;
    }

    TaskType type() const override { return TaskType::Custom; }
    std::string description() const override { return "SimpleTestTask(" + std::to_string(m_value) + ")"; }

    [[nodiscard]] bool wasExecuted() const { return m_executed; }
    [[nodiscard]] int value() const { return m_value; }

private:
    int m_value;
    bool m_executed;
};

/**
 * @brief 延迟任务（用于测试优先级顺序）
 */
class DelayedTask : public ITask {
public:
    DelayedTask(int value, std::vector<int>& executionOrder, std::mutex& mutex)
        : m_value(value)
        , m_executionOrder(executionOrder)
        , m_mutex(mutex)
    {}

    bool execute(const std::atomic<bool>& abortSignal) override
    {
        if (abortSignal.load(std::memory_order_acquire)) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::lock_guard<std::mutex> lock(m_mutex);
        m_executionOrder.push_back(m_value);
        return true;
    }

    TaskType type() const override { return TaskType::Custom; }
    std::string description() const override { return "DelayedTask(" + std::to_string(m_value) + ")"; }

private:
    int m_value;
    std::vector<int>& m_executionOrder;
    std::mutex& m_mutex;
};

/**
 * @brief 抛出异常的任务
 */
class ThrowingTask : public ITask {
public:
    bool execute(const std::atomic<bool>& abortSignal) override
    {
        (void)abortSignal;
        throw std::runtime_error("Test exception");
    }

    TaskType type() const override { return TaskType::Custom; }
    std::string description() const override { return "ThrowingTask"; }
};

// ============================================================================
// ServerWorkerPool 测试固件
// ============================================================================

class ServerWorkerPoolTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// 构造和生命周期测试
// ============================================================================

TEST_F(ServerWorkerPoolTest, DefaultConstructor)
{
    ServerWorkerPool pool;
    EXPECT_FALSE(pool.isRunning());
    EXPECT_GT(pool.threadCount(), 0); // 自动检测线程数
}

TEST_F(ServerWorkerPoolTest, CustomThreadCount)
{
    ServerWorkerPool pool(4, "TestWorker");
    EXPECT_FALSE(pool.isRunning());
    EXPECT_EQ(pool.threadCount(), 4);
}

TEST_F(ServerWorkerPoolTest, StartStop)
{
    ServerWorkerPool pool(2, "TestWorker");

    EXPECT_FALSE(pool.isRunning());

    pool.start();
    EXPECT_TRUE(pool.isRunning());

    pool.shutdown();
    EXPECT_FALSE(pool.isRunning());
}

TEST_F(ServerWorkerPoolTest, DoubleStart)
{
    ServerWorkerPool pool(2, "TestWorker");

    pool.start();
    EXPECT_TRUE(pool.isRunning());

    // 重复启动应该无效
    pool.start();
    EXPECT_TRUE(pool.isRunning());

    pool.shutdown();
}

TEST_F(ServerWorkerPoolTest, DoubleShutdown)
{
    ServerWorkerPool pool(2, "TestWorker");

    pool.start();
    pool.shutdown();
    EXPECT_FALSE(pool.isRunning());

    // 重复关闭应该安全
    pool.shutdown();
    EXPECT_FALSE(pool.isRunning());
}

// ============================================================================
// 任务提交测试
// ============================================================================

TEST_F(ServerWorkerPoolTest, SubmitSimpleTask)
{
    ServerWorkerPool pool(2, "TestWorker");
    pool.start();

    std::atomic<bool> completed{false};
    std::atomic<bool> success{false};

    auto task = std::make_unique<SimpleTestTask>(42);
    pool.submit(
        std::move(task),
        [&](bool s, ITask* task) {
            completed = true;
            success = s;
        },
        TaskPriority::Normal);

    // 等待完成
    for (int i = 0; i < 100 && !completed; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(completed);
    EXPECT_TRUE(success);

    pool.shutdown();
}

TEST_F(ServerWorkerPoolTest, SubmitMultipleTasks)
{
    ServerWorkerPool pool(4, "TestWorker");
    pool.start();

    std::atomic<int> completedCount{0};
    const int numTasks = 10;

    for (int i = 0; i < numTasks; ++i) {
        auto task = std::make_unique<SimpleTestTask>(i);
        pool.submit(std::move(task), [&completedCount](bool, ITask*) { completedCount++; }, TaskPriority::Normal);
    }

    // 等待所有完成
    for (int i = 0; i < 200 && completedCount < numTasks; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(completedCount, numTasks);
    EXPECT_EQ(pool.pendingTaskCount(), 0);

    pool.shutdown();
}

TEST_F(ServerWorkerPoolTest, SubmitTaskWhenNotRunning)
{
    ServerWorkerPool pool(2, "TestWorker");
    // 不启动

    std::atomic<bool> completed{false};
    std::atomic<bool> success{true};

    auto task = std::make_unique<SimpleTestTask>(42);
    pool.submit(
        std::move(task),
        [&](bool s, ITask*) {
            completed = true;
            success = s;
        },
        TaskPriority::Normal);

    // 任务池未启动，回调会立即被调用（返回失败）
    EXPECT_TRUE(completed);
    EXPECT_FALSE(success);

    // 启动后应该能执行新任务
    completed = false;
    success = false;
    pool.start();

    auto task2 = std::make_unique<SimpleTestTask>(43);
    pool.submit(
        std::move(task2),
        [&](bool s, ITask*) {
            completed = true;
            success = s;
        },
        TaskPriority::Normal);

    for (int i = 0; i < 100 && !completed; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(completed);
    EXPECT_TRUE(success);

    pool.shutdown();
}

// ============================================================================
// 优先级测试
// ============================================================================

TEST_F(ServerWorkerPoolTest, PriorityOrdering)
{
    ServerWorkerPool pool(1, "TestWorker"); // 单线程确保顺序执行
    pool.start();

    std::vector<int> executionOrder;
    std::mutex orderMutex;

    // 等待第一个任务开始执行，确保后续任务进入队列
    std::atomic<bool> firstStarted{false};
    auto firstTask = std::make_unique<DelayedTask>(0, executionOrder, orderMutex);

    // 提交任务，优先级不同（数值越小优先级越高）
    pool.submit(std::move(firstTask), [&](bool, ITask*) { firstStarted = true; }, TaskPriority::Normal);

    // 等待第一个任务开始
    for (int i = 0; i < 100 && !firstStarted; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 提交三个任务，优先级不同
    pool.submit(std::make_unique<DelayedTask>(3, executionOrder, orderMutex), nullptr, TaskPriority::Low);  // 低优先级
    pool.submit(std::make_unique<DelayedTask>(1, executionOrder, orderMutex), nullptr, TaskPriority::High); // 高优先级
    pool.submit(
        std::make_unique<DelayedTask>(2, executionOrder, orderMutex), nullptr, TaskPriority::Normal); // 普通优先级

    // 等待完成
    pool.waitForCompletion();

    // 第一个任务（值=0）先执行，然后是高优先级任务（值=1）
    ASSERT_GE(executionOrder.size(), 4);
    EXPECT_EQ(executionOrder[0], 0); // 第一个任务
    EXPECT_EQ(executionOrder[1], 1); // 高优先级第二个执行
}

TEST_F(ServerWorkerPoolTest, CriticalPriorityHighest)
{
    ServerWorkerPool pool(1, "TestWorker");
    pool.start();

    std::vector<int> executionOrder;
    std::mutex orderMutex;

    std::atomic<bool> firstStarted{false};
    pool.submit(
        std::make_unique<DelayedTask>(0, executionOrder, orderMutex),
        [&](bool, ITask*) { firstStarted = true; },
        TaskPriority::Normal);

    // 等待第一个任务开始
    for (int i = 0; i < 100 && !firstStarted; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Critical 应该比 High 更优先
    pool.submit(std::make_unique<DelayedTask>(2, executionOrder, orderMutex), nullptr, TaskPriority::High);
    pool.submit(std::make_unique<DelayedTask>(1, executionOrder, orderMutex), nullptr, TaskPriority::Critical);

    pool.waitForCompletion();

    // 第一个任务（值=0）先执行，然后是 Critical 任务（值=1）
    ASSERT_GE(executionOrder.size(), 3);
    EXPECT_EQ(executionOrder[0], 0); // 第一个任务
    EXPECT_EQ(executionOrder[1], 1); // Critical 第二个执行
}

// ============================================================================
// 取消测试
// ============================================================================

TEST_F(ServerWorkerPoolTest, AbortSignalSkipsExecution)
{
    ServerWorkerPool pool(1, "TestWorker");
    pool.start();

    auto abortSignal = std::make_shared<std::atomic<bool>>(true);
    std::atomic<bool> completed{false};
    std::atomic<bool> success{true};

    auto task = std::make_unique<SimpleTestTask>(42);
    pool.submit(
        std::move(task),
        [&](bool s, ITask*) {
            completed = true;
            success = s;
        },
        TaskPriority::Normal,
        abortSignal);

    for (int i = 0; i < 100 && !completed; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(completed);
    EXPECT_FALSE(success); // 应该因取消而失败

    pool.shutdown();
}

TEST_F(ServerWorkerPoolTest, PruneCancelledTasks)
{
    ServerWorkerPool pool(1, "TestWorker");
    pool.start();

    std::atomic<bool> firstStarted{false};
    pool.submit(std::make_unique<SimpleTestTask>(0), [&](bool, ITask*) { firstStarted = true; }, TaskPriority::Normal);

    // 等待第一个任务开始
    for (int i = 0; i < 100 && !firstStarted; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto abortSignal = std::make_shared<std::atomic<bool>>(true);
    pool.submit(std::make_unique<SimpleTestTask>(1), nullptr, TaskPriority::Low, abortSignal);

    // 裁剪已取消的任务
    pool.pruneCancelledTasks();

    // 队列应该为空或接近空
    EXPECT_LE(pool.pendingTaskCount(), 1u);

    pool.shutdown();
}

// ============================================================================
// 异常处理测试
// ============================================================================

TEST_F(ServerWorkerPoolTest, TaskException)
{
    ServerWorkerPool pool(2, "TestWorker");
    pool.start();

    std::atomic<bool> completed{false};
    std::atomic<bool> success{true};

    auto task = std::make_unique<ThrowingTask>();
    pool.submit(
        std::move(task),
        [&](bool s, ITask*) {
            completed = true;
            success = s;
        },
        TaskPriority::Normal);

    for (int i = 0; i < 100 && !completed; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(completed);
    EXPECT_FALSE(success); // 异常导致失败

    pool.shutdown();
}

// ============================================================================
// 统计测试
// ============================================================================

TEST_F(ServerWorkerPoolTest, PendingTaskCount)
{
    ServerWorkerPool pool(1, "TestWorker"); // 单线程
    pool.start();

    std::atomic<bool> firstStarted{false};
    pool.submit(
        std::make_unique<SimpleTestTask>(0),
        [&](bool, ITask*) {
            firstStarted = true;
            // 保持第一个任务运行一段时间
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        },
        TaskPriority::Normal);

    // 等待第一个任务开始
    for (int i = 0; i < 100 && !firstStarted; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 提交更多任务
    for (int i = 1; i < 5; ++i) {
        pool.submit(std::make_unique<SimpleTestTask>(i), nullptr, TaskPriority::Normal);
    }

    // 应该有待处理任务（第一个任务正在执行，其余在队列中）
    EXPECT_GT(pool.pendingTaskCount(), 0);

    pool.shutdown();
    EXPECT_EQ(pool.pendingTaskCount(), 0);
}

TEST_F(ServerWorkerPoolTest, RunningTaskCount)
{
    ServerWorkerPool pool(4, "TestWorker");
    pool.start();

    std::atomic<int> runningCount{0};
    std::atomic<int> maxRunning{0};
    std::mutex mtx;

    for (int i = 0; i < 8; ++i) {
        auto task = std::make_unique<SimpleTestTask>(i);
        pool.submit(
            std::move(task),
            [&](bool, ITask*) {
                runningCount++;
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    maxRunning = std::max(maxRunning.load(), runningCount.load());
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                runningCount--;
            },
            TaskPriority::Normal);
    }

    pool.waitForCompletion();

    // 由于线程池有4个线程，最大并发应该在1-4之间
    EXPECT_GE(maxRunning, 1);
    EXPECT_LE(maxRunning, 4);

    pool.shutdown();
}

// ============================================================================
// 等待完成测试
// ============================================================================

TEST_F(ServerWorkerPoolTest, WaitForCompletion)
{
    ServerWorkerPool pool(4, "TestWorker");
    pool.start();

    std::atomic<int> completedCount{0};
    const int numTasks = 20;

    for (int i = 0; i < numTasks; ++i) {
        auto task = std::make_unique<SimpleTestTask>(i);
        pool.submit(std::move(task), [&completedCount](bool, ITask*) { completedCount++; }, TaskPriority::Normal);
    }

    pool.waitForCompletion();

    EXPECT_EQ(completedCount, numTasks);
    EXPECT_EQ(pool.pendingTaskCount(), 0);
    EXPECT_EQ(pool.runningTaskCount(), 0);

    pool.shutdown();
}

// ============================================================================
// 线程安全测试
// ============================================================================

TEST_F(ServerWorkerPoolTest, ConcurrentSubmissions)
{
    ServerWorkerPool pool(4, "TestWorker");
    pool.start();

    std::atomic<int> completedCount{0};
    std::vector<std::thread> threads;

    // 多个线程同时提交任务
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&pool, &completedCount, t]() {
            for (int i = 0; i < 10; ++i) {
                auto task = std::make_unique<SimpleTestTask>(t * 10 + i);
                pool.submit(std::move(task), [&completedCount](bool, ITask*) { completedCount++; });
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    pool.waitForCompletion();

    EXPECT_EQ(completedCount, 40);
    pool.shutdown();
}

TEST_F(ServerWorkerPoolTest, ShutdownWhileTasksRunning)
{
    ServerWorkerPool pool(4, "TestWorker");
    pool.start();

    std::atomic<int> completedCount{0};

    // 提交一些任务
    for (int i = 0; i < 10; ++i) {
        auto task = std::make_unique<SimpleTestTask>(i);
        pool.submit(std::move(task), [&completedCount](bool, ITask*) { completedCount++; });
    }

    // 立即关闭（不等待完成）
    pool.shutdown();

    // shutdown() 应该等待正在执行的任务完成
    EXPECT_EQ(completedCount, 10);
}

// ============================================================================
// 任务类型和描述测试
// ============================================================================

TEST_F(ServerWorkerPoolTest, TaskTypeAndDescription)
{
    ServerWorkerPool pool(2, "TestWorker");
    pool.start();

    std::atomic<bool> completed{false};
    TaskType capturedType = TaskType::Custom;
    std::string capturedDescription;

    auto task = std::make_unique<SimpleTestTask>(42);
    pool.submit(std::move(task), [&](bool, ITask* t) {
        EXPECT_NE(t, nullptr);
        if (t) {
            capturedType = t->type();
            capturedDescription = t->description();
        }
        completed = true;
    });

    for (int i = 0; i < 100 && !completed; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(completed);
    EXPECT_EQ(capturedType, TaskType::Custom);
    EXPECT_EQ(capturedDescription, "SimpleTestTask(42)");

    pool.shutdown();
}
