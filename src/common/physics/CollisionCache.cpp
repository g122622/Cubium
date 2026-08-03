/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "CollisionCache.hpp"
#include "common/core/Types.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include <atomic>
#include <cstddef>
#include <mutex>
#include <shared_mutex>
#include <utility>
#include <vector>

namespace mc::physics {

// ========== 缓存操作实现 ==========

const std::vector<AxisAlignedBB>* CollisionCache::getChunkCollisionBoxes(ChunkCoord chunkX, ChunkCoord chunkZ) const
{
    u64 key = _makeKey(chunkX, chunkZ);

    // 读锁
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_hitCount.fetch_add(1, std::memory_order::relaxed);
        return &it->second.boxes;
    }

    m_missCount.fetch_add(1, std::memory_order::relaxed);
    return nullptr;
}

const CollisionCache::ChunkCache* CollisionCache::getChunkCache(ChunkCoord chunkX, ChunkCoord chunkZ) const
{
    u64 key = _makeKey(chunkX, chunkZ);

    // 读锁
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_hitCount.fetch_add(1, std::memory_order::relaxed);
        return &it->second;
    }

    m_missCount.fetch_add(1, std::memory_order::relaxed);
    return nullptr;
}

void CollisionCache::cacheChunkCollisionBoxes(
    ChunkCoord chunkX, ChunkCoord chunkZ, std::vector<AxisAlignedBB>&& boxes, u64 version)
{
    u64 key = _makeKey(chunkX, chunkZ);

    // 写锁
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    ChunkCache& cache = m_cache[key];
    cache.boxes = std::move(boxes);
    cache.version = version;
}

void CollisionCache::cacheChunkCollisionBoxes(
    ChunkCoord chunkX, ChunkCoord chunkZ, const std::vector<AxisAlignedBB>& boxes, u64 version)
{
    u64 key = _makeKey(chunkX, chunkZ);

    // 写锁
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    ChunkCache& cache = m_cache[key];
    cache.boxes = boxes;
    cache.version = version;
}

bool CollisionCache::invalidateChunk(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    u64 key = _makeKey(chunkX, chunkZ);

    // 写锁
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_cache.erase(it);
        return true;
    }

    return false;
}

void CollisionCache::invalidateChunkAndNeighbors(ChunkCoord chunkX, ChunkCoord chunkZ, i32 radius)
{
    // 写锁
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    for (i32 dx = -radius; dx <= radius; ++dx) {
        for (i32 dz = -radius; dz <= radius; ++dz) {
            u64 key = _makeKey(chunkX + dx, chunkZ + dz);
            auto it = m_cache.find(key);
            if (it != m_cache.end()) {
                m_cache.erase(it);
            }
        }
    }
}

void CollisionCache::clear()
{
    // 写锁
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_cache.clear();
}

// ========== 统计信息实现 ==========

size_t CollisionCache::size() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_cache.size();
}

bool CollisionCache::empty() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_cache.empty();
}

void CollisionCache::resetStats()
{
    // 原子操作，不需要锁
    m_hitCount.store(0, std::memory_order::relaxed);
    m_missCount.store(0, std::memory_order::relaxed);
}

// ========== 私有方法实现 ==========

u64 CollisionCache::_makeKey(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    // 使用类似于 ChunkPos 的哈希方式
    // 将两个 32 位整数组合成一个 64 位整数
    u64 ux = static_cast<u64>(static_cast<u32>(chunkX));
    u64 uz = static_cast<u64>(static_cast<u32>(chunkZ));
    return (ux << 32) | uz;
}

} // namespace mc::physics
