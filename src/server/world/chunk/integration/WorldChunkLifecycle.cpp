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

#include "server/world/chunk/integration/WorldChunkLifecycle.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "server/network/sync/ChunkSendManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/chunk/base/completion/ChunkCompletionQueue.hpp"
#include "server/world/chunk/base/completion/ChunkRequestCompletion.hpp"
#include "server/world/chunk/base/pipeline/ChunkPyramid.hpp"
#include "server/world/chunk/lifecycle/generation/ChunkLightingProvider.hpp"
#include "server/world/chunk/lifecycle/state/ChunkRegistry.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include "server/world/lighting/engine/BlockLightEngine.hpp"
#include "server/world/lighting/engine/SkyLightEngine.hpp"
#include "server/world/lighting/manager/WorldLightManager.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <utility>
#include <spdlog/spdlog.h>
using namespace mc::trace;

namespace mc::server {
using namespace mc::world::chunk;
namespace ChunkStatuses = mc::world::chunk::ChunkStatuses;

WorldChunkLifecycle::WorldChunkLifecycle(
    ServerWorld* world, ChunkRegistry& registry, world::chunk::ChunkDemandController& demand)
    : m_world(world)
    , m_registry(registry)
    , m_ticketManager(demand)
{}

void WorldChunkLifecycle::_postProcessChunk(ChunkData& chunk)
{
    const auto pendingBlockTicks = chunk.takePendingBlockTicks();
    if (m_world != nullptr && !pendingBlockTicks.empty()) {
        const u64 now = m_world->currentTick();
        for (const world::chunk::PendingBlockTick& tick : pendingBlockTicks) {
            const u64 scheduledAt = static_cast<u64>(tick.scheduledTick);
            const u64 dueTick = scheduledAt + static_cast<u64>(tick.delay);
            const i32 remainingDelay =
                now >= dueTick ? 0 : static_cast<i32>(std::min<u64>(dueTick - now, static_cast<u64>(INT32_MAX)));
            m_world->tickManager().scheduleBlockTick(tick.pos, *tick.block, remainingDelay, tick.priority);
        }
    }
    const auto pendingFluidTicks = chunk.takePendingFluidTicks();
    if (m_world != nullptr && !pendingFluidTicks.empty()) {
        const u64 now = m_world->currentTick();
        for (const world::chunk::PendingFluidTick& tick : pendingFluidTicks) {
            const u64 scheduledAt = static_cast<u64>(tick.scheduledTick);
            const u64 dueTick = scheduledAt + static_cast<u64>(tick.delay);
            const i32 remainingDelay =
                now >= dueTick ? 0 : static_cast<i32>(std::min<u64>(dueTick - now, static_cast<u64>(INT32_MAX)));
            m_world->tickManager().scheduleFluidTick(tick.pos, *tick.fluid, remainingDelay, tick.priority);
        }
    }
    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();
    const i32 startX = chunkX * world::CHUNK_WIDTH;
    const i32 startZ = chunkZ * world::CHUNK_WIDTH;

    for (i32 sectionIdx = 0; sectionIdx < world::CHUNK_SECTIONS; ++sectionIdx) {
        const auto& packedPositions = chunk.postProcessingSections()[sectionIdx];
        if (packedPositions.empty()) {
            continue;
        }

        for (u16 packed : packedPositions) {
            const i32 localX = packed & world::CHUNK_MASK;
            const i32 localY = (packed >> world::SECTION_SHIFT) & world::CHUNK_MASK;
            const i32 localZ = (packed >> (world::SECTION_SHIFT * 2)) & world::CHUNK_MASK;
            const i32 worldX = localX + startX;
            const i32 worldY = localY + (sectionIdx * world::CHUNK_SECTION_HEIGHT + world::MIN_BUILD_HEIGHT);
            const i32 worldZ = localZ + startZ;

            const BlockPos pos(worldX, worldY, worldZ);
            const BlockState* blockState = chunk.getBlockState(localX, worldY, localZ);
            if (blockState == nullptr) {
                continue;
            }
            if (blockState->isLiquid()) {
                const auto* fluidState = blockState->getFluidState();
                if (fluidState != nullptr && !fluidState->isEmpty()) {
                    const auto& fluid = fluidState->getFluid();
                    m_world->tickManager().scheduleFluidTick(pos, fluid, fluid.getTickDelay(*m_world));
                }
            }
            if (!blockState->isLiquid()) {
                BlockState updated = Block::updateFromNeighbourShapes(*blockState, *m_world, pos);
                if (updated != *blockState) {
                    m_world->setBlockState(pos,
                        &updated,
                        world::BlockUpdateFlags::UPDATE_INVISIBLE | world::BlockUpdateFlags::UPDATE_KNOWN_SHAPE |
                            world::BlockUpdateFlags::UPDATE_SKIP_BLOCK_ENTITY_SIDEEFFECTS);
                }
            }
        }

        chunk.clearPostProcessingForSection(sectionIdx);
    }
}

void WorldChunkLifecycle::enqueue(
    ChunkCoord x, ChunkCoord z, std::vector<SpawnedEntityData>&& spawnedEntities, bool needsPostProcess)
{
    PendingPostProcess item;
    item.holder = m_registry.findShared(x, z);
    item.x = x;
    item.z = z;
    item.spawnedEntities = std::move(spawnedEntities);
    item.needsPostProcess = needsPostProcess;
    {
        std::lock_guard<std::mutex> lock(m_pendingPostProcessMutex);
        m_pendingPostProcess.push_back(std::move(item));
        if (m_pendingPostProcess.size() > PENDING_POST_PROCESS_WARN_THRESHOLD) {
            spdlog::warn("Pending post-process backlog reached {} (threshold {}); main tick may be lagging",
                m_pendingPostProcess.size(),
                PENDING_POST_PROCESS_WARN_THRESHOLD);
        }
    }
}

void WorldChunkLifecycle::drain()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk, "WorldChunkLifecycle::drainPendingPostProcess");
    drainChunkCompletions(
        m_pendingPostProcess, m_pendingPostProcessMutex, std::chrono::milliseconds(2), [this](auto& item) {
            if (m_registry.findShared(item.x, item.z) != item.holder || item.holder == nullptr) {
                return;
            }
            const u64 key = posToKey(item.x, item.z);
            {
                std::lock_guard<std::mutex> lock(m_pendingPostProcessMutex);
                if (!m_postProcessedChunks.insert(key).second) {
                    return; // 已处理，丢弃重复项（spawnedEntities 随 item 析构，不 spawn）
                }
            }
            if (m_world) {
                m_world->onChunkLoaded(item.x, item.z);
            }
            if (m_chunkLoadedCallback) {
                m_chunkLoadedCallback(item.x, item.z);
            }
            if (!item.spawnedEntities.empty() && m_world) {
                m_world->spawnEntitiesFromChunkGeneration(item.spawnedEntities);
            }
            if (item.needsPostProcess && m_world) {
                std::lock_guard<std::mutex> lock(m_pendingPostProcessMutex);
                m_pendingPostProcessChunks.insert(key);
            }
        });
}

