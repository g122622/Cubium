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

#include "common/util/concurrent/MultiThreadedQueue.hpp"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <gtest/gtest.h>

using mc::util::MultiThreadedQueue;

// 测试用元素类型：裸指针（int*），对齐 ReentrantAreaLock 的 ThreadHandle* 用法。
// 队列存储裸指针，nullptr 为哨兵。每个测试用局部 int 变量取地址作为元素。

// ============================================================================
// 基本 add / pollOrBlockAdds 顺序
// ============================================================================

TEST(MultiThreadedQueueTest, AddAndPollPreservesFifoOrder)
{
    MultiThreadedQueue<int*> q;
    int a = 1, b = 2, c = 3;

    ASSERT_TRUE(q.add(&a));
    ASSERT_TRUE(q.add(&b));
    ASSERT_TRUE(q.add(&c));

    EXPECT_EQ(q.pollOrBlockAdds(), &a);
    EXPECT_EQ(q.pollOrBlockAdds(), &b);
    EXPECT_EQ(q.pollOrBlockAdds(), &c);
}

TEST(MultiThreadedQueueTest, PollEmptyQueueBlocksAdds)
{
    // 空队列 pollOrBlockAdds 应阻止后续 add（返回 nullptr，置 add-blocked）
    MultiThreadedQueue<int*> q;
    EXPECT_EQ(q.pollOrBlockAdds(), nullptr);
    EXPECT_TRUE(q.isAddBlocked());
}

TEST(MultiThreadedQueueTest, PollEmptyQueueReturnsNullptr)
{
    MultiThreadedQueue<int*> q;
    EXPECT_EQ(q.pollOrBlockAdds(), nullptr);
}

// ============================================================================
// preventAdds / allowAdds
// ============================================================================

TEST(MultiThreadedQueueTest, PreventAddsBlocksSubsequentAdd)
{
    MultiThreadedQueue<int*> q;
    EXPECT_TRUE(q.preventAdds()); // 首次阻止
    EXPECT_TRUE(q.isAddBlocked());

    int a = 1;
    EXPECT_FALSE(q.add(&a));       // add-blocked，添加失败
    EXPECT_TRUE(q.isAddBlocked()); // add 失败不改变 blocked 状态，仍 blocked
}

TEST(MultiThreadedQueueTest, PreventAddsTwiceReturnsFalse)
{
    MultiThreadedQueue<int*> q;
    EXPECT_TRUE(q.preventAdds());  // 首次阻止
    EXPECT_FALSE(q.preventAdds()); // 已阻止，返回 false
}

TEST(MultiThreadedQueueTest, AllowAddsRestoresAddCapability)
{
    MultiThreadedQueue<int*> q;
    q.preventAdds();
    EXPECT_TRUE(q.isAddBlocked());

    q.allowAdds();
    EXPECT_FALSE(q.isAddBlocked());

    int a = 1;
    EXPECT_TRUE(q.add(&a));
    EXPECT_EQ(q.pollOrBlockAdds(), &a);
}

TEST(MultiThreadedQueueTest, AllowAddsWithoutPreventIsHarmless)
{
    // allowAdds 未先 preventAdds 调用，对齐 Moonrise 的 undefined behaviour 警告，
    // 但本实现 allowAdds 找到尾节点（next==null）置 null，幂等无害。
    MultiThreadedQueue<int*> q;
    int a = 1;
    ASSERT_TRUE(q.add(&a));
    q.allowAdds(); // 不应崩溃，队列仍可正常 poll
    EXPECT_EQ(q.pollOrBlockAdds(), &a);
}

// ============================================================================
// forceAdd 解除 add-blocked 并加入
// ============================================================================

TEST(MultiThreadedQueueTest, ForceAddUnblocksAndAdds)
{
    MultiThreadedQueue<int*> q;
    q.preventAdds();
    EXPECT_TRUE(q.isAddBlocked());

    int a = 1;
    // forceAdd 在 add-blocked 时解除阻止并加入，返回 true（之前被阻止）
    EXPECT_TRUE(q.forceAdd(&a));
    EXPECT_FALSE(q.isAddBlocked());
    EXPECT_EQ(q.pollOrBlockAdds(), &a);
}

TEST(MultiThreadedQueueTest, ForceAddOnUnblockedReturnsFalse)
{
    MultiThreadedQueue<int*> q;
    int a = 1;
    // 队列未阻止，forceAdd 加入并返回 false（之前允许添加）
    EXPECT_FALSE(q.forceAdd(&a));
    EXPECT_EQ(q.pollOrBlockAdds(), &a);
}

