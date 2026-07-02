/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, without limitation the use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to furnish
 * the same to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"

#include <atomic>

namespace mc::util {

/**
 * @brief 无锁单链表 FIFO 队列，对齐 Moonrise `MultiThreadedQueue`
 *
 * 只移植 ReentrantAreaLock 实际用到的子集（add/forceAdd/preventAdds/allowAdds/pollOrBlockAdds/isAddBlocked）。
 * 高并发读写性能优于 `std::mutex` 保护的队列。
 *
 * ## 元素类型约束
 *
 * `E` 必须是指针类型（实际只用 `LockSupport::ThreadHandle*`）。`nullptr` 作为哨兵值（对应 Java 的 null），
 * 表示"无元素"。`std::atomic<E>` 要求 E 是 trivially copyable，指针类型满足。
 *
 * ## 生命周期约束（重要）
 *
 * 元素的生命周期由调用方保证：元素在队列中期间不得被销毁。对 `ThreadHandle*` 而言，
 * ReentrantAreaLock 的等待协议保证：被 park 的线程在持有 Node 等待队列成员身份期间不会退出
 * （park 阻塞，无法继续执行到线程结束）。这与 Java `MultiThreadedQueue<Thread>` 的 GC 语义等价。
 *
 * ## add-blocking 协议（preventAdds / pollOrBlockAdds）
 *
 * 队列支持"阻止入队"状态：`preventAdds` 入队一个死结节点（`next` 指向自身），后续 `add` 失败返回 false。
 * `pollOrBlockAdds` 原子地"取队首 or 阻止入队"：队首非空则取走；为空则 CAS 把尾的 `next` 从 null 置为自环，
 * 阻止后续 add 并返回 nullptr。这套协议是 ReentrantAreaLock 无锁唤醒的核心：
 * unlock 侧 `pollOrBlockAdds` 排空等待线程并阻止新等待者入队，与 lock 侧 `add` 的 CAS 竞争，保证不丢唤醒。
 *
 * ## 内存序（对齐 Java VarHandle）
 *
 * - `get`/`set`（plain）→ `memory_order_relaxed`
 * - `getOpaque`/`setOpaque` → `memory_order_relaxed`
 * - `getVolatile`/`setVolatile`/`getAndSet`/`compareAndExchange` → `memory_order_seq_cst`
 * - `getAcquire` → `memory_order_acquire`
 */
template <typename E>
class MultiThreadedQueue {
public:
    MultiThreadedQueue()
        : m_head(new LinkedNode(E{}, nullptr))
        , m_tail(m_head.load(std::memory_order::relaxed))
    {}

    ~MultiThreadedQueue()
    {
        // 析构：从头遍历释放所有 LinkedNode（含死结节点）
        LinkedNode* curr = m_head.load(std::memory_order::acquire);
        while (curr != nullptr) {
            LinkedNode* nextRaw = curr->m_next.load(std::memory_order::acquire);
            LinkedNode* next = (nextRaw == curr) ? nullptr : nextRaw; // 死结（自环）→ 停止
            delete curr;
            curr = next;
        }
    }

    MultiThreadedQueue(const MultiThreadedQueue&) = delete;
    MultiThreadedQueue& operator=(const MultiThreadedQueue&) = delete;
    MultiThreadedQueue(MultiThreadedQueue&&) = delete;
    MultiThreadedQueue& operator=(MultiThreadedQueue&&) = delete;

    /**
     * @brief 向队尾添加元素
     *
     * 若队列处于 add-blocked 状态（preventAdds 已调用），返回 false 且不添加。
     * @return true 表示添加成功，false 表示队列 add-blocked
     */
    bool add(E element) { return offer(element); }

    /**
     * @brief 同 add（对齐 Queue.offer 语义）
     */
    bool offer(E element) { return appendList(element); }

    /**
     * @brief 强制添加元素，若队列处于 add-blocked 状态则先解除阻止再加入
     *
     * 用于 lock 重试路径：重试前 `allowAdds` 恢复入队能力。
     * @return true 表示队列之前处于 add-blocked 状态（现已解除并加入），false 表示之前允许添加
     *
     * 对齐 Moonrise `forceAdd`：`return !forceAppendList(node, node)`，
     * `forceAppendList` 返回 `next != curr`（true=之前允许 next==null，false=之前阻止 next==curr），
     * 故 `forceAdd` 取反：true=之前被阻止，false=之前允许。
     */
    bool forceAdd(E element)
    {
        LinkedNode* node = new LinkedNode(element, nullptr);
        LinkedNode* tail = node;
        return !forceAppendList(node, tail);
    }

