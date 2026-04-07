#include "SkyLightEngine.hpp"
#include "LightEngineUtils.hpp"
#include "../../IWorld.hpp"
#include "../../block/Block.hpp"
#include "../../chunk/IChunk.hpp"
#include <climits>
#include <algorithm>
#include "common/perfetto/TraceEvents.hpp"
#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

SkyLightEngine::SkyLightEngine(IChunkLightProvider* provider)
    : LevelBasedGraph(16, 8192, provider)
    , m_storage(provider) {
}

// ============================================================================
// 光照操作
// ============================================================================

void SkyLightEngine::checkLight(const BlockPos& pos) {
    MC_TRACE_INSTANT("server.lighting",
        "SkyLightEngine::checkLight",
        "pos", fmt::format("({}, {}, {})", pos.x, pos.y, pos.z),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
            flow(ctx);
    });

    m_storage.processAllLevelUpdates();

    i64 packedPos = LightEngineUtils::packPos(pos);
    i64 sectionPos = LightEngineUtils::worldToSectionPos(packedPos);

    // 获取当前光照等级
    i32 currentLevel = getLevel(packedPos);

    // 参考 Starlight: SkyStarLightEngine.checkBlock。
    // 注意本实现内部 level 为“距离值”：0 最亮，15 最暗。

    if (m_storage.hasSection(sectionPos)) {
        i32 x, y, z;
        LightEngineUtils::unpackPos(packedPos, x, y, z);

        i32 rootLevel = getEdgeLevel(LightEngineUtils::ROOT_POS, packedPos, 0);
        if (rootLevel < 15) {
            // 与 Starlight 一致：当前位置可接收天空源时，
            // 先写入源级别，再走增亮传播。
            setLevel(packedPos, rootLevel);
            appendToIncreaseQueue(encodeQueueEntry(x, y, z,
                static_cast<u8>(rootLevel), DIR_ALL, FLAG_HAS_SIDED_TRANSPARENT));
        } else {
            // 先清空当前位置（最暗），再走减亮重算链。
            setLevel(packedPos, 15);
        }

        // 加入减亮队列，传播到邻居
        appendToDecreaseQueue(encodeQueueEntry(x, y, z, static_cast<u8>(currentLevel), DIR_ALL, 0));
    } else {
        // 向上查找有效的区块段
        i32 x, y, z;
        LightEngineUtils::unpackPos(packedPos, x, y, z);
        // 对齐到区块段底部 (清除Y的低4位)
        i64 currentPos = LightEngineUtils::packPos(x, y & ~0xF, z);
        SectionPos currentSection = SectionPos::fromLong(LightEngineUtils::worldToSectionPos(currentPos));
        i64 currentSectionPos = currentSection.toLong();

        while (!m_storage.hasSection(currentSectionPos) &&
               !m_storage.isAboveWorld(currentSectionPos)) {
            // 向上移动一个区块段 (16格)
            currentSection = currentSection.offset(Direction::Up);
            currentSectionPos = currentSection.toLong();
            currentPos = LightEngineUtils::packPos(x, currentSection.worldY(), z);
        }

        if (m_storage.hasSection(currentSectionPos)) {
            LightEngineUtils::unpackPos(currentPos, x, y, z);
            appendToIncreaseQueue(encodeQueueEntry(x, y, z, 0, DIR_ALL, FLAG_HAS_SIDED_TRANSPARENT));
        }
    }
}

u8 SkyLightEngine::getLightFor(const BlockPos& pos) const {
    return m_storage.getLightOrDefault(LightEngineUtils::packPos(pos));
}

void SkyLightEngine::updateSectionStatus(const SectionPos& pos, bool isEmpty) {
    m_storage.updateSectionStatus(pos.toLong(), isEmpty);
}

void SkyLightEngine::setData(const SectionPos& pos, SWMRNibbleArray&& array, bool retain) {
    m_storage.setData(pos.toLong(), std::move(array), retain);
}

void SkyLightEngine::setData(const SectionPos& pos, const NibbleArray& array, bool retain) {
    m_storage.setData(pos.toLong(), array, retain);
}

SWMRNibbleArray* SkyLightEngine::getData(const SectionPos& pos) {
    return m_storage.getArray(pos.toLong());
}

void SkyLightEngine::setColumnEnabled(i64 columnPos, bool enabled) {
    m_storage.setColumnEnabled(columnPos, enabled);
}

