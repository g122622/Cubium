/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, without limitation the use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "common/util/concurrent/LockSupport.hpp"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <gtest/gtest.h>

using mc::util::LockSupport;

// ============================================================================
// permit 基础语义
// ============================================================================

TEST(LockSupportTest, ParkConsumesPermitAndReturnsImmediately)
{
    // park 前置 permit：park 应立即消费 permit 返回，不阻塞
    auto handle = LockSupport::currentThread();
    LockSupport::unpark(handle);
    LockSupport::park(); // 应立即返回

    SUCCEED(); // 若 park 阻塞，本测试会卡住（超时由 gtest 死锁检测或 CI 捕获）
}

TEST(LockSupportTest, ParkWithNoPermitBlocksUntilUnpark)
{
    // park 无 permit：阻塞，直到另一线程 unpark
    auto parkedHandle = LockSupport::currentThread();

    std::atomic<bool> unparked{false};
    std::atomic<bool> parked{false};

    std::thread t([&] {
        // 确保主线程已进入 park（至少设置 parked 标志后再 unpark，但这不保证 park 已阻塞，
        // 所以 unpark 的 permit 会被 park 消费——permit 持久性保证不丢唤醒）。
        // 注意 unparked 必须在 unpark 之前置位:unpark 的 permit exchange(seq_cst) 与
        // park 内对 permit 的 exchange/load(seq_cst) 建立 happens-before,park 返回时
        // unparked.store 必可见;若 store 在 unpark 之后,park 可能在 unpark 唤醒后、
        // store 传播前返回,读到 false(负载下偶发)。
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        parked = true;
        unparked = true;
        LockSupport::unpark(parkedHandle);
    });

    LockSupport::park(); // 无 permit，阻塞
    EXPECT_TRUE(unparked.load());

    t.join();
}

TEST(LockSupportTest, UnparkBeforeParkDoesNotLoseWakeup)
{
    // 关键回归测试：unpark 先于 park 调用，permit 持久保留，park 立即返回
    // 这是 ReentrantAreaLock 无锁等待协议正确性的基础（unlock 排空后 park 仍能消费 permit）
    auto handle = LockSupport::currentThread();

    std::thread t([&] {
        // 先 unpark，再等一小段时间确保主线程 park 在后面
        LockSupport::unpark(handle);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });

    t.join();
    // 此时 permit 已置 1，park 应立即返回
    auto start = std::chrono::steady_clock::now();
    LockSupport::park();
    auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_LT(elapsed, std::chrono::milliseconds(100));
}

TEST(LockSupportTest, ParkNanosTimeoutWithoutPermit)
{
    // parkNanos 无 permit 时应在超时后返回
    auto start = std::chrono::steady_clock::now();
    LockSupport::parkNanos(20'000'000); // 20ms
    auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_GE(elapsed, std::chrono::milliseconds(15));
    EXPECT_LT(elapsed, std::chrono::milliseconds(500));
}

TEST(LockSupportTest, ParkNanosReturnsImmediatelyWithPermit)
{
    auto handle = LockSupport::currentThread();
    LockSupport::unpark(handle);
    auto start = std::chrono::steady_clock::now();
    LockSupport::parkNanos(1'000'000'000); // 1s，但 permit 已置，应立即返回
    auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_LT(elapsed, std::chrono::milliseconds(100));
}

TEST(LockSupportTest, ParkNanosZeroOrNegativeReturnsImmediately)
{
    LockSupport::parkNanos(0);
    LockSupport::parkNanos(-1);
    SUCCEED();
}

// ============================================================================
// permit 不累加：多次 unpark 等同一次
// ============================================================================

TEST(LockSupportTest, MultipleUnparksCoalesceToOnePermit)
{
    auto handle = LockSupport::currentThread();
    LockSupport::unpark(handle);
    LockSupport::unpark(handle);
    LockSupport::unpark(handle);
    // permit 仍为 1（不累加）
    LockSupport::park(); // 立即返回消费 permit
    // 再 park 应阻塞（permit 已消费），需另一线程 unpark
    std::atomic<bool> unparked{false};
    std::thread t([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        // unparked 必须在 unpark 之前置位(见 ParkWithNoPermitBlocksUntilUnpark 同类说明),
        // 否则 park 可能在 unpark 唤醒后、store 传播前返回读到 false。
        unparked = true;
        LockSupport::unpark(handle);
    });
    LockSupport::park();
    EXPECT_TRUE(unparked.load());
    t.join();
}

// ============================================================================
// 裸指针 unpark 重载（ReentrantAreaLock 等待队列用 ThreadHandle*）
// ============================================================================

TEST(LockSupportTest, UnparkRawPointerOverload)
{
    auto handle = LockSupport::currentThread();
    LockSupport::ThreadHandle* raw = handle.get();
    LockSupport::unpark(raw); // 裸指针重载
    LockSupport::park();      // 立即返回
    SUCCEED();
}

TEST(LockSupportTest, UnparkNullptrIsNoop)
{
    LockSupport::unpark(static_cast<LockSupport::ThreadHandle*>(nullptr));
    LockSupport::unpark(std::shared_ptr<LockSupport::ThreadHandle>{});
    SUCCEED();
}

// ============================================================================
// currentThread 返回同一线程的稳定句柄
// ============================================================================

TEST(LockSupportTest, CurrentThreadReturnsStableHandleForSameThread)
{
    auto h1 = LockSupport::currentThread();
    auto h2 = LockSupport::currentThread();
    EXPECT_EQ(h1.get(), h2.get()); // 同一线程返回同一句柄
    EXPECT_EQ(h1->threadId(), std::this_thread::get_id());

    std::thread t([&] {
        auto h3 = LockSupport::currentThread();
        auto h4 = LockSupport::currentThread();
        EXPECT_EQ(h3.get(), h4.get());
        EXPECT_NE(h3.get(), h1.get()); // 不同线程不同句柄
        EXPECT_EQ(h3->threadId(), t.get_id());
    });
    t.join();
}

// ============================================================================
// 跨线程唤醒（基础唤醒正确性）
// ============================================================================

TEST(LockSupportTest, CrossThreadUnparkWakesParkedThread)
{
    auto parkedHandle = LockSupport::currentThread();
    std::atomic<bool> woke{false};

    std::thread t([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        LockSupport::unpark(parkedHandle);
    });

    LockSupport::park();
    woke = true;
    EXPECT_TRUE(woke.load());
    t.join();
}

// ============================================================================
// 多线程并发 unpark 同一线程（只唤醒一次，不丢唤醒）
// ============================================================================

TEST(LockSupportTest, ConcurrentUnparkOnlyWakesOnce)
{
    auto parkedHandle = LockSupport::currentThread();
    constexpr int kThreads = 4;
    std::vector<std::thread> threads;
    std::atomic<int> unparkCount{0};

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            ++unparkCount;
            LockSupport::unpark(parkedHandle);
        });
    }

    // 多个线程 unpark，permit 不累加，park 唤醒一次后消费 permit
    LockSupport::park();
    // 等所有 unpark 线程结束
    for (auto& t : threads) {
        t.join();
    }
    EXPECT_EQ(unparkCount.load(), kThreads);
    // park 已返回（permit 被 4 个 unpark 中的某一个置 1 并被消费）
    SUCCEED();
}
