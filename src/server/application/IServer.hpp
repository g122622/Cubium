#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include <memory>

namespace mc {
class AbstractContainerMenu;
class WorldLightManager;
class PhysicsEngine;
class EntityManager;
class ServerDimensionManager;
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

    // ========== 交互管理器 ==========

    [[nodiscard]] virtual interaction::BlockInteractionManager& blockInteractionManager() = 0;
    [[nodiscard]] virtual const interaction::BlockInteractionManager& blockInteractionManager() const = 0;

    [[nodiscard]] virtual interaction::MiningManager& miningManager() = 0;
    [[nodiscard]] virtual const interaction::MiningManager& miningManager() const = 0;

    [[nodiscard]] virtual interaction::ContainerManager& containerManager() = 0;
    [[nodiscard]] virtual const interaction::ContainerManager& containerManager() const = 0;

    [[nodiscard]] virtual interaction::InventoryManager& inventoryManager() = 0;
    [[nodiscard]] virtual const interaction::InventoryManager& inventoryManager() const = 0;

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
};

} // namespace mc::server
