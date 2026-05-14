#pragma once

#include "../core/Types.hpp"
#include "../util/AxisAlignedBB.hpp"
#include "../world/chunk/ChunkPos.hpp"
#include <atomic>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace mc::physics {

/**
 * @brief 碰撞箱缓存
 *
 * 缓存区块内的方块碰撞箱，避免每帧重新计算。
 * 参考 MC 1.16.5 的碰撞缓存机制。
 *
 * 线程安全：
 * - 使用读写锁保护缓存访问
 * - 读操作可以并发
 * - 写操作需要独占锁
 *
 * 使用方式：
 * 1. 获取缓存时先调用 getChunkCollisionBoxes()
 * 2. 如果返回 nullptr，说明缓存失效，需要重新计算并缓存
 * 3. 当区块发生变化时，调用 invalidateChunk() 使缓存失效
 */
class CollisionCache {
public:
    /**
     * @brief 缓存条目
     */
    struct ChunkCache {
        std::vector<AxisAlignedBB> boxes; ///< 碰撞箱列表
        u64 version;                      ///< 区块版本号，用于检测变化
    };

    CollisionCache() = default;
    ~CollisionCache() = default;

    // 禁止拷贝
    CollisionCache(const CollisionCache&) = delete;
    CollisionCache& operator=(const CollisionCache&) = delete;

    // 允许移动
    CollisionCache(CollisionCache&&) = default;
    CollisionCache& operator=(CollisionCache&&) = default;

    // ========== 缓存操作 ==========

    /**
     * @brief 获取区块的碰撞箱列表
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @return 碰撞箱列表指针（如果缓存失效或不存在返回 nullptr）
     *
     * 注意：返回的指针在下次修改操作后可能失效
     */
    [[nodiscard]] const std::vector<AxisAlignedBB>* getChunkCollisionBoxes(ChunkCoord chunkX, ChunkCoord chunkZ) const;

    /**
     * @brief 获取区块缓存（包含版本信息）
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @return 缓存条目指针（如果不存在返回 nullptr）
     */
    [[nodiscard]] const ChunkCache* getChunkCache(ChunkCoord chunkX, ChunkCoord chunkZ) const;

    /**
     * @brief 缓存区块碰撞箱
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param boxes 碰撞箱列表（会被移动）
     * @param version 区块版本号（默认为0）
     */
    void cacheChunkCollisionBoxes(
        ChunkCoord chunkX, ChunkCoord chunkZ, std::vector<AxisAlignedBB>&& boxes, u64 version = 0);

    /**
     * @brief 缓存区块碰撞箱（拷贝版本）
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param boxes 碰撞箱列表
     * @param version 区块版本号（默认为0）
     */
    void cacheChunkCollisionBoxes(
        ChunkCoord chunkX, ChunkCoord chunkZ, const std::vector<AxisAlignedBB>& boxes, u64 version = 0);

    /**
     * @brief 使指定区块的缓存失效
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @return 是否成功移除缓存
     */
    bool invalidateChunk(ChunkCoord chunkX, ChunkCoord chunkZ);

    /**
     * @brief 使指定区块及其邻居的缓存失效
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @param radius 影响半径（区块数）
     */
    void invalidateChunkAndNeighbors(ChunkCoord chunkX, ChunkCoord chunkZ, i32 radius = 1);

    /**
     * @brief 清除所有缓存
     */
    void clear();

    // ========== 统计信息 ==========

    /**
     * @brief 获取缓存条目数量
     */
    [[nodiscard]] size_t size() const;

    /**
     * @brief 检查缓存是否为空
     */
    [[nodiscard]] bool empty() const;

    /**
     * @brief 获取缓存命中次数
     */
    [[nodiscard]] u64 hitCount() const { return m_hitCount; }

    /**
     * @brief 获取缓存未命中次数
     */
    [[nodiscard]] u64 missCount() const { return m_missCount; }

    /**
     * @brief 重置统计计数器
     */
    void resetStats();

private:
    /**
     * @brief 生成缓存键
     */
    [[nodiscard]] static u64 makeKey(ChunkCoord chunkX, ChunkCoord chunkZ);

    mutable std::unordered_map<u64, ChunkCache> m_cache;
    mutable std::shared_mutex m_mutex;

    // 统计（使用原子变量保证线程安全）
    mutable std::atomic<u64> m_hitCount{0};
    mutable std::atomic<u64> m_missCount{0};
};

} // namespace mc::physics
