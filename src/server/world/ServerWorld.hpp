#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/dimension/DimensionType.hpp"
#include "common/world/village/VillageManager.hpp"
#include "common/world/village/raid/RaidManager.hpp"
#include "server/world/spawn/VillageSiege.hpp"
#include "common/world/storage/WorldStorageService.hpp"
#include "common/world/storage/save/SaveManager.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/physics/CollisionCache.hpp"
#include "common/world/WorldConfig.hpp"
#include "common/util/math/random/Random.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/entity/EntityTracker.hpp"
#include "server/world/entity/ItemPickupManager.hpp"
#include "server/world/weather/WeatherManager.hpp"
#include <memory>
#include <functional>
#include <stdexcept>
#include <utility>

namespace mc {

// 前向声明
struct SpawnedEntityData;

namespace client::renderer::trident::particle {
enum class ParticleTypeId : u16;
}

namespace server::core {
class TimeManager;  // 前向声明
}

namespace loot {
class LootTableManager;  // 前向声明
}

namespace server {

// ============================================================================
// 服务端世界配置
// ============================================================================

// TODO 这个结构体是多余的。比如isDebugWorld设为true之后根本无法启用调试区块生成器。
// 要在这里才能配置世界生成器类型 D:\MiscProjects\minecraft-reborn\src\server\application\IntegratedServer.hpp
struct ServerWorldConfig {
    i32 viewDistance = 10;              // 视距
    DimensionId dimension = 0;          // 维度ID
    u64 seed = 114514;                   // 世界种子
    bool isDebugWorld = true;          // 是否为调试世界
    std::string worldName = "world";    // 世界名称（用于存档目录）
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
    ServerWorld();
    explicit ServerWorld(const ServerWorldConfig& config);
    ~ServerWorld() override;

    using IWorld::setBlockState;
    using IWorld::getBlockState;
    using IWorld::getFluidState;
    using IWorld::getBlockLight;
    using IWorld::getSkyLight;
    using IWorld::isWithinWorldBounds;

    // 初始化
    [[nodiscard]] Result<void> initialize();
    void shutdown();

    // ========== 存储系统 ==========

    /**
     * @brief 获取存储服务
     *
     * 这是访问存档功能的唯一入口。
     * 通过返回的 WorldStorageService 可以访问所有子服务：
     * - sectionManager(dimension): Section级存储
     * - worldListService(): 世界列表管理
     * - backupManager(): 快照管理
     */
    [[nodiscard]] world::storage::WorldStorageService& storage() { return m_storage; }
    [[nodiscard]] const world::storage::WorldStorageService& storage() const { return m_storage; }

    /**
     * @brief 获取保存协调器
     */
    [[nodiscard]] world::storage::SaveManager* saveManager() { return m_saveManager.get(); }
    [[nodiscard]] const world::storage::SaveManager* saveManager() const { return m_saveManager.get(); }

    /**
     * @brief 检查存储服务是否已打开
     */
    [[nodiscard]] bool isStorageOpen() const { return m_storage.isOpen(); }

    /**
     * @brief 保存所有脏数据
     *
     * 遍历所有维度的 SectionManager，刷新脏Section到磁盘。
     * 通常在服务器关闭时调用。
     */
    Result<size_t> saveAll();

    // 配置
    void setConfig(const ServerWorldConfig& config);
    [[nodiscard]] const ServerWorldConfig& config() const { return m_config; }

    // ========== 时间管理（设置外部 TimeManager） ==========

    void setTimeManager(core::TimeManager* timeManager) { m_timeManager = timeManager; }
    [[nodiscard]] core::TimeManager* timeManager() { return m_timeManager; }
    [[nodiscard]] const core::TimeManager* timeManager() const { return m_timeManager; }

    // ========== 区块管理 ==========

    [[nodiscard]] ChunkData* getChunk(ChunkCoord x, ChunkCoord z);
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord x, ChunkCoord z) const override;
    [[nodiscard]] bool hasChunk(ChunkCoord x, ChunkCoord z) const override;
    [[nodiscard]] ChunkData* getChunkSync(ChunkCoord x, ChunkCoord z);
    void unloadChunk(ChunkCoord x, ChunkCoord z);

    // ========== 方块操作 ==========

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override;
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override;

    // ========== 更新循环 ==========

    void tick();

private:
    /**
     * @brief 执行环境随机刻
     *
     * 对每个已加载区块，随机选择位置执行方块和流体的随机刻。
     * 这是农作物生长、冰融化、火焰蔓延等机制的核心。
     *
     * 参考: MC 1.16.5 ServerWorld.tickEnvironment()
     *
     * @param randomTickSpeed 随机刻速度（默认为3，可在游戏规则中设置）
     */
    void tickEnvironment(i32 randomTickSpeed);

