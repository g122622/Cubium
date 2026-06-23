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
 * LIABILITY, IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/util/concurrent/ReentrantAreaLock.hpp"

#include <atomic>
#include <barrier>
#include <chrono>
#include <thread>
#include <vector>
#include <gtest/gtest.h>

using mc::util::ReentrantAreaLock;

// ============================================================================
// 单区块锁（coordinateShift = 0，一区块一锁条目）
// ============================================================================

TEST(ReentrantAreaLockTest, SingleChunkLockAcquireAndRelease)
{
    ReentrantAreaLock lock(0);

    // 锁定单个区块
    auto node = lock.lock(5, 3);
    ASSERT_NE(node, nullptr);

    // 当前线程持有该区块锁
    EXPECT_TRUE(lock.isHeldByCurrentThread(5, 3));

    // 释放
    node.reset();
    EXPECT_FALSE(lock.isHeldByCurrentThread(5, 3));
}

TEST(ReentrantAreaLockTest, ReentrantSameThreadSameArea)
{
    ReentrantAreaLock lock(0);

    // 同线程对同一区块重入加锁
    auto node1 = lock.lock(10, 10);
    auto node2 = lock.lock(10, 10); // 重入，应立即成功
    ASSERT_NE(node1, nullptr);
    ASSERT_NE(node2, nullptr);

    EXPECT_TRUE(lock.isHeldByCurrentThread(10, 10));

    // 释放内层
    node2.reset();
    // 仍持有（外层未释放）
    EXPECT_TRUE(lock.isHeldByCurrentThread(10, 10));

    // 释放外层
    node1.reset();
    EXPECT_FALSE(lock.isHeldByCurrentThread(10, 10));
}

TEST(ReentrantAreaLockTest, ReentrantSubArea)
{
    // 锁大区域 [0..4, 0..4]，再锁子区域 [2..2, 2..2]
    ReentrantAreaLock lock(0);

    auto outer = lock.lock(2, 2, 2); // [0..4, 0..4]
    ASSERT_NE(outer, nullptr);
    EXPECT_TRUE(lock.isHeldByCurrentThread(2, 2, 2));

    // 子区域重入
    auto inner = lock.lock(2, 2); // [2..2]
    ASSERT_NE(inner, nullptr);
    EXPECT_TRUE(lock.isHeldByCurrentThread(2, 2));

    inner.reset();
    EXPECT_TRUE(lock.isHeldByCurrentThread(2, 2, 2)); // 外层仍持有

    outer.reset();
    EXPECT_FALSE(lock.isHeldByCurrentThread(2, 2, 2));
}

TEST(ReentrantAreaLockTest, DifferentChunksParallel)
{
    // 不同区块（不相交）的锁互不影响
    ReentrantAreaLock lock(0);

    auto nodeA = lock.lock(0, 0);
    auto nodeB = lock.lock(100, 100);
    ASSERT_NE(nodeA, nullptr);
    ASSERT_NE(nodeB, nullptr);

    EXPECT_TRUE(lock.isHeldByCurrentThread(0, 0));
    EXPECT_TRUE(lock.isHeldByCurrentThread(100, 100));
    EXPECT_FALSE(lock.isHeldByCurrentThread(50, 50));

    nodeA.reset();
    nodeB.reset();
    EXPECT_FALSE(lock.isHeldByCurrentThread(0, 0));
    EXPECT_FALSE(lock.isHeldByCurrentThread(100, 100));
}

// ============================================================================
// tryLock
// ============================================================================

TEST(ReentrantAreaLockTest, TryLockSuccessOnFreeArea)
{
    ReentrantAreaLock lock(0);

    auto node = lock.tryLock(5, 5);
    ASSERT_NE(node, nullptr);
    EXPECT_TRUE(lock.isHeldByCurrentThread(5, 5));
    node.reset();
    EXPECT_FALSE(lock.isHeldByCurrentThread(5, 5));
}

TEST(ReentrantAreaLockTest, TryLockFailsWhenHeldByOtherThread)
{
    ReentrantAreaLock lock(0);

    std::atomic<bool> otherHolds{false};
    std::atomic<bool> tryLockDone{false};
    std::atomic<bool> tryLockResult{true};

    std::barrier sync(2);

    std::thread other([&] {
        auto node = lock.lock(7, 7);
        otherHolds = true;
        // 等主线程尝试 tryLock
        sync.arrive_and_wait();
        // 等主线程 tryLock 完成
        sync.arrive_and_wait();
        node.reset();
    });

    // 等其他线程持有锁
    sync.arrive_and_wait();

    // 主线程 tryLock 应失败（其他线程持有）
    auto node = lock.tryLock(7, 7);
    tryLockResult = (node != nullptr);
    tryLockDone = true;

    sync.arrive_and_wait();
    other.join();

    EXPECT_FALSE(tryLockResult.load());
}

// ============================================================================
// 多线程互斥：相交区域必须串行
// ============================================================================

