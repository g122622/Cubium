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
// 区域互斥的用途：FEATURES/LIGHT/SPAWN/FULL 等会写方块状态的生成步骤经带坐标的
// submit 重载提交，避免并发生成时同一区块被两个任务同时写入。
//
// 【注意】submit 会把任务对象的所有权转移给任务池，池在任务执行完毕（含完成回调）后立即
// 销毁该对象。因此测试不得把任务对象的裸指针当作同步句柄在提交后访问（详见 TaskSignals）。
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
// 测试任务类与同步信号
// ============================================================================

// 任务同步信号：由测试线程持有，任务只持引用。
//
// 【重要】同步状态不能放在任务对象自身：submit 会把任务对象的所有权转移给任务池，
// 池在任务执行完毕（含完成回调）后立即销毁该对象。若测试保留任务对象的裸指针并在提交后
// 访问（waitEntered/release/waitCompleted），就会在池已释放该对象之后读到已回收的内存：
// 多数情况下残留值恰好正确而侥幸通过，一旦该块内存被复用（并发负载下极易发生）就会
// 永久等待，表现为 CTest 超时。故同步量必须放在独立于任务对象、生命周期由测试掌控的
// 对象里。
struct TaskSignals {
    std::atomic<bool> entered{false};
    std::atomic<bool> released{false};
    std::atomic<bool> completed{false};

    // 等待任务进入执行
    void waitEntered() { entered.wait(false); }

    // 允许任务继续执行
    void release()
    {
        released.store(true, std::memory_order::release);
        released.notify_one();
    }

    // 等待任务执行结束
    void waitCompleted() { completed.wait(false); }
};

// 带信号灯的区域任务：execute 期间阻塞，直到 signals.release() 被调用。
// 用于精确控制任务的重叠窗口，验证区域互斥。
// 进入时 ++concurrent，离开时 --concurrent，调用方通过 tracker.maxConcurrent 判断是否并发。
class BlockingAreaTask : public ITask {
public:
    BlockingAreaTask(ConcurrencyTracker& tracker, TaskSignals& signals)
        : m_tracker(tracker)
        , m_signals(signals)
    {}

    bool execute(const std::atomic<bool>& abortSignal) override
    {
        if (abortSignal.load(std::memory_order::acquire)) {
            return false;
        }
        m_tracker.enter();
        m_signals.entered.store(true, std::memory_order::release);
        m_signals.entered.notify_one(); // 通知测试线程任务已进入执行

        // 阻塞直到 release() 被调用
        m_signals.released.wait(false);

        m_tracker.leave();
        m_signals.completed.store(true, std::memory_order::release);
        m_signals.completed.notify_one();
        return true;
    }

    TaskType type() const override { return TaskType::Custom; }
    std::string description() const override { return "BlockingAreaTask"; }

private:
    ConcurrencyTracker& m_tracker;
    TaskSignals& m_signals;
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
    TaskSignals signalsA;
    TaskSignals signalsB;

    // 任务 A：中心 (0,0)，writeRadius=1，覆盖 [-1,1]×[-1,1]
    // 任务 B：中心 (1,0)，writeRadius=1，覆盖 [0,2]×[-1,1] —— 与 A 在 (0,0)/(1,0) 重叠
    pool.submit(std::make_unique<BlockingAreaTask>(tracker, signalsA), nullptr, 0, 0, 1, TaskPriority::Normal);
    pool.submit(std::make_unique<BlockingAreaTask>(tracker, signalsB), nullptr, 1, 0, 1, TaskPriority::Normal);

    // 等待 A 进入执行
    signalsA.waitEntered();
    // 给调度器时间尝试执行 B（B 应因区域冲突被阻塞）
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // A 在执行中，B 应被阻塞，maxConcurrent == 1
    EXPECT_EQ(tracker.maxConcurrent.load(), 1);
    EXPECT_FALSE(signalsB.entered.load()); // B 未进入执行

    // 释放 A，B 应开始执行
    signalsA.release();
    signalsA.waitCompleted();

    // 等待 B 进入执行
    signalsB.waitEntered();
    EXPECT_TRUE(signalsB.entered.load());
    EXPECT_EQ(tracker.maxConcurrent.load(), 1); // B 独自执行，仍为 1

