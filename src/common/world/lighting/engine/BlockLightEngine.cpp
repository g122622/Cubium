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

BlockLightEngine::BlockLightEngine(IChunkLightProvider* provider)
    : LevelBasedGraph(16, 8192, provider)
    , m_storage(provider) {
}

// ============================================================================
// 光照操作
// ============================================================================

void BlockLightEngine::checkLight(const BlockPos& pos) {
    MC_TRACE_INSTANT("server.lighting",
        "BlockLightEngine::checkLight",
        "pos", fmt::format("({}, {}, {})", pos.x, pos.y, pos.z),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
            flow(ctx);
    });

    m_storage.processAllLevelUpdates();

    i64 packedPos = LightEngineUtils::packPos(pos);
    if (m_storage.hasSection(LightEngineUtils::worldToSectionPos(packedPos))) {
        scheduleUpdate(packedPos);
    }

    // 通知相邻方块
    for (Direction dir : LightEngineUtils::ALL_DIRECTIONS) {
        scheduleUpdate(LightEngineUtils::offsetPos(packedPos, dir));
    }
}

void BlockLightEngine::onBlockEmissionIncrease(const BlockPos& pos, i32 lightLevel) {
    m_storage.processAllLevelUpdates();
    i64 packedPos = LightEngineUtils::packPos(pos);
    i64 sourcePos = LightEngineUtils::ROOT_POS;  // 根节点表示光源

    scheduleUpdate(sourcePos, packedPos, 15 - lightLevel, true);
}

u8 BlockLightEngine::getLightFor(const BlockPos& pos) const {
    return m_storage.getLightOrDefault(LightEngineUtils::packPos(pos));
}

void BlockLightEngine::updateSectionStatus(const SectionPos& pos, bool isEmpty) {
    m_storage.updateSectionStatus(pos.toLong(), isEmpty);
}

void BlockLightEngine::setData(const SectionPos& pos, SWMRNibbleArray&& array, bool retain) {
    m_storage.setData(pos.toLong(), std::move(array), retain);
}

void BlockLightEngine::setData(const SectionPos& pos, const NibbleArray& array, bool retain) {
    m_storage.setData(pos.toLong(), array, retain);
}

SWMRNibbleArray* BlockLightEngine::getData(const SectionPos& pos) {
    return m_storage.getArray(pos.toLong());
}

bool BlockLightEngine::hasWork() const {
    return needsUpdate() || m_storage.hasSectionsToUpdate();
}

i32 BlockLightEngine::tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight) {
    MC_TRACE_EVENT("server.lighting", "BlockLightEngine::tick",
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

bool BlockLightEngine::isRoot(i64 pos) const {
    return pos == LightEngineUtils::ROOT_POS;
}

i32 BlockLightEngine::computeLevel(i64 pos, i64 excludedSource, i32 level) {
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

void BlockLightEngine::notifyNeighbors(i64 pos, i32 level, bool isDecreasing) {
    i64 sectionPos = LightEngineUtils::worldToSectionPos(pos);

    for (Direction dir : LightEngineUtils::ALL_DIRECTIONS) {
        i64 neighborPos = LightEngineUtils::offsetPos(pos, dir);
        i64 neighborSectionPos = LightEngineUtils::worldToSectionPos(neighborPos);

        if (sectionPos == neighborSectionPos || m_storage.hasSection(neighborSectionPos)) {
            propagateLevel(pos, neighborPos, level, isDecreasing);
        }
    }
}

i32 BlockLightEngine::getLevel(i64 pos) const {
    if (pos == LightEngineUtils::ROOT_POS) {
        return 0;
    }
    return 15 - m_storage.getLightOrDefault(pos);
}

void BlockLightEngine::setLevel(i64 pos, i32 level) {
    m_storage.setLight(pos, static_cast<u8>(15 - std::min(level, 15)));
}

i32 BlockLightEngine::getEdgeLevel(i64 fromPos, i64 toPos, i32 startLevel) {
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
    IWorld* world = getChunkProvider()->getWorld();

    // 检查面遮挡
    if (fromState != nullptr && toState != nullptr &&
        LightEngineUtils::facesHaveOcclusion(world, *fromState, BlockPos(fromX, fromY, fromZ),
                              *toState, BlockPos(toX, toY, toZ),
                              direction, opacity)) {
        return 15;
    }

    return startLevel + std::max(1, opacity);
}

// ============================================================================
// 私有方法
// ============================================================================

i32 BlockLightEngine::getLightValue(i64 worldPos) const {
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

const IChunk* BlockLightEngine::getChunkCached(i32 chunkX, i32 chunkZ) const {
    // 使用基类的缓存方法
    return LevelBasedGraph::getCachedChunk(chunkX, chunkZ);
}

// ============================================================================
// 空区块段检测
// ============================================================================

void BlockLightEngine::updateEmptinessMap(i32 chunkX, i32 chunkZ, const IChunk* chunk) {
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

bool BlockLightEngine::isSectionEmpty(i64 sectionPos) const {
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

EmptinessMap* BlockLightEngine::getOrCreateEmptinessMap(i64 columnPos) {
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
