/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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

#include "common/core/Types.hpp"
#include "server/application/RemoteClientSession.hpp"
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

namespace mc::server::net {

/// 远程玩家世界参数（createPlayerForConnection 的 hardcore/seed/isFlat 三参打包）。
/// 两子类来源不同：StandaloneServer 取 m_settings（levelType/parseSeed/hardcore），
/// IntegratedServer 取 m_params（worldType/seed/hardcore）。由调用方经 provider 注入。
struct RemoteWorldParams {
    bool hardcore;
    i64 seed;
    bool isFlat;
};

/**
 * @brief 远程 TCP 会话管理门面（批2c 下沉自 StandaloneServer / IntegratedServer）
 *
 * 承载两子类完全重复的远程会话四件套：
 *  - onClientConnect：建 RemoteClientSession（握手状态机 + Play 路由器）+ 装配
 *    入站派发分支 + StatusInfo provider + onPlayerReady 回调。
 *  - onPlayerReady（私有）：握手完成后调 createPlayerForConnection + 回填路由器 playerId。
 *  - onClientDisconnect：移除 session + 玩家实体 + PlayerManager 会话 + 物品栏清理。
 *  - clear：保销毁顺序（session 持 ServerClientConnection&，须先于 ServerNetwork 销毁）。
 *
 * 差异注入（两子类仅此三项不同）：
 *  - logPrefix：日志前缀（"StandaloneServer" / "IntegratedServer"）。
 *  - compressionThreshold：握手压缩阈值（两子类均 256，保留参数供将来分化）。
 *  - worldParamsProvider：返回 RemoteWorldParams 的回调（来源 m_settings / m_params）。
 *
 * StatusInfo（服务器列表 ping）两子类均读基类 m_settings（motd/maxPlayers/onlineMode），
 * 完全一致，故本 manager 直接经 m_server.settings() 取值，无需注入。
 *
 * 本地客户端（sessionId=0）不进此 manager：IntegratedServer 的本地客户端经
 * m_clientHandshake/m_clientPlayRouter/_onClientPlayerReady 独立路径，与本 manager 无交叠。
 *
 * 线程模型：onClientConnect/onClientDisconnect 由 ServerNetwork::tick 在主线程回调
 * （accept 线程仅 enqueue，主线程 drainInbound 派发）；onPlayerReady 由主线程
 * drainInbound 内握手状态机触发。m_sessions 经 m_sessionsMutex 保护（与原两子类一致）。
 */
class RemoteSessionManager {
public:
    RemoteSessionManager(MinecraftServer& server,
        std::string logPrefix,
        i32 compressionThreshold,
        std::function<RemoteWorldParams()> worldParamsProvider)
        : m_server(server)
        , m_logPrefix(std::move(logPrefix))
        , m_compressionThreshold(compressionThreshold)
        , m_worldParamsProvider(std::move(worldParamsProvider))
    {}

    // 不可拷贝/移动（持引用 + mutex）。
    RemoteSessionManager(const RemoteSessionManager&) = delete;
    RemoteSessionManager& operator=(const RemoteSessionManager&) = delete;
    RemoteSessionManager(RemoteSessionManager&&) = delete;
    RemoteSessionManager& operator=(RemoteSessionManager&&) = delete;

    /// accept 线程 accept 出新 Wire 连接时回调（主线程 drainInbound 内实际触发）。
    /// 建 session + 装配入站派发 + 注册握手完成/Status 回调。
    void onClientConnect(ServerClientConnection& conn);

    /// 远程客户端断开回调（主线程 tick 内派发）：移除 session + 玩家清理。
    void onClientDisconnect(ServerClientConnection& conn);

    /// 清空所有远程会话。须在 ServerNetwork 销毁前调用（session 持 ServerClientConnection&，
    /// 连接所有权归 ServerNetwork，悬垂引用须先清）。对应两子类 stop() 中 m_remoteSessions.clear()。
    void clear() noexcept;

    /// 当前远程会话数（诊断/测试用）。
    [[nodiscard]] std::size_t sessionCount() const noexcept;

private:
    /// 握手完成回调（onClientConnect 内绑入 handshake.onPlayerReady）：创建玩家实体 +
    /// 回填路由器 playerId。
    void onPlayerReady(u32 sessionId, const std::string& username, const std::array<u8, 16>& offlineUuid);

    MinecraftServer& m_server;
    std::string m_logPrefix;
    i32 m_compressionThreshold;
    std::function<RemoteWorldParams()> m_worldParamsProvider;

    std::unordered_map<u32, std::unique_ptr<RemoteClientSession>> m_sessions;
    mutable std::mutex m_sessionsMutex;
};

} // namespace mc::server::net
