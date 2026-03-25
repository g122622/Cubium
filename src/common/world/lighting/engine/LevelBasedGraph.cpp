#include "LevelBasedGraph.hpp"
#include "../IChunkLightProvider.hpp"
#include <algorithm>

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

LevelBasedGraph::LevelBasedGraph(i32 levelCount, i32 expectedUpdates, IChunkLightProvider* provider)
    : m_levelCount(levelCount)
    , m_increaseQueue(static_cast<size_t>(expectedUpdates))
    , m_decreaseQueue(static_cast<size_t>(expectedUpdates))
    , m_chunkProvider(provider)
    , m_needsUpdate(false) {
    m_cache.setProvider(provider);
}

// ============================================================================
// 缓存管理
// ============================================================================

void LevelBasedGraph::enableCache(i32 centerX, i32 centerY, i32 centerZ,
                                   bool relaxed, bool loadTwoRadius) {
    m_cacheEnabled = true;
    m_cache.setupCaches(centerX, centerY, centerZ, relaxed, loadTwoRadius);
}

void LevelBasedGraph::disableCache() {
    m_cacheEnabled = false;
    m_cache.destroyCaches();
}

f32 LevelBasedGraph::getCacheHitRate() const {
    return m_cache.getCacheHitRate();
}

const IChunk* LevelBasedGraph::getCachedChunk(i32 chunkX, i32 chunkZ) const {
    if (m_cacheEnabled) {
        return m_cache.getChunk(chunkX, chunkZ);
    }
    if (m_chunkProvider != nullptr) {
        return m_chunkProvider->getChunkForLight(chunkX, chunkZ);
    }
    return nullptr;
}

bool LevelBasedGraph::isCachedSectionEmpty(i32 sectionX, i32 sectionY, i32 sectionZ) const {
    if (m_cacheEnabled) {
        return m_cache.isSectionEmpty(sectionX, sectionY, sectionZ);
    }
    return false;
}

bool LevelBasedGraph::isSectionEmpty(i64 sectionPos) const {
    // 默认实现：使用缓存检查
    SectionPos pos = SectionPos::fromLong(sectionPos);
    return isCachedSectionEmpty(pos.x, pos.y, pos.z);
}

// ============================================================================
// 公共接口
// ============================================================================

void LevelBasedGraph::scheduleUpdate(i64 pos) {
    // 解码世界坐标
    i32 x, y, z;
    unpackWorldPos(pos, x, y, z);

    // 编码为队列格式，所有方向
    u64 entry = encodeQueueEntry(x, y, z, 15, DIR_ALL, FLAG_RECHECK_LEVEL);
    appendToIncreaseQueue(entry);
}

void LevelBasedGraph::scheduleUpdate(i64 fromPos, i64 toPos, i32 level, bool isIncrease) {
    // 解码目标坐标
    i32 toX, toY, toZ;
    unpackWorldPos(toPos, toX, toY, toZ);

    // 计算方向
    i32 fromX, fromY, fromZ;
    unpackWorldPos(fromPos, fromX, fromY, fromZ);

    DirectionBit fromDir = DIR_ALL;
    if (fromPos != LightEngineUtils::ROOT_POS) {
        i32 dx = (toX > fromX) ? 1 : ((toX < fromX) ? -1 : 0);
        i32 dy = (toY > fromY) ? 1 : ((toY < fromY) ? -1 : 0);
        i32 dz = (toZ > fromZ) ? 1 : ((toZ < fromZ) ? -1 : 0);

        fromDir = static_cast<DirectionBit>(
            (dx != 0 ? (dx > 0 ? DIR_WEST : DIR_EAST) : 0) |
            (dy != 0 ? (dy > 0 ? DIR_DOWN : DIR_UP) : 0) |
            (dz != 0 ? (dz > 0 ? DIR_NORTH : DIR_SOUTH) : 0)
        );
    }

    if (isIncrease) {
        appendToIncreaseQueue(encodeQueueEntry(toX, toY, toZ,
            static_cast<u8>(level), static_cast<u8>(fromDir), FLAG_RECHECK_LEVEL));
    } else {
        appendToDecreaseQueue(encodeQueueEntry(toX, toY, toZ,
            static_cast<u8>(level), static_cast<u8>(fromDir), 0));
    }
}

void LevelBasedGraph::cancelUpdate(i64 pos) {
    // 从队列中移除（线性搜索，但通常队列不大）
    i32 x, y, z;
    unpackWorldPos(pos, x, y, z);
    i64 coord = static_cast<i64>((x & 0x3F) | ((z & 0x3F) << 6) | ((y & 0xFFF) << 12));
    u64 targetCoord = static_cast<u64>((coord + m_coordinateOffset) & COORD_MASK);

    // 从增亮队列移除
    for (i32 i = 0; i < m_increaseQueueInitialLength; ++i) {
        if ((m_increaseQueue[i] & COORD_MASK) == targetCoord) {
            // 交换到末尾并减少长度
            m_increaseQueue[i] = m_increaseQueue[--m_increaseQueueInitialLength];
            break;
        }
    }

    // 从减亮队列移除
    for (i32 i = 0; i < m_decreaseQueueInitialLength; ++i) {
        if ((m_decreaseQueue[i] & COORD_MASK) == targetCoord) {
            m_decreaseQueue[i] = m_decreaseQueue[--m_decreaseQueueInitialLength];
            break;
        }
    }

    m_needsUpdate = m_increaseQueueInitialLength > 0 || m_decreaseQueueInitialLength > 0;
}