    /**
     * @brief 阻止后续 add（入队死结节点）
     *
     * 入队一个 `next` 指向自身的死结节点，后续 `add` 遇到 `next == curr` 返回 false。
     * @return true 表示首次阻止，false 表示已被阻止
     */
    bool preventAdds()
    {
        auto* deadEnd = new LinkedNode(E{}, nullptr);
        deadEnd->m_next.store(deadEnd, std::memory_order::relaxed); // 自环
        if (!appendListInternal(deadEnd, deadEnd)) {
            delete deadEnd;
            return false;
        }
        // 尝试更新 tail 到 deadEnd，便于 allowAdds 找到
        m_tail.store(deadEnd, std::memory_order::release);
        return true;
    }

    /**
     * @brief 解除 add-blocked 状态
     *
     * 找到死结节点（next 指向自身），将其 next 置为 null，恢复入队能力。
     * 非 MT-Safe（对齐 Moonrise：调用方保证单线程调用，ReentrantAreaLock 在 lock 重试路径单线程调用）。
     */
    void allowAdds()
    {
        LinkedNode* tail = m_tail.load(std::memory_order::relaxed);
        // 沿 next 链找到真正的尾（tail 可能 stale）
        // 对齐 Moonrise: while (tail != (tail = tail.getNextPlain())) {}
        while (true) {
            LinkedNode* next = tail->m_next.load(std::memory_order::relaxed);
            if (next == tail || next == nullptr) {
                break;
            }
            tail = next;
        }
        // tail 此时指向死结节点（next 自环）或正常尾（next==null，未阻止）
        // 置 null 解除阻止（若已是 null 无害）
        tail->m_next.store(nullptr, std::memory_order::seq_cst);
    }

    /**
     * @brief 原子地"取队首 or 阻止入队"
     *
     * 队首非空→取走队首并返回；队列为空→CAS 把尾的 next 从 null 置为自环（阻止入队），返回 nullptr。
     * 若队列已 add-blocked 且空，返回 nullptr（no-op）。
     *
     * 这是 unlock 唤醒等待线程的核心：循环调用直到返回 nullptr，逐个 unpark 取出的线程。
     *
     * @return 队首元素，或 nullptr（队列已阻止入队）
     */
    E pollOrBlockAdds()
    {
        int failures = 0;
        for (LinkedNode *head = m_head.load(std::memory_order::acquire), *curr = head;;) {
            E currentVal = curr->m_element.load(std::memory_order::relaxed);
            LinkedNode* next = curr->m_next.load(std::memory_order::acquire);

            if (next == curr) {
                return nullptr; // 队列已 add-blocked
            }

            for (int i = 0; i < failures; ++i) {
                backoff();
            }

            if (currentVal != E{}) {
                // 原子取走：exchange(null)，若原值为 null 说明被其他 poll 抢先
                E oldVal = curr->m_element.exchange(E{}, std::memory_order::seq_cst);
                if (oldVal == E{}) {
                    // 被抢先，重试
                    ++failures;
                    continue;
                }
                // 成功取走，更新 head
                if (m_head.load(std::memory_order::acquire) == head) {
                    m_head.store((next != nullptr) ? next : curr, std::memory_order::release);
                }
                return oldVal;
            }

            if (next == nullptr) {
                // 队列为空，尝试 CAS 阻止入队：把 next 从 null 置为 curr（自环）
                if (curr != head && m_head.load(std::memory_order::acquire) == head) {
                    m_head.store(curr, std::memory_order::release);
                }
                LinkedNode* expected = nullptr;
                if (curr->m_next.compare_exchange_strong(
                        expected, curr, std::memory_order::seq_cst, std::memory_order::relaxed)) {
                    return nullptr; // 成功阻止入队
                }
                // CAS 失败（有 add），重试
                curr = expected;
                ++failures;
                continue;
            }

            if (head == curr) {
                // head 可能 stale，前进
                curr = next;
            } else {
                // 尝试更新 head
                LinkedNode* oldHead = head;
                head = m_head.load(std::memory_order::acquire);
                if (oldHead == head) {
                    curr = next;
                } else {
                    curr = head;
                }
            }
        }
    }

    /**
     * @brief 队列是否处于 add-blocked 状态
     */
    [[nodiscard]] bool isAddBlocked() const
    {
        for (LinkedNode* tail = m_tail.load(std::memory_order::acquire);;) {
            LinkedNode* next = tail->m_next.load(std::memory_order::acquire);
            if (next == nullptr) {
                return false;
            }
            if (next == tail) {
                return true;
            }
            tail = next;
        }
    }

private:
    /**
     * @brief 链表节点
     *
     * element 用 nullptr 哨兵表示"已取出/无元素"（对齐 Java null）。
     * next 为 nullptr 表示尾，next == this（自环）表示死结（add-blocked）。
     */
    struct LinkedNode {
        std::atomic<E> m_element;
        std::atomic<LinkedNode*> m_next;

