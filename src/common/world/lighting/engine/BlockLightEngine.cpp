#include "BlockLightEngine.hpp"
#include "LightEngineUtils.hpp"
#include "../../IWorld.hpp"
#include "../../block/Block.hpp"
#include "../../chunk/IChunk.hpp"
#include <climits>
#include <algorithm>
#include "common/perfetto/TraceEvents.hpp"
#include <fmt/format.h>

namespace mc {

// ============================================================================
// 构造函数
// ============================================================================

BlockStarLightEngine::BlockStarLightEngine(StarLightLightingProvider* provider)
    : StarLightEngine(16, 8192, provider)
    , m_storage(provider) {
}

// ============================================================================
// 光照操作
// ============================================================================

void BlockStarLightEngine::checkBlock(StarLightLightingProvider* lightAccess, i32 worldX, i32 worldY, i32 worldZ) {
    (void)lightAccess;
    MC_TRACE_INSTANT("server.lighting",
        "BlockStarLightEngine::checkBlock",
        "pos", fmt::format("({}, {}, {})", worldX, worldY, worldZ),
        [flow = ::perfetto::Flow::ProcessScoped(BlockPos(worldX, worldY, worldZ).toId())](::perfetto::EventContext ctx) {
            flow(ctx);
    });

    m_storage.processAllLevelUpdates();

    i64 packedPos = LightEngineUtils::packPos(worldX, worldY, worldZ);
    i64 sectionPos = LightEngineUtils::worldToSectionPos(packedPos);
    if (!m_storage.hasSection(sectionPos)) {
        return;
    }

    i32 currentLevel = getLevel(packedPos);
    i32 sourceLevel = getEdgeLevel(LightEngineUtils::ROOT_POS, packedPos, 0);

    // 与 Starlight 一致：先写当前位置，再通过增减队列统一处理邻居传播。
    setLevel(packedPos, sourceLevel);

    i32 x = worldX;
    i32 y = worldY;
    i32 z = worldZ;

    if (sourceLevel < 15) {
        appendToIncreaseQueue(encodeQueueEntry(x, y, z,
            static_cast<u8>(sourceLevel), DIR_ALL, 0));
    }

    appendToDecreaseQueue(encodeQueueEntry(x, y, z,
        static_cast<u8>(currentLevel), DIR_ALL, 0));
}

void BlockStarLightEngine::onBlockEmissionIncrease(StarLightLightingProvider* lightAccess, i32 worldX, i32 worldY, i32 worldZ, i32 lightLevel) {
    (void)lightAccess;
    m_storage.processAllLevelUpdates();

    const i64 packedPos = LightEngineUtils::packPos(worldX, worldY, worldZ);
    const i64 sectionPos = LightEngineUtils::worldToSectionPos(packedPos);
    if (!m_storage.hasSection(sectionPos)) {
        return;
    }

    const i32 currentLevel = getLightLevel(packedPos);
    const i32 sourceLevel = std::clamp(lightLevel, 0, 15);
    if (sourceLevel <= currentLevel) {
        return;
    }

    setLightLevel(packedPos, sourceLevel);

    const i32 x = worldX;
    const i32 y = worldY;
    const i32 z = worldZ;

    appendToIncreaseQueue(encodeQueueEntry(x, y, z,
        static_cast<u8>(StarLightEngine::MAX_LEVEL_COUNT - 1 - sourceLevel), DIR_ALL, 0));
}

u8 BlockStarLightEngine::getLightFor(i32 worldX, i32 worldY, i32 worldZ) const {
    return m_storage.getLightOrDefault(LightEngineUtils::packPos(worldX, worldY, worldZ));
}

void BlockStarLightEngine::updateSectionStatus(const SectionPos& pos, bool isEmpty) {
    m_storage.updateSectionStatus(pos.toLong(), isEmpty);
}

void BlockStarLightEngine::setData(const SectionPos& pos, SWMRNibbleArray&& array, bool retain) {
    m_storage.setData(pos.toLong(), std::move(array), retain);
}

void BlockStarLightEngine::setData(const SectionPos& pos, const NibbleArray& array, bool retain) {
    m_storage.setData(pos.toLong(), array, retain);
}

SWMRNibbleArray* BlockStarLightEngine::getData(const SectionPos& pos) {
    return m_storage.getArray(pos.toLong());
}

bool BlockStarLightEngine::hasWork() const {
    return needsUpdate() || m_storage.hasSectionsToUpdate();
}

i32 BlockStarLightEngine::tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight) {
    MC_TRACE_EVENT("server.lighting", "BlockStarLightEngine::tick",
                     "maxUpdates", maxUpdates,
                     "updateSkyLight", updateSkyLight,
                     "updateBlockLight", updateBlockLight);

    (void)updateSkyLight;   // 方块光照引擎不处理天空光照
    (void)updateBlockLight; // 参数保留用于接口一致性

    // 处理存储更新（如区块段状态变化）
    m_storage.processAllLevelUpdates();

    // 处理光照传播
    if (needsUpdate()) {
        maxUpdates = processUpdates(maxUpdates);
        if (maxUpdates == 0) {
            return 0;
        }
    }

    m_storage.updateAndNotify();
    return maxUpdates;
}