void LevelBasedGraph::cancelUpdates(const std::function<bool(i64)>& predicate) {
    // 从增亮队列移除
    i32 writeIdx = 0;
    for (i32 i = 0; i < m_increaseQueueInitialLength; ++i) {
        u64 entry = m_increaseQueue[i];
        i32 x, y, z;
        decodeQueueEntry(entry, x, y, z);
        i64 worldPos = packWorldPos(x, y, z);
        if (!predicate(worldPos)) {
            m_increaseQueue[writeIdx++] = entry;
        }
    }
    m_increaseQueueInitialLength = writeIdx;

    // 从减亮队列移除
    writeIdx = 0;
    for (i32 i = 0; i < m_decreaseQueueInitialLength; ++i) {
        u64 entry = m_decreaseQueue[i];
        i32 x, y, z;
        decodeQueueEntry(entry, x, y, z);
        i64 worldPos = packWorldPos(x, y, z);
        if (!predicate(worldPos)) {
            m_decreaseQueue[writeIdx++] = entry;
        }
    }
    m_decreaseQueueInitialLength = writeIdx;

    m_needsUpdate = m_increaseQueueInitialLength > 0 || m_decreaseQueueInitialLength > 0;
}

i32 LevelBasedGraph::processUpdates(i32 maxUpdates) {
    // 先处理减亮队列（减少光照）
    if (m_decreaseQueueInitialLength > 0) {
        maxUpdates = processDecreaseQueue(maxUpdates);
        if (maxUpdates <= 0) {
            return 0;
        }
    }

    // 再处理增亮队列（增加光照）
    if (m_increaseQueueInitialLength > 0) {
        maxUpdates = processIncreaseQueue(maxUpdates);
    }

    m_needsUpdate = m_increaseQueueInitialLength > 0 || m_decreaseQueueInitialLength > 0;
    return maxUpdates;
}

// ============================================================================
// 队列处理
// ============================================================================

