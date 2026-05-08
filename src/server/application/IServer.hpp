#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include <memory>

namespace mc {
class AbstractContainerMenu;
class WorldLightManager;
class PhysicsEngine;
class EntityManager;
class PlayerInventory;
class ServerDimensionManager;
namespace server {
class ServerPlayerEntityManager;
}
namespace time {
class GameTime;
}
namespace world {
namespace tick {
class TickManager;
}
}
namespace network {
class ChunkSyncManager;
}
namespace command {
class CommandRegistry;
}
}

namespace mc::server {

// 前向声明
namespace core {
class PlayerManager;
class ConnectionManager;
class TimeManager;
class TeleportManager;
class KeepAliveManager;
class PositionTracker;
class PacketHandler;
class GameModeManager;
}

namespace interaction {
class BlockInteractionManager;
class MiningManager;
class ContainerManager;
class InventoryManager;
}

namespace sync {
class EntitySyncManager;
class ChunkSendManager;
class LightSyncManager;
}

class ServerWorld;
class ServerChunkManager;
class EntityTracker;
class ItemPickupManager;
class WeatherManager;

/**
 * @brief 服务器接口
 *
 * 定义了所有服务器类型的统一接口。
 * 提供对各个 Manager 的直接访问，避免二次封装。
 */
class IServer {
public:
    virtual ~IServer() = default;

    // ========== 生命周期 ==========

    /**
     * @brief 初始化服务器
     */
    [[nodiscard]] virtual Result<void> initialize() = 0;

    /**
     * @brief 关闭服务器
     */
    virtual void shutdown() = 0;

    /**
     * @brief 执行一个 tick
     */
    virtual void tick() = 0;

    /**
     * @brief 检查服务器是否正在运行
     */
    [[nodiscard]] virtual bool isRunning() const = 0;

    // ========== 核心管理器 ==========

    [[nodiscard]] virtual core::PlayerManager& playerManager() = 0;
    [[nodiscard]] virtual const core::PlayerManager& playerManager() const = 0;

    [[nodiscard]] virtual core::ConnectionManager& connectionManager() = 0;
    [[nodiscard]] virtual const core::ConnectionManager& connectionManager() const = 0;

    [[nodiscard]] virtual core::TimeManager& timeManager() = 0;
    [[nodiscard]] virtual const core::TimeManager& timeManager() const = 0;

    [[nodiscard]] virtual core::TeleportManager& teleportManager() = 0;
    [[nodiscard]] virtual const core::TeleportManager& teleportManager() const = 0;

    [[nodiscard]] virtual core::KeepAliveManager& keepAliveManager() = 0;
    [[nodiscard]] virtual const core::KeepAliveManager& keepAliveManager() const = 0;

    [[nodiscard]] virtual core::PositionTracker& positionTracker() = 0;
    [[nodiscard]] virtual const core::PositionTracker& positionTracker() const = 0;

    [[nodiscard]] virtual core::PacketHandler& packetHandler() = 0;
    [[nodiscard]] virtual const core::PacketHandler& packetHandler() const = 0;

    [[nodiscard]] virtual core::GameModeManager& gameModeManager() = 0;
    [[nodiscard]] virtual const core::GameModeManager& gameModeManager() const = 0;

    // ========== 维度管理器 ==========

    /**
     * @brief 获取维度管理器
     * @return 维度管理器引用
     */
    [[nodiscard]] virtual ServerDimensionManager& dimensionManager() = 0;
    [[nodiscard]] virtual const ServerDimensionManager& dimensionManager() const = 0;

    // ========== 世界管理器 ==========

    [[nodiscard]] virtual ServerWorld& world() = 0;
    [[nodiscard]] virtual const ServerWorld& world() const = 0;

    [[nodiscard]] virtual ServerChunkManager& chunkManager() = 0;
    [[nodiscard]] virtual const ServerChunkManager& chunkManager() const = 0;

    [[nodiscard]] virtual WorldLightManager* lightManager() = 0;
    [[nodiscard]] virtual const WorldLightManager* lightManager() const = 0;

    [[nodiscard]] virtual mc::EntityManager& entityManager() = 0;
    [[nodiscard]] virtual const mc::EntityManager& entityManager() const = 0;

    [[nodiscard]] virtual EntityTracker& entityTracker() = 0;
    [[nodiscard]] virtual const EntityTracker& entityTracker() const = 0;

    [[nodiscard]] virtual PhysicsEngine* physicsEngine() = 0;
    [[nodiscard]] virtual const PhysicsEngine* physicsEngine() const = 0;

    [[nodiscard]] virtual WeatherManager& weatherManager() = 0;
    [[nodiscard]] virtual const WeatherManager& weatherManager() const = 0;

