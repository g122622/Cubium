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
#include "common/world/border/WorldBorder.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/map/MapDataManager.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/world/village/VillageManager.hpp"
#include "common/world/village/raid/RaidManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/entity/EntityChunkTracker.hpp"
#include "server/world/entity/EntityTracker.hpp"
#include "server/world/entity/ItemPickupManager.hpp"
#include "server/world/spawn/VillageSiege.hpp"
#include "server/world/weather/WeatherManager.hpp"
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

namespace mc {

// 前向声明
struct SpawnedEntityData;
class INamedContainerProvider;

namespace client::renderer::trident::particle {
enum class ParticleTypeId : u16;
}

namespace server::core {
class TimeManager; // 前向声明
}

namespace loot {
class LootTableManager; // 前向声明
}

namespace server {

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

    /**
     * @brief 触发共享存储执行全量保存
     *
     * 该方法会直接委托给世界级共享 SingleLevelStorageManager，
     * 因此会一次性保存所有维度和玩家数据。
     * 适用于 `/save-all` 这类显式全量保存入口。
     */
    Result<size_t> saveAll();

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

private:
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
    [[nodiscard]] bool isClientSide() noexcept override { return false; }

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

    // ========== 类型转换 ==========

    [[nodiscard]] ServerWorld* asServerWorld() noexcept override { return this; }
    [[nodiscard]] const ServerWorld* asServerWorld() const noexcept override { return this; }

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

    // ========== 方块更新通知 ==========

    void notifyBlockUpdate(const BlockPos& pos) override;

    // ========== 容器打开回调 ==========

    using OpenContainerCallback = std::function<bool(ContainerType, const BlockPos&, Player&)>;
    using OpenEntityContainerCallback = std::function<bool(INamedContainerProvider&, Player&)>;

    void setOnOpenContainer(OpenContainerCallback callback)
    {
        if (m_onOpenContainer) {
            throw std::runtime_error("Open container callback already set");
        }

        m_onOpenContainer = std::move(callback);
    }
    [[nodiscard]] bool openContainer(ContainerType type, const BlockPos& pos, Player& player) override;

    /**
     * @brief 设置实体容器打开回调
     * @param callback 回调函数
     */
    void setOnOpenEntityContainer(OpenEntityContainerCallback callback)
    {
        m_onOpenEntityContainer = std::move(callback);
    }

    /**
     * @brief 打开实体容器
     * @param provider 命名容器提供者（村民、矿车等）
     * @param player 玩家
     * @return 如果成功打开返回 true
     */
    [[nodiscard]] bool openEntityContainer(INamedContainerProvider& provider, Player& player) override;

    // ========== 粒子广播回调 ==========

    /**
     * @brief 粒子广播回调类型
     *
     * 当服务端需要广播粒子给玩家时调用。
     * 参数：粒子类型、位置、速度、偏移、数量
     */
    using ParticleBroadcastCallback = std::function<void(client::renderer::trident::particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count)>;

    void setOnBroadcastParticle(ParticleBroadcastCallback callback) { m_onBroadcastParticle = std::move(callback); }

    // ========== 实体状态广播回调 ==========

    /**
     * @brief 实体状态广播回调类型
     *
     * 当服务端需要广播实体状态事件给玩家时调用。
     * 参数：实体ID、状态码
     */
    using EntityStatusCallback = std::function<void(EntityId entityId, u8 status)>;

    void setOnBroadcastEntityStatus(EntityStatusCallback callback) { m_onBroadcastEntityStatus = std::move(callback); }

    // ========== 实体动画广播回调 ==========

    /**
     * @brief 实体动画广播回调类型
     *
     * 当实体动画事件需要广播给客户端时触发。
     * @param entityId 实体ID
     * @param animation 动画类型
     */
    using EntityAnimationCallback = std::function<void(EntityId entityId, u8 animation)>;

