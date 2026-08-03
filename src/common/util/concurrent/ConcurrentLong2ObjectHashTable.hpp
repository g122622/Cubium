/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction to the use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND OR EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, IN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, IN CONNECTION WITH
 * THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <mutex>

namespace mc::util {

/**
 * @brief 并发哈希表（u64 key → 对象指针），对齐 Moonrise `ConcurrentChainedLong2ReferenceHashTable`
 *
 * Moonrise 的 `ConcurrentChainedLong2ReferenceHashTable` 用 **每桶 `synchronized(node)` 分段锁**
 * （见 templates/ConcurrentChainedHashTable.java.txt：putIfAbsent/remove/get 在 `synchronized(node)` 块内
 * 遍历链表，物理摘除在锁内完成）。本实现忠实对齐这一设计：
 *
 * - 固定 4096 桶，每桶一个 `std::mutex` + 链表头（`std::unique_ptr<BucketNode>`）。
 * - `putIfAbsent`/`remove`/`get` 加桶锁后遍历链表，物理摘除在锁内完成（无逻辑删除节点堆积）。
 * - 不同桶完全并行；同桶操作串行化在桶锁上（争用时阻塞睡眠，而非忙等）。
 *
 * ## 为何不用无锁（std::atomic<shared_ptr>）方案
 *
 * 先前版本用 `std::atomic<std::shared_ptr<BucketNode>>` 桶头/next + 逻辑删除 + 尽力物理摘除（不重试）。
 * 问题：
 * 1. `std::atomic<shared_ptr>` 在 MSVC 下每个原子操作持对象内部 mutex，高争用下竞争该 mutex；
 * 2. `physicallyUnlink` 不重试 → 逻辑删除节点堆积 → 桶链变长 → get/putIfAbsent 遍历变慢（CPU 升高）；
 * 3. putIfAbsent 的 CAS 重试（head 变化）+ 链遍历无退避 → 高争用忙等（CPU 100%）。
 *
 * 在 ReentrantAreaLock 的 `onChunkGenComplete` 持 2*maxAccessRadius（2025 sections）巨型锁场景下，
 * 多 worker 并发 EMPTY 生成争用同批 section，无锁方案的忙等 + 链堆积导致 livelock（CPU 单核吃满，
 * 进度极慢）。每桶 mutex 分段锁将争用从"全局单锁"降到"每桶锁"（4096 桶），同桶争用时阻塞睡眠
 * （不占 CPU），与 Moonrise 原版语义一致，且最接近重构前的 mutex+condition_variable 基线行为。
 *
 * ## ABA 与生命周期
 *
 * 桶锁内操作是单线程串行的（同桶任一时刻只有一个线程在修改链表），物理摘除在锁内完成，
 * 节点出链后不会再被任何线程访问（值 T 由调用方 shared_ptr 保活，链表节点由 unique_ptr 管理）。
 * 无 ABA 问题，无逻辑删除堆积。
 *
 * ## resize
 *
 * 不做 resize（固定 4096 桶）。区块 section 数量有上界（活跃区块范围），固定桶 + 链表足够。
 * Moonrise 的 resize 在分段锁下也是 synchronized(binNode) 迁移，本实现省略 resize 简化设计。
 *
 * @tparam T 值类型，通常是指针类型（如 `ReentrantAreaLock::Node` 的 `shared_ptr`）
 */
template <typename T>
class ConcurrentLong2ObjectHashTable {
public:
    ConcurrentLong2ObjectHashTable() = default;

    // 迭代释放各桶链表，避免 ~unique_ptr 递归析构长链导致栈溢出。
    ~ConcurrentLong2ObjectHashTable()
    {
        for (Bucket& bucket : m_buckets) {
            // 迭代释放链表节点，避免递归 ~unique_ptr 在长链上栈溢出
            while (bucket.head) {
                bucket.head = std::move(bucket.head->next);
            }
        }
    }

    ConcurrentLong2ObjectHashTable(const ConcurrentLong2ObjectHashTable&) = delete;
    ConcurrentLong2ObjectHashTable& operator=(const ConcurrentLong2ObjectHashTable&) = delete;
    ConcurrentLong2ObjectHashTable(ConcurrentLong2ObjectHashTable&&) = delete;
    ConcurrentLong2ObjectHashTable& operator=(ConcurrentLong2ObjectHashTable&&) = delete;

