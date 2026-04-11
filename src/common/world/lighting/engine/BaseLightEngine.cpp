#include "BaseLightEngine.hpp"
#include "../IChunkLightProvider.hpp"
#include "../../chunk/IChunk.hpp"
#include "../../chunk/ChunkData.hpp"
#include "../../block/Block.hpp"
#include "../../IWorld.hpp"
#include "../../../util/Direction.hpp"
#include "../../../physics/collision/CollisionShape.hpp"
#include "../../../physics/shape/Shapes.hpp"
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// 静态成员初始化
// ============================================================================

std::array<std::vector<LightAxisDirection>, 64> StarLightEngine::s_oldCheckDirections;
bool StarLightEngine::s_directionsInitialized = false;

void StarLightEngine::initializeDirections() {
    if (s_directionsInitialized) return;

    for (i32 i = 0; i < 64; ++i) {
        std::vector<LightAxisDirection> directions;
        for (i32 bitset = i, count = 0; count < 6 && bitset != 0; ++count) {
            i32 trailing = bitset & -bitset;  // 获取最低位1
            i32 index = 0;
            while ((1 << index) != trailing) ++index;
            directions.push_back(static_cast<LightAxisDirection>(index));
            bitset ^= trailing;
        }
        s_oldCheckDirections[static_cast<size_t>(i)] = std::move(directions);
    }
    s_directionsInitialized = true;
}

// ============================================================================
// 构造函数
// ============================================================================

StarLightEngine::StarLightEngine(bool isSkyLight)
    : m_isSkyLight(isSkyLight)
    , m_emittedLightMask(isSkyLight ? 0 : 0xF) {

    initializeDirections();
    m_chunkCache.fill(nullptr);
    m_emptinessMapCache.fill(nullptr);
}

// ============================================================================
// 缓存管理
// ============================================================================

void StarLightEngine::setWorld(void* world) {
    m_world = world;
    // 子类会设置更多参数
}

void StarLightEngine::setupEncodeOffset(i32 centerX, i32 centerY, i32 centerZ) {
    // 31 = center + encodeOffset，使中心坐标映射到 [0, 62] 范围
    m_encodeOffsetX = 31 - centerX;
    m_encodeOffsetY = -(m_minLightSection - 1) << 4;  // 使最小光照段Y=0
    m_encodeOffsetZ = 31 - centerZ;

    // coordinateIndex = x | (z << 6) | (y << (6 + 6))
    m_coordinateOffset = m_encodeOffsetX + (m_encodeOffsetZ << 6) + (m_encodeOffsetY << 12);

    // 2 = (centerX >> 4) + chunkOffset，使中心区块在缓存中心
    m_chunkOffsetX = 2 - (centerX >> 4);
    m_chunkOffsetY = -(m_minLightSection - 1);  // 最低段Y=0
    m_chunkOffsetZ = 2 - (centerZ >> 4);

    // chunk index = x + (5 * z)
    m_chunkIndexOffset = m_chunkOffsetX + (5 * m_chunkOffsetZ);

    // chunk section index = x + (5 * z) + ((5 * 5) * y)
    m_chunkSectionIndexOffset = m_chunkIndexOffset + ((5 * 5) * m_chunkOffsetY);
}

void StarLightEngine::setupCaches(StarLightLightingProvider* lightAccess,
                                   i32 centerX, i32 centerY, i32 centerZ,
                                   bool relaxed, bool loadTwoRadius) {
    i32 centerChunkX = centerX >> 4;
    i32 centerChunkY = centerY >> 4;
    i32 centerChunkZ = centerZ >> 4;

    setupEncodeOffset(centerChunkX * 16 + 7, centerChunkY * 16 + 7, centerChunkZ * 16 + 7);

    i32 radius = loadTwoRadius ? 2 : 1;

    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            i32 cx = centerChunkX + dx;
            i32 cz = centerChunkZ + dz;
            bool isTwoRadius = std::max(std::abs(dx), std::abs(dz)) == 2;

            const IChunk* chunk = lightAccess->getChunkForLight(cx, cz);
            if (chunk == nullptr) {
                if (relaxed || isTwoRadius) {
                    continue;
                }
                // 非宽松模式下一倍半径内的区块必须存在
                continue;
            }

            if (!canUseChunk(chunk)) {
                continue;
            }

            setChunkInCache(cx, cz, chunk);
            setEmptinessMapCache(cx, cz, getEmptinessMap(chunk));

            if (!isTwoRadius) {
                // 设置区块段和 Nibble 数组到缓存
                setBlocksForChunkInCache(cx, cz, chunk->getSections());
                setNibblesForChunkInCache(cx, cz, getNibblesOnChunk(chunk));
            }
        }
    }
}

void StarLightEngine::destroyCaches() {
    m_chunkCache.fill(nullptr);
    m_emptinessMapCache.fill(nullptr);

    if (m_sectionCache != nullptr) {
        for (i32 i = 0; i < m_sectionCacheSize; ++i) {
            m_sectionCache[i] = nullptr;
        }
    }

    if (m_nibbleCache != nullptr) {
        for (i32 i = 0; i < m_sectionCacheSize; ++i) {
            m_nibbleCache[i] = nullptr;
        }
    }

    if (m_notifyUpdateCache != nullptr) {
        for (i32 i = 0; i < m_sectionCacheSize; ++i) {
            m_notifyUpdateCache[i] = false;
        }
    }
}

