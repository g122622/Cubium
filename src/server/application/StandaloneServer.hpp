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
#include "common/core/GameDirectory.hpp"
#include "server/network/TcpServer.hpp"
#include "server/settings/ServerSettings.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <atomic>
#include <filesystem>
#include <memory>
#include <thread>
#include <unordered_map>

namespace mc::server {

/**
 * @brief 独立服务器启动参数
 *
 * 仅用于指定配置文件路径，其他配置通过配置文件管理。
 */
struct StandaloneServerParams {
    std::optional<std::string> configPath;
};

/**
 * @brief 独立服务器（多人模式）
 *
 * 继承 MinecraftServer，提供：
 * - TCP 网络通信
 * - 多玩家支持
 * - 完整的数据包处理
 */
class StandaloneServer : public MinecraftServer {
public:
    StandaloneServer();
    ~StandaloneServer() override;

    // 禁止拷贝
    StandaloneServer(const StandaloneServer&) = delete;
    StandaloneServer& operator=(const StandaloneServer&) = delete;

    // ========== IServer 接口实现 ==========

    [[nodiscard]] Result<void> initialize() override;
    void shutdown() override;

    [[nodiscard]] bool isIntegrated() const noexcept override { return false; }
    [[nodiscard]] bool isDedicated() const noexcept override { return true; }

    /**
     * @brief 发布到局域网（独立服务器不支持）
     *
     * 独立服务器已经在启动时通过 TCP 监听对外提供服务，无需也无法再次发布到局域网。
     * 调用此方法将返回错误。
     */
    [[nodiscard]] Result<void> publishToLan(i32 port, bool allowCheats) override;

protected:
    void pollNetwork() override;
    void broadcastPacket(const u8* data, size_t size) override;

    [[nodiscard]] PlayerId getPlayerIdForSession(u32 sessionId) const override
    {
        return m_playerManager->getPlayerIdBySession(sessionId);
    }

    void sendPacketToPlayer(PlayerId playerId, const u8* data, size_t size) override
    {
        auto* player = m_playerManager->getPlayer(playerId);
        if (player && player->hasConnection()) {
            player->send(data, size);
        }
    }

    // ========== 数据包处理（特有逻辑） ==========

    void handleLoginRequestPacket(u32 sessionId, const u8* data, size_t size) override;
    void handleHotbarSelectPacket(PlayerId playerId, const u8* data, size_t size) override;
    void handleContainerClickPacket(PlayerId playerId, const u8* data, size_t size) override;
    void handleCloseContainerPacket(PlayerId playerId, const u8* data, size_t size) override;

    /**
     * @brief 回写所有在线玩家运行时状态到 PlayerDataManager 缓存
     *
     * 在 stop() 中 stopCore() 之前调用。遍历所有维度的在线 Player 实体，
     * 通过 PlayerDataManager::fromPlayer() 提取运行时状态，再用 savePlayer()
     * 更新缓存并标记脏。后续 stopCore() → shutdownManagers() → saveAllWorldData()
     * 会通过 PlayerDataManager::saveAll() 把缓存落盘到 RocksDB。
     */
    void savePlayerRuntimeState() override;

public:
    // ========== StandaloneServer 特有接口 ==========

    /**
     * @brief 使用参数初始化
     */
    [[nodiscard]] Result<void> initialize(const StandaloneServerParams& params);

    /**
     * @brief 启动服务端主循环线程（非阻塞）
     *
     * 与 IntegratedServer::initialize() 类似，在内部启动一个线程运行 _mainLoop()，
     * 立即返回。线程由 StandaloneServer 自身持有，stop() 会先 join 再清理，
     * 确保 savePlayerRuntimeState() 与 stopCore() 执行时主循环已退出，
     * 避免与 tick() 产生数据竞争。
     *
     * @return 启动成功返回 ok，未初始化或已运行则返回错误
     */
    [[nodiscard]] Result<void> run();

    /**
     * @brief 停止服务器
     *
     * 顺序：
     * 1. 设置 m_running = false，通知主循环退出
     * 2. join 主循环线程，确保 tick() 已结束
     * 3. 调用 savePlayerRuntimeState() 回写在线玩家运行时状态
     * 4. 调用 stopCore() 落盘区块、level.dat、玩家数据
     * 5. 关闭网络服务器、保存设置、关闭性能追踪
     */
    void stop();

    /**
     * @brief 获取设置
     */
    [[nodiscard]] ServerSettings& settings() noexcept { return m_settings; }
    [[nodiscard]] const ServerSettings& settings() const noexcept { return m_settings; }

    // ========== 玩家实体管理 ==========

    [[nodiscard]] ServerPlayerEntityManager& playerEntityManager() override { return m_playerEntityManager; }
    [[nodiscard]] const ServerPlayerEntityManager& playerEntityManager() const override
    {
        return m_playerEntityManager;
    }

    [[nodiscard]] ItemStack getHeldItemForPlacement(PlayerId playerId) override;
    [[nodiscard]] i32 getSelectedHotbarSlot(PlayerId playerId) override;
    void setInventoryItem(PlayerId playerId, i32 slotIndex, const ItemStack& stack) override;
    void syncPlayerInventory(PlayerId playerId) override;
    [[nodiscard]] bool tryOpenCraftingContainer(PlayerId playerId, const BlockPos& pos) override;

private:
    void _mainLoop();

    // 加载设置
    [[nodiscard]] Result<void> _loadSettings(const std::string& path);
    void _applySettings();

    // 网络事件处理
    void _onClientConnect(TcpSession* session);
    void _onClientDisconnect(TcpSession* session, const std::string& reason);

    // 数据包发送
    void _sendLoginResponse(TcpSession* session,
        bool success,
        PlayerId playerId,
        EntityInstanceId entityId,
        const Uuid& uuid,
        const std::string& username,
        const std::string& message);

    ServerSettings m_settings;
    // 当前会话生效的设置文件路径（加载/自动保存/退出保存统一使用）
    std::filesystem::path m_settingsPath;
    // 游戏目录管理器（统一管理数据包、存档等路径）
    GameDirectory m_gameDirectory;
    std::unique_ptr<TcpServer> m_tcpServer;

    // 服务端主循环线程（由 StandaloneServer 自身持有，stop() 中先 join 再清理，
    // 避免与 tick() 产生数据竞争）
    std::unique_ptr<std::thread> m_serverThread;

    // 玩家实体管理器
    ServerPlayerEntityManager m_playerEntityManager;

    // PlayerId -> EntityInstanceId 映射（用于快速查找）
    std::unordered_map<PlayerId, EntityInstanceId> m_playerEntityIds;
};

} // namespace mc::server
