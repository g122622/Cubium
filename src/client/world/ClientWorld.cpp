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

#include "ClientWorld.hpp"
#include "../renderer/trident/chunk/ChunkMesher.hpp"
#include "../renderer/trident/particle/ParticleManager.hpp"
#include "../renderer/trident/particle/ParticleRegistry.hpp"
#include "../renderer/trident/particle/ParticleTypes.hpp"
#include "../renderer/trident/particle/data/EntityEffectParticleData.hpp"
#include "../renderer/trident/particle/particles/block/DiggingParticle.hpp"
#include "../renderer/trident/particle/particles/block/DustPillarParticle.hpp"
#include "../renderer/trident/particle/particles/block/ItemParticle.hpp"
#include "common/core/Constants.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/sync/ChunkSync.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/NibbleArray.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/frustum/Frustum.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/core/BlockEntityRegistry.hpp"
#include "common/world/chunk/base/SectionPos.hpp"
#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <spdlog/spdlog.h>

#undef BYTE_SIZE // Re-undef after includes which may re-define BYTE_SIZE

using namespace mc::trace;

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

// ============================================================================
// ClientChunk 实现
// ============================================================================

ClientChunk::ClientChunk()
    : m_memTrack(this)
{}

ClientChunk::~ClientChunk() = default; // 守卫自动发 free

ClientChunk::ClientChunk(ClientChunk&& other) noexcept
    : m_memTrack() // 默认构造为非活跃，body 中重绑定
    , chunkId(other.chunkId)
    , data(std::move(other.data))
    , solidMesh(std::move(other.solidMesh))
    , transparentMesh(std::move(other.transparentMesh))
    , hasMeshResult(other.hasMeshResult)
    , needsMeshUpdate(other.needsMeshUpdate)
    , meshRebuildPending(other.meshRebuildPending)
    , isLoaded(other.isLoaded)
    , activeMeshTaskId(other.activeMeshTaskId)
{
    // 对象级追踪重绑定：释放源地址、分配目标地址（守卫不可移动，故在 body 处理，
    // 初始化列表中默认构造为非活跃）。若不重绑定，move 后源地址仍留在 Tracy 活跃集，
    // 堆复用该地址时触发 MemAllocTwice 硬失败。
    other.m_memTrack.unbind();
    m_memTrack.bind(this);
}

ClientChunk& ClientChunk::operator=(ClientChunk&& other) noexcept
{
    if (this != &other) {
        chunkId = other.chunkId;
        data = std::move(other.data);
        solidMesh = std::move(other.solidMesh);
        transparentMesh = std::move(other.transparentMesh);
        hasMeshResult = other.hasMeshResult;
        needsMeshUpdate = other.needsMeshUpdate;
        meshRebuildPending = other.meshRebuildPending;
        isLoaded = other.isLoaded;
        activeMeshTaskId = other.activeMeshTaskId;

        // 对象级追踪重绑定（同 move ctor 语义）：释放双方旧地址、目标重新绑定新地址
        m_memTrack.unbind();
        other.m_memTrack.unbind();
        m_memTrack.bind(this);
    }
    return *this;
}

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

    // 等待在途反序列化任务归零，防止 worker 回调访问已析构的 this。
    // 必须在 shutdownMeshSystem() 置空 m_computeWorkerPool 之前执行；池属 ClientApplication，此刻尚未关停。
    if (m_computeWorkerPool != nullptr) {
        m_computeWorkerPool->waitForCompletion();
    }

    shutdownMeshSystem();
    m_chunks.clear();

    spdlog::info("ClientWorld destroyed");
    m_destroyed = true;
}