void StarLightEngine::updateVisible(StarLightLightingProvider* lightAccess) {
    for (i32 index = 0, max = m_sectionCacheSize; index < max; ++index) {
        SWMRNibbleArray* nibble = m_nibbleCache[index];
        if (nibble == nullptr) {
            continue;
        }

        bool shouldNotify = m_notifyUpdateCache != nullptr && m_notifyUpdateCache[index];
        if (!shouldNotify && !nibble->isDirty()) {
            continue;
        }

        i32 chunkX = (index % 5) - m_chunkOffsetX;
        i32 chunkZ = ((index / 5) % 5) - m_chunkOffsetZ;
        i32 ySections = m_maxSection - m_minSection + 1;
        i32 chunkY = ((index / (5 * 5)) % (ySections + 2 + 2)) - m_chunkOffsetY;

        if (nibble->updateVisible() || shouldNotify) {
            lightAccess->markLightChanged(
                m_isSkyLight ? LightType::SKY : LightType::BLOCK,
                SectionPos(chunkX, chunkY, chunkZ)
            );
        }
    }
}

// ============================================================================
// 区块光照操作
// ============================================================================

void StarLightEngine::blocksChangedInChunk(StarLightLightingProvider* lightAccess,
                                             i32 chunkX, i32 chunkZ,
                                             const std::vector<BlockPos>& positions,
                                             const std::vector<bool>& changedSections) {
    setupCaches(lightAccess, chunkX * 16 + 7, 128, chunkZ * 16 + 7, true, true);

    const IChunk* chunk = getChunkInCache(chunkX, chunkZ);
    if (chunk == nullptr) {
        destroyCaches();
        return;
    }

    // 处理空区块段变化
    if (!changedSections.empty()) {
        std::vector<bool> emptinessChanges(changedSections.size());
        for (size_t i = 0; i < changedSections.size(); ++i) {
            emptinessChanges[i] = changedSections[i];
        }
        std::vector<bool> result = handleEmptySectionChanges(lightAccess, chunk, emptinessChanges, false);
        if (!result.empty()) {
            // 注意: std::vector<bool> 没有 data() 方法，需要转换
            // 这里暂时使用空指针，子类应该管理自己的空映射存储
            setEmptinessMap(chunk, nullptr);
        }
    }

    // 传播方块变化
    if (!positions.empty()) {
        propagateBlockChanges(lightAccess, chunk, positions);
    }

    updateVisible(lightAccess);
    destroyCaches();
}

std::vector<bool> StarLightEngine::handleEmptySectionChanges(StarLightLightingProvider* lightAccess,
                                                              const IChunk* chunk,
                                                              const std::vector<bool>& emptinessChanges,
                                                              bool isUnlit) {
    ChunkPos chunkPos = chunk->pos();
    i32 chunkX = chunkPos.x;
    i32 chunkZ = chunkPos.z;

    std::vector<bool> chunkEmptinessMap;
    const bool* existingMap = getEmptinessMap(chunkX, chunkZ);
    bool needsInit = isUnlit || existingMap == nullptr;

    if (needsInit) {
        chunkEmptinessMap.resize(static_cast<size_t>(m_maxSection - m_minSection + 1), false);
    }

    // 更新空映射
    for (i32 sectionIndex = static_cast<i32>(emptinessChanges.size()) - 1; sectionIndex >= 0; --sectionIndex) {
        bool isEmpty = emptinessChanges[static_cast<size_t>(sectionIndex)];
        i32 sectionY = sectionIndex + m_minSection;

        if (needsInit) {
            chunkEmptinessMap[static_cast<size_t>(sectionIndex)] = isEmpty;
        }

        if (isEmpty) {
            continue;
        }

        // 非空区块段：初始化邻居 Nibble 数组
        for (i32 dz = -1; dz <= 1; ++dz) {
            for (i32 dx = -1; dx <= 1; ++dx) {
                bool extrude = (dx | dz) != 0 || !isUnlit;
                for (i32 dy = 1; dy >= -1; --dy) {
                    initNibble(dx + chunk->x(), dy + sectionY, dz + chunk->z(), extrude, false);
                }
            }
        }
    }

    // 检查是否需要反初始化或延迟初始化
    for (i32 dz = -1; dz <= 1; ++dz) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            // 检查邻居是否都有空映射
            bool neighboursLoaded = true;
            for (i32 dz2 = -1; dz2 <= 1 && neighboursLoaded; ++dz2) {
                for (i32 dx2 = -1; dx2 <= 1 && neighboursLoaded; ++dx2) {
                    if (getEmptinessMap(dx + dx2 + chunk->x(), dz + dz2 + chunk->z()) == nullptr) {
                        neighboursLoaded = false;
                    }
                }
            }

            for (i32 sectionY = m_maxLightSection; sectionY >= m_minLightSection; --sectionY) {
                bool allEmpty = true;
                for (i32 dy2 = -1; dy2 <= 1 && allEmpty; ++dy2) {
                    for (i32 dz2 = -1; dz2 <= 1 && allEmpty; ++dz2) {
                        for (i32 dx2 = -1; dx2 <= 1 && allEmpty; ++dx2) {
                            i32 y = sectionY + dy2;
                            if (y < m_minSection || y > m_maxSection) {
                                continue;
                            }
                            const bool* emptinessMap = getEmptinessMap(dx + dx2 + chunk->x(), dz + dz2 + chunk->z());
                            if (emptinessMap != nullptr) {
                                if (!emptinessMap[static_cast<size_t>(y - m_minSection)]) {
                                    allEmpty = false;
                                }
                            } else {
                                // 没有空映射，需要检查区块段
                                const ChunkSection* section = getChunkSection(dx + dx2 + chunk->x(), y, dz + dz2 + chunk->z());
                                if (section != nullptr && !section->isEmpty()) {
                                    allEmpty = false;
                                }
                            }
                        }
                    }
                }

                if (allEmpty && neighboursLoaded) {
                    // 可以反初始化
                    setNibbleNull(dx + chunk->x(), sectionY, dz + chunk->z());
                } else if (!allEmpty) {
                    // 必须初始化
                    bool extrude = (dx | dz) != 0 || !isUnlit;
                    initNibble(dx + chunk->x(), sectionY, dz + chunk->z(), extrude, false);
                }
            }
        }
    }

    return needsInit ? chunkEmptinessMap : std::vector<bool>{};
}

