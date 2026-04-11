#include "SkyLightEngine.hpp"
#include "../IChunkLightProvider.hpp"
#include "../../chunk/IChunk.hpp"
#include "../../chunk/ChunkData.hpp"
#include "../../block/Block.hpp"
#include "../../IWorld.hpp"
#include "../../../physics/collision/CollisionShape.hpp"
#include <algorithm>
#include <cstring>
#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

SkyStarLightEngine::SkyStarLightEngine(StarLightLightingProvider* provider)
    : StarLightEngine(true) {  // true = 天空光照

    // 设置世界高度范围
    m_minSection = 0;
    m_maxSection = 15;
    m_minLightSection = -1;
    m_maxLightSection = 16;

    i32 totalLightSections = m_maxLightSection - m_minLightSection + 1;

    // 初始化 Nibble 缓存
    i32 sectionCacheSize = 5 * 5 * (totalLightSections + 2 + 2);
    m_sectionCacheSize = sectionCacheSize;
    m_sectionCache = new const ChunkSection*[static_cast<size_t>(sectionCacheSize)]();
    m_nibbleCache = new SWMRNibbleArray*[static_cast<size_t>(sectionCacheSize)]();
    m_notifyUpdateCache = new bool[static_cast<size_t>(sectionCacheSize)]();

    // 初始化 null 区块段检查缓存
    m_nullPropagationCheckCache.resize(static_cast<size_t>(totalLightSections), false);

    // 初始化队列
    m_increaseQueue.resize(16 * 16 * 16);
    m_decreaseQueue.resize(16 * 16 * 16);

    // 初始化高度图
    m_heightMapBlockChange.fill(INT_MIN);

    (void)provider;  // 暂时未使用
}

// ============================================================================
// 世界设置
// ============================================================================

void SkyStarLightEngine::setWorld(void* world) {
    StarLightEngine::setWorld(world);
    // 缓存已在构造函数中初始化
}

// ============================================================================
// 空映射管理
// ============================================================================

const bool* SkyStarLightEngine::getEmptinessMap(const IChunk* chunk) const {
    return chunk->getSkyEmptinessMap();
}

void SkyStarLightEngine::setEmptinessMap(const IChunk* chunk, const bool* map) {
    // 注意: const_cast 是因为 IChunk 接口定义的 set 方法不是 const
    const_cast<IChunk*>(chunk)->setSkyEmptinessMap(map);
}

SWMRNibbleArray* const* SkyStarLightEngine::getNibblesOnChunk(const IChunk* chunk) const {
    return chunk->getSkyNibbles();
}

void SkyStarLightEngine::setNibbles(const IChunk* chunk, SWMRNibbleArray* const* nibbles) {
    const_cast<IChunk*>(chunk)->setSkyNibbles(nibbles);
}

bool SkyStarLightEngine::canUseChunk(const IChunk* chunk) const {
    // 区块必须处于 LIGHT 状态或之后，且光照数据正确
    ChunkLoadStatus status = chunk->getStatus();
    return status == ChunkLoadStatus::Generated ||
           status == ChunkLoadStatus::Loaded;
}

// ============================================================================
// Nibble 数组管理
// ============================================================================

void SkyStarLightEngine::initNibble(i32 chunkX, i32 chunkY, i32 chunkZ, bool extrude, bool initRemovedNibbles) {
    if (chunkY < m_minLightSection || chunkY > m_maxLightSection) {
        return;
    }

    if (getChunkInCache(chunkX, chunkZ) == nullptr) {
        return;
    }

    SWMRNibbleArray* nibble = getNibbleFromCache(chunkX, chunkY, chunkZ);
    if (nibble == nullptr) {
        if (!initRemovedNibbles) {
            return;
        }
        nibble = new SWMRNibbleArray(nullptr, true);  // Null 状态
        setNibbleInCache(chunkX, chunkY, chunkZ, nibble);
    }

    initNibble(nibble, chunkX, chunkY, chunkZ, extrude);
}

