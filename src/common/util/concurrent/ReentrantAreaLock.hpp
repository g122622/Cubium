/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to the Software is
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

#pragma once

#include "common/core/Types.hpp"
#include "common/perfetto/TraceEvents.hpp"

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace mc::util {

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
 *    `coordinateShift = 0` 时一个区块一个锁条目（最细粒度，对齐 Moonrise 默认配置）。
 *    多个区块可以共用同一个 section 锁，减少内存占用但增大锁竞争粒度。
 *
 * 2. **Node 所有权**：一次 `lock` 调用创建一个 `Node`，该 Node 代表调用线程对区域内所有
 *    section 的占有。内部 map 为 `sectionKey -> Node*`。同一线程对已占有 section 的重入
 *    加锁会命中自己的 Node（视为已占有，不阻塞）。
 *
 * 3. **不相交不变量**：不同线程不能持有相交区域。若 A 线程持有区域 R1，B 线程尝试锁
 *    与 R1 相交的 R2，B 会阻塞直到 A 释放。同一线程只能重入"完全被自己已持有区域覆盖"
 *    的子区域，不能重入"相交但不被覆盖"的区域（这会抛出异常，对齐 Moonrise 的
 *    "Should never acquire intersecting areas"）。
 *
 * 4. **RAII**：`lock()` 返回 `Node`（持有锁所有权），Node 析构时自动 `unlock`。
 *    `tryLock()` 返回 `std::unique_ptr<Node>`，失败返回 nullptr。
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
    /**
     * @brief 锁句柄，RAII 释放
     *
     * Node 析构时会释放其占有的所有 section。Node 不可拷贝、不可移动，
     * 只能由 `ReentrantAreaLock::lock` / `tryLock` 创建。
     * 调用方通常以 `auto node = lock.lock(x, z, r);` 持有，作用域结束自动释放。
     */
    class Node {
    public:
        Node(const Node&) = delete;
        Node& operator=(const Node&) = delete;
        Node(Node&&) noexcept = delete;
        Node& operator=(Node&&) noexcept = delete;

        ~Node() { m_lock.unlock(*this); }

    private:
        friend class ReentrantAreaLock;

        Node(ReentrantAreaLock& lock, std::vector<u64> areaAffected, std::thread::id thread)
            : m_lock(lock)
            , m_areaAffected(std::move(areaAffected))
            , m_areaAffectedLen(0)
            , m_thread(thread)
        {}

        ReentrantAreaLock& m_lock;
        std::vector<u64> m_areaAffected; ///< 该 Node 实际占有写入 map 的 section 键
        u64 m_areaAffectedLen;           ///< m_areaAffected 中有效条目数
        std::thread::id m_thread;        ///< 持有线程
    };

    /**
     * @brief 构造区域锁
     *
     * @param coordinateShift 区块坐标右移位数，得到 section 键。0 表示一个区块一个锁条目。
     *                        Moonrise 使用 0（区块级粒度）。负值非法。
     */
    explicit ReentrantAreaLock(i32 coordinateShift);

    ReentrantAreaLock(const ReentrantAreaLock&) = delete;
    ReentrantAreaLock& operator=(const ReentrantAreaLock&) = delete;

    /**
     * @brief 获取覆盖单个区块 (x, z) 的锁，阻塞直到获得
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 锁句柄，作用域结束自动释放
     */
    [[nodiscard]] std::unique_ptr<Node> lock(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 获取覆盖 `[centerX±radius, centerZ±radius]` 区域的锁，阻塞直到获得
     *
     * @param centerX 中心区块 X 坐标
     * @param centerZ 中心区块 Z 坐标
     * @param radius 半径（区块数），覆盖边长 2*radius+1
     * @return 锁句柄，作用域结束自动释放
     */
    [[nodiscard]] std::unique_ptr<Node> lock(ChunkCoord centerX, ChunkCoord centerZ, i32 radius);

    /**
     * @brief 获取覆盖 `[fromX..toX, fromZ..toZ]` 区域的锁，阻塞直到获得
     *
     * @param fromX 区域最小 X（含）
     * @param fromZ 区域最小 Z（含）
     * @param toX 区域最大 X（含）
     * @param toZ 区域最大 Z（含）
     * @return 锁句柄，作用域结束自动释放
     */
    [[nodiscard]] std::unique_ptr<Node> lock(ChunkCoord fromX, ChunkCoord fromZ, ChunkCoord toX, ChunkCoord toZ);

    /**
     * @brief 尝试获取单个区块 (x, z) 的锁，非阻塞
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     * @return 锁句柄；失败返回 nullptr
     */
    [[nodiscard]] std::unique_ptr<Node> tryLock(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 尝试获取 `[centerX±radius, centerZ±radius]` 区域的锁，非阻塞
     *
     * @return 锁句柄；失败返回 nullptr
     */
    [[nodiscard]] std::unique_ptr<Node> tryLock(ChunkCoord centerX, ChunkCoord centerZ, i32 radius);

    /**
     * @brief 尝试获取 `[fromX..toX, fromZ..toZ]` 区域的锁，非阻塞
     *
     * @return 锁句柄；失败返回 nullptr
     */
    [[nodiscard]] std::unique_ptr<Node> tryLock(ChunkCoord fromX, ChunkCoord fromZ, ChunkCoord toX, ChunkCoord toZ);

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
     * 通常由 Node 析构自动调用，也可手动调用后由析构再次调用（幂等：areaAffectedLen==0 时直接返回）。
     */
    void unlock(Node& node);

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
    [[nodiscard]] static u64 _packKey(i32 sectionX, i32 sectionZ);

    /**
     * @brief 计算区块坐标对应的 section 键
     */
    [[nodiscard]] u64 _sectionKey(ChunkCoord x, ChunkCoord z) const;

    i32 m_coordinateShift;

    /**
     * @brief section 键 -> Node 的并发哈希表
     *
     * 每个被占有的 section 都映射到持有它的 Node。多个 section 可映射到同一个 Node
     * （一次区域锁覆盖多个 section 时）。map 中存储的是裸指针，Node 持有自身所有权，
     * unlock 时从 map 移除并最终由 unique_ptr 析构释放 Node。
     *
     * 用 mutable 是因为 isHeldByCurrentThread 是逻辑 const 但需要读锁。
     */
    std::unordered_map<u64, Node*> m_nodes;
    mutable std::mutex m_mutex;

    /**
     * @brief 阻塞等待队列：每个 section 键对应一组等待线程
     *
     * 当某 section 被其他线程占有时，等待者通过该 section 的条件变量阻塞。
     * unlock 唤醒对应 section 的等待者。
     *
     * 用 shared_ptr 持有，因为等待线程会拷贝一份 shared_ptr 副本以在 cv.wait 期间
     * 保证 SectionWaiters 生命周期（其他等待者被唤醒后可能擦除 map 条目）。
     */
    struct SectionWaiters {
        std::condition_variable cv;
        u64 waiterCount = 0; ///< 等待该 section 的线程数
    };
    std::unordered_map<u64, std::shared_ptr<SectionWaiters>> m_waiters;
};

} // namespace mc::util
