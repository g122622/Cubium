/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the the use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT ANY WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, IN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
 * THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"

#include <array>
#include <atomic>
#include <memory>

namespace mc::util {

/**
 * @brief 无锁并发哈希表（u64 key → 对象指针），对齐 Moonrise `ConcurrentChainedLong2ReferenceHashTable`
 *
 * Moonrise 用模板生成的带 resize 的无锁链式哈希表。C++ 无 GC，完整移植无锁链表需解决 ABA 问题
 * （remove 时 CAS 摘除节点，节点内存可能被重用导致 ABA）。本实现采用 **固定桶 + 每桶链表 +
 * `std::shared_ptr<BucketNode>` 链表节点 + `std::atomic<std::shared_ptr>` 桶头/next** 的方案：
 * shared_ptr 的引用计数天然解决 ABA（节点在任意线程持有引用期间不会被释放），保证 CAS 正确性。
 *
 * ## 并发正确性：逻辑删除 + 物理摘除（不复用节点）
 *
 * remove 分两步：(1) 逻辑删除——CAS value 从 expected 到 nullptr，使节点对外不可见（get/putIfAbsent
 * 跳过）；(2) 物理摘除——CAS 绕过节点（从链表移除）。逻辑删除保证正确性（即使物理摘除失败，节点
 * 仍不可见），物理摘除回收链表空间。
 *
 * **不复用逻辑删除节点**：putIfAbsent 遇到 value==null 的节点直接跳过（继续遍历或插新节点到头），
 * 不 CAS 复用。这是正确性的关键——若 putIfAbsent 复用逻辑删除节点（null→value），而该节点正被
 * 另一线程的 physicallyUnlink 摘除，会产生竞态：physicallyUnlink 校验 m_value==null 通过后，
 * putIfAbsent CAS 复用（m_value 变为 value），physicallyUnlink 仍摘除节点，丢失复用线程的活跃节点。
 * 不复用则逻辑删除节点始终保持 m_value==null，physicallyUnlink 可安全摘除。
 *
 * 不复用带来的链表增长有界：同一 key 最多有一个活跃节点 + 瞬时逻辑删除节点（remove 的逻辑删除
 * 与物理摘除之间）。remove 物理摘除后节点出链，putIfAbsent 插入新头节点。get/remove 遍历时跳过
 * 逻辑删除节点（value==null），找到活跃节点。
 *
 * ## 物理摘除不重试（避免高争用活锁）
 *
 * physicallyUnlink 单次遍历找到 target 的前驱 prev，CAS 绕过，**失败直接返回不重试**。重试整条
 * 链表在多线程同桶下会反复 CAS 失败导致活锁。逻辑删除已保证正确性，物理摘除失败仅留逻辑删除
 * 节点（对 get/putIfAbsent 不可见），后续 remove 遍历可再次尝试摘除。
 *
 * 仅提供 ReentrantAreaLock 需要的最小 API：
 * - `putIfAbsent(key, value)`：若 key 不存在活跃节点则插入新节点到头，返回 nullptr；若存在返回旧值。
 * - `remove(key, expected)`：仅当 key 对应的活跃节点值 == expected 才逻辑删除 + 物理摘除，返回是否成功。
 * - `get(key)`：返回活跃节点值或 nullptr。
 *
 * 不做 resize（固定 4096 桶）。区块 section 数量有上界（活跃区块范围），固定桶 + 链表足够。
 *
 * @tparam T 值类型，通常是指针类型（如 `ReentrantAreaLock::Node*`）
 */
template <typename T>
class ConcurrentLong2ObjectHashTable {
public:
    ConcurrentLong2ObjectHashTable() = default;

    ~ConcurrentLong2ObjectHashTable() = default;

    ConcurrentLong2ObjectHashTable(const ConcurrentLong2ObjectHashTable&) = delete;
    ConcurrentLong2ObjectHashTable& operator=(const ConcurrentLong2ObjectHashTable&) = delete;
    ConcurrentLong2ObjectHashTable(ConcurrentLong2ObjectHashTable&&) = delete;
    ConcurrentLong2ObjectHashTable& operator=(ConcurrentLong2ObjectHashTable&&) = delete;