void StarLightEngine::checkChunkEdges(StarLightLightingProvider* lightAccess, i32 chunkX, i32 chunkZ) {
    setupCaches(lightAccess, chunkX * 16 + 7, 128, chunkZ * 16 + 7, true, false);

    const IChunk* chunk = getChunkInCache(chunkX, chunkZ);
    if (chunk != nullptr) {
        checkChunkEdges(lightAccess, chunk, m_minLightSection, m_maxLightSection);
        updateVisible(lightAccess);
    }

    destroyCaches();
}

void StarLightEngine::checkChunkEdges(StarLightLightingProvider* lightAccess, const IChunk* chunk,
                                       i32 fromSection, i32 toSection) {
    ChunkPos chunkPos = chunk->pos();
    i32 chunkX = chunkPos.x;
    i32 chunkZ = chunkPos.z;

    for (i32 sectionY = toSection; sectionY >= fromSection; --sectionY) {
        checkChunkEdge(lightAccess, chunk, chunkX, sectionY, chunkZ);
    }

    performLightDecrease(lightAccess);
}

void StarLightEngine::checkChunkEdge(StarLightLightingProvider* lightAccess, const IChunk* chunk,
                                      i32 chunkX, i32 chunkY, i32 chunkZ) {
    SWMRNibbleArray* currNibble = getNibbleFromCache(chunkX, chunkY, chunkZ);
    if (currNibble == nullptr) {
        return;
    }

    for (LightAxisDirection dir : ONLY_HORIZONTAL_DIRECTIONS) {
        i32 dx, dy, dz;
        getDirectionOffset(dir, dx, dy, dz);

        SWMRNibbleArray* neighbourNibble = getNibbleFromCache(chunkX + dx, chunkY, chunkZ + dz);
        if (neighbourNibble == nullptr) {
            continue;
        }

        if (!currNibble->isInitializedUpdating() && !neighbourNibble->isInitializedUpdating()) {
            // 两者都是零，无需检查
            continue;
        }

        // 当前区块
        i32 incX, incZ, startX, startZ;
        if (dx != 0) {
            // X 方向
            incX = 0;
            incZ = 1;
            if (dx < 0) {
                // 负方向
                startX = chunkX << 4;
            } else {
                startX = (chunkX << 4) | 15;
            }
            startZ = chunkZ << 4;
        } else {
            // Z 方向
            incX = 1;
            incZ = 0;
            if (dz < 0) {
                // 负方向
                startZ = chunkZ << 4;
            } else {
                startZ = (chunkZ << 4) | 15;
            }
            startX = chunkX << 4;
        }

        i32 centerDelayedChecks = 0;
        i32 neighbourDelayedChecks = 0;
        for (i32 currY = chunkY << 4, maxY = currY | 15; currY <= maxY; ++currY) {
            for (i32 i = 0, currX = startX, currZ = startZ; i < 16; ++i, currX += incX, currZ += incZ) {
                i32 neighbourX = currX + dx;
                i32 neighbourZ = currZ + dz;

                i32 currentIndex = (currX & 15) | ((currZ & 15) << 4) | ((currY & 15) << 8);
                i32 currentLevel = currNibble->getUpdating(currentIndex);

                i32 neighbourIndex = (neighbourX & 15) | ((neighbourZ & 15) << 4) | ((currY & 15) << 8);
                i32 neighbourLevel = neighbourNibble->getUpdating(neighbourIndex);

                // 延迟检查，因为 checkBlock 方法会覆盖光照值，这会影响后续的 calculateLightValue 操作
                // 虽然这种行为上不显著，但会因排队更多值而影响性能

                if (calculateLightValue(lightAccess, currX, currY, currZ, currentLevel) != currentLevel) {
                    m_chunkCheckDelayedUpdatesCenter[static_cast<size_t>(centerDelayedChecks++)] = currentIndex;
                }

                if (calculateLightValue(lightAccess, neighbourX, currY, neighbourZ, neighbourLevel) != neighbourLevel) {
                    m_chunkCheckDelayedUpdatesNeighbour[static_cast<size_t>(neighbourDelayedChecks++)] = neighbourIndex;
                }
            }
        }

        i32 currentChunkOffX = chunkX << 4;
        i32 currentChunkOffZ = chunkZ << 4;
        i32 neighbourChunkOffX = (chunkX + dx) << 4;
        i32 neighbourChunkOffZ = (chunkZ + dz) << 4;
        i32 chunkOffY = chunkY << 4;
        for (i32 i = 0, len = std::max(centerDelayedChecks, neighbourDelayedChecks); i < len; ++i) {
            // 尝试将邻居数据一起排队
            // index = x | (z << 4) | (y << 8)
            if (i < centerDelayedChecks) {
                i32 value = m_chunkCheckDelayedUpdatesCenter[static_cast<size_t>(i)];
                checkBlock(lightAccess, currentChunkOffX | (value & 15),
                          chunkOffY | (value >> 8),
                          currentChunkOffZ | ((value >> 4) & 0xF));
            }
            if (i < neighbourDelayedChecks) {
                i32 value = m_chunkCheckDelayedUpdatesNeighbour[static_cast<size_t>(i)];
                checkBlock(lightAccess, neighbourChunkOffX | (value & 15),
                          chunkOffY | (value >> 8),
                          neighbourChunkOffZ | ((value >> 4) & 0xF));
            }
        }
    }
}

