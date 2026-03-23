#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "common/network/connection/LocalConnection.hpp"
#include "common/network/connection/LocalServerConnection.hpp"
#include "common/network/packet/ProtocolPackets.hpp"
#include "common/network/sync/ChunkSync.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/world/chunk/ChunkStatus.hpp"
#include "common/world/chunk/ChunkPos.hpp"
#include "common/world/chunk/ChunkLoadTicketManager.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/world/lighting/LightType.hpp"
#include "common/entity/Player.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/entity/loot/LootTable.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/core/ServerCore.hpp"
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <vector>

namespace mc {
class AbstractContainerMenu;
class WorldLightManager;
}

namespace mc::server {
class IntegratedLightingWorld;
class IntegratedLightProvider;

/**
 * @brief 内置服务端配置
 */
struct IntegratedServerConfig {
    String worldName = "singleplayer";
    i64 seed = 0;
    GameMode defaultGameMode = GameMode::Survival;
    i32 viewDistance = 6;
    i32 tickRate = 20;  // TPS
};

/**
 * @brief 待发送区块数据
 *
 * 用于从 Worker 线程传递序列化后的区块数据到主线程。
 */
struct PendingChunkSend {
    ChunkCoord x;
    ChunkCoord z;
    std::vector<u8> serializedData;
};

/**
 * @brief 内置服务端
 *
 * 参考 Minecraft Java 版的 IntegratedServer：
 * - 运行在独立线程
 * - 20 TPS 固定时间步进
 * - 通过 LocalConnection 与客户端通信
 * - 使用 ServerCore 进行核心逻辑管理
 */
class IntegratedServer {
public:
    IntegratedServer();
    ~IntegratedServer();

    // 禁止拷贝
    IntegratedServer(const IntegratedServer&) = delete;
    IntegratedServer& operator=(const IntegratedServer&) = delete;

    /**
     * @brief 初始化并启动服务端线程
     */
    [[nodiscard]] Result<void> initialize(const IntegratedServerConfig& config);

    /**
     * @brief 停止服务端线程
     */
    void stop();

    /**
     * @brief 获取客户端连接端点
     *
     * 客户端通过此端点发送/接收数据
     */
    network::LocalEndpoint* getClientEndpoint();

    /**
     * @brief 是否正在运行
     */
    [[nodiscard]] bool isRunning() const noexcept { return m_running.load(); }

    /**
     * @brief 获取配置
     */
    [[nodiscard]] const IntegratedServerConfig& config() const noexcept { return m_config; }

    /**
     * @brief 获取当前 tick 数
     */
    [[nodiscard]] u64 tickCount() const noexcept;

    /**
     * @brief 获取日光时间
     */
    [[nodiscard]] i64 dayTime() const noexcept;

    /**
     * @brief 获取游戏时间
     */
    [[nodiscard]] i64 gameTime() const noexcept;

    /**
     * @brief 获取 ServerCore
     */
    [[nodiscard]] ServerCore& serverCore() { return *m_serverCore; }
    [[nodiscard]] const ServerCore& serverCore() const { return *m_serverCore; }

private:
    [[nodiscard]] static u64 chunkKey(ChunkCoord x, ChunkCoord z) {
        return (static_cast<u64>(static_cast<u32>(x)) << 32) | static_cast<u32>(z);
    }

    static void chunkKeyToCoord(u64 key, ChunkCoord& x, ChunkCoord& z) {
        x = static_cast<ChunkCoord>(key >> 32);
        z = static_cast<ChunkCoord>(key & 0xFFFFFFFF);
    }

    void mainLoop();
    void tick();
    void shutdown();

    // 区块管理
    void requestChunkAsync(ChunkCoord x, ChunkCoord z);
    void processPendingChunkSends();
    void sendChunkToClient(ChunkCoord x, ChunkCoord z);

    // 票据系统
    void handlePlayerChunkMove(ChunkCoord newChunkX, ChunkCoord newChunkZ);
    void onChunkLevelChanged(ChunkCoord x, ChunkCoord z, i32 oldLevel, i32 newLevel);
    void processPendingChunkUnloads();

    // 光照链路
    void initializeChunkLighting(ChunkCoord x, ChunkCoord z);
    void tickLighting();
    void onBlockStateChangedForLighting(i32 x, i32 y, i32 z, i32 oldLightLevel, i32 newLightLevel);
    void markLightChanged(LightType type, const SectionPos& pos);
    void syncLightDataToChunk(LightType type, const SectionPos& pos);
    void broadcastLightUpdate(LightType type, const SectionPos& pos);

    // 网络事件处理
    void onPacketReceived(const u8* data, size_t size);

    // 数据包处理
    void handleLoginRequest(const u8* data, size_t size);
    void handlePlayerMove(const u8* data, size_t size);
    void handleBlockInteraction(const u8* data, size_t size);
    void handleBlockPlacement(const u8* data, size_t size);
    void handleHotbarSelect(const u8* data, size_t size);
    void handleContainerClick(const u8* data, size_t size);
    void handleCloseContainer(const u8* data, size_t size);
    void handleTeleportConfirm(const u8* data, size_t size);
    void handleKeepAlive(const u8* data, size_t size);
    void handleChatMessage(const u8* data, size_t size);

    // 挖掘进度处理
    void updatePlayerMining(const BlockPos& pos, network::BlockInteractionAction action);
    void tickPlayerMining();
    void broadcastMiningProgress();

