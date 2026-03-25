#include "LightEngineCache.hpp"
#include "../IChunkLightProvider.hpp"
#include "../../chunk/ChunkData.hpp"
#include <algorithm>
#include <cmath>

namespace mc {

LightEngineCache::LightEngineCache() {
    m_chunkCache.fill(nullptr);
    m_emptinessMapCache.fill(nullptr);
}

void LightEngineCache::setProvider(IChunkLightProvider* provider) {
    m_provider = provider;
}

void LightEngineCache::setHeightRange(i32 minSection, i32 maxSection) {
    m_minSection = minSection;
    m_maxSection = maxSection;
    m_sectionCount = maxSection - minSection + 1;
    // 包括上下各一个缓冲区块段
    m_totalLightSections = m_sectionCount + 2;

    // 计算总缓存大小
    m_sectionCacheSize = CHUNK_DIM * CHUNK_DIM * m_totalLightSections;
}

void LightEngineCache::allocateCaches() {
    if (m_sectionCacheSize > 0) {
        m_sectionCache = std::make_unique<const void*[]>(static_cast<size_t>(m_sectionCacheSize));
        m_nibbleCache = std::make_unique<SWMRNibbleArray*[]>(static_cast<size_t>(m_sectionCacheSize));

        // 初始化为 nullptr
        for (i32 i = 0; i < m_sectionCacheSize; ++i) {
            m_sectionCache[static_cast<size_t>(i)] = nullptr;
            m_nibbleCache[static_cast<size_t>(i)] = nullptr;
        }
    }
}

void LightEngineCache::calculateEncodeOffset(i32 centerX, i32 centerY, i32 centerZ) {
    // 缓存偏移量设置为使中心区块位于缓存的中心
    // 对于 5x5 缓存，中心偏移为 2
    m_cacheOffsetX = (centerX >> 4) - CACHE_RADIUS;
    m_cacheOffsetY = m_minSection - 1;  // 包括下方缓冲区
    m_cacheOffsetZ = (centerZ >> 4) - CACHE_RADIUS;

    // 计算索引偏移量
    m_chunkIndexOffset = m_cacheOffsetX + m_cacheOffsetZ * CHUNK_DIM;
    m_sectionIndexOffset = m_chunkIndexOffset + m_cacheOffsetY * CHUNK_DIM * CHUNK_DIM;
}

void LightEngineCache::setupCaches(i32 centerX, i32 centerY, i32 centerZ,
                                    bool relaxed, bool loadTwoRadius) {
    // 计算中心区块坐标
    i32 centerChunkX = centerX >> 4;
    i32 centerChunkZ = centerZ >> 4;

    // 设置编码偏移
    calculateEncodeOffset(centerX, centerY, centerZ);

    // 先清除旧缓存（在分配新缓存之前）
    destroyCaches();

    // 分配缓存
    allocateCaches();

    // 确定加载半径
    i32 radius = loadTwoRadius ? 2 : 1;

    // 预加载区块
    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            i32 chunkX = centerChunkX + dx;
            i32 chunkZ = centerChunkZ + dz;

            // 判断是否在两倍半径边界
            bool isTwoRadius = std::max(std::abs(dx), std::abs(dz)) == 2;

            // 获取区块
            const IChunk* chunk = m_provider->getChunkForLight(chunkX, chunkZ);

            if (chunk == nullptr) {
                if (!relaxed && !isTwoRadius) {
                    // 一倍半径内的区块必须存在
                    continue;
                }
                continue;
            }

            // 验证区块可用性
            if (!canUseChunk(chunk)) {
                continue;
            }

            // 存储区块到缓存
            setChunk(chunkX, chunkZ, chunk);

            // 加载区块段到缓存
            loadSectionsFromChunk(chunkX, chunkZ, chunk);
        }
    }
}

void LightEngineCache::loadSectionsFromChunk(i32 chunkX, i32 chunkZ, const IChunk* chunk) {
    if (chunk == nullptr) {
        return;
    }

    // 遍历所有区块段
    for (i32 sectionY = m_minSection; sectionY <= m_maxSection; ++sectionY) {
        i32 sectionIndex = sectionY - m_minSection;

        // 获取区块段
        const ChunkSection* section = chunk->getSection(sectionIndex);

        // 存储区块段到缓存
        setSection(chunkX, sectionY, chunkZ, section);

        // 检查是否为空区块段
        bool isEmpty = (section == nullptr) || section->isEmpty();
        // 注意：这里不设置空状态，因为需要外部提供 emptiness map
    }
}