void SkyStarLightEngine::initNibble(SWMRNibbleArray* currNibble, i32 chunkX, i32 chunkY, i32 chunkZ, bool extrude) {
    // 跳过已初始化且有数据的数组
    if (currNibble->isInitializedUpdating()) {
        return;  // 已有数据，不需要重新初始化
    }

    const bool* emptinessMap = StarLightEngine::getEmptinessMap(chunkX, chunkZ);

    // 找到最高非空区块段
    i32 lowestY = m_minLightSection - 1;
    for (i32 currY = m_maxSection; currY >= m_minSection; --currY) {
        if (emptinessMap != nullptr) {
            if (emptinessMap[currY - m_minSection]) {
                continue;
            }
        } else {
            const ChunkSection* current = getChunkSection(chunkX, currY, chunkZ);
            if (current == nullptr || current->isEmpty()) {
                continue;
            }
        }
        lowestY = currY;
        break;
    }

    if (chunkY > lowestY) {
        // 在最高非空区块段之上，设置为全亮
        SWMRNibbleArray* nibble = getNibbleFromCache(chunkX, chunkY, chunkZ);
        if (nibble != nullptr) {
            nibble->setNonNull();
            nibble->setFull();
        }
        return;
    }

    if (extrude) {
        // 从上方非 null 区块段挤出光照
        for (i32 currY = chunkY + 1; currY <= m_maxLightSection; ++currY) {
            SWMRNibbleArray* nibble = getNibbleFromCache(chunkX, currY, chunkZ);
            if (nibble != nullptr && !nibble->isNullUpdating()) {
                currNibble->setNonNull();
                currNibble->extrudeLower(*nibble);
                break;
            }
        }
    } else {
        currNibble->setNonNull();
    }
}

void SkyStarLightEngine::setNibbleNull(i32 chunkX, i32 chunkY, i32 chunkZ) {
    SWMRNibbleArray* nibble = getNibbleFromCache(chunkX, chunkY, chunkZ);
    if (nibble != nullptr) {
        nibble->setNull();
    }
}

// ============================================================================
// Null 区块段检查
// ============================================================================

void SkyStarLightEngine::rewriteNibbleCacheForSkylight(const IChunk* chunk) {
    for (i32 index = 0; index < m_sectionCacheSize; ++index) {
        SWMRNibbleArray* nibble = m_nibbleCache[index];
        if (nibble != nullptr && nibble->isNullUpdating()) {
            // 停止在 null 区域的传播
            m_nibbleCache[index] = nullptr;
            nibble->updateVisible();
        }
    }
}

bool SkyStarLightEngine::checkNullSection(i32 chunkX, i32 chunkY, i32 chunkZ, bool extrudeInitialised) {
    if (chunkY < m_minLightSection || chunkY > m_maxLightSection) {
        return false;
    }

    if (m_nullPropagationCheckCache[static_cast<size_t>(chunkY - m_minLightSection)]) {
        return false;
    }
    m_nullPropagationCheckCache[static_cast<size_t>(chunkY - m_minLightSection)] = true;

    // 检查水平邻居是否有非 null Nibble
    bool needInitNeighbours = false;
    for (i32 dz = -1; dz <= 1 && !needInitNeighbours; ++dz) {
        for (i32 dx = -1; dx <= 1 && !needInitNeighbours; ++dx) {
            SWMRNibbleArray* nibble = getNibbleFromCache(dx + chunkX, chunkY, dz + chunkZ);
            if (nibble != nullptr && !nibble->isNullUpdating()) {
                needInitNeighbours = true;
            }
        }
    }

    if (needInitNeighbours) {
        for (i32 dz = -1; dz <= 1; ++dz) {
            for (i32 dx = -1; dx <= 1; ++dx) {
                initNibble(dx + chunkX, chunkY, dz + chunkZ, (dx | dz) == 0 ? extrudeInitialised : true, true);
            }
        }
    }

    return needInitNeighbours;
}

