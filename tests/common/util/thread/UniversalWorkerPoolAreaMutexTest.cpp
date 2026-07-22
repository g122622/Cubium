/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to furnished copies of the following:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

// ============================================================================
// UniversalWorkerPool 区域互斥（area mutex）单元测试
//
// 验证带 (centerX, centerZ, writeRadius) 的 submit 重载：
//   - 同区域任务串行执行（写入区域重叠时不并发）
//   - 不同区域任务并行执行（写入区域不重叠时可并发）
//   - canExecuteNow 正确反映区域占用状态
//   - 无区域任务不受区域互斥影响（可与区域任务并行）
//   - 区域释放后被阻塞的任务恢复执行
//
// 对齐 Moonrise 区域锁执行器：FEATURES/LIGHT/SPAWN/FULL 等写方块状态的任务
// 通过区域互斥提交，避免并发生成时同一区块被两个任务同时写入。
// ============================================================================

#include "common/util/thread/ITask.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using namespace mc;
using namespace mc::util;

namespace {

// 共享并发计数器：多个任务共用，追踪同时执行的最大任务数
struct ConcurrencyTracker {
    std::atomic<int> concurrent{0};
    std::atomic<int> maxConcurrent{0};
    std::mutex mutex;

    void enter()
    {
        std::lock_guard<std::mutex> lock(mutex);
        int cur = ++concurrent;
        if (cur > maxConcurrent.load(std::memory_order::relaxed)) {
            maxConcurrent.store(cur, std::memory_order::relaxed);
        }
    }

    void leave()
    {
        std::lock_guard<std::mutex> lock(mutex);
        --concurrent;
    }
};

// ============================================================================
// 测试任务类
// ============================================================================

// 带信号灯的区域任务：execute 期间阻塞，直到 release() 被调用。
// 用于精确控制任务的重叠窗口，验证区域互斥。
// 进入时 ++concurrent，离开时 --concurrent，调用方通过 tracker.maxConcurrent 判断是否并发。
class BlockingAreaTask : public ITask {
public:
    BlockingAreaTask(ConcurrencyTracker& tracker, std::atomic<bool>& entered)
        : m_tracker(tracker)
        , m_entered(entered)
    {}

    bool execute(const std::atomic<bool>& abortSignal) override
    {
        if (abortSignal.load(std::memory_order::acquire)) {
            return false;
        }
        m_tracker.enter();
        m_entered.store(true);
        m_entered.notify_one(); // 通知测试线程任务已进入执行

        // 阻塞直到 release() 被调用
        m_released.wait(false);

        m_tracker.leave();
        m_completed.store(true);
        m_completed.notify_one();
        return true;
    }

    void release()
    {
        m_released.store(true);
        m_released.notify_one();
    }

    // 等待任务完成（release 后）
    void waitCompleted() { m_completed.wait(false); }

    // 等待任务进入执行
    void waitEntered() { m_entered.wait(false); }

    TaskType type() const override { return TaskType::Custom; }
    std::string description() const override { return "BlockingAreaTask"; }

private:
    ConcurrencyTracker& m_tracker;
    std::atomic<bool>& m_entered; // 引用外部的 entered 标志
    std::atomic<bool> m_released{false};
    std::atomic<bool> m_completed{false};
};

// 简单计数任务：execute 时递增计数并短暂休眠，用于验证无区域任务的并行性
class CountingTask : public ITask {
public:
    CountingTask(ConcurrencyTracker& tracker, std::atomic<int>& counter, std::chrono::milliseconds duration)
        : m_tracker(tracker)
        , m_counter(counter)
        , m_duration(duration)
    {}

    bool execute(const std::atomic<bool>& abortSignal) override
    {
        if (abortSignal.load(std::memory_order::acquire)) {
            return false;
        }
        m_tracker.enter();
        ++m_counter;
        std::this_thread::sleep_for(m_duration);
        m_tracker.leave();
        return true;
    }

    TaskType type() const override { return TaskType::Custom; }
    std::string description() const override { return "CountingTask"; }

private:
    ConcurrencyTracker& m_tracker;
    std::atomic<int>& m_counter;
    std::chrono::milliseconds m_duration;
};

// 辅助：等待 tracker.maxConcurrent 达到期望值或超时
bool waitForMaxConcurrent(ConcurrencyTracker& tracker, int expected, std::chrono::milliseconds timeout)
{
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (tracker.maxConcurrent.load(std::memory_order::relaxed) >= expected) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return tracker.maxConcurrent.load(std::memory_order::relaxed) >= expected;
}

} // namespace

