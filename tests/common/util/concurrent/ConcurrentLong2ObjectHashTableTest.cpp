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
 * IMPLIED, THE SAME. A IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "common/util/concurrent/ConcurrentLong2ObjectHashTable.hpp"

#include <atomic>
#include <barrier>
#include <memory>
#include <thread>
#include <vector>
#include <gtest/gtest.h>

using mc::u64;
using mc::util::ConcurrentLong2ObjectHashTable;

// 测试值类型：shared_ptr<Value>（对齐 ReentrantAreaLock 的 shared_ptr<Node> 用法）
struct Value {
    int id;
    explicit Value(int i)
        : id(i)
    {}
};
using ValPtr = std::shared_ptr<Value>;

// ============================================================================
// putIfAbsent / get 基础
// ============================================================================

TEST(ConcurrentLong2ObjectHashTableTest, PutIfAbsentInsertsAndReturnsNull)
{
    ConcurrentLong2ObjectHashTable<ValPtr> table;
    auto v = std::make_shared<Value>(1);
    EXPECT_EQ(table.putIfAbsent(100, v), nullptr); // 插入成功
    EXPECT_EQ(table.get(100), v);                  // 可读回
}

TEST(ConcurrentLong2ObjectHashTableTest, PutIfAbsentReturnsExistingOnConflict)
{
    ConcurrentLong2ObjectHashTable<ValPtr> table;
    auto v1 = std::make_shared<Value>(1);
    auto v2 = std::make_shared<Value>(2);

    EXPECT_EQ(table.putIfAbsent(100, v1), nullptr);
    EXPECT_EQ(table.putIfAbsent(100, v2), v1); // 冲突，返回旧值 v1，不插入 v2
    EXPECT_EQ(table.get(100), v1);             // 仍是 v1
    EXPECT_NE(table.get(100), v2);
}

TEST(ConcurrentLong2ObjectHashTableTest, GetMissingKeyReturnsNull)
{
    ConcurrentLong2ObjectHashTable<ValPtr> table;
    EXPECT_EQ(table.get(999), nullptr);
    auto v = std::make_shared<Value>(1);
    table.putIfAbsent(100, v);
    EXPECT_EQ(table.get(999), nullptr); // 其他 key 仍 null
}

// ============================================================================
// remove(key, expected) 值校验
// ============================================================================

TEST(ConcurrentLong2ObjectHashTableTest, RemoveWithMatchingValueSucceeds)
{
    ConcurrentLong2ObjectHashTable<ValPtr> table;
    auto v = std::make_shared<Value>(1);
    table.putIfAbsent(100, v);

    EXPECT_TRUE(table.remove(100, v));  // 值匹配，摘除成功
    EXPECT_EQ(table.get(100), nullptr); // 已移除
}

TEST(ConcurrentLong2ObjectHashTableTest, RemoveWithMismatchedValueFails)
{
    ConcurrentLong2ObjectHashTable<ValPtr> table;
    auto v1 = std::make_shared<Value>(1);
    auto v2 = std::make_shared<Value>(2);
    table.putIfAbsent(100, v1);

    EXPECT_FALSE(table.remove(100, v2)); // 值不匹配（v2 != v1），不摘除
    EXPECT_EQ(table.get(100), v1);       // 仍是 v1
}

TEST(ConcurrentLong2ObjectHashTableTest, RemoveMissingKeyFails)
{
    ConcurrentLong2ObjectHashTable<ValPtr> table;
    auto v = std::make_shared<Value>(1);
    EXPECT_FALSE(table.remove(999, v)); // key 不存在
}

TEST(ConcurrentLong2ObjectHashTableTest, RemoveThenReinsertWorks)
{
    ConcurrentLong2ObjectHashTable<ValPtr> table;
    auto v1 = std::make_shared<Value>(1);
    auto v2 = std::make_shared<Value>(2);

    table.putIfAbsent(100, v1);
    EXPECT_TRUE(table.remove(100, v1));
    // 摘除后可重新插入不同的值
    EXPECT_EQ(table.putIfAbsent(100, v2), nullptr);
    EXPECT_EQ(table.get(100), v2);
    EXPECT_TRUE(table.remove(100, v2));
}

// ============================================================================
// 多 key 并发安全（不同桶 + 同桶链表）
// ============================================================================

