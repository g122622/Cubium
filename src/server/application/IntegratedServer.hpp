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

#include "MinecraftServer.hpp"
#include "common/core/DefaultValues.hpp"
#include "common/core/GameDirectory.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/network/connection/LocalConnection.hpp"
#include "common/world/WorldConfig.hpp"
#include "server/network/TcpServer.hpp"
#include "server/settings/ServerSettings.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace mc::server {

/**
 * @brief 内置服务器初始化参数
 *
 * 用于从客户端传入集成服务器的世界创建参数。
 * 运行时配置通过 ServerSettings 管理。
 */
struct IntegratedServerParams {
    std::string worldName;
    std::string gameDirectoryRoot;
    std::string displayName;
    i64 seed;
    GameMode defaultGameMode;
    i32 viewDistance;
    i32 tickRate;
    WorldType worldType;
    /// 世界预设资源位置（数据驱动装配查 WorldPresetRegistry，如 "minecraft:normal"）
    resource::ResourceLocation worldPresetId{"minecraft", "normal"};
    Difficulty difficulty;
    bool hardcore;
    bool allowCommands; ///< 单机作弊开关，对应原版 WorldData.isAllowCommands()
    /// 是否为新创建的世界（首次打开存档前需写入初始 level.dat）
    bool isNewWorld;
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

    [[nodiscard]] bool isIntegrated() const noexcept override { return true; }
    [[nodiscard]] bool isDedicated() const noexcept override { return false; }

    /**
     * @brief 单机主机作弊提升：主机在开启作弊时运行时视为 OP（Owner 级），不写 ops.json。
     */
    [[nodiscard]] i32 resolveOpLevel(const std::string& uuid) const noexcept override;

    /**
     * @brief 发布到局域网（仅集成服务器有效）
     *
     * 启动 TCP 监听器，允许局域网内其他玩家加入本局游戏；同时可切换作弊开关。
     * 对应原版 MinecraftIntegratedServer.publishServer()。
     *
     * @param port 监听端口（1-65535）
     * @param allowCheats 是否允许作弊
     * @return 成功返回 ok；已发布/端口占用等情况返回错误
     */
    [[nodiscard]] Result<void> publishToLan(i32 port, bool allowCheats) override;

protected:
    void pollNetwork() override;
    void broadcastPacket(const u8* data, size_t size) override;

    /**
     * @brief 将会话ID转换为玩家ID
     *
     * 双路径架构：
     * - sessionId == 0：本地客户端（LocalConnection），返回 m_clientPlayerId
     * - sessionId != 0：远程 TCP 客户端，通过 PlayerManager 查询
     *
     * @param sessionId 会话ID
     * @return 玩家ID，如果无效返回 0
     */
    [[nodiscard]] PlayerId getPlayerIdForSession(u32 sessionId) const override
    {
        if (sessionId == 0) {
            return m_clientPlayerId;
        }
        return m_playerManager->getPlayerIdBySession(sessionId);
    }

    /**
     * @brief 向指定玩家发送数据包
     *
     * 双路径架构：
     * - 本地客户端（playerId == m_clientPlayerId）：直接走 LocalEndpoint（零拷贝优化）
     * - 远程 TCP 玩家：通过 ServerPlayerData::send() 走 TcpConnection
     *
     * @param playerId 玩家ID
     * @param data 数据指针
     * @param size 数据大小
     */
    void sendPacketToPlayer(PlayerId playerId, const u8* data, size_t size) override
    {
        if (playerId == m_clientPlayerId) {
            _sendToClient(data, size);
            return;
        }

        auto* player = m_playerManager->getPlayer(playerId);
        if (player != nullptr) {
            player->send(data, size);
        }
    }

    // ========== 数据包处理（特有逻辑） ==========

    void handleLoginRequestPacket(u32 sessionId, const u8* data, size_t size) override;
    void handleHotbarSelectPacket(PlayerId playerId, const u8* data, size_t size) override;
    void handleContainerClickPacket(PlayerId playerId, const u8* data, size_t size) override;
    void handleCloseContainerPacket(PlayerId playerId, const u8* data, size_t size) override;
    [[nodiscard]] bool openContainerRequest(ContainerType type, const BlockPos& pos, Player& player) override;

    /**
     * @brief 回写所有在线玩家运行时状态到 PlayerDataManager 缓存
     *
     * 在 stop() 中 clearAll 之前调用。遍历所有维度的在线 Player 实体，
     * 通过 PlayerDataManager::fromPlayer() 提取运行时状态，再用 savePlayer()
     * 更新缓存并标记脏。后续 stopCore() → shutdownManagers() → saveAllWorldData()
     * 会通过 PlayerDataManager::saveAll() 把缓存落盘到 RocksDB。
     */
    void savePlayerRuntimeState() override;

public:
    // ========== IntegratedServer 特有接口 ==========

