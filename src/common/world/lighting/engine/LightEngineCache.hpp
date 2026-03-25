#pragma once

#include "../../../core/Types.hpp"
#include "../storage/SWMRNibbleArray.hpp"
#include "../../chunk/ChunkPos.hpp"
#include "../../block/Block.hpp"
#include <array>
#include <cstring>
#include <memory>

namespace mc {

// 前向声明
class IChunk;
class IChunkSection;
class BlockState;
class IChunkLightProvider;

/**
 * @brief 光照引擎缓存系统
 *
 * 参考 Starlight 的缓存设计，使用扁平数组存储区块段和光照数据，
 * 避免重复的区块查找调用。
 *
 * 缓存范围：以中心区块为中心的 5x5 区块区域
 * 区块段缓存大小：5 * 5 * (maxSection - minSection + 3)
 *
 * 索引计算：
 * - 区块索引：chunkX + 5 * chunkZ (偏移到 0-4 范围)
 * - 区块段索引：chunkX + 5 * chunkZ + 25 * chunkY
 */
class LightEngineCache {
public:
    // 缓存半径（以区块为单位）
    static constexpr i32 CACHE_RADIUS = 2;
    static constexpr i32 CHUNK_CACHE_SIZE = (2 * CACHE_RADIUS + 1) * (2 * CACHE_RADIUS + 1);  // 5x5 = 25

    // 区块缓存维度
    static constexpr i32 CHUNK_DIM = 2 * CACHE_RADIUS + 1;  // 5

    // 空区块段标记
    static constexpr u16 NULL_SECTION_INDEX = 0xFFFF;

    /**
     * @brief 构造函数
     */
    LightEngineCache();

    /**
     * @brief 设置缓存提供者
     */
    void setProvider(IChunkLightProvider* provider);

    /**
     * @brief 设置世界高度范围
     *
     * @param minSection 最小区块段Y
     * @param maxSection 最大区块段Y
     */
    void setHeightRange(i32 minSection, i32 maxSection);

    /**
     * @brief 初始化缓存
     *
     * 在光照计算开始前调用，预加载所有需要的区块和区块段。
     *
     * @param centerX 中心X坐标（世界坐标）
     * @param centerY 中心Y坐标（世界坐标）
     * @param centerZ 中心Z坐标（世界坐标）
     * @param relaxed 是否宽松模式（允许部分区块缺失）
     * @param loadTwoRadius 是否加载两倍半径的区块
     */
    void setupCaches(i32 centerX, i32 centerY, i32 centerZ,
                     bool relaxed = false, bool loadTwoRadius = false);

    /**
     * @brief 清除缓存
     *
     * 在光照计算完成后调用。
     */
    void destroyCaches();

    // ========================================================================
    // 区块访问
    // ========================================================================

    /**
     * @brief 从缓存获取区块
     *
     * @param chunkX 区块X坐标（世界坐标）
     * @param chunkZ 区块Z坐标（世界坐标）
     * @return 区块指针，如果不在缓存范围或不存在返回nullptr
     */
    [[nodiscard]] const IChunk* getChunk(i32 chunkX, i32 chunkZ) const;

    /**
     * @brief 设置区块到缓存
     */
    void setChunk(i32 chunkX, i32 chunkZ, const IChunk* chunk);

    // ========================================================================
    // 区块段访问（仅用于光照引擎内部）
    // ========================================================================

    /**
     * @brief 获取区块段指针
     *
     * @param sectionX 区块段X坐标
     * @param sectionY 区块段Y坐标
     * @param sectionZ 区块段Z坐标
     * @return 区块段指针，如果不存在返回nullptr
     */
    [[nodiscard]] const void* getSection(i32 sectionX, i32 sectionY, i32 sectionZ) const;

    /**
     * @brief 设置区块段到缓存
     */
    void setSection(i32 sectionX, i32 sectionY, i32 sectionZ, const void* section);

