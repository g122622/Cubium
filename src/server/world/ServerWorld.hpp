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
#include "common/physics/PhysicsEngine.hpp"
#include "common/physics/CollisionCache.hpp"
#include "common/world/WorldConfig.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/entity/EntityTracker.hpp"
#include "server/world/entity/ItemPickupManager.hpp"
#include "server/world/weather/WeatherManager.hpp"
#include <memory>
#include <functional>

namespace mc {

// 前向声明
struct SpawnedEntityData;

namespace server::core {
class TimeManager;  // 前向声明
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

    // 初始化
    [[nodiscard]] Result<void> initialize();
    void shutdown();

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

    bool setBlock(i32 x, i32 y, i32 z, const BlockState* state) override;
    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override;

    // ========== 更新循环 ==========

    void tick();

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
    [[nodiscard]] DimensionId dimension() const override { return m_config.dimension; }
    [[nodiscard]] DimensionType getDimensionType() const;
    [[nodiscard]] u64 seed() const override { return m_config.seed; }
    [[nodiscard]] bool isHardcore() const override { return false; }
    [[nodiscard]] i32 difficulty() const override { return 1; }

    // ========== 类型转换 ==========

    [[nodiscard]] ServerWorld* asServerWorld() override { return this; }
    [[nodiscard]] const ServerWorld* asServerWorld() const override { return this; }

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

    void scheduleBlockTick(const BlockPos& pos, Block& block, i32 delay,
                          world::tick::TickPriority priority = world::tick::TickPriority::Normal) override;
    void scheduleFluidTick(const BlockPos& pos, fluid::Fluid& fluid, i32 delay,
                          world::tick::TickPriority priority = world::tick::TickPriority::Normal) override;

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

    // ========== 村庄管理 ==========

    [[nodiscard]] world::village::VillageManager* villageManager() { return m_villageManager.get(); }
    [[nodiscard]] const world::village::VillageManager* villageManager() const { return m_villageManager.get(); }

    // ========== 袭击管理 ==========

    [[nodiscard]] world::village::raid::RaidManager* raidManager() { return m_raidManager.get(); }
    [[nodiscard]] const world::village::raid::RaidManager* raidManager() const { return m_raidManager.get(); }

private:
    void syncLightDataToChunk(LightType type, const SectionPos& pos);

private:
    ServerWorldConfig m_config;
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

    // 村庄和袭击系统
    std::unique_ptr<world::village::VillageManager> m_villageManager;
    std::unique_ptr<world::village::raid::RaidManager> m_raidManager;

    std::function<void(LightType, const SectionPos&)> m_onLightChanged;
};

} // namespace server
} // namespace mc