void StarLightEngine::propagateNeighbourLevels(StarLightLightingProvider* lightAccess, const IChunk* chunk,
                                                i32 fromSection, i32 toSection) {
    ChunkPos chunkPos = chunk->pos();
    i32 chunkX = chunkPos.x;
    i32 chunkZ = chunkPos.z;
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
                // 无法从 0 拉取
                continue;
            }

            // 邻居区块
            i32 incX, incZ, startX, startZ;
            if (dx != 0) {
                // X 方向
                incX = 0;
                incZ = 1;
                if (dx < 0) {
                    // 负方向
                    startX = (chunkX << 4) - 1;
                } else {
                    startX = (chunkX << 4) + 16;
                }
                startZ = chunkZ << 4;
            } else {
                // Z 方向
                incX = 1;
                incZ = 0;
                if (dz < 0) {
                    // 负方向
                    startZ = (chunkZ << 4) - 1;
                } else {
                    startZ = (chunkZ << 4) + 16;
                }
                startX = chunkX << 4;
            }

            i32 propagateDirection = getDirectionBitset(getOppositeDirection(dir));  // 只想在这个方向检查向这个区块的传播
            i32 sectionOffset = m_chunkSectionIndexOffset;

            for (i32 currY = currSectionY << 4, maxY = currY | 15; currY <= maxY; ++currY) {
                for (i32 i = 0, currX = startX, currZ = startZ; i < 16; ++i, currX += incX, currZ += incZ) {
                    i32 level = neighbourNibble->getUpdating(
                        (currX & 15) | ((currZ & 15) << 4) | ((currY & 15) << 8)
                    );

                    if (level <= 1) {
                        // 无需传播
                        continue;
                    }

                    appendToIncreaseQueue(
                        ((currX + (currZ << 6) + (currY << 12) + encodeOffset) & 0xFFFFFFFF) |
                        (static_cast<u64>(level & 0xF) << 28) |
                        (static_cast<u64>(propagateDirection) << 32) |
                        FLAG_HAS_SIDED_TRANSPARENT_BLOCKS  // 不知道当前方块是否透明，必须检查
                    );
                }
            }
        }
    }
}

void StarLightEngine::lightChunk(StarLightLightingProvider* lightAccess, const IChunk* chunk, bool needsEdgeChecks) {
    // 子类实现
}

void StarLightEngine::light(StarLightLightingProvider* lightAccess, const IChunk* chunk, bool needsEdgeChecks) {
    if (chunk == nullptr) {
        return;
    }

    i32 chunkX = chunk->x();
    i32 chunkZ = chunk->z();
    i32 centerX = chunkX * 16 + 7;
    i32 centerY = 128;  // 中间高度
    i32 centerZ = chunkZ * 16 + 7;

    // 重置队列状态（确保多次调用不会累积队列条目）
    m_increaseQueueInitialLength = 0;
    m_decreaseQueueInitialLength = 0;
    m_needsUpdate = false;

    // 初始化缓存
    setupCaches(lightAccess, centerX, centerY, centerZ, true, true);

    try {
        // 设置当前区块到缓存
        setChunkInCache(chunkX, chunkZ, chunk);

        // 设置区块段到缓存
        const ChunkSection* const* sections = chunk->getSections();
        if (sections != nullptr) {
            setBlocksForChunkInCache(chunkX, chunkZ, sections);
        }

        // 设置 Nibble 数组（从区块获取）
        SWMRNibbleArray* const* nibbles = getNibblesOnChunk(chunk);
        if (nibbles != nullptr) {
            setNibblesForChunkInCache(chunkX, chunkZ, nibbles);
        }

        // 设置空映射
        const bool* emptinessMap = getEmptinessMap(chunk);
        setEmptinessMapCache(chunkX, chunkZ, emptinessMap);

        // 执行光照计算
        lightChunk(lightAccess, chunk, needsEdgeChecks);

        // 更新可见数据
        updateVisible(lightAccess);
    } catch (...) {
        destroyCaches();
        throw;
    }

    destroyCaches();
}

// ============================================================================
// 缓存访问方法
// ============================================================================

const IChunk* StarLightEngine::getChunkInCache(i32 chunkX, i32 chunkZ) const {
    i32 dx = chunkX + m_chunkOffsetX;
    i32 dz = chunkZ + m_chunkOffsetZ;
    if (dx < 0 || dx >= 5 || dz < 0 || dz >= 5) {
        return nullptr;
    }
    return m_chunkCache[static_cast<size_t>(dx + dz * 5)];
}

void StarLightEngine::setChunkInCache(i32 chunkX, i32 chunkZ, const IChunk* chunk) {
    i32 dx = chunkX + m_chunkOffsetX;
    i32 dz = chunkZ + m_chunkOffsetZ;
    if (dx >= 0 && dx < 5 && dz >= 0 && dz < 5) {
        m_chunkCache[static_cast<size_t>(dx + dz * 5)] = chunk;
    }
}

const ChunkSection* StarLightEngine::getChunkSection(i32 chunkX, i32 chunkY, i32 chunkZ) const {
    if (m_sectionCache == nullptr) {
        return nullptr;
    }
    i32 index = chunkX + 5 * chunkZ + (5 * 5) * chunkY + m_chunkSectionIndexOffset;
    if (index < 0 || index >= m_sectionCacheSize) {
        return nullptr;
    }
    return static_cast<const ChunkSection*>(m_sectionCache[index]);
}

void StarLightEngine::setChunkSectionInCache(i32 chunkX, i32 chunkY, i32 chunkZ, const ChunkSection* section) {
    if (m_sectionCache == nullptr) {
        return;
    }
    i32 index = chunkX + 5 * chunkZ + (5 * 5) * chunkY + m_chunkSectionIndexOffset;
    if (index >= 0 && index < m_sectionCacheSize) {
        m_sectionCache[index] = section;
    }
}

void StarLightEngine::setBlocksForChunkInCache(i32 chunkX, i32 chunkZ, const ChunkSection* const* sections) {
    for (i32 cy = m_minLightSection; cy <= m_maxLightSection; ++cy) {
        setChunkSectionInCache(chunkX, cy, chunkZ,
            sections == nullptr ? nullptr : (cy >= m_minSection && cy <= m_maxSection ? sections[cy - m_minSection] : nullptr));
    }
}