// ============================================================================
// LevelBasedGraph 接口实现
// ============================================================================

bool BlockStarLightEngine::isRoot(i64 pos) const {
    return pos == LightEngineUtils::ROOT_POS;
}

i32 BlockStarLightEngine::computeLevel(i64 pos, i64 excludedSource, i32 level) {
    i32 minLevel = level;

    // 如果不是从根节点排除，检查光源贡献
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

        if (neighborArray == nullptr) {
            continue;
        }

        // 获取相邻位置的光照等级
        i32 x, localY, z;
        LightEngineUtils::extractNibbleIndices(neighborPos, x, localY, z);

        i32 neighborLevel = 15 - neighborArray->getUpdating(x, localY, z);
        i32 edgeLevel = getEdgeLevel(neighborPos, pos, neighborLevel);

        if (minLevel > edgeLevel) {
            minLevel = edgeLevel;
        }

        if (minLevel == 0) {
            return 0;
        }
    }

    return minLevel;
}

void BlockStarLightEngine::notifyNeighbors(i64 pos, i32 level, bool isDecreasing, u8 directionBits) {
    i64 sectionPos = LightEngineUtils::worldToSectionPos(pos);
    DirectionBit dirs = static_cast<DirectionBit>(directionBits);
    if (dirs == DIR_NONE) {
        dirs = DIR_ALL;
    }

    for (Direction dir : LightEngineUtils::ALL_DIRECTIONS) {
        if ((dirs & DirectionBits::fromDirection(dir)) == 0) {
            continue;
        }

        i64 neighborPos = LightEngineUtils::offsetPos(pos, dir);
        i64 neighborSectionPos = LightEngineUtils::worldToSectionPos(neighborPos);

        if (sectionPos == neighborSectionPos || m_storage.hasSection(neighborSectionPos)) {
            propagateLevel(pos, neighborPos, level, isDecreasing);
        }
    }
}

i32 BlockStarLightEngine::getLevel(i64 pos) const {
    if (pos == LightEngineUtils::ROOT_POS) {
        return 0;
    }
    return 15 - m_storage.getLightOrDefault(pos);
}

void BlockStarLightEngine::setLevel(i64 pos, i32 level) {
    m_storage.setLight(pos, static_cast<u8>(15 - std::min(level, 15)));
}

i32 BlockStarLightEngine::getEdgeLevel(i64 fromPos, i64 toPos, i32 startLevel) {
    if (toPos == LightEngineUtils::ROOT_POS) {
        return 15;
    }

    if (fromPos == LightEngineUtils::ROOT_POS) {
        // 从光源发出
        return startLevel + 15 - getLightValue(toPos);
    }

    if (startLevel >= 15) {
        return startLevel;
    }

    // 计算方向
    i32 fromX, fromY, fromZ;
    i32 toX, toY, toZ;
    LightEngineUtils::unpackPos(fromPos, fromX, fromY, fromZ);
    LightEngineUtils::unpackPos(toPos, toX, toY, toZ);

    i32 dx = (toX > fromX) ? 1 : ((toX < fromX) ? -1 : 0);
    i32 dy = (toY > fromY) ? 1 : ((toY < fromY) ? -1 : 0);
    i32 dz = (toZ > fromZ) ? 1 : ((toZ < fromZ) ? -1 : 0);

    Direction direction = Directions::fromDelta(dx, dy, dz);
    if (direction == Direction::None) {
        return 15;
    }

    i32 opacity = 0;
    i32 toChunkX = toX >> 4;
    i32 toChunkZ = toZ >> 4;

    // 使用缓存获取区块
    const IChunk* toChunk = getChunkCached(toChunkX, toChunkZ);
    const BlockState* toState = LightEngineUtils::getBlockAndOpacity(toChunk, toPos, &opacity);

    if (opacity >= 15) {
        return 15;
    }

    i32 fromChunkX = fromX >> 4;
    i32 fromChunkZ = fromZ >> 4;

    // 优化：如果两个位置在同一区块，复用区块指针
    const IChunk* fromChunk;
    if (fromChunkX == toChunkX && fromChunkZ == toChunkZ) {
        fromChunk = toChunk;
    } else {
        fromChunk = getChunkCached(fromChunkX, fromChunkZ);
    }

    const BlockState* fromState = LightEngineUtils::getBlockAndOpacity(fromChunk, fromPos, nullptr);

    // Starlight 中来源面遮挡主要用于“条件不透明”形状。
    // 对于完整立方体（如萤石）不能在这里直接阻断，否则光源无法向外扩散。
    if (fromState != nullptr) {
        const CollisionShape& fromShape = LightEngineUtils::getVoxelShape(*fromState);
        if (!fromShape.isFullBlock() && LightEngineUtils::blocksLightInDirection(*fromState, direction)) {
            return 15;
        }
    }

    return startLevel + std::max(1, opacity);
}

