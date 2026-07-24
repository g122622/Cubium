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
#include "server/network/ServerHandshake.hpp"
#include "server/network/ServerNetwork.hpp"
#include "server/network/ServerPlayRouter.hpp"

namespace mc::server::net {

/**
 * @brief 单个远程 TCP 客户端的会话簿记（StandaloneServer / IntegratedServer LAN 共用）
 *
 * 值持有 ServerHandshakeStateMachine（持 ServerClientConnection&）与 ServerPlayRouter
 * （持 MinecraftServer&）。两者皆按连接隔离的多实例安全对象，故 RemoteClientSession
 * 可按值存入 unique_ptr 容器。
 *
 * 生命周期约束：connection 非拥有指针，其所有权归 ServerNetwork::m_connections；
 * RemoteClientSession 必须先于对应 ServerClientConnection 销毁（子类 stop() 中
 * m_remoteSessions.clear() 先于 m_serverNetwork.reset()）。
 *
 * 入站派发：ServerClientConnection 的 onPacket 监听器仅 enqueueInbound（接收线程），
 * ServerNetwork::tick() 在主线程 drainInbound 调用 setInboundHandler 装配的分支：
 *   handshake.handleInbound(pkt) 返回 true=握手/Configuration 已消费；false=Play 包
 *   交 playRouter.handle(pkt)。
 */
class RemoteClientSession {
public:
    RemoteClientSession(ServerClientConnection& conn,
        bool isOfflineMode,
        i32 compressionThreshold,
        MinecraftServer& server,
        PlayerId playerId,
        u32 sessionId)
        : m_connection(&conn)
        , m_handshake(conn, isOfflineMode, compressionThreshold)
        , m_playRouter(server, playerId, sessionId)
        , m_sessionId(sessionId)
    {}

    RemoteClientSession(const RemoteClientSession&) = delete;
    RemoteClientSession& operator=(const RemoteClientSession&) = delete;
    // 不可移动：ServerHandshakeStateMachine 含引用成员（不可重绑），移动赋值被删除。
    // 本类经 unique_ptr 存入容器，无需移动语义。
    RemoteClientSession(RemoteClientSession&&) = delete;
    RemoteClientSession& operator=(RemoteClientSession&&) = delete;

    [[nodiscard]] ServerHandshakeStateMachine& handshake() noexcept { return m_handshake; }
    [[nodiscard]] ServerPlayRouter& playRouter() noexcept { return m_playRouter; }
    [[nodiscard]] ServerClientConnection* connection() const noexcept { return m_connection; }
    [[nodiscard]] u32 sessionId() const noexcept { return m_sessionId; }

private:
    ServerClientConnection* m_connection; // 非拥有，所有权归 ServerNetwork
    ServerHandshakeStateMachine m_handshake;
    ServerPlayRouter m_playRouter;
    u32 m_sessionId;
};

} // namespace mc::server::net
