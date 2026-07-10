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

#include "common/util/concurrent/ReentrantAreaLock.hpp"

#include "common/util/assert/AssertAll.hpp"

#include <thread>

using namespace mc::trace;

namespace mc::util {

// ============================================================================
// Node 构造由 lock/tryLock 内部完成（构造函数私有，ReentrantAreaLock 为友元）
// unlock 定义在此处：头文件中 ReentrantAreaLock 仍是不完整类型，无法内联调用 m_lock.unlock
// ============================================================================

void ReentrantAreaLockNode::unlock()
{
    m_lock.unlock(*this);
}

ReentrantAreaLock::LockHandle ReentrantAreaLock::lock(ChunkCoord x, ChunkCoord z)
{
    return lock(x, z, x, z);
}

ReentrantAreaLock::LockHandle ReentrantAreaLock::lock(ChunkCoord centerX, ChunkCoord centerZ, i32 radius)
{
    return lock(centerX - radius, centerZ - radius, centerX + radius, centerZ + radius);
}

ReentrantAreaLock::LockHandle ReentrantAreaLock::lock(
    ChunkCoord fromX, ChunkCoord fromZ, ChunkCoord toX, ChunkCoord toZ)
{
    MC_ASSERT_RELEASE_MSG(fromX <= toX, "ReentrantAreaLock::lock: fromX > toX");
    MC_ASSERT_RELEASE_MSG(fromZ <= toZ, "ReentrantAreaLock::lock: fromZ > toZ");
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Server.Chunk, "ReentrantAreaLock::lock", "fromX", fromX, "fromZ", fromZ, "toX", toX, "toZ", toZ);

    const std::thread::id currThread = std::this_thread::get_id();
    const i32 shift = m_coordinateShift;
    const i32 fromSectionX = fromX >> shift;
    const i32 fromSectionZ = fromZ >> shift;
    const i32 toSectionX = toX >> shift;
    const i32 toSectionZ = toZ >> shift;

    // 单 section 快速路径：区域经 section 压缩后只有一个 section（对齐 Moonrise）
    if (((fromSectionX ^ toSectionX) | (fromSectionZ ^ toSectionZ)) == 0) {
        return lockSingle(fromX, fromZ);
    }

    const i32 areaW = toSectionX - fromSectionX + 1;
    const i32 areaH = toSectionZ - fromSectionZ + 1;
    std::vector<u64> areaAffected(static_cast<std::size_t>(areaW) * static_cast<std::size_t>(areaH));

    // Node 由 shared_ptr 管理：哈希表 + LockHandle + 阻塞线程的 park 引用共同保活
    std::shared_ptr<Node> node(new Node(*this, std::move(areaAffected), currThread));
    LockHandle handle(node);

    // 当前线程的 ThreadHandle（延迟获取：仅在第一次需要阻塞时取，避免无冲突路径的开销）
    std::shared_ptr<LockSupport::ThreadHandle> currThreadHandle;

