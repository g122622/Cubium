#pragma once

#include "RegionFile.hpp"
#include "../../../core/Types.hpp"
#include "../../../core/Result.hpp"
#include "../../../world/chunk/ChunkPos.hpp"
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <list>
#include <mutex>

namespace mc::world::save::region {

/**
 * @brief Region 文件缓存
 *
 * 使用 LRU 策略管理打开的 Region 文件。
 * 参考 MC 1.16.5 RegionFileCache.java
 *
 * ## 使用示例
 * ```cpp
 * RegionFileCache cache("saves/MyWorld/region", true);
 *
 * // 读取区块
 * auto chunkResult = cache.readChunk(chunkX, chunkZ);
 * if (chunkResult.success() && chunkResult.value().has_value()) {
 *     auto& nbt = chunkResult.value().value();
 * }
 *
 * // 写入区块
 * cache.writeChunk(chunkX, chunkZ, *nbt);
 *
 * // 同步所有文件
 * cache.flush();
 *
 * // 关闭所有文件
 * cache.close();
 * ```
 *
 * ## 线程安全
 *
 * 所有公共方法都是线程安全的。
 */
class RegionFileCache {
public:
    /// 默认最大缓存数量
    static constexpr u32 DEFAULT_MAX_CACHE_SIZE = 256;

    /**
     * @brief 构造 Region 文件缓存
     *
     * @param regionDir Region 文件目录
     * @param sync 是否同步写入
     * @param maxCacheSize 最大缓存数量
     */
    explicit RegionFileCache(const std::filesystem::path& regionDir,
                              bool sync = false,
                              u32 maxCacheSize = DEFAULT_MAX_CACHE_SIZE);

    ~RegionFileCache();

    // 禁止拷贝
    RegionFileCache(const RegionFileCache&) = delete;
    RegionFileCache& operator=(const RegionFileCache&) = delete;

    // 允许移动
    RegionFileCache(RegionFileCache&& other) noexcept;
    RegionFileCache& operator=(RegionFileCache&& other) noexcept;

    // ========== 区块操作 ==========

    /**
     * @brief 读取区块数据
     *
     * @param chunkX 区块 X 坐标（世界坐标）
     * @param chunkZ 区块 Z 坐标（世界坐标）
     * @return 成功返回 NBT 数据（区块不存在返回 nullopt），失败返回错误
     */
    [[nodiscard]] Result<std::optional<nbt::CompoundTag>>
    readChunk(ChunkCoord chunkX, ChunkCoord chunkZ);

    /**
     * @brief 写入区块数据
     *
     * @param chunkX 区块 X 坐标（世界坐标）
     * @param chunkZ 区块 Z 坐标（世界坐标）
     * @param nbt 区块 NBT 数据
     * @return 成功返回 void，失败返回错误
     */
    Result<void> writeChunk(ChunkCoord chunkX, ChunkCoord chunkZ, const nbt::CompoundTag& nbt);

    /**
     * @brief 检查区块是否存在
     *
     * @param chunkX 区块 X 坐标（世界坐标）
     * @param chunkZ 区块 Z 坐标（世界坐标）
     * @return 如果区块存在返回 true
     */
    [[nodiscard]] bool hasChunk(ChunkCoord chunkX, ChunkCoord chunkZ);

    /**
     * @brief 删除区块
     *
     * @param chunkX 区块 X 坐标（世界坐标）
     * @param chunkZ 区块 Z 坐标（世界坐标）
     * @return 成功返回 void，失败返回错误
     */
    Result<void> deleteChunk(ChunkCoord chunkX, ChunkCoord chunkZ);

    // ========== 同步与关闭 ==========

    /**
     * @brief 同步所有打开的文件到磁盘
     */
    Result<void> flush();

    /**
     * @brief 关闭所有文件
     */
    void close();

    /**
     * @brief 关闭指定 Region 文件
     *
     * @param regionX Region X 坐标
     * @param regionZ Region Z 坐标
     */
    void closeRegion(i32 regionX, i32 regionZ);

    // ========== 信息查询 ==========

    /**
     * @brief 获取 Region 文件目录
     */
    [[nodiscard]] std::filesystem::path regionDir() const { return m_regionDir; }

    /**
     * @brief 获取当前缓存的文件数量
     */
    [[nodiscard]] u32 cacheSize() const;

    /**
     * @brief 获取最大缓存数量
     */
    [[nodiscard]] u32 maxCacheSize() const { return m_maxCacheSize; }

    /**
     * @brief 设置最大缓存数量
     *
     * 如果当前缓存超过新限制，会关闭最久未使用的文件。
     */
    void setMaxCacheSize(u32 maxCacheSize);

private:
    /**
     * @brief 获取或创建 Region 文件
     *
     * @param regionX Region X 坐标
     * @param regionZ Region Z 坐标
     * @return 成功返回 Region 文件引用，失败返回错误
     */
    Result<std::shared_ptr<RegionFile>> getOrCreateRegionFile(i32 regionX, i32 regionZ);

    /**
     * @brief 计算 Region 坐标
     */
    static void getRegionCoords(ChunkCoord chunkX, ChunkCoord chunkZ, i32& regionX, i32& regionZ) {
        regionX = chunkX >> 5;  // chunkX / 32
        regionZ = chunkZ >> 5;  // chunkZ / 32
    }

    /**
     * @brief 计算 Region 键
     */
    static u64 getRegionKey(i32 regionX, i32 regionZ) {
        u64 x = static_cast<u64>(static_cast<i64>(regionX) & 0xFFFFFFFFLL);
        u64 z = static_cast<u64>(static_cast<i64>(regionZ) & 0xFFFFFFFFLL);
        return (x << 32) | z;
    }

    /**
     * @brief 驱逐最久未使用的文件
     */
    void evictOldest();

    /**
     * @brief 更新 LRU 顺序
     */
    void touch(u64 key);

    // ========== 成员变量 ==========

    std::filesystem::path m_regionDir;    ///< Region 文件目录
    bool m_sync;                           ///< 是否同步写入
    u32 m_maxCacheSize;                    ///< 最大缓存数量

    // 缓存映射
    std::unordered_map<u64, std::shared_ptr<RegionFile>> m_cache;

    // LRU 列表（最久未使用在前）
    std::list<u64> m_lruList;

    // LRU 列表迭代器映射
    std::unordered_map<u64, std::list<u64>::iterator> m_lruMap;

    // 线程安全锁
    mutable std::mutex m_mutex;
};

} // namespace mc::world::save::region