// ============================================================================
// canExecuteNow 基础测试
// ============================================================================

TEST(UniversalWorkerPoolAreaMutexTest, CanExecuteNowEmptyPoolReturnsTrue)
{
    UniversalWorkerPool pool(2, "AreaMutexTest", 900);
    pool.start();

    // 空池时任何区域都应可执行
    EXPECT_TRUE(pool.canExecuteNow(0, 0, 0));
    EXPECT_TRUE(pool.canExecuteNow(5, 5, 2));
    EXPECT_TRUE(pool.canExecuteNow(-10, -10, 3));

    pool.shutdown();
}

TEST(UniversalWorkerPoolAreaMutexTest, CanExecuteNowNegativeRadiusTreatedAsZero)
{
    UniversalWorkerPool pool(2, "AreaMutexTest", 900);
    pool.start();

    // writeRadius < 0 视作 0（仅中心区块）
    EXPECT_TRUE(pool.canExecuteNow(0, 0, -1));
    EXPECT_TRUE(pool.canExecuteNow(0, 0, -5));

    pool.shutdown();
}

// ============================================================================
// 同区域任务串行执行（写入区域重叠 → 不并发）
// ============================================================================

TEST(UniversalWorkerPoolAreaMutexTest, OverlappingAreaTasksAreSerialized)
{
    // 2 个工作线程，2 个区域重叠任务（都覆盖 (0,0)）
    // 区域互斥生效时 maxConcurrent == 1（串行）；失效时 == 2（并行，这是 bug）
    UniversalWorkerPool pool(2, "AreaMutexTest", 900);
    pool.start();

    ConcurrencyTracker tracker;
    std::atomic<bool> aEntered{false};
    std::atomic<bool> bEntered{false};

    // 任务 A：中心 (0,0)，writeRadius=1，覆盖 [-1,1]×[-1,1]
    auto taskA = std::make_unique<BlockingAreaTask>(tracker, aEntered);
    auto* taskAPtr = taskA.get();

    // 任务 B：中心 (1,0)，writeRadius=1，覆盖 [0,2]×[-1,1] —— 与 A 在 (0,0)/(1,0) 重叠
    auto taskB = std::make_unique<BlockingAreaTask>(tracker, bEntered);
    auto* taskBPtr = taskB.get();

    pool.submit(std::move(taskA), nullptr, 0, 0, 1, TaskPriority::Normal);
    pool.submit(std::move(taskB), nullptr, 1, 0, 1, TaskPriority::Normal);

    // 等待 A 进入执行
    taskAPtr->waitEntered();
    // 给调度器时间尝试执行 B（B 应因区域冲突被阻塞）
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // A 在执行中，B 应被阻塞，maxConcurrent == 1
    EXPECT_EQ(tracker.maxConcurrent.load(), 1);
    EXPECT_FALSE(bEntered.load()); // B 未进入执行

    // 释放 A，B 应开始执行
    taskAPtr->release();
    taskAPtr->waitCompleted();

    // 等待 B 进入执行
    bEntered.wait(false);
    EXPECT_TRUE(bEntered.load());
    EXPECT_EQ(tracker.maxConcurrent.load(), 1); // B 独自执行，仍为 1

    taskBPtr->release();
    taskBPtr->waitCompleted();

    pool.waitForCompletion();
    pool.shutdown();

    // 核心断言：重叠区域任务从不并发执行
    EXPECT_EQ(tracker.maxConcurrent.load(), 1);
}

// ============================================================================
// 不同区域任务并行执行（写入区域不重叠 → 可并发）
// ============================================================================

