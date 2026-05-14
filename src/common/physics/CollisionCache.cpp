#include "CollisionCache.hpp"
#include <spdlog/spdlog.h>

namespace mc::physics {

// ========== 缓存操作实现 ==========

const std::vector<AxisAlignedBB>* CollisionCache::getChunkCollisionBoxes(ChunkCoord chunkX, ChunkCoord chunkZ) const
{

    u64 key = makeKey(chunkX, chunkZ);

    // 读锁
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_hitCount.fetch_add(1, std::memory_order_relaxed);
        return &it->second.boxes;
    }

    m_missCount.fetch_add(1, std::memory_order_relaxed);
    return nullptr;
}

const CollisionCache::ChunkCache* CollisionCache::getChunkCache(ChunkCoord chunkX, ChunkCoord chunkZ) const
{

    u64 key = makeKey(chunkX, chunkZ);

    // 读锁
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_hitCount.fetch_add(1, std::memory_order_relaxed);
        return &it->second;
    }

    m_missCount.fetch_add(1, std::memory_order_relaxed);
    return nullptr;
}

void CollisionCache::cacheChunkCollisionBoxes(
    ChunkCoord chunkX, ChunkCoord chunkZ, std::vector<AxisAlignedBB>&& boxes, u64 version)
{

    u64 key = makeKey(chunkX, chunkZ);

    // 写锁
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    ChunkCache& cache = m_cache[key];
    cache.boxes = std::move(boxes);
    cache.version = version;

    SPDLOG_TRACE("CollisionCache: Cached {} boxes for chunk ({}, {})", cache.boxes.size(), chunkX, chunkZ);
}

void CollisionCache::cacheChunkCollisionBoxes(
    ChunkCoord chunkX, ChunkCoord chunkZ, const std::vector<AxisAlignedBB>& boxes, u64 version)
{

    u64 key = makeKey(chunkX, chunkZ);

    // 写锁
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    ChunkCache& cache = m_cache[key];
    cache.boxes = boxes;
    cache.version = version;

    SPDLOG_TRACE("CollisionCache: Cached {} boxes for chunk ({}, {})", cache.boxes.size(), chunkX, chunkZ);
}

bool CollisionCache::invalidateChunk(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    u64 key = makeKey(chunkX, chunkZ);

    // 写锁
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        m_cache.erase(it);
        SPDLOG_TRACE("CollisionCache: Invalidated cache for chunk ({}, {})", chunkX, chunkZ);
        return true;
    }

    return false;
}

void CollisionCache::invalidateChunkAndNeighbors(ChunkCoord chunkX, ChunkCoord chunkZ, i32 radius)
{

    // 写锁
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    i32 count = 0;
    for (i32 dx = -radius; dx <= radius; ++dx) {
        for (i32 dz = -radius; dz <= radius; ++dz) {
            u64 key = makeKey(chunkX + dx, chunkZ + dz);
            auto it = m_cache.find(key);
            if (it != m_cache.end()) {
                m_cache.erase(it);
                ++count;
            }
        }
    }

    if (count > 0) {
        SPDLOG_TRACE("CollisionCache: Invalidated {} chunks around ({}, {})", count, chunkX, chunkZ);
    }
}

void CollisionCache::clear()
{
    // 写锁
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_cache.clear();

    SPDLOG_DEBUG("CollisionCache: Cleared all caches");
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
    m_hitCount.store(0, std::memory_order_relaxed);
    m_missCount.store(0, std::memory_order_relaxed);
}

// ========== 私有方法实现 ==========

u64 CollisionCache::makeKey(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    // 使用类似于 ChunkPos 的哈希方式
    // 将两个 32 位整数组合成一个 64 位整数
    u64 ux = static_cast<u64>(static_cast<u32>(chunkX));
    u64 uz = static_cast<u64>(static_cast<u32>(chunkZ));
    return (ux << 32) | uz;
}

} // namespace mc::physics