i32 SkyStarLightEngine::getLightLevelExtruded(i32 worldX, i32 worldY, i32 worldZ) {
    i32 chunkX = worldX >> 4;
    i32 chunkY = worldY >> 4;
    i32 chunkZ = worldZ >> 4;

    SWMRNibbleArray* nibble = getNibbleFromCache(chunkX, chunkY, chunkZ);
    if (nibble != nullptr) {
        return nibble->getUpdating(worldX, worldY, worldZ);
    }

    // 向上查找非 null 区块段
    for (;;) {
        if (++chunkY > m_maxLightSection) {
            return 15;  // 天空光照默认15
        }

        nibble = getNibbleFromCache(chunkX, chunkY, chunkZ);
        if (nibble != nullptr) {
            return nibble->getUpdating(worldX, 0, worldZ);
        }
    }
}

// ============================================================================
// 天空光照传播
// ============================================================================

i32 SkyStarLightEngine::tryPropagateSkylight(IWorld* world, i32 worldX, i32 startY, i32 worldZ,
                                              bool extrudeInitialised, bool delayLightSet) {
    i32 encodeOffset = m_coordinateOffset;
    i32 propagateDirection = getEverythingButDirection(LightAxisDirection::POSITIVE_Y);  // 不检查向上

    if (getLightLevelExtruded(worldX, startY + 1, worldZ) != 15) {
        return startY;
    }

    // 确保此区块段被检查
    checkNullSection(worldX >> 4, startY >> 4, worldZ >> 4, extrudeInitialised);

    const BlockState* above = getBlockState(worldX, startY + 1, worldZ);

    for (i32 currY = startY; currY >= (m_minLightSection << 4); --currY) {
        if ((currY & 15) == 15) {
            checkNullSection(worldX >> 4, currY >> 4, worldZ >> 4, extrudeInitialised);
        }

        const BlockState* current = getBlockState(worldX, currY, worldZ);

        // 条件透明检查：检查从上方方块到当前方块的面是否被遮挡
        // 注意：天空光向下传播，所以方向是 NEGATIVE_Y
        if (above != nullptr && current != nullptr) {
            if (isFaceOccluded(above, current, LightAxisDirection::NEGATIVE_Y)) {
                break;  // 面被遮挡，停止传播
            }
        }

        i32 opacity = 0;
        if (current != nullptr) {
            opacity = std::max(0, current->getBlock().getOpacity(*current));
        }

        if (opacity > 0) {
            break;  // 不透明，停止传播
        }

        // 检查当前方块是否需要形状遮挡检测
        bool hasSidedTransparent = (current != nullptr && current->useShapeForLightOcclusion());

        // 添加到增亮队列（可能延迟设置）
        appendToIncreaseQueue(
            ((worldX + (worldZ << 6) + (currY << 12) + encodeOffset) & 0xFFFFFFFF) |
            (static_cast<u64>(15) << 28) |
            (static_cast<u64>(propagateDirection) << 32) |
            (hasSidedTransparent ? FLAG_HAS_SIDED_TRANSPARENT_BLOCKS : 0)
        );

        above = current;

        if (getNibbleFromCache(worldX >> 4, currY >> 4, worldZ >> 4) == nullptr) {
            // 跳过 null 区块段
            --m_increaseQueueInitialLength;  // 移除最后添加的队列条目
            currY = currY & ~15;  // 跳到下一区块段的顶部
            above = nullptr;  // 空气
        } else if (!delayLightSet) {
            setLightLevel(worldX, currY, worldZ, 15);
        }
    }

    return startY;
}