bool SkyLightEngine::hasWork() const {
    bool result = needsUpdate() || m_storage.hasSectionsToUpdate();
    return result;
}

i32 SkyLightEngine::tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight) {
    MC_TRACE_EVENT("server.lighting", "SkyLightEngine::tick",
                     "maxUpdates", maxUpdates,
                     "updateSkyLight", updateSkyLight,
                     "updateBlockLight", updateBlockLight);

    (void)updateBlockLight;  // 天空光照引擎不处理方块光照

    // 处理存储更新
    m_storage.processAllLevelUpdates();

    // 处理区块段更新（添加/移除）
    i32 updatesBeforeSections = maxUpdates;
    maxUpdates = m_storage.updateSections(this, maxUpdates, updateSkyLight, false);
    bool sectionsConsumedAll = (maxUpdates == 0);
    bool hasNeedsUpdate = needsUpdate();

    // 处理光照传播
    i32 updatesBeforeProcess = maxUpdates;
    if (hasNeedsUpdate && updateSkyLight) {
        maxUpdates = processUpdates(maxUpdates);
    }

    // 只在有实际光照变更时记录
    m_storage.updateAndNotify();

    return maxUpdates;
}

// ============================================================================
// LevelBasedGraph 接口实现
// ============================================================================

bool SkyLightEngine::isRoot(i64 pos) const {
    (void)pos;
    return false;  // 天空光照没有根节点
}

i32 SkyLightEngine::computeLevel(i64 pos, i64 excludedSource, i32 level) {
    i32 minLevel = level;

    // 如果不是从根节点排除，检查天空光照贡献
    if (excludedSource != LightEngineUtils::ROOT_POS) {
        i32 sourceContribution = getEdgeLevel(LightEngineUtils::ROOT_POS, pos, 0);
        if (level > sourceContribution) {
            minLevel = sourceContribution;
        }

        if (minLevel == 0) {
            return 0;
        }
    }

    i64 sectionPos = LightEngineUtils::worldToSectionPos(pos);
    const SWMRNibbleArray* array = m_storage.getArray(sectionPos, true);

    // 检查所有相邻方向
    for (Direction dir : LightEngineUtils::ALL_DIRECTIONS) {
        i64 neighborPos = LightEngineUtils::offsetPos(pos, dir);
        if (neighborPos == excludedSource) {
            continue;
        }

        i64 neighborSectionPos = LightEngineUtils::worldToSectionPos(neighborPos);
        const SWMRNibbleArray* neighborArray;

        if (neighborSectionPos == sectionPos) {
            neighborArray = array;
        } else {
            neighborArray = m_storage.getArray(neighborSectionPos, true);
        }

        if (neighborArray != nullptr) {
            i32 neighborLevel = getLevelFromArray(neighborArray, neighborPos);
            i32 edgeLevel = getEdgeLevel(neighborPos, pos, neighborLevel);

            if (minLevel > edgeLevel) {
                minLevel = edgeLevel;
            }

            if (minLevel == 0) {
                return 0;
            }
        } else if (dir != Direction::Down) {
            // 向上查找有效的区块段
            // 参考 MC: BlockPos.atSectionBottomY 将Y对齐到区块段底部
            i32 nx, ny, nz;
            LightEngineUtils::unpackPos(neighborPos, nx, ny, nz);
            // 对齐到区块段底部
            i64 searchPos = LightEngineUtils::packPos(nx, ny & ~0xF, nz);
            SectionPos searchSection = SectionPos::fromLong(neighborSectionPos);
            i64 searchSectionPos = searchSection.toLong();

            while (!m_storage.hasSection(searchSectionPos) &&
                   !m_storage.isAboveWorld(searchSectionPos)) {
                // 向上移动一个区块段 (16格)
                searchSection = searchSection.offset(Direction::Up);
                searchSectionPos = searchSection.toLong();
                searchPos = LightEngineUtils::packPos(nx, searchSection.worldY(), nz);
            }

            const SWMRNibbleArray* searchArray = m_storage.getArray(searchSectionPos, true);
            if (neighborPos != excludedSource) {
                i32 searchLevel;
                if (searchArray != nullptr) {
                    searchLevel = getLevelFromArray(searchArray, searchPos);
                } else {
                    searchLevel = m_storage.isSectionEnabled(searchSectionPos) ? 0 : 15;
                }

                if (minLevel > searchLevel) {
                    minLevel = searchLevel;
                }

                if (minLevel == 0) {
                    return 0;
                }
            }
        }
    }

    return minLevel;
}

