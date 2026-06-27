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

using mc::i32;
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
    constexpr int kIterations = 200;

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

// ============================================================================
// 回归：大区域 unlock 不阻塞其他线程（:notify 热点的核心回归测试）
//
// 旧实现 unlock 对每个 key 逐一 notify_all（onChunkGenComplete 半径 22 → 2025 key），
// 且全表共用一把 mutex，导致远处不相交区域的 lock 被 unlock 阻塞。新实现无锁：
// unlock 只排空本 Node 的等待队列，不影响不相交区域的 lock。
// 此测试：线程 A 持有大区域 [0±10]，线程 B 锁远处不相交区域 [1000,1000] 应立即成功。
// ============================================================================

TEST(ReentrantAreaLockTest, LargeAreaUnlockDoesNotBlockDistantArea)
{
    ReentrantAreaLock lock(0);

    std::atomic<bool> otherHoldsLarge{false};
    std::atomic<bool> distantAcquired{false};
    std::atomic<bool> distantDone{false};
    std::barrier sync(2);

    std::thread tA([&] {
        // 持有大区域 [0..20, 0..20]（441 个 section）
        auto node = lock.lock(10, 10, 10);
        otherHoldsLarge = true;
        // 等 B 尝试锁远处区域
        sync.arrive_and_wait();
        // 等 B 完成
        sync.arrive_and_wait();
        node.reset();
    });

    std::thread tB([&] {
        sync.arrive_and_wait();
        // A 持有大区域时，B 锁 [1000,1000]（不相交）应立即成功（无锁，不被 A 的 441-key 操作阻塞）
        auto start = std::chrono::steady_clock::now();
        auto node = lock.tryLock(1000, 1000);
        auto elapsed = std::chrono::steady_clock::now() - start;
        distantAcquired = (node != nullptr);
        node.reset();
        distantDone = true;
        sync.arrive_and_wait();

        // tryLock 应在毫秒级完成（不被 A 阻塞）
        EXPECT_LT(elapsed, std::chrono::milliseconds(500));
    });

    tA.join();
    tB.join();

    EXPECT_TRUE(otherHoldsLarge.load());
    EXPECT_TRUE(distantAcquired.load());
    EXPECT_TRUE(distantDone.load());
}

// ============================================================================
// 回归：大区域 unlock 不阻塞其他线程的 lock（阻塞路径，非 tryLock）
//
// 与上一测试的区别：B 用 lock()（阻塞路径），验证 A 的大区域持有期间 B 的不相交区域
// lock() 立即返回（不需要等 A 释放）。这覆盖 :notify 热点的典型场景——旧实现下 B 会
// 在 m_mutex 上排队等 A 的 unlock 完成。
// ============================================================================

TEST(ReentrantAreaLockTest, LargeAreaHeldDoesNotBlockDistantLock)
{
    ReentrantAreaLock lock(0);

    std::atomic<bool> distantAcquired{false};
    std::barrier sync(2);

    std::thread tA([&] {
        auto node = lock.lock(10, 10, 10); // [0..20, 0..20]
        sync.arrive_and_wait();            // B 开始
        // 等 B 锁到远处区域
        sync.arrive_and_wait();
        node.reset();
    });

    std::thread tB([&] {
        sync.arrive_and_wait();
        auto start = std::chrono::steady_clock::now();
        auto node = lock.lock(1000, 1000); // 不相交，应立即返回
        auto elapsed = std::chrono::steady_clock::now() - start;
        distantAcquired = true;
        node.reset();
        sync.arrive_and_wait();

        EXPECT_LT(elapsed, std::chrono::milliseconds(500));
    });

    tA.join();
    tB.join();

    EXPECT_TRUE(distantAcquired.load());
}

// ============================================================================
// N 线程锁重叠区域：严格互斥，临界区并发数始终为 1
// ============================================================================

