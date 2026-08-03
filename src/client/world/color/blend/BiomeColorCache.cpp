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

#include "BiomeColorCache.hpp"
#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"
#include <cstddef>
#include <mutex>
#include <optional>

namespace mc::client {

// ============================================================================
// BiomeColorCacheEntry 实现
// ============================================================================

std::optional<u32> BiomeColorCacheEntry::getColor(i32 localX, i32 localZ, size_t resolverId) const
{
    if (m_fullyInvalid) {
        return {};
    }

    const i32 index = localZ * CACHE_SIZE + localX;
    const u32 validBits = m_validBits[index];

    // 检查该解析器的位是否设置
    if ((validBits & (1u << resolverId)) == 0) {
        return {};
    }

    return m_caches[resolverId][index];
}

void BiomeColorCacheEntry::setColor(i32 localX, i32 localZ, size_t resolverId, u32 color)
{
    const i32 index = localZ * CACHE_SIZE + localX;

    m_caches[resolverId][index] = color;
    m_validBits[index] |= (1u << resolverId);
    m_fullyInvalid = false;
}

void BiomeColorCacheEntry::invalidate()
{
    m_validBits.fill(0);
    m_fullyInvalid = true;
}

void BiomeColorCacheEntry::invalidatePosition(i32 localX, i32 localZ)
{
    const i32 index = localZ * CACHE_SIZE + localX;
    m_validBits[index] = 0;
}

// ============================================================================
// BiomeColorCache 实现
// ============================================================================

BiomeColorCacheEntry& BiomeColorCache::_getOrCreateEntry(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    const u64 key = makeKey(chunkX, chunkZ);
    auto it = m_entries.find(key);

    if (it != m_entries.end()) {
        return it->second;
    }

    auto [inserted, _] = m_entries.emplace(key, BiomeColorCacheEntry{});
    return inserted->second;
}

void BiomeColorCache::invalidateChunk(ChunkCoord chunkX, ChunkCoord chunkZ)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const u64 key = makeKey(chunkX, chunkZ);
    auto it = m_entries.find(key);

    if (it != m_entries.end()) {
        it->second.invalidate();
    }

    // 同时使周围区块的边缘缓存失效（因为混合需要邻居数据）
    for (i32 dx = -1; dx <= 1; ++dx) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            if (dx == 0 && dz == 0) continue;

            const u64 neighborKey = makeKey(chunkX + dx, chunkZ + dz);
            auto neighborIt = m_entries.find(neighborKey);

            if (neighborIt != m_entries.end()) {
                // 只使边缘位置失效
                // 如果邻居在东/西边，失效对应的列
                // 如果邻居在北/南边，失效对应的行
                auto& entry = neighborIt->second;

                if (dx == -1) {
                    // 邻居在西边，失效东边缘 (localX = CACHE_SIZE - 1)
                    for (i32 z = 0; z < BiomeColorCacheEntry::CACHE_SIZE; ++z) {
                        entry.invalidatePosition(BiomeColorCacheEntry::CACHE_SIZE - 1, z);
                    }
                } else if (dx == 1) {
                    // 邻居在东边，失效西边缘 (localX = 0)
                    for (i32 z = 0; z < BiomeColorCacheEntry::CACHE_SIZE; ++z) {
                        entry.invalidatePosition(0, z);
                    }
                }

                if (dz == -1) {
                    // 邻居在北边，失效南边缘 (localZ = CACHE_SIZE - 1)
                    for (i32 x = 0; x < BiomeColorCacheEntry::CACHE_SIZE; ++x) {
                        entry.invalidatePosition(x, BiomeColorCacheEntry::CACHE_SIZE - 1);
                    }
                } else if (dz == 1) {
                    // 邻居在南边，失效北边缘 (localZ = 0)
                    for (i32 x = 0; x < BiomeColorCacheEntry::CACHE_SIZE; ++x) {
                        entry.invalidatePosition(x, 0);
                    }
                }
            }
        }
    }
}

void BiomeColorCache::invalidatePosition(i32 x, i32 z)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const ChunkCoord chunkX = x >> world::CHUNK_SHIFT;
    const ChunkCoord chunkZ = z >> world::CHUNK_SHIFT;
    const i32 localX = x & world::CHUNK_MASK;
    const i32 localZ = z & world::CHUNK_MASK;

    const u64 key = makeKey(chunkX, chunkZ);
    auto it = m_entries.find(key);

    if (it != m_entries.end()) {
        it->second.invalidatePosition(localX, localZ);
    }
}

void BiomeColorCache::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.clear();
    m_cacheHits = 0;
    m_cacheMisses = 0;
}

BiomeColorCache::Stats BiomeColorCache::getStats() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    Stats stats;
    stats.totalEntries = m_entries.size();
    stats.cacheHits = m_cacheHits;
    stats.cacheMisses = m_cacheMisses;
    return stats;
}

} // namespace mc::client