    for (long failures = 0;;) {
        std::shared_ptr<Node> park; // 冲突的占有者 Node（阻塞在其等待队列上）
        bool addedToArea = false;   // 本次尝试是否成功 putIfAbsent 至少一个 section
        bool alreadyOwned = false;  // 是否遇到其他线程占有的 section
        bool allOwned = true;       // 所有 section 是否都被本线程重入占有
        u64 areaAffectedLen = 0;    // 本次尝试写入 m_areaAffected 的条目数

        // 尝试原子占有区域内所有 section
        for (i32 sz = fromSectionZ; sz <= toSectionZ; ++sz) {
            for (i32 sx = fromSectionX; sx <= toSectionX; ++sx) {
                const u64 coordinate = packKey(sx, sz);

                std::shared_ptr<Node> prev = m_nodes.putIfAbsent(coordinate, node);

                if (prev == nullptr) {
                    // section 空闲，占有之
                    addedToArea = true;
                    allOwned = false;
                    node->m_areaAffected[areaAffectedLen] = coordinate;
                    ++areaAffectedLen;
                    continue;
                }

                if (prev->m_thread != currThread) {
                    // 被其他线程占有，冲突
                    park = prev;
                    alreadyOwned = true;
                    break;
                }
                // prev->m_thread == currThread：同线程重入，跳过（不记入 areaAffected）
            }
        }

        // 失败判定（对齐 Moonrise）：
        //   (park != null && addedToArea) —— 已插入部分 section 但遇到冲突，需回滚 + 排空等待者
        //   (park == null && alreadyOwned && !allOwned) —— 同线程重入但区域相交不被完全覆盖，非法锁使用
        if ((park != nullptr && addedToArea) || (park == nullptr && alreadyOwned && !allOwned)) {
            // 回滚本次写入的 section（值校验 remove，防误删）
            for (u64 i = 0; i < areaAffectedLen; ++i) {
                const u64 key = node->m_areaAffected[i];
                const bool removed = m_nodes.remove(key, node);
                MC_ASSERT_RELEASE_MSG(
                    removed, "ReentrantAreaLock::lock: section not owned by this node during rollback");
            }

            // 因为本线程可能已插入 node（addedToArea），其他线程可能在本 node 的等待队列上排队，
            // 排空等待队列并唤醒它们（对齐 Moonrise: 无条件 pollOrBlockAdds 循环 unpark）。
            // pollOrBlockAdds 在队列空时阻塞入队（preventAdds），与并发 lock 的 add 竞争保证不丢唤醒；
            // 阻塞状态由下方 if (addedToArea) allowAdds() 恢复。
            LockSupport::ThreadHandle* unpark;
            while ((unpark = node->pollOrBlockAdds()) != nullptr) {
                LockSupport::unpark(unpark);
            }
        }

        if (park == nullptr) {
            // 成功占有所有 section（或纯重入）
            if (alreadyOwned && !allOwned) {
                // 同线程重入相交但不被覆盖的区域：非法锁使用（对齐 Moonrise "Should never acquire intersecting areas"）
                MC_ASSERT_RELEASE_MSG(
                    false, "ReentrantAreaLock::lock: Improper lock usage - intersecting areas not fully covered");
            }
            node->m_areaAffectedLen = areaAffectedLen;
            return handle;
        }

        // 失败，阻塞后重试
        ++failures;

        if (failures > 128L) {
            // 高争用：尝试把自己加入冲突 Node 的等待队列，成功则 park
            if (!currThreadHandle) {
                currThreadHandle = LockSupport::currentThread();
            }
            // park 是 shared_ptr（putIfAbsent 返回的拷贝），保活冲突 Node：
            // 阻塞线程遍历 park 的等待队列链表（add 的 CAS）期间，持有线程可能 unlock + 释放 LockHandle。
            // shared_ptr 保证 park 在 add + park 期间存活，防止 ~MultiThreadedQueue 析构导致 use-after-free。
            if (park->add(currThreadHandle.get())) {
                // add 成功：把自己入队到 park（冲突 Node），等待 unlock 唤醒
                LockSupport::park();
            } else {
                // add 失败：park 的队列被 preventAdds（unlock 正在排空），落入退避分支不阻塞
                backoff(failures);
            }
        } else {
            // 低争用：自旋 / 短 park 退避（对齐 Moonrise failures < 128 分支）
            backoff(failures);
        }

        if (addedToArea) {
            // 重试前恢复 node 的入队能力：排空时 pollOrBlockAdds 可能已阻止入队，
            // allowAdds 解除阻止，使后续重试中其他线程可在本 node 上排队等待（对齐 Moonrise ret.allowAdds()）
            node->allowAdds();
        }
    }
}

ReentrantAreaLock::LockHandle ReentrantAreaLock::tryLock(ChunkCoord x, ChunkCoord z)
{
    return tryLock(x, z, x, z);
}

ReentrantAreaLock::LockHandle ReentrantAreaLock::tryLock(ChunkCoord centerX, ChunkCoord centerZ, i32 radius)
{
    return tryLock(centerX - radius, centerZ - radius, centerX + radius, centerZ + radius);
}

