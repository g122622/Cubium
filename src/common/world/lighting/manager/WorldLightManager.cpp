#include "WorldLightManager.hpp"
#include <algorithm>
#include "common/util/assert/AssertAll.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <spdlog/spdlog.h>

namespace mc {

WorldLightManager::WorldLightManager(
    IChunkLightProvider* provider,
    bool hasBlockLight,
    bool hasSkyLight)
    : m_provider(provider)
    , m_hasBlockLight(hasBlockLight)
    , m_hasSkyLight(hasSkyLight) {

    if (hasBlockLight) {
        m_blockLight = std::make_unique<BlockLightEngine>(provider);
    }

    if (hasSkyLight) {
        m_skyLight = std::make_unique<SkyLightEngine>(provider);
    }
}

// ============================================================================
// 光照操作
// ============================================================================

void WorldLightManager::checkBlock(const BlockPos& pos) {
    MC_TRACE_INSTANT("server.lighting",
        "WorldLightManager::checkBlock",
        "pos", fmt::format("({}, {}, {})", pos.x, pos.y, pos.z),
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) {
            flow(ctx);
    });

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_blockLight != nullptr) {
        m_blockLight->checkLight(pos);
    }

    if (m_skyLight != nullptr) {
        m_skyLight->checkLight(pos);
    }
}

void WorldLightManager::onBlockEmissionIncrease(const BlockPos& pos, i32 lightLevel) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_blockLight != nullptr) {
        m_blockLight->onBlockEmissionIncrease(pos, lightLevel);
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

    const auto clampRemaining = [](i32 remaining, i32 budget) -> i32 {
        return std::clamp(remaining, 0, budget);
    };

    if (m_blockLight != nullptr && m_skyLight != nullptr) {
        // 两个引擎都有，分配更新配额
        i32 blockQuota = maxUpdates / 2;
        i32 blockRemaining = clampRemaining(
            m_blockLight->tick(blockQuota, updateSkyLight, updateBlockLight),
            blockQuota);

        i32 skyQuota = maxUpdates - blockQuota + blockRemaining;
        i32 skyRemaining = clampRemaining(
            m_skyLight->tick(skyQuota, updateSkyLight, updateBlockLight),
            skyQuota);

        // 如果方块光照用完了配额但天空光照还有剩余，给方块光照更多配额
        if (blockRemaining == 0 && skyRemaining > 0) {
            return clampRemaining(
                m_blockLight->tick(skyRemaining, updateSkyLight, updateBlockLight),
                skyRemaining);
        }
        return skyRemaining;
    }

    if (m_blockLight != nullptr) {
        return clampRemaining(
            m_blockLight->tick(maxUpdates, false, updateBlockLight),
            maxUpdates);
    }

    if (m_skyLight != nullptr) {
        return clampRemaining(
            m_skyLight->tick(maxUpdates, updateSkyLight, false),
            maxUpdates);
    }

    return maxUpdates;
}

// ============================================================================
// 区块段管理
// ============================================================================

void WorldLightManager::updateSectionStatus(const SectionPos& pos, bool isEmpty) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_blockLight != nullptr) {
        m_blockLight->updateSectionStatus(pos, isEmpty);
    }

    if (m_skyLight != nullptr) {
        m_skyLight->updateSectionStatus(pos, isEmpty);
    }
}

void WorldLightManager::enableLightSources(const ChunkPos& pos, bool enable) {
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

BlockLightEngine* WorldLightManager::getBlockLightEngine() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_blockLight.get();
}

const BlockLightEngine* WorldLightManager::getBlockLightEngine() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_blockLight.get();
}

SkyLightEngine* WorldLightManager::getSkyLightEngine() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_skyLight.get();
}

const SkyLightEngine* WorldLightManager::getSkyLightEngine() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_skyLight.get();
}

i32 WorldLightManager::getLightSubtracted(const BlockPos& pos, i32 skyDarkening) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    i32 skyLight = 0;
    if (m_skyLight != nullptr) {
        skyLight = static_cast<i32>(m_skyLight->getLightFor(pos)) - skyDarkening;
        skyLight = std::max(0, skyLight);
    }

    i32 blockLight = 0;
    if (m_blockLight != nullptr) {
        blockLight = static_cast<i32>(m_blockLight->getLightFor(pos));
    }

    return std::max(blockLight, skyLight);
}

u8 WorldLightManager::getBlockLight(const BlockPos& pos) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_blockLight != nullptr) {
        return m_blockLight->getLightFor(pos);
    }
    return 0;
}

u8 WorldLightManager::getSkyLight(const BlockPos& pos) const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_skyLight != nullptr) {
        return m_skyLight->getLightFor(pos);
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
