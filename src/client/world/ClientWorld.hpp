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
#include "../renderer/mesh/MeshDataPool.hpp"
#include "../renderer/mesh/MeshResultQueue.hpp"
#include "../renderer/trident/particle/data/ParticleData.hpp"
#include "ClientWeather.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/sync/ChunkSync.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/profiler/MemoryTracking.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/math/frustum/Frustum.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/thread/ITask.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/IBlockAnimateContext.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "entity/ClientEntityManager.hpp"
#include <array>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace mc {
class ItemStack;
class BlockEntity;
} // namespace mc

namespace mc::client {

// 前向声明
namespace renderer::trident::particle {
class ParticleManager;
} // namespace renderer::trident::particle

/**
 * @brief 客户端区块数据
 * TODO 提取到单独的文件中
 *
 * 含对象级内存追踪守卫（m_memTrack），故非聚合体：构造在 .cpp 中定义（默认构造
 * 绑定 this 发 alloc，析构发 free），move ctor/assign 显式重绑定。内部 MeshData 的
 * vector 缓冲区另由 "ChunkMesh" 分配器追踪，与此对象级追踪互补。
 */
struct ClientChunk {
private:
    // 对象级内存追踪守卫：绑定本对象地址，ctor 发 alloc、dtor 发 free。move 时由
    // move ctor/assign 显式「释放旧地址 + 分配新地址」重绑定（守卫不可移动）。
    // 仅 MC_ENABLE_MEMORY && MC_ENABLE_TRACY 时发事件，其余分支空操作。
    // 声明于数据成员之前，使 ctor 初始化列表可将其置于首位（满足 -Wreorder-ctor）。
    ::mc::profiler::TracyObjectTracker<"ClientChunk"> m_memTrack;

public:
    ClientChunk();
    ~ClientChunk();

    ClientChunk(const ClientChunk&) = delete;
    ClientChunk& operator=(const ClientChunk&) = delete;
    ClientChunk(ClientChunk&& other) noexcept;
    ClientChunk& operator=(ClientChunk&& other) noexcept;

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
 * 管理客户端区块数据、异步网格构建、维度校验和天气时间同步。
 */
class ClientWorld : public ICollisionWorld, public IBlockAnimateContext {
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
     * @param x 方块 X 坐标
     * @param z 方块 Z 坐标
     * @return 最高非空气方块的 Y 坐标，如果没有返回 MIN_BUILD_HEIGHT
     */
    [[nodiscard]] i32 getHeight(i32 x, i32 z) const;

    /**
     * @brief 获取指定位置指定类型的高度图值
     *
     * 查询 (x, z) 位置指定高度图类型的最高方块 Y 坐标。
     * 如果指定类型的高度图不存在，回退到基本高度图（WorldSurface 语义）。
     *
     * @param type 高度图类型（如 HeightmapType::MotionBlocking）
     * @param x 方块 X 坐标（世界坐标）
     * @param z 方块 Z 坐标（世界坐标）
     * @return 指定类型高度图的最高方块 Y 坐标，如果区块未加载返回 MIN_BUILD_HEIGHT
     */
    [[nodiscard]] i32 getTopBlockY(world::chunk::HeightmapType type, i32 x, i32 z) const;

    /**
     * @brief 检查指定位置是否可以看到天空
     *
     * 没有天空光照的维度（下界、末地）始终返回 false。
     * 有天空光照的维度中，只有天空光照达到最大值 15 时才返回 true。
     *
     * @param pos 方块位置
     * @return 如果该位置可以看到天空返回 true
     */
    [[nodiscard]] bool canSeeSky(const BlockPos& pos) const;

    /**
     * @brief 检查当前维度是否有天空光照
     *
     * 只有主世界（维度 ID 0）有天空光照。
     * 下界和末地没有天空光照，canSeeSky 始终返回 false。
     *
     * @return 如果当前维度有天空光照返回 true
     */
    [[nodiscard]] bool hasSkyLight() const { return m_dimensionId == 0; }

