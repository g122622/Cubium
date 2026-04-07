#include "LevelBasedGraph.hpp"
#include "../IChunkLightProvider.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc {

namespace {

DirectionBit computeCheckDirections(i64 fromPos, i64 toPos) {
    if (fromPos == LightEngineUtils::ROOT_POS) {
        return DIR_ALL;
    }

    i32 fromX, fromY, fromZ;
    i32 toX, toY, toZ;
    LightEngineUtils::unpackPos(fromPos, fromX, fromY, fromZ);
    LightEngineUtils::unpackPos(toPos, toX, toY, toZ);

    i32 dx = (toX > fromX) ? 1 : ((toX < fromX) ? -1 : 0);
    i32 dy = (toY > fromY) ? 1 : ((toY < fromY) ? -1 : 0);
    i32 dz = (toZ > fromZ) ? 1 : ((toZ < fromZ) ? -1 : 0);

    Direction moveDir = Directions::fromDelta(dx, dy, dz);
    if (moveDir == Direction::None) {
        return DIR_ALL;
    }

    Direction blockedDirection = Directions::opposite(moveDir);
    return DirectionBits::allExcept(DirectionBits::fromDirection(blockedDirection));
}

} // namespace

// ============================================================================
// 构造函数
// ============================================================================

StarLightEngine::StarLightEngine(i32 levelCount, i32 expectedUpdates, StarLightLightingProvider* provider)
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

void StarLightEngine::enableCache(i32 centerX, i32 centerY, i32 centerZ,
                                   bool relaxed, bool loadTwoRadius) {
    m_cacheEnabled = true;
    m_cache.setupCaches(centerX, centerY, centerZ, relaxed, loadTwoRadius);
}

void StarLightEngine::disableCache() {
    m_cacheEnabled = false;
    m_cache.destroyCaches();
}

f32 StarLightEngine::getCacheHitRate() const {
    return m_cache.getCacheHitRate();
}

const IChunk* StarLightEngine::getCachedChunk(i32 chunkX, i32 chunkZ) const {
    if (m_cacheEnabled) {
        return m_cache.getChunk(chunkX, chunkZ);
    }
    if (m_chunkProvider != nullptr) {
        return m_chunkProvider->getChunkForLight(chunkX, chunkZ);
    }
    return nullptr;
}

bool StarLightEngine::isCachedSectionEmpty(i32 sectionX, i32 sectionY, i32 sectionZ) const {
    if (m_cacheEnabled) {
        return m_cache.isSectionEmpty(sectionX, sectionY, sectionZ);
    }
    return false;
}

bool StarLightEngine::isSectionEmpty(i64 sectionPos) const {
    // 默认实现：使用缓存检查
    SectionPos pos = SectionPos::fromLong(sectionPos);
    return isCachedSectionEmpty(pos.x, pos.y, pos.z);
}

// ============================================================================
// 公共接口
// ============================================================================

void StarLightEngine::scheduleUpdate(i64 pos) {
    // 对齐 Starlight 的 checkBlock 语义：
    // 1) 如果当前位置是满亮源（level=0），先安排一次增亮重传播
    // 2) 否则先将该点清为最暗
    // 3) 再统一走减亮传播链
    i32 currentLevel = getLevel(pos);

    i32 x, y, z;
    unpackWorldPos(pos, x, y, z);

    if (currentLevel == 0) {
        appendToIncreaseQueue(encodeQueueEntry(x, y, z,
            static_cast<u8>(currentLevel), DIR_ALL,
            FLAG_HAS_SIDED_TRANSPARENT | FLAG_RECHECK_LEVEL));
    } else {
        setLevel(pos, m_levelCount - 1);
    }

    appendToDecreaseQueue(encodeQueueEntry(x, y, z,
        static_cast<u8>(currentLevel), DIR_ALL, 0));
}

