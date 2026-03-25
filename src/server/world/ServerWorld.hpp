#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/entity/PlayerManager.hpp"
#include "common/network/sync/ChunkSync.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/world/time/GameTime.hpp"
#include "common/world/gen/spawn/WorldGenSpawner.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/physics/CollisionCache.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/world/entity/EntityTracker.hpp"
#include "server/world/weather/WeatherManager.hpp"
#include "server/world/entity/ItemPickupManager.hpp"
#include <unordered_map>
#include <memory>
#include <functional>
#include <mutex>

namespace mc {

// 前向声明
class WorldLightManager;
class ItemEntity;

namespace server {

// 前向声明
class ServerChunkManager;

// ============================================================================
// 区块缓存条目
// ============================================================================

struct ChunkCacheEntry {
    std::unique_ptr<ChunkData> chunk;
    u64 lastAccessTime = 0;
    bool isGenerated = false;
    bool isModified = false;
    std::unordered_set<PlayerId> subscribers;
};

// ============================================================================
// 服务端世界配置
// ============================================================================

struct ServerWorldConfig {
    i32 viewDistance = 10;              // 视距
    i32 maxChunksPerPlayer = 1024;       // 每玩家最大区块数
    i32 chunkUnloadDelay = 30000;        // 区块卸载延迟（毫秒）
    i32 keepAliveInterval = 15000;        // 心跳间隔（毫秒）
    i32 keepAliveTimeout = 30000;         // 心跳超时（毫秒）
    DimensionId dimension = 0;            // 维度ID
    u64 seed = 12345;                     // 世界种子
};

// ============================================================================
// 服务端世界
// ============================================================================

class ServerWorld : public IWorld, public ICollisionWorld, public IChunkLightProvider {
public:
    ServerWorld();
    explicit ServerWorld(const ServerWorldConfig& config);
    ~ServerWorld() override;

    // 初始化
    [[nodiscard]] Result<void> initialize();
    void shutdown();

    // 配置
    void setConfig(const ServerWorldConfig& config);
    [[nodiscard]] const ServerWorldConfig& config() const { return m_config; }

    // 区块管理
    [[nodiscard]] ChunkData* getChunk(ChunkCoord x, ChunkCoord z);
    [[nodiscard]] const ChunkData* getChunk(ChunkCoord x, ChunkCoord z) const override;
    [[nodiscard]] bool hasChunk(ChunkCoord x, ChunkCoord z) const override;
    [[nodiscard]] ChunkData* getChunkSync(ChunkCoord x, ChunkCoord z);
    void unloadChunk(ChunkCoord x, ChunkCoord z);

    // 玩家管理
    void addPlayer(PlayerId playerId, const String& username, network::ConnectionPtr connection);
    void removePlayer(PlayerId playerId);
    [[nodiscard]] ServerPlayerData* getPlayer(PlayerId playerId);
    [[nodiscard]] const ServerPlayerData* getPlayer(PlayerId playerId) const;
    [[nodiscard]] bool hasPlayer(PlayerId playerId) const;
    [[nodiscard]] size_t playerCount() const;

    /// 遍历所有在线玩家实体，回调函数返回 false 时停止遍历
    /// 使用 EntityManager 按类型过滤，避免全实体扫描
    template<typename Fn>
    void forEachPlayerEntity(Fn&& fn) {
        auto players = m_entityManager.getEntitiesByType(LegacyEntityType::Player);
        for (Entity* entity : players) {
            if (entity && entity->isAlive()) {
                if (!fn(*static_cast<Player*>(entity))) {
                    break;
                }
            }
        }
    }

    /// 遍历所有物品实体，回调函数返回 false 时停止遍历
    template<typename Fn>
    void forEachItemEntity(Fn&& fn) {
        auto items = m_entityManager.getEntitiesByType(LegacyEntityType::Item);
        for (Entity* entity : items) {
            if (entity && entity->isAlive()) {
                if (!fn(*static_cast<ItemEntity*>(entity))) {
                    break;
                }
            }
        }
    }

    // 位置更新（网络协议使用 f64，内部转换为 f32）
    void updatePlayerPosition(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw, f32 pitch, bool onGround);
    void confirmTeleport(PlayerId playerId, u32 teleportId);

    // 传送玩家（网络协议使用 f64）
    void teleportPlayer(PlayerId playerId, f64 x, f64 y, f64 z, f32 yaw = 0.0f, f32 pitch = 0.0f);

    // 玩家模式
    [[nodiscard]] bool setPlayerGameMode(PlayerId playerId, GameMode mode);

    // 区块同步
    void sendInitialChunks(PlayerId playerId);
    void updateChunkSubscription(PlayerId playerId);

    // 方块操作
    bool setBlock(i32 x, i32 y, i32 z, const BlockState* state) override;
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override;