void WorldChunkLifecycle::postProcessTicking()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk, "WorldChunkLifecycle::processTickingChunkPostProcess");
    {
        std::lock_guard<std::mutex> lock(m_pendingPostProcessMutex);
        if (m_pendingPostProcessChunks.empty()) {
            return;
        }
    }
    const i32 simulationDistance = m_world->entityManager().simulationDistance();
    std::vector<u64> completed;
    _forEachTicking([this, simulationDistance, &completed](ChunkData& chunk) {
        const u64 key = posToKey(chunk.x(), chunk.z());
        {
            std::lock_guard<std::mutex> lock(m_pendingPostProcessMutex);
            if (m_pendingPostProcessChunks.count(key) == 0) {
                return true;
            }
        }
        if (!m_ticketManager.isTickingChunk(chunk.x(), chunk.z(), simulationDistance)) {
            return true;
        }
        if (!chunk.isPostProcessingDone()) {
            _postProcessChunk(chunk);
            chunk.setPostProcessingDone(true);
        }
        completed.push_back(key);
        return true;
    });

    if (!completed.empty()) {
        std::lock_guard<std::mutex> lock(m_pendingPostProcessMutex);
        for (const u64 key : completed) {
            m_pendingPostProcessChunks.erase(key);
        }
    }
}

void WorldChunkLifecycle::notifyUnload(ChunkCoord x, ChunkCoord z)
{
    if (m_world) {
        m_world->onChunkUnloading(x, z);
    }
    if (m_chunkUnloadedCallback) {
        m_chunkUnloadedCallback(x, z);
    }
}

