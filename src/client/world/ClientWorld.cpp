// 在macOS系统头文件中，BYTE_SIZE被定义为宏，会与NibbleArray的静态常数冲突
// 使用pragma push_macro/pop_macro来暂时屏蔽系统宏
#pragma push_macro("BYTE_SIZE")
#undef BYTE_SIZE

#include "ClientWorld.hpp"
#include "../renderer/trident/chunk/ChunkMesher.hpp"
#include "../renderer/trident/particle/ParticleManager.hpp"
#include "../renderer/trident/particle/ParticleRegistry.hpp"
#include "../renderer/trident/particle/ParticleTypes.hpp"
#include "common/core/Constants.hpp"
#include "common/network/sync/ChunkSync.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/NibbleArray.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/frustum/Frustum.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <spdlog/spdlog.h>

#pragma pop_macro("BYTE_SIZE")

namespace mc::client {

using namespace mc::world;

namespace {

f32 chunkDistanceInChunks(const MeshSchedulerViewState& viewState, const ChunkId& chunkId)
{
    const f32 chunkCenterOffset = static_cast<f32>(world::CHUNK_WIDTH) * 0.5f;
    const f32 centerX = static_cast<f32>(chunkId.x * world::CHUNK_WIDTH) + chunkCenterOffset;
    const f32 centerZ = static_cast<f32>(chunkId.z * world::CHUNK_WIDTH) + chunkCenterOffset;

    const glm::vec2 delta(centerX - viewState.cameraPosition.x, centerZ - viewState.cameraPosition.z);
    const f32 distanceBlocks = glm::length(delta);
    return distanceBlocks / static_cast<f32>(world::CHUNK_WIDTH);
}

} // namespace

ClientWorld::ClientWorld() = default;

ClientWorld::~ClientWorld()
{
    destroy();
}

Result<void> ClientWorld::initialize(u64 seed)
{
    m_seed = seed;
    m_destroyed = false;
    spdlog::info("ClientWorld initialized with seed: {}", seed);
    return Result<void>::ok();
}

void ClientWorld::destroy()
{
    if (m_destroyed) {
        return;
    }

    shutdownMeshSystem();
    m_chunks.clear();

    spdlog::info("ClientWorld destroyed");
    m_destroyed = true;
}

void ClientWorld::update(const MeshSchedulerViewState& viewState)
{
    m_cameraPosition = viewState.cameraPosition;
    m_renderDistance = viewState.renderDistanceChunks;
    m_minBuildHeight = viewState.minBuildHeight;
    m_maxBuildHeight = viewState.maxBuildHeight;

    // 更新视锥体
    m_frustum.extractFromMatrix(viewState.viewProjectionMatrix);
    m_frustum.setCameraPosition(viewState.cameraPosition);

    if (m_meshBuildScheduler && m_meshWorkerPool && m_meshWorkerPool->isRunning()) {
        m_meshBuildScheduler->setViewState(viewState);
        m_meshBuildScheduler->tick();

        for (auto& [chunkId, chunk] : m_chunks) {
            (void)chunkId;
            if (!chunk || chunk->activeMeshTaskId == 0) {
                continue;
            }

            if (!m_meshBuildScheduler->isTaskTracked(chunk->activeMeshTaskId)) {
                chunk->activeMeshTaskId = 0;
            }
        }

        scheduleVisibleChunksWithoutMesh(viewState, 8);
    }
}

ClientChunk* ClientWorld::getChunk(const ChunkId& id)
{
    auto it = m_chunks.find(id);
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

const ClientChunk* ClientWorld::getChunk(const ChunkId& id) const
{
    auto it = m_chunks.find(id);
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

const BlockState* ClientWorld::getBlockState(i32 x, i32 y, i32 z) const
{
    if (!isValidY(y)) {
        return nullptr;
    }

    const i32 chunkX = toChunkCoord(x);
    const i32 chunkZ = toChunkCoord(z);
    const ChunkId id(chunkX, chunkZ);

    const ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        return nullptr;
    }

    const i32 localX = toLocalCoord(x);
    const i32 localZ = toLocalCoord(z);

    return chunk->data->getBlockState(localX, y, localZ);
}

const Biome* ClientWorld::getBiomeAtBlock(i32 x, i32 y, i32 z) const
{
    if (!isValidY(y)) {
        return nullptr;
    }

    const i32 chunkX = toChunkCoord(x);
    const i32 chunkZ = toChunkCoord(z);
    const ChunkId id(chunkX, chunkZ);

    const ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        return nullptr;
    }

    const i32 localX = toLocalCoord(x);
    const i32 localZ = toLocalCoord(z);
    const BiomeId biomeId = chunk->data->getBiomeAtBlock(localX, y, localZ);

    return &BiomeRegistry::instance().get(biomeId);
}

u8 ClientWorld::getSkyLight(i32 x, i32 y, i32 z) const
{
    if (!isValidY(y)) {
        return 15;
    }

    const i32 chunkX = toChunkCoord(x);
    const i32 chunkZ = toChunkCoord(z);
    const ChunkId id(chunkX, chunkZ);

    const ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        return 15;
    }

