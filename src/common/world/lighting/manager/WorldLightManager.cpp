/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "WorldLightManager.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include <algorithm>
#include <fmt/format.h>

using namespace mc::trace;

namespace mc {

WorldLightManager::WorldLightManager(StarLightLightingProvider* provider, bool hasBlockLight, bool hasSkyLight)
    : m_provider(provider)
    , m_hasBlockLight(hasBlockLight)
    , m_hasSkyLight(hasSkyLight)
{

    if (hasBlockLight) {
        m_blockLight = std::make_unique<BlockStarLightEngine>();
    }

    if (hasSkyLight) {
        m_skyLight = std::make_unique<SkyStarLightEngine>();
    }
}

// ============================================================================
// 光照操作
// ============================================================================

void WorldLightManager::checkBlock(i32 x, i32 y, i32 z)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting,
        "WorldLightManager::checkBlock",
        "pos",
        fmt::format("({}, {}, {})", x, y, z),
        [flow = ::perfetto::Flow::ProcessScoped(BlockPos(x, y, z).toId())](
            ::perfetto::EventContext ctx) { flow(ctx); });

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    i32 chunkX = x >> world::CHUNK_SHIFT;
    i32 chunkZ = z >> world::CHUNK_SHIFT;

    // 使用 blocksChangedInChunk 流程处理方块变化
    std::vector<BlockPos> positions;
    positions.emplace_back(x, y, z);
    std::vector<bool> changedSections; // 空的，因为我们不知道段是否为空

    if (m_skyLight != nullptr) {
        m_skyLight->blocksChangedInChunk(m_provider, chunkX, chunkZ, positions, changedSections);
    }

    if (m_blockLight != nullptr) {
        m_blockLight->blocksChangedInChunk(m_provider, chunkX, chunkZ, positions, changedSections);
    }
}

void WorldLightManager::onBlockEmissionIncrease(i32 x, i32 y, i32 z, i32 lightLevel)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting,
        "WorldLightManager::onBlockEmissionIncrease",
        "pos",
        fmt::format("({}, {}, {})", x, y, z),
        "lightLevel",
        lightLevel,
        [flow = ::perfetto::Flow::ProcessScoped(BlockPos(x, y, z).toId())](
            ::perfetto::EventContext ctx) { flow(ctx); });

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_blockLight != nullptr) {
        m_blockLight->onBlockEmissionIncrease(m_provider, x, y, z, lightLevel);
    }
}

bool WorldLightManager::hasLightWork() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    bool skyHasWork = m_skyLight != nullptr && m_skyLight->hasWork();
    bool blockHasWork = m_blockLight != nullptr && m_blockLight->hasWork();

    return skyHasWork || blockHasWork;
}

i32 WorldLightManager::tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting,
        "WorldLightManager::tick",
        "maxUpdates",
        maxUpdates,
        "updateSkyLight",
        updateSkyLight,
        "updateBlockLight",
        updateBlockLight);

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

void WorldLightManager::updateSectionStatus(const SectionPos& pos, bool isEmpty)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting,
        "WorldLightManager::updateSectionStatus",
        "sectionPos",
        fmt::format("({}, {}, {})", pos.x, pos.y, pos.z),
        "isEmpty",
        isEmpty,
        [flow = ::perfetto::Flow::ProcessScoped(pos.toLong())](::perfetto::EventContext ctx) { flow(ctx); });

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_blockLight != nullptr) {
        m_blockLight->updateSectionStatus(pos, isEmpty);
    }

    if (m_skyLight != nullptr) {
        m_skyLight->updateSectionStatus(pos, isEmpty);
    }
}

// ============================================================================
// 光照访问
// ============================================================================

BlockStarLightEngine* WorldLightManager::getBlockLightEngine()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_blockLight.get();
}

const BlockStarLightEngine* WorldLightManager::getBlockLightEngine() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_blockLight.get();
}

SkyStarLightEngine* WorldLightManager::getSkyLightEngine()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_skyLight.get();
}

const SkyStarLightEngine* WorldLightManager::getSkyLightEngine() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_skyLight.get();
}

i32 WorldLightManager::getLightSubtracted(const BlockPos& pos, i32 skyDarkening) const
{
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

u8 WorldLightManager::getBlockLight(i32 x, i32 y, i32 z) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_blockLight != nullptr) {
        return m_blockLight->getLightFor(x, y, z);
    }
    return 0;
}

u8 WorldLightManager::getSkyLight(i32 x, i32 y, i32 z) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_skyLight != nullptr) {
        return m_skyLight->getLightFor(x, y, z);
    }
    return 0;
}

// ============================================================================
// 数据管理
// ============================================================================

