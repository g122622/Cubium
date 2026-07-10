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
 * LIABILITY, IN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/concurrent/ConcurrentLong2ObjectHashTable.hpp"
#include "common/util/concurrent/LockSupport.hpp"
#include "common/util/concurrent/MultiThreadedQueue.hpp"

#include <memory>
#include <thread>
#include <vector>

namespace mc::util {

class ReentrantAreaLock;

/**
 * @brief 锁节点，同时是该 Node 的等待线程队列
 *
 * 继承 `MultiThreadedQueue<ThreadHandle*>`：冲突线程把自身加入被冲突 Node 的队列，
 * unlock 时 pollOrBlockAdds 排空队列并 unpark。对齐 Moonrise `Node extends MultiThreadedQueue<Thread>`。
 *
 * ## 生命周期（C++ 无 GC，对齐 Moonrise 的 Thread 引用保活语义）
 *
 * Moonrise 依赖 Java GC 保证：阻塞线程持有冲突 Node 的引用（`park` 局部变量 + 等待队列成员）期间，
 * 持有线程释放该 Node 不会析构它（GC 延迟到所有引用消失）。C++ 用 `std::shared_ptr` 等价实现：
 *
 * - `m_nodes` 哈希表存储 `shared_ptr<ReentrantAreaLockNode>`，持有线程的 `LockHandle` 也持有 `shared_ptr`。
 * - `putIfAbsent` 返回 `shared_ptr`（冲突时返回占有者的 Node），阻塞线程持有该 `shared_ptr`（`park`），
 *   在 `add` + `park` 期间保活冲突 Node，防止持有线程 `~LockHandle`（unlock + 析构 `~MultiThreadedQueue` 链表）
 *   导致 use-after-free。
 * - 持有线程 `~LockHandle` 调用 `unlock`（从哈希表移除 + 排空等待队列 + unpark），再释放 `shared_ptr`。
 *   `~MultiThreadedQueue` 链表析构在最后一个 `shared_ptr` 释放时（所有阻塞线程已 release）才执行。
 * - `enable_shared_from_this` 让 `unlock` 从 `*this` 获取 `shared_ptr` 用于哈希表的值校验 remove。
 *
 * Node 不可拷贝、不可移动，只能由 `ReentrantAreaLock::lock` / `tryLock` 创建（通过 `shared_ptr` 持有）。
 */
class ReentrantAreaLockNode : public MultiThreadedQueue<LockSupport::ThreadHandle*>,
                              public std::enable_shared_from_this<ReentrantAreaLockNode> {
public:
    ReentrantAreaLockNode(const ReentrantAreaLockNode&) = delete;
    ReentrantAreaLockNode& operator=(const ReentrantAreaLockNode&) = delete;
    ReentrantAreaLockNode(ReentrantAreaLockNode&&) = delete;
    ReentrantAreaLockNode& operator=(ReentrantAreaLockNode&&) = delete;

    // ~Node 析构必须 public：shared_ptr<Node> 的删除器需要访问析构函数。
    // ~Node 不调用 unlock：unlock 由持有线程的 LockHandle 析构显式调用。
    // ~MultiThreadedQueue 链表析构在最后一个 shared_ptr 释放时执行（阻塞线程已 release）。
    ~ReentrantAreaLockNode() = default;

    /**
     * @brief 释放本 Node 占有的所有 section（从哈希表移除 + 排空等待队列 + unpark）
     *
     * 由持有线程的 LockHandle 析构调用。幂等：areaAffectedLen==0 时直接返回。
     * 定义在 .cpp（此处 ReentrantAreaLock 仍是不完整类型，无法访问其成员）。
     */
    void unlock();

private:
    friend class ReentrantAreaLock;

    ReentrantAreaLockNode(ReentrantAreaLock& lock, std::vector<u64> areaAffected, std::thread::id thread)
        : m_lock(lock)
        , m_areaAffected(std::move(areaAffected))
        , m_areaAffectedLen(0)
        , m_thread(thread)
    {}

    ReentrantAreaLock& m_lock;
    std::vector<u64> m_areaAffected; ///< 该 Node 实际占有写入 map 的 section 键（[0, m_areaAffectedLen) 有效）
    u64 m_areaAffectedLen;           ///< m_areaAffected 中有效条目数
    std::thread::id m_thread;        ///< 持有线程
};

/**
 * @brief 按区块坐标分区的可重入区域锁
 *
 * 对齐 Moonrise 的 `ca.spottedleaf.concurrentutil.lock.ReentrantAreaLock`。
 *
 * 该锁按区块坐标区域（矩形 `[centerX±radius, centerZ±radius]`）加锁，保证同一时刻
 * 不存在两个线程持有"相交"的区域。同一线程可以对已持有的区域（或其子区域）重入加锁，
 * 不会被自身阻塞。
 *
 * ## 设计要点
 *
 * 1. **分区粒度（coordinateShift）**：区块坐标右移 `coordinateShift` 位得到分区(section)键。
 *    `coordinateShift = 0` 时一个区块一个锁条目（最细粒度）；`coordinateShift = N` 时每 `(1<<N)×(1<<N)` 个区块
 *    共用一个 section 锁，减少哈希表条目数与 `lock`/`unlock` 的 section 操作数，但增大锁竞争粒度。
 *    调用方按场景选择：`ChunkTaskScheduler` 用 6（对齐 Moonrise `getChunkSystemLockShift()`），使
 *    `2*maxAccessRadius` 锁只触达 1~4 个 section 而非 2025 个。
 *
 * 2. **Node 所有权**：一次 `lock` 调用创建一个 `Node`，该 Node 代表调用线程对区域内所有
 *    section 的占有。内部哈希表为 `sectionKey -> shared_ptr<Node>`。同一线程对已占有 section 的重入
 *    加锁会命中自己的 Node（视为已占有，不阻塞）。
 *
 * 3. **不相交不变量**：不同线程不能持有相交区域。若 A 线程持有区域 R1，B 线程尝试锁
 *    与 R1 相交的 R2，B 会阻塞直到 A 释放。同一线程只能重入"完全被自己已持有区域覆盖"
 *    的子区域，不能重入"相交但不被覆盖"的区域（这会触发断言，对齐 Moonrise 的
 *    "Should never acquire intersecting areas"）。
 *
 * 4. **RAII**：`lock()` 返回 `LockHandle`（持有锁所有权），`LockHandle` 析构时
 *    `unlock` + 释放 `shared_ptr`。`tryLock()` 返回 `LockHandle`，失败返回空 `LockHandle`。
 *
 * ## 实现（对齐 Moonrise）
 *
 * - `m_nodes`：每桶 mutex 分段锁哈希表 `ConcurrentLong2ObjectHashTable<shared_ptr<Node>>`
 *   （putIfAbsent/remove(key,expected)/get），对齐 Moonrise 的 `ConcurrentChainedLong2ReferenceHashTable`
 *   （每桶 `synchronized(node)` 分段锁）。4096 桶，每桶独立锁，不同桶完全并行，同桶争用时阻塞睡眠。
 *   先前版本用 `std::atomic<shared_ptr>` 无锁方案，高争用下（onChunkGenComplete 持 2025-section 锁）
 *   逻辑删除节点堆积 + CAS 忙等导致 livelock；改回分段锁消除该问题。
 * - **等待队列**：每个 Node 自身继承 `MultiThreadedQueue<ThreadHandle*>`，作为该 Node 的等待线程队列。
 *   冲突线程把自己加入被冲突 Node 的等待队列（`park->add(currThread)`），然后 `LockSupport::park()` 阻塞。
 *   unlock 时 `node.pollOrBlockAdds()` 排空等待队列并 `LockSupport::unpark` 逐个唤醒。
 *   这套无锁等待协议替代了旧的 mutex+condition_variable，消除 `unlock:notify` 的 per-key notify_all 惊群。
 *
 * ## 在区块生成中的作用
 *
 * `ChunkTaskScheduler` 用它保护 `schedule`/`checkNeighbour`/`onChunkGenComplete` 的原子性：
 * 锁覆盖 `[x±accessRadius, z±accessRadius]`，保证检查邻居状态→建立依赖→创建任务→完成通知
 * 整个流程不被其他线程打断。`onChunkGenComplete` 持有 `2 * maxAccessRadius` 的锁以覆盖
 * 邻居的邻居。
 *
 * ## 坐标打包
 *
 * section 键用 `(sectionX, sectionZ)` 打包为 `u64`：`((u64)sectionZ << 32) | (u32)sectionX`，
 * 对齐 Moonrise `IntPairUtil.key`。
 */
class ReentrantAreaLock {
public:
    using Node = ReentrantAreaLockNode;

