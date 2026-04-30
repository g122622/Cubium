// 在macOS系统头文件中，BYTE_SIZE被定义为宏，会与NibbleArray的静态常数冲突
// 使用pragma push_macro/pop_macro来暂时屏蔽系统宏
#pragma push_macro("BYTE_SIZE")
#undef BYTE_SIZE

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

#pragma pop_macro("BYTE_SIZE")

namespace mc::server::sync {

LightSyncManager::LightSyncManager(WorldLightManager& lightManager,
                                   ServerChunkManager& chunkManager)
    : m_lightManager(lightManager)
    , m_chunkManager(chunkManager)
{
}

void LightSyncManager::initializeChunkLighting(ChunkCoord x, ChunkCoord z)
{
    MC_TRACE_EVENT("server.lighting", "LightSyncManager::initializeChunkLighting",
                   "Chunk", fmt::format("({}, {})", x, z));

    const ChunkData* chunk = nullptr;
    {
        MC_TRACE_EVENT("server.lighting", "GetChunkData");
        chunk = m_chunkManager.getChunk(x, z);
        if (!chunk) {
            return;
        }
    }

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
    const ChunkSection* const* sections = chunk->getSections();
    constexpr i32 sectionCount = world::CHUNK_SECTIONS;
    emptySections.resize(static_cast<size_t>(sectionCount), false);

    for (i32 sectionY = 0; sectionY < sectionCount; ++sectionY) {
        const ChunkSection* section = (sections != nullptr) ? sections[sectionY] : nullptr;
        emptySections[static_cast<size_t>(sectionY)] = (section == nullptr || section->hasOnlyAir());
    }

    // 检查区块是否已正确光照
    bool isLightCorrect = chunk->isLightCorrect();
    ChunkLoadStatus status = chunk->getStatus();
    bool hasLightStatus = (status == ChunkLoadStatus::Generated || status == ChunkLoadStatus::Loaded);

    if (isLightCorrect && hasLightStatus) {
        // 区块已正确光照，只需要重新加载光照数据并检查边缘
        // 与 Moonrise 一致：使用 forceLoadInChunk + checkChunkEdges
        // spdlog::debug("[LightSync] Chunk ({}, {}) already light correct, using forceLoadInChunk", x, z);
        m_lightManager.forceLoadInChunk(chunk, emptySections);
        m_lightManager.checkChunkEdges(x, z);
    } else {
        // 区块需要完整光照计算
        // 与 Moonrise 一致：设置 lightCorrect = false，执行 lightChunk，然后设置 lightCorrect = true
        // spdlog::debug("[LightSync] Chunk ({}, {}) needs lighting", x, z);

        // 先更新空区块段状态
        BlockStarLightEngine* blockLightEngine = m_lightManager.getBlockLightEngine();
        if (blockLightEngine != nullptr) {
            blockLightEngine->updateEmptinessMap(x, z, chunk);
        }

        for (i32 sectionY = 0; sectionY < sectionCount; ++sectionY) {
            const ChunkSection* section = (sections != nullptr) ? sections[sectionY] : nullptr;
            SectionPos sectionPos(x, sectionY, z);
            bool isEmpty = (section == nullptr || section->isEmpty());
            m_lightManager.updateSectionStatus(sectionPos, isEmpty);
        }

        // 设置光照状态为不正确
        const_cast<ChunkData*>(chunk)->setLightCorrect(false);

        // 执行光照计算
        m_lightManager.lightChunk(chunk, true);

        // 设置光照状态为正确
        const_cast<ChunkData*>(chunk)->setLightCorrect(true);
    }
}

void LightSyncManager::onBlockStateChanged(i32 x, i32 y, i32 z, i32 oldLightLevel, i32 newLightLevel)
{
    m_lightManager.checkBlock(x, y, z);

    if (newLightLevel > oldLightLevel) {
        spdlog::debug("[LightSync] Emission increased at ({}, {}, {}): {} -> {}",
                      x, y, z, oldLightLevel, newLightLevel);
        m_lightManager.onBlockEmissionIncrease(x, y, z, newLightLevel);
    }
}

void LightSyncManager::markLightChanged(LightType type, const SectionPos& pos)
{
    MC_TRACE_EVENT("server.lighting", "LightSyncManager::markLightChanged",
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