TEST(UniversalWorkerPoolAreaMutexTest, NonOverlappingAreaTasksAreParallel)
{
    // 2 个工作线程，2 个区域不重叠任务
    // 区域互斥不应阻止并行，maxConcurrent == 2
    UniversalWorkerPool pool(2, "AreaMutexTest", 900);
    pool.start();

    ConcurrencyTracker tracker;
    std::atomic<bool> aEntered{false};
    std::atomic<bool> bEntered{false};

    // 任务 A：中心 (0,0)，writeRadius=1，覆盖 [-1,1]×[-1,1]
    auto taskA = std::make_unique<BlockingAreaTask>(tracker, aEntered);
    auto* taskAPtr = taskA.get();

    // 任务 B：中心 (100,100)，writeRadius=1，覆盖 [99,101]×[99,101] —— 与 A 完全不重叠
    auto taskB = std::make_unique<BlockingAreaTask>(tracker, bEntered);
    auto* taskBPtr = taskB.get();

    pool.submit(std::move(taskA), nullptr, 0, 0, 1, TaskPriority::Normal);
    pool.submit(std::move(taskB), nullptr, 100, 100, 1, TaskPriority::Normal);

    // 等待两个任务都进入执行
    taskAPtr->waitEntered();
    taskBPtr->waitEntered();

    // 不重叠区域应允许并行，maxConcurrent == 2
    EXPECT_EQ(tracker.maxConcurrent.load(), 2);

    taskAPtr->release();
    taskBPtr->release();
    taskAPtr->waitCompleted();
    taskBPtr->waitCompleted();

    pool.waitForCompletion();
    pool.shutdown();

    EXPECT_EQ(tracker.maxConcurrent.load(), 2);
}

// ============================================================================
// writeRadius=0 任务：仅中心区块，相邻区块不冲突
// ============================================================================

TEST(UniversalWorkerPoolAreaMutexTest, ZeroRadiusAdjacentChunksParallel)
{
    // writeRadius=0 的两个任务，中心分别为 (0,0) 和 (1,0)
    // 区域不重叠（各只占一个区块），应可并行
    UniversalWorkerPool pool(2, "AreaMutexTest", 900);
    pool.start();

    ConcurrencyTracker tracker;
    std::atomic<bool> aEntered{false};
    std::atomic<bool> bEntered{false};

    auto taskA = std::make_unique<BlockingAreaTask>(tracker, aEntered);
    auto* taskAPtr = taskA.get();
    auto taskB = std::make_unique<BlockingAreaTask>(tracker, bEntered);
    auto* taskBPtr = taskB.get();

    pool.submit(std::move(taskA), nullptr, 0, 0, 0, TaskPriority::Normal);
    pool.submit(std::move(taskB), nullptr, 1, 0, 0, TaskPriority::Normal);

    taskAPtr->waitEntered();
    taskBPtr->waitEntered();
    EXPECT_EQ(tracker.maxConcurrent.load(), 2);

    taskAPtr->release();
    taskBPtr->release();
    taskAPtr->waitCompleted();
    taskBPtr->waitCompleted();

    pool.waitForCompletion();
    pool.shutdown();

    EXPECT_EQ(tracker.maxConcurrent.load(), 2);
}

TEST(UniversalWorkerPoolAreaMutexTest, ZeroRadiusSameChunkSerialized)
{
    // writeRadius=0 的两个任务，中心都是 (0,0) —— 区域完全重叠，应串行
    UniversalWorkerPool pool(2, "AreaMutexTest", 900);
    pool.start();

    ConcurrencyTracker tracker;
    std::atomic<bool> aEntered{false};
    std::atomic<bool> bEntered{false};

    auto taskA = std::make_unique<BlockingAreaTask>(tracker, aEntered);
    auto* taskAPtr = taskA.get();
    auto taskB = std::make_unique<BlockingAreaTask>(tracker, bEntered);
    auto* taskBPtr = taskB.get();

    pool.submit(std::move(taskA), nullptr, 0, 0, 0, TaskPriority::Normal);
    pool.submit(std::move(taskB), nullptr, 0, 0, 0, TaskPriority::Normal);

    taskAPtr->waitEntered();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(tracker.maxConcurrent.load(), 1);
    EXPECT_FALSE(bEntered.load());

    taskAPtr->release();
    taskAPtr->waitCompleted();

    bEntered.wait(false);
    EXPECT_TRUE(bEntered.load());
    EXPECT_EQ(tracker.maxConcurrent.load(), 1);

    taskBPtr->release();
    taskBPtr->waitCompleted();

    pool.waitForCompletion();
    pool.shutdown();

    EXPECT_EQ(tracker.maxConcurrent.load(), 1);
}

// ============================================================================
// 无区域任务不受区域互斥影响（可与区域任务并行）
// ============================================================================

