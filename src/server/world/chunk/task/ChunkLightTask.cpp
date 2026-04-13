#include "server/world/chunk/task/ChunkLightTask.hpp"
#include "server/world/chunk/ChunkTaskScheduler.hpp"
#include "server/world/ServerWorld.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/perfetto/TraceEvents.hpp"

namespace mc {

ChunkLightTask::ChunkLightTask(ChunkTaskScheduler& scheduler, server::ServerWorld& world,
                                 i32 chunkX, i32 chunkZ)
    : ChunkProgressionTask(scheduler, world, chunkX, chunkZ) {
}

void ChunkLightTask::schedule() {
    if (m_scheduled.exchange(true, std::memory_order_acq_rel)) {
        return;  // 已经调度
    }

    if (m_cancelled.load(std::memory_order_acquire)) {
        complete(nullptr, "Task cancelled");
        return;
    }

    // 光照需要写入半径 2，使用半径感知调度器
    auto* radiusAware = &m_scheduler.getRadiusAwareScheduler();
    if (radiusAware) {
        radiusAware->addTask(
            [this]() { executeLight(); },
            m_chunkX, m_chunkZ, 2, m_priority);
    }
}

void ChunkLightTask::cancel() {
    markCancelled();
}

void ChunkLightTask::setChunkData(ChunkData* data) {
    m_chunkData = data;
}

void ChunkLightTask::executeLight() {
    if (m_cancelled.load(std::memory_order_acquire)) {
        complete(nullptr, "Task cancelled");
        return;
    }

    if (m_completed.load(std::memory_order_acquire)) {
        return;  // 已经完成
    }

    MC_TRACE_EVENT("world.chunk_task", "LightTask");

    // 实际的光照计算逻辑
    if (m_chunkData) {
        auto* lightManager = m_world.lightManager();
        if (lightManager) {
            // 参考 Moonrise ChunkLightTask：区分已光照和未光照区块
            if (m_chunkData->isLightCorrect()) {
                // 已正确光照：只需重新加载光照数据并检查边缘
                std::vector<bool> emptySections;
                for (i32 i = 0; i < ChunkData::SECTIONS; ++i) {
                    emptySections.push_back(!m_chunkData->hasSection(i) ||
                                           m_chunkData->getSection(i) == nullptr ||
                                           m_chunkData->getSection(i)->isEmpty());
                }
                lightManager->forceLoadInChunk(m_chunkData, emptySections);
                lightManager->checkChunkEdges(m_chunkX, m_chunkZ);
            } else {
                // 需要完整光照计算
                m_chunkData->setLightCorrect(false);
                lightManager->lightChunk(m_chunkData, true);
                m_chunkData->setLightCorrect(true);
            }
        }
    }

    complete(nullptr, "");
}

} // namespace mc