    /**
     * @brief 锁区域句柄（RAII）
     *
     * 持有 `shared_ptr<ReentrantAreaLockNode>`。析构时调用 `unlock`（移除 section + 排空队列 + unpark）
     * 再释放 `shared_ptr`（引用计数 -1，归零则析构 Node 的 `~MultiThreadedQueue` 链表）。
     * 不可拷贝，可移动。调用方通常以 `auto lock = areaLock.lock(x, z, r);` 持有，作用域结束自动释放。
     */
    class LockHandle {
    public:
        LockHandle() noexcept = default;
        LockHandle(std::nullptr_t) noexcept {}

        explicit LockHandle(std::shared_ptr<Node> node) noexcept
            : m_node(std::move(node))
        {}

        LockHandle(const LockHandle&) = delete;
        LockHandle& operator=(const LockHandle&) = delete;

        LockHandle(LockHandle&& other) noexcept
            : m_node(std::move(other.m_node))
        {}

        LockHandle& operator=(LockHandle&& other) noexcept
        {
            if (this != &other) {
                reset();
                m_node = std::move(other.m_node);
            }
            return *this;
        }

        ~LockHandle() { reset(); }

        [[nodiscard]] Node& operator*() const noexcept { return *m_node; }
        [[nodiscard]] Node* operator->() const noexcept { return m_node.get(); }
        [[nodiscard]] Node* get() const noexcept { return m_node.get(); }
        [[nodiscard]] explicit operator bool() const noexcept { return m_node != nullptr; }

