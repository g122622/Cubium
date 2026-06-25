/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, restriction to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell
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

#include "common/util/assert/AssertAll.hpp"

namespace mc::util {

ReentrantAreaLock::ReentrantAreaLock(i32 coordinateShift)
    : m_coordinateShift(coordinateShift)
{
    MC_ASSERT_RELEASE(coordinateShift >= 0);
}

u64 ReentrantAreaLock::_packKey(i32 sectionX, i32 sectionZ)
{
    // 高 32 位为 Z，低 32 位为 X，对齐 Moonrise IntPairUtil.key
    return (static_cast<u64>(static_cast<u32>(sectionZ)) << 32) | static_cast<u64>(static_cast<u32>(sectionX));
}

u64 ReentrantAreaLock::_sectionKey(ChunkCoord x, ChunkCoord z) const
{
    // 算术右移保证负坐标向负无穷方向取整（与 Java 的 >> 一致）
    const i32 sectionX = x >> m_coordinateShift;
    const i32 sectionZ = z >> m_coordinateShift;
    return _packKey(sectionX, sectionZ);
}

// ============================================================================
// Node 构造由 lock/tryLock 内部完成（构造函数私有，ReentrantAreaLock 为友元）
// ============================================================================

std::unique_ptr<ReentrantAreaLock::Node> ReentrantAreaLock::lock(ChunkCoord x, ChunkCoord z)
{
    return lock(x, z, x, z);
}

std::unique_ptr<ReentrantAreaLock::Node> ReentrantAreaLock::lock(ChunkCoord centerX, ChunkCoord centerZ, i32 radius)
{
    return lock(centerX - radius, centerZ - radius, centerX + radius, centerZ + radius);
}

std::unique_ptr<ReentrantAreaLock::Node> ReentrantAreaLock::lock(
    ChunkCoord fromX, ChunkCoord fromZ, ChunkCoord toX, ChunkCoord toZ)
{
    MC_ASSERT_RELEASE_MSG(fromX <= toX, "ReentrantAreaLock::lock: fromX > toX");
    MC_ASSERT_RELEASE_MSG(fromZ <= toZ, "ReentrantAreaLock::lock: fromZ > toZ");

    const std::thread::id currThread = std::this_thread::get_id();
    const i32 shift = m_coordinateShift;
    const i32 fromSectionX = fromX >> shift;
    const i32 fromSectionZ = fromZ >> shift;
    const i32 toSectionX = toX >> shift;
    const i32 toSectionZ = toZ >> shift;

    // 预计算区域内所有 section 键，按固定顺序（行优先）以避免死锁
    std::vector<u64> sectionKeys;
    sectionKeys.reserve(static_cast<size_t>(toSectionX - fromSectionX + 1) * (toSectionZ - fromSectionZ + 1));
    for (i32 sz = fromSectionZ; sz <= toSectionZ; ++sz) {
        for (i32 sx = fromSectionX; sx <= toSectionX; ++sx) {
            sectionKeys.push_back(_packKey(sx, sz));
        }
    }

    auto node = std::unique_ptr<Node>(new Node(*this, sectionKeys, currThread));

    // 重试循环：尝试原子地占有区域内所有 section。若被其他线程占有则等待后重试。
    std::unique_lock<std::mutex> guard(m_mutex);

    for (;;) {
        u64 waitKey = 0; // 阻塞在哪个 section（0 表示无阻塞）

        u64 areaAffectedLen = 0;
        bool conflict = false;

        for (u64 key : sectionKeys) {
            auto it = m_nodes.find(key);
            if (it == m_nodes.end()) {
                // section 空闲，占有之
                m_nodes[key] = node.get();
                node->m_areaAffected[areaAffectedLen] = key;
                ++areaAffectedLen;
                continue;
            }

            Node* prev = it->second;
            if (prev->m_thread == currThread) {
                // 同一线程已占有该 section，重入，不增加 areaAffected
                continue;
            }

            // 被其他线程占有，冲突
            conflict = true;
            waitKey = key;
            break;
        }

        if (!conflict) {
            // 成功占有所有 section
            node->m_areaAffectedLen = areaAffectedLen;
            return node;
        }

        // 冲突：回滚本次写入的 section
        for (u64 i = 0; i < areaAffectedLen; ++i) {
            const u64 key = node->m_areaAffected[i];
            [[maybe_unused]] const auto erased = m_nodes.erase(key);
            MC_ASSERT_RELEASE(erased == 1);
        }
        node->m_areaAffectedLen = 0;

        // 注册等待者并阻塞在 waitKey 的 condition variable 上（持锁等待，避免丢失唤醒）
        if (waitKey != 0) {
            // 取 shared_ptr 副本，保证 cv 生命周期超出 map 擦除：
            // 其他等待者被 notify_all 唤醒后可能先于本线程擦除 m_waiters[waitKey]，
            // 若直接持有引用会导致 use-after-free。
            std::shared_ptr<SectionWaiters> waiters;
            auto it = m_waiters.find(waitKey);
            if (it == m_waiters.end()) {
                waiters = std::make_shared<SectionWaiters>();
                m_waiters[waitKey] = waiters;
            } else {
                waiters = it->second;
            }
            ++waiters->waiterCount;

            // cv.wait 释放 guard，阻塞直到 unlock 唤醒；唤醒后重新持有 guard
            waiters->cv.wait(guard);

            // 重新查找条目（可能已被其他等待者擦除），安全递减计数
            auto it2 = m_waiters.find(waitKey);
            if (it2 != m_waiters.end()) {
                --it2->second->waiterCount;
                if (it2->second->waiterCount == 0) {
                    m_waiters.erase(it2);
                }
            }
            // 循环回到顶部，重新尝试获取
        } else {
            // 不应发生：conflict 为 true 时 waitKey 必非 0，但保守处理
            guard.unlock();
            std::this_thread::yield();
            guard.lock();
        }
    }
}

std::unique_ptr<ReentrantAreaLock::Node> ReentrantAreaLock::tryLock(ChunkCoord x, ChunkCoord z)
{
    return tryLock(x, z, x, z);
}

std::unique_ptr<ReentrantAreaLock::Node> ReentrantAreaLock::tryLock(ChunkCoord centerX, ChunkCoord centerZ, i32 radius)
{
    return tryLock(centerX - radius, centerZ - radius, centerX + radius, centerZ + radius);
}

std::unique_ptr<ReentrantAreaLock::Node> ReentrantAreaLock::tryLock(
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

    std::vector<u64> sectionKeys;
    sectionKeys.reserve(static_cast<size_t>(toSectionX - fromSectionX + 1) * (toSectionZ - fromSectionZ + 1));
    for (i32 sz = fromSectionZ; sz <= toSectionZ; ++sz) {
        for (i32 sx = fromSectionX; sx <= toSectionX; ++sx) {
            sectionKeys.push_back(_packKey(sx, sz));
        }
    }

    auto node = std::unique_ptr<Node>(new Node(*this, sectionKeys, currThread));

    std::lock_guard<std::mutex> guard(m_mutex);

    u64 areaAffectedLen = 0;
    for (u64 key : sectionKeys) {
        auto it = m_nodes.find(key);
        if (it == m_nodes.end()) {
            m_nodes[key] = node.get();
            node->m_areaAffected[areaAffectedLen] = key;
            ++areaAffectedLen;
            continue;
        }

        Node* prev = it->second;
        if (prev->m_thread == currThread) {
            // 同线程重入
            continue;
        }

        // 被其他线程占有，回滚并失败
        for (u64 i = 0; i < areaAffectedLen; ++i) {
            const u64 key2 = node->m_areaAffected[i];
            [[maybe_unused]] const auto erased = m_nodes.erase(key2);
            MC_ASSERT_RELEASE(erased == 1);
        }
        return nullptr;
    }

    node->m_areaAffectedLen = areaAffectedLen;
    return node;
}

bool ReentrantAreaLock::isHeldByCurrentThread(ChunkCoord x, ChunkCoord z) const
{
    const std::thread::id currThread = std::this_thread::get_id();
    const u64 key = _sectionKey(x, z);

    std::lock_guard<std::mutex> guard(m_mutex);
    auto it = m_nodes.find(key);
    return it != m_nodes.end() && it->second->m_thread == currThread;
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

    std::lock_guard<std::mutex> guard(m_mutex);
    for (i32 sz = fromSectionZ; sz <= toSectionZ; ++sz) {
        for (i32 sx = fromSectionX; sx <= toSectionX; ++sx) {
            const u64 key = _packKey(sx, sz);
            auto it = m_nodes.find(key);
            if (it == m_nodes.end() || it->second->m_thread != currThread) {
                return false;
            }
        }
    }
    return true;
}

void ReentrantAreaLock::unlock(Node& node)
{
    MC_TRACE_EVENT("server.chunk", "ReentrantAreaLock::unlock");

    const u64 areaAffectedLen = node.m_areaAffectedLen;
    if (areaAffectedLen == 0) {
        // 未占有任何 section（纯重入场景），无需清理
        return;
    }

    std::vector<u64> keysToNotify;
    {
        MC_TRACE_EVENT("server.chunk", "ReentrantAreaLock::unlock:erase");
        std::lock_guard<std::mutex> guard(m_mutex);
        for (u64 i = 0; i < areaAffectedLen; ++i) {
            const u64 key = node.m_areaAffected[i];
            [[maybe_unused]] const auto erased = m_nodes.erase(key);
            MC_ASSERT_RELEASE_MSG(erased == 1, "ReentrantAreaLock::unlock: section not owned by this node");
            keysToNotify.push_back(key);
        }
        node.m_areaAffectedLen = 0;
    }

    // 唤醒所有等待这些 section 的线程（锁外通知，减少临界区）
    {
        MC_TRACE_EVENT("server.chunk", "ReentrantAreaLock::unlock:notify");
        for (u64 key : keysToNotify) {
            std::lock_guard<std::mutex> guard(m_mutex);
            auto it = m_waiters.find(key);
            if (it != m_waiters.end() && it->second) {
                it->second->cv.notify_all();
            }
        }
    }
}

} // namespace mc::util