void ClientWorld::update(const MeshSchedulerViewState& viewState)
{
    // 先 drain worker 反序列化续延队列，再处理 mesh；保证 mesh 调度读到最新 ChunkData。
    _processPendingDeserializedChunks();

    m_cameraPosition = viewState.cameraPosition;
    m_renderDistance = viewState.renderDistanceChunks;
    m_minBuildHeight = viewState.minBuildHeight;
    m_maxBuildHeight = viewState.maxBuildHeight;

    // 更新视锥体
    m_frustum.extractFromMatrix(viewState.viewProjectionMatrix);
    m_frustum.setCameraPosition(viewState.cameraPosition);

    if (m_meshBuildScheduler) {
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

        _scheduleVisibleChunksWithoutMesh(viewState, 8);
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
    const ChunkId id(chunkX, chunkZ, 0);

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
    const ChunkId id(chunkX, chunkZ, 0);

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
    const ChunkId id(chunkX, chunkZ, 0);

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
    const ChunkId id(chunkX, chunkZ, 0);

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
    MC_TRACE_INSTANT_EVENT(TraceEvents.Client.Lighting,
        "ClientWorld::setBlockState",
        "pos",
        fmt::format("({}, {}, {})", x, y, z),
        "stateId",
        state ? state->stateId() : 0,
        [flow = ::perfetto::Flow::ProcessScoped(pos.toId())](::perfetto::EventContext ctx) { flow(ctx); });

    const i32 chunkX = toChunkCoord(x);
    const i32 chunkZ = toChunkCoord(z);
    const ChunkId id(chunkX, chunkZ, 0);

    ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        return;
    }

    const i32 localX = toLocalCoord(x);
    const i32 localZ = toLocalCoord(z);

    chunk->data->setBlockState(localX, y, localZ, state);
    chunk->data->setDirty(true);
    chunk->hasMeshResult = false;

    _scheduleChunkMeshRebuild(id);

    if (localX == 0) {
        _scheduleChunkMeshRebuild(ChunkId(chunkX - 1, chunkZ, 0));
    }
    if (localX == CHUNK_WIDTH - 1) {
        _scheduleChunkMeshRebuild(ChunkId(chunkX + 1, chunkZ, 0));
    }
    if (localZ == 0) {
        _scheduleChunkMeshRebuild(ChunkId(chunkX, chunkZ - 1, 0));
    }
    if (localZ == CHUNK_WIDTH - 1) {
        _scheduleChunkMeshRebuild(ChunkId(chunkX, chunkZ + 1, 0));
    }
}

const ChunkData* ClientWorld::getChunkAt(ChunkCoord x, ChunkCoord z) const
{
    const ChunkId id(x, z, 0);
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
    const ChunkId id(chunkX, chunkZ, 0);

    const ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        return m_minBuildHeight;
    }

    const i32 localX = toLocalCoord(x);
    const i32 localZ = toLocalCoord(z);

    return chunk->data->getHighestBlock(localX, localZ);
}

i32 ClientWorld::getTopBlockY(world::chunk::HeightmapType type, i32 x, i32 z) const
{
    const ChunkCoord chunkX = toChunkCoord(x);
    const ChunkCoord chunkZ = toChunkCoord(z);
    const ChunkId id(chunkX, chunkZ, 0);

    const ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        return m_minBuildHeight;
    }

    const i32 localX = toLocalCoord(x);
    const i32 localZ = toLocalCoord(z);

    return chunk->data->getTopBlockY(type, localX, localZ);
}

bool ClientWorld::canSeeSky(const BlockPos& pos) const
{
    // 没有天空光照的维度（下界、末地）永远看不到天空
    if (!hasSkyLight()) {
        return false;
    }
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
    _scheduleChunkMeshRebuild(id);
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

void ClientWorld::_rebuildMesh(ClientChunk& chunk)
{
    if (!chunk.data) {
        return;
    }

    const ChunkData* neighbors[6] = {nullptr};
    _getNeighborChunks(chunk.chunkId, neighbors);

    ChunkMesher::generateSplitMesh(*chunk.data, chunk.solidMesh, chunk.transparentMesh, neighbors, nullptr);

    chunk.meshRebuildPending = false;
    chunk.hasMeshResult = true;
    chunk.needsMeshUpdate = true;
}

void ClientWorld::_scheduleChunkMeshRebuild(const ChunkId& id)
{
    ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data || !chunk->isLoaded) {
        return;
    }

    chunk->hasMeshResult = false;
    chunk->needsMeshUpdate = false;
    chunk->meshRebuildPending = false;

    if (m_meshBuildScheduler) {
        MeshBuildRequest request;
        request.chunkId = id;
        request.chunkData = chunk->data;
        request.neighbors = _getNeighborChunkData(id);

        const u64 taskId = m_meshBuildScheduler->submit(std::move(request));
        if (taskId != 0) {
            chunk->activeMeshTaskId = taskId;
            return;
        }
    }

    _rebuildMesh(*chunk);
    chunk->activeMeshTaskId = 0;
}