ReentrantAreaLock::LockHandle ReentrantAreaLock::tryLock(
    ChunkCoord fromX, ChunkCoord fromZ, ChunkCoord toX, ChunkCoord toZ)
{
    MC_ASSERT_RELEASE_MSG(fromX <= toX, "ReentrantAreaLock::tryLock: fromX > toX");
    MC_ASSERT_RELEASE_MSG(fromZ <= toZ, "ReentrantAreaLock::tryLock: fromZ > toZ");

    const std::thread::id currThread = std::this_thread::get_id();
    const i32 shift = m_coordinateShift;
    const i32 fromSectionX = fromX >> shift;
    const i32 fromSectionZ = fromZ >> shift;
    const i32 toSectionX = toX >> shift;
    const i32 toSectionZ = toZ >> shift;

    const i32 areaW = toSectionX - fromSectionX + 1;
    const i32 areaH = toSectionZ - fromSectionZ + 1;
    std::vector<u64> areaAffected(static_cast<std::size_t>(areaW) * static_cast<std::size_t>(areaH));

    std::shared_ptr<Node> node(new Node(*this, std::move(areaAffected), currThread));
    LockHandle handle(node);

    bool failed = false;
    u64 areaAffectedLen = 0;

    // 尝试一次性占有所有 section（无重试、不阻塞）
    for (i32 sz = fromSectionZ; sz <= toSectionZ; ++sz) {
        for (i32 sx = fromSectionX; sx <= toSectionX; ++sx) {
            const u64 coordinate = packKey(sx, sz);

            std::shared_ptr<Node> prev = m_nodes.putIfAbsent(coordinate, node);

            if (prev == nullptr) {
                node->m_areaAffected[areaAffectedLen] = coordinate;
                ++areaAffectedLen;
                continue;
            }

            if (prev->m_thread != currThread) {
                failed = true;
                break;
            }
            // 同线程重入，跳过
        }
    }

    if (!failed) {
        node->m_areaAffectedLen = areaAffectedLen;
        return handle;
    }

    // 失败：回滚已占有的 section
    if (areaAffectedLen != 0) {
        for (u64 i = 0; i < areaAffectedLen; ++i) {
            const u64 key = node->m_areaAffected[i];
            const bool removed = m_nodes.remove(key, node);
            MC_ASSERT_RELEASE_MSG(
                removed, "ReentrantAreaLock::tryLock: section not owned by this node during rollback");
        }

        // 排空等待队列（与 lock 失败路径一致：本线程插入了 node，可能有等待者在排队）
        LockSupport::ThreadHandle* unpark;
        while ((unpark = node->pollOrBlockAdds()) != nullptr) {
            LockSupport::unpark(unpark);
        }
    }

    // 失败：handle.reset() 调用 unlock（areaAffectedLen==0 幂等）+ 释放 shared_ptr
    handle.reset();
    return LockHandle();
}

bool ReentrantAreaLock::isHeldByCurrentThread(ChunkCoord x, ChunkCoord z) const
{
    const std::thread::id currThread = std::this_thread::get_id();
    const u64 key = sectionKey(x, z);
    const std::shared_ptr<Node> node = m_nodes.get(key);
    return node != nullptr && node->m_thread == currThread;
}

bool ReentrantAreaLock::isHeldByCurrentThread(ChunkCoord centerX, ChunkCoord centerZ, i32 radius) const
{
    return isHeldByCurrentThread(centerX - radius, centerZ - radius, centerX + radius, centerZ + radius);
}

bool ReentrantAreaLock::isHeldByCurrentThread(ChunkCoord fromX, ChunkCoord fromZ, ChunkCoord toX, ChunkCoord toZ) const
{
    MC_ASSERT_RELEASE_MSG(fromX <= toX, "ReentrantAreaLock::isHeldByCurrentThread: fromX > toX");
    MC_ASSERT_RELEASE_MSG(fromZ <= toZ, "ReentrantAreaLock::isHeldByCurrentThread: fromZ > toZ");

    const std::thread::id currThread = std::this_thread::get_id();
    const i32 shift = m_coordinateShift;
    const i32 fromSectionX = fromX >> shift;
    const i32 fromSectionZ = fromZ >> shift;
    const i32 toSectionX = toX >> shift;
    const i32 toSectionZ = toZ >> shift;

    for (i32 sz = fromSectionZ; sz <= toSectionZ; ++sz) {
        for (i32 sx = fromSectionX; sx <= toSectionX; ++sx) {
            const u64 key = packKey(sx, sz);
            const std::shared_ptr<Node> node = m_nodes.get(key);
            if (node == nullptr || node->m_thread != currThread) {
                return false;
            }
        }
    }
    return true;
}