        [[nodiscard]] friend bool operator==(const LockHandle& h, std::nullptr_t) noexcept
        {
            return h.m_node == nullptr;
        }
        [[nodiscard]] friend bool operator!=(const LockHandle& h, std::nullptr_t) noexcept
        {
            return h.m_node != nullptr;
        }
        [[nodiscard]] friend bool operator==(std::nullptr_t, const LockHandle& h) noexcept
        {
            return h.m_node == nullptr;
        }
        [[nodiscard]] friend bool operator!=(std::nullptr_t, const LockHandle& h) noexcept
        {
            return h.m_node != nullptr;
        }

        void reset() noexcept
        {
            if (m_node) {
                m_node->unlock();
                m_node.reset();
            }
        }

    private:
        friend class ReentrantAreaLock;
        std::shared_ptr<Node> m_node;
    };

    /**
     * @brief 构造区域锁
     *
     * @param coordinateShift 区块坐标右移位数，得到 section 键。0 表示一个区块一个锁条目（最细粒度）；
     *                        N>0 时每 (1<<N)×(1<<N) 个区块共用一个 section 锁。由调用方按场景选择
     *                        （ChunkTaskScheduler 用 6，对齐 Moonrise getChunkSystemLockShift()）。负值非法。
     */
    explicit ReentrantAreaLock(i32 coordinateShift)
        : m_coordinateShift(coordinateShift)
        , m_nodes()
    {
        MC_ASSERT_RELEASE(coordinateShift >= 0);
    }

    ReentrantAreaLock(const ReentrantAreaLock&) = delete;
    ReentrantAreaLock& operator=(const ReentrantAreaLock&) = delete;