void ClientWorld::_requestChunkMeshRebuild(const ChunkId& id)
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

    _scheduleChunkMeshRebuild(id);
}

void ClientWorld::_scheduleNeighborMeshRebuild(const ChunkId& id)
{
    const ChunkId neighborIds[4] = {
        ChunkId(id.x - 1, id.z, 0), ChunkId(id.x + 1, id.z, 0), ChunkId(id.x, id.z - 1, 0), ChunkId(id.x, id.z + 1, 0)};

    for (const ChunkId& neighborId : neighborIds) {
        ClientChunk* neighbor = getChunk(neighborId);
        if (!neighbor || !neighbor->data || !neighbor->isLoaded) {
            continue;
        }

        _scheduleChunkMeshRebuild(neighborId);
    }
}

void ClientWorld::_scheduleVisibleChunksWithoutMesh(const MeshSchedulerViewState& viewState, u32 maxChunkCount)
{
    if (!m_meshBuildScheduler) {
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

        _scheduleChunkMeshRebuild(chunkId);
        ++scheduledCount;
    }
}

std::array<std::shared_ptr<const ChunkData>, 6> ClientWorld::_getNeighborChunkData(const ChunkId& id)
{
    std::array<std::shared_ptr<const ChunkData>, 6> neighbors;

    ClientChunk* neighbor = getChunk(ChunkId(id.x - 1, id.z, 0));
    neighbors[0] = (neighbor && neighbor->data) ? neighbor->data : nullptr;

    neighbor = getChunk(ChunkId(id.x + 1, id.z, 0));
    neighbors[1] = (neighbor && neighbor->data) ? neighbor->data : nullptr;

    neighbor = getChunk(ChunkId(id.x, id.z - 1, 0));
    neighbors[2] = (neighbor && neighbor->data) ? neighbor->data : nullptr;

    neighbor = getChunk(ChunkId(id.x, id.z + 1, 0));
    neighbors[3] = (neighbor && neighbor->data) ? neighbor->data : nullptr;

    neighbors[4] = nullptr;
    neighbors[5] = nullptr;

    return neighbors;
}

void ClientWorld::_getNeighborChunks(const ChunkId& id, const ChunkData* neighbors[6])
{
    ClientChunk* neighbor = getChunk(ChunkId(id.x - 1, id.z, 0));
    neighbors[0] = (neighbor && neighbor->data) ? neighbor->data.get() : nullptr;

    neighbor = getChunk(ChunkId(id.x + 1, id.z, 0));
    neighbors[1] = (neighbor && neighbor->data) ? neighbor->data.get() : nullptr;

    neighbor = getChunk(ChunkId(id.x, id.z - 1, 0));
    neighbors[2] = (neighbor && neighbor->data) ? neighbor->data.get() : nullptr;

    neighbor = getChunk(ChunkId(id.x, id.z + 1, 0));
    neighbors[3] = (neighbor && neighbor->data) ? neighbor->data.get() : nullptr;

    neighbors[4] = nullptr;
    neighbors[5] = nullptr;
}