void ReentrantAreaLock::unlock(ReentrantAreaLockNode& node)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk, "ReentrantAreaLock::unlock");

    const u64 areaAffectedLen = node.m_areaAffectedLen;
    if (areaAffectedLen == 0) {
        // 纯重入场景：未占有任何 section，无需移除或唤醒
        return;
    }

    MC_ASSERT_RELEASE_MSG(
        areaAffectedLen <= node.m_areaAffected.size(), "ReentrantAreaLock::unlock: areaAffectedLen out of range");

    // shared_from_this：用于哈希表的值校验 remove（shared_ptr 指针相等比较）
    const std::shared_ptr<ReentrantAreaLockNode> self = node.shared_from_this();

    // 从哈希表移除本 node 占有的所有 section（值校验，防误删）
    for (u64 i = 0; i < areaAffectedLen; ++i) {
        const u64 coordinate = node.m_areaAffected[i];
        const bool removed = m_nodes.remove(coordinate, self);
        MC_ASSERT_RELEASE_MSG(removed, "ReentrantAreaLock::unlock: section not owned by this node");
    }

    // 排空本 node 的等待队列并唤醒所有等待线程（对齐 Moonrise: pollOrBlockAdds 循环 unpark）
    // pollOrBlockAdds 在队列空时阻塞入队（preventAdds），与并发 lock 的 add 竞争保证不丢唤醒
    LockSupport::ThreadHandle* unpark;
    while ((unpark = node.pollOrBlockAdds()) != nullptr) {
        LockSupport::unpark(unpark);
    }
}

// ============================================================================
// 单 section 快速路径（对齐 Moonrise lock(int,int)）
// ============================================================================

ReentrantAreaLock::LockHandle ReentrantAreaLock::lockSingle(ChunkCoord x, ChunkCoord z)
{
    const std::thread::id currThread = std::this_thread::get_id();
    const u64 coordinate = sectionKey(x, z);

    std::vector<u64> areaAffected(1);
    areaAffected[0] = coordinate;

    std::shared_ptr<Node> node(new Node(*this, std::move(areaAffected), currThread));
    LockHandle handle(node);

    std::shared_ptr<LockSupport::ThreadHandle> currThreadHandle; // 延迟获取

    for (long failures = 0;;) {
        std::shared_ptr<Node> prev = m_nodes.putIfAbsent(coordinate, node);

        if (prev == nullptr) {
            // 占有成功
            node->m_areaAffectedLen = 1;
            return handle;
        }
        if (prev->m_thread == currThread) {
            // 同线程重入：单 section 已被自己占有，areaAffectedLen=0（纯重入）
            return handle;
        }

        // 被其他线程占有，冲突
        std::shared_ptr<Node> park = prev;
        ++failures;

        if (failures > 128L) {
            if (!currThreadHandle) {
                currThreadHandle = LockSupport::currentThread();
            }
            // park 是 shared_ptr，保活冲突 Node（持有线程 unlock+release 期间 add 安全）
            if (park->add(currThreadHandle.get())) {
                LockSupport::park();
            } else {
                backoff(failures);
            }
        } else {
            backoff(failures);
        }

        // 单 section 路径无 addedToArea（要么占有成功返回，要么冲突未插入），无需 allowAdds
    }
}

// ============================================================================
// 退避调度（对齐 Moonrise 的 failures 分级退避）
//
// failures 按引用传入：自旋分支将 failures 翻倍（指数退避），park 分支线性递增。
// 对齐 Moonrise lock(int,int) / lock(int,int,int,int) 的 failures 更新逻辑。
// ============================================================================

void ReentrantAreaLock::backoff(long& failures)
{
    if (failures < 128L) {
        // 自旋（对齐 Moonrise Thread.onSpinWait × failures，并 failures 翻倍）
        for (long i = 0; i < failures; ++i) {
            cpuRelax();
        }
        failures = failures << 1;
    } else if (failures < 1200L) {
        // 中等争用：park 1µs（对齐 Moonrise parkNanos(1_000L)）
        LockSupport::parkNanos(1000L);
        failures = failures + 1L;
    } else {
        // 高争用：yield + park 100µs × failures（对齐 Moonrise 100_000L * failures）
        std::this_thread::yield();
        LockSupport::parkNanos(100000L * static_cast<i64>(failures));
        failures = failures + 1L;
    }
}

} // namespace mc::util