void StarLightEngine::scheduleUpdate(i64 fromPos, i64 toPos, i32 level, bool isIncrease) {
    // 解码目标坐标
    i32 toX, toY, toZ;
    unpackWorldPos(toPos, toX, toY, toZ);

    DirectionBit checkDirs = computeCheckDirections(fromPos, toPos);

    if (isIncrease) {
        u64 flags = (fromPos == LightEngineUtils::ROOT_POS)
            ? FLAG_WRITE_LEVEL
            : FLAG_RECHECK_LEVEL;

        appendToIncreaseQueue(encodeQueueEntry(toX, toY, toZ,
            static_cast<u8>(level), static_cast<u8>(checkDirs), flags));
    } else {
        appendToDecreaseQueue(encodeQueueEntry(toX, toY, toZ,
            static_cast<u8>(level), static_cast<u8>(checkDirs), 0));
    }
}

void StarLightEngine::cancelUpdate(i64 pos) {
    i32 writeIdx = 0;
    for (i32 i = 0; i < m_increaseQueueInitialLength; ++i) {
        if (m_increaseQueue[i].pos != pos) {
            m_increaseQueue[writeIdx++] = m_increaseQueue[i];
        }
    }
    m_increaseQueueInitialLength = writeIdx;

    writeIdx = 0;
    for (i32 i = 0; i < m_decreaseQueueInitialLength; ++i) {
        if (m_decreaseQueue[i].pos != pos) {
            m_decreaseQueue[writeIdx++] = m_decreaseQueue[i];
        }
    }
    m_decreaseQueueInitialLength = writeIdx;

    m_needsUpdate = m_increaseQueueInitialLength > 0 || m_decreaseQueueInitialLength > 0;
}

void StarLightEngine::cancelUpdates(const std::function<bool(i64)>& predicate) {
    // 从增亮队列移除
    i32 writeIdx = 0;
    for (i32 i = 0; i < m_increaseQueueInitialLength; ++i) {
        const QueueEntry& entry = m_increaseQueue[i];
        i64 worldPos = entry.pos;
        if (!predicate(worldPos)) {
            m_increaseQueue[writeIdx++] = entry;
        }
    }
    m_increaseQueueInitialLength = writeIdx;

    // 从减亮队列移除
    writeIdx = 0;
    for (i32 i = 0; i < m_decreaseQueueInitialLength; ++i) {
        const QueueEntry& entry = m_decreaseQueue[i];
        i64 worldPos = entry.pos;
        if (!predicate(worldPos)) {
            m_decreaseQueue[writeIdx++] = entry;
        }
    }
    m_decreaseQueueInitialLength = writeIdx;

    m_needsUpdate = m_increaseQueueInitialLength > 0 || m_decreaseQueueInitialLength > 0;
}