    void forEachChunk(std::function<void(const ChunkId&, ClientChunk&)> func);
    void forEachDirtyMesh(std::function<void(const ChunkId&, ClientChunk&)> func);

    [[nodiscard]] size_t chunkCount() const { return m_chunks.size(); }

    void setChunkUnloadCallback(std::function<void(const ChunkId&)> callback)
    {
        // 只允许注册一次
        MC_ASSERT_RELEASE(!m_chunkUnloadCallback);
        m_chunkUnloadCallback = std::move(callback);
    }

    /**
     * @brief 设置本地音效播放回调
     *
     * 由 ClientApplication 注册，委托给 AudioService 播放音效。
     *
     * @param callback 音效播放回调函数
     */
    void setPlayLocalSoundCallback(
        std::function<void(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32)> callback)
    {
        m_playLocalSoundCallback = std::move(callback);
    }

    void rebuildChunkMesh(const ChunkId& id);
    void markChunkDirty(const ChunkId& id);

    void setRenderDistance(i32 distance) { m_renderDistance = distance; }
    [[nodiscard]] i32 renderDistance() const { return m_renderDistance; }

    [[nodiscard]] u64 seed() const { return m_seed; }

    // ========== 维度 ==========

    /**
     * @brief 获取当前维度ID
     */
    [[nodiscard]] DimensionId dimensionId() const { return m_dimensionId; }

    /**
     * @brief 设置当前维度ID
     *
     * 在维度切换时调用，用于验证后续收到的区块数据包是否属于当前维度。
     * 应在 clearChunks() 之前调用。
     */
    void setDimensionId(DimensionId dimensionId) { m_dimensionId = dimensionId; }

    // ========== 天气 ==========

    /**
     * @brief 重置天气状态
     *
     * 维度切换时调用，清除降雨、雷暴和闪电状态。
     */
    void resetWeather();

    void onChunkData(ChunkCoord x, ChunkCoord z, DimensionId dimension, std::vector<u8>&& data);
    void onChunkUnload(ChunkCoord x, ChunkCoord z, DimensionId dimension);

    /**
     * @brief 清空所有区块数据
     *
     * 用于维度切换时清空旧维度的区块。
     *
     * 此方法会：
     * 1. 取消所有待处理的网格构建任务
     * 2. 调用区块卸载回调通知渲染器
     * 3. 清空区块映射
     */
    void clearChunks();

    void onTimeUpdate(i64 gameTime, i64 dayTime, bool daylightCycleEnabled);
    [[nodiscard]] f32 getInterpolatedCelestialAngle(f32 partialTick) const;

    [[nodiscard]] i64 dayTime() const { return m_dayTime; }
    [[nodiscard]] i64 dayTimeOfDay() const { return m_dayTime % 24000; }
    [[nodiscard]] i64 gameTime() const { return m_gameTime; }
    [[nodiscard]] bool daylightCycleEnabled() const { return m_daylightCycleEnabled; }

    // ========== 难度 ==========

    /**
     * @brief 获取世界难度
     *
     * 由服务端通过 ServerDifficulty 包同步。
     * 默认为 Normal 难度。
     */
    [[nodiscard]] Difficulty difficulty() const { return m_difficulty; }

    /**
     * @brief 设置世界难度
     *
     * 由网络层收到 ServerDifficulty 包后调用。
     */
    void setDifficulty(Difficulty difficulty) { m_difficulty = difficulty; }

    /**
     * @brief 获取难度是否锁定
     *
     * 由服务端通过 ServerDifficulty 包同步。
     * 当难度锁定时，玩家无法在游戏内更改难度。
     */
    [[nodiscard]] bool isDifficultyLocked() const { return m_difficultyLocked; }

    /**
     * @brief 设置难度是否锁定
     */
    void setDifficultyLocked(bool locked) { m_difficultyLocked = locked; }