void SkyStarLightEngine::processDelayedIncreases() {
    i32 decodeOffsetX = -m_encodeOffsetX;
    i32 decodeOffsetY = -m_encodeOffsetY;
    i32 decodeOffsetZ = -m_encodeOffsetZ;

    for (i32 i = 0; i < m_increaseQueueInitialLength; ++i) {
        u64 queueValue = m_increaseQueue[static_cast<size_t>(i)];

        i32 posX = (static_cast<i32>(queueValue) & 63) + decodeOffsetX;
        i32 posZ = ((static_cast<i32>(queueValue) >> 6) & 63) + decodeOffsetZ;
        i32 posY = ((static_cast<i32>(queueValue) >> 12) & 0xFFFF) + decodeOffsetY;
        i32 propagatedLevel = static_cast<i32>((queueValue >> 28) & 0xF);

        setLightLevel(posX, posY, posZ, propagatedLevel);
    }
}

void SkyStarLightEngine::processDelayedDecreases() {
    i32 decodeOffsetX = -m_encodeOffsetX;
    i32 decodeOffsetY = -m_encodeOffsetY;
    i32 decodeOffsetZ = -m_encodeOffsetZ;

    for (i32 i = 0; i < m_decreaseQueueInitialLength; ++i) {
        u64 queueValue = m_decreaseQueue[static_cast<size_t>(i)];

        i32 posX = (static_cast<i32>(queueValue) & 63) + decodeOffsetX;
        i32 posZ = ((static_cast<i32>(queueValue) >> 6) & 63) + decodeOffsetZ;
        i32 posY = ((static_cast<i32>(queueValue) >> 12) & 0xFFFF) + decodeOffsetY;

        setLightLevel(posX, posY, posZ, 0);
    }
}

// ============================================================================
// 方块检查
// ============================================================================

void SkyStarLightEngine::checkBlock(StarLightLightingProvider* lightAccess,
                                     i32 worldX, i32 worldY, i32 worldZ) {
    // 方块可以改变透明度和传播方向

    i32 encodeOffset = m_coordinateOffset;
    i32 currentLevel = getLightLevel(worldX, worldY, worldZ);

    if (currentLevel == 15) {
        // 必须重新传播被覆盖的天空源
        appendToIncreaseQueue(
            ((worldX + (worldZ << 6) + (worldY << 12) + encodeOffset) & 0xFFFFFFFF) |
            (static_cast<u64>(currentLevel & 0xF) << 28) |
            (static_cast<u64>(ALL_DIRECTIONS_BITSET) << 32) |
            FLAG_HAS_SIDED_TRANSPARENT_BLOCKS
        );
    } else {
        setLightLevel(worldX, worldY, worldZ, 0);
    }

    appendToDecreaseQueue(
        ((worldX + (worldZ << 6) + (worldY << 12) + encodeOffset) & 0xFFFFFFFF) |
        (static_cast<u64>(currentLevel & 0xF) << 28) |
        (static_cast<u64>(ALL_DIRECTIONS_BITSET) << 32)
    );
}

// ============================================================================
// 光照计算
// ============================================================================

i32 SkyStarLightEngine::calculateLightValue(StarLightLightingProvider* lightAccess,
                                             i32 worldX, i32 worldY, i32 worldZ,
                                             i32 expected) {
    if (expected == 15) {
        return expected;
    }

    const BlockState* centerState = getBlockState(worldX, worldY, worldZ);

    i32 opacity = 0;
    if (centerState != nullptr) {
        opacity = std::max(1, centerState->getBlock().getOpacity(*centerState));
    }

    // 检查中心方块是否使用形状遮挡
    bool centerUseShape = (centerState != nullptr && centerState->useShapeForLightOcclusion());

    i32 level = 0;
    i32 sectionOffset = m_chunkSectionIndexOffset;

    for (LightAxisDirection dir : ALL_AXIS_DIRECTIONS) {
        i32 dx, dy, dz;
        getDirectionOffset(dir, dx, dy, dz);

        i32 offX = worldX + dx;
        i32 offY = worldY + dy;
        i32 offZ = worldZ + dz;

        i32 sectionIndex = (offX >> 4) + 5 * (offZ >> 4) + (5 * 5) * (offY >> 4) + sectionOffset;
        i32 localIndex = (offX & 15) | ((offZ & 15) << 4) | ((offY & 15) << 8);

        i32 neighbourLevel = getLightLevel(sectionIndex, localIndex);

        if ((neighbourLevel - 1) <= level) {
            continue;
        }

        const BlockState* neighbourState = getBlockState(offX, offY, offZ);

        // 条件透明检查：如果任一方块使用形状遮挡，检查面是否被遮挡
        if (centerUseShape || (neighbourState != nullptr && neighbourState->useShapeForLightOcclusion())) {
            // 光从邻居方块传播到中心方块，所以方向是反的
            if (isFaceOccluded(neighbourState, centerState, getOppositeDirection(dir))) {
                continue;
            }
        }

        i32 calculated = neighbourLevel - opacity;
        level = std::max(calculated, level);

        if (level > expected) {
            return level;
        }
    }

    return level;
}