void ClientWorld::onChunkData(ChunkCoord x,
    ChunkCoord z,
    DimensionId dimension,
    const u8* data,
    size_t size,
    std::shared_ptr<std::vector<u8>> buffer)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Network,
        "ClientWorld::onChunkData",
        "chunkX",
        x,
        "chunkZ",
        z,
        "dimension",
        static_cast<i32>(dimension),
        "dataSize",
        size);

    if (dimension != m_dimensionId) {
        spdlog::warn("Received chunk data for dimension {} but current dimension is {}, discarding chunk ({}, {})",
            static_cast<i32>(dimension),
            static_cast<i32>(m_dimensionId),
            x,
            z);
        return;
    }

    // worker 池未注入（测试/启动早期）：主线程同步反序列化。
    // buffer 此刻仍在栈上保活，直接传指针即可。
    if (m_computeWorkerPool == nullptr) {
        _applyDeserializedChunk(x, z, data, size);
        return;
    }

    // 异步反序列化：payload 以原始网络缓冲的视图下沉 worker，由 shared_ptr buffer 保活到 worker 完成。
    // worker 产出全新 ChunkData（不读客户端现有数据，无共享写竞争，完全并行）。
    // 代际号用于丢弃过期结果（同坐标重发包时旧任务的结果不应覆盖新数据）。
    const ChunkId id(x, z, 0);
    const u64 generation = ++m_chunkDeserializeGeneration[id];
    auto task = std::make_unique<util::FunctionTask>(
        util::TaskType::Custom,
        fmt::format("DeserializeChunk({},{})", x, z),
        [this, x, z, id, generation, dataPtr = data, dataSize = size, buffer = std::move(buffer)](
            const std::atomic<bool>& abortSignal) -> bool {
            // 任务可能被取消（维度切换/关服），检查后安全跳过。
            if (abortSignal.load(std::memory_order::acquire)) {
                return false;
            }
            auto result = network::ChunkSerializer::deserializeChunk(x, z, dataPtr, dataSize);
            if (result.failed()) {
                spdlog::error("Failed to deserialize chunk ({}, {}): {}", x, z, result.error().message());
                return true;
            }
            PendingDeserializedChunk pending;
            pending.id = id;
            pending.generation = generation;
            pending.data = std::shared_ptr<ChunkData>(result.value());
            {
                std::lock_guard<std::mutex> lock(m_pendingChunksMutex);
                m_pendingDeserializedChunks.push_back(std::move(pending));
            }
            return true;
        },
        "client_chunk_deserialize");

    m_computeWorkerPool->submit(std::move(task), /*callback=*/nullptr, util::TaskPriority::Normal);
}

void ClientWorld::_applyDeserializedChunk(ChunkCoord x, ChunkCoord z, const u8* data, size_t size)
{
    auto result = network::ChunkSerializer::deserializeChunk(x, z, data, size);
    if (result.failed()) {
        spdlog::error("Failed to deserialize chunk ({}, {}): {}", x, z, result.error().message());
        return;
    }
    const ChunkId id(x, z, 0);
    _applyChunkData(id, std::shared_ptr<ChunkData>(result.value()));
}

void ClientWorld::_applyChunkData(const ChunkId& id, std::shared_ptr<ChunkData> chunkData)
{
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

    _scheduleChunkMeshRebuild(id);
    _scheduleNeighborMeshRebuild(id);
}

void ClientWorld::_processPendingDeserializedChunks()
{
    std::vector<PendingDeserializedChunk> pending;
    {
        std::lock_guard<std::mutex> lock(m_pendingChunksMutex);
        pending.swap(m_pendingDeserializedChunks);
    }

    for (auto& item : pending) {
        // 代际过滤：同坐标若已有更新的反序列化提交，丢弃本次过期结果。
        auto genIt = m_chunkDeserializeGeneration.find(item.id);
        if (genIt == m_chunkDeserializeGeneration.end() || genIt->second != item.generation) {
            continue;
        }
        _applyChunkData(item.id, std::move(item.data));
    }
}

void ClientWorld::onChunkUnload(ChunkCoord x, ChunkCoord z, DimensionId dimension)
{
    if (dimension != m_dimensionId) {
        spdlog::warn("Received chunk unload for dimension {} but current dimension is {}, discarding chunk ({}, {})",
            static_cast<i32>(dimension),
            static_cast<i32>(m_dimensionId),
            x,
            z);
        return;
    }

    const ChunkId id(x, z, 0);

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
    m_chunkDeserializeGeneration.erase(id);
    ++m_chunksUnloaded;
}