    // ========================================================================
    // 光照数据访问
    // ========================================================================

    /**
     * @brief 获取光照数组
     *
     * @param sectionX 区块段X坐标
     * @param sectionY 区块段Y坐标
     * @param sectionZ 区块段Z坐标
     * @return SWMRNibbleArray指针，如果不存在返回nullptr
     */
    [[nodiscard]] SWMRNibbleArray* getNibble(i32 sectionX, i32 sectionY, i32 sectionZ);
    [[nodiscard]] const SWMRNibbleArray* getNibble(i32 sectionX, i32 sectionY, i32 sectionZ) const;

    /**
     * @brief 设置光照数组到缓存
     */
    void setNibble(i32 sectionX, i32 sectionY, i32 sectionZ, SWMRNibbleArray* nibble);

    // ========================================================================
    // 空区块段检测
    // ========================================================================

    /**
     * @brief 检查区块段是否为空
     *
     * @param sectionX 区块段X坐标
     * @param sectionY 区块段Y坐标
     * @param sectionZ 区块段Z坐标
     * @return 如果区块段全空气返回true
     */
    [[nodiscard]] bool isSectionEmpty(i32 sectionX, i32 sectionY, i32 sectionZ) const;

    /**
     * @brief 设置区块段空状态
     */
    void setSectionEmpty(i32 sectionX, i32 sectionY, i32 sectionZ, bool empty);

    /**
     * @brief 获取空区块段映射
     *
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     * @return 空映射数组指针
     */
    [[nodiscard]] const bool* getEmptinessMap(i32 chunkX, i32 chunkZ) const;

    /**
     * @brief 设置空区块段映射
     */
    void setEmptinessMap(i32 chunkX, i32 chunkZ, const bool* map);

    // ========================================================================
    // 方块状态访问（带缓存）
    // ========================================================================

    /**
     * @brief 获取方块状态（带缓存）
     *
     * @param worldX 世界X坐标
     * @param worldY 世界Y坐标
     * @param worldZ 世界Z坐标
     * @return 方块状态指针，如果是空气返回nullptr
     */
    [[nodiscard]] const BlockState* getBlockState(i32 worldX, i32 worldY, i32 worldZ) const;

    // ========================================================================
    // 光照等级访问（带缓存）
    // ========================================================================

    /**
     * @brief 获取光照等级
     *
     * @param worldX 世界X坐标
     * @param worldY 世界Y坐标
     * @param worldZ 世界Z坐标
     * @return 光照等级 (0-15)
     */
    [[nodiscard]] u8 getLightLevel(i32 worldX, i32 worldY, i32 worldZ) const;

    /**
     * @brief 设置光照等级
     */
    void setLightLevel(i32 worldX, i32 worldY, i32 worldZ, u8 level);

    // ========================================================================
    // 坐标转换
    // ========================================================================

    /**
     * @brief 计算区块缓存索引
     *
     * @return 缓存索引，如果超出范围返回 -1
     */
    [[nodiscard]] i32 getChunkCacheIndex(i32 chunkX, i32 chunkZ) const;

    /**
     * @brief 计算区块段缓存索引
     *
     * @return 缓存索引，如果超出范围返回 -1
     */
    [[nodiscard]] i32 getSectionCacheIndex(i32 sectionX, i32 sectionY, i32 sectionZ) const;

    // ========================================================================
    // 调试信息
    // ========================================================================

    /**
     * @brief 获取缓存命中率
     */
    [[nodiscard]] f32 getCacheHitRate() const;

    /**
     * @brief 重置统计计数器
     */
    void resetStats();

    /**
     * @brief 获取区块缓存大小
     */
    static constexpr i32 getChunkCacheSize() { return CHUNK_CACHE_SIZE; }

    /**
     * @brief 获取区块段缓存大小
     */
    [[nodiscard]] i32 getSectionCacheSize() const { return m_sectionCacheSize; }

private:
    // 缓存提供者
    IChunkLightProvider* m_provider = nullptr;