    /**
     * @brief 初始化并启动服务端线程
     */
    [[nodiscard]] Result<void> initialize(const IntegratedServerParams& params);

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
     * @brief 获取初始化参数
     */
    [[nodiscard]] const IntegratedServerParams& params() const noexcept { return m_params; }

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
    void _mainLoop();

    // 发送数据包
    void _sendLoginResponse(bool success,
        PlayerId playerId,
        EntityInstanceId entityId,
        const std::string& username,
        const std::string& message);
    void _sendTeleport(f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId);
    void _sendPlayerInventory();
    void _sendContainerContent(const AbstractContainerMenu& menu);
    void _sendOpenContainer(ContainerId containerId, mc::ContainerType type, const std::string& title, i32 slotCount);
    void _sendCloseContainer(ContainerId containerId);
    void _sendToClient(const u8* data, size_t size);
    void _sendBlockBreakAnim(EntityInstanceId breakerId, i32 x, i32 y, i32 z, i8 stage);
    [[nodiscard]] bool _openContainerMenu(ContainerType type, const BlockPos& pos);
    void _closeCurrentContainer(bool sendClosePacket);
    void _openCraftingTableMenu();

    // ========== 远程 TCP 玩家支持（局域网发布后启用）==========

    /**
     * @brief TCP 客户端连接事件回调
     */
    void _onRemoteClientConnect(TcpSession* session);

    /**
     * @brief TCP 客户端断开事件回调
     */
    void _onRemoteClientDisconnect(TcpSession* session, const std::string& reason);

    /**
     * @brief 向远程 TCP 玩家发送登录响应
     */
    void _sendLoginResponseToSession(TcpSession* session,
        bool success,
        PlayerId playerId,
        EntityInstanceId entityId,
        const std::string& username,
        const std::string& message);

    /**
     * @brief 处理远程 TCP 玩家的登录请求
     *
     * 与本地客户端登录流程不同：
     * - 不使用 LocalServerConnection，而是创建 TcpConnection
     * - 不使用 m_clientInventory，而是使用 InventoryManager
     * - 不使用 m_openMenu 等单玩家容器字段，而是使用 ContainerManager
     * - 通过 PlayerManager 建立会话映射
     *
     * @param sessionId TCP 会话ID（非 0）
     * @param data 登录请求数据包载荷
     * @param size 数据大小
     */
    void _handleRemoteLoginRequest(u32 sessionId, const u8* data, size_t size);

    void onCreativeInventoryInitialized(PlayerId playerId, PlayerInventory& inventory) override;
    [[nodiscard]] ItemStack getHeldItemForPlacement(PlayerId playerId) override;
    [[nodiscard]] i32 getSelectedHotbarSlot(PlayerId playerId) override;
    void setInventoryItem(PlayerId playerId, i32 slotIndex, const ItemStack& stack) override;
    void syncPlayerInventory(PlayerId playerId) override;
    [[nodiscard]] bool tryOpenCraftingContainer(PlayerId playerId, const BlockPos& pos) override;

    // 玩家数据便捷方法
    ServerPlayerData* _getPlayerData()
    {
        return m_playerManager ? m_playerManager->getPlayer(m_clientPlayerId) : nullptr;
    }

    /**
     * @brief 获取玩家实体管理器
     */
    [[nodiscard]] ServerPlayerEntityManager& playerEntityManager() override { return m_playerEntityManager; }
    [[nodiscard]] const ServerPlayerEntityManager& playerEntityManager() const override
    {
        return m_playerEntityManager;
    }

    IntegratedServerParams m_params;
    ServerSettings m_integratedSettings;
    GameDirectory m_gameDirectory;

    // 服务端线程
    std::unique_ptr<std::thread> m_serverThread;

    // 本地连接
    std::unique_ptr<network::LocalConnectionPair> m_connectionPair;
    network::LocalEndpoint* m_serverEndpoint = nullptr;

    // 客户端玩家ID
    PlayerId m_clientPlayerId = 0;

    // 客户端玩家实体ID
    EntityInstanceId m_clientEntityId = INVALID_ENTITY_ID;

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

    // ========== 局域网发布（TCP 监听器）==========
    // publishToLan() 调用后创建，允许远程玩家通过 TCP 加入本局游戏。
    // 本地客户端仍走 LocalConnection（m_connectionPair），不受此监听器影响。
    std::unique_ptr<TcpServer> m_lanTcpServer;

    // 远程玩家实体ID映射（PlayerId -> EntityInstanceId），用于快速查找
    std::unordered_map<PlayerId, EntityInstanceId> m_remotePlayerEntityIds;
    mutable std::mutex m_remotePlayersMutex;

    // 局域网发布状态
    bool m_lanPublished = false;
    i32 m_lanPort = 0;
};

} // namespace mc::server
