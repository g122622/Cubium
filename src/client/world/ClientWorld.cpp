#include "ClientWorld.hpp"
#include "../renderer/trident/chunk/ChunkMesher.hpp"
#include "../../common/world/WorldConstants.hpp"
#include "../../common/network/sync/ChunkSync.hpp"
#include "../../common/core/Constants.hpp"
#include "../../common/world/biome/BiomeRegistry.hpp"
#include "../../common/world/chunk/ChunkData.hpp"
#include "../../common/util/NibbleArray.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>

namespace mc::client {

using namespace mc::world;

ClientWorld::ClientWorld() = default;

ClientWorld::~ClientWorld() {
    destroy();
}

Result<void> ClientWorld::initialize(u64 seed) {
    m_seed = seed;
    m_destroyed = false;
    spdlog::info("ClientWorld initialized with seed: {}", seed);
    return Result<void>::ok();
}

void ClientWorld::destroy() {
    if (m_destroyed) {
        return;
    }

    // 先关闭网格构建线程池
    shutdownMeshWorkerPool();

    m_chunks.clear();

    while (!m_loadQueue.empty()) {
        m_loadQueue.pop();
    }
    m_queuedChunks.clear();

    spdlog::info("ClientWorld destroyed");
    m_destroyed = true;
}

void ClientWorld::update(const glm::vec3& cameraPosition, i32 renderDistance) {
    m_renderDistance = renderDistance;
    m_cameraPosition = cameraPosition;
    // 区块生命周期由服务端统一控制。
    // 客户端若自行按距离卸载，会与服务端的已发送集合产生状态漂移，
    // 导致回头后部分旧区块无法重新下发。
}

ClientChunk* ClientWorld::getChunk(const ChunkId& id) {
    auto it = m_chunks.find(id);
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

const ClientChunk* ClientWorld::getChunk(const ChunkId& id) const {
    auto it = m_chunks.find(id);
    if (it != m_chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

const BlockState* ClientWorld::getBlockState(i32 x, i32 y, i32 z) const {
    // 检查Y范围
    if (!isValidY(y)) {
        return nullptr;
    }

    // 获取区块
    i32 chunkX = toChunkCoord(x);
    i32 chunkZ = toChunkCoord(z);
    ChunkId id(chunkX, chunkZ);

    const ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        return nullptr;
    }

    // 转换为本地坐标
    i32 localX = toLocalCoord(x);
    i32 localZ = toLocalCoord(z);

    return chunk->data->getBlock(localX, y, localZ);
}

const Biome* ClientWorld::getBiomeAtBlock(i32 x, i32 y, i32 z) const {
    if (!isValidY(y)) {
        return nullptr;
    }

    i32 chunkX = toChunkCoord(x);
    i32 chunkZ = toChunkCoord(z);
    ChunkId id(chunkX, chunkZ);

    const ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        return nullptr;
    }

    const i32 localX = toLocalCoord(x);
    const i32 localZ = toLocalCoord(z);
    const BiomeId biomeId = chunk->data->getBiomeAtBlock(localX, y, localZ);

    return &BiomeRegistry::instance().get(biomeId);
}

u8 ClientWorld::getSkyLight(i32 x, i32 y, i32 z) const {
    // 检查Y范围
    if (!isValidY(y)) {
        return 15;  // 世界范围外返回最大天空光照
    }

    // 获取区块
    i32 chunkX = toChunkCoord(x);
    i32 chunkZ = toChunkCoord(z);
    ChunkId id(chunkX, chunkZ);

    const ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        return 15;  // 区块未加载返回最大天空光照
    }

    // 转换为本地坐标
    i32 localX = toLocalCoord(x);
    i32 localZ = toLocalCoord(z);

    // 计算区块段索引
    i32 sectionIndex = (y - m_minBuildHeight) / ChunkSection::SIZE;
    const ChunkSection* section = chunk->data->getSection(sectionIndex);
    if (!section) {
        return 15;  // 区块段不存在返回最大天空光照
    }

    // 计算段内Y坐标
    i32 localY = y - m_minBuildHeight - sectionIndex * ChunkSection::SIZE;
    return section->getSkyLight(localX, localY, localZ);
}

u8 ClientWorld::getBlockLight(i32 x, i32 y, i32 z) const {
    // 检查Y范围
    if (!isValidY(y)) {
        return 0;  // 世界范围外返回无方块光照
    }

    // 获取区块
    i32 chunkX = toChunkCoord(x);
    i32 chunkZ = toChunkCoord(z);
    ChunkId id(chunkX, chunkZ);

    const ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        return 0;  // 区块未加载返回无方块光照
    }