    // 高度范围
    i32 m_minSection = 0;
    i32 m_maxSection = 15;
    i32 m_sectionCount = 16;  // maxSection - minSection + 1
    i32 m_totalLightSections = 0;  // 包括缓冲区的总段数
    i32 m_sectionCacheSize = 0;    // 区块段缓存总大小

    // 缓存偏移量（中心区块坐标 - 2）
    i32 m_cacheOffsetX = 0;
    i32 m_cacheOffsetY = 0;
    i32 m_cacheOffsetZ = 0;

    // 编码偏移量（用于快速计算索引）
    i32 m_chunkIndexOffset = 0;
    i32 m_sectionIndexOffset = 0;

    // 区块缓存 [5 * 5]
    std::array<const IChunk*, CHUNK_CACHE_SIZE> m_chunkCache{};

    // 区块段缓存（存储为 void* 以适应不同类型）
    // 使用动态大小数组
    std::unique_ptr<const void*[]> m_sectionCache{};

    // 光照数据缓存（SWMRNibbleArray指针）
    std::unique_ptr<SWMRNibbleArray*[]> m_nibbleCache{};

    // 空区块段映射缓存 [5 * 5]
    std::array<const bool*, CHUNK_CACHE_SIZE> m_emptinessMapCache{};

    // 统计信息
    mutable u32 m_cacheHits = 0;
    mutable u32 m_cacheMisses = 0;

    /**
     * @brief 计算编码偏移量
     */
    void calculateEncodeOffset(i32 centerX, i32 centerY, i32 centerZ);

    /**
     * @brief 验证区块是否可用
     */
    [[nodiscard]] bool canUseChunk(const IChunk* chunk) const;

    /**
     * @brief 分配缓存数组
     */
    void allocateCaches();

    /**
     * @brief 从区块加载区块段到缓存
     */
    void loadSectionsFromChunk(i32 chunkX, i32 chunkZ, const IChunk* chunk);