// ============================================================================
// pollOrBlockAdds 排空队列后阻止入队（ReentrantAreaLock unlock 的核心语义）
// ============================================================================

TEST(MultiThreadedQueueTest, PollOrBlockAddsDrainsThenBlocks)
{
    // 模拟 unlock 语义：排空所有等待者后阻止后续 add
    MultiThreadedQueue<int*> q;
    int a = 1, b = 2, c = 3;
    q.add(&a);
    q.add(&b);
    q.add(&c);

    EXPECT_EQ(q.pollOrBlockAdds(), &a);
    EXPECT_EQ(q.pollOrBlockAdds(), &b);
    EXPECT_EQ(q.pollOrBlockAdds(), &c);
    EXPECT_EQ(q.pollOrBlockAdds(), nullptr); // 排空后阻止入队
    EXPECT_TRUE(q.isAddBlocked());
}

TEST(MultiThreadedQueueTest, PollThenAddAfterBlockRequiresAllowAdds)
{
    // pollOrBlockAdds 排空并阻止后，add 失败；allowAdds 恢复
    MultiThreadedQueue<int*> q;
    int a = 1;
    q.add(&a);
    EXPECT_EQ(q.pollOrBlockAdds(), &a);
    EXPECT_EQ(q.pollOrBlockAdds(), nullptr); // 阻止
    EXPECT_TRUE(q.isAddBlocked());

    int b = 2;
    EXPECT_FALSE(q.add(&b)); // 被阻止
    q.allowAdds();
    EXPECT_TRUE(q.add(&b));
    EXPECT_EQ(q.pollOrBlockAdds(), &b);
}

// ============================================================================
// 析构回收全部链表节点（含被 pollOrBlockAdds 前移越过的前缀）
// ============================================================================

TEST(MultiThreadedQueueTest, DestructorReleasesConsumedPrefixNodes)
{
    // 回归：pollOrBlockAdds 取走元素时只前移"逻辑队首" m_head（节点从不摘链），早期析构从
    // m_head 出发遍历，被越过的前缀（含构造期哨兵）永远不被释放。LSan 在 nightly 集成测试中
    // 报出 3696 个此类泄漏节点（区块调度锁的等待队列），并使 20/24 分片以 SIGABRT 退出。
    const std::size_t before = MultiThreadedQueue<int*>::liveNodeCount();

    {
        MultiThreadedQueue<int*> q;
        int a = 1, b = 2, c = 3;
        ASSERT_TRUE(q.add(&a));
        ASSERT_TRUE(q.add(&b));
        ASSERT_TRUE(q.add(&c));

        // 排空全部元素：逻辑队首依次前移越过哨兵与 a/b/c 节点
        EXPECT_EQ(q.pollOrBlockAdds(), &a);
        EXPECT_EQ(q.pollOrBlockAdds(), &b);
        EXPECT_EQ(q.pollOrBlockAdds(), &c);
        // 空队列 poll 会把尾的 next 置自环（add-blocked），随后 allowAdds 解环再入队
        EXPECT_EQ(q.pollOrBlockAdds(), nullptr);
        q.allowAdds();
        EXPECT_TRUE(q.add(&a));

        EXPECT_GT(MultiThreadedQueue<int*>::liveNodeCount(), before);
    }

    EXPECT_EQ(MultiThreadedQueue<int*>::liveNodeCount(), before)
        << "队列析构后仍有链表节点未释放（逻辑队首前移导致的链路前缀泄漏）";
}

TEST(MultiThreadedQueueTest, DestructorReleasesPreventAddsDeadEndNode)
{
    // 回归：preventAdds 入队的死结节点（自环保留到析构）也必须释放——析构遍历以自环终止，
    // 死结节点本身在自环之前已访问到，故必须计入释放。
    const std::size_t before = MultiThreadedQueue<int*>::liveNodeCount();

    {
        MultiThreadedQueue<int*> q;
        EXPECT_TRUE(q.preventAdds());
        EXPECT_TRUE(q.isAddBlocked());
        EXPECT_GT(MultiThreadedQueue<int*>::liveNodeCount(), before);
    }

    EXPECT_EQ(MultiThreadedQueue<int*>::liveNodeCount(), before);
}

// ============================================================================
// 并发 add + poll：不丢元素
// ============================================================================