        LinkedNode(E element, LinkedNode* next)
            : m_element(element)
            , m_next(next)
        {}

        LinkedNode(const LinkedNode&) = delete;
        LinkedNode& operator=(const LinkedNode&) = delete;
    };

    // head/tail 用 atomic 指针，对齐 Moonrise 的 volatile LinkedNode head/tail
    std::atomic<LinkedNode*> m_head;
    std::atomic<LinkedNode*> m_tail;

    /**
     * @brief 退避（对齐 Moonrise ConcurrentUtil.backoff = Thread.onSpinWait）
     *
     * x86 上用 _mm_pause 提示 CPU 流水线（减少争用功耗、避免内存顺序违例惩罚）。
     */
    static void backoff() { __builtin_ia32_pause(); }

    /**
     * @brief 内部入队：把单个元素追加到队列尾
     *
     * @return true 添加成功，false 队列 add-blocked
     * 对齐 Moonrise appendList。
     */
    bool appendList(E element)
    {
        auto* node = new LinkedNode(element, nullptr);
        LinkedNode* tail = node;
        return appendListInternal(node, tail);
    }

    /**
     * @brief 内部入队实现（对齐 Moonrise appendList）
     *
     * @param headNode 要追加的链表头
     * @param tailNode 要追加的链表尾（追加后成为新尾）
     * @return true 添加成功，false 队列 add-blocked
     */
    bool appendListInternal(LinkedNode* headNode, LinkedNode* tailNode)
    {
        int failures = 0;
        LinkedNode* currTail = m_tail.load(std::memory_order::acquire);
        LinkedNode* curr = currTail;
        for (;;) {
            LinkedNode* next = curr->m_next.load(std::memory_order::acquire);

            if (next == curr) {
                // add-blocked
                return false;
            }

            for (int i = 0; i < failures; ++i) {
                backoff();
            }

            if (next == nullptr) {
                // CAS 追加到尾
                LinkedNode* expected = nullptr;
                if (curr->m_next.compare_exchange_strong(
                        expected, headNode, std::memory_order::seq_cst, std::memory_order::relaxed)) {
                    // 追加成功，更新 tail（CAS 避免设置 stale tail）
                    if (m_tail.load(std::memory_order::acquire) == currTail) {
                        m_tail.store(tailNode, std::memory_order::release);
                    }
                    return true;
                }
                // CAS 失败，其他线程抢先追加
                ++failures;
                curr = expected;
                continue;
            }

            if (curr == currTail) {
                // tail 可能 stale，前进
                curr = next;
            } else {
                // 尝试更新到 tail
                LinkedNode* oldCurrTail = currTail;
                currTail = m_tail.load(std::memory_order::acquire);
                if (oldCurrTail == currTail) {
                    curr = next;
                } else {
                    curr = currTail;
                }
            }
        }
    }

    /**
     * @brief 强制入队实现（对齐 Moonrise forceAppendList）
     *
     * 与 appendListInternal 的区别：遇到死结（next == curr）时解除阻止（CAS next 从 curr 置 headNode）再追加。
     * @return true 队列之前允许添加（未阻止），false 之前被阻止（现已解除并加入）
     */
    bool forceAppendList(LinkedNode* headNode, LinkedNode* tailNode)
    {
        int failures = 0;
        LinkedNode* currTail = m_tail.load(std::memory_order::acquire);
        LinkedNode* curr = currTail;
        for (;;) {
            LinkedNode* next = curr->m_next.load(std::memory_order::acquire);

            for (int i = 0; i < failures; ++i) {
                backoff();
            }

            if (next == nullptr || next == curr) {
                // next==null：正常追加；next==curr：死结，强制解除并追加
                LinkedNode* expected = next;
                if (curr->m_next.compare_exchange_strong(
                        expected, headNode, std::memory_order::seq_cst, std::memory_order::relaxed)) {
                    // 追加成功，更新 tail
                    if (m_tail.load(std::memory_order::acquire) == currTail) {
                        m_tail.store(tailNode, std::memory_order::release);
                    }
                    return next != curr; // true=之前允许（next==null），false=之前阻止（next==curr）
                }
                // CAS 失败
                ++failures;
                curr = expected;
                continue;
            }

            if (curr == currTail) {
                curr = next;
            } else {
                LinkedNode* oldCurrTail = currTail;
                currTail = m_tail.load(std::memory_order::acquire);
                if (oldCurrTail == currTail) {
                    curr = next;
                } else {
                    curr = currTail;
                }
            }
        }
    }
};

} // namespace mc::util
