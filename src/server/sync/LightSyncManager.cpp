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

// 在macOS系统头文件中，BYTE_SIZE被定义为宏，会与NibbleArray的静态常数冲突
// 使用pragma push_macro/pop_macro来暂时屏蔽系统宏
#pragma push_macro("BYTE_SIZE")
#undef BYTE_SIZE

#include "LightSyncManager.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/NibbleArray.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/lighting/storage/SWMRNibbleArray.hpp"
#include "server/world/ServerChunkManager.hpp"
#include <spdlog/spdlog.h>

#undef BYTE_SIZE // Re-undef after includes which may re-define BYTE_SIZE

using namespace mc::trace;

namespace mc::server::sync {

LightSyncManager::LightSyncManager(WorldLightManager& lightManager, ServerChunkManager& chunkManager) noexcept
    : m_lightManager(lightManager)
    , m_chunkManager(chunkManager)
{}

void LightSyncManager::initializeChunkLighting(ChunkCoord x, ChunkCoord z)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting,
        "LightSyncManager::initializeChunkLighting",
        "Chunk",
        fmt::format("({}, {})", x, z));

    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting, "GetChunkData");
    auto chunk = m_chunkManager.tryToGetChunkSharedInMem(x, z);
    if (!chunk) {
        return;
    }

    const ChunkData* chunkData = chunk.get();

    // 与 Moonrise ChunkLightTask 一致：区分区块是否已正确光照
    // 参考: ChunkLightTask.java 第154-165行
    //
    // if (task.fromChunk.isLightCorrect() && task.fromChunk.getPersistedStatus().isOrAfter(ChunkStatus.LIGHT)) {
    //     this.lightEngine.forceLoadInChunk(task.fromChunk, emptySections);
    //     this.lightEngine.checkChunkEdges(task.chunkX, task.chunkZ);
    // } else {
    //     task.fromChunk.setLightCorrect(false);
    //     this.lightEngine.lightChunk(task.fromChunk, emptySections);
    //     task.fromChunk.setLightCorrect(true);
    // }

    // 计算空区块段
    std::vector<bool> emptySections;
    const ChunkSection* const* sections = chunkData->getSections();
    constexpr i32 sectionCount = world::CHUNK_SECTIONS;
    emptySections.resize(static_cast<size_t>(sectionCount), false);

    for (i32 sectionY = 0; sectionY < sectionCount; ++sectionY) {
        const ChunkSection* section = (sections != nullptr) ? sections[sectionY] : nullptr;
        emptySections[static_cast<size_t>(sectionY)] = (section == nullptr || section->isEmpty());
    }

    // 检查区块是否已正确光照
    bool isLightCorrect = chunkData->isLightCorrect();
    ChunkLoadStatus status = chunkData->getStatus();
    bool hasLightStatus = (status == ChunkLoadStatus::Generated || status == ChunkLoadStatus::Loaded);

    if (isLightCorrect && hasLightStatus) {
        // 区块已正确光照，只需要重新加载光照数据并检查边缘
        // 与 Moonrise 一致：使用 forceLoadInChunk + checkChunkEdges
        m_lightManager.forceLoadInChunk(chunkData, emptySections);
        m_lightManager.checkChunkEdges(x, z);
    } else {
        // 区块需要完整光照计算
        // 与 Moonrise 一致：设置 lightCorrect = false，执行 lightChunk，然后设置 lightCorrect = true

        // 先更新空区块段状态
        BlockStarLightEngine* blockLightEngine = m_lightManager.getBlockLightEngine();
        if (blockLightEngine != nullptr) {
            blockLightEngine->updateEmptinessMap(x, z, chunkData);
        }

        for (i32 sectionY = 0; sectionY < sectionCount; ++sectionY) {
            const ChunkSection* section = (sections != nullptr) ? sections[sectionY] : nullptr;
            SectionPos sectionPos(x, world::sectionIndexToCoord(sectionY), z);
            bool isEmpty = (section == nullptr || section->isEmpty());
            m_lightManager.updateSectionStatus(sectionPos, isEmpty);
        }

        // 设置光照状态为不正确
        const_cast<ChunkData*>(chunkData)->setLightCorrect(false);

        // 执行光照计算
        m_lightManager.lightChunk(chunkData, true);

        // 设置光照状态为正确
        const_cast<ChunkData*>(chunkData)->setLightCorrect(true);
    }
}

void LightSyncManager::onBlockStateChanged(i32 x, i32 y, i32 z, i32 oldLightLevel, i32 newLightLevel)
{
    m_lightManager.checkBlock(x, y, z);

    if (newLightLevel > oldLightLevel) {
        m_lightManager.onBlockEmissionIncrease(x, y, z, newLightLevel);
    }
}

void LightSyncManager::markLightChanged(LightType type, const SectionPos& pos)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting,
        "LightSyncManager::markLightChanged",
        "Type",
        (type == LightType::SKY) ? "SKY" : "BLOCK",
        "Section",
        fmt::format("({}, {}, {})", pos.x, pos.y, pos.z));

    // 标记区块为脏
    auto chunk = m_chunkManager.tryToGetChunkSharedInMem(pos.x, pos.z);
    if (chunk) {
        chunk->setDirty(true);
    }

    // 同步光照数据到 ChunkSection
    syncLightDataToChunk(type, pos);
}

void LightSyncManager::syncLightDataToChunk(LightType type, const SectionPos& pos)
{
    auto chunk = m_chunkManager.tryToGetChunkSharedInMem(pos.x, pos.z);
    if (!chunk) {
        return;
    }

    ChunkData* chunkData = chunk.get();

    const i32 sectionIndex = world::sectionCoordToIndex(pos.y);
    if (sectionIndex < 0 || sectionIndex >= world::CHUNK_SECTIONS) {
        return;
    }

    ChunkSection* section = chunkData->getSection(sectionIndex);
    if (!section) {
        return;
    }

    // 从 WorldLightManager 获取光照数据
    SWMRNibbleArray* lightData = m_lightManager.getData(type, pos);
    if (!lightData) {
        return;
    }

    // 获取数据副本并同步到 ChunkSection
    std::vector<u8> data = lightData->toByteArray();
    if (data.size() == NibbleArray::BYTE_SIZE) {
        NibbleArray& targetArray = (type == LightType::SKY) ? section->skyLightNibble() : section->blockLightNibble();
        targetArray.data() = std::move(data);
    }
}

} // namespace mc::server::sync

#pragma pop_macro("BYTE_SIZE")