TEST(ConcurrentLong2ObjectHashTableTest, MultipleKeysIndependent)
{
    ConcurrentLong2ObjectHashTable<ValPtr> table;
    constexpr int kCount = 100;
    std::vector<ValPtr> values;
    values.reserve(kCount);
    for (int i = 0; i < kCount; ++i) {
        values.push_back(std::make_shared<Value>(i));
        EXPECT_EQ(table.putIfAbsent(static_cast<u64>(i), values[i]), nullptr);
    }
    for (int i = 0; i < kCount; ++i) {
        EXPECT_EQ(table.get(static_cast<u64>(i)), values[i]);
    }
    // 摘除偶数 key
    for (int i = 0; i < kCount; i += 2) {
        EXPECT_TRUE(table.remove(static_cast<u64>(i), values[i]));
    }
    for (int i = 0; i < kCount; ++i) {
        if (i % 2 == 0) {
            EXPECT_EQ(table.get(static_cast<u64>(i)), nullptr);
        } else {
            EXPECT_EQ(table.get(static_cast<u64>(i)), values[i]);
        }
    }
}

// ============================================================================
// 并发 putIfAbsent 同一 key：只有一个胜出
// ============================================================================

TEST(ConcurrentLong2ObjectHashTableTest, ConcurrentPutIfAbsentSameKeyOneWinner)
{
    // N 线程对同一 key 并发 putIfAbsent，只有一个返回 nullptr（插入成功），其余返回胜出者的值
    constexpr int kThreads = 8;
    ConcurrentLong2ObjectHashTable<ValPtr> table;
    std::vector<ValPtr> values;
    values.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        values.push_back(std::make_shared<Value>(i));
    }

    std::atomic<int> winners{0};
    std::barrier sync(kThreads);

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&, i] {
            sync.arrive_and_wait();
            ValPtr prev = table.putIfAbsent(42, values[i]);
            if (prev == nullptr) {
                ++winners;
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(winners.load(), 1); // 只有一个胜出者
    // 当前值应是胜出者之一（具体哪个不确定，但必须是 values 中的某个）
    ValPtr current = table.get(42);
    ASSERT_NE(current, nullptr);
    bool found = false;
    for (int i = 0; i < kThreads; ++i) {
        if (current == values[i]) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

// ============================================================================
// 并发 put + remove + get 不崩溃，最终一致
// ============================================================================

TEST(ConcurrentLong2ObjectHashTableTest, ConcurrentPutRemoveGetConsistent)
{
    // 多线程对各自的 key 范围操作（不交叉），最终各自 key 应可读可移除
    constexpr int kThreads = 8;
    constexpr int kPerThread = 200;
    ConcurrentLong2ObjectHashTable<ValPtr> table;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            const u64 base = static_cast<u64>(t) * 10000;
            std::vector<ValPtr> local;
            local.reserve(kPerThread);
            for (int i = 0; i < kPerThread; ++i) {
                local.push_back(std::make_shared<Value>(t * 1000 + i));
                ASSERT_EQ(table.putIfAbsent(base + static_cast<u64>(i), local[i]), nullptr);
            }
            // 校验自己的 key
            for (int i = 0; i < kPerThread; ++i) {
                ASSERT_EQ(table.get(base + static_cast<u64>(i)), local[i]);
            }
            // 摘除自己的 key
            for (int i = 0; i < kPerThread; ++i) {
                ASSERT_TRUE(table.remove(base + static_cast<u64>(i), local[i]));
            }
            for (int i = 0; i < kPerThread; ++i) {
                ASSERT_EQ(table.get(base + static_cast<u64>(i)), nullptr);
            }
        });
    }
    for (auto& t : threads) {
        t.join();
    }

    // 所有 key 都被摘除
    for (int t = 0; t < kThreads; ++t) {
        const u64 base = static_cast<u64>(t) * 10000;
        for (int i = 0; i < kPerThread; ++i) {
            EXPECT_EQ(table.get(base + static_cast<u64>(i)), nullptr);
        }
    }
}

// ============================================================================
// remove 的 ABA 安全：摘除后旧值不再匹配（即使指针复用）
// ============================================================================