TEST(MultiThreadedQueueTest, ConcurrentAddPollNoLostElements)
{
    // N 生产者各 add M 个元素，主线程 poll 直到收齐 N*M 个
    // pollOrBlockAdds 排空后阻止，生产者 add 失败时用 allowAdds 恢复（模拟 unlock 路径）
    constexpr int kProducers = 4;
    constexpr int kPerProducer = 500;

    MultiThreadedQueue<int*> q;
    std::vector<int> values(kProducers * kPerProducer, 0);
    std::atomic<int> produced{0};

    std::vector<std::thread> threads;
    for (int p = 0; p < kProducers; ++p) {
        threads.emplace_back([&, p] {
            for (int i = 0; i < kPerProducer; ++i) {
                int idx = p * kPerProducer + i;
                values[idx] = idx + 1; // 非零标记
                while (!q.add(&values[idx])) {
                    // add 被阻止（pollOrBlockAdds 阻塞入队），等待消费侧 allowAdds
                    std::this_thread::yield();
                }
                ++produced;
            }
        });
    }

    // 消费侧：poll 所有元素
    std::vector<int*> received;
    received.reserve(kProducers * kPerProducer);
    int target = kProducers * kPerProducer;
    while (produced.load() < target || received.size() < static_cast<std::size_t>(target)) {
        int* v = q.pollOrBlockAdds();
        if (v != nullptr) {
            received.push_back(v);
        } else {
            // 排空并阻止：恢复入队能力供生产者继续
            q.allowAdds();
        }
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(received.size(), static_cast<std::size_t>(kProducers * kPerProducer));

    // 校验无丢失：所有 values[i] 都应被收到（值为 i+1 的地址在 received 中出现一次）
    std::vector<int> counts(kProducers * kPerProducer, 0);
    for (int* v : received) {
        int idx = *v - 1;
        ASSERT_GE(idx, 0);
        ASSERT_LT(idx, kProducers * kPerProducer);
        ++counts[idx];
    }
    for (int i = 0; i < kProducers * kPerProducer; ++i) {
        EXPECT_EQ(counts[i], 1) << "元素 " << i << " 丢失或重复";
    }
}

// ============================================================================
// 并发 preventAdds / allowAdds / add 不崩溃（协议鲁棒性）
// ============================================================================

TEST(MultiThreadedQueueTest, ConcurrentPreventAllowAddsRobust)
{
    // 多线程并发 add + 一个线程反复 preventAdds/allowAdds，验证不死锁、不崩溃、不泄漏节点。
    //
    // 【为什么 allowAdds 只能由单一线程调用】allowAdds 的契约是"非 MT-Safe，调用方保证
    // 单线程调用"（见 MultiThreadedQueue.hpp 的声明处）。它的实现是"找到链尾后
    // tail->m_next.store(nullptr)"：若多个线程并发调用，A 线程刚把新节点 Y 链到 X 之后，
    // B 线程就可能把 X.m_next 置回 nullptr，把 Y 及其后继整段摘出链外——析构时从 m_first
    // 遍历再也到不了它们，即泄漏（实测 LeakSanitizer: 960 bytes in 60 objects from
    // appendList + 864 bytes in 54 objects from preventAdds）。
    // 故本测试让 preventAdds/allowAdds 只由 preventer 线程调用，adders 只 add/poll。
    constexpr int kAdders = 4;
    constexpr int kIterations = 1000;

    MultiThreadedQueue<int*> q;
    std::atomic<bool> stop{false};
    std::atomic<int> added{0};

    std::vector<std::thread> threads;
    for (int i = 0; i < kAdders; ++i) {
        threads.emplace_back([&, i] {
            int local = i + 1;
            for (int j = 0; j < kIterations; ++j) {
                if (q.add(&local)) {
                    ++added;
                    // 立即 poll 掉，避免队列无限增长
                    q.pollOrBlockAdds();
                }
                // add 失败表示当前被 preventAdds 阻止；等待 preventer 解除即可，不自行 allowAdds。
            }
        });
    }

    // preventAdds / allowAdds 抖动线程：本线程独占这对调用的调用权（契约要求）。
    std::thread preventer([&] {
        while (!stop.load()) {
            q.preventAdds();
            q.allowAdds();
        }
        // 退出前保证队列处于"允许添加"状态，避免析构时残留自环死结。
        q.allowAdds();
    });

    for (auto& t : threads) {
        t.join();
    }
    stop = true;
    preventer.join();

    SUCCEED(); // 不崩溃、不死锁即通过
}