    /**
     * @brief 生成随机方块位置
     *
     * 使用 MC 风格的 LCG 生成随机位置，确保同一 tick 内位置分布均匀。
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
    [[nodiscard]] DimensionId dimension() const override { return m_config.dimension; }
    [[nodiscard]] bool isUltraWarm() const override { return getDimensionType().ultraWarm(); }
    [[nodiscard]] DimensionType getDimensionType() const;
    [[nodiscard]] u64 seed() const override { return m_config.seed; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] Difficulty difficulty() const override { return Difficulty::Easy; }
    [[nodiscard]] bool isClientSide() override { return false; }

    // ========== 随机数生成器 ==========

    [[nodiscard]] math::Random& getRandom() override { return m_random; }
    [[nodiscard]] const math::Random& getRandom() const override { return m_random; }

    // ========== 类型转换 ==========

    [[nodiscard]] ServerWorld* asServerWorld() override { return this; }
    [[nodiscard]] const ServerWorld* asServerWorld() const override { return this; }

    // ========== 天气接口 (IWorld) ==========

    [[nodiscard]] bool isRaining() const override;
    [[nodiscard]] bool isThundering() const override;
    [[nodiscard]] f32 rainStrength(f32 partialTick = 0.0f) const override;
    [[nodiscard]] f32 thunderStrength(f32 partialTick = 0.0f) const override;
    [[nodiscard]] bool canRainAt(const BlockPos& pos) const override;

    // ========== 声音播放 ==========

    void setOnPlaySound(std::function<void(const ResourceLocation&, sound::SoundCategory, const Vector3&, f32, f32)> callback) {
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

    // ========== 容器打开回调 ==========

    using OpenContainerCallback = std::function<bool(ContainerType, const BlockPos&, Player&)>;

    void setOnOpenContainer(OpenContainerCallback callback) {
        if (m_onOpenContainer) {
            throw std::runtime_error("Open container callback already set");
        }

        m_onOpenContainer = std::move(callback);
    }
    [[nodiscard]] bool openContainer(ContainerType type, const BlockPos& pos, Player& player) override;

    // ========== 粒子广播回调 ==========

    /**
     * @brief 粒子广播回调类型
     *
     * 当服务端需要广播粒子给玩家时调用。
     * 参数：粒子类型、位置、速度、偏移、数量
     */
    using ParticleBroadcastCallback = std::function<void(
        client::renderer::trident::particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count)>;

    void setOnBroadcastParticle(ParticleBroadcastCallback callback) {
        m_onBroadcastParticle = std::move(callback);
    }

    // ========== 实体状态广播回调 ==========

    /**
     * @brief 实体状态广播回调类型
     *
     * 当服务端需要广播实体状态事件给玩家时调用。
     * 参数：实体ID、状态码
     */
    using EntityStatusCallback = std::function<void(EntityId entityId, u8 status)>;

    void setOnBroadcastEntityStatus(EntityStatusCallback callback) {
        m_onBroadcastEntityStatus = std::move(callback);
    }

    // ========== 世界事件回调 ==========

    /**
     * @brief 世界事件广播回调类型
     *
     * 当服务端需要广播世界事件给玩家时调用。
     * 参数：事件ID、位置、数据
     */
    using WorldEventCallback = std::function<void(i32 eventId, i32 x, i32 y, i32 z, i32 data)>;

    void setOnBroadcastWorldEvent(WorldEventCallback callback) {
        m_onBroadcastWorldEvent = std::move(callback);
    }

    // ========== 爆炸广播回调 ==========

    /**
     * @brief 爆炸广播回调类型
     *
     * 当服务端需要广播爆炸事件给玩家时调用。
     * 参数：爆炸位置、威力、受影响方块列表、玩家击退映射
     */
    using ExplosionBroadcastCallback = std::function<void(
        const Vector3& position,
        f32 strength,
        const std::vector<BlockPos>& affectedBlocks,
        const std::unordered_map<u64, Vector3>& playerKnockback)>;

    void setOnBroadcastExplosion(ExplosionBroadcastCallback callback) {
        m_onBroadcastExplosion = std::move(callback);
    }

    // ========== 袭击事件回调 ==========

    /**
     * @brief 袭击事件广播回调类型
     *
     * 当袭击发生特定事件时调用，用于通知玩家。
     */
    using RaidEventCallback = std::function<void(
        i32 raidId,                ///< 袭击 ID
        i32 eventType,             ///< 事件类型 (0=开始, 1=胜利, 2=失败, 3=波次开始)
        const BlockPos& pos,       ///< 相关位置
        i32 data                   ///< 额外数据
    )>;