    const i32 localX = toLocalCoord(x);
    const i32 localZ = toLocalCoord(z);

    const i32 sectionIndex = (y - m_minBuildHeight) / ChunkSection::SIZE;
    const ChunkSection* section = chunk->data->getSection(sectionIndex);
    if (!section) {
        return 15;
    }

    const i32 localY = y - m_minBuildHeight - sectionIndex * ChunkSection::SIZE;
    return section->getSkyLight(localX, localY, localZ);
}

u8 ClientWorld::getBlockLight(i32 x, i32 y, i32 z) const
{
    if (!isValidY(y)) {
        return 0;
    }

    const i32 chunkX = toChunkCoord(x);
    const i32 chunkZ = toChunkCoord(z);
    const ChunkId id(chunkX, chunkZ);

    const ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        return 0;
    }

    const i32 localX = toLocalCoord(x);
    const i32 localZ = toLocalCoord(z);

    const i32 sectionIndex = (y - m_minBuildHeight) / ChunkSection::SIZE;
    const ChunkSection* section = chunk->data->getSection(sectionIndex);
    if (!section) {
        return 0;
    }

    const i32 localY = y - m_minBuildHeight - sectionIndex * ChunkSection::SIZE;
    return section->getBlockLight(localX, localY, localZ);
}

void ClientWorld::setBlockState(i32 x, i32 y, i32 z, const BlockState* state)
{
    if (!isValidY(y)) {
        return;
    }

    const BlockPos pos(x, y, z);
    MC_TRACE_INSTANT("client.lighting",
        "ClientWorld::setBlockState",
        "pos",
        fmt::format("({}, {}, {})", x, y, z),
        "stateId",
        state ? state->stateId() : 0,
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    const i32 chunkX = toChunkCoord(x);
    const i32 chunkZ = toChunkCoord(z);
    const ChunkId id(chunkX, chunkZ);

    ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        return;
    }

    const i32 localX = toLocalCoord(x);
    const i32 localZ = toLocalCoord(z);

    chunk->data->setBlockState(localX, y, localZ, state);
    chunk->data->setDirty(true);
    chunk->hasMeshResult = false;

    scheduleChunkMeshRebuild(id);

    if (localX == 0) {
        scheduleChunkMeshRebuild(ChunkId(chunkX - 1, chunkZ));
    }
    if (localX == CHUNK_WIDTH - 1) {
        scheduleChunkMeshRebuild(ChunkId(chunkX + 1, chunkZ));
    }
    if (localZ == 0) {
        scheduleChunkMeshRebuild(ChunkId(chunkX, chunkZ - 1));
    }
    if (localZ == CHUNK_WIDTH - 1) {
        scheduleChunkMeshRebuild(ChunkId(chunkX, chunkZ + 1));
    }
}

const ChunkData* ClientWorld::getChunkAt(ChunkCoord x, ChunkCoord z) const
{
    const ChunkId id(x, z);
    const ClientChunk* chunk = getChunk(id);
    if (chunk && chunk->data) {
        return chunk->data.get();
    }
    return nullptr;
}

i32 ClientWorld::getHeight(i32 x, i32 z) const
{
    const ChunkCoord chunkX = toChunkCoord(x);
    const ChunkCoord chunkZ = toChunkCoord(z);
    const ChunkId id(chunkX, chunkZ);

    const ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        return m_minBuildHeight;
    }

    const i32 localX = toLocalCoord(x);
    const i32 localZ = toLocalCoord(z);

    return chunk->data->getHighestBlock(localX, localZ);
}