    void initializeMeshSystem(util::UniversalWorkerPool& workerPool,
        std::shared_ptr<MeshDataPool> dataPool,
        std::shared_ptr<MeshResultQueue> resultQueue,
        const MeshSchedulerConfig& schedulerConfig);
    void shutdownMeshSystem();
    void processMeshBuildResults(u32 maxPerFrame);

    [[nodiscard]] MeshDataPool* meshDataPool() const { return m_meshDataPool.get(); }
    [[nodiscard]] const MeshBuildScheduler* meshBuildScheduler() const { return m_meshBuildScheduler.get(); }

    [[nodiscard]] ClientEntityManager& entityManager() { return m_entityManager; }
    [[nodiscard]] const ClientEntityManager& entityManager() const { return m_entityManager; }

    [[nodiscard]] ClientWeather& weather() { return m_weather; }
    [[nodiscard]] const ClientWeather& weather() const { return m_weather; }

    // ========== 闪电闪烁效果 ==========

    /**
     * @brief 设置闪电闪烁时间
     *
     * 当闪电击中时调用，产生天空闪烁效果。
     *
     * @param time 闪烁时间（ticks），通常为 2
     */
    void setTimeLightningFlash(i32 time) { m_weather.setTimeLightningFlash(time); }

    /**
     * @brief 获取当前闪电闪烁时间
     */
    [[nodiscard]] i32 lightningFlashTime() const { return m_weather.lightningFlashTime(); }

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
    void addParticle(::mc::particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity);

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
    void addParticle(::mc::particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count);

    /**
     * @brief 生成携带方块状态纹理的粒子
     *
     * 创建携带方块纹理的粒子（如 DustPillar、Block、Breaking 等），
     * 直接使用指定的 BlockState 作为粒子纹理来源。
     *
     * @param type 粒子类型（必须是需要方块状态的类型）
     * @param pos 粒子位置
     * @param velocity 粒子速度
     * @param blockState 方块状态（用于粒子纹理和颜色）
     */
    void addBlockParticle(
        ::mc::particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const BlockState& blockState);

    /**
     * @brief 生成携带物品堆纹理的粒子
     *
     * 创建携带物品纹理的粒子（如 Item、ItemSlime、ItemCobweb、ItemSnowball），
     * 直接使用指定的 ItemStack 作为粒子纹理来源。
     * 方块物品复用 BlockModelCache 获取方块纹理，普通物品使用 ItemTextureAtlas。
     *
     * @param type 粒子类型（必须为 requiresItemData 返回 true 的类型）
     * @param pos 粒子位置
     * @param velocity 粒子速度
     * @param itemStack 物品堆（用于粒子纹理）
     */
    void addItemParticle(::mc::particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const ::mc::ItemStack& itemStack);

    /**
     * @brief 携带粒子数据生成粒子
     *
     * 当粒子需要额外数据（如目标位置、颜色等）时使用此方法。
     * 粒子数据将通过数据工厂传递到对应粒子类的 createWithXxx() 方法。
     *
     * 供游戏逻辑（方块动画、命令系统等）编程式调用。网络层粒子回调
     * 使用 ParticleManager::addPendingParticle() 直接传递，因为网络
     * 数据包的粒子数据已在此处解码完成。
     *
     * @param type 粒子类型
     * @param pos 粒子位置
     * @param velocity 粒子速度
     * @param data 粒子数据（不可为空）
     */
    void addParticleWithData(::mc::particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        std::unique_ptr<renderer::trident::particle::data::ParticleData> data);

    /**
     * @brief 生成带颜色的实体效果粒子
     *
     * 客户端实现：直接通过粒子数据管线创建 EntityEffect 粒子。
     * 服务端对应实现会广播给附近玩家，客户端收到后走相同的数据管线。
     *
     * @param pos 粒子位置
     * @param velocity 粒子速度
     * @param offset 随机偏移范围（客户端在偏移范围内随机分布粒子）
     * @param count 粒子数量
     * @param color 粒子颜色（ARGB 格式）
     */
    void addEntityEffectParticle(
        const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count, u32 color);

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