i32 StarLightEngine::processUpdates(i32 maxUpdates) {
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

i32 StarLightEngine::processIncreaseQueue(i32 maxUpdates) {
    i32 readIndex = 0;

    while (readIndex < m_increaseQueueInitialLength && maxUpdates > 0) {
        --maxUpdates;

        // FIFO：按写入顺序处理，保持波前传播语义
        const QueueEntry entry = m_increaseQueue[readIndex++];

        const i64 worldPos = entry.pos;
        const i32 propagatedLevel = static_cast<i32>(entry.level);
        const u64 flags = entry.flags;

        // 重新检查标志
        if (flags & FLAG_RECHECK_LEVEL) {
            if (getLevel(worldPos) != propagatedLevel) {
                continue;
            }
        }

        // 写入等级标志
        if (flags & FLAG_WRITE_LEVEL) {
            setLevel(worldPos, propagatedLevel);
        }

        notifyNeighbors(worldPos, propagatedLevel, false, entry.directions);
    }
    // 压缩未处理队列项，保留到下一次 tick。
    if (readIndex > 0) {
        const i32 remaining = m_increaseQueueInitialLength - readIndex;
        for (i32 i = 0; i < remaining; ++i) {
            m_increaseQueue[i] = m_increaseQueue[readIndex + i];
        }
        m_increaseQueueInitialLength = remaining;
    }

    return maxUpdates;
}

i32 StarLightEngine::processDecreaseQueue(i32 maxUpdates) {
    i32 readIndex = 0;

    while (readIndex < m_decreaseQueueInitialLength && maxUpdates > 0) {
        --maxUpdates;

        // FIFO：按写入顺序处理，先清暗再级联。
        const QueueEntry entry = m_decreaseQueue[readIndex++];

        const i64 worldPos = entry.pos;
        const i32 propagatedLevel = static_cast<i32>(entry.level);

        notifyNeighbors(worldPos, propagatedLevel, true, entry.directions);
    }
    // 压缩未处理队列项，保留到下一次 tick。
    if (readIndex > 0) {
        const i32 remaining = m_decreaseQueueInitialLength - readIndex;
        for (i32 i = 0; i < remaining; ++i) {
            m_decreaseQueue[i] = m_decreaseQueue[readIndex + i];
        }
        m_decreaseQueueInitialLength = remaining;
    }

    return maxUpdates;
}

void StarLightEngine::propagateLevel(i64 fromPos, i64 toPos, i32 level, bool isDecreasing) {
    if (toPos == LightEngineUtils::ROOT_POS) {
        return;
    }

    i32 targetLevel = getEdgeLevel(fromPos, toPos, level);
    if (targetLevel >= m_levelCount) {
        return;
    }

    i32 currentLevel = getLevel(toPos);

    i32 toX, toY, toZ;
    unpackWorldPos(toPos, toX, toY, toZ);

    if (!isDecreasing) {
        if (targetLevel >= currentLevel) {
            return;
        }

        setLevel(toPos, targetLevel);

        if (targetLevel > 0) {
            DirectionBit checkDirs = computeCheckDirections(fromPos, toPos);
            appendToIncreaseQueue(encodeQueueEntryWorld(toX, toY, toZ,
                static_cast<u8>(targetLevel), static_cast<u8>(checkDirs), FLAG_RECHECK_LEVEL));
        }
        return;
    }

    if (currentLevel >= (m_levelCount - 1)) {
        return;
    }

    if (currentLevel < targetLevel) {
        appendToIncreaseQueue(encodeQueueEntryWorld(toX, toY, toZ,
            static_cast<u8>(currentLevel), DIR_ALL, FLAG_RECHECK_LEVEL));

        // 在当前实现中，当边传播目标已经是最暗时，
        // 仅做“回补重检”会导致封顶场景残留旧亮度。
        // 这里先强制清暗并继续减亮级联，再由增亮重检恢复真实幸存光源。
        if (targetLevel >= (m_levelCount - 1) &&
            getEdgeLevel(LightEngineUtils::ROOT_POS, toPos, 0) >= (m_levelCount - 1)) {
            setLevel(toPos, m_levelCount - 1);

            for (Direction dir : LightEngineUtils::ALL_DIRECTIONS) {
                const i64 neighborPos = LightEngineUtils::offsetPos(toPos, dir);
                const i32 neighborLevel = getLevel(neighborPos);
                if (neighborLevel >= (m_levelCount - 1)) {
                    continue;
                }

                i32 nx, ny, nz;
                unpackWorldPos(neighborPos, nx, ny, nz);
                appendToIncreaseQueue(encodeQueueEntryWorld(nx, ny, nz,
                    static_cast<u8>(neighborLevel), DIR_ALL, FLAG_RECHECK_LEVEL));
            }

            DirectionBit checkDirs = computeCheckDirections(fromPos, toPos);
            appendToDecreaseQueue(encodeQueueEntryWorld(toX, toY, toZ,
                static_cast<u8>(m_levelCount - 1), static_cast<u8>(checkDirs), 0));
        }

        return;
    }

    i32 rootLevel = getEdgeLevel(LightEngineUtils::ROOT_POS, toPos, 0);
    if (rootLevel < (m_levelCount - 1)) {
        appendToIncreaseQueue(encodeQueueEntryWorld(toX, toY, toZ,
            static_cast<u8>(rootLevel), DIR_ALL, FLAG_WRITE_LEVEL));
    }

    setLevel(toPos, m_levelCount - 1);

    if (targetLevel < (m_levelCount - 1)) {
        DirectionBit checkDirs = computeCheckDirections(fromPos, toPos);
        appendToDecreaseQueue(encodeQueueEntryWorld(toX, toY, toZ,
            static_cast<u8>(targetLevel), static_cast<u8>(checkDirs), 0));
    }
}

} // namespace mc