    /**
     * @brief 若 key 不存在活跃节点则插入 value，返回旧值
     *
     * @param key 键
     * @param value 要插入的值（不可为 nullptr，nullptr 表示逻辑删除）
     * @return nullptr 表示插入成功；非空表示 key 已存在活跃节点，返回现有值
     *
     * 遍历链表查找 key 的活跃节点（value != null）：
     * - 找到 key 且 value != null：返回 value（key 已存在）
     * - 找到 key 且 value == null（逻辑删除）：跳过，继续遍历（不复用）
     * - 遍历完未找到活跃节点：CAS 插入新节点到桶头
     */
    T putIfAbsent(u64 key, T value)
    {
        const std::size_t idx = bucketIndex(key);
        auto& head = m_buckets[idx];
        for (;;) {
            // headSnapshot：本次遍历开始时的桶头，用于无活跃节点时 CAS 插入新节点到头。
            // 遍历仅用于查找活跃节点，不消费 headSnapshot——新节点 m_next 指向 headSnapshot，
            // CAS head 从 headSnapshot 到 newNode。若遍历后 headSnapshot 已过期（其他线程改了头），
            // CAS 失败重试。
            std::shared_ptr<BucketNode> headSnapshot = head.load(std::memory_order_seq_cst);
            std::shared_ptr<BucketNode> curr = headSnapshot;
            while (curr != nullptr) {
                if (curr->m_key == key) {
                    T existing = curr->m_value.load(std::memory_order_seq_cst);
                    if (existing != nullptr) {
                        return existing; // key 已存在活跃节点
                    }
                    // existing == null（逻辑删除节点）：跳过，继续遍历（不复用）
                }
                curr = curr->m_next.load(std::memory_order_seq_cst);
            }
            // 遍历完未找到活跃节点，CAS 插入新节点到链表头
            auto newNode = std::make_shared<BucketNode>(key, value, headSnapshot);
            if (head.compare_exchange_strong(
                    headSnapshot, newNode, std::memory_order_seq_cst, std::memory_order_seq_cst)) {
                return nullptr; // 插入成功
            }
            // CAS 失败（头已变），重试
        }
    }

    /**
     * @brief 仅当 key 对应的活跃节点值 == expected 时逻辑删除 + 物理摘除，返回是否成功
     *
     * @param key 键
     * @param expected 期望的当前值（指针相等比较）
     * @return true 删除成功，false（key 不存在活跃节点 / 值不匹配）
     *
     * 遍历链表跳过逻辑删除节点（value==null），找到 key 的活跃节点（value==expected）：
     * - 值匹配：CAS value 从 expected 到 null（逻辑删除），然后 physicallyUnlink 物理摘除，返回 true
     * - 值不匹配（value==null 或其他值）：继续遍历找下一个同 key 节点（可能有逻辑删除 + 活跃两个同 key 节点）
     * - 遍历完未找到匹配：返回 false
     */
    bool remove(u64 key, T expected)
    {
        const std::size_t idx = bucketIndex(key);
        auto& head = m_buckets[idx];
        std::shared_ptr<BucketNode> curr = head.load(std::memory_order_seq_cst);
        while (curr != nullptr) {
            if (curr->m_key == key) {
                T existing = curr->m_value.load(std::memory_order_seq_cst);
                if (existing == expected) {
                    // 找到活跃节点（value == expected）：逻辑删除 CAS value → null
                    T expectedVal = expected;
                    if (curr->m_value.compare_exchange_strong(
                            expectedVal, nullptr, std::memory_order_seq_cst, std::memory_order_seq_cst)) {
                        // 逻辑删除成功，物理摘除（尽力，失败留逻辑删除节点，对 get/putIfAbsent 不可见）
                        physicallyUnlink(idx, head, curr);
                        return true;
                    }
                    // CAS 失败（值已变，其他线程改了），继续遍历找下一个同 key 节点
                }
                // existing != expected（逻辑删除或值不匹配）：继续遍历找下一个同 key 节点
            }
            curr = curr->m_next.load(std::memory_order_seq_cst);
        }
        return false; // 未找到匹配的活跃节点
    }