void WorldChunkLifecycle::incrementInhabitedTime()
{
    for (auto& data : m_registry.chunks()) {
        auto& chunk = *data;
        const u64 key = ChunkPos(chunk.x(), chunk.z()).toId();
        if (m_ticketManager.hasTrackingPlayers(key)) {
            chunk.incrementInhabitedTime(1);
        }
    }
}

void WorldChunkLifecycle::_forEachTicking(const std::function<bool(ChunkData&)>& callback)
{
    MC_ASSERT_RELEASE(m_world != nullptr);
    for (auto& chunk : m_registry.chunks()) {
        if (m_ticketManager.isTickingChunk(chunk->x(), chunk->z(), m_world->entityManager().simulationDistance()) &&
            !callback(*chunk)) {
            break;
        }
    }
}

void WorldChunkLifecycle::forget(ChunkCoord x, ChunkCoord z)
{
    std::lock_guard lock(m_pendingPostProcessMutex);
    auto key = posToKey(x, z);
    m_postProcessedChunks.erase(key);
    m_pendingPostProcessChunks.erase(key);
}

void WorldChunkLifecycle::clear()
{
    std::lock_guard lock(m_pendingPostProcessMutex);
    m_pendingPostProcess.clear();
    m_postProcessedChunks.clear();
    m_pendingPostProcessChunks.clear();
}

bool WorldChunkLifecycle::pending() const
{
    std::lock_guard lock(m_pendingPostProcessMutex);
    return !m_pendingPostProcess.empty();
}

DimensionId WorldChunkLifecycle::dimension() const
{
    return m_world ? m_world->dimension() : 0;
}

void WorldChunkLifecycle::configureRegion(WorldGenRegion& region) const
{
    if (m_world) {
        region.setCurrentTick(m_world->currentTick());
        region.setDayTime(m_world->dayTime());
        region.setHardcore(m_world->isHardcore());
        region.setDifficulty(m_world->difficulty());
    }
}

void WorldChunkLifecycle::deliverStandaloneEntities(const std::vector<SpawnedEntityData>& entities)
{
    if (m_entitySpawnCallback && !entities.empty()) {
        m_entitySpawnCallback(entities);
    }
}

void WorldChunkLifecycle::lightGeneration(ChunkPrimer& chunk, WorldGenRegion& region)
{
    ServerWorld* world = m_world;
    WorldLightManager* lightManager = (world != nullptr) ? world->lightManager() : nullptr;
    if (lightManager == nullptr) {
        return;
    }

    ChunkData* chunkData = chunk.getChunkData();
    const ChunkCoord chunkX = chunk.x();
    const ChunkCoord chunkZ = chunk.z();
    const auto sectionsArr = chunkData->getSections();
    constexpr i32 sectionCount = world::CHUNK_SECTIONS;
    ChunkLightingProvider provider(*world, region, chunkX, chunkZ);

    chunkData->setLightCorrect(false);

    if (lightManager->hasBlockLight()) {
        auto* blockEngine = WorldLightManager::acquireBlockLightEngine();
        blockEngine->updateEmptinessMap(chunkX, chunkZ, chunkData);
        for (i32 sectionY = 0; sectionY < sectionCount; ++sectionY) {
            const ChunkSection* section = sectionsArr[static_cast<size_t>(sectionY)];
            const SectionPos sectionPos(chunkX, world::sectionIndexToCoord(sectionY), chunkZ);
            blockEngine->updateSectionStatus(sectionPos, section == nullptr || section->isEmpty());
        }
        blockEngine->light(&provider, chunkData, /*needsEdgeChecks=*/true);
        WorldLightManager::releaseBlockLightEngine(blockEngine);
    }

    if (lightManager->hasSkyLight()) {
        auto* skyEngine = WorldLightManager::acquireSkyLightEngine();
        for (i32 sectionY = 0; sectionY < sectionCount; ++sectionY) {
            const ChunkSection* section = sectionsArr[static_cast<size_t>(sectionY)];
            const SectionPos sectionPos(chunkX, world::sectionIndexToCoord(sectionY), chunkZ);
            skyEngine->updateSectionStatus(sectionPos, section == nullptr || section->isEmpty());
        }
        skyEngine->light(&provider, chunkData, /*needsEdgeChecks=*/true);
        WorldLightManager::releaseSkyLightEngine(skyEngine);
    }

    chunkData->setLightCorrect(true);
}

} // namespace mc::server