bool ClientWorld::canSeeSky(const BlockPos& pos) const
{
    // MC 1.16.5: return this.getLightFor(LightType.SKY, pos) >= this.getMaxLightLevel();
    // 基于天空光照判断，只有天空光照达到最大值 15 时才能看到天空
    return getSkyLight(pos.x, pos.y, pos.z) >= 15;
}

void ClientWorld::forEachChunk(std::function<void(const ChunkId&, ClientChunk&)> func)
{
    for (auto& [id, chunk] : m_chunks) {
        func(id, *chunk);
    }
}

void ClientWorld::forEachDirtyMesh(std::function<void(const ChunkId&, ClientChunk&)> func)
{
    for (auto& [id, chunk] : m_chunks) {
        if (chunk && chunk->needsMeshUpdate && chunk->isLoaded) {
            func(id, *chunk);
        }
    }
}

void ClientWorld::rebuildChunkMesh(const ChunkId& id)
{
    scheduleChunkMeshRebuild(id);
}

void ClientWorld::markChunkDirty(const ChunkId& id)
{
    ClientChunk* chunk = getChunk(id);
    if (!chunk) {
        return;
    }

    chunk->hasMeshResult = false;
    chunk->needsMeshUpdate = false;
    chunk->meshRebuildPending = true;
}

void ClientWorld::rebuildMesh(ClientChunk& chunk)
{
    if (!chunk.data) {
        return;
    }

    const ChunkData* neighbors[6] = {nullptr};
    getNeighborChunks(chunk.chunkId, neighbors);

    ChunkMesher::generateSplitMesh(*chunk.data, chunk.solidMesh, chunk.transparentMesh, neighbors, nullptr);

    chunk.meshRebuildPending = false;
    chunk.hasMeshResult = true;
    chunk.needsMeshUpdate = true;
}

void ClientWorld::scheduleChunkMeshRebuild(const ChunkId& id)
{
    ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data || !chunk->isLoaded) {
        return;
    }

    chunk->hasMeshResult = false;
    chunk->needsMeshUpdate = false;
    chunk->meshRebuildPending = false;

    if (m_meshBuildScheduler && m_meshWorkerPool && m_meshWorkerPool->isRunning()) {
        MeshBuildRequest request;
        request.chunkId = id;
        request.chunkData = chunk->data;
        request.neighbors = getNeighborChunkData(id);

        const u64 taskId = m_meshBuildScheduler->submit(std::move(request));
        if (taskId != 0) {
            chunk->activeMeshTaskId = taskId;
            return;
        }
    }

    rebuildMesh(*chunk);
    chunk->activeMeshTaskId = 0;
}

void ClientWorld::requestChunkMeshRebuild(const ChunkId& id)
{
    ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data || !chunk->isLoaded) {
        return;
    }

    chunk->hasMeshResult = false;
    chunk->needsMeshUpdate = false;
    chunk->meshRebuildPending = true;

    if (chunk->activeMeshTaskId != 0) {
        return;
    }

    scheduleChunkMeshRebuild(id);
}

void ClientWorld::scheduleNeighborMeshRebuild(const ChunkId& id)
{
    const ChunkId neighborIds[4] = {
        ChunkId(id.x - 1, id.z), ChunkId(id.x + 1, id.z), ChunkId(id.x, id.z - 1), ChunkId(id.x, id.z + 1)};

    for (const ChunkId& neighborId : neighborIds) {
        ClientChunk* neighbor = getChunk(neighborId);
        if (!neighbor || !neighbor->data || !neighbor->isLoaded) {
            continue;
        }

        scheduleChunkMeshRebuild(neighborId);
    }
}

void ClientWorld::scheduleVisibleChunksWithoutMesh(const MeshSchedulerViewState& viewState, u32 maxChunkCount)
{
    if (!m_meshBuildScheduler || !m_meshWorkerPool || !m_meshWorkerPool->isRunning()) {
        return;
    }

    if (maxChunkCount == 0) {
        return;
    }

    const f32 maxDistanceChunks = static_cast<f32>(viewState.renderDistanceChunks) + 1.0f;

    u32 scheduledCount = 0;
    for (auto& [chunkId, chunk] : m_chunks) {
        if (scheduledCount >= maxChunkCount) {
            break;
        }

        if (!chunk || !chunk->data || !chunk->isLoaded) {
            continue;
        }

        if (chunk->hasMeshResult || chunk->activeMeshTaskId != 0) {
            continue;
        }

        const f32 distanceChunks = chunkDistanceInChunks(viewState, chunkId);
        if (distanceChunks > maxDistanceChunks) {
            continue;
        }

        // 使用 Frustum 类进行视锥剔除
        if (!m_frustum.isChunkVisible(chunkId.x, chunkId.z, viewState.minBuildHeight, viewState.maxBuildHeight)) {
            continue;
        }

        scheduleChunkMeshRebuild(chunkId);
        ++scheduledCount;
    }
}