// ============================================================================
// 传播方块变化
// ============================================================================

void SkyStarLightEngine::propagateBlockChanges(StarLightLightingProvider* lightAccess,
                                                const IChunk* chunk,
                                                const std::vector<BlockPos>& positions) {
    IWorld* world = lightAccess->getWorld();

    rewriteNibbleCacheForSkylight(chunk);
    std::fill(m_nullPropagationCheckCache.begin(), m_nullPropagationCheckCache.end(), false);

    i32 chunkX = chunk->pos().x;
    i32 chunkZ = chunk->pos().z;
    i32 heightMapOffset = chunkX * -16 + (chunkZ * (-16 * 16));

    // 设置高度图用于变化
    for (const BlockPos& pos : positions) {
        i32 index = (pos.x & 15) | ((pos.z & 15) << 4);
        i32 curr = m_heightMapBlockChange[static_cast<size_t>(index)];
        if (pos.y > curr) {
            m_heightMapBlockChange[static_cast<size_t>(index)] = pos.y;
        }
    }

    // 重新计算变化列的源
    for (i32 index = 0; index < 256; ++index) {
        i32 maxY = m_heightMapBlockChange[static_cast<size_t>(index)];
        if (maxY == INT_MIN) {
            continue;  // 未变化
        }
        m_heightMapBlockChange[static_cast<size_t>(index)] = INT_MIN;  // 恢复默认

        i32 columnX = (index & 15) | (chunkX << 4);
        i32 columnZ = (index >> 4) | (chunkZ << 4);

        // 尝试从上方 Y 传播天空光
        i32 maxPropagationY = tryPropagateSkylight(world, columnX, maxY, columnZ, true, true);

        // 移除下方所有 15 级源
        i32 encodeOffset = m_coordinateOffset;
        i32 propagateDirection = getEverythingButDirection(LightAxisDirection::POSITIVE_Y);

        if (getLightLevelExtruded(columnX, maxPropagationY, columnZ) == 15) {
            checkNullSection(columnX >> 4, maxPropagationY >> 4, columnZ >> 4, true);

            for (i32 currY = maxPropagationY; currY >= (m_minLightSection << 4); --currY) {
                if ((currY & 15) == 15) {
                    checkNullSection(columnX >> 4, currY >> 4, columnZ >> 4, true);
                }

                SWMRNibbleArray* nibble = getNibbleFromCache(columnX >> 4, currY >> 4, columnZ >> 4);
                if (nibble == nullptr) {
                    currY = currY & ~15;
                    continue;
                }

                if (nibble->getUpdating(columnX, currY, columnZ) != 15) {
                    break;
                }

                appendToDecreaseQueue(
                    ((columnX + (columnZ << 6) + (currY << 12) + encodeOffset) & 0xFFFFFFFF) |
                    (static_cast<u64>(15) << 28) |
                    (static_cast<u64>(propagateDirection) << 32)
                );
            }
        }
    }

    // 处理延迟的光照设置
    processDelayedIncreases();
    processDelayedDecreases();

    // 检查变化位置
    for (const BlockPos& pos : positions) {
        checkBlock(lightAccess, pos.x, pos.y, pos.z);
    }

    performLightDecrease(lightAccess);
}

