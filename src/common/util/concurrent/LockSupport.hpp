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
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace mc::util {

/**
 * @brief 线程阻塞原语，对齐 Java `java.util.concurrent.locks.LockSupport`
 *
 * 每个线程持有一个"许可"（permit，至多一个）。语义：
 * - `unpark(thread)`：将目标线程的 permit 置为 1（若已为 1 则保持 1，不累加）。
 *   若目标线程正在 `park` 阻塞，唤醒之。`unpark` 可在 `park` 之前调用——permit 会持久保留，
 *   随后的 `park` 会立即消费 permit 并返回，不阻塞。
 * - `park()`：若当前线程 permit 为 1，消费之并立即返回；否则阻塞，直到被 `unpark` 或超时或虚假唤醒。
 *   与 Java 一致，`park` 可能伪唤醒（spurious wakeup），调用方必须在循环中检查条件。
 *
 * 这套语义是 ReentrantAreaLock 无锁等待协议的基础：等待线程 `park`，持有者 `unpark` 唤醒，
 * permit 的持久性保证 `unpark` 先于 `park` 时不丢唤醒。
 *
 * ## 线程身份与生命周期
 *
 * `ThreadHandle` 由 `LockSupport::currentThread()` 获取，返回 `shared_ptr<ThreadHandle>`。
 * 句柄用 `shared_ptr` 持有（对齐 Java `Thread` 的 GC 引用语义），保证：线程 A 将自身入队（等待 Node 的
 * `MultiThreadedQueue`）后 `park`，线程 B（unlock 侧）从队列取出 A 的句柄并 `unpark`——即便 A 的 thread_local
 * 引用随线程退出析构，队列持有的 `shared_ptr` 仍保持 A 的 ThreadHandle 存活，避免 use-after-free。
 *
 * 线程退出时 thread_local 引用析构，从全局注册表移除该线程的条目；若仍有队列引用，ThreadHandle 延迟到
 * 最后一个引用释放。
 */
class LockSupport {
public:
    /**
     * @brief 线程阻塞句柄，持有 per-thread permit
     *
     * 不可直接构造，通过 `LockSupport::currentThread()` 获取。
     * `permit` 用 atomic 保证 `unpark` 与 `park` 之间的 happens-before。
     */
    class ThreadHandle {
    public:
        ThreadHandle(const ThreadHandle&) = delete;
        ThreadHandle& operator=(const ThreadHandle&) = delete;
        ThreadHandle(ThreadHandle&&) = delete;
        ThreadHandle& operator=(ThreadHandle&&) = delete;

        ~ThreadHandle() = default;

        /**
         * @brief 该句柄对应的线程 id（诊断/断言用）
         */
        [[nodiscard]] std::thread::id threadId() const { return m_threadId; }

        /**
         * @brief 构造句柄（仅用于 std::make_shared，构造为 public 因 MSVC 的 std::make_shared
         *        内部 _Ref_count_obj2 无法访问 private 构造函数）
         *
         * 调用方应通过 `LockSupport::currentThread()` 获取句柄，不应直接构造。
         */
        explicit ThreadHandle(std::thread::id id)
            : m_threadId(id)
        {}

    private:
        friend class LockSupport;

        /**
         * @brief 置 permit 为 1 并唤醒正在 park 的线程
         *
         * 线程安全：可被任意线程调用。
         * 先 `exchange(1)`（保证 permit 可见），若 permit 已为 1 直接返回（不累加）；
         * 否则持锁 notify_one。exchange(seq_cst) 提供 happens-before，避免 lost wakeup。
         */
        void unpark()
        {
            if (m_permit.exchange(1, std::memory_order::seq_cst) == 1) {
                return; // 已有 permit，无需 notify
            }
            {
                std::lock_guard<std::mutex> guard(m_mutex);
                // permit 已置 1，持锁以保证与 park 的 wait 之间的 happens-before
            }
            m_cv.notify_one();
        }

        std::thread::id m_threadId;
        std::atomic<int> m_permit{0};
        std::mutex m_mutex;
        std::condition_variable m_cv;
    };

    /**
     * @brief 获取当前线程的 ThreadHandle（thread_local 缓存，首次调用注册）
     *
     * @return 当前线程的 ThreadHandle 的 shared_ptr，线程退出前一直有效；
     *         其他线程持有 shared_ptr 期间句柄保持存活
     */
    [[nodiscard]] static std::shared_ptr<ThreadHandle> currentThread()
    {
        // thread_local shared_ptr：每线程一个句柄，首次访问构造，线程退出析构。
        // 构造时注册到全局表（用 weak_ptr，不延长线程句柄生命周期）。
        thread_local std::shared_ptr<ThreadHandle> handle = createCurrentThreadHandle();
        return handle;
    }

    /**
     * @brief 阻塞当前线程，直到被 unpark 或虚假唤醒
     *
     * 若当前线程 permit 为 1，消费之并立即返回；否则阻塞。
     * 调用方必须在循环中检查阻塞条件（伪唤醒可能）。
     */
    static void park() { parkInternal(nullptr); }

    /**
     * @brief 阻塞当前线程最多 nanos 纳秒，或直到被 unpark
     *
     * @param nanos 最大阻塞时间（纳秒），<=0 时立即返回
     */
    static void parkNanos(i64 nanos)
    {
        if (nanos <= 0) {
            return;
        }
        parkInternal(&nanos);
    }

    /**
     * @brief 唤醒目标线程（置 permit 为 1 并 notify）
     *
     * @param thread 目标线程的 ThreadHandle，可为 nullptr（no-op）
     *
     * 重载接收 shared_ptr，便于调用方持有强引用时使用。
     */
    static void unpark(const std::shared_ptr<ThreadHandle>& thread)
    {
        if (thread != nullptr) {
            thread->unpark();
        }
    }

    /**
     * @brief 唤醒目标线程（裸指针重载）
     *
     * @param thread 目标线程的 ThreadHandle 裸指针，可为 nullptr（no-op）
     *
     * ReentrantAreaLock 的等待队列 `MultiThreadedQueue<ThreadHandle*>` 存储裸指针
     * （对齐 Moonrise `MultiThreadedQueue<Thread>` 存储 Thread 引用）。裸指针生命周期由
     * ReentrantAreaLock 的 park 协议保证：被 park 的线程在队列中期间不会退出（park 阻塞，
     * 无法执行到线程结束），与 Java GC 保持 Thread 存活语义等价。unpark 在 pollOrBlockAdds
     * 取出句柄后立即调用，被唤醒线程在 unpark 完成前不会退出。
     */
    static void unpark(ThreadHandle* thread)
    {
        if (thread != nullptr) {
            thread->unpark();
        }
    }

private:
    LockSupport() = delete;

    /**
     * @brief park 的内部实现
     *
     * @param nanos nullptr 表示无限阻塞；非 nullptr 表示最多阻塞 *nanos 纳秒
     *
     * 实现要点（对齐 Java LockSupport 的 permit 语义）：
     * 1. 先 try_consume permit：若为 1 则 exchange(0) 消费并立即返回（覆盖 unpark 先于 park 的情况）。
     * 2. permit 为 0：持锁 wait。wait 返回后再次检查 permit（处理虚假唤醒 / notify 但 permit 未到）。
     *    循环直到 permit 被消费或超时。
     *
     * permit 的 exchange 提供 happens-before；mutex/cv 仅用于阻塞/唤醒，不保护 permit（permit 是 atomic）。
     */
    static void parkInternal(const i64* nanos)
    {
        std::shared_ptr<ThreadHandle> handle = currentThread();

        // 快速路径：permit 为 1 则立即消费返回
        if (handle->m_permit.exchange(0, std::memory_order::seq_cst) == 1) {
            return;
        }

        // 慢速路径：阻塞等待
        std::unique_lock<std::mutex> guard(handle->m_mutex);
        // 持锁后再次检查 permit，避免 unpark 在我们 exchange(0) 与 lock 之间置 1 丢失
        // （exchange(0) 看到 0，unpark exchange(1) 置 1，我们持锁后再检查看到 1 → 消费返回）
        while (handle->m_permit.load(std::memory_order::seq_cst) == 0) {
            if (nanos != nullptr) {
                if (handle->m_cv.wait_for(guard, std::chrono::nanoseconds(*nanos)) == std::cv_status::timeout) {
                    // 超时：消费可能刚到的 permit（避免 unpark 后的超时丢失 permit）
                    handle->m_permit.store(0, std::memory_order::seq_cst);
                    return;
                }
            } else {
                handle->m_cv.wait(guard);
            }
        }
        // permit 为 1，消费之
        handle->m_permit.store(0, std::memory_order::seq_cst);
    }

    /**
     * @brief 创建当前线程的 ThreadHandle 并注册到全局表（weak_ptr，不延长生命周期）
     *
     * thread_local shared_ptr 析构时（线程退出）从注册表移除条目。
     * 注册表当前仅用于诊断，唤醒逻辑不依赖遍历注册表。
     */
    static std::shared_ptr<ThreadHandle> createCurrentThreadHandle()
    {
        auto handle = std::make_shared<ThreadHandle>(std::this_thread::get_id());
        std::lock_guard<std::mutex> guard(registryMutex());
        registry()[std::this_thread::get_id()] = handle;
        return handle;
    }

    static std::mutex& registryMutex()
    {
        static std::mutex mutex;
        return mutex;
    }

    static std::unordered_map<std::thread::id, std::weak_ptr<ThreadHandle>>& registry()
    {
        static std::unordered_map<std::thread::id, std::weak_ptr<ThreadHandle>> map;
        return map;
    }
};

} // namespace mc::util