    void setOnRaidEvent(RaidEventCallback callback) {
        m_onRaidEvent = std::move(callback);
    }

    // ========== IWorld 接口实现 ==========

    void addParticle(
        client::renderer::trident::particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity) override;

    void addParticle(
        client::renderer::trident::particle::ParticleTypeId type,
        const Vector3& pos,
        const Vector3& velocity,
        const Vector3& offset,
        u32 count) override;

    [[nodiscard]] bool shouldSpawnParticleAt(
        const Vector3& pos,
        f32 maxDistance = 256.0f) const override;

    // ========== 实体状态广播 (IWorld override) ==========

    void broadcastEntityStatus(EntityId entityId, u8 status) override;

    // ========== 爆炸 ==========

    void createExplosion(
        const Vector3& position,
        f32 radius,
        world::explosion::ExplosionMode mode = world::explosion::ExplosionMode::Destroy,
        bool causesFire = false,
        Entity* source = nullptr) override;

    /**
     * @brief 设置掉落表管理器
     *
     * 掉落表管理器由 MinecraftServer 持有，ServerWorld 通过此方法获取引用。
     * 用于爆炸时的方块掉落生成和方块实体的战利品表填充。
     *
     * @param lootTableManager 掉落表管理器指针（非拥有）
     */
    void setLootTableManager(const loot::LootTableManager* lootTableManager) {
        m_lootTableManager = lootTableManager;
    }

    /**
     * @brief 获取战利品表管理器（IWorld 接口实现）
     *
     * 只有 ServerWorld 会返回有效的指针。
     * 用于方块实体填充战利品表。
     *
     * 参考 MC 1.16.5: World.getLootTableManager()
     *
     * @return LootTableManager指针，如果不存在返回nullptr
     */
    [[nodiscard]] const loot::LootTableManager* lootTableManager() const override {
        return m_lootTableManager;
    }

    // ========== 物理引擎 ==========

    [[nodiscard]] PhysicsEngine* physicsEngine() override { return m_physicsEngine.get(); }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return m_physicsEngine.get(); }

    // ========== 碰撞缓存 ==========

    void invalidateCollisionCache(ChunkCoord chunkX, ChunkCoord chunkZ);
    void clearCollisionCache();

    // ========== ICollisionWorld 接口实现 ==========

    [[nodiscard]] const ChunkData* getChunkAt(ChunkCoord x, ChunkCoord z) const override {
        return getChunk(x, z);
    }

    // ========== 实体管理 ==========

    std::unique_ptr<Entity> removeEntity(EntityId id);
    [[nodiscard]] EntityId spawnEntity(std::unique_ptr<Entity> entity) override;
    [[nodiscard]] Entity* getEntity(EntityId id) override;
    [[nodiscard]] const Entity* getEntity(EntityId id) const override;
    [[nodiscard]] bool hasEntity(EntityId id) const;
    [[nodiscard]] size_t entityCount() const;

    [[nodiscard]] EntityManager& entityManager() { return m_entityManager; }
    [[nodiscard]] const EntityManager& entityManager() const { return m_entityManager; }

    [[nodiscard]] EntityTracker& entityTracker() { return m_entityTracker; }
    [[nodiscard]] const EntityTracker& entityTracker() const { return m_entityTracker; }

    [[nodiscard]] server::ItemPickupManager& itemPickupManager() { return m_itemPickupManager; }
    [[nodiscard]] const server::ItemPickupManager& itemPickupManager() const { return m_itemPickupManager; }

    // ========== 区块生成实体 ==========

    i32 spawnEntitiesFromChunkGeneration(const std::vector<SpawnedEntityData>& entities);

    // ========== Tick管理 ==========

    [[nodiscard]] world::tick::TickManager& tickManager() { return *m_tickManager; }
    [[nodiscard]] const world::tick::TickManager& tickManager() const { return *m_tickManager; }

    // ========== StarLightLightingProvider 接口实现 ==========

    [[nodiscard]] IChunk* getChunkForLight(ChunkCoord x, ChunkCoord z) override;
    [[nodiscard]] const IChunk* getChunkForLight(ChunkCoord x, ChunkCoord z) const override;
    [[nodiscard]] const BlockState* getBlockStateForLight(const BlockPos& pos) const override;
    [[nodiscard]] IWorld* getWorld() override;
    [[nodiscard]] const IWorld* getWorld() const override;
    void markLightChanged(LightType type, const SectionPos& pos) override;
    [[nodiscard]] bool hasSkyLight() const override;
    [[nodiscard]] i32 getMinBuildHeight() const override;
    [[nodiscard]] i32 getMaxBuildHeight() const override;
    [[nodiscard]] i32 getSectionCount() const override;