    /**
     * @brief 快速索引计算（无边界检查）
     */
    [[nodiscard]] inline i32 getSectionIndexFast(i32 sectionX, i32 sectionY, i32 sectionZ) const {
        return (sectionX - m_cacheOffsetX) +
               (sectionZ - m_cacheOffsetZ) * CHUNK_DIM +
               (sectionY - m_cacheOffsetY) * CHUNK_DIM * CHUNK_DIM +
               m_sectionIndexOffset;
    }
};

// ============================================================================
// 内联实现
// ============================================================================

inline i32 LightEngineCache::getChunkCacheIndex(i32 chunkX, i32 chunkZ) const {
    i32 dx = chunkX - m_cacheOffsetX;
    i32 dz = chunkZ - m_cacheOffsetZ;

    if (dx < 0 || dx >= CHUNK_DIM || dz < 0 || dz >= CHUNK_DIM) {
        return -1;
    }

    return dx + dz * CHUNK_DIM;
}

inline i32 LightEngineCache::getSectionCacheIndex(i32 sectionX, i32 sectionY, i32 sectionZ) const {
    i32 dx = sectionX - m_cacheOffsetX;
    i32 dz = sectionZ - m_cacheOffsetZ;
    i32 dy = sectionY - m_cacheOffsetY;

    if (dx < 0 || dx >= CHUNK_DIM || dz < 0 || dz >= CHUNK_DIM) {
        return -1;
    }

    if (dy < 0 || dy >= m_totalLightSections) {
        return -1;
    }

    return dx + dz * CHUNK_DIM + dy * CHUNK_DIM * CHUNK_DIM;
}

inline const IChunk* LightEngineCache::getChunk(i32 chunkX, i32 chunkZ) const {
    i32 index = getChunkCacheIndex(chunkX, chunkZ);
    if (index < 0) {
        ++m_cacheMisses;
        return nullptr;
    }

    const IChunk* chunk = m_chunkCache[static_cast<size_t>(index)];
    if (chunk != nullptr) {
        ++m_cacheHits;
    } else {
        ++m_cacheMisses;
    }
    return chunk;
}

inline void LightEngineCache::setChunk(i32 chunkX, i32 chunkZ, const IChunk* chunk) {
    i32 index = getChunkCacheIndex(chunkX, chunkZ);
    if (index >= 0) {
        m_chunkCache[static_cast<size_t>(index)] = chunk;
    }
}

inline const void* LightEngineCache::getSection(i32 sectionX, i32 sectionY, i32 sectionZ) const {
    i32 index = getSectionCacheIndex(sectionX, sectionY, sectionZ);
    if (index < 0 || m_sectionCache == nullptr) {
        return nullptr;
    }
    return m_sectionCache[static_cast<size_t>(index)];
}

inline void LightEngineCache::setSection(i32 sectionX, i32 sectionY, i32 sectionZ, const void* section) {
    i32 index = getSectionCacheIndex(sectionX, sectionY, sectionZ);
    if (index >= 0 && m_sectionCache != nullptr) {
        m_sectionCache[static_cast<size_t>(index)] = section;
    }
}

inline SWMRNibbleArray* LightEngineCache::getNibble(i32 sectionX, i32 sectionY, i32 sectionZ) {
    i32 index = getSectionCacheIndex(sectionX, sectionY, sectionZ);
    if (index < 0 || m_nibbleCache == nullptr) {
        return nullptr;
    }
    return m_nibbleCache[static_cast<size_t>(index)];
}

inline const SWMRNibbleArray* LightEngineCache::getNibble(i32 sectionX, i32 sectionY, i32 sectionZ) const {
    i32 index = getSectionCacheIndex(sectionX, sectionY, sectionZ);
    if (index < 0 || m_nibbleCache == nullptr) {
        return nullptr;
    }
    return m_nibbleCache[static_cast<size_t>(index)];
}

inline void LightEngineCache::setNibble(i32 sectionX, i32 sectionY, i32 sectionZ, SWMRNibbleArray* nibble) {
    i32 index = getSectionCacheIndex(sectionX, sectionY, sectionZ);
    if (index >= 0 && m_nibbleCache != nullptr) {
        m_nibbleCache[static_cast<size_t>(index)] = nibble;
    }
}

inline const bool* LightEngineCache::getEmptinessMap(i32 chunkX, i32 chunkZ) const {
    i32 index = getChunkCacheIndex(chunkX, chunkZ);
    if (index < 0) {
        return nullptr;
    }
    return m_emptinessMapCache[static_cast<size_t>(index)];
}

inline void LightEngineCache::setEmptinessMap(i32 chunkX, i32 chunkZ, const bool* map) {
    i32 index = getChunkCacheIndex(chunkX, chunkZ);
    if (index >= 0) {
        m_emptinessMapCache[static_cast<size_t>(index)] = map;
    }
}

inline u8 LightEngineCache::getLightLevel(i32 worldX, i32 worldY, i32 worldZ) const {
    const SWMRNibbleArray* nibble = getNibble(worldX >> 4, worldY >> 4, worldZ >> 4);
    if (nibble == nullptr || nibble->isNullUpdating()) {
        return 0;
    }
    return nibble->getUpdating(worldX & 15, worldY & 15, worldZ & 15);
}

inline void LightEngineCache::setLightLevel(i32 worldX, i32 worldY, i32 worldZ, u8 level) {
    SWMRNibbleArray* nibble = getNibble(worldX >> 4, worldY >> 4, worldZ >> 4);
    if (nibble != nullptr && !nibble->isNullUpdating()) {
        nibble->set(worldX & 15, worldY & 15, worldZ & 15, level);
    }
}

} // namespace mc