TEST(ReentrantAreaLockTest, IntersectingAreasSerializeAcrossThreads)
{
    // 两个线程锁相交区域，必须串行执行（不能并行进入临界区）
    ReentrantAreaLock lock(0);

    std::atomic<int> inCriticalSection{0};
    std::atomic<int> maxConcurrent{0};
    std::atomic<bool> start{false};

    auto worker = [&](int centerX) {
        // 等待发令
        while (!start.load()) {
            std::this_thread::yield();
        }
        // 锁 [centerX-1 .. centerX+1]，两线程区域在 centerX=0/1 时相交
        auto node = lock.lock(centerX, 0, 1);
        int cur = ++inCriticalSection;
        int prevMax = maxConcurrent.load();
        while (cur > prevMax) {
            if (maxConcurrent.compare_exchange_weak(prevMax, cur)) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        --inCriticalSection;
        node.reset();
    };

    std::thread t1(worker, 0);
    std::thread t2(worker, 1); // 与 t0 区域相交

    start = true;
    t1.join();
    t2.join();

    // 相交区域必须串行，因此最大并发数应为 1
    EXPECT_EQ(maxConcurrent.load(), 1);
}

TEST(ReentrantAreaLockTest, NonIntersectingAreasParallelAcrossThreads)
{
    // 不相交区域可以并行（tryLock 不应被远处的锁阻塞）
    ReentrantAreaLock lock(0);

    std::barrier sync(2);
    std::atomic<bool> bothAcquired{false};

    std::thread t1([&] {
        auto node = lock.lock(0, 0);
        sync.arrive_and_wait(); // 同步：双方都持有
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        node.reset();
    });

    std::thread t2([&] {
        sync.arrive_and_wait();
        // t1 持有 [0,0]，t2 锁 [1000,1000]（不相交），应立即成功
        auto node = lock.tryLock(1000, 1000);
        bothAcquired = (node != nullptr);
        node.reset();
    });

    t1.join();
    t2.join();

    EXPECT_TRUE(bothAcquired.load());
}

// ============================================================================
// coordinateShift 分区粒度
// ============================================================================

TEST(ReentrantAreaLockTest, CoordinateShiftGroupsChunksIntoSections)
{
    // coordinateShift=2 → 4 个区块共用一个 section 键
    // 锁 (0,0) 应阻塞锁 (1,1)（同 section）
    ReentrantAreaLock lock(2);

    auto node = lock.lock(0, 0);
    ASSERT_NE(node, nullptr);

    // (1,1) 与 (0,0) 同 section（0>>2=0, 1>>2=0），应被阻塞
    EXPECT_TRUE(lock.isHeldByCurrentThread(0, 0));
    EXPECT_TRUE(lock.isHeldByCurrentThread(1, 1)); // 同 section，同线程持有

    // (4,4) 不同 section（4>>2=1），不应被持有
    EXPECT_FALSE(lock.isHeldByCurrentThread(4, 4));

    node.reset();
    EXPECT_FALSE(lock.isHeldByCurrentThread(0, 0));
    EXPECT_FALSE(lock.isHeldByCurrentThread(1, 1));
}

// ============================================================================
// 负坐标
// ============================================================================

TEST(ReentrantAreaLockTest, NegativeCoordinates)
{
    ReentrantAreaLock lock(0);

    auto node = lock.lock(-5, -3, 2); // [-7..-3, -5..-1]
    ASSERT_NE(node, nullptr);

    EXPECT_TRUE(lock.isHeldByCurrentThread(-5, -3, 2));
    EXPECT_TRUE(lock.isHeldByCurrentThread(-7, -5));  // 左下角
    EXPECT_TRUE(lock.isHeldByCurrentThread(-3, -1));  // 右上角
    EXPECT_FALSE(lock.isHeldByCurrentThread(-8, -5)); // 越界左
    EXPECT_FALSE(lock.isHeldByCurrentThread(0, 0));   // 越界右上

    node.reset();
    EXPECT_FALSE(lock.isHeldByCurrentThread(-5, -3, 2));
}

// ============================================================================
// 覆盖完整区域的 isHeldByCurrentThread(fromX, fromZ, toX, toZ)
// ============================================================================

TEST(ReentrantAreaLockTest, IsHeldByCurrentThreadRectangular)
{
    ReentrantAreaLock lock(0);

    auto node = lock.lock(10, 10, 2); // [8..12, 8..12]
    ASSERT_NE(node, nullptr);

    // 整个矩形被持有
    EXPECT_TRUE(lock.isHeldByCurrentThread(8, 8, 12, 12));
    // 子区域被持有
    EXPECT_TRUE(lock.isHeldByCurrentThread(9, 9, 11, 11));
    // 超出范围
    EXPECT_FALSE(lock.isHeldByCurrentThread(7, 8, 12, 12)); // x 越界
    EXPECT_FALSE(lock.isHeldByCurrentThread(8, 8, 13, 12)); // x 越界
    EXPECT_FALSE(lock.isHeldByCurrentThread(8, 7, 12, 12)); // z 越界

    node.reset();
    EXPECT_FALSE(lock.isHeldByCurrentThread(8, 8, 12, 12));
}

// ============================================================================
// 死锁避免：固定顺序加锁不会死锁（多线程多次嵌套）
// ============================================================================

TEST(ReentrantAreaLockTest, NoDeadlockUnderHighContention)
{
    // 多线程高竞争同一区域，验证不死锁、最终都能完成
    ReentrantAreaLock lock(0);

    constexpr int kThreads = 8;
    constexpr int kIterations = 50;

    std::atomic<int> completed{0};

    auto worker = [&]() {
        for (int i = 0; i < kIterations; ++i) {
            auto node = lock.lock(0, 0, 5);
            // 临界区
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            ++completed;
            node.reset();
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back(worker);
    }
    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(completed.load(), kThreads * kIterations);
}

// ============================================================================
// 锁释放后可被其他线程获取
// ============================================================================

TEST(ReentrantAreaLockTest, LockReleasedThenReacquiredByOtherThread)
{
    ReentrantAreaLock lock(0);

    std::atomic<bool> reacquired{false};

    std::thread t1([&] {
        auto node = lock.lock(3, 3);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        node.reset();
    });

    std::thread t2([&] {
        // 等待 t1 释放后获取
        auto node = lock.lock(3, 3);
        reacquired = true;
        node.reset();
    });

    t1.join();
    t2.join();

    EXPECT_TRUE(reacquired.load());
}