// ============================================================================
// 区块照亮
// ============================================================================

void SkyStarLightEngine::lightChunk(StarLightLightingProvider* lightAccess,
                                     const IChunk* chunk, bool needsEdgeChecks) {
    IWorld* world = lightAccess->getWorld();

    rewriteNibbleCacheForSkylight(chunk);
    std::fill(m_nullPropagationCheckCache.begin(), m_nullPropagationCheckCache.end(), false);

    i32 chunkX = chunk->pos().x;
    i32 chunkZ = chunk->pos().z;

    // 获取区块段数组
    const ChunkSection* const* sections = chunk->getSections();

    // 找到最高非空区块段（参考 Moonrise SkyStarLightEngine.lightChunk）
    i32 highestNonEmptySection = m_maxSection;
    while (highestNonEmptySection >= m_minSection &&
           (sections[highestNonEmptySection - m_minSection] == nullptr ||
            sections[highestNonEmptySection - m_minSection]->isEmpty())) {

        // 对于空区块段，将 Nibble 设置为全亮
        // 这是天空光照的关键：空区块段以上都应该有 15 级光照
        SWMRNibbleArray* nibble = getNibbleFromCache(chunkX, highestNonEmptySection, chunkZ);
        if (nibble != nullptr) {
            nibble->setNonNull();
            nibble->setFull();
        }

        checkNullSection(chunkX, highestNonEmptySection, chunkZ, false);

        // 尝试向邻居传播全亮（空区块段需要将全亮传播到邻居）
        for (LightAxisDirection dir : ONLY_HORIZONTAL_DIRECTIONS) {
            i32 dx, dy, dz;
            getDirectionOffset(dir, dx, dy, dz);

            i32 neighbourX = chunkX + dx;
            i32 neighbourZ = chunkZ + dz;
            SWMRNibbleArray* neighbourNibble = getNibbleFromCache(neighbourX, highestNonEmptySection, neighbourZ);
            if (neighbourNibble == nullptr) {
                // 未加载的邻居，跳过
                continue;
            }

            // 计算边界传播参数
            i32 incX, incZ, startX, startZ;
            if (dx != 0) {
                // X 方向
                incX = 0;
                incZ = 1;
                startX = (dx < 0) ? (chunkX << 4) : ((chunkX << 4) | 15);
                startZ = chunkZ << 4;
            } else {
                // Z 方向
                incX = 1;
                incZ = 0;
                startX = chunkX << 4;
                startZ = (dz < 0) ? (chunkZ << 4) : ((chunkZ << 4) | 15);
            }

            i32 encodeOffset = m_coordinateOffset;
            i32 propagateDir = getDirectionBitset(dir);

            // 向邻居传播全亮
            for (i32 currY = highestNonEmptySection << 4, maxY = currY | 15; currY <= maxY; ++currY) {
                for (i32 i = 0, currX = startX, currZ = startZ; i < 16; ++i, currX += incX, currZ += incZ) {
                    appendToIncreaseQueue(
                        ((currX + (currZ << 6) + (currY << 12) + encodeOffset) & 0xFFFFFFFF) |
                        (static_cast<u64>(15) << 28) |  // 全亮
                        (static_cast<u64>(propagateDir) << 32)
                        // 无需 FLAG_HAS_SIDED_TRANSPARENT_BLOCKS，因为区块段是空的
                    );
                }
            }
        }

        --highestNonEmptySection;
    }

    if (highestNonEmptySection >= m_minSection) {
        // 填充其他源（从最高非空区块段的顶层向下传播）
        i32 minX = chunkX << 4;
        i32 maxX = (chunkX << 4) | 15;
        i32 minZ = chunkZ << 4;
        i32 maxZ = (chunkZ << 4) | 15;
        i32 startY = (highestNonEmptySection << 4) | 15;

        for (i32 currZ = minZ; currZ <= maxZ; ++currZ) {
            for (i32 currX = minX; currX <= maxX; ++currX) {
                tryPropagateSkylight(world, currX, startY + 1, currZ, false, false);
            }
        }
    }
    // else: 区块是空的

    spdlog::info("lightChunk: before needsEdgeChecks branch, needsEdgeChecks={}, highestNonEmptySection={}",
                 needsEdgeChecks, highestNonEmptySection);

    if (needsEdgeChecks) {
        // 不需要在这里传播，但可以减少边缘检查的开销
        spdlog::info("lightChunk: calling performLightIncrease (needsEdgeChecks=true)");
        performLightIncrease(lightAccess);
        spdlog::info("lightChunk: performLightIncrease done, calling checkNullSection loop");

        for (i32 y = highestNonEmptySection; y >= m_minLightSection; --y) {
            checkNullSection(chunkX, y, chunkZ, false);
        }
        spdlog::info("lightChunk: checkNullSection loop done, calling checkChunkEdges");

        checkChunkEdges(lightAccess, chunk, m_minLightSection, highestNonEmptySection);
        spdlog::info("lightChunk: checkChunkEdges done");
    } else {
        spdlog::info("lightChunk: needsEdgeChecks=false branch");
        for (i32 y = highestNonEmptySection; y >= m_minLightSection; --y) {
            checkNullSection(chunkX, y, chunkZ, false);
        }
        spdlog::info("lightChunk: checkNullSection loop done, calling propagateNeighbourLevels");

        propagateNeighbourLevels(lightAccess, chunk, m_minLightSection, highestNonEmptySection);
        spdlog::info("lightChunk: propagateNeighbourLevels done, calling performLightIncrease");

        performLightIncrease(lightAccess);
        spdlog::info("lightChunk: performLightIncrease done");
    }
    spdlog::info("lightChunk: END");
}

