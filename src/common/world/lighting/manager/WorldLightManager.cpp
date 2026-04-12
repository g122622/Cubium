#include "WorldLightManager.hpp"
#include "../../chunk/IChunk.hpp"
#include "../../chunk/ChunkData.hpp"
#include "../../WorldConstants.hpp"
#include "../../../util/assert/AssertAll.hpp"
#include "../../../perfetto/TraceEvents.hpp"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace mc {

WorldLightManager::WorldLightManager(
    StarLightLightingProvider* provider,
    bool hasBlockLight,
    bool hasSkyLight)
    : m_provider(provider)
    , m_hasBlockLight(hasBlockLight)
    , m_hasSkyLight(hasSkyLight) {

    if (hasBlockLight) {
        m_blockLight = std::make_unique<BlockStarLightEngine>(provider);
    }

    if (hasSkyLight) {
        m_skyLight = std::make_unique<SkyStarLightEngine>(provider);
    }
}

// ============================================================================
// 光照操作
// ============================================================================

void WorldLightManager::checkBlock(i32 x, i32 y, i32 z) {
    MC_TRACE_EVENT("server.lighting",
        "WorldLightManager::checkBlock",
        "pos", fmt::format("({}, {}, {})", x, y, z),
        [flow = ::perfetto::Flow::ProcessScoped(BlockPos(x, y, z).toId())](::perfetto::EventContext ctx) {
            flow(ctx);
    });

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    i32 chunkX = x >> 4;
    i32 chunkZ = z >> 4;

    // 使用 blocksChangedInChunk 流程（与 Moonrise StarLightInterface.blockChange 一致）
    // 这会调用 propagateBlockChanges，对于天空光照会正确传播天空光
    std::vector<BlockPos> positions;
    positions.emplace_back(x, y, z);
    std::vector<bool> changedSections;  // 空的，因为我们不知道段是否为空

    if (m_skyLight != nullptr) {
        m_skyLight->blocksChangedInChunk(m_provider, chunkX, chunkZ, positions, changedSections);
    }

    if (m_blockLight != nullptr) {
        m_blockLight->blocksChangedInChunk(m_provider, chunkX, chunkZ, positions, changedSections);
    }
}

void WorldLightManager::onBlockEmissionIncrease(i32 x, i32 y, i32 z, i32 lightLevel) {
    MC_TRACE_EVENT("server.lighting",
        "WorldLightManager::onBlockEmissionIncrease",
        "pos", fmt::format("({}, {}, {})", x, y, z),
        "lightLevel", lightLevel,
        [flow = ::perfetto::Flow::ProcessScoped(BlockPos(x, y, z).toId())](::perfetto::EventContext ctx) {
            flow(ctx);
    });

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_blockLight != nullptr) {
        m_blockLight->onBlockEmissionIncrease(m_provider, x, y, z, lightLevel);
    }
}

bool WorldLightManager::hasLightWork() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    bool skyHasWork = m_skyLight != nullptr && m_skyLight->hasWork();
    bool blockHasWork = m_blockLight != nullptr && m_blockLight->hasWork();

    return skyHasWork || blockHasWork;
}

i32 WorldLightManager::tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight) {
    MC_TRACE_EVENT("server.lighting", "WorldLightManager::tick",
                   "maxUpdates", maxUpdates,
                   "updateSkyLight", updateSkyLight,
                   "updateBlockLight", updateBlockLight);

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    MC_ASSERT_RELEASE_MSG(maxUpdates >= 0, "Max updates must be positive or zero");

    i32 remaining = maxUpdates;

    if (m_blockLight != nullptr && updateBlockLight) {
        remaining = m_blockLight->tick(remaining, false, true);
    }

    if (m_skyLight != nullptr && updateSkyLight && remaining > 0) {
        remaining = m_skyLight->tick(remaining, true, false);
    }

    return std::clamp(remaining, 0, maxUpdates);
}

// ============================================================================
// 区块段管理
// ============================================================================

void WorldLightManager::updateSectionStatus(const SectionPos& pos, bool isEmpty) {
    MC_TRACE_EVENT("server.lighting",
        "WorldLightManager::updateSectionStatus",
        "sectionPos", fmt::format("({}, {}, {})", pos.x, pos.y, pos.z),
        "isEmpty", isEmpty,
        [flow = ::perfetto::Flow::ProcessScoped(pos.toLong())](::perfetto::EventContext ctx) {
            flow(ctx);
    });

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_blockLight != nullptr) {
        m_blockLight->updateSectionStatus(pos, isEmpty);
    }

    if (m_skyLight != nullptr) {
        m_skyLight->updateSectionStatus(pos, isEmpty);
    }
}

void WorldLightManager::enableLightSources(const ChunkPos& pos, bool enable) {
    MC_TRACE_EVENT("server.lighting",
        "WorldLightManager::enableLightSources",
        "chunkPos", fmt::format("({}, {})", pos.x, pos.z),
        "enable", enable,
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
            flow(ctx);
    });

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // 区块列位置编码
    i64 columnPos = (static_cast<i64>(pos.x) & 0x3FFFFFLL) << 42 |
                    (static_cast<i64>(pos.z) & 0x3FFFFFLL) << 20;

    if (m_blockLight != nullptr) {
        // 方块光照启用区块列
        // 通过存储层处理
    }

    if (m_skyLight != nullptr) {
        m_skyLight->setColumnEnabled(columnPos, enable);
    }
}