TEST(UniversalWorkerPoolAreaMutexTest, NonAreaTaskDoesNotParticipateInAreaMutex)
{
    // 无区域任务（普通 submit）不进入 m_runningRegions，不影响 canExecuteNow，
    // 也不被区域任务阻塞。应可与区域任务并行执行。
    UniversalWorkerPool pool(2, "AreaMutexTest", 900);
    pool.start();

    ConcurrencyTracker tracker;
    std::atomic<bool> aEntered{false};
    std::atomic<int> bCounter{0};

    // 区域任务 A 占据 (0,0) 区域
    auto taskA = std::make_unique<BlockingAreaTask>(tracker, aEntered);
    auto* taskAPtr = taskA.get();

    // 无区域任务 B（普通 submit，不带坐标）—— 不应被 A 阻塞
    auto taskB = std::make_unique<CountingTask>(tracker, bCounter, std::chrono::milliseconds(30));

    pool.submit(std::move(taskA), nullptr, 0, 0, 1, TaskPriority::Normal);
    pool.submit(std::move(taskB), nullptr, TaskPriority::Normal);

    taskAPtr->waitEntered();
    // B 应与 A 并行执行（无区域互斥），maxConcurrent 达到 2
    bool reached2 = waitForMaxConcurrent(tracker, 2, std::chrono::milliseconds(200));
    EXPECT_TRUE(reached2) << "无区域任务应与区域任务并行，maxConcurrent 应达到 2";

    taskAPtr->release();
    taskAPtr->waitCompleted();

    pool.waitForCompletion();
    pool.shutdown();

    EXPECT_EQ(bCounter.load(), 1);
    EXPECT_EQ(tracker.maxConcurrent.load(), 2);
}

// ============================================================================
// 区域释放后被阻塞的任务恢复执行
// ============================================================================

TEST(UniversalWorkerPoolAreaMutexTest, BlockedTaskResumesAfterAreaReleased)
{
    // A 占据 (0,0) 区域，B 等待（同区域），A 释放后 B 恢复执行
    UniversalWorkerPool pool(2, "AreaMutexTest", 900);
    pool.start();

    ConcurrencyTracker tracker;
    std::atomic<bool> aEntered{false};
    std::atomic<bool> bEntered{false};

    auto taskA = std::make_unique<BlockingAreaTask>(tracker, aEntered);
    auto* taskAPtr = taskA.get();
    auto taskB = std::make_unique<BlockingAreaTask>(tracker, bEntered);
    auto* taskBPtr = taskB.get();

    pool.submit(std::move(taskA), nullptr, 0, 0, 1, TaskPriority::Normal);
    pool.submit(std::move(taskB), nullptr, 0, 0, 1, TaskPriority::Normal);

    // A 进入执行，B 被阻塞
    taskAPtr->waitEntered();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(bEntered.load());

    // 释放 A，B 应恢复执行
    taskAPtr->release();
    taskAPtr->waitCompleted();

    // B 应进入执行（区域已释放）
    bEntered.wait(false);
    EXPECT_TRUE(bEntered.load());

    taskBPtr->release();
    taskBPtr->waitCompleted();

    pool.waitForCompletion();
    pool.shutdown();

    // 重叠区域任务从不并发
    EXPECT_EQ(tracker.maxConcurrent.load(), 1);
}

// ============================================================================
// 多个任务排队等待同一区域，全部完成（无死锁）
// ============================================================================

TEST(UniversalWorkerPoolAreaMutexTest, MultipleTasksSameAreaAllComplete)
{
    // 4 个工作线程，3 个同区域任务 —— 即使有 4 个线程，同区域任务也应串行，
    // 但最终全部完成（验证区域互斥不会导致任务饿死或死锁）。
    UniversalWorkerPool pool(4, "AreaMutexTest", 900);
    pool.start();

    std::atomic<int> completedCount{0};

    // 短任务：进入→短暂休眠→完成，递增 completedCount
    class ShortAreaTask : public ITask {
    public:
        ShortAreaTask(std::atomic<int>& counter)
            : m_counter(counter)
        {}

        bool execute(const std::atomic<bool>& abortSignal) override
        {
            if (abortSignal.load(std::memory_order::acquire)) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            ++m_counter;
            return true;
        }

        TaskType type() const override { return TaskType::Custom; }
        std::string description() const override { return "ShortAreaTask"; }

    private:
        std::atomic<int>& m_counter;
    };

    // 三个任务同区域（中心都是 (0,0)，writeRadius=1）
    for (int i = 0; i < 3; ++i) {
        pool.submit(std::make_unique<ShortAreaTask>(completedCount), nullptr, 0, 0, 1, TaskPriority::Normal);
    }

    // 等待全部完成（最多 2 秒）
    for (int i = 0; i < 200 && completedCount.load() < 3; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(completedCount.load(), 3);

    pool.shutdown();
}