TEST(ConcurrentLong2ObjectHashTableTest, RemoveAfterReplaceByOtherThread)
{
    // 线程 A 插入 v1，线程 B 摘除 v1 并插入 v2，线程 A 试图用 v1 remove 应失败（值已变）
    ConcurrentLong2ObjectHashTable<ValPtr> table;
    auto v1 = std::make_shared<Value>(1);
    auto v2 = std::make_shared<Value>(2);

    table.putIfAbsent(100, v1);

    std::thread t([&] {
        // 摘除 v1，插入 v2
        EXPECT_TRUE(table.remove(100, v1));
        EXPECT_EQ(table.putIfAbsent(100, v2), nullptr);
    });
    t.join();

    // 此时 key 100 对应 v2，用 v1 remove 应失败（值校验防误删）
    EXPECT_FALSE(table.remove(100, v1));
    EXPECT_EQ(table.get(100), v2);
    EXPECT_TRUE(table.remove(100, v2));
}

// ============================================================================
// 并发 remove 相邻 key（同桶链表）：验证 remove 不留残留（prev 被并发摘除导致 target 残留的 bug）
//
// N 线程各插入一串 key（落在同桶），然后并发摘除各自的 key。
// 若 remove 有"prev 被并发摘除导致 target 残留"的 bug，摘除后某些 key 仍可 get 到（残留）。
// ============================================================================

TEST(ConcurrentLong2ObjectHashTableTest, ConcurrentRemoveAdjacentKeysNoResidue)
{
    // 选一组落在同一桶的 key（bucketIndex = (key ^ (key>>32)) & 4095）
    // 用 key = base + i，base 选使得 (base+i) ^ ((base+i)>>32) & 4095 相同
    // 简单做法：用连续小 key（0..N），它们分散到不同桶；为强制同桶，
    // 选 key 使得 key & 4095 相同（bucketIndex = key ^ (key>>32)，对小 key key>>32=0，故 bucketIndex = key & 4095）
    // 所以 key = 4096 * t + offset 都落同桶（offset 固定，t 变化）。但 key>>32 对这些 key 仍为 0（key < 2^32）。
    // 用 key = offset, offset+4096, offset+8192, ...（同桶 offset）
    constexpr int kThreads = 8;
    constexpr int kPerThread = 50;
    constexpr u64 kBucketStep = 4096; // 同桶 key 间隔
    constexpr u64 kBaseOffset = 7;    // 桶内偏移

    for (int round = 0; round < 200; ++round) {
        ConcurrentLong2ObjectHashTable<ValPtr> table;

        // 每个线程插入 kPerThread 个 key（同桶，相邻链表位置），记录 value
        std::vector<std::vector<ValPtr>> values(kThreads);
        std::vector<std::vector<u64>> keys(kThreads);
        for (int t = 0; t < kThreads; ++t) {
            for (int i = 0; i < kPerThread; ++i) {
                u64 key =
                    kBaseOffset + static_cast<u64>(t) * kPerThread * kBucketStep + static_cast<u64>(i) * kBucketStep;
                // 确保同桶：bucketIndex = key ^ (key>>32) = key (key < 2^32)，& 4095 = key & 4095 = kBaseOffset
                auto v = std::make_shared<Value>(static_cast<int>(key));
                values[t].push_back(v);
                keys[t].push_back(key);
                ASSERT_EQ(table.putIfAbsent(key, v), nullptr) << "round " << round << " t " << t << " i " << i;
            }
        }

        // 并发摘除各自的 key
        std::vector<std::thread> threads;
        std::atomic<int> failures{0};
        for (int t = 0; t < kThreads; ++t) {
            threads.emplace_back([&, t] {
                for (int i = 0; i < kPerThread; ++i) {
                    if (!table.remove(keys[t][i], values[t][i])) {
                        ++failures;
                    }
                }
            });
        }
        for (auto& th : threads) {
            th.join();
        }

        // 所有 key 应已摘除（get 返回 nullptr）
        // 若 remove 有残留 bug，某些 key 仍可 get 到
        int residue = 0;
        for (int t = 0; t < kThreads; ++t) {
            for (int i = 0; i < kPerThread; ++i) {
                ValPtr got = table.get(keys[t][i]);
                if (got != nullptr) {
                    ++residue;
                }
            }
        }
        EXPECT_EQ(residue, 0) << "round " << round << " remove 残留 " << residue
                              << " 个 key，failures=" << failures.load();
    }
}