    // 发送数据包给玩家
    void sendPacket(PlayerId playerId, const std::vector<u8>& data);
    void broadcastPacket(const std::vector<u8>& data);
    void broadcastPacketExcept(PlayerId excludePlayerId, const std::vector<u8>& data);

    // 更新循环
    void tick();

    // 统计
    [[nodiscard]] size_t chunkCount() const;
    [[nodiscard]] size_t loadedChunkCount() const;

    // 区块坐标转换（使用 f32 坐标）
    static ChunkCoord blockToChunk(f32 blockCoord) {
        return static_cast<ChunkCoord>(std::floor(blockCoord / 16.0f));
    }

    // 获取区块管理器
    [[nodiscard]] ServerChunkManager* chunkManager() { return m_chunkManager.get(); }
    [[nodiscard]] const ServerChunkManager* chunkManager() const { return m_chunkManager.get(); }

    // ========== 时间管理 ==========

    /**
     * @brief 获取游戏时间
     */
    [[nodiscard]] time::GameTime& gameTime() { return m_gameTime; }
    [[nodiscard]] const time::GameTime& gameTime() const { return m_gameTime; }

    /**
     * @brief 设置一天内的时间 (/time set)
     * @param time 时间值 (0-23999)
     */
    void setDayTime(i64 time);

    /**
     * @brief 增加时间 (/time add)
     * @param ticks 要增加的 tick 数
     */
    void addDayTime(i64 ticks);

    /**
     * @brief 设置日光周期是否启用
     * @param enabled true 启用
     */
    void setDaylightCycleEnabled(bool enabled);

    // ========== 天气管理 ==========

    /**
     * @brief 获取天气管理器
     */
    [[nodiscard]] WeatherManager& weatherManager() { return *m_weatherManager; }
    [[nodiscard]] const WeatherManager& weatherManager() const { return *m_weatherManager; }

    // ========== 其他 IWorld 接口 ==========

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
    [[nodiscard]] DimensionId dimension() const override { return m_config.dimension; }
    [[nodiscard]] u64 seed() const override { return m_config.seed; }
    [[nodiscard]] u64 currentTick() const override { return m_currentTick; }
    [[nodiscard]] i64 dayTime() const override { return m_gameTime.dayTime(); }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] i32 difficulty() const override { return 1; } // Normal

    // ========== 天气接口 (IWorld) ==========

    [[nodiscard]] bool isRaining() const override;
    [[nodiscard]] bool isThundering() const override;
    [[nodiscard]] f32 rainStrength(f32 partialTick = 0.0f) const override;
    [[nodiscard]] f32 thunderStrength(f32 partialTick = 0.0f) const override;
    [[nodiscard]] bool canRainAt(const BlockPos& pos) const override;

    // ========== 物理引擎 ==========

    [[nodiscard]] PhysicsEngine* physicsEngine() override { return m_physicsEngine.get(); }
    [[nodiscard]] const PhysicsEngine* physicsEngine() const override { return m_physicsEngine.get(); }

    // ========== 碰撞缓存 ==========

    /**
     * @brief 使指定区块的碰撞缓存失效
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     */
    void invalidateCollisionCache(ChunkCoord chunkX, ChunkCoord chunkZ);

    /**
     * @brief 清除所有碰撞缓存
     */
    void clearCollisionCache();

    /**
     * @brief 初始化区块的光照引擎数据
     *
     * 当新区块加载或生成后调用，将 ChunkSection 中的光照数据
     * 同步到光照引擎，并通知引擎区块段的状态。
     *
     * @param chunkX 区块X坐标
     * @param chunkZ 区块Z坐标
     */
    void initializeChunkLighting(ChunkCoord chunkX, ChunkCoord chunkZ);

    // ========== ICollisionWorld 接口实现 ==========

    [[nodiscard]] const ChunkData* getChunkAt(ChunkCoord x, ChunkCoord z) const override {
        return getChunk(x, z);
    }

    // 注意：getMinBuildHeight() 和 getMaxBuildHeight() 在 IChunkLightProvider 接口部分实现

    // ========== 实体管理 ==========

    /**
     * @brief 生成实体到世界
     * @param entity 实体指针（世界获得所有权）
     * @return 实体ID
     */
    EntityId spawnEntity(std::unique_ptr<Entity> entity);

    /**
     * @brief 移除实体
     * @param id 实体ID
     * @return 被移除的实体指针
     */
    std::unique_ptr<Entity> removeEntity(EntityId id);

    /**
     * @brief 获取实体
     * @param id 实体ID
     * @return 实体指针
     */
    [[nodiscard]] Entity* getEntity(EntityId id);
    [[nodiscard]] const Entity* getEntity(EntityId id) const;

    /**
     * @brief 检查实体是否存在
     * @param id 实体ID
     */
    [[nodiscard]] bool hasEntity(EntityId id) const;

    /**
     * @brief 获取实体数量
     */
    [[nodiscard]] size_t entityCount() const;