    // ========== 光照管理 ==========

    [[nodiscard]] WorldLightManager* lightManager() { return m_lightManager.get(); }
    [[nodiscard]] const WorldLightManager* lightManager() const { return m_lightManager.get(); }
    void setLightManager(std::unique_ptr<WorldLightManager> manager) { m_lightManager = std::move(manager); }

    // ========== 区块管理器设置 ==========

    void setChunkManager(std::unique_ptr<ServerChunkManager> manager);

    // ========== 光照变化回调 ==========

    void setOnLightChanged(std::function<void(LightType, const SectionPos&)> callback) {
        if (m_onLightChanged) {
            throw std::runtime_error("Light change callback already set");
        }
        m_onLightChanged = std::move(callback);
    }

    // ========== 方块变化回调 ==========

    void setOnBlockChanged(std::function<void(const BlockPos&, u32)> callback) {
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
    [[nodiscard]] Vector3d worldSpawnPoint() const { return m_worldSpawnPoint; }

    /**
     * @brief 设置世界出生点
     *
     * @param pos 新的出生点位置
     */
    void setWorldSpawnPoint(const Vector3d& pos) { m_worldSpawnPoint = pos; }

    // ========== 调试模式 ==========

    /**
     * @brief 检查是否为调试世界
     *
     * 调试世界特性：
     * - 无法放置或破坏方块
     * - 计划刻不会执行
     * - 方块状态由调试生成器决定
     *
     * @return 是否为调试世界
     */
    [[nodiscard]] bool isDebugWorld() const { return m_config.isDebugWorld; }

    // ========== 世界边界 ==========

    /**
     * @brief 获取世界边界
     *
     * @return 世界边界对象
     */
    [[nodiscard]] world::border::WorldBorder& worldBorder() override { return m_worldBorder; }
    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override { return m_worldBorder; }

    // ========== 村庄管理 ==========

    [[nodiscard]] ::mc::world::village::VillageManager* villageManager() override { return m_villageManager.get(); }
    [[nodiscard]] const ::mc::world::village::VillageManager* villageManager() const override { return m_villageManager.get(); }

    // ========== 袭击管理 ==========

    [[nodiscard]] ::mc::world::village::raid::RaidManager* raidManager() { return m_raidManager.get(); }
    [[nodiscard]] const ::mc::world::village::raid::RaidManager* raidManager() const { return m_raidManager.get(); }

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
    [[nodiscard]] bool allPlayersSleeping() const { return m_allPlayersSleeping; }

    /**
     * @brief 更新全员睡眠标志
     *
     * 当玩家开始或停止睡眠时调用。
     */
    void updateAllPlayersSleepingFlag();

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
    void syncLightDataToChunk(LightType type, const SectionPos& pos);

private:
    ServerWorldConfig m_config;
    world::storage::WorldStorageService m_storage;  ///< 存储服务（唯一对外接口）
    std::unique_ptr<world::storage::SaveManager> m_saveManager;  ///< 保存协调器
    std::unique_ptr<ServerChunkManager> m_chunkManager;
    EntityManager m_entityManager;
    EntityTracker m_entityTracker;
    std::unique_ptr<PhysicsEngine> m_physicsEngine;
    std::unique_ptr<physics::CollisionCache> m_collisionCache;
    std::unique_ptr<world::tick::TickManager> m_tickManager;
    std::unique_ptr<WorldLightManager> m_lightManager;
    std::unique_ptr<WeatherManager> m_weatherManager;
    server::ItemPickupManager m_itemPickupManager;
    core::TimeManager* m_timeManager = nullptr;  // 外部引用，不拥有
    bool m_initialized = false;
    bool m_allPlayersSleeping = false;  // 全员睡眠标志
    Vector3d m_worldSpawnPoint{0.0, static_cast<f64>(world::SEA_LEVEL) + 1.0, 0.0};  // 世界出生点

    OpenContainerCallback m_onOpenContainer;

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
    WorldEventCallback m_onBroadcastWorldEvent;
    ExplosionBroadcastCallback m_onBroadcastExplosion;
    RaidEventCallback m_onRaidEvent;  ///< 袭击事件回调

    // 随机刻系统
    math::Random m_random;            ///< 世界随机数生成器
    i64 m_updateLCG = 0;              ///< 用于随机刻位置的 LCG 状态
    i32 m_randomTickSpeed = 3;        ///< 随机刻速度（游戏规则可配置）

    // 世界边界
    world::border::WorldBorder m_worldBorder;  ///< 世界边界

    // 掉落表管理器（非拥有，由 MinecraftServer 持有）
    const loot::LootTableManager* m_lootTableManager = nullptr;
};

} // namespace server
} // namespace mc