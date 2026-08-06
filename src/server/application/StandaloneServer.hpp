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
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "server/network/ServerNetwork.hpp"
#include "server/settings/ServerSettings.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

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
 * - TCP 网络通信（经 ServerNetwork::startAccept → Wire 模式 ServerClientConnection）
 * - 多玩家支持（每连接一个 RemoteClientSession：握手状态机 + Play 路由器）
 * - 完整的数据包处理（Wire 入站经接收线程 enqueueInbound，主线程 drainInbound 派发）
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

    [[nodiscard]] const GameDirectory& gameDirectory() const noexcept override { return m_gameDirectory; }

protected:
    // 注：pollNetwork/broadcastPacket/getPlayerIdForSession/sendPacketToPlayer 四纯虚
    // 已于批2a 统一为 MinecraftServer 基类默认实现。StandaloneServer 为纯远程独立服，
    // 不注入本地客户端钩子（m_localClientPlayerId 保持 nullopt），基类默认实现退化为
    // 纯 PlayerManager 遍历/查询，与原 StandaloneServer 实现完全一致。不再 override。

    // ========== 数据包处理 ==========

    // 注：handleHotbarSelect/handleContainerClick/handleCloseContainer/handleOpenPlayerInventory
    // 及 5 个 inventory 查询/操作（getHeldItemForPlacement/getSelectedHotbarSlot/setInventoryItem/
    // syncPlayerInventory/tryOpenCraftingContainer）已于批9 下沉至 MinecraftServer 基类默认实现
    // （纯远程路径：InventoryManager/ContainerManager 委托）。StandaloneServer 为纯远程独立服，
    // 无本地客户端分支，直接继承基类默认即原 StandaloneServer 行为，不再 override。
    // 远程 TCP 登录的真 Java 互通已就位：RemoteSessionManager 按 server.properties online-mode 决定
    // 加密链路（批8），客户端 HelloBound→Key→setupEncryption 装加密层。新网络层登录由
    // ServerHandshakeStateMachine 驱动，handleLoginRequestPacket 已从基类移除。

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

private:
    void _mainLoop();

    // 加载设置
    [[nodiscard]] Result<void> _loadSettings(const std::string& path);
    void _applySettings();

    /// 装配 ContainerManager 回调（菜单工厂 + 开/关/更新事件→客户端协议包）。
    /// 必须在 initializeInteractionManagers()（创建 m_containerManager）之后调用，
    /// 否则 containerManager() 返回空指针解引用崩溃。
    void _setupContainerCallbacks();

    // 远程 TCP 会话管理（批2c 下沉至 RemoteSessionManager 门面）：
    // onClientConnect/onClientDisconnect 在 startAccept 前注册到 m_serverNetwork，
    // 内部建 RemoteClientSession + 握手/Play 派发 + 玩家创建/断开清理。
    // 批9：门面成员 m_remoteSessionManager 已上提 MinecraftServer 基类，initialize() 经
    // 基类 _setupRemoteSessions 构造、stop() 经基类 _shutdownRemoteSessions 保销毁顺序
    // （session 持 ServerClientConnection&，须先于 m_serverNetwork 销毁）。
    /// 独立服压缩阈值（Java 默认 256）。在线模式按 server.properties online-mode 决定
    /// （批8：默认 false 离线基线，置 true 走 RSA+AES 加密链路支持真 Java 互通）。
    static constexpr i32 kStandaloneCompressionThreshold = 256;

    ServerSettings m_settings;
    // 当前会话生效的设置文件路径（加载/自动保存/退出保存统一使用）
    std::filesystem::path m_settingsPath;
    // 游戏目录管理器（统一管理数据包、存档等路径）
    GameDirectory m_gameDirectory;

    // 服务端主循环线程（由 StandaloneServer 自身持有，stop() 中先 join 再清理，
    // 避免与 tick() 产生数据竞争）
    std::unique_ptr<std::thread> m_serverThread;
};

} // namespace mc::server