    void setOnBroadcastEntityAnimation(EntityAnimationCallback callback)
    {
        m_onBroadcastEntityAnimation = std::move(callback);
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

    // ========== 袭击事件回调 ==========

    /**
     * @brief 袭击事件广播回调类型
     *
     * 当袭击发生特定事件时调用，用于通知玩家。
     */
    using RaidEventCallback = std::function<void(i32 raidId, ///< 袭击 ID
        i32 eventType,                                       ///< 事件类型 (0=开始, 1=胜利, 2=失败, 3=波次开始)
        const BlockPos& pos,                                 ///< 相关位置
        i32 data                                             ///< 额外数据
        )>;

    void setOnRaidEvent(RaidEventCallback callback) { m_onRaidEvent = std::move(callback); }

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

    void addParticle(
        client::renderer::trident::particle::ParticleTypeId type, const Vector3& pos, const Vector3& velocity) override;

    void addParticle(client::renderer::trident::particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count) override;

    [[nodiscard]] bool shouldSpawnParticleAt(const Vector3& pos, f32 maxDistance = 256.0f) const override;

    // ========== 实体状态广播 (IWorld override) ==========

    void broadcastEntityStatus(EntityId entityId, u8 status) override;

    void broadcastEntityAnimation(EntityId entityId, u8 animation) override;

    // ========== 爆炸 ==========

    void createExplosion(const Vector3& position,
        f32 radius,
        world::explosion::ExplosionMode mode = world::explosion::ExplosionMode::Destroy,
        bool causesFire = false,
        Entity* source = nullptr) override;

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
    std::unique_ptr<Entity> removeEntity(EntityId id);
    [[nodiscard]] EntityId spawnEntity(std::unique_ptr<Entity> entity) override;
    [[nodiscard]] Entity* getEntity(EntityId id) override;
    [[nodiscard]] const Entity* getEntity(EntityId id) const override;

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
     */
    void setWorldSpawnPoint(const Vector3d& pos) noexcept { m_worldSpawnPoint = pos; }

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

    // ========== 地图数据管理 ==========

    [[nodiscard]] world::map::MapDataManager* mapDataManager() noexcept override { return m_mapDataManager.get(); }
    [[nodiscard]] const world::map::MapDataManager* mapDataManager() const noexcept override
    {
        return m_mapDataManager.get();
    }

    // ========== 袭击管理 ==========

    [[nodiscard]] ::mc::world::village::raid::RaidManager* raidManager() noexcept { return m_raidManager.get(); }
    [[nodiscard]] const ::mc::world::village::raid::RaidManager* raidManager() const noexcept
    {
        return m_raidManager.get();
    }

    // ========== 睡眠管理 ==========

    /**
     * @brief 跳到早晨
     *
     * 将当前时间设置为下一个早晨（dayTime = 0）。
     * 当所有玩家都入睡时调用。
     */
    void skipToMorning();

    /**
     * @brief 检查是否可以跳过夜晚
     *
     * 检查日光周期是否启用。
     *
     * @return true 如果可以跳过夜晚
     */
    [[nodiscard]] bool canSkipNight() const;

    /**
     * @brief 检查是否可以清除天气
     *
     * 检查天气周期是否启用。
     *
     * @return true 如果可以清除天气
     */
    [[nodiscard]] bool canClearWeather() const;

    /**
     * @brief 检查是否所有玩家都在睡眠
     * @return true 如果所有非观察者玩家都在睡眠
     */
    [[nodiscard]] bool allPlayersSleeping() const noexcept { return m_allPlayersSleeping; }

    /**
     * @brief 更新全员睡眠标志
     *
     * 当玩家开始或停止睡眠时调用。
     */
    void updateAllPlayersSleepingFlag();

    /**
     * @brief 通知世界玩家睡眠状态变化
     *
     * 重写 IWorld::onPlayerSleepingChanged()，调用 updateAllPlayersSleepingFlag()。
     */
    void onPlayerSleepingChanged() override { updateAllPlayersSleepingFlag(); }

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

    // ========== 结构定位 ==========

    /**
     * @brief 查找最近的结构
     *
     * 在指定范围内搜索指定类型结构的最近位置。
     * 使用螺旋搜索算法，从中心向外扩展搜索。
     *
     * @param center 搜索中心位置
     * @param structureType 结构类型
     * @param maxDistance 最大搜索距离（格），默认 50
     * @param skipExisting 是否跳过已找到的结构（用于定位命令的多次搜索），默认 false
     * @return 最近结构位置，如果未找到返回空
     */
    [[nodiscard]] std::optional<BlockPos> findNearestStructure(const BlockPos& center,
        world::gen::structure::StructureType structureType,
        i32 maxDistance,
        bool skipExisting = false) override;

    /**
     * @brief 检查并处理全员睡眠
     *
     * 在 tick() 中调用，检查是否所有玩家都完全入睡，
     * 如果是则跳过夜晚并唤醒所有玩家。
     */
    void checkSleepStatus();

    /**
     * @brief 唤醒所有玩家
     *
     * 当夜晚跳过后调用。
     */
    void wakeUpAllPlayers();

private:
    void _syncLightDataToChunk(LightType type, const SectionPos& pos);
    [[nodiscard]] std::vector<std::reference_wrapper<Entity>> _collectLoadedEntitiesForSave();
    [[nodiscard]] std::vector<std::reference_wrapper<const BlockEntity>> _collectLoadedBlockEntitiesForSave() const;

private:
    ServerWorldConfig m_config;
    world::storage::SingleLevelStorageManager* m_storage = nullptr; ///< 世界级共享单存档存储门面（不拥有）
    std::unique_ptr<ServerChunkManager> m_chunkManager;
    EntityManager m_entityManager;
    EntityTracker m_entityTracker;
    EntityChunkTracker m_entityChunkTracker;
    std::unique_ptr<PhysicsEngine> m_physicsEngine;
    std::unique_ptr<physics::CollisionCache> m_collisionCache;
    std::unique_ptr<world::tick::TickManager> m_tickManager;
    std::unique_ptr<WorldLightManager> m_lightManager;
    std::unique_ptr<WeatherManager> m_weatherManager;
    std::unique_ptr<world::map::MapDataManager> m_mapDataManager;
    server::ItemPickupManager m_itemPickupManager;
    core::TimeManager* m_timeManager = nullptr;       // 外部引用，不拥有
    std::function<Difficulty()> m_difficultyCallback; ///< 难度获取回调（从 MinecraftServer 获取）
    bool m_initialized = false;
    bool m_allPlayersSleeping = false;                                              // 全员睡眠标志
    Vector3d m_worldSpawnPoint{0.0, static_cast<f64>(world::SEA_LEVEL) + 1.0, 0.0}; // 世界出生点

    OpenContainerCallback m_onOpenContainer;
    OpenEntityContainerCallback m_onOpenEntityContainer;

    // 村庄和袭击系统
    std::unique_ptr<::mc::world::village::VillageManager> m_villageManager;
    std::unique_ptr<::mc::world::village::raid::RaidManager> m_raidManager;

    // 村庄围攻系统
    server::spawn::VillageSiege m_villageSiege;

    std::function<void(LightType, const SectionPos&)> m_onLightChanged;
    std::function<void(const BlockPos&, u32)> m_onBlockChanged;
    std::function<void(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32)> m_onPlaySound;
    ParticleBroadcastCallback m_onBroadcastParticle;
    EntityStatusCallback m_onBroadcastEntityStatus;
    EntityAnimationCallback m_onBroadcastEntityAnimation;
    WorldEventCallback m_onBroadcastWorldEvent;
    ExplosionBroadcastCallback m_onBroadcastExplosion;
    RaidEventCallback m_onRaidEvent;           ///< 袭击事件回调
    CommandExecuteCallback m_onExecuteCommand; ///< 命令执行回调

    // 随机刻系统
    math::Random m_random; ///< 世界随机数生成器
    i64 m_updateLCG = 0;   ///< 用于随机刻位置的 LCG 状态

    // 游戏规则
    world::gamerule::GameRules m_gameRules; ///< 游戏规则管理器

    // 世界边界
    world::border::WorldBorder m_worldBorder; ///< 世界边界

    // 掉落表管理器（非拥有，由 MinecraftServer 持有）
    const loot::LootTableManager* m_lootTableManager = nullptr;
};

} // namespace server
} // namespace mc
