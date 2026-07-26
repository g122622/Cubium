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
 */

#include "ChunkLoadLightTask.hpp"

#include "ServerWorld.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/chunk/base/SectionPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/ChunkSection.hpp"
#include "common/world/chunk/data/IChunk.hpp"
#include "common/world/lighting/engine/BlockLightEngine.hpp"
#include "common/world/lighting/engine/SkyLightEngine.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include <fmt/format.h>

using namespace mc::trace;

namespace mc::server {

ChunkLoadLightTask::ChunkLoadLightTask(ServerWorld& world, ChunkCoord chunkX, ChunkCoord chunkZ)
    : m_world(&world)
    , m_chunkX(chunkX)
    , m_chunkZ(chunkZ)
    , m_provider(world, chunkX, chunkZ)
{}

bool ChunkLoadLightTask::execute(const std::atomic<bool>& abortSignal)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting,
        "ChunkLoadLightTask::execute",
        "chunk",
        fmt::format("({}, {})", m_chunkX, m_chunkZ),
        [flow = ::perfetto::Flow::ProcessScoped(ChunkPos(m_chunkX, m_chunkZ).toId())](
            ::perfetto::EventContext ctx) { flow(ctx); });

    // 任务可能被取消（关服/区块卸载），检查后安全跳过。
    // onCancel 负责票据释放，此处不处理。
    if (abortSignal.load(std::memory_order::acquire)) {
        return false;
    }

    // 中心区块（保活范围已包含，类型必为 ChunkData）。
    IChunk* centerIChunk = m_provider.getChunkForLight(m_chunkX, m_chunkZ);
    if (centerIChunk == nullptr) {
        // 中心区块在入队后、执行前被卸载且保活失败（shared_ptr 为空）——跳过，
        // 发送续延不触发，客户端不会收到该区块。onCancel 不再走（execute 已返回），
        // 票据由 enqueueChunkLoadLight 的对称 remove 路径处理（见 ServerWorld）。
        return true;
    }

    auto* chunkData = static_cast<ChunkData*>(centerIChunk);

    // 计算空区块段标记（与原 LightSyncManager::initializeChunkLighting 同构，
    // 对齐 Moonrise ChunkLightTask 的 emptySections 计算）。
    const ChunkSection* const* sections = chunkData->getSections();
    constexpr i32 sectionCount = world::CHUNK_SECTIONS;
    std::vector<bool> emptySections;
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting, "ChunkLoadLightTask::execute::EmptySections");
        emptySections.resize(static_cast<size_t>(sectionCount), false);
        for (i32 sectionY = 0; sectionY < sectionCount; ++sectionY) {
            const ChunkSection* section = (sections != nullptr) ? sections[static_cast<size_t>(sectionY)] : nullptr;
            emptySections[static_cast<size_t>(sectionY)] = (section == nullptr || section->isEmpty());
        }
    }

    // 区分区块是否已正确光照（对齐 Moonrise ChunkLightTask.java:154-165）。
    const bool isLightCorrect = chunkData->isLightCorrect();
    const ChunkLoadStatus status = chunkData->getStatus();
    const bool hasLightStatus = (status == ChunkLoadStatus::Generated || status == ChunkLoadStatus::Loaded);

    WorldLightManager* lightManager = m_world->lightManager();
    MC_ASSERT_RELEASE(lightManager != nullptr);

    if (isLightCorrect && hasLightStatus) {
        // 区块已正确光照，只需重新加载光照数据到缓存并检查边缘（对齐 Moonrise forceLoadInChunk + checkChunkEdges）。
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting, "ChunkLoadLightTask::execute::RelightEdges");
        if (lightManager->hasSkyLight()) {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting, "ChunkLoadLightTask::execute::RelightEdges::Sky");
            auto* skyEngine = WorldLightManager::acquireSkyLightEngine();
            skyEngine->forceHandleEmptySectionChanges(&m_provider, chunkData, emptySections);
            skyEngine->StarLightEngine::checkChunkEdges(&m_provider, m_chunkX, m_chunkZ);
            WorldLightManager::releaseSkyLightEngine(skyEngine);
        }
        if (lightManager->hasBlockLight()) {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting, "ChunkLoadLightTask::execute::RelightEdges::Block");
            auto* blockEngine = WorldLightManager::acquireBlockLightEngine();
            blockEngine->forceHandleEmptySectionChanges(&m_provider, chunkData, emptySections);
            blockEngine->StarLightEngine::checkChunkEdges(&m_provider, m_chunkX, m_chunkZ);
            WorldLightManager::releaseBlockLightEngine(blockEngine);
        }
    } else {
        // 区块需完整光照计算：先更新空映射与段状态，再 light()，最后标记光照正确。
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting, "ChunkLoadLightTask::execute::ComputeLight");
        chunkData->setLightCorrect(false);

        if (lightManager->hasBlockLight()) {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting, "ChunkLoadLightTask::execute::ComputeLight::Block");
            auto* blockEngine = WorldLightManager::acquireBlockLightEngine();
            blockEngine->updateEmptinessMap(m_chunkX, m_chunkZ, chunkData);
            for (i32 sectionY = 0; sectionY < sectionCount; ++sectionY) {
                const ChunkSection* section = (sections != nullptr) ? sections[static_cast<size_t>(sectionY)] : nullptr;
                const SectionPos sectionPos(m_chunkX, world::sectionIndexToCoord(sectionY), m_chunkZ);
                blockEngine->updateSectionStatus(sectionPos, section == nullptr || section->isEmpty());
            }
            blockEngine->light(&m_provider, chunkData, /*needsEdgeChecks=*/true);
            WorldLightManager::releaseBlockLightEngine(blockEngine);
        }

        if (lightManager->hasSkyLight()) {
            MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting, "ChunkLoadLightTask::execute::ComputeLight::Sky");
            auto* skyEngine = WorldLightManager::acquireSkyLightEngine();
            for (i32 sectionY = 0; sectionY < sectionCount; ++sectionY) {
                const ChunkSection* section = (sections != nullptr) ? sections[static_cast<size_t>(sectionY)] : nullptr;
                const SectionPos sectionPos(m_chunkX, world::sectionIndexToCoord(sectionY), m_chunkZ);
                skyEngine->updateSectionStatus(sectionPos, section == nullptr || section->isEmpty());
            }
            skyEngine->light(&m_provider, chunkData, /*needsEdgeChecks=*/true);
            WorldLightManager::releaseSkyLightEngine(skyEngine);
        }

        chunkData->setLightCorrect(true);
    }

    // 取出 worker 收集的 dirty section，入主线程 flush 队列。
    // 主线程下一 tick _drainPendingLightFlushes 调真正的 markLightChanged
    // （_syncLightDataToChunk 把 visible nibble 同步到 ChunkSection + m_onLightChanged 网络包）。
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Lighting, "ChunkLoadLightTask::execute::FlushDirtySections");
        auto dirtySections = m_provider.takeDirtySections();
        if (!dirtySections.empty()) {
            m_world->_enqueueLightFlush(std::move(dirtySections));
        }
    }

    // 入区块发送续延队列：主线程 _drainPendingChunkSends（在 flush 之后）调
    // sendChunkToTrackingPlayers，serialize 读已 flush 的 ChunkSection nibble，
    // 保证客户端收到正确光照而非全黑区块。
    m_world->_enqueueChunkSend(m_chunkX, m_chunkZ);

    return true;
}

void ChunkLoadLightTask::onCancel()
{
    // 任务被取消（未执行或执行返回 false 后取消）时，LIGHT 票据需经主线程释放，
    // 防止区块永不卸载。复用区块发送续延队列携带坐标回主线程——
    // _drainPendingChunkSends 会调 removeLightTicket（无论发送成功与否）。
    // 中心区块未在保活范围（极端竞态）时 _enqueueChunkSend 仍入坐标，
    // removeLightTicket 对未持票据的 chunk 是 no-op（releaseTicket 容忍）。
    m_world->_enqueueChunkSend(m_chunkX, m_chunkZ);
}

std::string ChunkLoadLightTask::description() const
{
    return fmt::format("ChunkLoadLightTask(chunk=({},{}))", m_chunkX, m_chunkZ);
}

} // namespace mc::server