void SkyLightEngine::notifyNeighbors(i64 pos, i32 level, bool isDecreasing, u8 directionBits) {
    i64 sectionPos = LightEngineUtils::worldToSectionPos(pos);
    const SectionPos sourceSection = SectionPos::fromLong(sectionPos);
    DirectionBit dirs = static_cast<DirectionBit>(directionBits);
    if (dirs == DIR_NONE) {
        dirs = DIR_ALL;
    }

    i32 x, y, z;
    LightEngineUtils::unpackPos(pos, x, y, z);

    if (!isDecreasing) {
        const IChunk* fromChunk = getChunkCached(x >> 4, z >> 4);
        const BlockState* fromState = LightEngineUtils::getBlockAndOpacity(fromChunk, pos, nullptr);

        if (fromState != nullptr) {
            bool hasOpenPropagationFace = false;
            for (Direction dir : LightEngineUtils::ALL_DIRECTIONS) {
                if ((dirs & DirectionBits::fromDirection(dir)) == 0) {
                    continue;
                }

                if (!LightEngineUtils::blocksLightInDirection(*fromState, dir)) {
                    hasOpenPropagationFace = true;
                    break;
                }
            }

            if (!hasOpenPropagationFace) {
                return;
            }
        }
    }

    i32 localY = y & 0xF;
    i32 sectionY = sourceSection.y;

    // 计算需要向下传播多少区块段
    i32 skipSections = 0;
    if (localY == 0) {
        // 在区块段底部，需要检查下方有多少空区块段
        while (m_storage.isAboveBottom(sectionY - skipSections - 1)) {
            i64 belowSectionPos = SectionPos(sourceSection.x, sectionY - skipSections - 1, sourceSection.z).toLong();
            if (m_storage.hasSection(belowSectionPos)) {
                break;
            }
            ++skipSections;
        }
    }

    // 向下传播（可能跨越多个区块段）
    if ((dirs & DIR_DOWN) != 0) {
        const i32 verticalDrop = 1 + skipSections * 16;
        i64 downPos = LightEngineUtils::packPos(x, y - verticalDrop, z);
        i64 downSectionPos = LightEngineUtils::worldToSectionPos(downPos);

        if (sectionPos == downSectionPos || m_storage.hasSection(downSectionPos)) {
            propagateLevel(pos, downPos, level, isDecreasing);
        }
    }

    // 向上传播
    if ((dirs & DIR_UP) != 0) {
        i64 upPos = LightEngineUtils::offsetPos(pos, Direction::Up);
        i64 upSectionPos = LightEngineUtils::worldToSectionPos(upPos);
        if (sectionPos == upSectionPos || m_storage.hasSection(upSectionPos)) {
            propagateLevel(pos, upPos, level, isDecreasing);
        }
    }

    // 水平方向传播
    const i32 maxHorizontalDrop = (skipSections > 0) ? (skipSections * 16 + 1) : 0;
    for (Direction dir : LightEngineUtils::HORIZONTAL_DIRECTIONS) {
        if ((dirs & DirectionBits::fromDirection(dir)) == 0) {
            continue;
        }

        i32 dx = (dir == Direction::East) ? 1 : ((dir == Direction::West) ? -1 : 0);
        i32 dz = (dir == Direction::South) ? 1 : ((dir == Direction::North) ? -1 : 0);

        for (i32 offset = 0; offset <= maxHorizontalDrop; ++offset) {
            i64 neighborPos = LightEngineUtils::packPos(x + dx, y - offset, z + dz);
            i64 neighborSectionPos = LightEngineUtils::worldToSectionPos(neighborPos);

            if (sectionPos == neighborSectionPos) {
                propagateLevel(pos, neighborPos, level, isDecreasing);
                break;
            }

            if (m_storage.hasSection(neighborSectionPos)) {
                propagateLevel(pos, neighborPos, level, isDecreasing);
            }
        }
    }
}

i32 SkyLightEngine::getLevel(i64 pos) const {
    return 15 - m_storage.getLightOrDefault(pos);
}

void SkyLightEngine::setLevel(i64 pos, i32 level) {
    m_storage.setLight(pos, static_cast<u8>(15 - std::min(level, 15)));
}

