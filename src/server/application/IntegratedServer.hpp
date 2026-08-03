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
#include "common/command/ICommandSource.hpp" // for Uuid
#include "common/core/DefaultValues.hpp"
#include "common/core/GameDirectory.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/AbstractContainerMenu.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/transport/LocalTransport.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/WorldConfig.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/network/ServerHandshake.hpp"
#include "server/network/ServerNetwork.hpp"
#include "server/network/ServerPlayRouter.hpp"
#include "server/settings/ServerSettings.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

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
    i32 simulationDistance;
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
     * @brief 该玩家是否为单机/LAN 主机（本地客户端玩家）。
     *
     * 对齐 Java IntegratedServer.isSingleplayerOwner。主机身份即 m_clientPlayerId，
     * 不依赖 allowCommands（与 resolveOpLevel 的作弊提升判定解耦）。
     */
    [[nodiscard]] bool isSingleplayerOwner(PlayerId playerId) const noexcept override
    {
        return m_clientPlayerId != 0 && playerId == m_clientPlayerId;
    }

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
    /**
     * @brief 主循环 tick：先驱动基类世界/实体/网络 tick，再同步打开容器的动态数据。
     *
     * 熔炉菜单的燃烧/熔炼进度每 tick 从方块实体刷新到 tracked int，经
     * detectAndSendChanges 检测变化后由 int 监听器发 WindowPropertyPacket 下推客户端。
     */
    void tick() override;

    // 注：pollNetwork/broadcastPacket/getPlayerIdForSession/sendPacketToPlayer 四纯虚
    // 已于批2a 统一为 MinecraftServer 基类默认实现（含本地客户端钩子注入），
    // 本子类不再 override。本地客户端经 initialize() 注入 m_localClientSender +
    // _onClientPlayerReady() 设置 m_localClientPlayerId 完成双路径接驳。

    // ========== 数据包处理（特有逻辑） ==========

    // 注：handleLoginRequestPacket 已移除——登录全由 ServerHandshake 驱动，
    // 玩家创建在 _onClientPlayerReady 回调中完成。
    void handleHotbarSelectPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet) override;
    void handleContainerClickPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet) override;
    void handleCloseContainerPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet) override;
    void handleOpenPlayerInventoryPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet) override;
    [[nodiscard]] bool openContainerRequest(ContainerType type, const BlockPos& pos, Player& player) override;

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
     * @brief 取出客户端侧 LocalTransport（交 ClientNetwork::connectLocal 持有）
     *
     * 新网络层：本地客户端经 LocalTransportPair 与服务端 ServerClientConnection 配对，
     * 零拷贝直传 ir::IrPacket。客户端在 connectLocal 前调用本方法取出对端 transport。
     * 仅可调用一次（所有权转移）；二次调用返回 nullptr。
     */
    [[nodiscard]] std::unique_ptr<mc::network::transport::ILocalTransport> takeClientTransport();

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

    // 发送数据包（新网络层：构 ir::IrPacket 经 m_clientConnection->send 出站）
    // 注：play::Login 已下沉到 MinecraftServer::sendLoginResponseForConnection（共享 Local+Wire）
    void _sendTeleport(f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId);
    void _sendPlayerInventory();
    void _sendContainerContent(const AbstractContainerMenu& menu);
    void _sendOpenContainer(ContainerId containerId, mc::ContainerType type, const std::string& title, i32 slotCount);
    void _sendCloseContainer(ContainerId containerId);
    /// 向本地客户端发送 IR 包（封装 m_clientConnection->send 的连接有效性检查）
    void _sendToClientIr(mc::network::ir::IrPacket packet);
    void _sendBlockBreakAnim(EntityInstanceId breakerId, i32 x, i32 y, i32 z, i8 stage);
    /**
     * @brief 发送 WindowPropertyPacket（熔炉燃烧/熔炼进度等 tracked int 下推客户端）
     */
    void _sendWindowProperty(ContainerId containerId, i16 property, i16 value);
    [[nodiscard]] bool _openContainerMenu(ContainerType type, const BlockPos& pos);
    void _closeCurrentContainer(bool sendClosePacket);
    void _openCraftingTableMenu();
    void _openPlayerInventoryMenu();
    void _openItemPickerMenu();

    /**
     * @brief 为熔炉菜单注册 tracked int 监听器（变化时发 WindowPropertyPacket）
     * @return 监听器ID（用于关闭时移除），非熔炉菜单返回 -1
     */
    i32 _registerFurnaceIntListener(AbstractContainerMenu& menu);
    /**
     * @brief 每 tick 同步打开容器的动态数据（熔炉进度：从实体刷新 + detectAndSendChanges）
     */
    void _tickOpenContainer();

    // ========== 远程 TCP 玩家支持（局域网发布后启用）==========

    /// 集成服 LAN 发布压缩阈值（Java 默认 256）。离线模式（跳过 RSA）。
    static constexpr i32 kLanCompressionThreshold = 256;

    // 注：远程会话四件套（_onRemoteClientConnect/_onRemotePlayerReady/
    // _onRemoteClientDisconnect）已于批2c 下沉至 RemoteSessionManager 门面。本子类不再
    // 直接持有 m_remoteSessions/m_remoteSessionsMutex，门面成员 m_remoteSessionManager 已
    // 于批9 上提 MinecraftServer 基类（publishToLan 经基类 _setupRemoteSessions 构造并注册
    // 到 m_serverNetwork）。本地客户端（sessionId=0）不经此 manager，保留下方
    // _onClientPlayerReady/_installClientInboundListener 独立路径。
    // _drainDisconnectedSessions 已于批2a 提升为基类 virtual 默认实现，本子类不再 override。

    /**
     * @brief 本地客户端握手完成回调（ServerHandshake Configuration 结束后触发）
     *
     * 在此执行玩家创建序列（迁自旧 handleLoginRequestPacket）：
     * addPlayer/setupInitialPlayerState/createPlayerEntity/playerJoinDimension/
     * OP-level/saved-data-load/inventory-init/sendLoginResponse/play::Login。
     *
     * @param username 登录用户名
     * @param offlineUuid 离线模式 UUID
     */
    void _onClientPlayerReady(const std::string& username, const std::array<u8, 16>& offlineUuid);

    /**
     * @brief 安装本地客户端连接的入站监听器：握手包交 ServerHandshake，Play 包交 ServerPlayRouter
     */
    void _installClientInboundListener();

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

    IntegratedServerParams m_params;
    ServerSettings m_integratedSettings;
    GameDirectory m_gameDirectory;

    // 服务端线程
    std::unique_ptr<std::thread> m_serverThread;

    // 本地客户端侧 transport（取出交 ClientNetwork::connectLocal 前暂存）
    std::unique_ptr<mc::network::transport::ILocalTransport> m_pendingClientTransport;

    // 本地客户端连接（所有权归 m_serverNetwork，此处非拥有指针）
    mc::server::net::ServerClientConnection* m_clientConnection = nullptr;

    // 本地客户端握手状态机（Configuration 完成后触发玩家创建）
    std::unique_ptr<mc::server::net::ServerHandshakeStateMachine> m_clientHandshake;

    // 本地客户端 Play 路由器（sessionId=0）
    std::unique_ptr<mc::server::net::ServerPlayRouter> m_clientPlayRouter;

    // 客户端玩家ID
    PlayerId m_clientPlayerId = 0;

    // 客户端玩家实体ID
    EntityInstanceId m_clientEntityId = INVALID_ENTITY_ID;

    // 客户端物品栏（单玩家特有）
    PlayerInventory m_clientInventory;
    std::unique_ptr<AbstractContainerMenu> m_openMenu;
    std::shared_ptr<IInventory> m_openInventoryOwner;
    mc::ContainerType m_openContainerType = mc::ContainerType::Player;
    BlockPos m_openContainerPos;
    ContainerId m_nextContainerId = 1;
    /// 熔炉菜单 tracked int 监听器ID（-1 表示未注册），关闭时移除
    i32 m_furnaceIntListenerId = -1;
    mutable std::mutex m_clientDataMutex;

    // ========== 局域网发布（TCP 监听器）==========
    // publishToLan() 调用基类 _setupRemoteSessions 装配 m_remoteSessionManager 门面并
    // m_serverNetwork->startAccept 启动 TCP 监听，允许远程玩家通过 TCP 加入本局游戏。
    // 本地客户端仍走 LocalTransport（m_clientConnection，sessionId=0，经 createLocalClientSide
    // 在 initialize() 内联接线），与 Wire 连接共用同一 m_serverNetwork：startAccept 触
    // m_listenPort/m_ioContext/m_acceptor/m_acceptThread；createLocalClientSide 触
    // m_connections/m_onConnect——成员不相交无冲突。单 tick()（pollNetwork 内）经
    // isLocalMode() 分支同时 drain Local(pumpLocal)+Wire。
    // 批2c/批9：远程会话四件套下沉至基类 m_remoteSessionManager 门面（原 m_remoteSessions/
    // m_remoteSessionsMutex 已删，成员本身上提 MinecraftServer 基类）。session 持
    // ServerClientConnection&（非拥有，所有权归基类 m_serverNetwork），故 stop() 中
    // _shutdownRemoteSessions() 须先 reset m_remoteSessionManager 再 reset m_serverNetwork。
    // 批2b：m_serverNetwork/m_playerEntityManager 已上提 MinecraftServer 基类；远程玩家实体ID
    // 映射（原 m_remotePlayerEntityIds，写了不读的死映射）已删除。

    // 局域网发布状态
    bool m_lanPublished = false;
    i32 m_lanPort = 0;
};

} // namespace mc::server