    // 转换为本地坐标
    i32 localX = toLocalCoord(x);
    i32 localZ = toLocalCoord(z);

    // 计算区块段索引
    i32 sectionIndex = (y - m_minBuildHeight) / ChunkSection::SIZE;
    const ChunkSection* section = chunk->data->getSection(sectionIndex);
    if (!section) {
        return 0;  // 区块段不存在返回无方块光照
    }

    // 计算段内Y坐标
    i32 localY = y - m_minBuildHeight - sectionIndex * ChunkSection::SIZE;
    return section->getBlockLight(localX, localY, localZ);
}

void ClientWorld::setBlock(i32 x, i32 y, i32 z, const BlockState* state) {
    // 检查Y范围
    if (!isValidY(y)) {
        return;
    }

    // 获取区块
    i32 chunkX = toChunkCoord(x);
    i32 chunkZ = toChunkCoord(z);
    ChunkId id(chunkX, chunkZ);

    ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        return;
    }

    // 转换为本地坐标
    i32 localX = toLocalCoord(x);
    i32 localZ = toLocalCoord(z);

    // spdlog::info("[Mining] ClientWorld setBlock pos=({}, {}, {}) state={} loadedChunk=({}, {})",
    //              x,
    //              y,
    //              z,
    //              state ? state->blockLocation().toString() : String("<null>"),
    //              chunkX,
    //              chunkZ);

    chunk->data->setBlock(localX, y, localZ, state);
    chunk->data->setDirty(true);
    scheduleChunkMeshRebuild(id);

    // 标记相邻区块也需要更新（如果方块在边界）
    if (localX == 0) scheduleChunkMeshRebuild(ChunkId(chunkX - 1, chunkZ));
    if (localX == CHUNK_WIDTH - 1) scheduleChunkMeshRebuild(ChunkId(chunkX + 1, chunkZ));
    if (localZ == 0) scheduleChunkMeshRebuild(ChunkId(chunkX, chunkZ - 1));
    if (localZ == CHUNK_WIDTH - 1) scheduleChunkMeshRebuild(ChunkId(chunkX, chunkZ + 1));
}

const ChunkData* ClientWorld::getChunkAt(ChunkCoord x, ChunkCoord z) const {
    ChunkId id(x, z);
    const ClientChunk* chunk = getChunk(id);
    if (chunk && chunk->data) {
        return chunk->data.get();
    }
    return nullptr;
}

void ClientWorld::forEachChunk(std::function<void(const ChunkId&, ClientChunk&)> func) {
    for (auto& [id, chunk] : m_chunks) {
        func(id, *chunk);
    }
}

void ClientWorld::forEachDirtyMesh(std::function<void(const ChunkId&, ClientChunk&)> func) {
    for (auto& [id, chunk] : m_chunks) {
        // 跳过正在构建网格的区块
        if (chunk && chunk->needsMeshUpdate && chunk->isLoaded && !chunk->meshBuilding) {
            func(id, *chunk);
        }
    }
}

void ClientWorld::getChunksInRange(const glm::vec3& position, i32 range,
                                    std::vector<ChunkId>& outChunks) const {
    i32 centerChunkX = toChunkCoord(static_cast<i32>(position.x));
    i32 centerChunkZ = toChunkCoord(static_cast<i32>(position.z));

    outChunks.clear();
    outChunks.reserve(static_cast<size_t>((2 * range + 1) * (2 * range + 1)));

    for (i32 dx = -range; dx <= range; ++dx) {
        for (i32 dz = -range; dz <= range; ++dz) {
            // 使用圆形范围
            f32 dist = std::sqrt(static_cast<f32>(dx * dx + dz * dz));
            if (dist <= static_cast<f32>(range)) {
                outChunks.emplace_back(centerChunkX + dx, centerChunkZ + dz);
            }
        }
    }
}

