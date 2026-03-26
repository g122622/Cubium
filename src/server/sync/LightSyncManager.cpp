#include "LightSyncManager.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/lighting/storage/SWMRNibbleArray.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/chunk/IChunk.hpp"
#include "common/util/NibbleArray.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include <spdlog/spdlog.h>

namespace mc::server::sync {

LightSyncManager::LightSyncManager(WorldLightManager& lightManager,
                                   ServerChunkManager& chunkManager)
    : m_lightManager(lightManager)
    , m_chunkManager(chunkManager)
{
}

void LightSyncManager::initializeChunkLighting(ChunkCoord x, ChunkCoord z)
{
    MC_TRACE_EVENT("server.lighting", "InitializeChunkLighting",
                   "Chunk", fmt::format("({}, {})", x, z));

    const ChunkData* chunk = m_chunkManager.getChunk(x, z);
    if (!chunk) {
        spdlog::warn("[LightSync] Chunk not loaded for ({}, {})", x, z);
        return;
    }

    const ChunkPos chunkPos(x, z);

    for (i32 sectionY = 0; sectionY < world::CHUNK_SECTIONS; ++sectionY) {
        const ChunkSection* section = chunk->getSection(sectionY);
        const SectionPos sectionPos(x, sectionY, z);

        const bool isEmpty = (section == nullptr || section->isEmpty());
        m_lightManager.updateSectionStatus(sectionPos, isEmpty);

        if (section != nullptr) {
            if (m_lightManager.getSkyLightEngine()) {
                NibbleArray skyLightCopy = section->skyLightNibble().copy();
                m_lightManager.setData(LightType::SKY, sectionPos, skyLightCopy, false);
            }

            if (m_lightManager.getBlockLightEngine()) {
                NibbleArray blockLightCopy = section->blockLightNibble().copy();
                m_lightManager.setData(LightType::BLOCK, sectionPos, blockLightCopy, false);
            }
        }
    }

    m_lightManager.enableLightSources(chunkPos, true);
}

void LightSyncManager::onBlockStateChanged(i32 x, i32 y, i32 z, i32 oldLightLevel, i32 newLightLevel)
{
    const BlockPos pos(x, y, z);
    m_lightManager.checkBlock(pos);

    if (newLightLevel > oldLightLevel) {
        spdlog::debug("[LightSync] Emission increased at ({}, {}, {}): {} -> {}",
                      x, y, z, oldLightLevel, newLightLevel);
        m_lightManager.onBlockEmissionIncrease(pos, newLightLevel);
    }
}

void LightSyncManager::markLightChanged(LightType type, const SectionPos& pos)
{
    MC_TRACE_EVENT("server.lighting", "MarkLightChanged",
                   "Type", (type == LightType::SKY) ? "SKY" : "BLOCK",
                   "Section", fmt::format("({}, {}, {})", pos.x, pos.y, pos.z));

    // 标记区块为脏
    ChunkData* chunk = m_chunkManager.getChunk(pos.x, pos.z);
    if (chunk) {
        chunk->setDirty(true);
    }

    // 同步光照数据到 ChunkSection
    syncLightDataToChunk(type, pos);

    const char* typeName = (type == LightType::SKY) ? "SKY" : "BLOCK";
    spdlog::trace("[LightSync] {} light changed at section ({}, {}, {})",
                  typeName, pos.x, pos.y, pos.z);
}

void LightSyncManager::syncLightDataToChunk(LightType type, const SectionPos& pos)
{
    ChunkData* chunk = m_chunkManager.getChunk(pos.x, pos.z);
    if (!chunk) {
        return;
    }

    const i32 sectionIndex = pos.y;
    if (sectionIndex < 0 || sectionIndex >= world::CHUNK_SECTIONS) {
        return;
    }

    ChunkSection* section = chunk->getSection(sectionIndex);
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
        NibbleArray& targetArray = (type == LightType::SKY)
            ? section->skyLightNibble()
            : section->blockLightNibble();
        targetArray.data() = std::move(data);
    }
}

} // namespace mc::server::sync