std::array<std::shared_ptr<const ChunkData>, 6> ClientWorld::getNeighborChunkData(const ChunkId& id)
{
    std::array<std::shared_ptr<const ChunkData>, 6> neighbors;

    ClientChunk* neighbor = getChunk(ChunkId(id.x - 1, id.z));
    neighbors[0] = (neighbor && neighbor->data) ? neighbor->data : nullptr;

    neighbor = getChunk(ChunkId(id.x + 1, id.z));
    neighbors[1] = (neighbor && neighbor->data) ? neighbor->data : nullptr;

    neighbor = getChunk(ChunkId(id.x, id.z - 1));
    neighbors[2] = (neighbor && neighbor->data) ? neighbor->data : nullptr;

    neighbor = getChunk(ChunkId(id.x, id.z + 1));
    neighbors[3] = (neighbor && neighbor->data) ? neighbor->data : nullptr;

    neighbors[4] = nullptr;
    neighbors[5] = nullptr;

    return neighbors;
}

void ClientWorld::getNeighborChunks(const ChunkId& id, const ChunkData* neighbors[6])
{
    ClientChunk* neighbor = getChunk(ChunkId(id.x - 1, id.z));
    neighbors[0] = (neighbor && neighbor->data) ? neighbor->data.get() : nullptr;

    neighbor = getChunk(ChunkId(id.x + 1, id.z));
    neighbors[1] = (neighbor && neighbor->data) ? neighbor->data.get() : nullptr;

    neighbor = getChunk(ChunkId(id.x, id.z - 1));
    neighbors[2] = (neighbor && neighbor->data) ? neighbor->data.get() : nullptr;

    neighbor = getChunk(ChunkId(id.x, id.z + 1));
    neighbors[3] = (neighbor && neighbor->data) ? neighbor->data.get() : nullptr;

    neighbors[4] = nullptr;
    neighbors[5] = nullptr;
}

void ClientWorld::onChunkData(ChunkCoord x, ChunkCoord z, std::vector<u8>&& data)
{
    const ChunkId id(x, z);

    auto result = network::ChunkSerializer::deserializeChunk(x, z, data);
    if (result.failed()) {
        spdlog::error("Failed to deserialize chunk ({}, {}): {}", x, z, result.error().message());
        return;
    }

    auto chunkData = std::shared_ptr<ChunkData>(result.value());

    ClientChunk* chunk = getChunk(id);
    if (!chunk) {
        auto newChunk = std::make_unique<ClientChunk>();
        newChunk->chunkId = id;
        newChunk->data = chunkData;
        newChunk->isLoaded = true;
        newChunk->hasMeshResult = false;
        newChunk->needsMeshUpdate = false;

        chunk = newChunk.get();
        m_chunks[id] = std::move(newChunk);
        ++m_chunksLoaded;
    } else {
        chunk->data = chunkData;
        chunk->isLoaded = true;
        chunk->hasMeshResult = false;
        chunk->needsMeshUpdate = false;
        chunk->activeMeshTaskId = 0;
    }

    scheduleChunkMeshRebuild(id);
    scheduleNeighborMeshRebuild(id);
}

void ClientWorld::onChunkUnload(ChunkCoord x, ChunkCoord z)
{
    const ChunkId id(x, z);

    if (m_meshBuildScheduler) {
        m_meshBuildScheduler->cancelChunk(id);
    }

    auto it = m_chunks.find(id);
    if (it == m_chunks.end()) {
        return;
    }

    if (m_chunkUnloadCallback) {
        m_chunkUnloadCallback(id);
    }

    ChunkMesher::invalidateBiomeColorCache(x, z);

    m_chunks.erase(it);
    ++m_chunksUnloaded;
}