void ClientWorld::loadChunk(const ChunkId& id) {
    // 检查是否已加载
    if (m_chunks.find(id) != m_chunks.end()) {
        return;
    }

    // 添加到加载队列
    if (m_queuedChunks.find(id) == m_queuedChunks.end()) {
        m_loadQueue.push({id, ChunkLoadPriority::Normal});
        m_queuedChunks.insert(id);
    }
}

void ClientWorld::unloadChunk(const ChunkId& id, std::vector<ChunkId>* outUnloadedChunkIds) {
    auto it = m_chunks.find(id);
    if (it != m_chunks.end()) {
        // 调用卸载回调（通知 ChunkRenderer 释放 GPU 缓冲区）
        if (m_chunkUnloadCallback) {
            m_chunkUnloadCallback(id);
        }

        // 使生物群系颜色缓存失效
        ChunkMesher::invalidateBiomeColorCache(id.x, id.z);

        m_chunks.erase(it);
        m_chunksUnloaded++;
        spdlog::debug("Unloaded chunk ({}, {})", id.x, id.z);

        if (outUnloadedChunkIds) {
            outUnloadedChunkIds->push_back(id);
        }
    }
}

void ClientWorld::rebuildChunkMesh(const ChunkId& id) {
    ClientChunk* chunk = getChunk(id);
    if (chunk && chunk->data) {
        rebuildMesh(*chunk);
    }
}

void ClientWorld::markChunkDirty(const ChunkId& id) {
    ClientChunk* chunk = getChunk(id);
    if (chunk) {
        chunk->needsMeshUpdate = true;
    }
}

void ClientWorld::loadChunksInRange(const glm::vec3& position, i32 range) {
    std::vector<ChunkId> chunksToLoad;
    getChunksInRange(position, range, chunksToLoad);

    for (const auto& id : chunksToLoad) {
        // 检查是否已加载
        if (m_chunks.find(id) != m_chunks.end()) {
            continue;
        }

        // 添加到加载队列
        if (m_queuedChunks.find(id) == m_queuedChunks.end()) {
            world::ChunkLoadPriority priority = calculatePriority(id, position);
            m_loadQueue.push({id, priority});
            m_queuedChunks.insert(id);
        }
    }
}

void ClientWorld::unloadChunksOutOfRange(const glm::vec3& position, i32 range) {
    i32 centerChunkX = toChunkCoord(static_cast<i32>(position.x));
    i32 centerChunkZ = toChunkCoord(static_cast<i32>(position.z));

    std::vector<ChunkId> toRemove;
    for (const auto& [id, chunk] : m_chunks) {
        i32 dx = id.x - centerChunkX;
        i32 dz = id.z - centerChunkZ;
        f32 dist = std::sqrt(static_cast<f32>(dx * dx + dz * dz));

        if (dist > static_cast<f32>(range)) {
            toRemove.push_back(id);
        }
    }

    for (const auto& id : toRemove) {
        unloadChunk(id);
    }
}

void ClientWorld::processLoadQueue() {
    i32 loaded = 0;
    while (!m_loadQueue.empty() && loaded < m_maxChunksPerFrame) {
        ChunkLoadRequest request = m_loadQueue.top();
        m_loadQueue.pop();
        m_queuedChunks.erase(request.chunkId);

        // 跳过已加载的区块
        if (m_chunks.find(request.chunkId) != m_chunks.end()) {
            continue;
        }

        // 创建新区块
        auto chunk = std::make_unique<ClientChunk>();
        chunk->chunkId = request.chunkId;
        chunk->data = std::make_shared<ChunkData>(request.chunkId.x, request.chunkId.z);

        // 生成地形
        generateChunk(*chunk);

        // 构建网格
        rebuildMesh(*chunk);

        chunk->isLoaded = true;
        chunk->needsMeshUpdate = true;  // 标记需要上传到GPU
        chunk->meshBuilding = false;

        m_chunks[request.chunkId] = std::move(chunk);
        m_chunksLoaded++;
        loaded++;
    }
}

void ClientWorld::generateChunk(ClientChunk& chunk) {
    // 客户端不再本地生成地形
    // 区块数据从服务端接收，通过 onChunkData() 方法传入
    chunk.isGenerating = false;
}

