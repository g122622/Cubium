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

#pragma once

#include "../ColorResolver.hpp"
#include "common/core/Types.hpp"
#include "common/world/WorldConstants.hpp"
#include <array>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace mc {
namespace world::biome {
class Biome;
} // namespace world::biome
using world::biome::Biome;
namespace world::chunk {
class ChunkData;
}
using world::chunk::ChunkData;

namespace client {

/**
 * @brief 生物群系颜色缓存条目
 *
 * 存储单个区块的颜色缓存数据。
 * 每个区块有 16x16 的水平颜色网格（Y轴使用传入的坐标）。
 */
class BiomeColorCacheEntry {
public:
    static constexpr i32 CACHE_SIZE = mc::world::CHUNK_WIDTH; // 每个区块宽度
    static constexpr size_t COLOR_RESOLVER_COUNT = 4;         // 颜色解析器数量（草/树叶/水/干枯植被）

    BiomeColorCacheEntry() = default;

    /**
     * @brief 获取缓存的颜色
     * @param localX 区块内X坐标 (0-15)
     * @param localZ 区块内Z坐标 (0-15)
     * @param resolverId 解析器ID (用于区分草/树叶/水)
     * @return 缓存的颜色，如果未缓存返回 nullopt
     */
    [[nodiscard]] std::optional<u32> getColor(i32 localX, i32 localZ, size_t resolverId) const;

    /**
     * @brief 设置缓存的颜色
     */
    void setColor(i32 localX, i32 localZ, size_t resolverId, u32 color);

    /**
     * @brief 使整个缓存失效
     */
    void invalidate();

    /**
     * @brief 使特定位置失效
     */
    void invalidatePosition(i32 localX, i32 localZ);

private:
    // 每个颜色解析器一个缓存层
    // [resolverId][localZ * CACHE_SIZE + localX]
    std::array<std::array<u32, CACHE_SIZE * CACHE_SIZE>, COLOR_RESOLVER_COUNT> m_caches{};

    // 有效标志位（使用位图）
    // 每个位置3位，分别对应3个解析器
    std::array<u32, CACHE_SIZE * CACHE_SIZE> m_validBits{};

    bool m_fullyInvalid = true;
};

/**
 * @brief 生物群系颜色缓存管理器
 *
 * 管理所有区块的颜色缓存，支持：
 * - 快速查询已计算的颜色
 * - 区块卸载时自动清理
 * - 线程安全访问
 */
class BiomeColorCache {
public:
    BiomeColorCache() = default;

    /**
     * @brief 获取或计算颜色
     *
     * 如果缓存命中则返回缓存值，否则调用计算函数并缓存结果。
     *
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param localX 区块内X坐标 (0-15)
     * @param localZ 区块内Z坐标 (0-15)
     * @param resolverId 解析器ID
     * @param compute 计算函数（缓存未命中时调用）
     * @return 颜色值
     */
    template <typename ComputeFunc>
    [[nodiscard]] u32 getOrCompute(
        ChunkCoord chunkX, ChunkCoord chunkZ, i32 localX, i32 localZ, size_t resolverId, ComputeFunc compute);

    /**
     * @brief 使指定区块的缓存失效
     *
     * 当区块卸载或生物群系变化时调用。
     * 同时清理周围区块的边界缓存。
     *
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     */
    void invalidateChunk(ChunkCoord chunkX, ChunkCoord chunkZ);

    /**
     * @brief 使指定位置的缓存失效
     *
     * 当方块变化可能影响生物群系颜色时调用。
     *
     * @param x 方块X坐标
     * @param z 方块Z坐标
     */
    void invalidatePosition(i32 x, i32 z);

    /**
     * @brief 清空所有缓存
     */
    void clear();

    /**
     * @brief 获取缓存统计信息
     */
    struct Stats {
        size_t totalEntries; // 总缓存条目数
        size_t cacheHits;    // 缓存命中次数
        size_t cacheMisses;  // 缓存未命中次数
    };
    [[nodiscard]] Stats getStats() const;

private:
    /**
     * @brief 获取或创建缓存条目
     */
    [[nodiscard]] BiomeColorCacheEntry& _getOrCreateEntry(ChunkCoord chunkX, ChunkCoord chunkZ);

    /**
     * @brief 构建区块键
     */
    [[nodiscard]] static u64 makeKey(ChunkCoord x, ChunkCoord z) noexcept
    {
        return (static_cast<u64>(static_cast<u32>(x)) << 32) | static_cast<u64>(static_cast<u32>(z));
    }

    std::unordered_map<u64, BiomeColorCacheEntry> m_entries;
    mutable std::mutex m_mutex;

    // 统计
    mutable size_t m_cacheHits = 0;
    mutable size_t m_cacheMisses = 0;
};

// ============================================================================
// 模板实现
// ============================================================================

template <typename ComputeFunc>
u32 BiomeColorCache::getOrCompute(
    ChunkCoord chunkX, ChunkCoord chunkZ, i32 localX, i32 localZ, size_t resolverId, ComputeFunc compute)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const u64 key = makeKey(chunkX, chunkZ);
    auto it = m_entries.find(key);

    if (it != m_entries.end()) {
        auto cached = it->second.getColor(localX, localZ, resolverId);
        if (cached.has_value()) {
            ++m_cacheHits;
            return cached.value();
        }
    }

    // 缓存未命中
    ++m_cacheMisses;

    // 计算颜色
    u32 color = compute();

    // 缓存结果
    auto& entry = _getOrCreateEntry(chunkX, chunkZ);
    entry.setColor(localX, localZ, resolverId, color);

    return color;
}

} // namespace client
} // namespace mc
