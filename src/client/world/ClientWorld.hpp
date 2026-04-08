#pragma once

#include "../../common/core/Result.hpp"
#include "../../common/core/Types.hpp"
#include "../../common/network/sync/ChunkSync.hpp"
#include "../../common/physics/PhysicsEngine.hpp"
#include "../../common/world/WorldConstants.hpp"
#include "../../common/world/biome/Biome.hpp"
#include "../../common/world/chunk/ChunkData.hpp"
#include "../renderer/MeshTypes.hpp"
#include "../renderer/mesh/MeshBuildScheduler.hpp"
#include "ClientWeather.hpp"
#include "entity/ClientEntityManager.hpp"
#include <array>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc::client {

/**
 * @brief 客户端区块数据
 */
struct ClientChunk {
    ChunkId chunkId;
    std::shared_ptr<ChunkData> data;
    MeshData solidMesh;
    MeshData transparentMesh;
    bool hasMeshResult = false;
    bool needsMeshUpdate = true;
    bool meshRebuildPending = false;
    bool isLoaded = false;
    u64 activeMeshTaskId = 0;
};

/**
 * @brief 客户端世界管理器
 *
 * 管理客户端区块数据、异步网格构建和天气时间同步。
 */
class ClientWorld : public ICollisionWorld {
public:
    ClientWorld();
    ~ClientWorld() override;

    ClientWorld(const ClientWorld&) = delete;
    ClientWorld& operator=(const ClientWorld&) = delete;

    [[nodiscard]] Result<void> initialize(u64 seed);
    void destroy();

    void update(const MeshSchedulerViewState& viewState);

    [[nodiscard]] ClientChunk* getChunk(const ChunkId& id);
    [[nodiscard]] const ClientChunk* getChunk(const ChunkId& id) const;

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] const Biome* getBiomeAtBlock(i32 x, i32 y, i32 z) const;
    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const;
    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const;
    void setBlock(i32 x, i32 y, i32 z, const BlockState* state);

    [[nodiscard]] bool isWithinWorldBounds(i32 /*x*/, i32 y, i32 /*z*/) const override {
        return y >= m_minBuildHeight && y < m_maxBuildHeight;
    }

    [[nodiscard]] const ChunkData* getChunkAt(ChunkCoord x, ChunkCoord z) const override;

    [[nodiscard]] i32 getMinBuildHeight() const override { return m_minBuildHeight; }
    [[nodiscard]] i32 getMaxBuildHeight() const override { return m_maxBuildHeight; }

    void forEachChunk(std::function<void(const ChunkId&, ClientChunk&)> func);
    void forEachDirtyMesh(std::function<void(const ChunkId&, ClientChunk&)> func);

    [[nodiscard]] size_t chunkCount() const { return m_chunks.size(); }

    void setChunkUnloadCallback(std::function<void(const ChunkId&)> callback) {
        m_chunkUnloadCallback = std::move(callback);
    }

    void rebuildChunkMesh(const ChunkId& id);
    void markChunkDirty(const ChunkId& id);

    void setRenderDistance(i32 distance) { m_renderDistance = distance; }
    [[nodiscard]] i32 renderDistance() const { return m_renderDistance; }

    [[nodiscard]] u64 seed() const { return m_seed; }

    void onChunkData(ChunkCoord x, ChunkCoord z, std::vector<u8>&& data);
    void onChunkUnload(ChunkCoord x, ChunkCoord z);

    void onTimeUpdate(i64 gameTime, i64 dayTime, bool daylightCycleEnabled);
    [[nodiscard]] f32 getInterpolatedCelestialAngle(f32 partialTick) const;

    [[nodiscard]] i64 dayTime() const { return m_dayTime; }
    [[nodiscard]] i64 gameTime() const { return m_gameTime; }
    [[nodiscard]] bool daylightCycleEnabled() const { return m_daylightCycleEnabled; }

    void initializeMeshSystem(i32 threadCount, const MeshSchedulerConfig& schedulerConfig);
    void shutdownMeshSystem();
    void processMeshBuildResults(u32 maxPerFrame);

    [[nodiscard]] const MeshWorkerPool* meshWorkerPool() const { return m_meshWorkerPool.get(); }
    [[nodiscard]] const MeshBuildScheduler* meshBuildScheduler() const { return m_meshBuildScheduler.get(); }

    [[nodiscard]] ClientEntityManager& entityManager() { return m_entityManager; }
    [[nodiscard]] const ClientEntityManager& entityManager() const { return m_entityManager; }

    [[nodiscard]] ClientWeather& weather() { return m_weather; }
    [[nodiscard]] const ClientWeather& weather() const { return m_weather; }

    void onRainStrengthChange(f32 strength);
    void onThunderStrengthChange(f32 strength);
    void onBeginRaining();
    void onEndRaining();

    void onLightUpdate(
        i32 chunkX,
        i32 chunkZ,
        i32 sectionY,
        const std::vector<u8>& skyLight,
        const std::vector<u8>& blockLight,
        bool trustEdges
    );

private:
    void rebuildMesh(ClientChunk& chunk);
    void scheduleChunkMeshRebuild(const ChunkId& id);
    void requestChunkMeshRebuild(const ChunkId& id);
    void scheduleNeighborMeshRebuild(const ChunkId& id);
    void scheduleVisibleChunksWithoutMesh(const MeshSchedulerViewState& viewState, u32 maxChunkCount);

    std::array<std::shared_ptr<const ChunkData>, 6> getNeighborChunkData(const ChunkId& id);
    void getNeighborChunks(const ChunkId& id, const ChunkData* neighbors[6]);

private:
    std::unordered_map<ChunkId, std::unique_ptr<ClientChunk>> m_chunks;

    std::function<void(const ChunkId&)> m_chunkUnloadCallback;

    std::unique_ptr<MeshWorkerPool> m_meshWorkerPool;
    std::unique_ptr<MeshBuildScheduler> m_meshBuildScheduler;

    i32 m_renderDistance = 12;
    u64 m_seed = 0;

    glm::vec3 m_cameraPosition{0.0f, 0.0f, 0.0f};

    i32 m_minBuildHeight = 0;
    i32 m_maxBuildHeight = 256;

    u32 m_chunksLoaded = 0;
    u32 m_chunksUnloaded = 0;

    i64 m_gameTime = 0;
    i64 m_dayTime = 0;
    i64 m_prevDayTime = 0;
    i64 m_targetDayTime = 0;
    bool m_daylightCycleEnabled = true;

    ClientEntityManager m_entityManager;
    ClientWeather m_weather;

    bool m_destroyed = false;
};

} // namespace mc::client