    // 发送数据包
    void sendLoginResponse(bool success, PlayerId playerId, const String& username, const String& message);
    void sendKeepAlive(u64 timestamp);
    void sendTeleport(f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId);
    void sendBlockUpdate(i32 x, i32 y, i32 z, u32 blockStateId);
    void sendPlayerInventory();
    void sendChunkData(ChunkCoord x, ChunkCoord z, const std::vector<u8>& data);
    void sendUnloadChunk(ChunkCoord x, ChunkCoord z);
    void sendContainerContent(const mc::AbstractContainerMenu& menu);
    void sendOpenContainer(ContainerId containerId, ContainerType type, const String& title, i32 slotCount);
    void sendCloseContainer(ContainerId containerId);
    void sendToClient(const u8* data, size_t size);
    void sendTimeUpdate();
    void sendWeatherUpdate();
    void sendInitialWeatherState();
    void sendBlockBreakAnim(EntityId breakerId, i32 x, i32 y, i32 z, i8 stage);
    void openCraftingTableMenu();

    /**
     * @brief 获取玩家数据（便捷方法）
     */
    ServerPlayerData* getPlayerData() {
        return m_serverCore ? m_serverCore->getPlayer(m_clientPlayerId) : nullptr;
    }

    IntegratedServerConfig m_config;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_initialized{false};

    // 核心逻辑管理器
    std::unique_ptr<ServerCore> m_serverCore;
    PlayerId m_clientPlayerId = 0;  ///< 客户端玩家ID

    // 区块管理器（异步生成）
    std::unique_ptr<ServerChunkManager> m_chunkManager;

    // 光照系统（内置服专用）
    std::unique_ptr<IntegratedLightingWorld> m_lightingWorld;
    std::unique_ptr<IntegratedLightProvider> m_lightProvider;
    std::unique_ptr<WorldLightManager> m_lightManager;

    // 跨线程发送队列
    std::vector<PendingChunkSend> m_pendingSends;
    std::mutex m_pendingSendsMutex;

    // 服务端线程
    std::unique_ptr<std::thread> m_serverThread;

    // 本地连接
    std::unique_ptr<network::LocalConnectionPair> m_connectionPair;
    network::LocalEndpoint* m_serverEndpoint = nullptr;

    /// 客户端连接（持有 shared_ptr 以防止 ServerPlayerData 中的 weak_ptr 失效）
    network::ConnectionPtr m_clientConnection;

    // 客户端特有数据（容器、物品栏等）
    struct ClientGameData {
        mc::PlayerInventory inventory;  ///< 玩家物品栏
        std::unique_ptr<mc::AbstractContainerMenu> openMenu;
        ContainerType openContainerType = ContainerType::Player;
        ContainerId nextContainerId = 1;
    };
    ClientGameData m_clientData;
    mutable std::mutex m_clientDataMutex;

    // 区块加载票据管理器
    std::unique_ptr<world::ChunkLoadTicketManager> m_ticketManager;

    // 延迟卸载队列（防抖，避免边缘区块瞬时闪动）
    std::unordered_map<u64, u64> m_pendingChunkUnloads;
    static constexpr u64 CHUNK_UNLOAD_GRACE_TICKS = 8;

    // 玩家当前区块位置（用于检测跨区块移动）
    ChunkCoord m_lastPlayerChunkX = std::numeric_limits<ChunkCoord>::max();
    ChunkCoord m_lastPlayerChunkZ = std::numeric_limits<ChunkCoord>::max();

    // 日光周期
    bool m_daylightCycleEnabled = true;

    // 上次发送的天气强度（用于检测变化）
    f32 m_lastSentRainStrength = 0.0f;
    f32 m_lastSentThunderStrength = 0.0f;

    // 服务端实体管理器
    EntityManager m_entityManager;

    // 实体位置追踪（用于同步给客户端）
    struct EntityTrackData {
        Vector3 lastPosition;
        f32 lastYaw = 0.0f;
        f32 lastPitch = 0.0f;
        bool needsFullUpdate = true;
    };
    std::unordered_map<EntityId, EntityTrackData> m_entityTrackData;

    // 玩家挖掘状态追踪
    struct PlayerMiningState {
        BlockPos position;          ///< 挖掘位置
        f32 progress = 0.0f;        ///< 当前进度 (0.0 - 1.0)
        u8 lastStage = 255;         ///< 上次广播的阶段 (用于检测变化)
        bool active = false;        ///< 是否正在挖掘
        u64 startTick = 0;          ///< 开始tick
    };
    PlayerMiningState m_playerMining;

    // 掉落表管理器
    loot::LootTableManager m_lootTableManager;

    // 物理引擎碰撞世界适配器
    class ServerCollisionWorld : public ICollisionWorld {
    public:
        ServerCollisionWorld(ServerChunkManager& chunkManager) : m_chunkManager(chunkManager) {}
        [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override;
        [[nodiscard]] bool isWithinWorldBounds(i32 x, i32 y, i32 z) const override;
        [[nodiscard]] const ChunkData* getChunkAt(ChunkCoord x, ChunkCoord z) const override;
        [[nodiscard]] i32 getMinBuildHeight() const override { return 0; }
        [[nodiscard]] i32 getMaxBuildHeight() const override { return 256; }
    private:
        ServerChunkManager& m_chunkManager;
    };

    std::unique_ptr<ServerCollisionWorld> m_collisionWorld;
    std::unique_ptr<PhysicsEngine> m_physicsEngine;

    /**
     * @brief 处理从区块生成的实体
     */
    void handleSpawnedEntities(const std::vector<SpawnedEntityData>& entities);

    /**
     * @brief 发送实体生成包到客户端
     */
    void sendEntitySpawnPackets(const std::vector<std::pair<EntityId, const SpawnedEntityData*>>& entities);

    /**
     * @brief 同步实体位置到客户端
     *
     * 检查所有实体的位置变化，发送EntityTeleport包到客户端
     */
    void syncEntityPositions();
};

} // namespace mc::server