    // ========== IBlockAnimateContext 接口实现 ==========

    /**
     * @brief 生成动画粒子（委托给 addParticle）
     */
    void addAnimateParticle(::mc::particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity) override
    {
        addParticle(type, pos, velocity);
    }

    /**
     * @brief 播放本地音效（通过音频服务）
     */
    void playLocalSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override;

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

    // ========== 方块动画 tick ==========

    /**
     * @brief 执行方块动画 tick
     *
     * 在客户端每 tick 调用，在玩家周围随机采样位置，对采样到的方块调用 animateTick。
     * 用于生成视觉效果粒子、播放环境音效等。
     *
     * 参考 MC 1.21.11 ClientLevel.animateTick / doAnimateTick
     *
     * @param playerX 玩家方块 X 坐标
     * @param playerY 玩家方块 Y 坐标
     * @param playerZ 玩家方块 Z 坐标
     */
    void animateTick(i32 playerX, i32 playerY, i32 playerZ);

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

    // ========== 方块实体 ==========

    // TODO: 渲染层集成待实现（数据同步层已完成）
    //
    // 当前 ClientWorld 已实现方块实体数据同步的完整链路：
    //   NetworkClient::_handleBlockEntityData → onBlockEntityData → BlockEntityRegistry::create
    //   → BlockEntity::loadFromNBT → m_blockEntities 存储
    //
    // 但渲染层尚有缺失，需后续实现：
    //   1. SignRenderer（告示牌渲染器）尚未实现 — 需在
    //      src/client/renderer/trident/blockentity/renderers/ 下新增 SignRenderer.hpp/cpp，
    //      参考原版 net.minecraft.client.renderer.blockentity.SignRenderer 实现文本渲染
    //      （正反面区分、彩色文本、发光文本、木/竹告示牌变体纹理）。
    //   2. BlockEntityRendererDispatcher 尚未集成到主渲染循环 — 虽然
    //      src/client/renderer/trident/blockentity/BlockEntityRendererDispatcher.hpp/cpp
    //      已存在且 initializeDefaults() 注册了 Chest/Beacon/Banner 渲染器，但
    //      TridentEngine::render() 中尚未调用它。需在区块渲染之后、实体渲染之前
    //      遍历可见区块中的方块实体并调用 dispatcher->render()。
    //      参考原版 LevelRenderer.renderLevel() 中的 blockEntityRenderDispatcher 调用。
    //
    // 现状：数据同步层已完工，告示牌编辑器可正常工作（通过 getBlockEntity 读取文本），
    // 但客户端不会在世界中渲染告示牌文本（及其他未注册渲染器的方块实体）。
    // 后续实现 SignRenderer 和 dispatcher 集成后，此 TODO 应删除。

    /**
     * @brief 处理服务端发来的方块实体数据更新包
     *
     * 收到 BlockEntityData 包后，根据类型在本地查找或创建对应的 BlockEntity，
     * 并通过 loadFromNBT() 更新其状态。
     *
     * @param pos 方块位置
     * @param type 方块实体类型
     * @param nbtData NBT 字节流（Java 版大端序二进制格式）
     */
    void onBlockEntityData(const BlockPos& pos, BlockEntityType type, const std::vector<u8>& nbtData);

    /**
     * @brief 获取指定位置的方块实体
     * @param pos 方块位置
     * @return 方块实体指针，不存在返回 nullptr
     */
    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos);

    /**
     * @brief 获取指定位置的方块实体（const 版本）
     * @param pos 方块位置
     * @return 方块实体指针，不存在返回 nullptr
     */
    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const;

    /**
     * @brief 移除指定位置的方块实体
     * @param pos 方块位置
     */
    void removeBlockEntity(const BlockPos& pos);