// ============================================================================
// 区块边缘检查
// ============================================================================

void SkyStarLightEngine::checkChunkEdges(StarLightLightingProvider* lightAccess,
                                          const IChunk* chunk,
                                          i32 fromSection, i32 toSection) {
    std::fill(m_nullPropagationCheckCache.begin(), m_nullPropagationCheckCache.end(), false);
    rewriteNibbleCacheForSkylight(chunk);

    i32 chunkX = chunk->pos().x;
    i32 chunkZ = chunk->pos().z;

    for (i32 y = toSection; y >= fromSection; --y) {
        checkNullSection(chunkX, y, chunkZ, true);
    }

    StarLightEngine::checkChunkEdges(lightAccess, chunk, fromSection, toSection);
}

// ============================================================================
// 邻居传播
// ============================================================================

void SkyStarLightEngine::propagateNeighbourLevels(StarLightLightingProvider* lightAccess,
                                                   const IChunk* chunk,
                                                   i32 fromSection, i32 toSection) {
    i32 chunkX = chunk->pos().x;
    i32 chunkZ = chunk->pos().z;
    i32 encodeOffset = m_coordinateOffset;

    for (i32 currSectionY = toSection; currSectionY >= fromSection; --currSectionY) {
        SWMRNibbleArray* currNibble = getNibbleFromCache(chunkX, currSectionY, chunkZ);
        if (currNibble == nullptr) {
            continue;
        }

        for (LightAxisDirection dir : ONLY_HORIZONTAL_DIRECTIONS) {
            i32 dx, dy, dz;
            getDirectionOffset(dir, dx, dy, dz);

            SWMRNibbleArray* neighbourNibble = getNibbleFromCache(chunkX + dx, currSectionY, chunkZ + dz);
            if (neighbourNibble == nullptr || !neighbourNibble->isInitializedUpdating()) {
                continue;
            }

            // 计算边界传播参数
            i32 startX, startZ, incX, incZ;
            if (dx != 0) {
                incX = 0;
                incZ = 1;
                startX = dx < 0 ? (chunkX << 4) : ((chunkX << 4) | 15);
                startZ = chunkZ << 4;
            } else {
                incX = 1;
                incZ = 0;
                startX = chunkX << 4;
                startZ = dz < 0 ? (chunkZ << 4) : ((chunkZ << 4) | 15);
            }

            i32 propagateDir = getDirectionBitset(getOppositeDirection(dir));

            for (i32 currY = currSectionY << 4, maxY = currY | 15; currY <= maxY; ++currY) {
                for (i32 i = 0, currX = startX, currZ = startZ; i < 16; ++i, currX += incX, currZ += incZ) {
                    i32 level = neighbourNibble->getUpdating((currX & 15) | ((currZ & 15) << 4) | ((currY & 15) << 8));

                    if (level <= 1) {
                        continue;
                    }

                    appendToIncreaseQueue(
                        ((currX + (currZ << 6) + (currY << 12) + encodeOffset) & 0xFFFFFFFF) |
                        (static_cast<u64>(level & 0xF) << 28) |
                        (static_cast<u64>(propagateDir) << 32) |
                        FLAG_HAS_SIDED_TRANSPARENT_BLOCKS
                    );
                }
            }
        }
    }
}