    [[nodiscard]] virtual ItemPickupManager& itemPickupManager() = 0;
    [[nodiscard]] virtual const ItemPickupManager& itemPickupManager() const = 0;

    // ========== 玩家实体管理 ==========

    /**
     * @brief 获取玩家实体管理器
     *
     * 用于管理玩家的实体对象（Player实例）。
     */
    [[nodiscard]] virtual ServerPlayerEntityManager& playerEntityManager() = 0;
    [[nodiscard]] virtual const ServerPlayerEntityManager& playerEntityManager() const = 0;

    // ========== 交互管理器 ==========

    [[nodiscard]] virtual interaction::BlockInteractionManager& blockInteractionManager() = 0;
    [[nodiscard]] virtual const interaction::BlockInteractionManager& blockInteractionManager() const = 0;

    [[nodiscard]] virtual interaction::MiningManager& miningManager() = 0;
    [[nodiscard]] virtual const interaction::MiningManager& miningManager() const = 0;

    [[nodiscard]] virtual interaction::ContainerManager& containerManager() = 0;
    [[nodiscard]] virtual const interaction::ContainerManager& containerManager() const = 0;

    [[nodiscard]] virtual interaction::InventoryManager& inventoryManager() = 0;
    [[nodiscard]] virtual const interaction::InventoryManager& inventoryManager() const = 0;

    /**
     * @brief 获取指定玩家的物品栏。
     */
    [[nodiscard]] virtual PlayerInventory* playerInventory(PlayerId playerId) = 0;
    [[nodiscard]] virtual const PlayerInventory* playerInventory(PlayerId playerId) const = 0;

    // ========== 同步管理器 ==========

    [[nodiscard]] virtual sync::EntitySyncManager& entitySyncManager() = 0;
    [[nodiscard]] virtual const sync::EntitySyncManager& entitySyncManager() const = 0;

    [[nodiscard]] virtual sync::ChunkSendManager& chunkSendManager() = 0;
    [[nodiscard]] virtual const sync::ChunkSendManager& chunkSendManager() const = 0;

    [[nodiscard]] virtual sync::LightSyncManager& lightSyncManager() = 0;
    [[nodiscard]] virtual const sync::LightSyncManager& lightSyncManager() const = 0;

    // ========== 命令系统 ==========

    [[nodiscard]] virtual mc::command::CommandRegistry& commandRegistry() = 0;
    [[nodiscard]] virtual const mc::command::CommandRegistry& commandRegistry() const = 0;

    // ========== 配置 ==========

    [[nodiscard]] virtual i32 viewDistance() const = 0;
    [[nodiscard]] virtual i32 maxPlayers() const = 0;
    [[nodiscard]] virtual u64 seed() const = 0;
    [[nodiscard]] virtual u64 currentTick() const = 0;

    /**
     * @brief 获取当前世界难度。
     */
    [[nodiscard]] virtual Difficulty difficulty() const = 0;

    /**
     * @brief 设置当前世界难度。
     */
    virtual void setDifficulty(Difficulty difficulty) = 0;

    /**
     * @brief 获取服务器默认游戏模式。
     */
    [[nodiscard]] virtual GameMode defaultGameMode() const = 0;

    /**
     * @brief 设置服务器默认游戏模式。
     */
    virtual void setDefaultGameMode(GameMode mode) = 0;

    /**
     * @brief 获取玩家挂机超时，单位为分钟。
     */
    [[nodiscard]] virtual i32 playerIdleTimeoutMinutes() const = 0;

    /**
     * @brief 设置玩家挂机超时，单位为分钟。
     */
    virtual void setPlayerIdleTimeoutMinutes(i32 timeoutMinutes) = 0;

    /**
     * @brief 向所有在线玩家广播服务器系统消息。
     */
    virtual void broadcastServerMessage(std::string_view message) = 0;

    /**
     * @brief 请求服务器优雅停机。
     *
     * 只触发停机流程，不在调用线程里直接释放资源。
     */
    virtual void requestStop() = 0;

    // ========== 粒子广播方法 ==========

    /**
     * @brief 广播粒子给指定范围内的玩家
     *
     * @param type 粒子类型
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @param velocityX X速度
     * @param velocityY Y速度
     * @param velocityZ Z速度
     * @param offsetX X偏移范围
     * @param offsetY Y偏移范围
     * @param offsetZ Z偏移范围
     * @param count 粒子数量
     * @param range 广播范围（格），默认 256 格
     */
    virtual void broadcastParticleInRange(
        u32 type,
        f64 x, f64 y, f64 z,
        f32 velocityX, f32 velocityY, f32 velocityZ,
        f32 offsetX, f32 offsetY, f32 offsetZ,
        u32 count,
        f32 range = 256.0f) = 0;
};

} // namespace mc::server
