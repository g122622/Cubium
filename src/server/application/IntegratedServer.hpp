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

#include <memory>
#include "MinecraftServer.hpp"
#include "common/core/DefaultValues.hpp"
#include "common/core/GameDirectory.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/network/connection/LocalConnection.hpp"
#include "common/world/WorldConfig.hpp"
#include "server/settings/ServerSettings.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <mutex>
#include <thread>

namespace mc::server {

/**
 * @brief 内置服务器初始化参数
 *
 * 用于从客户端传入集成服务器的世界创建参数。
 * 运行时配置通过 ServerSettings 管理。
 */
struct IntegratedServerParams {
    std::string worldName = defaults::integratedServer::worldName;
    std::string gameDirectoryRoot;
    i64 seed = defaults::integratedServer::seed;
    GameMode defaultGameMode = GameMode::Survival;
    i32 viewDistance = defaults::integratedServer::viewDistance;
    i32 tickRate = defaults::integratedServer::tickRate;
    WorldType worldType = WorldType::Default;
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

protected:
    void pollNetwork() override;
    void broadcastPacket(const u8* data, size_t size) override;

    [[nodiscard]] PlayerId getPlayerIdForSession(u32 sessionId) const override
    {
        (void)sessionId;
        return m_clientPlayerId;
    }

    void sendPacketToPlayer(PlayerId playerId, const u8* data, size_t size) override
    {
        if (playerId == m_clientPlayerId) {
            sendToClient(data, size);
        }
    }

    // ========== 数据包处理（特有逻辑） ==========

    void handleLoginRequestPacket(u32 sessionId, const u8* data, size_t size) override;
    void handleHotbarSelectPacket(PlayerId playerId, const u8* data, size_t size) override;
    void handleContainerClickPacket(PlayerId playerId, const u8* data, size_t size) override;
    void handleCloseContainerPacket(PlayerId playerId, const u8* data, size_t size) override;
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
    void mainLoop();

    // 发送数据包
    void sendLoginResponse(
        bool success, PlayerId playerId, EntityId entityId, const std::string& username, const std::string& message);
    void sendTeleport(f64 x, f64 y, f64 z, f32 yaw, f32 pitch, u32 teleportId);
    void sendPlayerInventory();
    void sendContainerContent(const AbstractContainerMenu& menu);
    void sendOpenContainer(ContainerId containerId, mc::ContainerType type, const std::string& title, i32 slotCount);
    void sendCloseContainer(ContainerId containerId);
    void sendToClient(const u8* data, size_t size);
    void sendBlockBreakAnim(EntityId breakerId, i32 x, i32 y, i32 z, i8 stage);
    [[nodiscard]] bool openContainerMenu(ContainerType type, const BlockPos& pos);
    void closeCurrentContainer(bool sendClosePacket);
    void openCraftingTableMenu();

    void onCreativeInventoryInitialized(PlayerId playerId, PlayerInventory& inventory) override;
    [[nodiscard]] ItemStack getHeldItemForPlacement(PlayerId playerId) override;
    [[nodiscard]] i32 getSelectedHotbarSlot(PlayerId playerId) override;
    void setInventoryItem(PlayerId playerId, i32 slotIndex, const ItemStack& stack) override;
    void syncPlayerInventory(PlayerId playerId) override;
    [[nodiscard]] bool tryOpenCraftingContainer(PlayerId playerId, const BlockPos& pos) override;

    // 玩家数据便捷方法
    ServerPlayerData* getPlayerData()
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