void ClientWorld::clearChunks()
{
    // 1. 取消所有待处理的网格构建任务
    if (m_meshBuildScheduler) {
        m_meshBuildScheduler->cancelAll();
    }

    // 2. 调用区块卸载回调通知渲染器（移除 GPU 缓冲区）
    if (m_chunkUnloadCallback) {
        for (const auto& [chunkId, chunk] : m_chunks) {
            (void)chunk; // 未使用
            m_chunkUnloadCallback(chunkId);
        }
    }

    // 3. 使所有生物群系颜色缓存失效
    for (const auto& [chunkId, chunk] : m_chunks) {
        (void)chunk; // 未使用
        ChunkMesher::invalidateBiomeColorCache(chunkId.x, chunkId.z);
    }

    // 4. 清空区块映射
    m_chunks.clear();
    m_chunkDeserializeGeneration.clear();

    // 5. 清空方块实体（维度切换时旧维度的方块实体不再有效）
    clearBlockEntities();

    spdlog::info("[ClientWorld] Cleared all chunks for dimension change");
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

void ClientWorld::initializeMeshSystem(util::UniversalWorkerPool& workerPool,
    std::shared_ptr<MeshDataPool> dataPool,
    std::shared_ptr<MeshResultQueue> resultQueue,
    const MeshSchedulerConfig& schedulerConfig)
{
    if (m_meshBuildScheduler) {
        spdlog::warn("ClientWorld: mesh system already initialized");
        return;
    }

    // 池由 ClientApplication 持有并已 start()，这里仅存注入指针与共享对象，构造 scheduler。
    m_computeWorkerPool = &workerPool;
    m_meshDataPool = std::move(dataPool);
    m_meshResultQueue = std::move(resultQueue);

    m_meshBuildScheduler =
        std::make_unique<MeshBuildScheduler>(workerPool, m_meshDataPool, m_meshResultQueue, schedulerConfig);

    spdlog::info("ClientWorld: mesh system initialized with {} compute threads", workerPool.threadCount());

    for (auto& [chunkId, chunk] : m_chunks) {
        if (!chunk || !chunk->data || !chunk->isLoaded) {
            continue;
        }
        _scheduleChunkMeshRebuild(chunkId);
    }
}

void ClientWorld::shutdownMeshSystem()
{
    // scheduler::shutdown 取消所有任务并等待在途归零，之后析构 scheduler。
    // 不关池——池属 ClientApplication，其生命周期长于 ClientWorld（会话级）。
    if (m_meshBuildScheduler) {
        m_meshBuildScheduler->shutdown();
        m_meshBuildScheduler.reset();
    }

    m_computeWorkerPool = nullptr;
    m_meshDataPool.reset();
    m_meshResultQueue.reset();

    for (auto& [chunkId, chunk] : m_chunks) {
        (void)chunkId;
        if (chunk) {
            chunk->activeMeshTaskId = 0;
        }
    }
}

void ClientWorld::processMeshBuildResults(u32 maxPerFrame)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.ChunkMesh, "ClientWorld::processMeshBuildResults");

    if (!m_meshBuildScheduler) {
        return;
    }

    const u32 completedBacklog = static_cast<u32>(m_meshResultQueue->size());
    const u32 dynamicBudget = std::max(maxPerFrame, std::min(completedBacklog, 64u));

    m_meshBuildScheduler->drainCompleted(
        [this](MeshWorkerResult&& result) {
            ClientChunk* chunk = getChunk(result.chunkId);
            if (!chunk) {
                // chunk 已卸载：结果网格未进入 chunk，归还 capacity 给回收池。
                m_meshDataPool->recycle(false, std::move(result.solidMesh));
                m_meshDataPool->recycle(true, std::move(result.transparentMesh));
                return;
            }

            if (chunk->activeMeshTaskId != 0 && result.taskId != chunk->activeMeshTaskId) {
                // 过期结果（已被更新的任务取代）：归还 capacity 给回收池。
                m_meshDataPool->recycle(false, std::move(result.solidMesh));
                m_meshDataPool->recycle(true, std::move(result.transparentMesh));
                return;
            }

            const bool shouldResubmit = chunk->meshRebuildPending;

            if (result.cancelled || !result.success) {
                chunk->meshRebuildPending = false;
                chunk->activeMeshTaskId = 0;
                // 取消/失败：结果网格未进入 chunk，归还 capacity（取消时空壳归还最有价值）。
                m_meshDataPool->recycle(false, std::move(result.solidMesh));
                m_meshDataPool->recycle(true, std::move(result.transparentMesh));
                if (shouldResubmit) {
                    _requestChunkMeshRebuild(result.chunkId);
                }
                return;
            }

            if (shouldResubmit) {
                chunk->meshRebuildPending = false;
                chunk->activeMeshTaskId = 0;
                _requestChunkMeshRebuild(result.chunkId);
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

void ClientWorld::resetWeather()
{
    m_weather.reset();
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
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Lighting,
        "ClientWorld::onLightUpdate",
        "Section",
        fmt::format("({}, {}, {})", chunkX, sectionY, chunkZ),
        "SkyLightSize",
        skyLight.size(),
        "BlockLightSize",
        blockLight.size(),
        [flow = ::perfetto::Flow::ProcessScoped(SectionPos(chunkX, sectionY, chunkZ).toLong())](
            ::perfetto::EventContext ctx) { flow(ctx); });

    const ChunkId id(chunkX, chunkZ, 0);
    ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        return;
    }

    ChunkSection* section = chunk->data->getSection(world::sectionCoordToIndex(sectionY));
    if (!section) {
        return;
    }

    if (!skyLight.empty() && skyLight.size() == NibbleArray::BYTE_SIZE) {
        section->skyLightNibble() = NibbleArray(skyLight);
    }

    if (!blockLight.empty() && blockLight.size() == NibbleArray::BYTE_SIZE) {
        section->blockLightNibble() = NibbleArray(blockLight);
    }

    _requestChunkMeshRebuild(id);
}

// ========== 出生点 ==========

void ClientWorld::setSpawnPoint(i32 x, i32 y, i32 z, f32 angle)
{
    m_spawnPoint = BlockPos(x, y, z);
    m_spawnAngle = angle;
    spdlog::info("World spawn point set to ({}, {}, {}) angle={:.1f}", x, y, z, angle);
}

// ========== 粒子接口实现 ==========

void ClientWorld::addParticle(::mc::particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity)
{
    if (!m_particleManager) {
        return;
    }

    // 粒子质量过滤：根据 ParticleMode 设置决定是否接受该粒子
    if (!m_particleManager->shouldShowParticle(type)) {
        return;
    }

    auto particle = renderer::trident::particle::ParticleRegistry::instance().createParticle(
        type, glm::vec3(pos.x, pos.y, pos.z), glm::vec3(velocity.x, velocity.y, velocity.z), this);

    if (particle) {
        m_particleManager->addParticle(std::move(particle));
    }
}

void ClientWorld::addParticle(
    ::mc::particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count)
{
    if (!m_particleManager) {
        return;
    }

    // 粒子质量过滤：根据 ParticleMode 设置决定是否接受该粒子
    if (!m_particleManager->shouldShowParticle(type)) {
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

void ClientWorld::addBlockParticle(
    ::mc::particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const BlockState& blockState)
{
    if (!m_particleManager) {
        return;
    }

    // 粒子质量过滤：根据 ParticleMode 设置决定是否接受该粒子
    if (!m_particleManager->shouldShowParticle(type)) {
        return;
    }

    using namespace renderer::trident::particle;

    glm::vec3 glmPos(pos.x, pos.y, pos.z);
    glm::vec3 glmVel(velocity.x, velocity.y, velocity.z);

    if (type == ParticleTypeId::DustPillar) {
        // DustPillar 粒子：使用 createWithBlock 以传递方块状态
        auto particle = particles::DustPillarParticle::createWithBlock(glmPos, glmVel, blockState);
        if (particle) {
            m_particleManager->addParticle(std::move(particle));
        }
    } else if (type == ParticleTypeId::Block || type == ParticleTypeId::BlockMarker ||
        type == ParticleTypeId::Breaking || type == ParticleTypeId::FallingDust ||
        type == ParticleTypeId::BlockCrumble) {
        // 其他方块粒子：使用 DiggingParticle::createWithBlock
        auto particle = particles::DiggingParticle::createWithBlock(glmPos, glmVel, blockState);
        if (particle) {
            m_particleManager->addParticle(std::move(particle));
        }
    }
    // 其他粒子类型暂时不支持方块状态，忽略
}

void ClientWorld::addItemParticle(
    ::mc::particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const ::mc::ItemStack& itemStack)
{
    if (!m_particleManager) {
        return;
    }

    // 粒子质量过滤：根据 ParticleMode 设置决定是否接受该粒子
    if (!m_particleManager->shouldShowParticle(type)) {
        return;
    }

    using namespace renderer::trident::particle;

    glm::vec3 glmPos(pos.x, pos.y, pos.z);
    glm::vec3 glmVel(velocity.x, velocity.y, velocity.z);

    // 物品粒子：使用 ItemParticle::createWithItemStack 传递物品堆
    if (type == ParticleTypeId::Item || type == ParticleTypeId::ItemSlime || type == ParticleTypeId::ItemCobweb ||
        type == ParticleTypeId::ItemSnowball) {
        auto particle = particles::ItemParticle::createWithItemStack(glmPos, glmVel, itemStack);
        if (particle) {
            m_particleManager->addParticle(std::move(particle));
        }
    }
    // 其他粒子类型暂时不支持物品数据，忽略
}

void ClientWorld::addParticleWithData(::mc::particle::ParticleTypeId type,
    const Vector3& pos,
    const Vector3& velocity,
    std::unique_ptr<renderer::trident::particle::data::ParticleData> data)
{
    if (!m_particleManager || !data) {
        return;
    }

    glm::vec3 glmPos(pos.x, pos.y, pos.z);
    glm::vec3 glmVel(velocity.x, velocity.y, velocity.z);

    m_particleManager->addPendingParticle(type, glmPos, glmVel, this, std::move(data));
}

void ClientWorld::addEntityEffectParticle(
    const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count, u32 color)
{
    if (!m_particleManager) {
        return;
    }

    // 粒子质量过滤：根据 ParticleMode 设置决定是否接受该粒子
    if (!m_particleManager->shouldShowParticle(::mc::particle::ParticleTypeId::EntityEffect)) {
        return;
    }

    // 通过粒子数据管线创建带颜色的 EntityEffect 粒子
    // 对应 MC Java 的 ColorParticleOption.create(ParticleTypes.ENTITY_EFFECT, color)
    using namespace renderer::trident::particle;

    glm::vec3 glmPos(pos.x, pos.y, pos.z);
    glm::vec3 glmVel(velocity.x, velocity.y, velocity.z);

    math::Random rng;
    for (u32 i = 0; i < count; ++i) {
        // 在偏移范围内随机分布粒子位置（与 addParticle(count) 行为一致）
        glm::vec3 particlePos(pos.x + (i > 0 ? (rng.nextFloat() * 2.0f - 1.0f) * offset.x : 0.0f),
            pos.y + (i > 0 ? (rng.nextFloat() * 2.0f - 1.0f) * offset.y : 0.0f),
            pos.z + (i > 0 ? (rng.nextFloat() * 2.0f - 1.0f) * offset.z : 0.0f));

        auto effectData = std::make_unique<data::EntityEffectParticleData>(color);
        m_particleManager->addPendingParticle(
            ParticleTypeId::EntityEffect, particlePos, glmVel, this, std::move(effectData));
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

// ========== IBlockAnimateContext 接口实现 ==========

void ClientWorld::playLocalSound(
    const ResourceLocation& soundEventId, sound::SoundCategory category, const Vector3& position, f32 volume, f32 pitch)
{
    if (m_playLocalSoundCallback) {
        m_playLocalSoundCallback(soundEventId, category, position, volume, pitch);
    }
}

// ========== 方块动画 tick ==========

void ClientWorld::animateTick(i32 playerX, i32 playerY, i32 playerZ)
{
    math::Random random;
    constexpr i32 animateTickIterations = 667;
    constexpr i32 nearRange = 16;
    constexpr i32 farRange = 32;

    for (i32 i = 0; i < animateTickIterations; ++i) {
        _doAnimateTick(playerX, playerY, playerZ, nearRange, random);
        _doAnimateTick(playerX, playerY, playerZ, farRange, random);
    }
}

void ClientWorld::_doAnimateTick(i32 centerX, i32 centerY, i32 centerZ, i32 range, math::Random& random)
{
    // 在玩家周围随机采样一个位置（三角分布近似）
    const i32 x = centerX + random.nextInt(range) - random.nextInt(range);
    const i32 y = centerY + random.nextInt(range) - random.nextInt(range);
    const i32 z = centerZ + random.nextInt(range) - random.nextInt(range);

    // 高度范围检查
    if (y < m_minBuildHeight || y >= m_maxBuildHeight) {
        return;
    }

    const BlockState* blockState = getBlockState(x, y, z);
    if (blockState == nullptr) {
        return;
    }

    // 调用方块的 animateTick（基类默认实现为空，只有覆写的方块会产生效果）
    const Block& block = blockState->getBlock();
    BlockPos pos(x, y, z);
    block.animateTick(*this, pos, *blockState, random);
}

// ========== 方块实体 ==========
//
// TODO: 渲染层集成待实现（数据同步层已完成）
//
// 本节实现了方块实体数据同步层：onBlockEntityData/getBlockEntity/removeBlockEntity/clearBlockEntities。
// 数据可在 m_blockEntities 中正确存储和更新，告示牌编辑器等 UI 可通过 getBlockEntity 读取文本。
//
// 但渲染层缺失，客户端不会在世界中渲染方块实体（告示牌文本、箱子开合、信标光束等）：
//   1. SignRenderer 尚未实现（参考 src/client/renderer/trident/blockentity/renderers/）
//   2. BlockEntityRendererDispatcher 尚未集成到 TridentEngine::render() 主渲染循环
//
// 详见 ClientWorld.hpp 中「方块实体」小节的 TODO 注释。

void ClientWorld::onBlockEntityData(const BlockPos& pos, BlockEntityType type, const nbt::CompoundTag& tag)
{
    const i64 key = pos.asLong();

    // 查找或创建对应的 BlockEntity
    auto it = m_blockEntities.find(key);
    if (it == m_blockEntities.end()) {
        // 通过注册表创建新实例
        auto entity = blockentity::BlockEntityRegistry::instance().create(type, pos);
        if (entity == nullptr) {
            spdlog::warn("ClientWorld: failed to create BlockEntity (type={}) at ({}, {}, {})",
                static_cast<u32>(type),
                pos.x,
                pos.y,
                pos.z);
            return;
        }
        it = m_blockEntities.emplace(key, std::move(entity)).first;
    }

    // 加载 NBT 数据更新状态（1.21.11 BlockEntityData 直接携 CompoundTag，无需字节反序列化）
    it->second->loadFromNBT(tag);
}

BlockEntity* ClientWorld::getBlockEntity(const BlockPos& pos)
{
    const i64 key = pos.asLong();
    auto it = m_blockEntities.find(key);
    return (it != m_blockEntities.end()) ? it->second.get() : nullptr;
}

const BlockEntity* ClientWorld::getBlockEntity(const BlockPos& pos) const
{
    const i64 key = pos.asLong();
    auto it = m_blockEntities.find(key);
    return (it != m_blockEntities.end()) ? it->second.get() : nullptr;
}

void ClientWorld::removeBlockEntity(const BlockPos& pos)
{
    const i64 key = pos.asLong();
    m_blockEntities.erase(key);
}

void ClientWorld::clearBlockEntities()
{
    m_blockEntities.clear();
}

} // namespace mc::client

#pragma pop_macro("BYTE_SIZE")