    /**
     * @brief 若 key 不存在则插入 value，返回旧值
     *
     * @param key 键
     * @param value 要插入的值（不可为 nullptr，nullptr 在语义上表示"不存在"）
     * @return nullptr 表示插入成功；非空表示 key 已存在，返回现有值
     *
     * 加桶锁后遍历链表查找 key：找到返回现有值；未找到则头插新节点。
     */
    T putIfAbsent(u64 key, T value)
    {
        const std::size_t idx = bucketIndex(key);
        Bucket& bucket = m_buckets[idx];
        std::lock_guard<std::mutex> guard(bucket.mutex);
        // 遍历链表查找 key
        for (BucketNode* curr = bucket.head.get(); curr != nullptr; curr = curr->next.get()) {
            if (curr->key == key) {
                return curr->value; // key 已存在，返回现有值
            }
        }
        // 未找到：头插新节点
        auto node = std::make_unique<BucketNode>(key, std::move(value), std::move(bucket.head));
        bucket.head = std::move(node);
        return nullptr;
    }

    /**
     * @brief 仅当 key 对应的当前值 == expected 时移除，返回是否成功
     *
     * @param key 键
     * @param expected 期望的当前值（指针相等比较）
     * @return true 删除成功，false（key 不存在 / 值不匹配）
     *
     * 加桶锁后遍历链表，找到 key 且值匹配则物理摘除（锁内完成，无堆积）。
     */
    bool remove(u64 key, T expected)
    {
        const std::size_t idx = bucketIndex(key);
        Bucket& bucket = m_buckets[idx];
        std::lock_guard<std::mutex> guard(bucket.mutex);
        BucketNode* prev = nullptr;
        for (BucketNode* curr = bucket.head.get(); curr != nullptr; prev = curr, curr = curr->next.get()) {
            if (curr->key == key) {
                if (curr->value != expected) {
                    return false; // 值不匹配
                }
                // 物理摘除：prev->next 绕过 curr（或 head 绕过 curr）
                if (prev == nullptr) {
                    bucket.head = std::move(curr->next);
                } else {
                    prev->next = std::move(curr->next);
                }
                return true; // curr 由 unique_ptr 析构释放
            }
        }
        return false; // key 不存在
    }

    /**
     * @brief 查询 key 对应的值
     *
     * @param key 键
     * @return 值或 nullptr（key 不存在）
     *
     * 加桶锁后遍历链表查找。返回值的拷贝（T 通常为 shared_ptr，引用计数 +1）。
     */
    T get(u64 key) const
    {
        const std::size_t idx = bucketIndex(key);
        const Bucket& bucket = m_buckets[idx];
        std::lock_guard<std::mutex> guard(bucket.mutex);
        for (BucketNode* curr = bucket.head.get(); curr != nullptr; curr = curr->next.get()) {
            if (curr->key == key) {
                return curr->value;
            }
        }
        return nullptr;
    }

private:
    /**
     * @brief 桶节点（链表）
     *
     * value 为 T 的拷贝（shared_ptr 时引用计数 +1）。next 由 unique_ptr 链式持有，
     * 析构递归释放整条链。锁内操作保证节点生命周期安全。
     */
    struct BucketNode {
        u64 key;
        T value;
        std::unique_ptr<BucketNode> next;

        BucketNode(u64 k, T v, std::unique_ptr<BucketNode> n)
            : key(k)
            , value(std::move(v))
            , next(std::move(n))
        {}

        BucketNode(const BucketNode&) = delete;
        BucketNode& operator=(const BucketNode&) = delete;
    };

    /**
     * @brief 桶：mutex + 链表头
     *
     * 每桶独立锁，不同桶完全并行。mutable mutex 使 get（const）也能加锁。
     */
    struct Bucket {
        mutable std::mutex mutex;
        std::unique_ptr<BucketNode> head;
    };

    static constexpr std::size_t kBucketCount = 4096;

    static std::size_t bucketIndex(u64 key)
    {
        return static_cast<std::size_t>((key ^ (key >> 32))) & (kBucketCount - 1);
    }

    std::array<Bucket, kBucketCount> m_buckets{};
};

} // namespace mc::util