i32 SkyLightEngine::getEdgeLevel(i64 fromPos, i64 toPos, i32 startLevel) {
    if (toPos == LightEngineUtils::ROOT_POS) {
        return 15;
    }

    if (fromPos == LightEngineUtils::ROOT_POS) {
        // 从天空发出
        if (!m_storage.isAtSurfaceTop(toPos)) {
            return 15;
        }
        startLevel = 0;
    }

    if (startLevel >= 15) {
        return startLevel;
    }

    i32 opacity = 0;
    i32 toX, toY, toZ;
    LightEngineUtils::unpackPos(toPos, toX, toY, toZ);

    // 使用缓存获取区块
    const IChunk* toChunk = getChunkCached(toX >> 4, toZ >> 4);
    const BlockState* toState = LightEngineUtils::getBlockAndOpacity(toChunk, toPos, &opacity);

    if (opacity >= 15) {
        return 15;
    }

    i32 fromX, fromY, fromZ;
    LightEngineUtils::unpackPos(fromPos, fromX, fromY, fromZ);

    // 优化：如果两个位置在同一区块，复用区块指针
    const IChunk* fromChunk;
    i32 fromChunkX = fromX >> 4;
    i32 fromChunkZ = fromZ >> 4;
    i32 toChunkX = toX >> 4;
    i32 toChunkZ = toZ >> 4;
    if (fromChunkX == toChunkX && fromChunkZ == toChunkZ) {
        fromChunk = toChunk;
    } else {
        fromChunk = getChunkCached(fromChunkX, fromChunkZ);
    }

    const BlockState* fromState = LightEngineUtils::getBlockAndOpacity(fromChunk, fromPos, nullptr);

    // 计算方向
    bool sameXZ = (fromX == toX) && (fromZ == toZ);
    i32 dx = (toX > fromX) ? 1 : ((toX < fromX) ? -1 : 0);
    i32 dy = (toY > fromY) ? 1 : ((toY < fromY) ? -1 : 0);
    i32 dz = (toZ > fromZ) ? 1 : ((toZ < fromZ) ? -1 : 0);

    Direction direction;
    if (fromPos == LightEngineUtils::ROOT_POS) {
        direction = Direction::Down;
    } else {
        direction = Directions::fromDelta(dx, dy, dz);
    }

    if (direction != Direction::None) {
        // 对齐 Vanilla/SkyStarLightEngine：相邻传播主要由来源方块对应面的遮挡决定。
        if (fromState != nullptr && LightEngineUtils::blocksLightInDirection(*fromState, direction)) {
            return 15;
        }
    } else {
        // 非相邻方块（Sky notifyNeighbors 的水平+垂直组合路径）。
        // 对齐 Vanilla：先检查来源向下遮挡，再检查目标在调整方向上的反向遮挡。
        if (fromState != nullptr && LightEngineUtils::blocksLightInDirection(*fromState, Direction::Down)) {
            return 15;
        }

        i32 adjustedDy = sameXZ ? -1 : 0;
        Direction adjustedDir = Directions::fromDelta(dx, adjustedDy, dz);
        if (adjustedDir == Direction::None) {
            return 15;
        }

        if (toState != nullptr &&
            LightEngineUtils::blocksLightInDirection(*toState, Directions::opposite(adjustedDir))) {
            return 15;
        }
    }

    // 天空光照特殊处理：从天空向下传播且无遮挡时，衰减为0
    bool isFromSky = (fromPos == LightEngineUtils::ROOT_POS) || (sameXZ && fromY > toY);
    if (isFromSky && startLevel == 0 && opacity == 0) {
        return 0;
    }

    return startLevel + std::max(1, opacity);
}

// ============================================================================
// 私有方法
// ============================================================================

i32 SkyLightEngine::getLightValue(i64 worldPos) const {
    // 天空光照引擎不处理发光方块
    (void)worldPos;
    return 0;
}

i32 SkyLightEngine::getLevelFromArray(const SWMRNibbleArray* array, i64 worldPos) const {
    if (array == nullptr) {
        return 15;
    }

    i32 x, localY, z;
    LightEngineUtils::extractNibbleIndices(worldPos, x, localY, z);

    return 15 - array->getUpdating(x, localY, z);
}

const IChunk* SkyLightEngine::getChunkCached(i32 chunkX, i32 chunkZ) const {
    // 使用基类的缓存方法
    return LevelBasedGraph::getCachedChunk(chunkX, chunkZ);
}

} // namespace mc