SWMRNibbleArray* StarLightEngine::getNibbleFromCache(i32 chunkX, i32 chunkY, i32 chunkZ) const {
    if (m_nibbleCache == nullptr) {
        return nullptr;
    }
    i32 index = chunkX + 5 * chunkZ + (5 * 5) * chunkY + m_chunkSectionIndexOffset;
    if (index < 0 || index >= m_sectionCacheSize) {
        return nullptr;
    }
    return m_nibbleCache[index];
}

void StarLightEngine::setNibbleInCache(i32 chunkX, i32 chunkY, i32 chunkZ, SWMRNibbleArray* nibble) {
    if (m_nibbleCache == nullptr) {
        return;
    }
    i32 index = chunkX + 5 * chunkZ + (5 * 5) * chunkY + m_chunkSectionIndexOffset;
    if (index >= 0 && index < m_sectionCacheSize) {
        m_nibbleCache[index] = nibble;
    }
}

void StarLightEngine::setNibblesForChunkInCache(i32 chunkX, i32 chunkZ, SWMRNibbleArray* const* nibbles) {
    for (i32 cy = m_minLightSection; cy <= m_maxLightSection; ++cy) {
        setNibbleInCache(chunkX, cy, chunkZ, nibbles == nullptr ? nullptr : nibbles[cy - m_minLightSection]);
    }
}

const bool* StarLightEngine::getEmptinessMap(i32 chunkX, i32 chunkZ) const {
    i32 dx = chunkX + m_chunkOffsetX;
    i32 dz = chunkZ + m_chunkOffsetZ;
    if (dx < 0 || dx >= 5 || dz < 0 || dz >= 5) {
        return nullptr;
    }
    return m_emptinessMapCache[static_cast<size_t>(dx + dz * 5)];
}

void StarLightEngine::setEmptinessMapCache(i32 chunkX, i32 chunkZ, const bool* map) {
    i32 dx = chunkX + m_chunkOffsetX;
    i32 dz = chunkZ + m_chunkOffsetZ;
    if (dx >= 0 && dx < 5 && dz >= 0 && dz < 5) {
        m_emptinessMapCache[static_cast<size_t>(dx + dz * 5)] = map;
    }
}

const BlockState* StarLightEngine::getBlockState(i32 worldX, i32 worldY, i32 worldZ) const {
    const ChunkSection* section = getChunkSection(worldX >> 4, worldY >> 4, worldZ >> 4);
    if (section == nullptr) {
        return nullptr;  // 空气
    }

    if (section->isEmpty()) {
        return nullptr;  // 空气
    }

    // 通过区块获取方块状态
    const IChunk* chunk = getChunkInCache(worldX >> 4, worldZ >> 4);
    if (chunk == nullptr) {
        return nullptr;
    }

    return chunk->getBlock(worldX & 15, worldY, worldZ & 15);
}

const BlockState* StarLightEngine::getBlockState(i32 sectionIndex, i32 localIndex) const {
    if (sectionIndex < 0 || sectionIndex >= m_sectionCacheSize || m_sectionCache == nullptr) {
        return nullptr;
    }

    const ChunkSection* section = static_cast<const ChunkSection*>(m_sectionCache[sectionIndex]);
    if (section == nullptr || section->isEmpty()) {
        return nullptr;
    }

    // localIndex = x | (z << 4) | (y << 8)
    i32 x = localIndex & 15;
    i32 z = (localIndex >> 4) & 15;
    i32 y = (localIndex >> 8) & 15;

    return section->getBlock(x, y, z);
}

i32 StarLightEngine::getLightLevel(i32 worldX, i32 worldY, i32 worldZ) const {
    SWMRNibbleArray* nibble = getNibbleFromCache(worldX >> 4, worldY >> 4, worldZ >> 4);
    if (nibble == nullptr) {
        return m_isSkyLight ? 15 : 0;  // 天空光照默认15，方块光照默认0
    }
    return nibble->getUpdating(worldX & 15, worldY & 15, worldZ & 15);
}

i32 StarLightEngine::getLightLevel(i32 sectionIndex, i32 localIndex) const {
    if (sectionIndex < 0 || sectionIndex >= m_sectionCacheSize || m_nibbleCache == nullptr) {
        return m_isSkyLight ? 15 : 0;
    }
    SWMRNibbleArray* nibble = m_nibbleCache[sectionIndex];
    if (nibble == nullptr) {
        return m_isSkyLight ? 15 : 0;
    }
    return nibble->getUpdating(localIndex);
}

void StarLightEngine::setLightLevel(i32 worldX, i32 worldY, i32 worldZ, i32 level) {
    i32 sectionIndex = (worldX >> 4) + 5 * (worldZ >> 4) + (5 * 5) * (worldY >> 4) + m_chunkSectionIndexOffset;
    SWMRNibbleArray* nibble = m_nibbleCache[sectionIndex];

    if (nibble != nullptr) {
        nibble->set(worldX & 15, worldY & 15, worldZ & 15, static_cast<u8>(level));

        // 客户端需要通知相邻区块段
        if (m_notifyUpdateCache != nullptr && m_isClientSide) {
            i32 cx1 = (worldX - 1) >> 4;
            i32 cx2 = (worldX + 1) >> 4;
            i32 cy1 = (worldY - 1) >> 4;
            i32 cy2 = (worldY + 1) >> 4;
            i32 cz1 = (worldZ - 1) >> 4;
            i32 cz2 = (worldZ + 1) >> 4;
            for (i32 x = cx1; x <= cx2; ++x) {
                for (i32 y = cy1; y <= cy2; ++y) {
                    for (i32 z = cz1; z <= cz2; ++z) {
                        i32 idx = x + 5 * z + (5 * 5) * y + m_chunkSectionIndexOffset;
                        if (idx >= 0 && idx < m_sectionCacheSize) {
                            m_notifyUpdateCache[idx] = true;
                        }
                    }
                }
            }
        }
    }
}