void ClientWorld::rebuildMesh(ClientChunk& chunk) {
    if (!chunk.data) return;

    // 获取相邻区块（用于边界面的剔除）
    const ChunkData* neighbors[6] = {nullptr};

    // -X
    auto neighborXNeg = getChunk(ChunkId(chunk.chunkId.x - 1, chunk.chunkId.z));
    if (neighborXNeg && neighborXNeg->data) neighbors[0] = neighborXNeg->data.get();

    // +X
    auto neighborXPos = getChunk(ChunkId(chunk.chunkId.x + 1, chunk.chunkId.z));
    if (neighborXPos && neighborXPos->data) neighbors[1] = neighborXPos->data.get();

    // -Z
    auto neighborZNeg = getChunk(ChunkId(chunk.chunkId.x, chunk.chunkId.z - 1));
    if (neighborZNeg && neighborZNeg->data) neighbors[2] = neighborZNeg->data.get();

    // +Z
    auto neighborZPos = getChunk(ChunkId(chunk.chunkId.x, chunk.chunkId.z + 1));
    if (neighborZPos && neighborZPos->data) neighbors[3] = neighborZPos->data.get();

    // -Y 和 +Y 暂时不考虑（多区块高度）

    // 生成分层网格
    ChunkMesher::generateSplitMesh(*chunk.data, chunk.solidMesh, chunk.transparentMesh, neighbors);

    chunk.needsMeshUpdate = false;
}

void ClientWorld::scheduleChunkMeshRebuild(const ChunkId& id) {
    ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        return;
    }

    rebuildMesh(*chunk);
    chunk->needsMeshUpdate = true;
    chunk->meshBuilding = false;

    // spdlog::info("[Mining] Rebuilt chunk mesh synchronously for chunk ({}, {}), vertices={}, indices={}",
    //              id.x,
    //              id.z,
    //              chunk->solidMesh.vertexCount(),
    //              chunk->solidMesh.indexCount());
}

std::array<std::shared_ptr<const ChunkData>, 6> ClientWorld::getNeighborChunkData(const ChunkId& id) {
    std::array<std::shared_ptr<const ChunkData>, 6> neighbors;

    // -X
    auto neighbor = getChunk(ChunkId(id.x - 1, id.z));
    neighbors[0] = (neighbor && neighbor->data) ? neighbor->data : nullptr;

    // +X
    neighbor = getChunk(ChunkId(id.x + 1, id.z));
    neighbors[1] = (neighbor && neighbor->data) ? neighbor->data : nullptr;

    // -Z
    neighbor = getChunk(ChunkId(id.x, id.z - 1));
    neighbors[2] = (neighbor && neighbor->data) ? neighbor->data : nullptr;

    // +Z
    neighbor = getChunk(ChunkId(id.x, id.z + 1));
    neighbors[3] = (neighbor && neighbor->data) ? neighbor->data : nullptr;

    // -Y 和 +Y 暂时为空
    neighbors[4] = nullptr;
    neighbors[5] = nullptr;

    return neighbors;
}

void ClientWorld::getNeighborChunks(const ChunkId& id, const ChunkData* neighbors[6]) {
    // -X
    auto neighbor = getChunk(ChunkId(id.x - 1, id.z));
    neighbors[0] = (neighbor && neighbor->data) ? neighbor->data.get() : nullptr;

    // +X
    neighbor = getChunk(ChunkId(id.x + 1, id.z));
    neighbors[1] = (neighbor && neighbor->data) ? neighbor->data.get() : nullptr;

    // -Z
    neighbor = getChunk(ChunkId(id.x, id.z - 1));
    neighbors[2] = (neighbor && neighbor->data) ? neighbor->data.get() : nullptr;

    // +Z
    neighbor = getChunk(ChunkId(id.x, id.z + 1));
    neighbors[3] = (neighbor && neighbor->data) ? neighbor->data.get() : nullptr;

    // -Y 和 +Y
    neighbors[4] = nullptr;
    neighbors[5] = nullptr;
}

