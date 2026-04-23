#pragma once

#include "MinecraftServer.hpp"
#include "common/network/connection/LocalConnection.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/world/WorldConfig.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <thread>
#include <mutex>

namespace mc::server {

/**
 * @brief 内置服务端配置
 */
struct IntegratedServerConfig {
    String worldName = "singleplayer";
    i64 seed = 0; // TODO 这个会被后面代码覆盖，导致数据流混乱，需要重构配置系统
    GameMode defaultGameMode = GameMode::Survival;
    i32 viewDistance = 6;
    i32 tickRate = 20;  // TPS
    WorldType worldType = WorldType::Default;  // 默认使用调试模式
};

/**
 * @brief 内置服务端（单机模式）
 *
 * 继承 MinecraftServer，提供：
 * - 独立线程运行
 * - LocalConnection 与客户端通信
 * - 单玩家优化
 */
class IntegratedServer : public MinecraftServer {
public:
    IntegratedServer();
    ~IntegratedServer() override;

    // 禁止拷贝
    IntegratedServer(const IntegratedServer&) = delete;
    IntegratedServer& operator=(const IntegratedServer&) = delete;

    // ========== IServer 接口实现 ==========

    [[nodiscard]] Result<void> initialize() override;
    void shutdown() override;
    void requestStop() override;

protected:
    void pollNetwork() override;
    void broadcastPacket(const u8* data, size_t size) override;

    void broadcastLightUpdate(ChunkCoord x, ChunkCoord z, i32 sectionY,
                              const std::vector<u8>& skyLight,
                              const std::vector<u8>& blockLight,
                              bool trustEdges) override;

    [[nodiscard]] PlayerId getPlayerIdForSession(u32 sessionId) const override {
        (void)sessionId;
        return m_clientPlayerId;
    }

    void sendPacketToPlayer(PlayerId playerId, const u8* data, size_t size) override {
        if (playerId == m_clientPlayerId) {
            sendToClient(data, size);
        }
    }

    // ========== 数据包处理（特有逻辑） ==========

    void handleLoginRequestPacket(u32 sessionId, const u8* data, size_t size) override;
    void handleBlockPlacementPacket(PlayerId playerId, const u8* data, size_t size) override;
    void handleHotbarSelectPacket(PlayerId playerId, const u8* data, size_t size) override;
    void handleCreativeInventoryActionPacket(PlayerId playerId, const u8* data, size_t size) override;
    void handleContainerClickPacket(PlayerId playerId, const u8* data, size_t size) override;
    void handleCloseContainerPacket(PlayerId playerId, const u8* data, size_t size) override;
    [[nodiscard]] bool openContainerRequest(ContainerType type, const BlockPos& pos, Player& player) override;

public:

    // ========== IntegratedServer 特有接口 ==========

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
    [[nodiscard]] network::LocalEndpoint* getClientEndpoint();

    /**
     * @brief 获取配置
     */
    [[nodiscard]] const IntegratedServerConfig& config() const noexcept { return m_integratedConfig; }

    /**
     * @brief 获取客户端玩家ID
     */
    [[nodiscard]] PlayerId clientPlayerId() const noexcept { return m_clientPlayerId; }

    /**
     * @brief 获取客户端物品栏
     */
    [[nodiscard]] PlayerInventory& clientInventory() noexcept { return m_clientInventory; }
    [[nodiscard]] const PlayerInventory& clientInventory() const noexcept { return m_clientInventory; }

    [[nodiscard]] PlayerInventory* playerInventory(PlayerId playerId) override;
    [[nodiscard]] const PlayerInventory* playerInventory(PlayerId playerId) const override;

private:
    void mainLoop();

    // 设置区块发送回调
    void setupChunkSendCallback();

    // 发送数据包
    void sendLoginResponse(bool success, PlayerId playerId, EntityId entityId, const String& username, const String& message);
    void sendTeleport(f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId);
    void sendPlayerInventory();
    void sendChunkData(ChunkCoord x, ChunkCoord z, const std::vector<u8>& data);
    void sendUnloadChunk(ChunkCoord x, ChunkCoord z);
    void sendContainerContent(const AbstractContainerMenu& menu);
    void sendOpenContainer(ContainerId containerId, mc::ContainerType type, const String& title, i32 slotCount);
    void sendCloseContainer(ContainerId containerId);
    void sendToClient(const u8* data, size_t size);
    void sendBlockBreakAnim(EntityId breakerId, i32 x, i32 y, i32 z, i8 stage);
    [[nodiscard]] bool openContainerMenu(ContainerType type, const BlockPos& pos);
    void closeCurrentContainer(bool sendClosePacket);
    void openCraftingTableMenu();

    // 玩家数据便捷方法
    ServerPlayerData* getPlayerData() {
        return m_playerManager ? m_playerManager->getPlayer(m_clientPlayerId) : nullptr;
    }

    /**
     * @brief 获取玩家实体管理器
     */
    [[nodiscard]] ServerPlayerEntityManager& playerEntityManager() { return m_playerEntityManager; }
    [[nodiscard]] const ServerPlayerEntityManager& playerEntityManager() const { return m_playerEntityManager; }

    IntegratedServerConfig m_integratedConfig;

    // 服务端线程
    std::unique_ptr<std::thread> m_serverThread;

    // 本地连接
    std::unique_ptr<network::LocalConnectionPair> m_connectionPair;
    network::LocalEndpoint* m_serverEndpoint = nullptr;

    // 客户端玩家ID
    PlayerId m_clientPlayerId = 0;

    // 客户端玩家实体ID
    EntityId m_clientEntityId = INVALID_ENTITY_ID;

    // 玩家实体管理器
    ServerPlayerEntityManager m_playerEntityManager;

    // 客户端连接（持有 shared_ptr 防止 weak_ptr 失效）
    network::ConnectionPtr m_clientConnection;

    // 客户端物品栏（单玩家特有）
    PlayerInventory m_clientInventory;
    std::unique_ptr<AbstractContainerMenu> m_openMenu;
    std::shared_ptr<IInventory> m_openInventoryOwner;
    mc::ContainerType m_openContainerType = mc::ContainerType::Player;
    BlockPos m_openContainerPos;
    ContainerId m_nextContainerId = 1;
    mutable std::mutex m_clientDataMutex;
};

} // namespace mc::server