void ClientWorld::onTimeUpdate(i64 gameTime, i64 dayTime, bool daylightCycleEnabled)
{
    m_prevDayTime = m_dayTime;
    m_gameTime = gameTime;
    m_targetDayTime = dayTime;
    m_dayTime = dayTime;
    m_daylightCycleEnabled = daylightCycleEnabled;
}

f32 ClientWorld::getInterpolatedCelestialAngle(f32 partialTick) const
{
    i64 dayTimeForInterp = m_dayTime;

    if (m_daylightCycleEnabled && m_prevDayTime != m_dayTime) {
        i64 diff = m_dayTime - m_prevDayTime;
        if (diff < 0) {
            diff += mc::game::DAY_LENGTH_TICKS;
        }
        dayTimeForInterp = m_prevDayTime + static_cast<i64>(diff * partialTick);
    }

    f32 d0 = std::fmod(static_cast<f32>(dayTimeForInterp) / static_cast<f32>(mc::game::DAY_LENGTH_TICKS) - 0.25f, 1.0f);
    if (d0 < 0.0f) {
        d0 += 1.0f;
    }
    const f32 d1 = 0.5f - std::cos(d0 * mc::math::PI) / 2.0f;

    return (d0 * 2.0f + d1) / 3.0f;
}

void ClientWorld::initializeMeshSystem(i32 threadCount, const MeshSchedulerConfig& schedulerConfig)
{
    if (m_meshWorkerPool) {
        spdlog::warn("ClientWorld: mesh system already initialized");
        return;
    }

    m_meshWorkerPool = std::make_unique<MeshWorkerPool>(threadCount);
    m_meshWorkerPool->start();

    m_meshBuildScheduler = std::make_unique<MeshBuildScheduler>(*m_meshWorkerPool, schedulerConfig);

    spdlog::info("ClientWorld: mesh system initialized with {} worker threads", m_meshWorkerPool->threadCount());

    for (auto& [chunkId, chunk] : m_chunks) {
        if (!chunk || !chunk->data || !chunk->isLoaded) {
            continue;
        }
        scheduleChunkMeshRebuild(chunkId);
    }
}

void ClientWorld::shutdownMeshSystem()
{
    if (m_meshBuildScheduler) {
        m_meshBuildScheduler->cancelAll();
        m_meshBuildScheduler.reset();
    }

    if (m_meshWorkerPool) {
        m_meshWorkerPool->shutdown();
        m_meshWorkerPool.reset();
    }

    for (auto& [chunkId, chunk] : m_chunks) {
        (void)chunkId;
        if (chunk) {
            chunk->activeMeshTaskId = 0;
        }
    }
}

void ClientWorld::processMeshBuildResults(u32 maxPerFrame)
{
    MC_TRACE_EVENT("rendering.chunk_mesh", "ClientWorld::processMeshBuildResults");

    if (!m_meshBuildScheduler || !m_meshWorkerPool || !m_meshWorkerPool->isRunning()) {
        return;
    }

    const u32 completedBacklog = static_cast<u32>(m_meshWorkerPool->completedTaskCount());
    const u32 dynamicBudget = std::max(maxPerFrame, std::min(completedBacklog, 64u));

    m_meshBuildScheduler->drainCompleted(
        [this](MeshWorkerResult&& result) {
            ClientChunk* chunk = getChunk(result.chunkId);
            if (!chunk) {
                return;
            }

            if (chunk->activeMeshTaskId != 0 && result.taskId != chunk->activeMeshTaskId) {
                return;
            }

            const bool shouldResubmit = chunk->meshRebuildPending;

            if (result.cancelled || !result.success) {
                chunk->meshRebuildPending = false;
                chunk->activeMeshTaskId = 0;
                if (shouldResubmit) {
                    requestChunkMeshRebuild(result.chunkId);
                }
                return;
            }

            if (shouldResubmit) {
                chunk->meshRebuildPending = false;
                chunk->activeMeshTaskId = 0;
                requestChunkMeshRebuild(result.chunkId);
                return;
            }

            chunk->solidMesh = std::move(result.solidMesh);
            chunk->transparentMesh = std::move(result.transparentMesh);
            chunk->meshRebuildPending = false;
            chunk->hasMeshResult = true;
            chunk->needsMeshUpdate = true;
            chunk->activeMeshTaskId = 0;
        },
        dynamicBudget);
}

