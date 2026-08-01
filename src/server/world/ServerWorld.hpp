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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/physics/CollisionCache.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConfig.hpp"
#include "common/world/blockevent/BlockEventData.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/dimension/end/EndDragonFight.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/world/gameevent/GameEventDispatcher.hpp"
#include "common/world/gameevent/PositionSource.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/map/MapDataManager.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/world/village/VillageManager.hpp"
#include "common/world/village/raid/RaidManager.hpp"
#include "server/world/RuntimeLightingProvider.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerLightQueue.hpp"
#include "server/world/blockentity/sculk/SculkVibrationSystem.hpp"
#include "server/world/entity/EntityChunkTracker.hpp"
#include "server/world/entity/EntityTracker.hpp"
#include "server/world/entity/ItemPickupManager.hpp"
#include "server/world/weather/WeatherManager.hpp"
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace mc {

// 前向声明
struct SpawnedEntityData;
class INamedContainerProvider;
class WorldGenRegion;

namespace particle {
enum class ParticleTypeId : u16;
}

namespace server::core {
class TimeManager; // 前向声明
}

namespace loot {
class LootTableManager; // 前向声明
}

namespace server {

// 前向声明
class IServer;

// ============================================================================
// 服务端世界配置
// ============================================================================

/**
 * @brief 服务端世界配置结构
 *
 * 包含世界创建和运行所需的基本配置参数。
 * 世界类型通过 chunkGenerator 动态判断，不在此存储。
 */
struct ServerWorldConfig {
    i32 viewDistance = 10;           ///< 视距（区块数）
    DimensionId dimension = 0;       ///< 维度ID（0=主世界，1=下界，2=末地）
    u64 seed = 114514;               ///< 世界种子
    std::string worldName = "world"; ///< 世界名称（用于存档目录）
};

// ============================================================================
// 服务端世界
//
// 纯粹的世界数据容器，职责：
// - 区块管理
// - 实体管理
// - 光照计算
// - 物理模拟
// - Tick 调度
// - 天气状态
//
// 不负责：
// - 玩家管理（由 PlayerManager 管理）
// - 网络通信（由 ConnectionManager 管理）
// - 时间管理（由 TimeManager 管理）
// - 传送（由 TeleportManager 管理）
// - 游戏模式（由 GameModeManager 管理）
// ============================================================================

class ServerWorld : public IWorld, public ICollisionWorld, public StarLightLightingProvider {
public:
    explicit ServerWorld(const ServerWorldConfig& config);
    ServerWorld(const ServerWorldConfig& config, std::unique_ptr<ServerChunkManager> chunkManager);
    ~ServerWorld() override;

    using IWorld::getBlockLight;
    using IWorld::getBlockState;
    using IWorld::getFluidState;
    using IWorld::getSkyLight;
    using IWorld::isWithinWorldBounds;
    using IWorld::setBlockState;

    // 初始化
    [[nodiscard]] Result<void> initialize();
    void shutdown();

    // ========== 存储系统 ==========

    /**
     * @brief 获取存储服务
     *
     * 这是访问存档功能的唯一入口。
     */
    [[nodiscard]] world::storage::SingleLevelStorageManager& storage()
    {
        MC_ASSERT_RELEASE(m_storage != nullptr);
        return *m_storage;
    }
    [[nodiscard]] const world::storage::SingleLevelStorageManager& storage() const
    {
        MC_ASSERT_RELEASE(m_storage != nullptr);
        return *m_storage;
    }

    /**
     * @brief 检查存储服务是否已打开
     */
    [[nodiscard]] bool isStorageOpen() const { return m_storage != nullptr && m_storage->isOpen(); }

    void setSharedStorage(world::storage::SingleLevelStorageManager* storage);

    // ========== 服务器接口（用于实体销毁广播等） ==========

    /**
     * @brief 注入服务器接口
     *
     * 由 MinecraftServer 在世界创建后设置，供 ServerWorld 在主动移除实体
     * （removeEntity / 区块卸载）时通过 EntityTracker 向追踪玩家发送
     * destroy 包。未设置时实体销毁只能依赖 EntityTracker::tick 下一轮补发，
     * 存在 ID 复用风险下会漏发（现 ID 已不复用，此处主动发送以消除时序窗口）。
     */
    void setServer(IServer* server) { m_server = server; }
    [[nodiscard]] IServer* server() noexcept { return m_server; }
    [[nodiscard]] const IServer* server() const noexcept { return m_server; }

    // 配置
    void setConfig(const ServerWorldConfig& config);
    [[nodiscard]] const ServerWorldConfig& config() const { return m_config; }

    // ========== 时间管理（设置外部 TimeManager） ==========

    void setTimeManager(core::TimeManager* timeManager) { m_timeManager = timeManager; }
    [[nodiscard]] core::TimeManager* timeManager() { return m_timeManager; }
    [[nodiscard]] const core::TimeManager* timeManager() const { return m_timeManager; }

    // ========== 难度管理（设置外部难度回调） ==========

    /**
     * @brief 设置难度获取回调
     *
     * ServerWorld 不直接持有难度值，而是通过回调从 MinecraftServer 获取。
     * 这样可以在运行时动态修改难度（如通过 /difficulty 命令）。
     *
     * @param callback 难度获取回调函数
     */
    void setDifficultyCallback(std::function<Difficulty()> callback) { m_difficultyCallback = std::move(callback); }

    // ========== 区块管理 ==========

    /**
     * @brief 获取区块（可变版本，供需要修改区块的场景使用）
     *
     * 这是 ServerWorld 特有的方法，提供非 const 访问。
     * IWorld 接口的 const 版本也会委托给此方法。
     */
    [[nodiscard]] ChunkData* getChunk(ChunkCoord x, ChunkCoord z);
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord x, ChunkCoord z) const override;
    [[nodiscard]] bool hasChunk(ChunkCoord x, ChunkCoord z) const override;
    [[nodiscard]] const ChunkData* getOrLoadChunk(ChunkCoord x, ChunkCoord z) override;

    // ========== 方块操作 ==========

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override;
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override;

    // ========== 方块实体管理 ==========

    /**
     * @brief 获取指定位置的方块实体
     * @param pos 方块位置
     * @return 方块实体指针，如果不存在返回 nullptr
     */
    [[nodiscard]] BlockEntity* getBlockEntity(const BlockPos& pos) override;
    [[nodiscard]] const BlockEntity* getBlockEntity(const BlockPos& pos) const override;

    /**
     * @brief 设置指定位置的方块实体
     * @param pos 方块位置
     * @param entity 方块实体指针（获取所有权）
     */
    void setBlockEntity(const BlockPos& pos, BlockEntity* entity) override;

    /**
     * @brief 移除指定位置的方块实体
     * @param pos 方块位置
     */
    void removeBlockEntity(const BlockPos& pos) override;

    // ========== 更新循环 ==========

    void tick();

    /**
     * @brief 处理降水对方块的影响（结冰和降雪）
     *
     * 对每个已加载区块，以 1/48 的概率选择一个位置，
     * 检查生物群系温度并执行结冰或降雪操作。
     * 结冰不受天气状态影响（低温即结冰），降雪仅在下雪时执行。
     *
     * 冰形成：生物群系温度 < 0.15 且方块光照 < 10 且流体为水的位置，
     *         将水替换为冰（checkNeighbors=true，防止深海大面积结冰）。
     * 降雪：下雪时，生物群系温度 < 0.15 且方块光照 < 10 且位置为空气/已有雪层，
     *        放置或增加雪层（受 snowAccumulationHeight 游戏规则控制）。
     */
    void tickPrecipitation(i32 randomTickSpeed);

private:
    /**
     * @brief 执行方块实体tick
     *
     * 遍历所有已加载区块中的方块实体，对 needsTick() 返回 true
     * 且未被移除的方块实体调用其 tick() 方法。
     * 对应 MC Java 的 Level.tickBlockEntities()。
     */
    void tickBlockEntities();

    /**
     * @brief 处理待执行的方块事件队列
     *
     * 每tick调用，依次处理队列中的方块事件：
     * 1. 如果对应区块未加载/未tick，则延迟到下tick处理
     * 2. 验证方块类型是否匹配（方块可能已被替换）
     * 3. 执行事件的 triggerEvent，如果返回 true 则广播给客户端
     *
     * 参考 MC Java: ServerLevel.runBlockEvents()
     */
    void runBlockEvents();

    /**
     * @brief 执行单个方块事件
     *
     * 验证方块是否仍匹配，匹配则调用 Block::triggerEvent()。
     *
     * @param event 方块事件数据
     * @return 事件是否被成功处理
     */
    [[nodiscard]] bool doBlockEvent(const BlockEventData& event);

    /**
     * @brief 执行环境随机刻
     *
     * 对每个已加载区块，随机选择位置执行方块和流体的随机刻。
     * 这是农作物生长、冰融化、火焰蔓延等机制的核心。
     *
     * @param randomTickSpeed 随机刻速度（默认为3，可在游戏规则中设置）
     */
    void tickEnvironment(i32 randomTickSpeed);

    /**
     * @brief 生成随机方块位置
     *
     * 使用 LCG 生成随机位置，确保同一 tick 内位置分布均匀。
     *
     * @param chunkX 区块 X 坐标（方块坐标）
     * @param sectionY 区块段 Y 坐标（方块坐标）
     * @param chunkZ 区块 Z 坐标（方块坐标）
     * @return 随机方块位置
     */
    [[nodiscard]] BlockPos getBlockRandomPos(i32 chunkX, i32 sectionY, i32 chunkZ);

public:
    // ========== 统计 ==========

    [[nodiscard]] size_t chunkCount() const;
    [[nodiscard]] size_t loadedChunkCount() const;

    // ========== 区块管理器 ==========

    [[nodiscard]] ServerChunkManager* chunkManager() { return m_chunkManager.get(); }
    [[nodiscard]] const ServerChunkManager* chunkManager() const { return m_chunkManager.get(); }

    // ========== 天气管理 ==========

    [[nodiscard]] WeatherManager* weatherManager() { return m_weatherManager.get(); }
    [[nodiscard]] const WeatherManager* weatherManager() const { return m_weatherManager.get(); }
    void setWeatherManager(std::unique_ptr<WeatherManager> manager) { m_weatherManager = std::move(manager); }

    // ========== 时间管理（委托给外部 TimeManager） ==========

    // IWorld 接口实现 - 从 TimeManager 获取时间
    [[nodiscard]] u64 currentTick() const override;
    [[nodiscard]] i64 dayTime() const override;
    [[nodiscard]] i64 dayTimeOfDay() const override;

    // ========== IWorld 接口 ==========

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] bool isWithinWorldBounds(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] bool isBlockInLine(
        const Vector3d& from, const Vector3d& to, std::function<bool(const BlockState&)> predicate) const override;
    [[nodiscard]] i32 getHeight(i32 x, i32 z) const override;
    [[nodiscard]] u8 getBlockLight(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] u8 getSkyLight(i32 x, i32 y, i32 z) const override;
    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB& box) const override;
    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB& box) const override;
    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB& box, const Entity* except = nullptr) const override;
    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(
        const AxisAlignedBB& box, const Entity* except = nullptr) const override;
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(
        const AxisAlignedBB& box, const Entity* except = nullptr) const override;
    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(
        const Vector3& pos, f32 range, const Entity* except = nullptr) const override;
    [[nodiscard]] std::vector<Entity*> getPlayers() const override;
    [[nodiscard]] std::vector<Entity*> getEntitiesByType(const std::string& typeId) const override;

    // ========== 最近玩家查询 ==========

    [[nodiscard]] Player* getClosestPlayer(const Vector3& pos, f32 maxDistance = -1.0f) override;
    [[nodiscard]] const Player* getClosestPlayer(const Vector3& pos, f32 maxDistance = -1.0f) const override;
    [[nodiscard]] Player* getClosestPlayer(const Vector3& pos, f32 maxDistance, const Entity* exclude) override;
    [[nodiscard]] const Player* getClosestPlayer(
        const Vector3& pos, f32 maxDistance, const Entity* exclude) const override;
    [[nodiscard]] f64 getClosestPlayerDistanceSq(const Vector3& pos) const override;

    [[nodiscard]] DimensionId dimension() const noexcept override { return m_config.dimension; }
    [[nodiscard]] bool isUltraWarm() const noexcept override { return getDimensionType().ultraWarm(); }
    [[nodiscard]] DimensionType getDimensionType() const;
    [[nodiscard]] u64 seed() const noexcept override { return m_config.seed; }
    [[nodiscard]] bool isHardcore() const noexcept override { return false; }
    [[nodiscard]] Difficulty difficulty() const override;
    [[nodiscard]] bool isClientSide() const noexcept override { return false; }

    /**
     * @brief 是否允许 PvP（读取 PVP 游戏规则）
     */
    [[nodiscard]] bool isPvpAllowed() const override;

    // ========== 随机数生成器 ==========

    [[nodiscard]] math::Random& getRandom() noexcept override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const noexcept override { return m_random; }

    // ========== 游戏规则 ==========

    /**
     * @brief 获取游戏规则管理器（只读）
     *
     * @return GameRules 常引用
     */
    [[nodiscard]] const world::gamerule::GameRules& getGameRules() const noexcept override { return m_gameRules; }

    /**
     * @brief 获取游戏规则管理器（可变）
     *
     * @return GameRules 引用
     */
    [[nodiscard]] world::gamerule::GameRules& getGameRules() noexcept override { return m_gameRules; }

    // ========== 游戏事件分发 ==========

    /**
     * @brief 获取游戏事件分发器
     */
    [[nodiscard]] gameevent::GameEventDispatcher& gameEventDispatcher() { return *m_gameEventDispatcher; }
    [[nodiscard]] const gameevent::GameEventDispatcher& gameEventDispatcher() const { return *m_gameEventDispatcher; }

    // ========== 类型转换 ==========

    [[nodiscard]] ServerWorld* asServerWorld() noexcept override { return this; }
    [[nodiscard]] const ServerWorld* asServerWorld() const noexcept override { return this; }

    // ========== 按需特征放置 ==========

    /**
     * @brief 从已加载区块构建临时 WorldGenRegion
     *
     * 在指定位置周围收集 3x3 区块窗口，构建 WorldGenRegion，
     * 用于 SaplingBlock::grow() 等按需特征放置场景。
     *
     * @param position 中心位置
     * @return 创建的 WorldGenRegion，如果区块未加载则返回 nullptr
     */
    [[nodiscard]] std::unique_ptr<WorldGenRegion> createFeatureRegion(const BlockPos& position) override;

    // ========== 天气接口 (IWorld) ==========

    [[nodiscard]] bool isRaining() const override;
    [[nodiscard]] bool isThundering() const override;
    [[nodiscard]] f32 rainStrength(f32 partialTick = 0.0f) const override;
    [[nodiscard]] f32 thunderStrength(f32 partialTick = 0.0f) const override;
    [[nodiscard]] bool canRainAt(const BlockPos& pos) const override;

    // ========== 声音播放 ==========

    void setOnPlaySound(
        std::function<void(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32)> callback)
    {
        if (m_onPlaySound) {
            throw std::runtime_error("Sound callback already set");
        }

        m_onPlaySound = std::move(callback);
    }

    void playSound(const ResourceLocation& soundEventId,
        sound::SoundCategory category,
        const Vector3& position,
        f32 volume,
        f32 pitch) override;

    // ========== 世界事件 ==========

    void playEvent(i32 eventId, const BlockPos& pos, i32 data) override;

    void destroyBlockProgress(EntityInstanceId breakerId, const BlockPos& pos, i32 progress) override;

    // ========== 游戏事件 ==========

    void gameEvent(
        const gameevent::GameEvent& event, const BlockPos& pos, const gameevent::GameEvent::Context& context) override;

    // ========== 方块更新通知 ==========

    void notifyBlockUpdate(const BlockPos& pos) override;

    // ========== 方块事件 ==========

    /**
     * @brief 触发方块事件
     *
     * 将方块事件加入队列，每tick处理时验证方块是否仍匹配，
     * 匹配则执行事件并广播给附近客户端。
     *
     * 参考 MC Java: ServerLevel.blockEvent(BlockPos, Block, int, int)
     *
     * @param pos 方块位置
     * @param block 方块类型（用于验证方块是否仍存在）
     * @param paramA 事件参数A
     * @param paramB 事件参数B
     */
    void blockEvent(const BlockPos& pos, const Block& block, i32 paramA, i32 paramB) override;

    // ========== 容器打开回调 ==========

    using OpenContainerCallback = std::function<bool(ContainerType, const BlockPos&, Player&)>;

    void setOnOpenContainer(OpenContainerCallback callback)
    {
        if (m_onOpenContainer) {
            throw std::runtime_error("Open container callback already set");
        }

        m_onOpenContainer = std::move(callback);
    }
    [[nodiscard]] bool openContainer(ContainerType type, const BlockPos& pos, Player& player) override;

    /**
     * @brief 打开实体容器
     * @param provider 命名容器提供者（村民、矿车等）
     * @param player 玩家
     * @return 如果成功打开返回 true
     *
     * TODO: 实体容器打开回调尚未接线（MinecraftServer 未注册），当前恒返回 false。
     */
    [[nodiscard]] bool openEntityContainer(INamedContainerProvider& provider, Player& player) override;

    // ========== 粒子广播回调 ==========

    /**
     * @brief 粒子广播回调类型
     *
     * 当服务端需要广播粒子给玩家时调用。
     * 参数：粒子类型、位置、速度、偏移、数量
     */
    using ParticleBroadcastCallback = std::function<void(
        particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count)>;

    void setOnBroadcastParticle(ParticleBroadcastCallback callback) { m_onBroadcastParticle = std::move(callback); }

    /**
     * @brief 振动粒子广播回调类型
     *
     * 当服务端需要广播振动粒子给玩家时调用。
     * 振动粒子需要额外的目标位置来源和到达时间信息。
     * 参数：粒子起始位置、目标位置来源、到达 tick 数
     *
     * 目标位置来源 (PositionSource) 决定网络序列化方式：
     * - BlockPositionSource: 序列化为 VarInt(0) + i64 packedBlockPos
     * - EntityPositionSource: 序列化为 VarInt(1) + VarInt entityId + f32 yOffset
     */
    using VibrationParticleBroadcastCallback =
        std::function<void(const Vector3& pos, const gameevent::PositionSource& targetSource, i32 arrivalInTicks)>;

    void setOnBroadcastVibrationParticle(VibrationParticleBroadcastCallback callback)
    {
        m_onBroadcastVibrationParticle = std::move(callback);
    }

    /**
     * @brief 轨迹粒子广播回调类型
     *
     * 当服务端需要广播轨迹粒子（TrailParticle）给玩家时调用。
     * 轨迹粒子需要额外的目标位置、ARGB 颜色和飞行持续时间。
     * 参数：粒子起始位置、目标位置、ARGB 颜色、飞行 tick 数
     */
    using TrailParticleBroadcastCallback =
        std::function<void(const Vector3& pos, const Vector3d& targetPosition, u32 color, i32 durationInTicks)>;

    void setOnBroadcastTrailParticle(TrailParticleBroadcastCallback callback)
    {
        m_onBroadcastTrailParticle = std::move(callback);
    }

    /**
     * @brief 实体效果粒子广播回调类型
     *
     * 当服务端需要广播带颜色的 EntityEffect 粒子给玩家时调用。
     * 参数：位置、速度、偏移、数量、ARGB 颜色
     */
    using EntityEffectParticleBroadcastCallback =
        std::function<void(const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count, u32 color)>;

    void setOnBroadcastEntityEffectParticle(EntityEffectParticleBroadcastCallback callback)
    {
        m_onBroadcastEntityEffectParticle = std::move(callback);
    }

    /**
     * @brief 方块粒子广播回调类型
     *
     * 当服务端需要广播携带方块状态的粒子（Block/Breaking 等）给玩家时调用。
     * 参数：粒子类型、位置、速度、方块状态 ID
     */
    using BlockParticleBroadcastCallback = std::function<void(
        particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, u32 blockStateId)>;

    void setOnBroadcastBlockParticle(BlockParticleBroadcastCallback callback)
    {
        m_onBroadcastBlockParticle = std::move(callback);
    }

    /**
     * @brief 物品粒子广播回调类型
     *
     * 当服务端需要广播携带物品堆的粒子（Item/ItemSlime/ItemCobweb/ItemSnowball）给玩家时调用。
     * 参数：粒子类型、位置、速度、物品堆
     */
    using ItemParticleBroadcastCallback = std::function<void(
        particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity, const ItemStack& itemStack)>;

    void setOnBroadcastItemParticle(ItemParticleBroadcastCallback callback)
    {
        m_onBroadcastItemParticle = std::move(callback);
    }

    // ========== 实体状态广播回调 ==========

    /**
     * @brief 实体状态广播回调类型
     *
     * 当服务端需要广播实体状态事件给玩家时调用。
     * 参数：实体ID、状态码
     */
    using EntityStatusCallback = std::function<void(EntityInstanceId entityId, u8 status)>;

    void setOnBroadcastEntityStatus(EntityStatusCallback callback) { m_onBroadcastEntityStatus = std::move(callback); }

    // ========== 实体动画广播回调 ==========

    /**
     * @brief 实体动画广播回调类型
     *
     * 当实体动画事件需要广播给客户端时触发。
     * @param entityId 实体ID
     * @param animation 动画类型
     */
    using EntityAnimationCallback = std::function<void(EntityInstanceId entityId, u8 animation)>;

    void setOnBroadcastEntityAnimation(EntityAnimationCallback callback)
    {
        m_onBroadcastEntityAnimation = std::move(callback);
    }

    /// 实体受伤动画广播回调（携带 hurtDir，对应 MC ClientboundHurtAnimationPacket）。
    using HurtAnimationCallback = std::function<void(EntityInstanceId entityId, f32 hurtDir)>;

    void setOnBroadcastHurtAnimation(HurtAnimationCallback callback)
    {
        m_onBroadcastHurtAnimation = std::move(callback);
    }

    // ========== 实体拴绳链接广播回调 ==========

    /**
     * @brief 实体拴绳链接广播回调类型
     *
     * 当拴绳状态变更时广播给客户端，用于绳索渲染同步。
     * 参数：被拴实体ID、持有者实体ID（0=解除拴绳）
     */
    using SetEntityLinkCallback = std::function<void(EntityInstanceId entityId, EntityInstanceId linkedEntityId)>;

    void setOnBroadcastSetEntityLink(SetEntityLinkCallback callback)
    {
        m_onBroadcastSetEntityLink = std::move(callback);
    }

    // ========== 世界事件回调 ==========

    /**
     * @brief 世界事件广播回调类型
     *
     * 当服务端需要广播世界事件给玩家时调用。
     * 参数：事件ID、位置、数据
     */
    using WorldEventCallback = std::function<void(i32 eventId, i32 x, i32 y, i32 z, i32 data)>;

    void setOnBroadcastWorldEvent(WorldEventCallback callback) { m_onBroadcastWorldEvent = std::move(callback); }

    // ========== 方块事件广播回调 ==========

    /**
     * @brief 方块事件广播回调类型
     *
     * 当服务端方块事件被成功执行后，广播给附近客户端。
     * 参数：位置x/y/z、事件参数A、事件参数B、方块状态ID
     */
    using BlockEventCallback = std::function<void(i32 x, i32 y, i32 z, u8 paramA, u8 paramB, u32 blockStateId)>;

    void setOnBroadcastBlockEvent(BlockEventCallback callback) { m_onBroadcastBlockEvent = std::move(callback); }

    // ========== 方块实体数据广播回调 ==========

    /**
     * @brief 方块实体数据广播回调类型
     *
     * 当服务端方块实体数据发生变化（如告示牌文本更新）时，将最新 NBT 数据
     * 广播给追踪该区块的客户端。参考 MC Java: Level.sendBlockUpdated。
     * 参数：方块位置
     */
    using BlockEntityBroadcastCallback = std::function<void(const BlockPos& pos)>;

    void setOnBroadcastBlockEntity(BlockEntityBroadcastCallback callback)
    {
        m_onBroadcastBlockEntity = std::move(callback);
    }

    /**
     * @brief 广播方块实体数据更新给附近客户端
     *
     * 方块实体数据变化后调用，触发将最新 NBT 快照发送给追踪该区块的客户端。
     * 实际的玩家筛选与包发送由上层（MinecraftServer）通过回调完成。
     *
     * 参考 MC Java: ServerLevel.sendBlockUpdated(BlockPos, BlockState, BlockState, int)
     *
     * @param pos 方块位置
     */
    void broadcastBlockEntity(const BlockPos& pos);

    /**
     * @brief 方块破坏进度回调类型
     *
     * 当服务端需要广播方块破坏进度动画给玩家时调用。
     * 对应 MC Java 中的 ServerLevel.destroyBlockProgress()。
     * 参数：破坏者实体ID、位置、进度（0-9 阶段，-1 移除）
     */
    using BlockBreakProgressCallback =
        std::function<void(EntityInstanceId breakerId, i32 x, i32 y, i32 z, i32 progress)>;

    void setOnDestroyBlockProgress(BlockBreakProgressCallback callback)
    {
        m_onDestroyBlockProgress = std::move(callback);
    }

    // ========== 爆炸广播回调 ==========

    /**
     * @brief 爆炸广播回调类型
     *
     * 当服务端需要广播爆炸事件给玩家时调用。
     * 参数：爆炸位置、威力、受影响方块列表、玩家击退映射
     */
    using ExplosionBroadcastCallback = std::function<void(const Vector3& position,
        f32 strength,
        const std::vector<BlockPos>& affectedBlocks,
        const std::unordered_map<u64, Vector3>& playerKnockback)>;

    void setOnBroadcastExplosion(ExplosionBroadcastCallback callback) { m_onBroadcastExplosion = std::move(callback); }

    // ========== 命令执行回调 ==========

    /**
     * @brief 命令执行回调类型
     *
     * 当实体需要执行命令时调用（如命令方块矿车）。
     * 参数：命令字符串、执行位置、权限级别
     * 返回：命令执行结果码（成功返回正整数，失败返回0）
     */
    using CommandExecuteCallback =
        std::function<i32(const std::string& command, const Vector3d& position, i32 permissionLevel)>;

    void setOnExecuteCommand(CommandExecuteCallback callback) { m_onExecuteCommand = std::move(callback); }

    // ========== IWorld 接口实现 ==========

    void addParticle(particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity) override;

    void addParticle(particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count) override;

    /**
     * @brief 添加振动粒子（带目标位置来源和到达时间）
     *
     * 振动粒子从 pos 飞向 targetSource 解析的位置，飞行时间为 arrivalInTicks 个 tick。
     * 与普通 addParticle 不同，振动粒子需要携带目标位置来源信息以实现定向飞行效果，
     * 并在网络上按 MC Java 1.21.11 VibrationParticleOption 格式序列化 PositionSource。
     *
     * @param pos 粒子起始位置（振动源位置）
     * @param targetSource 粒子飞向的目标位置来源（监听器位置来源）
     * @param arrivalInTicks 到达目标的 tick 数
     */
    void addVibrationParticle(const Vector3& pos, const gameevent::PositionSource& targetSource, i32 arrivalInTicks);

    /**
     * @brief 添加轨迹粒子（带目标位置、颜色和持续时间）
     *
     * 轨迹粒子从 pos 飞向 targetPosition，飞行持续 durationInTicks 个 tick。
     * 与普通 addParticle 不同，轨迹粒子需要携带目标位置/颜色/持续时间信息。
     * 主要用于眼眸花状态切换的转换粒子效果。
     *
     * 重写 IWorld::addTrailParticle，通过回调将粒子广播给附近玩家。
     *
     * @param pos 粒子起始位置
     * @param targetPosition 粒子飞向的目标位置
     * @param color 粒子颜色（ARGB 格式）
     * @param durationInTicks 飞行持续时间（tick 数）
     */
    void addTrailParticle(const Vector3& pos, const Vector3d& targetPosition, u32 color, i32 durationInTicks) override;

    /**
     * @brief 添加带颜色的实体效果粒子
     *
     * 与普通 addParticle 不同，EntityEffect 粒子需要携带 ARGB 颜色信息。
     * 用于 BellBlockEntity 共振、药水效果等场景。
     *
     * @param pos 粒子位置
     * @param velocity 粒子速度
     * @param offset 随机偏移范围
     * @param count 粒子数量
     * @param color 粒子颜色（ARGB 格式）
     */
    void addEntityEffectParticle(
        const Vector3& pos, const Vector3& velocity, const Vector3& offset, u32 count, u32 color) override;

    /**
     * @brief 添加方块粒子（携带方块状态 ID）
     *
     * 服务端通过 LevelParticles 的 ParticleOptions(Block) 编码 blockStateId，
     * 广播给附近玩家。客户端解码后调用 ClientWorld::addBlockParticle 生成粒子。
     *
     * @param type 粒子类型（必须为 requiresBlockState 返回 true 的类型）
     * @param pos 粒子位置
     * @param velocity 粒子速度
     * @param blockState 方块状态（用于粒子纹理和颜色）
     */
    void addBlockParticle(particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const BlockState& blockState) override;

    /**
     * @brief 添加物品粒子（携带物品堆）
     *
     * 服务端通过 LevelParticles 的 ParticleOptions(Item) 编码 ItemStack，
     * 广播给附近玩家。客户端解码后通过粒子数据管线生成物品粒子。
     *
     * @param type 粒子类型（必须为 requiresItemData 返回 true 的类型）
     * @param pos 粒子位置
     * @param velocity 粒子速度
     * @param itemStack 物品堆（用于粒子纹理）
     */
    void addItemParticle(particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const ItemStack& itemStack) override;

    [[nodiscard]] bool shouldSpawnParticleAt(const Vector3& pos, f32 maxDistance = 256.0f) const override;

    // ========== 实体状态广播 (IWorld override) ==========

    void broadcastEntityStatus(EntityInstanceId entityId, u8 status) override;

    void broadcastEntityAnimation(EntityInstanceId entityId, u8 animation) override;

    void broadcastHurtAnimation(EntityInstanceId entityId, f32 hurtDir) override;

    void broadcastSetEntityLink(EntityInstanceId entityId, EntityInstanceId linkedEntityId) override;

    void broadcastExplosion(const Vector3& position,
        f32 strength,
        const std::vector<BlockPos>& affectedBlocks,
        const std::unordered_map<u64, Vector3>& playerKnockback) override;

    // ========== 爆炸 ==========

    void createExplosion(const Vector3& position,
        f32 radius,
        world::explosion::ExplosionMode mode = world::explosion::ExplosionMode::Destroy,
        bool causesFire = false,
        Entity* source = nullptr) override;

    void createExplosionWithSource(const Vector3& position,
        f32 radius,
        world::explosion::ExplosionMode mode,
        bool causesFire,
        Entity* source,
        const DamageSource* damageSource) override;

    void createExplosionWithContext(const Vector3& position,
        f32 radius,
        world::explosion::ExplosionMode mode,
        bool causesFire,
        Entity* source,
        std::unique_ptr<world::explosion::ExplosionContext> context) override;

    // ========== 命令执行 (IWorld override) ==========

    /**
     * @brief 执行命令
     *
     * 通过 CommandRegistry 执行命令，用于命令方块矿车等实体执行命令。
     *
     * @param command 命令字符串（可包含或不包含 '/' 前缀）
     * @param position 命令执行位置
     * @param permissionLevel 权限级别（0-4，命令方块矿车使用2）
     * @return 命令执行结果码（成功返回正整数，失败返回0）
     */
    [[nodiscard]] i32 executeCommand(
        const std::string& command, const Vector3d& position, i32 permissionLevel) override;

    /**
     * @brief 设置掉落表管理器
     *
     * 掉落表管理器由 MinecraftServer 持有，ServerWorld 通过此方法获取引用。
     * 用于爆炸时的方块掉落生成和方块实体的战利品表填充。
     *
     * @param lootTableManager 掉落表管理器指针（非拥有）
     */
    void setLootTableManager(const loot::LootTableManager* lootTableManager) { m_lootTableManager = lootTableManager; }

    /**
     * @brief 获取战利品表管理器（IWorld 接口实现）
     *
     * 只有 ServerWorld 会返回有效的指针。
     * 用于方块实体填充战利品表。
     *
     * @return LootTableManager指针，如果不存在返回nullptr
     */
    [[nodiscard]] const loot::LootTableManager* lootTableManager() const noexcept override
    {
        return m_lootTableManager;
    }

    // ========== 物理引擎 ==========

    [[nodiscard]] PhysicsEngine* physicsEngine() noexcept override { return m_physicsEngine.get(); }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const noexcept override { return m_physicsEngine.get(); }

    // ========== 碰撞缓存 ==========

    /**
     * @brief 获取碰撞缓存
     *
     * 提供直接访问碰撞缓存的接口，用于需要手动操作碰撞缓存的场景。
     */
    [[nodiscard]] physics::CollisionCache* collisionCache() noexcept { return m_collisionCache.get(); }
    [[nodiscard]] const physics::CollisionCache* collisionCache() const noexcept { return m_collisionCache.get(); }

    // ========== ICollisionWorld 接口实现 ==========

    [[nodiscard]] const ChunkData* getChunkAt(ChunkCoord x, ChunkCoord z) const noexcept override
    {
        return getChunk(x, z);
    }

    // ========== 实体管理 ==========

    /**
     * @brief 移除实体
     *
     * 注意：此方法不仅从 EntityManager 移除实体，还会取消实体追踪。
     * 调用者应使用此方法而非直接调用 entityManager().removeEntity()，
     * 以确保实体追踪器状态正确更新。
     *
     * @param id 要移除的实体ID
     * @return 被移除的实体所有权，如果实体不存在返回 nullptr
     */
    std::unique_ptr<Entity> removeEntity(EntityInstanceId id);
    [[nodiscard]] EntityInstanceId spawnEntity(std::unique_ptr<Entity> entity) override;
    [[nodiscard]] Entity* getEntity(EntityInstanceId id) override;
    [[nodiscard]] const Entity* getEntity(EntityInstanceId id) const override;
    [[nodiscard]] Entity* getEntityByUuid(const std::string& uuid) override;
    [[nodiscard]] const Entity* getEntityByUuid(const std::string& uuid) const override;

    [[nodiscard]] EntityManager& entityManager() noexcept { return m_entityManager; }
    [[nodiscard]] const EntityManager& entityManager() const noexcept { return m_entityManager; }

    [[nodiscard]] EntityTracker& entityTracker() noexcept { return m_entityTracker; }
    [[nodiscard]] const EntityTracker& entityTracker() const noexcept { return m_entityTracker; }

    [[nodiscard]] server::ItemPickupManager& itemPickupManager() noexcept { return m_itemPickupManager; }
    [[nodiscard]] const server::ItemPickupManager& itemPickupManager() const noexcept { return m_itemPickupManager; }

    // ========== 区块生成实体 ==========

    i32 spawnEntitiesFromChunkGeneration(const std::vector<SpawnedEntityData>& entities);

    // ========== 实体区块持久化 ==========

    /**
     * @brief 区块加载时恢复存储中的实体
     *
     * 从 EntityStorageManager 加载区块内所有实体并通过 spawnEntity() 注入世界。
     * 同时在 EntityChunkTracker 中注册实体归属。
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     */
    void onChunkLoaded(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 区块卸载前保存并移除区块内实体
     *
     * 将区块内所有实体保存到 EntityStorageManager，然后从 EntityManager 移除。
     * 同时在 EntityChunkTracker 中注销实体归属。
     *
     * @param x 区块 X 坐标
     * @param z 区块 Z 坐标
     */
    void onChunkUnloading(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 获取实体区块跟踪器
     */
    [[nodiscard]] EntityChunkTracker& entityChunkTracker() noexcept { return m_entityChunkTracker; }
    [[nodiscard]] const EntityChunkTracker& entityChunkTracker() const noexcept { return m_entityChunkTracker; }

    // ========== Tick管理 ==========

    [[nodiscard]] world::tick::TickManager& tickManager() noexcept override { return *m_tickManager; }
    [[nodiscard]] const world::tick::TickManager& tickManager() const noexcept override { return *m_tickManager; }

    // ========== StarLightLightingProvider 接口实现 ==========

    [[nodiscard]] IChunk* getChunkForLight(ChunkCoord x, ChunkCoord z) override;
    [[nodiscard]] const IChunk* getChunkForLight(ChunkCoord x, ChunkCoord z) const override;
    [[nodiscard]] const BlockState* getBlockStateForLight(const BlockPos& pos) const override;
    [[nodiscard]] IWorld* getWorld() noexcept override;
    [[nodiscard]] const IWorld* getWorld() const noexcept override;
    void markLightChanged(LightType type, const SectionPos& pos) override;
    [[nodiscard]] bool hasSkyLight() const override;
    [[nodiscard]] i32 getMinBuildHeight() const noexcept override;
    [[nodiscard]] i32 getMaxBuildHeight() const noexcept override;
    [[nodiscard]] i32 getSectionCount() const noexcept override;

    // ========== 运行时光照 worker flush 队列（③-1） ==========

    /**
     * @brief worker 线程入队 dirty section（待主线程 flush）
     *
     * RuntimeLightTask / ChunkLoadLightTask 在 worker 线程完成光照传播后调用：
     * 把 provider 收集的 dirty section 列表入此队列。主线程下一 tick 开头
     * _drainPendingLightFlushes 时逐项调真正的 markLightChanged
     * （_syncLightDataToChunk + m_onLightChanged 网络包）。
     * 线程安全：内部持 m_pendingLightFlushesMutex。
     *
     * @param dirtySections worker 传播期间收集的 (光照类型, 段坐标) 列表
     */
    void _enqueueLightFlush(std::vector<std::pair<LightType, SectionPos>> dirtySections);

    // ========== 区块加载光照统一调度（③-2b） ==========

    /**
     * @brief 入队区块加载光照任务（主线程调用）
     *
     * 由 chunkLoadedCallback 调用。对中心区块 add LIGHT 票据（level=Full 保活）后，
     * 构造 ChunkLoadLightTask 提交到 UniversalWorkerPool 区域互斥池（writeRadius=2）。
     * 区块加载光照与运行时方块变更、LIGHT 生成阶段同池同 writeRadius，重叠 5×5 区域
     * 的 nibble 写必被区域锁串行 → 满足 SWMRNibbleArray 单写者语义，可安全删 m_mutex。
     *
     * executor 为空（启动早期/测试环境）时 fallback 主线程同步执行 _executeChunkLoadLight。
     *
     * @param x 区块 X
     * @param z 区块 Z
     */
    void enqueueChunkLoadLight(ChunkCoord x, ChunkCoord z);

    /**
     * @brief worker 线程入队区块发送续延（待主线程 send）
     *
     * ChunkLoadLightTask 在 worker 完成光照后调用：把 chunk 坐标入此队列。
     * 主线程 tick 在 _drainPendingLightFlushes 之后、_drainPendingChunkSends 中逐项调
     * chunkSendManager()->sendChunkToTrackingPlayers + removeLightTicket——serialize 读
     * 已 flush 的 ChunkSection nibble，保证客户端收到正确光照而非全黑区块；
     * removeLightTicket 释放 enqueueChunkLoadLight 时 add 的票据。
     * 线程安全：内部持 m_pendingChunkSendsMutex。
     *
     * 任务被取消（onCancel）时也经此队列携带坐标回主线程 removeLightTicket，
     * 防止票据泄漏。
     *
     * @param x 区块 X
     * @param z 区块 Z
     */
    void _enqueueChunkSend(ChunkCoord x, ChunkCoord z);

    /**
     * @brief 是否有待处理的光照工作
     *
     * 主线程调用（无锁近似，各队列 empty() 的瞬态读）。用于外部查询是否需要继续 tick。
     * 包括：运行时方块变更延迟队列、worker flush 队列、区块发送续延队列。
     */
    [[nodiscard]] bool hasPendingLightWork() const noexcept;

    // ========== 光照管理 ==========

    [[nodiscard]] WorldLightManager* lightManager() noexcept { return m_lightManager.get(); }
    [[nodiscard]] const WorldLightManager* lightManager() const noexcept { return m_lightManager.get(); }
    void setLightManager(std::unique_ptr<WorldLightManager> manager) { m_lightManager = std::move(manager); }

    // ========== 区块管理器设置 ==========

    void setChunkManager(std::unique_ptr<ServerChunkManager> manager);

    // ========== 光照变化回调 ==========

    void setOnLightChanged(std::function<void(LightType, const SectionPos&)> callback)
    {
        if (m_onLightChanged) {
            throw std::runtime_error("Light change callback already set");
        }
        m_onLightChanged = std::move(callback);
    }

    // ========== 方块变化回调 ==========

    void setOnBlockChanged(std::function<void(const BlockPos&, u32)> callback)
    {
        if (m_onBlockChanged) {
            throw std::runtime_error("Block change callback already set");
        }
        m_onBlockChanged = std::move(callback);
    }

    // ========== 出生点管理 ==========

    /**
     * @brief 初始化世界出生点
     *
     * 在世界初始化后调用，在 (0, 0) 区块查找合适的出生位置。
     * 如果找不到有效位置，使用默认值 (0, 64, 0)。
     */
    void initializeWorldSpawn();

    /**
     * @brief 获取世界出生点
     *
     * @return 世界出生点坐标
     */
    [[nodiscard]] Vector3d worldSpawnPoint() const noexcept { return m_worldSpawnPoint; }

    /**
     * @brief 设置世界出生点
     *
     * @param pos 新的出生点位置
     * @param angle 新的出生点朝向（度），默认 0.0f
     */
    void setWorldSpawnPoint(const Vector3d& pos, f32 angle = 0.0f) noexcept
    {
        m_worldSpawnPoint = pos;
        m_spawnAngle = angle;
    }

    /**
     * @brief 获取世界出生点朝向
     *
     * @return 出生点朝向角度（度）
     */
    [[nodiscard]] f32 spawnAngle() const noexcept { return m_spawnAngle; }

    /**
     * @brief 设置世界出生点朝向
     *
     * @param angle 出生点朝向角度（度）
     */
    void setSpawnAngle(f32 angle) noexcept { m_spawnAngle = angle; }

    /**
     * @brief 应用 level.dat 读取到的运行时世界状态
     *
     * 仅在世界初始化完成且共享存储已就绪后调用。
     */
    void applyLevelRuntimeData(const world::storage::LevelRuntimeData& runtimeData);

    // ========== 调试模式 ==========

    /**
     * @brief 检查是否为调试世界
     *
     * 通过检查区块生成器类型来判断是否为调试世界。
     * 调试世界特性：
     * - 无法放置或破坏方块
     * - 计划刻不会执行
     * - 方块状态由调试生成器决定
     *
     * @return true 如果使用调试区块生成器
     */
    [[nodiscard]] bool isDebugWorld() const;

    // ========== 世界边界 ==========

    /**
     * @brief 获取世界边界
     *
     * @return 世界边界对象
     */
    [[nodiscard]] world::border::WorldBorder& worldBorder() noexcept override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const noexcept override { return m_worldBorder; }

    // ========== 村庄管理 ==========

    [[nodiscard]] ::mc::world::village::VillageManager* villageManager() noexcept override
    {
        return m_villageManager.get();
    }
    [[nodiscard]] const ::mc::world::village::VillageManager* villageManager() const noexcept override
    {
        return m_villageManager.get();
    }

    // ========== 袭击管理 ==========

    [[nodiscard]] ::mc::world::village::raid::RaidManager* raidManager() noexcept override
    {
        return m_raidManager.get();
    }
    [[nodiscard]] const ::mc::world::village::raid::RaidManager* raidManager() const noexcept override
    {
        return m_raidManager.get();
    }

    // ========== 末影龙战斗管理 ==========

    /**
     * @brief 获取末影龙战斗管理器
     *
     * 只有末地维度的ServerWorld会返回有效的指针，其他维度返回nullptr。
     * 用于末影龙死亡后放置龙蛋、生成折跃门等战斗奖励逻辑。
     *
     * @return EndDragonFight指针，如果非末地维度返回nullptr
     */
    [[nodiscard]] EndDragonFight* dragonFight() noexcept override { return m_dragonFight.get(); }
    [[nodiscard]] const EndDragonFight* dragonFight() const noexcept override { return m_dragonFight.get(); }

    // ========== 地图数据管理 ==========

    [[nodiscard]] world::map::MapDataManager* mapDataManager() noexcept override { return m_mapDataManager.get(); }
    [[nodiscard]] const world::map::MapDataManager* mapDataManager() const noexcept override
    {
        return m_mapDataManager.get();
    }

    /**
     * @brief 通知世界方块被放置
     *
     * 重写 IWorld::onBlockPlaced()，发布 BlockPlaceEvent 用于进度触发。
     */
    void onBlockPlaced(PlayerId playerId, const BlockPos& pos, const BlockState* state, const ItemStack* item) override;

    /**
     * @brief 通知世界僵尸村民被治愈
     *
     * 重写 IWorld::onZombieVillagerCured()，发布 CuredZombieVillagerEvent 用于进度触发。
     *
     * @param starterUuid 治愈发起者玩家UUID（可能为空）
     * @param zombie 治愈前的僵尸村民实体
     * @param villager 治愈后的村民实体
     */
    void onZombieVillagerCured(const std::string& starterUuid, Entity* zombie, Entity* villager) override;

    /**
     * @brief 通知世界引雷附魔触发
     *
     * 重写 IWorld::onChanneledLightning()，发布 ChanneledLightningEvent 用于进度触发。
     *
     * @param casterId 施法者ID（引雷附魔的玩家）
     * @param victims 被闪电击中的实体列表
     */
    void onChanneledLightning(PlayerId casterId, const std::vector<Entity*>& victims) override;

    /**
     * @brief 通知世界动物繁殖
     *
     * 重写 IWorld::onBredAnimals()，发布 BredAnimalsEvent 用于进度触发。
     *
     * @param playerId 繁殖发起者玩家ID（喂食动物的玩家）
     * @param child 幼体实体
     * @param parent1 父母1
     * @param parent2 父母2
     */
    void onBredAnimals(PlayerId playerId, Entity* child, Entity* parent1, Entity* parent2) override;

    /**
     * @brief 通知世界玩家与村民完成交易
     *
     * 重写 IWorld::onVillagerTrade()，发布 VillagerTradeEvent 用于进度触发。
     *
     * @param playerId 交易玩家ID
     * @param villager 商人实体（村民或流浪商人）
     * @param resultItem 交易结果物品（玩家获得的物品）
     * @param paymentItem 交易支付物品（玩家付出的物品）
     */
    void onVillagerTrade(
        PlayerId playerId, Entity* villager, const ItemStack& resultItem, const ItemStack& paymentItem) override;

    /**
     * @brief 通知世界玩家物品销毁
     *
     * 重写 IWorld::onPlayerDestroyItem()，发布 PlayerDestroyItemEvent 用于进度触发。
     *
     * @param playerId 玩家ID
     * @param item 销毁前的物品副本
     * @param slot 物品所在槽位（主手=0，副手=40，其他为物品栏槽位，-1表示未知）
     * @param hand 使用的手（MainHand 或 OffHand）
     */
    void onPlayerDestroyItem(PlayerId playerId, const ItemStack& item, i32 slot, Hand hand) override;

    /**
     * @brief 通知世界玩家消耗物品
     *
     * 重写 IWorld::onConsumeItem()，发布 ConsumeItemEvent 用于进度触发。
     *
     * @param playerId 玩家ID
     * @param item 消耗的物品
     */
    void onConsumeItem(PlayerId playerId, const ItemStack& item) override;

    /**
     * @brief 通知世界物品耐久度变化
     *
     * 重写 IWorld::onItemDurabilityChange()，发布 ItemDurabilityEvent 用于进度触发。
     *
     * @param playerId 玩家ID
     * @param item 物品
     * @param oldDurability 旧耐久度
     * @param newDurability 新耐久度
     */
    void onItemDurabilityChange(
        PlayerId playerId, const ItemStack& item, i32 oldDurability, i32 newDurability) override;

    /**
     * @brief 通知世界附魔完成
     *
     * 重写 IWorld::onEnchantItem()，发布 EnchantItemEvent 用于进度触发。
     *
     * @param playerId 玩家ID
     * @param item 附魔的物品
     * @param levels 消耗的经验等级
     */
    void onEnchantItem(PlayerId playerId, const ItemStack& item, i32 levels) override;

    /**
     * @brief 通知世界桶填充完成
     *
     * 重写 IWorld::onFilledBucket()，发布 FilledBucketEvent 用于进度触发。
     *
     * @param playerId 玩家ID
     * @param bucket 填充后的桶物品
     */
    void onFilledBucket(PlayerId playerId, const ItemStack& bucket) override;

    /**
     * @brief 通知世界玩家进入方块
     *
     * 重写 IWorld::onEnterBlock()，发布 EnterBlockEvent 用于进度触发。
     *
     * @param playerId 玩家ID
     * @param pos 方块位置
     * @param state 方块状态
     */
    void onEnterBlock(PlayerId playerId, const BlockPos& pos, const BlockState* state) override;

    /**
     * @brief 通知世界玩家在方块上滑落
     *
     * 重写 IWorld::onSlideDownBlock()，发布 SlideDownBlockEvent 用于进度触发。
     *
     * @param playerId 玩家ID
     * @param pos 方块位置
     * @param state 方块状态
     */
    void onSlideDownBlock(PlayerId playerId, const BlockPos& pos, const BlockState* state) override;

    /**
     * @brief 通知世界蜂巢被破坏
     *
     * 重写 IWorld::onBeeNestDestroyed()，发布 BeeNestDestroyedEvent 用于进度触发。
     *
     * @param playerId 玩家ID
     * @param pos 方块位置
     * @param state 方块状态
     * @param tool 使用的工具
     * @param numBeesInside 蜂巢内的蜜蜂数量
     */
    void onBeeNestDestroyed(PlayerId playerId,
        const BlockPos& pos,
        const BlockState* state,
        const ItemStack& tool,
        i32 numBeesInside) override;

    /**
     * @brief 通知世界动物被驯服
     *
     * 重写 IWorld::onTameAnimal()，发布 TameAnimalEvent 用于进度触发。
     *
     * @param playerId 驯服动物的玩家ID
     * @param animal 被驯服的动物实体
     */
    void onTameAnimal(PlayerId playerId, Entity* animal) override;

    /**
     * @brief 通知世界实体被召唤
     *
     * 重写 IWorld::onSummonedEntity()，发布 SummonedEntityEvent 用于进度触发。
     *
     * @param playerId 召唤实体的玩家ID
     * @param entity 被召唤的实体
     */
    void onSummonedEntity(PlayerId playerId, Entity* entity) override;

    // ========== 结构定位 ==========

    /**
     * @brief 查找最近的结构
     *
     * 在指定范围内搜索指定结构的最近位置。
     * 使用 StructureSet/StructurePlacement 系统进行定位，
     * 支持 RandomSpread 和 ConcentricRings 两种放置策略。
     *
     * @param center 搜索中心位置
     * @param structureId 结构资源位置 ID（如 minecraft:village_plains）
     * @param maxDistance 最大搜索距离（格）
     * @param skipExisting 是否跳过已找到的结构（用于定位命令的多次搜索）
     * @return 最近结构位置，如果未找到返回空
     */
    [[nodiscard]] std::optional<BlockPos> findNearestStructure(const BlockPos& center,
        const ResourceLocation& structureId,
        i32 maxDistance,
        bool skipExisting = false) override;

    /**
     * @brief 按结构标签查找最近的结构
     *
     * 对应 MC 1.21.11 ServerLevel.findNearestMapStructure(TagKey<Structure>, BlockPos, int, boolean)。
     *
     * 遍历标签中的所有结构 ID，对每个结构调用 findNearestStructure，
     * 返回所有候选中距离最近的位置。
     *
     * @param center 搜索中心位置
     * @param tagId 结构标签资源位置（如 minecraft:dolphin_located）
     * @param maxDistance 最大搜索距离（格）
     * @param skipExisting 是否跳过已找到的结构
     * @return 最近结构位置，如果未找到返回空
     */
    [[nodiscard]] std::optional<BlockPos> findNearestMapStructure(
        const BlockPos& center, const ResourceLocation& tagId, i32 maxDistance, bool skipExisting = false) override;

private:
    void _syncLightDataToChunk(LightType type, const SectionPos& pos);

    /// 主线程 tick 开头调用：swap 出 m_pendingLightFlushes，逐项调 markLightChanged
    void _drainPendingLightFlushes();

    /// 主线程 tick 调用（在 _drainPendingLightFlushes 之后）：swap 出 m_pendingChunkSends，
    /// 逐区块调 chunkSendManager()->sendChunkToTrackingPlayers + removeLightTicket。
    /// 顺序关键：必须在 flush 之后——serialize 读已 flush 的 ChunkSection nibble。
    void _drainPendingChunkSends();

    /// 区块加载光照核心逻辑（worker 任务与 fallback 共用）。
    /// 主线程 fallback 路径（executor 为空）同步调用，与 ChunkLoadLightTask::execute 同构。
    void _executeChunkLoadLight(RuntimeLightingProvider& provider, ChunkCoord x, ChunkCoord z);

    /// 将反序列化得到的实体注入世界并挂载乘客。
    /// common 路径（takeLoadedEntities）与 native 路径（EntityStorageManager）共用：
    /// spawn → attachPassengers →（仅 native 路径）按实体真实所在区块重注册 EntityChunkTracker。
    /// entityChunk 为 nullopt 表示无需重注册（实体必属当前区块）；非 nullopt 时若与 (x,z) 不符则重注册。
    /// 必须在 spawnEntity 之前从 entity 读取坐标（spawnEntity 会 move 走所有权）。
    void _spawnLoadedEntity(std::unique_ptr<Entity> entity,
        ChunkCoord x,
        ChunkCoord z,
        std::optional<std::pair<ChunkCoord, ChunkCoord>> entityChunk);

    /// 三个 createExplosion 重载共用：构造 Explosion（context 非空走 9 参重载，否则 8 参）→
    /// explode → 广播给爆炸点 64 格范围内的玩家。
    void _explodeAndBroadcast(const Vector3& position,
        f32 radius,
        world::explosion::ExplosionMode mode,
        bool causesFire,
        Entity* source,
        std::unique_ptr<DamageSource> damageSource,
        std::unique_ptr<world::explosion::ExplosionContext> context);

private:
    ServerWorldConfig m_config;
    world::storage::SingleLevelStorageManager* m_storage = nullptr; ///< 世界级共享单存档存储门面（不拥有）
    IServer* m_server = nullptr;                                    ///< 服务器接口（不拥有，由 MinecraftServer 注入）
    std::unique_ptr<ServerChunkManager> m_chunkManager;
    EntityManager m_entityManager;
    EntityTracker m_entityTracker;
    EntityChunkTracker m_entityChunkTracker;
    std::unique_ptr<PhysicsEngine> m_physicsEngine;
    std::unique_ptr<physics::CollisionCache> m_collisionCache;
    std::unique_ptr<world::tick::TickManager> m_tickManager;
    std::unique_ptr<WorldLightManager> m_lightManager;
    ServerLightQueue m_lightQueue; ///< 运行时方块变更光照延迟队列（主线程批处理）

    /// worker 完成的 dirty section 待主线程 flush 队列（③-1：worker 传播→主线程 flush visible）
    std::mutex m_pendingLightFlushesMutex;
    std::vector<std::pair<LightType, SectionPos>> m_pendingLightFlushes;

    /// 区块加载光照完成后的区块发送续延队列（③-2b：worker 光照→主线程 send）。
    /// worker 在 _drainPendingLightFlushes 之后 drain：send serialize 读已 flush nibble + removeLightTicket。
    std::mutex m_pendingChunkSendsMutex;
    std::vector<std::pair<ChunkCoord, ChunkCoord>> m_pendingChunkSends;
    std::unique_ptr<WeatherManager> m_weatherManager;
    std::unique_ptr<world::map::MapDataManager> m_mapDataManager;
    server::ItemPickupManager m_itemPickupManager;
    core::TimeManager* m_timeManager = nullptr;       // 外部引用，不拥有
    std::function<Difficulty()> m_difficultyCallback; ///< 难度获取回调（从 MinecraftServer 获取）
    bool m_initialized = false;
    Vector3d m_worldSpawnPoint{0.0, static_cast<f64>(world::SEA_LEVEL) + 1.0, 0.0}; // 世界出生点
    f32 m_spawnAngle = 0.0f;                                                        // 世界出生点朝向（度）

    OpenContainerCallback m_onOpenContainer;

    // 村庄和袭击系统
    std::unique_ptr<::mc::world::village::VillageManager> m_villageManager;
    std::unique_ptr<::mc::world::village::raid::RaidManager> m_raidManager;

    // 末影龙战斗管理器（仅末地维度创建）
    std::unique_ptr<EndDragonFight> m_dragonFight;

    std::function<void(LightType, const SectionPos&)> m_onLightChanged;
    std::function<void(const BlockPos&, u32)> m_onBlockChanged;
    std::function<void(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32)> m_onPlaySound;
    ParticleBroadcastCallback m_onBroadcastParticle;
    VibrationParticleBroadcastCallback m_onBroadcastVibrationParticle;
    TrailParticleBroadcastCallback m_onBroadcastTrailParticle;
    EntityEffectParticleBroadcastCallback m_onBroadcastEntityEffectParticle;
    BlockParticleBroadcastCallback m_onBroadcastBlockParticle;
    ItemParticleBroadcastCallback m_onBroadcastItemParticle;
    EntityStatusCallback m_onBroadcastEntityStatus;
    EntityAnimationCallback m_onBroadcastEntityAnimation;
    HurtAnimationCallback m_onBroadcastHurtAnimation;
    SetEntityLinkCallback m_onBroadcastSetEntityLink;
    WorldEventCallback m_onBroadcastWorldEvent;
    BlockEventCallback m_onBroadcastBlockEvent;            ///< 方块事件广播回调
    BlockEntityBroadcastCallback m_onBroadcastBlockEntity; ///< 方块实体数据广播回调
    BlockBreakProgressCallback m_onDestroyBlockProgress;
    ExplosionBroadcastCallback m_onBroadcastExplosion;
    CommandExecuteCallback m_onExecuteCommand; ///< 命令执行回调

    // 随机刻系统
    math::Random m_random; ///< 世界随机数生成器
    i64 m_updateLCG = 0;   ///< 用于随机刻位置的 LCG 状态

    // 游戏规则
    world::gamerule::GameRules m_gameRules; ///< 游戏规则管理器

    // 游戏事件分发器
    std::unique_ptr<gameevent::GameEventDispatcher> m_gameEventDispatcher; ///< 游戏事件分发器

    // 幽匿方块实体振动系统管理器
    SculkVibrationManager m_sculkVibrationManager;

    // 方块事件队列
    std::vector<BlockEventData> m_blockEvents;             ///< 待处理方块事件队列（去重有序）
    std::vector<BlockEventData> m_blockEventsToReschedule; ///< 延迟到下tick处理的事件

    // 世界边界
    world::border::WorldBorder m_worldBorder; ///< 世界边界

    // 掉落表管理器（非拥有，由 MinecraftServer 持有）
    const loot::LootTableManager* m_lootTableManager = nullptr;
};

} // namespace server
} // namespace mc