TEST(ReentrantAreaLockTest, IntersectingAreasStrictMutexNThreads)
{
    // 8 线程锁完全相同的区域，验证同一时刻只有一个线程在临界区
    ReentrantAreaLock lock(0);

    constexpr int kThreads = 8;
    constexpr int kIterations = 50;

    std::atomic<int> inCriticalSection{0};
    std::atomic<int> maxConcurrent{0};
    std::atomic<int> completed{0};

    auto worker = [&]() {
        for (int i = 0; i < kIterations; ++i) {
            auto node = lock.lock(0, 0, 3); // 所有线程锁同一区域 [0..6]
            int cur = ++inCriticalSection;
            int prevMax = maxConcurrent.load();
            while (cur > prevMax) {
                if (maxConcurrent.compare_exchange_weak(prevMax, cur)) {
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::microseconds(50));
            --inCriticalSection;
            node.reset();
            ++completed;
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back(worker);
    }
    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(maxConcurrent.load(), 1); // 严格互斥
    EXPECT_EQ(completed.load(), kThreads * kIterations);
}

// ============================================================================
// 不相交多区域高并发：不同线程锁不同区域，无串行化，全部完成
// ============================================================================

TEST(ReentrantAreaLockTest, NonIntersectingAreasHighContentionAllComplete)
{
    // 每个线程锁自己独占的区域（不相交），验证无锁路径不误阻塞、全部完成
    ReentrantAreaLock lock(0);

    constexpr int kThreads = 8;
    constexpr int kIterations = 100;

    std::atomic<int> completed{0};

    auto worker = [&](int tid) {
        // 每个线程的区域中心间隔 1000，完全不相交
        i32 base = tid * 1000;
        for (int i = 0; i < kIterations; ++i) {
            auto node = lock.lock(base, base, 5);
            ++completed;
            node.reset();
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back(worker, i);
    }
    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(completed.load(), kThreads * kIterations);
}

// ============================================================================
// 同线程重入相交但不被覆盖的区域应断言失败（非法锁使用）
//
// 对齐 Moonrise "Should never acquire intersecting areas"：
// 同线程已持有 [0..4]，再锁 [3..6]（相交但不被覆盖）应触发断言。
// 注：MC_ASSERT_RELEASE_MSG 在 Release 下也触发，此测试验证断言路径。
// 由于断言会终止进程，本测试用 EXPECT_DEATH（若断言实现为 abort）。
// 若项目断言抛异常，则用 EXPECT_ANY_THROW。这里按 MC_ASSERT_RELEASE（abort）处理。
// ============================================================================

// 注：此测试默认禁用——MC_ASSERT_RELEASE 终止进程，EXPECT_DEATH 会 fork 子进程，
// 在某些构建配置下可能不可用。保留为注释，供需要时启用。
// 如需启用，取消注释并确保测试框架支持死亡测试。
//
// TEST(ReentrantAreaLockTest, IntersectingNotCoveredReentryAsserts)
// {
//     testing::FLAGS_gtest_death_test_style = "threadsafe";
//     EXPECT_DEATH(
//         {
//             ReentrantAreaLock lock(0);
//             auto outer = lock.lock(2, 2, 2); // [0..4]
//             auto inner = lock.lock(4, 4, 2); // [2..6] 相交但不被覆盖
//         },
//         "Intersecting areas not fully covered");
// }

// ============================================================================
// tryLock 多区域：tryLock 不相交区域全部成功，tryLock 相交区域失败
// ============================================================================

TEST(ReentrantAreaLockTest, TryLockNonIntersectingSucceedsConcurrent)
{
    // 主线程持有 [0,0]，另一线程 tryLock 多个不相交区域应全部成功
    ReentrantAreaLock lock(0);

    auto node = lock.tryLock(0, 0);
    ASSERT_NE(node, nullptr);

    std::atomic<int> success{0};
    std::thread t([&] {
        for (i32 x : {100, 200, 300, 400, 500}) {
            auto n = lock.tryLock(x, x);
            if (n != nullptr) {
                ++success;
                n.reset();
            }
        }
    });
    t.join();

    EXPECT_EQ(success.load(), 5); // 全部不相交，全部成功
    node.reset();
}

// ============================================================================
// 嵌套 lock 与 isHeldByCurrentThread 覆盖性
// ============================================================================

TEST(ReentrantAreaLockTest, NestedLockHoldsAllLayers)
{
    ReentrantAreaLock lock(0);

    auto a = lock.lock(0, 0); // [0,0]
    EXPECT_TRUE(lock.isHeldByCurrentThread(0, 0));
    auto b = lock.lock(10, 10, 2); // [8..12, 8..12]
    EXPECT_TRUE(lock.isHeldByCurrentThread(0, 0));
    EXPECT_TRUE(lock.isHeldByCurrentThread(10, 10, 2));
    auto c = lock.lock(20, 20); // [20,20]
    EXPECT_TRUE(lock.isHeldByCurrentThread(0, 0));
    EXPECT_TRUE(lock.isHeldByCurrentThread(10, 10, 2));
    EXPECT_TRUE(lock.isHeldByCurrentThread(20, 20));

    // 释放中间层，外层和内层仍持有
    b.reset();
    EXPECT_TRUE(lock.isHeldByCurrentThread(0, 0));
    EXPECT_FALSE(lock.isHeldByCurrentThread(10, 10, 2));
    EXPECT_TRUE(lock.isHeldByCurrentThread(20, 20));

    c.reset();
    EXPECT_FALSE(lock.isHeldByCurrentThread(20, 20));
    EXPECT_TRUE(lock.isHeldByCurrentThread(0, 0));

    a.reset();
    EXPECT_FALSE(lock.isHeldByCurrentThread(0, 0));
}

// ============================================================================
// 同线程重入单区块多次（areaAffectedLen=0 纯重入路径）
// ============================================================================

TEST(ReentrantAreaLockTest, ReentrantSingleChunkMultipleTimes)
{
    // 同线程对同一单区块重入多次，unlock 顺序无关，最终释放后可被其他线程获取
    ReentrantAreaLock lock(0);

    auto a = lock.lock(5, 5);
    auto b = lock.lock(5, 5);
    auto c = lock.lock(5, 5);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    ASSERT_NE(c, nullptr);
    EXPECT_TRUE(lock.isHeldByCurrentThread(5, 5));

    // 乱序释放
    b.reset();
    EXPECT_TRUE(lock.isHeldByCurrentThread(5, 5));
    c.reset();
    EXPECT_TRUE(lock.isHeldByCurrentThread(5, 5));
    a.reset();
    EXPECT_FALSE(lock.isHeldByCurrentThread(5, 5));

    // 其他线程可获取
    std::atomic<bool> got{false};
    std::thread t([&] {
        auto node = lock.tryLock(5, 5);
        got = (node != nullptr);
        node.reset();
    });
    t.join();
    EXPECT_TRUE(got.load());
}

// ============================================================================
// park/unpark-before-park 不丢唤醒（ReentrantAreaLock 集成层面）
//
// 线程 A 持有锁，线程 B 阻塞在 lock（park）。A 释放锁（unpark B）。
// 验证 B 被唤醒并获取锁。重复多次验证不丢唤醒。
// ============================================================================

TEST(ReentrantAreaLockTest, ParkUnparkNoLostWakeupRepeated)
{
    ReentrantAreaLock lock(0);

    constexpr int kRounds = 20;

    for (int round = 0; round < kRounds; ++round) {
        std::atomic<bool> aHolds{false};
        std::atomic<bool> bAcquired{false};
        std::barrier sync(2);

        std::thread tA([&] {
            auto node = lock.lock(7, 7);
            aHolds = true;
            sync.arrive_and_wait();                                     // B 开始尝试 lock（将 park）
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 确保 B 已 park
            node.reset();                                               // 释放，unpark B
            sync.arrive_and_wait();                                     // B 应已获取
        });

        std::thread tB([&] {
            sync.arrive_and_wait();
            auto node = lock.lock(7, 7); // 阻塞直到 A 释放
            bAcquired = true;
            node.reset();
            sync.arrive_and_wait();
        });

        tA.join();
        tB.join();

        EXPECT_TRUE(aHolds.load()) << "round " << round;
        EXPECT_TRUE(bAcquired.load()) << "round " << round;
    }
}