    /**
     * @brief 获取覆盖单个区块 (x, z) 的锁，阻塞直到获得
     */
    [[nodiscard]] LockHandle lock(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 获取覆盖 `[centerX±radius, centerZ±radius]` 区域的锁，阻塞直到获得
     */
    [[nodiscard]] LockHandle lock(ChunkCoord centerX, ChunkCoord centerZ, i32 radius);

    /**
     * @brief 获取覆盖 `[fromX..toX, fromZ..toZ]` 区域的锁，阻塞直到获得
     */
    [[nodiscard]] LockHandle lock(ChunkCoord fromX, ChunkCoord fromZ, ChunkCoord toX, ChunkCoord toZ);

    /**
     * @brief 尝试获取单个区块 (x, z) 的锁，非阻塞
     *
     * @return 锁句柄；失败返回空 LockHandle（`explicit operator bool` 为 false）
     */
    [[nodiscard]] LockHandle tryLock(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 尝试获取 `[centerX±radius, centerZ±radius]` 区域的锁，非阻塞
     */
    [[nodiscard]] LockHandle tryLock(ChunkCoord centerX, ChunkCoord centerZ, i32 radius);

    /**
     * @brief 尝试获取 `[fromX..toX, fromZ..toZ]` 区域的锁，非阻塞
     */
    [[nodiscard]] LockHandle tryLock(ChunkCoord fromX, ChunkCoord fromZ, ChunkCoord toX, ChunkCoord toZ);

    /**
     * @brief 检查当前线程是否持有覆盖单个区块 (x, z) 的锁
     */
    [[nodiscard]] bool isHeldByCurrentThread(ChunkCoord x, ChunkCoord z) const;

    /**
     * @brief 检查当前线程是否持有覆盖 `[centerX±radius, centerZ±radius]` 的锁
     */
    [[nodiscard]] bool isHeldByCurrentThread(ChunkCoord centerX, ChunkCoord centerZ, i32 radius) const;

    /**
     * @brief 检查当前线程是否持有覆盖 `[fromX..toX, fromZ..toZ]` 的锁
     */
    [[nodiscard]] bool isHeldByCurrentThread(ChunkCoord fromX, ChunkCoord fromZ, ChunkCoord toX, ChunkCoord toZ) const;

    /**
     * @brief 释放 Node 占有的所有 section
     *
     * 由 LockHandle 析构调用。幂等：areaAffectedLen==0 时直接返回（纯重入场景）。
     */
    void unlock(ReentrantAreaLockNode& node);

    /**
     * @brief 获取分区位移位数
     */
    [[nodiscard]] i32 coordinateShift() const { return m_coordinateShift; }

private:
    /**
     * @brief section 键打包：(sectionX, sectionZ) -> u64
     *
     * 对齐 Moonrise `IntPairUtil.key`：高 32 位为 Z，低 32 位为 X。
     */
    [[nodiscard]] static u64 packKey(i32 sectionX, i32 sectionZ)
    {
        return (static_cast<u64>(static_cast<u32>(sectionZ)) << 32) | static_cast<u64>(static_cast<u32>(sectionX));
    }

    /**
     * @brief 计算区块坐标对应的 section 键
     *
     * 算术右移保证负坐标向负无穷方向取整（与 Java 的 >> 一致）。
     */
    [[nodiscard]] u64 sectionKey(ChunkCoord x, ChunkCoord z) const
    {
        const i32 sectionX = x >> m_coordinateShift;
        const i32 sectionZ = z >> m_coordinateShift;
        return packKey(sectionX, sectionZ);
    }

    /**
     * @brief 单区块快速路径（对齐 Moonrise lock(int,int)）
     *
     * 单 section，无 areaAffected 数组分配（用固定 1 元素）。
     */
    [[nodiscard]] LockHandle lockSingle(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 退避调度（对齐 Moonrise failures 分级退避）
     *
     * @param failures 失败计数（按引用传入，自旋分支翻倍，park 分支线性递增）
     *
     * - failures < 128：自旋 failures 次后 failures 翻倍（指数退避）
     * - failures < 1200：park 1µs，failures + 1
     * - failures >= 1200：yield + park(100µs × failures)，failures + 1
     */
    static void backoff(long& failures);

    i32 m_coordinateShift;

    /**
     * @brief section 键 -> Node 的每桶 mutex 分段锁哈希表
     *
     * 每个被占有的 section 都映射到持有它的 Node。多个 section 可映射到同一个 Node
     * （一次区域锁覆盖多个 section 时）。Node 由 shared_ptr 管理：哈希表 + 持有线程的 LockHandle +
     * 阻塞线程的 park 引用共同保活，对齐 Moonrise 的 GC 语义。unlock 时从哈希表移除。
     */
    ConcurrentLong2ObjectHashTable<std::shared_ptr<Node>> m_nodes;
};

} // namespace mc::util