// ============================================================================
// 光照访问
// ============================================================================

BlockStarLightEngine* WorldLightManager::getBlockLightEngine() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_blockLight.get();
}

const BlockStarLightEngine* WorldLightManager::getBlockLightEngine() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_blockLight.get();
}

SkyStarLightEngine* WorldLightManager::getSkyLightEngine() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_skyLight.get();
}

const SkyStarLightEngine* WorldLightManager::getSkyLightEngine() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_skyLight.get();
}

i32 WorldLightManager::getLightSubtracted(const BlockPos& pos, i32 skyDarkening) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    i32 skyLight = 0;
    if (m_skyLight != nullptr) {
        skyLight = static_cast<i32>(m_skyLight->getLightFor(pos.x, pos.y, pos.z)) - skyDarkening;
        skyLight = std::max(0, skyLight);
    }

    i32 blockLight = 0;
    if (m_blockLight != nullptr) {
        blockLight = static_cast<i32>(m_blockLight->getLightFor(pos.x, pos.y, pos.z));
    }

    return std::max(blockLight, skyLight);
}

u8 WorldLightManager::getBlockLight(i32 x, i32 y, i32 z) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_blockLight != nullptr) {
        return m_blockLight->getLightFor(x, y, z);
    }
    return 0;
}

u8 WorldLightManager::getSkyLight(i32 x, i32 y, i32 z) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_skyLight != nullptr) {
        return m_skyLight->getLightFor(x, y, z);
    }
    return 0;
}

// ============================================================================
// 数据管理
// ============================================================================

void WorldLightManager::setData(
    LightType type,
    const SectionPos& pos,
    const NibbleArray& array,
    bool retain) {

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    switch (type) {
        case LightType::BLOCK:
            if (m_blockLight != nullptr) {
                m_blockLight->setData(pos, array, retain);
            }
            break;
        case LightType::SKY:
            if (m_skyLight != nullptr) {
                m_skyLight->setData(pos, array, retain);
            }
            break;
    }
}

SWMRNibbleArray* WorldLightManager::getData(LightType type, const SectionPos& pos) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    switch (type) {
        case LightType::BLOCK:
            if (m_blockLight != nullptr) {
                return m_blockLight->getData(pos);
            }
            break;
        case LightType::SKY:
            if (m_skyLight != nullptr) {
                return m_skyLight->getData(pos);
            }
            break;
    }
    return nullptr;
}

void WorldLightManager::retainData(const ChunkPos& pos, bool retain) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // 通过存储层保留数据
    // 目前简化实现，后续可扩展
    (void)pos;
    (void)retain;
}

// ============================================================================
// 区块光照初始化
// ============================================================================

void WorldLightManager::forceLoadInChunk(const IChunk* chunk, const std::vector<bool>& emptySections) {
    if (chunk == nullptr) {
        return;
    }

    MC_TRACE_EVENT("server.lighting", "WorldLightManager::forceLoadInChunk",
                   "chunk", fmt::format("({}, {})", chunk->x(), chunk->z()));

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // 与 Moonrise StarLightInterface.forceLoadInChunk 一致
    // 对已正确光照的区块，只需要加载光照数据到缓存并检查边缘

    if (m_skyLight != nullptr) {
        m_skyLight->forceHandleEmptySectionChanges(m_provider, chunk, emptySections);
    }

    if (m_blockLight != nullptr) {
        m_blockLight->forceHandleEmptySectionChanges(m_provider, chunk, emptySections);
    }
}

void WorldLightManager::lightChunk(const IChunk* chunk, bool needsEdgeChecks) {
    if (chunk == nullptr) {
        return;
    }

    MC_TRACE_EVENT("server.lighting", "WorldLightManager::lightChunk",
                   "chunk", fmt::format("({}, {})", chunk->x(), chunk->z()),
                   "needsEdgeChecks", needsEdgeChecks);

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // 执行天空光照计算
    if (m_skyLight != nullptr) {
        m_skyLight->light(m_provider, chunk, needsEdgeChecks);
    }

    // 执行方块光照计算
    if (m_blockLight != nullptr) {
        m_blockLight->light(m_provider, chunk, needsEdgeChecks);
    }

    // 启用区块光源
    enableLightSources(ChunkPos(chunk->x(), chunk->z()), true);
}

void WorldLightManager::checkChunkEdges(i32 chunkX, i32 chunkZ) {
    MC_TRACE_EVENT("server.lighting", "WorldLightManager::checkChunkEdges",
                   "chunk", fmt::format("({}, {})", chunkX, chunkZ));

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_skyLight != nullptr) {
        m_skyLight->StarLightEngine::checkChunkEdges(m_provider, chunkX, chunkZ);
    }

    if (m_blockLight != nullptr) {
        m_blockLight->StarLightEngine::checkChunkEdges(m_provider, chunkX, chunkZ);
    }
}

// ============================================================================
// 调试信息
// ============================================================================

String WorldLightManager::getDebugInfo(LightType type, const SectionPos& pos) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    (void)pos;  // 暂时未使用

    switch (type) {
        case LightType::BLOCK:
            if (m_blockLight != nullptr) {
                return "BlockLight: active";
            }
            return "BlockLight: N/A";
        case LightType::SKY:
            if (m_skyLight != nullptr) {
                return "SkyLight: active";
            }
            return "SkyLight: N/A";
    }
    return "Unknown";
}

} // namespace mc