    signalsB.release();
    signalsB.waitCompleted();

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
    TaskSignals signalsA;
    TaskSignals signalsB;

    // 任务 A：中心 (0,0)，writeRadius=1，覆盖 [-1,1]×[-1,1]
    // 任务 B：中心 (100,100)，writeRadius=1，覆盖 [99,101]×[99,101] —— 与 A 完全不重叠
    pool.submit(std::make_unique<BlockingAreaTask>(tracker, signalsA), nullptr, 0, 0, 1, TaskPriority::Normal);
    pool.submit(std::make_unique<BlockingAreaTask>(tracker, signalsB), nullptr, 100, 100, 1, TaskPriority::Normal);

    // 等待两个任务都进入执行
    signalsA.waitEntered();
    signalsB.waitEntered();

    // 不重叠区域应允许并行，maxConcurrent == 2
    EXPECT_EQ(tracker.maxConcurrent.load(), 2);

    signalsA.release();
    signalsB.release();
    signalsA.waitCompleted();
    signalsB.waitCompleted();

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
    TaskSignals signalsA;
    TaskSignals signalsB;

    pool.submit(std::make_unique<BlockingAreaTask>(tracker, signalsA), nullptr, 0, 0, 0, TaskPriority::Normal);
    pool.submit(std::make_unique<BlockingAreaTask>(tracker, signalsB), nullptr, 1, 0, 0, TaskPriority::Normal);

    signalsA.waitEntered();
    signalsB.waitEntered();
    EXPECT_EQ(tracker.maxConcurrent.load(), 2);

    signalsA.release();
    signalsB.release();
    signalsA.waitCompleted();
    signalsB.waitCompleted();

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
    TaskSignals signalsA;
    TaskSignals signalsB;

    pool.submit(std::make_unique<BlockingAreaTask>(tracker, signalsA), nullptr, 0, 0, 0, TaskPriority::Normal);
    pool.submit(std::make_unique<BlockingAreaTask>(tracker, signalsB), nullptr, 0, 0, 0, TaskPriority::Normal);

    signalsA.waitEntered();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_EQ(tracker.maxConcurrent.load(), 1);
    EXPECT_FALSE(signalsB.entered.load());

    signalsA.release();
    signalsA.waitCompleted();

    signalsB.waitEntered();
    EXPECT_TRUE(signalsB.entered.load());
    EXPECT_EQ(tracker.maxConcurrent.load(), 1);

    signalsB.release();
    signalsB.waitCompleted();

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
    TaskSignals signalsA;
    std::atomic<int> bCounter{0};

    // 区域任务 A 占据 (0,0) 区域
    pool.submit(std::make_unique<BlockingAreaTask>(tracker, signalsA), nullptr, 0, 0, 1, TaskPriority::Normal);

    // 无区域任务 B（普通 submit，不带坐标）—— 不应被 A 阻塞
    pool.submit(std::make_unique<CountingTask>(tracker, bCounter, std::chrono::milliseconds(30)),
        nullptr,
        TaskPriority::Normal);

    signalsA.waitEntered();
    // B 应与 A 并行执行（无区域互斥），maxConcurrent 达到 2
    bool reached2 = waitForMaxConcurrent(tracker, 2, std::chrono::milliseconds(200));
    EXPECT_TRUE(reached2) << "无区域任务应与区域任务并行，maxConcurrent 应达到 2";

    signalsA.release();
    signalsA.waitCompleted();

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
    TaskSignals signalsA;
    TaskSignals signalsB;

    pool.submit(std::make_unique<BlockingAreaTask>(tracker, signalsA), nullptr, 0, 0, 1, TaskPriority::Normal);
    pool.submit(std::make_unique<BlockingAreaTask>(tracker, signalsB), nullptr, 0, 0, 1, TaskPriority::Normal);

    // A 进入执行，B 被阻塞
    signalsA.waitEntered();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(signalsB.entered.load());

    // 释放 A，B 应恢复执行
    signalsA.release();
    signalsA.waitCompleted();

    // B 应进入执行（区域已释放）
    signalsB.waitEntered();
    EXPECT_TRUE(signalsB.entered.load());

    signalsB.release();
    signalsB.waitCompleted();

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