world::ChunkLoadPriority ClientWorld::calculatePriority(const ChunkId& id, const glm::vec3& cameraPos) const {
    i32 centerChunkX = toChunkCoord(static_cast<i32>(cameraPos.x));
    i32 centerChunkZ = toChunkCoord(static_cast<i32>(cameraPos.z));

    i32 dx = id.x - centerChunkX;
    i32 dz = id.z - centerChunkZ;
    f32 dist = std::sqrt(static_cast<f32>(dx * dx + dz * dz));

    if (dist < 1.0f) {
        return world::ChunkLoadPriority::Critical;
    } else if (dist < 3.0f) {
        return world::ChunkLoadPriority::High;
    } else if (dist < static_cast<f32>(m_renderDistance) * 0.5f) {
        return world::ChunkLoadPriority::Normal;
    } else {
        return world::ChunkLoadPriority::Low;
    }
}

void ClientWorld::onChunkData(ChunkCoord x, ChunkCoord z, std::vector<u8>&& data) {
    ChunkId id(x, z);

    // 反序列化区块数据
    auto result = network::ChunkSerializer::deserializeChunk(x, z, data);
    if (result.failed()) {
        spdlog::error("Failed to deserialize chunk ({}, {}): {}", x, z, result.error().message());
        return;
    }

    // 使用 shared_ptr 以支持异步网格构建
    auto chunkData = std::shared_ptr<ChunkData>(std::move(result.value()));

    auto chunk = std::make_unique<ClientChunk>();
    chunk->chunkId = id;
    chunk->data = chunkData;
    chunk->isLoaded = true;
    chunk->meshBuilding = true;  // 标记正在构建网格

    m_chunks[id] = std::move(chunk);
    m_chunksLoaded++;

    // 异步构建网格（如果线程池已初始化）
    if (m_meshWorkerPool && m_meshWorkerPool->isRunning()) {
        auto neighbors = getNeighborChunkData(id);
        i32 priority = static_cast<i32>(calculatePriority(id, m_cameraPosition));
        m_meshWorkerPool->submitTask(id, chunkData, neighbors, priority);
    } else {
        // 回退到同步构建（如果线程池不可用）
        ClientChunk* chunkPtr = getChunk(id);
        if (chunkPtr && chunkPtr->data) {
            rebuildMesh(*chunkPtr);
            chunkPtr->needsMeshUpdate = true;
            chunkPtr->meshBuilding = false;
        }
    }
}

void ClientWorld::onChunkUnload(ChunkCoord x, ChunkCoord z) {
    ChunkId id(x, z);
    auto it = m_chunks.find(id);
    if (it != m_chunks.end()) {
        // 调用卸载回调（通知 ChunkRenderer 释放 GPU 缓冲区）
        if (m_chunkUnloadCallback) {
            m_chunkUnloadCallback(id);
        }

        m_chunks.erase(it);
        m_chunksUnloaded++;
        spdlog::debug("Unloaded chunk ({}, {}) from server", x, z);
    }
}

// ============================================================================
// 时间管理
// ============================================================================

void ClientWorld::onTimeUpdate(i64 gameTime, i64 dayTime, bool daylightCycleEnabled) {
    m_prevDayTime = m_dayTime;
    m_gameTime = gameTime;
    m_targetDayTime = dayTime;
    m_dayTime = dayTime;
    m_daylightCycleEnabled = daylightCycleEnabled;
}

f32 ClientWorld::getInterpolatedCelestialAngle(f32 partialTick) const {
    // 计算插值后的 dayTime
    i64 dayTimeForInterp = m_dayTime;

    // 如果日光周期启用且时间在向前流动，进行插值
    if (m_daylightCycleEnabled && m_prevDayTime != m_dayTime) {
        // 简单线性插值
        // 注意：需要处理 dayTime 循环 (23999 -> 0) 的情况
        i64 diff = m_dayTime - m_prevDayTime;
        if (diff < 0) {
            // 时间从 23999 跳到 0，需要特殊处理
            diff += mc::game::DAY_LENGTH_TICKS;
        }
        dayTimeForInterp = m_prevDayTime + static_cast<i64>(diff * partialTick);
    }

    // 计算天体角度 (MC 1.16.5 算法)
    f32 d0 = std::fmod(static_cast<f32>(dayTimeForInterp) / static_cast<f32>(mc::game::DAY_LENGTH_TICKS) - 0.25f, 1.0f);
    if (d0 < 0.0f) {
        d0 += 1.0f;
    }
    f32 d1 = 0.5f - std::cos(d0 * mc::math::PI) / 2.0f;

    return (d0 * 2.0f + d1) / 3.0f;
}

// ============================================================================
// 网格构建线程池
// ============================================================================