    /**
     * @brief 获取实体管理器
     */
    [[nodiscard]] EntityManager& entityManager() { return m_entityManager; }
    [[nodiscard]] const EntityManager& entityManager() const { return m_entityManager; }

    /**
     * @brief 获取实体追踪器
     */
    [[nodiscard]] EntityTracker& entityTracker() { return m_entityTracker; }
    [[nodiscard]] const EntityTracker& entityTracker() const { return m_entityTracker; }

    /**
     * @brief 获取物品拾取管理器
     */
    [[nodiscard]] server::ItemPickupManager& itemPickupManager() { return m_itemPickupManager; }
    [[nodiscard]] const server::ItemPickupManager& itemPickupManager() const { return m_itemPickupManager; }

    // ========== 区块生成实体 ==========

    /**
     * @brief 处理区块生成时产生的实体
     *
     * 当区块生成完成后，调用此方法将 WorldGenSpawner 生成的
     * 被动动物实体真正创建到世界中。
     *
     * @param entities 生成的实体数据列表
     * @return 实际创建的实体数量
     */
    i32 spawnEntitiesFromChunkGeneration(const std::vector<SpawnedEntityData>& entities);

    // ========== Tick管理 ==========

    /**
     * @brief 获取Tick管理器
     */
    [[nodiscard]] world::tick::TickManager& tickManager() { return *m_tickManager; }
    [[nodiscard]] const world::tick::TickManager& tickManager() const { return *m_tickManager; }

    // ========== 方块和流体tick调度便捷方法 ==========

    /**
     * @brief 调度方块tick
     *
     * @param pos 方块位置
     * @param block 方块引用
     * @param delay 延迟tick数
     * @param priority 优先级（默认Normal）
     */
    void scheduleBlockTick(const BlockPos& pos, Block& block, i32 delay,
                          world::tick::TickPriority priority = world::tick::TickPriority::Normal);

    /**
     * @brief 调度流体tick
     *
     * @param pos 流体位置
     * @param fluid 流体引用
     * @param delay 延迟tick数
     * @param priority 优先级（默认Normal）
     */
    void scheduleFluidTick(const BlockPos& pos, fluid::Fluid& fluid, i32 delay,
                          world::tick::TickPriority priority = world::tick::TickPriority::Normal);

    // ========== IChunkLightProvider 接口实现 ==========

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

    /**
     * @brief 获取光照管理器
     */
    [[nodiscard]] WorldLightManager* lightManager() { return m_lightManager.get(); }
    [[nodiscard]] const WorldLightManager* lightManager() const { return m_lightManager.get(); }

private:
    // 内部方法
    void sendChunkToPlayer(PlayerId playerId, ChunkCoord x, ChunkCoord z);
    void sendUnloadChunkToPlayer(PlayerId playerId, ChunkCoord x, ChunkCoord z);
    void broadcastBlockUpdate(i32 x, i32 y, i32 z, u32 blockStateId);
    void broadcastTimeUpdate();  // 广播时间更新
    void broadcastLightUpdate(LightType type, const SectionPos& pos);  // 广播光照更新

    /**
     * @brief 从光照引擎同步光照数据到 ChunkSection
     *
     * 光照引擎计算的光照数据存储在其内部 storage 中，
     * 此方法将数据复制到 ChunkSection 的 NibbleArray 中，
     * 以便在序列化和广播时使用正确的光照值。
     *
     * @param type 光照类型
     * @param pos 区块段位置
     */
    void syncLightDataToChunk(LightType type, const SectionPos& pos);

private:
    ServerWorldConfig m_config;
    std::unique_ptr<ServerChunkManager> m_chunkManager;
    EntityManager m_entityManager;  // 实体管理器
    EntityTracker m_entityTracker;   // 实体追踪器
    std::unique_ptr<PhysicsEngine> m_physicsEngine;  // 物理引擎
    std::unique_ptr<physics::CollisionCache> m_collisionCache;  // 碰撞缓存
    std::unique_ptr<world::tick::TickManager> m_tickManager;  // Tick管理器
    std::unique_ptr<WorldLightManager> m_lightManager;  // 光照管理器
    std::unique_ptr<WeatherManager> m_weatherManager;  // 天气管理器
    server::ItemPickupManager m_itemPickupManager;  // 物品拾取管理器
    bool m_initialized = false;

    // 玩家存储
    mutable std::mutex m_playerMutex;
    std::unordered_map<PlayerId, ServerPlayerData> m_players;

    // 区块同步管理器
    network::ChunkSyncManager m_chunkSyncManager;

    // 时间
    time::GameTime m_gameTime;
    u64 m_currentTick = 0;
    u64 m_lastTimeSyncTick = 0;  // 上次时间同步的 tick
    u64 m_lastChunkUnloadCheck = 0;
};

} // namespace server
} // namespace mc
