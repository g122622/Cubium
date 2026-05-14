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

#pragma once

#include "../renderer/MeshTypes.hpp"
#include "../renderer/mesh/MeshBuildScheduler.hpp"
#include "ClientWeather.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/sync/ChunkSync.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/util/math/frustum/Frustum.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "entity/ClientEntityManager.hpp"
#include <array>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace mc::client {

// 前向声明
namespace renderer::trident::particle {
class ParticleManager;
enum class ParticleTypeId : u16;
} // namespace renderer::trident::particle

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
 * TODO 为什么不继承 IWorld？这是严重的设计问题！
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
    void setBlockState(i32 x, i32 y, i32 z, const BlockState* state);

    [[nodiscard]] bool isWithinWorldBounds(i32 /*x*/, i32 y, i32 /*z*/) const override
    {
        return y >= m_minBuildHeight && y < m_maxBuildHeight;
    }

    [[nodiscard]] const ChunkData* getChunkAt(ChunkCoord x, ChunkCoord z) const override;

    [[nodiscard]] i32 getMinBuildHeight() const override { return m_minBuildHeight; }
    [[nodiscard]] i32 getMaxBuildHeight() const override { return m_maxBuildHeight; }

    /**
     * @brief 获取指定位置的最高方块高度
     *
     * 参考 MC 1.16.5 IWorldReader#getHeight
     *
     * @param x 方块 X 坐标
     * @param z 方块 Z 坐标
     * @return 最高非空气方块的 Y 坐标，如果没有返回 MIN_BUILD_HEIGHT
     */
    [[nodiscard]] i32 getHeight(i32 x, i32 z) const;

    /**
     * @brief 检查指定位置是否可以看到天空
     *
     * 参考 MC 1.16.5 IWorldReader#canSeeSky
     *
     * @param pos 方块位置
     * @return 如果该位置可以看到天空返回 true
     */
    [[nodiscard]] bool canSeeSky(const BlockPos& pos) const;

    void forEachChunk(std::function<void(const ChunkId&, ClientChunk&)> func);
    void forEachDirtyMesh(std::function<void(const ChunkId&, ClientChunk&)> func);

    [[nodiscard]] size_t chunkCount() const { return m_chunks.size(); }

    void setChunkUnloadCallback(std::function<void(const ChunkId&)> callback)
    {
        // 只允许注册一次
        MC_ASSERT_RELEASE(!m_chunkUnloadCallback);
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

    // ========== 粒子管理 ==========

    /**
     * @brief 设置粒子管理器引用
     *
     * @param manager 粒子管理器指针（由 ClientApplication 管理）
     */
    void setParticleManager(renderer::trident::particle::ParticleManager* manager) { m_particleManager = manager; }

    [[nodiscard]] renderer::trident::particle::ParticleManager* particleManager() { return m_particleManager; }
    [[nodiscard]] const renderer::trident::particle::ParticleManager* particleManager() const
    {
        return m_particleManager;
    }

    // ========== 粒子生成接口 ==========

    /**
     * @brief 生成粒子
     *
     * 在客户端本地生成粒子效果。
     *
     * @param type 粒子类型
     * @param pos 粒子位置
     * @param velocity 粒子速度
     */
    void addParticle(renderer::trident::particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity);

    /**
     * @brief 生成粒子（带数量和偏移）
     *
     * 在指定位置附近随机生成多个粒子。
     *
     * @param type 粒子类型
     * @param pos 粒子中心位置
     * @param velocity 粒子基础速度
     * @param offset 随机偏移范围
     * @param count 粒子数量
     */
    void addParticle(renderer::trident::particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count);

    /**
     * @brief 检查是否应在指定位置生成粒子
     *
     * 用于距离裁剪，避免在玩家视野外生成粒子。
     *
     * @param pos 粒子位置
     * @param maxDistance 最大距离（默认 256 格）
     * @return 是否应生成粒子
     */
    [[nodiscard]] bool shouldSpawnParticleAt(const Vector3& pos, f32 maxDistance = 256.0f) const;

    void onRainStrengthChange(f32 strength);
    void onThunderStrengthChange(f32 strength);
    void onBeginRaining();
    void onEndRaining();

    void onLightUpdate(i32 chunkX,
        i32 chunkZ,
        i32 sectionY,
        const std::vector<u8>& skyLight,
        const std::vector<u8>& blockLight,
        bool trustEdges);

    // ========== 出生点 ==========

    /**
     * @brief 设置世界出生点
     *
     * 由服务端通过 SpawnPosition 包设置。用于指南针指向。
     *
     * @param x 出生点 X 坐标
     * @param y 出生点 Y 坐标
     * @param z 出生点 Z 坐标
     * @param angle 出生点偏航角（用于指南针）
     */
    void setSpawnPoint(i32 x, i32 y, i32 z, f32 angle);

    /**
     * @brief 获取世界出生点
     */
    [[nodiscard]] const BlockPos& getSpawnPoint() const { return m_spawnPoint; }

    /**
     * @brief 获取出生点偏航角
     */
    [[nodiscard]] f32 getSpawnAngle() const { return m_spawnAngle; }

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

    i32 m_minBuildHeight = mc::world::MIN_BUILD_HEIGHT;
    i32 m_maxBuildHeight = mc::world::MAX_BUILD_HEIGHT;

    u32 m_chunksLoaded = 0;
    u32 m_chunksUnloaded = 0;

    i64 m_gameTime = 0;
    i64 m_dayTime = 0;
    i64 m_prevDayTime = 0;
    i64 m_targetDayTime = 0;
    bool m_daylightCycleEnabled = true;

    ClientEntityManager m_entityManager;
    ClientWeather m_weather;

    /// 粒子管理器（外部引用，不拥有）
    renderer::trident::particle::ParticleManager* m_particleManager = nullptr;

    /// 视锥体，用于视锥剔除
    mc::math::frustum::Frustum m_frustum;

    /// 世界出生点（用于指南针指向）
    BlockPos m_spawnPoint{0, 64, 0};

    /// 出生点偏航角（用于指南针）
    f32 m_spawnAngle = 0.0f;

    bool m_destroyed = false;
};

} // namespace mc::client