    /**
     * @brief 查询 key 对应的活跃节点值
     *
     * @param key 键
     * @return 活跃节点值或 nullptr（key 不存在或所有同 key 节点均逻辑删除）
     */
    T get(u64 key) const
    {
        const std::size_t idx = bucketIndex(key);
        auto& head = m_buckets[idx];
        std::shared_ptr<BucketNode> scan = head.load(std::memory_order_seq_cst);
        while (scan != nullptr) {
            if (scan->m_key == key) {
                T value = scan->m_value.load(std::memory_order_seq_cst);
                if (value != nullptr) {
                    return value; // 活跃节点
                }
                // 逻辑删除节点，继续遍历找下一个同 key 节点
            }
            scan = scan->m_next.load(std::memory_order_seq_cst);
        }
        return nullptr;
    }

private:
    /**
     * @brief 桶节点
     *
     * m_value 用 nullptr 哨兵表示"逻辑删除"（remove 先置 null，再物理摘除）。
     * m_next 是 shared_ptr，引用计数保证 CAS 期间的 ABA 安全。
     */
    struct BucketNode {
        u64 m_key;
        std::atomic<T> m_value;
        std::atomic<std::shared_ptr<BucketNode>> m_next;

        BucketNode(u64 key, T value, std::shared_ptr<BucketNode> next)
            : m_key(key)
            , m_value(value)
            , m_next(std::move(next))
        {}

        BucketNode(const BucketNode&) = delete;
        BucketNode& operator=(const BucketNode&) = delete;
    };

    static constexpr std::size_t kBucketCount = 4096;

    static std::size_t bucketIndex(u64 key)
    {
        return static_cast<std::size_t>((key ^ (key >> 32))) & (kBucketCount - 1);
    }

    /**
     * @brief 尽力物理摘除 target（逻辑删除后的清理）
     *
     * 单次遍历找到 target 的前驱 prev，CAS 绕过。**不重试**：失败（prev->next 已变、head 已变、
     * target 已被摘除）直接返回。逻辑删除已使 target 对外不可见，物理摘除仅是链表清理。
     *
     * **不复用保证安全**：putIfAbsent 不复用逻辑删除节点，故 target 一旦逻辑删除（m_value==null）
     * 不会复活，physicallyUnlink 可安全摘除。校验 m_value==null 是防御性检查（理论上始终成立）。
     *
     * 不重试避免高争用下活锁：8 线程同桶 400 节点下 head CAS 反复失败、无限重试导致活锁。
     * 单次尽力摘除足够：正确性由逻辑删除保证，残留逻辑删除节点对 get/putIfAbsent 不可见。
     */
    void physicallyUnlink(std::size_t idx, auto& head, const std::shared_ptr<BucketNode>& target)
    {
        // 防御性校验：target 仍处于逻辑删除状态。若被复用（m_value != null）则不能摘除。
        // 不复用设计下此条件始终成立，但保留校验防止未来误改。
        if (target->m_value.load(std::memory_order_seq_cst) != nullptr) {
            return;
        }
        std::shared_ptr<BucketNode> curr = head.load(std::memory_order_seq_cst);
        std::shared_ptr<BucketNode> prev;
        while (curr != nullptr) {
            if (curr.get() == target.get()) {
                std::shared_ptr<BucketNode> targetNext = curr->m_next.load(std::memory_order_seq_cst);
                if (prev == nullptr) {
                    // target 是头节点：CAS head 绕过。失败则放弃（其他线程已改 head）
                    head.compare_exchange_strong(
                        curr, targetNext, std::memory_order_seq_cst, std::memory_order_seq_cst);
                } else {
                    // 校验 prev->next 仍指向 target（prev 未脱链），CAS 绕过。失败则放弃
                    std::shared_ptr<BucketNode> predNext = prev->m_next.load(std::memory_order_seq_cst);
                    if (predNext.get() == target.get()) {
                        prev->m_next.compare_exchange_strong(
                            predNext, targetNext, std::memory_order_seq_cst, std::memory_order_seq_cst);
                    }
                }
                return; // 单次尽力，不重试
            }
            prev = curr;
            curr = curr->m_next.load(std::memory_order_seq_cst);
        }
        // target 未找到（已被其他线程物理摘除），无需操作
    }

    std::array<std::atomic<std::shared_ptr<BucketNode>>, kBucketCount> m_buckets{};
};

} // namespace mc::util