void StarLightEngine::setLightLevel(i32 sectionIndex, i32 localIndex, i32 worldX, i32 worldY, i32 worldZ, i32 level) {
    if (sectionIndex < 0 || sectionIndex >= m_sectionCacheSize || m_nibbleCache == nullptr) {
        return;
    }
    SWMRNibbleArray* nibble = m_nibbleCache[sectionIndex];
    if (nibble != nullptr) {
        nibble->set(localIndex, static_cast<u8>(level));

        if (m_notifyUpdateCache != nullptr && m_isClientSide) {
            i32 cx1 = (worldX - 1) >> 4;
            i32 cx2 = (worldX + 1) >> 4;
            i32 cy1 = (worldY - 1) >> 4;
            i32 cy2 = (worldY + 1) >> 4;
            i32 cz1 = (worldZ - 1) >> 4;
            i32 cz2 = (worldZ + 1) >> 4;
            for (i32 x = cx1; x <= cx2; ++x) {
                for (i32 y = cy1; y <= cy2; ++y) {
                    for (i32 z = cz1; z <= cz2; ++z) {
                        i32 idx = x + 5 * z + (5 * 5) * y + m_chunkSectionIndexOffset;
                        if (idx >= 0 && idx < m_sectionCacheSize) {
                            m_notifyUpdateCache[idx] = true;
                        }
                    }
                }
            }
        }
    }
}

void StarLightEngine::postLightUpdate(i32 worldX, i32 worldY, i32 worldZ) {
    if (m_notifyUpdateCache == nullptr || !m_isClientSide) {
        return;
    }

    i32 cx1 = (worldX - 1) >> 4;
    i32 cx2 = (worldX + 1) >> 4;
    i32 cy1 = (worldY - 1) >> 4;
    i32 cy2 = (worldY + 1) >> 4;
    i32 cz1 = (worldZ - 1) >> 4;
    i32 cz2 = (worldZ + 1) >> 4;

    for (i32 x = cx1; x <= cx2; ++x) {
        for (i32 y = cy1; y <= cy2; ++y) {
            for (i32 z = cz1; z <= cz2; ++z) {
                i32 idx = x + 5 * z + (5 * 5) * y + m_chunkSectionIndexOffset;
                if (idx >= 0 && idx < m_sectionCacheSize) {
                    m_notifyUpdateCache[idx] = true;
                }
            }
        }
    }
}

// ============================================================================
// 队列操作
// ============================================================================

void StarLightEngine::appendToIncreaseQueue(u64 queueValue) {
    if (m_increaseQueueInitialLength >= static_cast<i32>(m_increaseQueue.size())) {
        resizeIncreaseQueue();
    }
    m_increaseQueue[static_cast<size_t>(m_increaseQueueInitialLength++)] = queueValue;
    m_needsUpdate = true;
}

void StarLightEngine::appendToDecreaseQueue(u64 queueValue) {
    if (m_decreaseQueueInitialLength >= static_cast<i32>(m_decreaseQueue.size())) {
        resizeDecreaseQueue();
    }
    m_decreaseQueue[static_cast<size_t>(m_decreaseQueueInitialLength++)] = queueValue;
    m_needsUpdate = true;
}

void StarLightEngine::resizeIncreaseQueue() {
    size_t newSize = std::max(static_cast<size_t>(4), m_increaseQueue.size() + (m_increaseQueue.size() >> 1));
    m_increaseQueue.resize(newSize);
}

void StarLightEngine::resizeDecreaseQueue() {
    size_t newSize = std::max(static_cast<size_t>(4), m_decreaseQueue.size() + (m_decreaseQueue.size() >> 1));
    m_decreaseQueue.resize(newSize);
}

i32 StarLightEngine::performUpdates(StarLightLightingProvider* lightAccess, i32 maxUpdates) {
    // 先处理减亮队列
    if (m_decreaseQueueInitialLength > 0) {
        performLightDecrease(lightAccess);
    }

    // 再处理增亮队列
    if (m_increaseQueueInitialLength > 0) {
        performLightIncrease(lightAccess);
    }

    m_needsUpdate = false;
    return maxUpdates;
}

// ============================================================================
// 光照传播（基础实现，子类可扩展）
// ============================================================================