void LightEngineCache::destroyCaches() {
    m_chunkCache.fill(nullptr);
    m_emptinessMapCache.fill(nullptr);

    if (m_sectionCache) {
        for (i32 i = 0; i < m_sectionCacheSize; ++i) {
            m_sectionCache[static_cast<size_t>(i)] = nullptr;
        }
    }

    if (m_nibbleCache) {
        for (i32 i = 0; i < m_sectionCacheSize; ++i) {
            m_nibbleCache[static_cast<size_t>(i)] = nullptr;
        }
    }
}

bool LightEngineCache::isSectionEmpty(i32 sectionX, i32 sectionY, i32 sectionZ) const {
    // 首先检查 emptiness map
    i32 chunkIndex = getChunkCacheIndex(sectionX, sectionZ);
    if (chunkIndex >= 0) {
        const bool* emptinessMap = m_emptinessMapCache[static_cast<size_t>(chunkIndex)];
        if (emptinessMap != nullptr) {
            i32 sectionIndex = sectionY - m_minSection;
            if (sectionIndex >= 0 && sectionIndex < m_sectionCount) {
                return emptinessMap[static_cast<size_t>(sectionIndex)];
            }
        }
    }

    // 如果没有 emptiness map，检查区块段
    const void* section = getSection(sectionX, sectionY, sectionZ);
    if (section == nullptr) {
        return true;  // 不存在，视为空
    }

    // 尝试转换为 ChunkSection 并检查
    const ChunkSection* chunkSection = static_cast<const ChunkSection*>(section);
    return chunkSection->isEmpty();
}

void LightEngineCache::setSectionEmpty(i32 sectionX, i32 sectionY, i32 sectionZ, bool empty) {
    i32 chunkIndex = getChunkCacheIndex(sectionX, sectionZ);
    if (chunkIndex < 0) {
        return;
    }

    bool* emptinessMap = const_cast<bool*>(m_emptinessMapCache[static_cast<size_t>(chunkIndex)]);
    if (emptinessMap == nullptr) {
        return;
    }

    i32 sectionIndex = sectionY - m_minSection;
    if (sectionIndex < 0 || sectionIndex >= m_sectionCount) {
        return;
    }

    emptinessMap[static_cast<size_t>(sectionIndex)] = empty;
}

const BlockState* LightEngineCache::getBlockState(i32 worldX, i32 worldY, i32 worldZ) const {
    // 获取区块
    i32 chunkX = worldX >> 4;
    i32 chunkZ = worldZ >> 4;
    const IChunk* chunk = getChunk(chunkX, chunkZ);

    if (chunk == nullptr) {
        return nullptr;  // 空气
    }

    // 获取区块段
    i32 sectionY = worldY >> 4;
    const void* sectionPtr = getSection(chunkX, sectionY, chunkZ);

    if (sectionPtr == nullptr) {
        return nullptr;  // 空气
    }

    const ChunkSection* section = static_cast<const ChunkSection*>(sectionPtr);

    // 检查是否全空气
    if (section->isEmpty()) {
        return nullptr;
    }

    // 获取方块状态
    return chunk->getBlock(worldX & 15, worldY, worldZ & 15);
}

bool LightEngineCache::canUseChunk(const IChunk* chunk) const {
    // 基本检查：区块指针有效
    if (chunk == nullptr) {
        return false;
    }

    // 可以添加更多检查，如区块是否已完成生成等
    return true;
}

f32 LightEngineCache::getCacheHitRate() const {
    u32 total = m_cacheHits + m_cacheMisses;
    if (total == 0) {
        return 0.0f;
    }
    return static_cast<f32>(m_cacheHits) / static_cast<f32>(total);
}

void LightEngineCache::resetStats() {
    m_cacheHits = 0;
    m_cacheMisses = 0;
}

} // namespace mc