void WorldLightManager::setData(LightType type, const SectionPos& pos, const NibbleArray& array, bool retain)
{

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

SWMRNibbleArray* WorldLightManager::getData(LightType type, const SectionPos& pos)
{
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

// ============================================================================
// 区块光照初始化
// ============================================================================

void WorldLightManager::forceLoadInChunk(const IChunk* chunk, const std::vector<bool>& emptySections)
{
    if (chunk == nullptr) {
        return;
    }

    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting,
        "WorldLightManager::forceLoadInChunk",
        "chunk",
        fmt::format("({}, {})", chunk->x(), chunk->z()));

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // 对已正确光照的区块，只需要加载光照数据到缓存并检查边缘

    if (m_skyLight != nullptr) {
        m_skyLight->forceHandleEmptySectionChanges(m_provider, chunk, emptySections);
    }

    if (m_blockLight != nullptr) {
        m_blockLight->forceHandleEmptySectionChanges(m_provider, chunk, emptySections);
    }
}

void WorldLightManager::lightChunk(const IChunk* chunk, bool needsEdgeChecks)
{
    if (chunk == nullptr) {
        return;
    }

    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting,
        "WorldLightManager::lightChunk",
        "chunk",
        fmt::format("({}, {})", chunk->x(), chunk->z()),
        "needsEdgeChecks",
        needsEdgeChecks);

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // 执行天空光照计算
    if (m_skyLight != nullptr) {
        m_skyLight->light(m_provider, chunk, needsEdgeChecks);
    }

    // 执行方块光照计算
    if (m_blockLight != nullptr) {
        m_blockLight->light(m_provider, chunk, needsEdgeChecks);
    }
}

void WorldLightManager::checkChunkEdges(i32 chunkX, i32 chunkZ)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting,
        "WorldLightManager::checkChunkEdges",
        "chunk",
        fmt::format("({}, {})", chunkX, chunkZ));

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_skyLight != nullptr) {
        m_skyLight->StarLightEngine::checkChunkEdges(m_provider, chunkX, chunkZ);
    }

    if (m_blockLight != nullptr) {
        m_blockLight->StarLightEngine::checkChunkEdges(m_provider, chunkX, chunkZ);
    }
}

// ============================================================================
// 区块光照初始化（指定 provider 重载，用于 LIGHT 阶段 worker 线程）
// ============================================================================

void WorldLightManager::lightChunk(StarLightLightingProvider* provider, const IChunk* chunk, bool needsEdgeChecks)
{
    if (chunk == nullptr) {
        return;
    }

    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting,
        "WorldLightManager::lightChunk(provider)",
        "chunk",
        fmt::format("({}, {})", chunk->x(), chunk->z()),
        "needsEdgeChecks",
        needsEdgeChecks);

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_skyLight != nullptr) {
        m_skyLight->light(provider, chunk, needsEdgeChecks);
    }

    if (m_blockLight != nullptr) {
        m_blockLight->light(provider, chunk, needsEdgeChecks);
    }
}

// ============================================================================
// 空区块段映射（线程安全）
// ============================================================================

void WorldLightManager::updateEmptinessMap(i32 chunkX, i32 chunkZ, const ChunkData* chunk)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting,
        "WorldLightManager::updateEmptinessMap",
        "chunk",
        fmt::format("({}, {})", chunkX, chunkZ));

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (m_blockLight != nullptr) {
        m_blockLight->updateEmptinessMap(chunkX, chunkZ, chunk);
    }
}

// ============================================================================
// 调试信息
// ============================================================================

std::string WorldLightManager::getDebugInfo(LightType type, const SectionPos& pos) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    switch (type) {
        case LightType::BLOCK: {
            if (m_blockLight == nullptr) {
                return "BlockLight: N/A";
            }

            // 获取区块段的 Nibble 数据状态
            const SWMRNibbleArray* nibble = m_blockLight->getData(pos);
            std::string sectionState;
            if (nibble == nullptr) {
                sectionState = "2"; // EMPTY - 无数据
            } else if (nibble->isNullUpdating()) {
                sectionState = "2"; // EMPTY - Null 状态
            } else if (nibble->isUninitializedUpdating()) {
                sectionState = "1"; // LIGHT_ONLY - 未初始化
            } else if (nibble->isHiddenUpdating()) {
                sectionState = "1"; // LIGHT_ONLY - 隐藏状态
            } else if (nibble->isInitializedUpdating()) {
                sectionState = "0"; // LIGHT_AND_DATA - 有完整数据
            }

            std::string result = "BlockLight:" + sectionState;

            // 附加脏标记
            if (nibble != nullptr && nibble->isDirty()) {
                result += "[dirty]";
            }

            // 附加引擎队列状态
            i32 queueSize = m_blockLight->queuedUpdateSize();
            if (queueSize > 0) {
                result += "[q:" + std::to_string(queueSize) + "]";
            }

            return result;
        }
        case LightType::SKY: {
            if (m_skyLight == nullptr) {
                return "SkyLight: N/A";
            }

            // 获取区块段的 Nibble 数据状态
            const SWMRNibbleArray* nibble = m_skyLight->getData(pos);
            std::string sectionState;
            if (nibble == nullptr) {
                sectionState = "2"; // EMPTY - 无数据
            } else if (nibble->isNullUpdating()) {
                sectionState = "2"; // EMPTY - Null 状态
            } else if (nibble->isUninitializedUpdating()) {
                sectionState = "1"; // LIGHT_ONLY - 未初始化
            } else if (nibble->isHiddenUpdating()) {
                sectionState = "1"; // LIGHT_ONLY - 隐藏状态
            } else if (nibble->isInitializedUpdating()) {
                sectionState = "0"; // LIGHT_AND_DATA - 有完整数据
            }

            std::string result = "SkyLight:" + sectionState;

            // 附加脏标记
            if (nibble != nullptr && nibble->isDirty()) {
                result += "[dirty]";
            }

            // 附加引擎队列状态
            i32 queueSize = m_skyLight->queuedUpdateSize();
            if (queueSize > 0) {
                result += "[q:" + std::to_string(queueSize) + "]";
            }

            return result;
        }
    }
    return "Unknown";
}

} // namespace mc