void StarLightEngine::performLightIncrease(StarLightLightingProvider* lightAccess) {
    i32 decodeOffsetX = -m_encodeOffsetX;
    i32 decodeOffsetY = -m_encodeOffsetY;
    i32 decodeOffsetZ = -m_encodeOffsetZ;
    i32 encodeOffset = m_coordinateOffset;
    i32 sectionOffset = m_chunkSectionIndexOffset;

    // 防止无限循环的安全限制
    constexpr i32 MAX_ITERATIONS = 100000;
    i32 iterations = 0;

    // 持续处理直到队列为空
    while (m_increaseQueueInitialLength > 0) {
        if (++iterations > MAX_ITERATIONS) {
            m_increaseQueueInitialLength = 0;
            break;
        }

        i32 queueLength = m_increaseQueueInitialLength;
        m_increaseQueueInitialLength = 0;

        for (i32 readIndex = 0; readIndex < queueLength; ++readIndex) {
            u64 queueValue = m_increaseQueue[static_cast<size_t>(readIndex)];

            // 解码队列条目
            i32 posX = (static_cast<i32>(queueValue) & 63) + decodeOffsetX;
            i32 posZ = ((static_cast<i32>(queueValue) >> 6) & 63) + decodeOffsetZ;
            i32 posY = ((static_cast<i32>(queueValue) >> 12) & 0xFFFF) + decodeOffsetY;
            i32 propagatedLevel = static_cast<i32>((queueValue >> 28) & 0xF);
            i32 directionBits = static_cast<i32>((queueValue >> 32) & 0x3F);

            // 检查重检标志
            if ((queueValue & FLAG_RECHECK_LEVEL) != 0) {
                if (getLightLevel(posX, posY, posZ) != propagatedLevel) {
                    continue;
                }
            }

            // 写入等级标志
            if ((queueValue & FLAG_WRITE_LEVEL) != 0) {
                setLightLevel(posX, posY, posZ, propagatedLevel);
            }

            // 获取源方块状态（用于条件透明检查）
            const BlockState* fromState = getBlockState(posX, posY, posZ);

            // 遍历方向传播
            const std::vector<LightAxisDirection>& directions = s_oldCheckDirections[static_cast<size_t>(directionBits)];
            for (LightAxisDirection dir : directions) {
                i32 dx, dy, dz;
                getDirectionOffset(dir, dx, dy, dz);

                i32 offX = posX + dx;
                i32 offY = posY + dy;
                i32 offZ = posZ + dz;

                i32 sectionIndex = (offX >> 4) + 5 * (offZ >> 4) + (5 * 5) * (offY >> 4) + sectionOffset;
                i32 localIndex = (offX & 15) | ((offZ & 15) << 4) | ((offY & 15) << 8);

                // 边界检查：确保 sectionIndex 在有效范围内
                if (sectionIndex < 0 || sectionIndex >= m_sectionCacheSize) {
                    continue;
                }

                SWMRNibbleArray* currentNibble = m_nibbleCache[sectionIndex];
                i32 currentLevel;
                if (currentNibble == nullptr || (currentLevel = currentNibble->getUpdating(localIndex)) >= (propagatedLevel - 1)) {
                    continue;
                }

                // 获取目标方块状态
                const BlockState* toState = getBlockState(offX, offY, offZ);

                // 条件透明检查：检查面是否被遮挡
                if (isFaceOccluded(fromState, toState, dir)) {
                    continue;
                }

                // 获取目标方块的透明度
                i32 opacity = 1;
                if (toState != nullptr) {
                    opacity = std::max(1, toState->getBlock().getOpacity(*toState));
                }

                // 计算传播后的光照等级
                i32 targetLevel = propagatedLevel - opacity;
                if (targetLevel <= currentLevel) {
                    continue;
                }

                // 检查目标方块是否需要形状遮挡（用于后续传播）
                u64 flags = 0;
                if (toState != nullptr && toState->useShapeForLightOcclusion()) {
                    flags |= FLAG_HAS_SIDED_TRANSPARENT_BLOCKS;
                }

                currentNibble->set(localIndex, static_cast<u8>(targetLevel));
                postLightUpdate(offX, offY, offZ);

                if (targetLevel > 1) {
                    appendToIncreaseQueue(
                        ((offX + (offZ << 6) + (offY << 12) + encodeOffset) & 0xFFFFFFFF) |
                        (static_cast<u64>(targetLevel & 0xF) << 28) |
                        (static_cast<u64>(getEverythingButOppositeDirection(dir)) << 32) |
                        flags
                    );
                }
            }
        }
    }
}

void StarLightEngine::performLightDecrease(StarLightLightingProvider* lightAccess) {
    i32 decodeOffsetX = -m_encodeOffsetX;
    i32 decodeOffsetY = -m_encodeOffsetY;
    i32 decodeOffsetZ = -m_encodeOffsetZ;
    i32 encodeOffset = m_coordinateOffset;
    i32 sectionOffset = m_chunkSectionIndexOffset;
    i32 emittedMask = m_emittedLightMask;

    // 防止无限循环的安全限制
    constexpr i32 MAX_ITERATIONS = 100000;
    i32 iterations = 0;

    // 持续处理直到队列为空
    while (m_decreaseQueueInitialLength > 0) {
        if (++iterations > MAX_ITERATIONS) {
            spdlog::warn("performLightDecrease hit iteration limit: iterations={}", iterations);
            m_decreaseQueueInitialLength = 0;
            break;
        }

        i32 queueLength = m_decreaseQueueInitialLength;
        m_decreaseQueueInitialLength = 0;

        for (i32 readIndex = 0; readIndex < queueLength; ++readIndex) {
            u64 queueValue = m_decreaseQueue[static_cast<size_t>(readIndex)];

            i32 posX = (static_cast<i32>(queueValue) & 63) + decodeOffsetX;
            i32 posZ = ((static_cast<i32>(queueValue) >> 6) & 63) + decodeOffsetZ;
            i32 posY = ((static_cast<i32>(queueValue) >> 12) & 0xFFFF) + decodeOffsetY;
            i32 propagatedLevel = static_cast<i32>((queueValue >> 28) & 0xF);
            i32 directionBits = static_cast<i32>((queueValue >> 32) & 0x3F);

            // 获取源方块状态（用于条件透明检查）
            const BlockState* fromState = getBlockState(posX, posY, posZ);

            const std::vector<LightAxisDirection>& directions = s_oldCheckDirections[static_cast<size_t>(directionBits)];
            for (LightAxisDirection dir : directions) {
                i32 dx, dy, dz;
                getDirectionOffset(dir, dx, dy, dz);

                i32 offX = posX + dx;
                i32 offY = posY + dy;
                i32 offZ = posZ + dz;

                i32 sectionIndex = (offX >> 4) + 5 * (offZ >> 4) + (5 * 5) * (offY >> 4) + sectionOffset;
                i32 localIndex = (offX & 15) | ((offZ & 15) << 4) | ((offY & 15) << 8);

                // 边界检查：确保 sectionIndex 在有效范围内
                if (sectionIndex < 0 || sectionIndex >= m_sectionCacheSize) {
                    continue;
                }

                SWMRNibbleArray* currentNibble = m_nibbleCache[sectionIndex];
                i32 lightLevel;

                if (currentNibble == nullptr || (lightLevel = currentNibble->getUpdating(localIndex)) == 0) {
                    continue;
                }

                // 获取目标方块状态
                const BlockState* toState = getBlockState(offX, offY, offZ);

                // 条件透明检查（减亮传播时也需要检查，因为如果面被遮挡，光线本来就不会传播过去）
                if (isFaceOccluded(fromState, toState, dir)) {
                    continue;
                }

                // 获取目标方块的透明度
                i32 opacity = 1;
                if (toState != nullptr) {
                    opacity = std::max(1, toState->getBlock().getOpacity(*toState));
                }

                i32 targetLevel = std::max(0, propagatedLevel - opacity);

                if (lightLevel > targetLevel) {
                    // 检查是否有光源发射
                    i32 emittedLevel = 0;
                    if (toState != nullptr) {
                        emittedLevel = toState->getBlock().getLightLevel(*toState) & emittedMask;
                    }

                    // 如果有光源，需要重新传播
                    if (emittedLevel > 0) {
                        appendToIncreaseQueue(
                            ((offX + (offZ << 6) + (offY << 12) + encodeOffset) & 0xFFFFFFFF) |
                            (static_cast<u64>(emittedLevel & 0xF) << 28) |
                            (static_cast<u64>(ALL_DIRECTIONS_BITSET) << 32) |
                            FLAG_RECHECK_LEVEL
                        );
                    } else {
                        // 重新计算该位置的光照值
                        appendToIncreaseQueue(
                            ((offX + (offZ << 6) + (offY << 12) + encodeOffset) & 0xFFFFFFFF) |
                            (static_cast<u64>(lightLevel & 0xF) << 28) |
                            (static_cast<u64>(ALL_DIRECTIONS_BITSET) << 32) |
                            FLAG_RECHECK_LEVEL
                        );
                    }
                    continue;
                }

                currentNibble->set(localIndex, 0);
                postLightUpdate(offX, offY, offZ);

                if (targetLevel > 0) {
                    appendToDecreaseQueue(
                        ((offX + (offZ << 6) + (offY << 12) + encodeOffset) & 0xFFFFFFFF) |
                        (static_cast<u64>(targetLevel & 0xF) << 28) |
                        (static_cast<u64>(getEverythingButOppositeDirection(dir)) << 32)
                    );
                }
            }
        }
    }

    // 处理增亮队列中的恢复光源
    performLightIncrease(lightAccess);
}