void ClientWorld::initializeMeshWorkerPool(i32 threadCount) {
    if (m_meshWorkerPool) {
        spdlog::warn("MeshWorkerPool already initialized");
        return;
    }

    m_meshWorkerPool = std::make_unique<MeshWorkerPool>(threadCount);
    m_meshWorkerPool->start();
    spdlog::info("ClientWorld: MeshWorkerPool initialized with {} threads",
                 m_meshWorkerPool->threadCount());
}

void ClientWorld::shutdownMeshWorkerPool() {
    if (m_meshWorkerPool) {
        m_meshWorkerPool->shutdown();
        m_meshWorkerPool.reset();
        spdlog::info("ClientWorld: MeshWorkerPool shutdown");
    }
}

void ClientWorld::processMeshBuildResults(u32 maxPerFrame) {
    if (!m_meshWorkerPool || !m_meshWorkerPool->isRunning()) {
        return;
    }

    // 根据完成队列积压动态扩容处理预算，减少大规模区块同步时的峰值内存占用。
    // 上限控制在 64，避免单帧处理过多导致卡顿。
    const u32 completedBacklog = static_cast<u32>(m_meshWorkerPool->completedTaskCount());
    const u32 dynamicBudget = std::max(maxPerFrame, std::min(completedBacklog, 64u));

    m_meshWorkerPool->processCompletedTasks(
        [this](MeshBuildResult result) {
            ClientChunk* chunk = getChunk(result.chunkId);
            if (!chunk) {
                // 区块可能已被卸载
                return;
            }

            if (result.success) {
                chunk->solidMesh = std::move(result.solidMesh);
                chunk->transparentMesh = std::move(result.transparentMesh);
                chunk->needsMeshUpdate = true;  // 标记需要 GPU 上传
            } else {
                spdlog::warn("Mesh build failed for chunk ({}, {})",
                             result.chunkId.x, result.chunkId.z);
            }

            chunk->meshBuilding = false;
        },
        dynamicBudget
    );
}

// ============================================================================
// 天气同步
// ============================================================================

void ClientWorld::onRainStrengthChange(f32 strength) {
    m_weather.setRainStrength(strength);
}

void ClientWorld::onThunderStrengthChange(f32 strength) {
    m_weather.setThunderStrength(strength);
}

void ClientWorld::onBeginRaining() {
    m_weather.beginRain();
}

void ClientWorld::onEndRaining() {
    m_weather.endRain();
}

// ============================================================================
// 光照更新
// ============================================================================

void ClientWorld::onLightUpdate(i32 chunkX, i32 chunkZ, i32 sectionY,
                                 const std::vector<u8>& skyLight,
                                 const std::vector<u8>& blockLight,
                                 bool /*trustEdges*/) {
    // 查找目标区块
    ChunkId id(chunkX, chunkZ);
    ClientChunk* chunk = getChunk(id);
    if (!chunk || !chunk->data) {
        spdlog::debug("LightUpdate: chunk ({}, {}) not loaded", chunkX, chunkZ);
        return;
    }

    // 获取目标区块段
    ChunkSection* section = chunk->data->getSection(sectionY);
    if (!section) {
        spdlog::debug("LightUpdate: section {} not found in chunk ({}, {})", sectionY, chunkX, chunkZ);
        return;
    }

    // 更新天空光照
    if (!skyLight.empty()) {
        if (skyLight.size() == NibbleArray::BYTE_SIZE) {
            section->skyLightNibble() = NibbleArray(skyLight);
        } else {
            spdlog::warn("LightUpdate: invalid sky light size {}, expected {}",
                         skyLight.size(), NibbleArray::BYTE_SIZE);
        }
    }

    // 更新方块光照
    if (!blockLight.empty()) {
        if (blockLight.size() == NibbleArray::BYTE_SIZE) {
            section->blockLightNibble() = NibbleArray(blockLight);
        } else {
            spdlog::warn("LightUpdate: invalid block light size {}, expected {}",
                         blockLight.size(), NibbleArray::BYTE_SIZE);
        }
    }

    // 标记区块需要重新构建网格
    chunk->needsMeshUpdate = true;

    spdlog::debug("LightUpdate: applied to chunk ({}, {}) section {}", chunkX, chunkZ, sectionY);
}

} // namespace mc::client