i32 LevelBasedGraph::processIncreaseQueue(i32 maxUpdates) {
    while (m_increaseQueueInitialLength > 0 && maxUpdates > 0) {
        --maxUpdates;

        // 取出队首元素
        u64 entry = m_increaseQueue[--m_increaseQueueInitialLength];

        // 解码
        i32 x, y, z;
        decodeQueueEntry(entry, x, y, z);
        u8 propagatedLevel = decodeLevel(entry);
        u8 directionBits = decodeDirections(entry);
        u64 flags = decodeFlags(entry);

        i64 worldPos = packWorldPos(x, y, z);

        // 重新检查标志
        if (flags & FLAG_RECHECK_LEVEL) {
            i32 currentLevel = getLevel(worldPos);
            if (currentLevel != propagatedLevel) {
                continue;
            }
        }

        // 写入等级标志
        if (flags & FLAG_WRITE_LEVEL) {
            setLevel(worldPos, propagatedLevel);
        }

        // 获取检查方向
        DirectionBit checkDirs = static_cast<DirectionBit>(directionBits);

        // 如果没有特殊标志，检查所有方向
        if ((flags & FLAG_HAS_SIDED_TRANSPARENT) == 0) {
            // 快速路径：无需检查方块透明性
            DirectionBit dirs = DirectionBits::opposite(checkDirs);

            // 向下
            if ((dirs & DIR_DOWN) && y > 0) {
                i64 neighborPos = packWorldPos(x, y - 1, z);
                // 空区块段优化：跳过空区块段
                if (!isSectionEmpty(LightEngineUtils::worldToSectionPos(neighborPos))) {
                    i32 newLevel = getEdgeLevel(worldPos, neighborPos, propagatedLevel);
                    if (newLevel < 15) {
                        setLevel(neighborPos, newLevel);
                        if (newLevel > 0) {
                            appendToIncreaseQueue(encodeQueueEntry(x, y - 1, z,
                                static_cast<u8>(newLevel), DIR_UP, FLAG_RECHECK_LEVEL));
                        }
                    }
                }
            }

            // 向上
            if ((dirs & DIR_UP)) {
                i64 neighborPos = packWorldPos(x, y + 1, z);
                // 空区块段优化：跳过空区块段
                if (!isSectionEmpty(LightEngineUtils::worldToSectionPos(neighborPos))) {
                    i32 newLevel = getEdgeLevel(worldPos, neighborPos, propagatedLevel);
                    if (newLevel < 15) {
                        setLevel(neighborPos, newLevel);
                        if (newLevel > 0) {
                            appendToIncreaseQueue(encodeQueueEntry(x, y + 1, z,
                                static_cast<u8>(newLevel), DIR_DOWN, FLAG_RECHECK_LEVEL));
                        }
                    }
                }
            }

            // 北 (Z-)
            if ((dirs & DIR_NORTH)) {
                i64 neighborPos = packWorldPos(x, y, z - 1);
                // 空区块段优化：跳过空区块段
                if (!isSectionEmpty(LightEngineUtils::worldToSectionPos(neighborPos))) {
                    i32 newLevel = getEdgeLevel(worldPos, neighborPos, propagatedLevel);
                    if (newLevel < 15) {
                        setLevel(neighborPos, newLevel);
                        if (newLevel > 0) {
                            appendToIncreaseQueue(encodeQueueEntry(x, y, z - 1,
                                static_cast<u8>(newLevel), DIR_SOUTH, FLAG_RECHECK_LEVEL));
                        }
                    }
                }
            }

            // 南 (Z+)
            if ((dirs & DIR_SOUTH)) {
                i64 neighborPos = packWorldPos(x, y, z + 1);
                // 空区块段优化：跳过空区块段
                if (!isSectionEmpty(LightEngineUtils::worldToSectionPos(neighborPos))) {
                    i32 newLevel = getEdgeLevel(worldPos, neighborPos, propagatedLevel);
                    if (newLevel < 15) {
                        setLevel(neighborPos, newLevel);
                        if (newLevel > 0) {
                            appendToIncreaseQueue(encodeQueueEntry(x, y, z + 1,
                                static_cast<u8>(newLevel), DIR_NORTH, FLAG_RECHECK_LEVEL));
                        }
                    }
                }
            }

            // 西 (X-)
            if ((dirs & DIR_WEST)) {
                i64 neighborPos = packWorldPos(x - 1, y, z);
                // 空区块段优化：跳过空区块段
                if (!isSectionEmpty(LightEngineUtils::worldToSectionPos(neighborPos))) {
                    i32 newLevel = getEdgeLevel(worldPos, neighborPos, propagatedLevel);
                    if (newLevel < 15) {
                        setLevel(neighborPos, newLevel);
                        if (newLevel > 0) {
                            appendToIncreaseQueue(encodeQueueEntry(x - 1, y, z,
                                static_cast<u8>(newLevel), DIR_EAST, FLAG_RECHECK_LEVEL));
                        }
                    }
                }
            }

            // 东 (X+)
            if ((dirs & DIR_EAST)) {
                i64 neighborPos = packWorldPos(x + 1, y, z);
                // 空区块段优化：跳过空区块段
                if (!isSectionEmpty(LightEngineUtils::worldToSectionPos(neighborPos))) {
                    i32 newLevel = getEdgeLevel(worldPos, neighborPos, propagatedLevel);
                    if (newLevel < 15) {
                        setLevel(neighborPos, newLevel);
                        if (newLevel > 0) {
                            appendToIncreaseQueue(encodeQueueEntry(x + 1, y, z,
                                static_cast<u8>(newLevel), DIR_WEST, FLAG_RECHECK_LEVEL));
                        }
                    }
                }
            }
        } else {
            // 慢速路径：需要检查方块透明性
            notifyNeighbors(worldPos, propagatedLevel, false);
        }
    }

    return maxUpdates;
}

i32 LevelBasedGraph::processDecreaseQueue(i32 maxUpdates) {
    while (m_decreaseQueueInitialLength > 0 && maxUpdates > 0) {
        --maxUpdates;

        // 取出队首元素
        u64 entry = m_decreaseQueue[--m_decreaseQueueInitialLength];

        // 解码
        i32 x, y, z;
        decodeQueueEntry(entry, x, y, z);
        u8 propagatedLevel = decodeLevel(entry);
        u8 directionBits = decodeDirections(entry);
        u64 flags = decodeFlags(entry);

        i64 worldPos = packWorldPos(x, y, z);

        // 获取当前等级
        i32 currentLevel = getLevel(worldPos);

        // 重新计算等级
        i32 newLevel = computeLevel(worldPos, LightEngineUtils::ROOT_POS, currentLevel);

        if (newLevel < currentLevel) {
            // 光照减少
            setLevel(worldPos, newLevel);

            if (newLevel > 0) {
                // 继续传播减少
                appendToIncreaseQueue(encodeQueueEntry(x, y, z,
                    static_cast<u8>(newLevel), DIR_ALL, FLAG_WRITE_LEVEL));
            }

            // 通知相邻方块
            notifyNeighbors(worldPos, currentLevel, false);
        } else if (newLevel > currentLevel) {
            // 光照增加（不应该在减亮队列中发生，但作为安全检查）
            if (newLevel > propagatedLevel) {
                appendToIncreaseQueue(encodeQueueEntry(x, y, z,
                    static_cast<u8>(newLevel), DIR_ALL, FLAG_RECHECK_LEVEL));
            }
        }
    }

    return maxUpdates;
}

} // namespace mc