void ClientWorld::onRainStrengthChange(f32 strength)
{
    m_weather.setRainStrength(strength);
}

void ClientWorld::onThunderStrengthChange(f32 strength)
{
    m_weather.setThunderStrength(strength);
}

void ClientWorld::onBeginRaining()
{
    m_weather.beginRain();
}

void ClientWorld::onEndRaining()
{
    m_weather.endRain();
}

void ClientWorld::onLightUpdate(i32 chunkX,
    i32 chunkZ,
    i32 sectionY,
    const std::vector<u8>& skyLight,
    const std::vector<u8>& blockLight,
    bool /*trustEdges*/
)
{
    MC_TRACE_EVENT("client.lighting",
        "ClientWorld::onLightUpdate",
        "Section",
        fmt::format("({}, {}, {})", chunkX, sectionY, chunkZ),
        "SkyLightSize",
        skyLight.size(),
        "BlockLightSize",
        blockLight.size(),
        [flow = ::perfetto::Flow::ProcessScoped(SectionPos(chunkX, sectionY, chunkZ).toLong())](
            ::perfetto::EventContext ctx) { flow(ctx); });

    const ChunkId id(chunkX, chunkZ);
    ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        return;
    }

    ChunkSection* section = chunk->data->getSection(sectionY);
    if (!section) {
        return;
    }

    if (!skyLight.empty() && skyLight.size() == NibbleArray::BYTE_SIZE) {
        section->skyLightNibble() = NibbleArray(skyLight);
    }

    if (!blockLight.empty() && blockLight.size() == NibbleArray::BYTE_SIZE) {
        section->blockLightNibble() = NibbleArray(blockLight);
    }

    requestChunkMeshRebuild(id);
}

// ========== 出生点 ==========

void ClientWorld::setSpawnPoint(i32 x, i32 y, i32 z, f32 angle)
{
    m_spawnPoint = BlockPos(x, y, z);
    m_spawnAngle = angle;
    spdlog::info("World spawn point set to ({}, {}, {}) angle={:.1f}", x, y, z, angle);
}

// ========== 粒子接口实现 ==========

void ClientWorld::addParticle(
    renderer::trident::particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity)
{
    if (!m_particleManager) {
        return;
    }

    auto particle = renderer::trident::particle::ParticleRegistry::instance().createParticle(
        type, glm::vec3(pos.x, pos.y, pos.z), glm::vec3(velocity.x, velocity.y, velocity.z), this);

    if (particle) {
        m_particleManager->addParticle(std::move(particle));
    }
}

void ClientWorld::addParticle(renderer::trident::particle::ParticleTypeId type,
    const Vector3& pos,
    const Vector3& velocity,
    const Vector3& offset,
    u32 count)
{
    if (!m_particleManager) {
        return;
    }

    math::Random rng;
    for (u32 i = 0; i < count; ++i) {
        glm::vec3 particlePos(pos.x + (i > 0 ? (rng.nextFloat() * 2.0f - 1.0f) * offset.x : 0.0f),
            pos.y + (i > 0 ? (rng.nextFloat() * 2.0f - 1.0f) * offset.y : 0.0f),
            pos.z + (i > 0 ? (rng.nextFloat() * 2.0f - 1.0f) * offset.z : 0.0f));
        glm::vec3 particleVel(velocity.x + (rng.nextFloat() * 2.0f - 1.0f) * 0.01f,
            velocity.y + (rng.nextFloat() * 2.0f - 1.0f) * 0.01f,
            velocity.z + (rng.nextFloat() * 2.0f - 1.0f) * 0.01f);

        auto particle = renderer::trident::particle::ParticleRegistry::instance().createParticle(
            type, particlePos, particleVel, this);

        if (particle) {
            m_particleManager->addParticle(std::move(particle));
        }
    }
}

bool ClientWorld::shouldSpawnParticleAt(const Vector3& pos, f32 maxDistance) const
{
    // 检查粒子位置到相机的距离
    const f32 dx = pos.x - m_cameraPosition.x;
    const f32 dy = pos.y - m_cameraPosition.y;
    const f32 dz = pos.z - m_cameraPosition.z;
    const f32 distSq = dx * dx + dy * dy + dz * dz;
    const f32 maxDistSq = maxDistance * maxDistance;
    return distSq <= maxDistSq;
}

} // namespace mc::client