// ============================================================================
// WorldLightManager 接口实现
// ============================================================================

i32 SkyStarLightEngine::tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight) {
    if (!updateSkyLight) {
        return maxUpdates;
    }
    return performUpdates(nullptr, maxUpdates);
}

void SkyStarLightEngine::updateSectionStatus(const SectionPos& pos, bool isEmpty) {
    // 更新区块段的空状态缓存
    // 这用于优化光照传播（空段不需要传播）
    i32 sectionY = pos.y;
    if (sectionY < m_minLightSection || sectionY > m_maxLightSection) {
        return;
    }
    i32 cacheIndex = sectionY - m_minLightSection;
    if (cacheIndex >= 0 && cacheIndex < static_cast<i32>(m_nullPropagationCheckCache.size())) {
        m_nullPropagationCheckCache[static_cast<size_t>(cacheIndex)] = isEmpty;
    }
}

u8 SkyStarLightEngine::getLightFor(i32 x, i32 y, i32 z) const {
    return static_cast<u8>(getLightLevel(x, y, z));
}

void SkyStarLightEngine::setData(const SectionPos& pos, const NibbleArray& array, bool retain) {
    // 设置指定区块段的光照数据
    i32 sectionY = pos.y;
    if (sectionY < m_minLightSection || sectionY > m_maxLightSection) {
        return;
    }

    SWMRNibbleArray* nibble = getNibbleFromCache(pos.x, sectionY, pos.z);
    if (nibble == nullptr) {
        nibble = new SWMRNibbleArray(nullptr, true);
        setNibbleInCache(pos.x, sectionY, pos.z, nibble);
    }

    // 从 NibbleArray 复制数据
    nibble->setNonNull();
    for (i32 i = 0; i < 4096; ++i) {
        nibble->set(i, array.get(i));
    }

    if (!retain) {
        nibble->setHidden();
    }
}

SWMRNibbleArray* SkyStarLightEngine::getData(const SectionPos& pos) {
    i32 sectionY = pos.y;
    if (sectionY < m_minLightSection || sectionY > m_maxLightSection) {
        return nullptr;
    }
    return getNibbleFromCache(pos.x, sectionY, pos.z);
}

void SkyStarLightEngine::setColumnEnabled(i64 columnPos, bool enabled) {
    // 启用或禁用区块列的光照更新
    // 这用于控制光照引擎的活动区域
    if (enabled) {
        m_enabledColumns.insert(columnPos);
    } else {
        m_enabledColumns.erase(columnPos);
    }
}

} // namespace mc