// ============================================================================
// 私有方法
// ============================================================================

i32 BlockStarLightEngine::getLightValue(i64 worldPos) const {
    i32 x, y, z;
    LightEngineUtils::unpackPos(worldPos, x, y, z);

    // 使用缓存获取区块
    const IChunk* chunk = getChunkCached(x >> 4, z >> 4);
    if (chunk == nullptr) {
        return 0;
    }

    const BlockState* state = chunk->getBlock(x & 0xF, y, z & 0xF);
    if (state == nullptr) {
        return 0;
    }

    // 使用动态光照等级（支持熔炉、重生锚等动态光源）
    return state->getBlock().getLightLevel(*state);
}

i32 BlockStarLightEngine::getLightLevel(i64 worldPos) const {
    return StarLightEngine::MAX_LEVEL_COUNT - 1 - getLevel(worldPos);
}

void BlockStarLightEngine::setLightLevel(i64 worldPos, i32 level) {
    setLevel(worldPos,
        StarLightEngine::MAX_LEVEL_COUNT - 1 - std::clamp(level, 0, StarLightEngine::MAX_LEVEL_COUNT - 1));
}

const IChunk* BlockStarLightEngine::getChunkCached(i32 chunkX, i32 chunkZ) const {
    // 使用基类的缓存方法
    return StarLightEngine::getCachedChunk(chunkX, chunkZ);
}

// ============================================================================
// 空区块段检测
// ============================================================================

void BlockStarLightEngine::updateEmptinessMap(i32 chunkX, i32 chunkZ, const IChunk* chunk) {
    if (chunk == nullptr) {
        return;
    }

    // 计算区块列位置
    i64 columnPos = SectionPos(chunkX, 0, chunkZ).toLong();

    // 获取或创建空区块段映射
    EmptinessMap* map = getOrCreateEmptinessMap(columnPos);
    if (map != nullptr) {
        map->updateFromChunk(*chunk);
    }
}

bool BlockStarLightEngine::isSectionEmpty(i64 sectionPos) const {
    // 从区块段位置提取区块列位置
    SectionPos pos = SectionPos::fromLong(sectionPos);
    i64 columnPos = SectionPos(pos.x, 0, pos.z).toLong();

    // 查找空区块段映射
    auto it = m_emptinessMaps.find(columnPos);
    if (it == m_emptinessMaps.end()) {
        // 没有映射，使用存储层检查
        return !m_storage.hasSection(sectionPos);
    }

    return it->second.isSectionEmpty(pos.y);
}

EmptinessMap* BlockStarLightEngine::getOrCreateEmptinessMap(i64 columnPos) {
    auto it = m_emptinessMaps.find(columnPos);
    if (it != m_emptinessMaps.end()) {
        return &it->second;
    }

    // 获取高度范围
    i32 minSection = getChunkProvider()->getMinBuildHeight() >> 4;
    i32 maxSection = (getChunkProvider()->getMaxBuildHeight() - 1) >> 4;

    // 创建新的映射
    auto result = m_emptinessMaps.emplace(columnPos, EmptinessMap(minSection, maxSection));
    return &result.first->second;
}

} // namespace mc