    /**
     * @brief 清空所有方块实体
     *
     * 用于维度切换时清空旧维度的方块实体数据。
     */
    void clearBlockEntities();

private:
    void _rebuildMesh(ClientChunk& chunk);
    void _scheduleChunkMeshRebuild(const ChunkId& id);
    void _requestChunkMeshRebuild(const ChunkId& id);
    void _scheduleNeighborMeshRebuild(const ChunkId& id);
    void _scheduleVisibleChunksWithoutMesh(const MeshSchedulerViewState& viewState, u32 maxChunkCount);
    void _doAnimateTick(i32 centerX, i32 centerY, i32 centerZ, i32 range, math::Random& random);

    std::array<std::shared_ptr<const ChunkData>, 6> _getNeighborChunkData(const ChunkId& id);
    void _getNeighborChunks(const ChunkId& id, const ChunkData* neighbors[6]);

    /// 将反序列化完成的 ChunkData 应用到 ClientChunk 并调度网格重建（主线程）。
    void _applyChunkData(const ChunkId& id, std::shared_ptr<ChunkData> data);
    /// fallback 同步反序列化路径（m_computeWorkerPool 未注入时，测试/启动早期）。
    void _applyDeserializedChunk(ChunkCoord x, ChunkCoord z, std::vector<u8> data);
    /// 主线程 drain worker 反序列化续延队列，代际过滤后调 _applyChunkData。
    void _processPendingDeserializedChunks();

private:
    std::unordered_map<ChunkId, std::unique_ptr<ClientChunk>> m_chunks;

    std::function<void(const ChunkId&)> m_chunkUnloadCallback;

    /// 本地音效播放回调（由 ClientApplication 注册，委托给 AudioService）
    std::function<void(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32)>
        m_playLocalSoundCallback;

    /// 客户端统一计算池（ClientCompute，进程级，由 ClientApplication 持有）。
    /// 非拥有，initializeMeshSystem 注入；mesh 系统关停时不关池（池属 ClientApplication）。
    /// 同时承载区块反序列化任务（onChunkData 提交，deserialize 在 worker、结果续延回主线程）。
    util::UniversalWorkerPool* m_computeWorkerPool = nullptr;
    std::shared_ptr<MeshDataPool> m_meshDataPool;
    std::shared_ptr<MeshResultQueue> m_meshResultQueue;
    std::unique_ptr<MeshBuildScheduler> m_meshBuildScheduler;

    /// 区块反序列化续延队列：worker 完成 deserialize 后入队，主线程 update drain。
    struct PendingDeserializedChunk {
        ChunkId id;
        u64 generation = 0; ///< 提交时的代际号，用于丢弃过期结果（同坐标重发包）
        std::shared_ptr<ChunkData> data;
    };
    std::mutex m_pendingChunksMutex;
    std::vector<PendingDeserializedChunk> m_pendingDeserializedChunks;
    /// 每区块最新反序列化代际号（主线程读写，无锁）。onChunkUnload/clearChunks 清理。
    std::unordered_map<ChunkId, u64> m_chunkDeserializeGeneration;

    i32 m_renderDistance = 12;
    u64 m_seed = 0;
    DimensionId m_dimensionId = 0; ///< 当前维度ID，用于验证区块数据包

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

    /// 世界难度（默认 Normal，由服务端同步）
    Difficulty m_difficulty = Difficulty::Normal;
    /// 难度是否锁定
    bool m_difficultyLocked = false;

    /// 粒子管理器（外部引用，不拥有）
    renderer::trident::particle::ParticleManager* m_particleManager = nullptr;

    /// 视锥体，用于视锥剔除
    mc::math::frustum::Frustum m_frustum;

    /// 世界出生点（用于指南针指向）
    BlockPos m_spawnPoint{0, 64, 0};

    /// 出生点偏航角（用于指南针）
    f32 m_spawnAngle = 0.0f;

    /// 客户端方块实体存储（key 为 BlockPos::asLong()）
    /// 收到 BlockEntityData 包时更新；维度切换或区块卸载时清除
    std::unordered_map<i64, std::unique_ptr<BlockEntity>> m_blockEntities;

    bool m_destroyed = false;
};

} // namespace mc::client