// ============================================================================
// 公共接口实现
// ============================================================================

i32 StarLightEngine::tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight) {
    // 参数由子类决定是否使用，基类忽略
    (void)updateSkyLight;
    (void)updateBlockLight;
    return maxUpdates;  // 子类重写
}

void StarLightEngine::updateSectionStatus(const SectionPos& pos, bool isEmpty) {
    // 子类实现
    (void)pos;
    (void)isEmpty;
}

// ============================================================================
// 条件透明检查
// ============================================================================

namespace {

/**
 * @brief 将 CollisionShape 转换为 VoxelShape
 *
 * 用于面遮挡检测。对于完整方块和空形状有优化路径。
 */
VoxelShape collisionShapeToVoxelShape(const CollisionShape& shape) {
    if (shape.isEmpty()) {
        return Shapes::empty();
    }
    if (shape.isFullBlock()) {
        return Shapes::block();
    }
    // 对于简单盒，创建对应的 VoxelShape
    const auto& boxes = shape.boxes();
    if (boxes.empty()) {
        return Shapes::empty();
    }
    // 使用第一个碰撞盒创建 VoxelShape
    // 注意：对于复杂形状（多个盒），这只是一个近似
    const auto& box = boxes[0];
    return Shapes::box(box.minX, box.minY, box.minZ, box.maxX, box.maxY, box.maxZ);
}

} // anonymous namespace

bool StarLightEngine::isFaceOccluded(const BlockState* fromState,
                                      const BlockState* toState,
                                      LightAxisDirection direction) {
    // 空气不遮挡任何面
    if (fromState == nullptr || toState == nullptr) {
        return false;
    }

    // 检查任一方块是否需要形状遮挡检测
    bool fromUseShape = useShapeForLightOcclusion(fromState);
    bool toUseShape = useShapeForLightOcclusion(toState);

    // 如果都不需要形状检测，使用传统的不透明度检查
    if (!fromUseShape && !toUseShape) {
        // 完整不透明方块会遮挡
        // 注意：对于普通方块，不透明度检查在调用方已经完成
        return false;
    }

    // 获取面遮挡方向
    Direction fromDir = getNMSDirection(direction);
    Direction toDir = getNMSDirection(getOppositeDirection(direction));

    // 获取遮挡形状
    const CollisionShape& fromCollisionShape = fromState->getOcclusionShape();
    const CollisionShape& toCollisionShape = toState->getOcclusionShape();

    // 转换为 VoxelShape 进行面遮挡检测
    VoxelShape fromShape = collisionShapeToVoxelShape(fromCollisionShape);
    VoxelShape toShape = collisionShapeToVoxelShape(toCollisionShape);

    // 使用 Shapes 类进行面遮挡检测
    return Shapes::blockOccludes(fromShape, toShape, fromDir);
}

bool StarLightEngine::useShapeForLightOcclusion(const BlockState* state) {
    if (state == nullptr) {
        return false;
    }
    return state->useShapeForLightOcclusion();
}

// ============================================================================
// 静态工具方法
// ============================================================================

std::vector<SWMRNibbleArray*> StarLightEngine::getFilledEmptyLight(i32 totalLightSections) {
    std::vector<SWMRNibbleArray*> ret;
    ret.reserve(static_cast<size_t>(totalLightSections));

    for (i32 i = 0; i < totalLightSections; ++i) {
        ret.push_back(new SWMRNibbleArray(nullptr, true));  // Null 状态
    }

    return ret;
}

} // namespace mc
