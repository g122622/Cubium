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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "server/network/base/ServerClientConnection.hpp"
#include "server/network/handshake/ServerHandshake.hpp"

namespace mc::server::net {

class ServerPlayHandler;

/**
 * @brief 单个客户端的会话簿记：协议状态 + 入站派发
 *
 * 值持有 `ServerHandshakeStateMachine`（持 `ServerClientConnection&`）与 `ServerPlayHandler&`
 * 单例门面，另记 `playerId` 与 `sessionId`。两者皆按连接隔离的多实例安全对象，故本类可经
 * `unique_ptr` 存入容器。
 *
 * 生命周期约束：`m_connection` 是非拥有指针，其所有权归 `ServerNetwork::m_connections`。
 * 本类必须先于对应 `ServerClientConnection` 销毁（子类 stop() 中先清会话再 reset `ServerNetwork`）。
 *
 * 入站派发：`handleInbound` 先交握手状态机；未被消费的 Play 包在 phase / playerId 双重守卫后
 * 交 `ServerPlayHandler::route`。守卫归位于此（而非某个独立路由器）的原因：phase 守卫需要连接
 * 状态、playerId 守卫需要会话状态，二者都只有会话层持有。
 */
class ClientSession {
public:
    ClientSession(ServerClientConnection& conn,
        bool isOfflineMode,
        i32 compressionThreshold,
        ServerPlayHandler& playHandler,
        PlayerId playerId,
        u32 sessionId)
        : m_connection(&conn)
        , m_handshake(conn, isOfflineMode, compressionThreshold)
        , m_playHandler(playHandler)
        , m_playerId(playerId)
        , m_sessionId(sessionId)
    {}

    ClientSession(const ClientSession&) = delete;
    ClientSession& operator=(const ClientSession&) = delete;
    // 不可移动：ServerHandshakeStateMachine 含引用成员（不可重绑），移动赋值被删除。
    // 本类经 unique_ptr 存入容器，无需移动语义。
    ClientSession(ClientSession&&) = delete;
    ClientSession& operator=(ClientSession&&) = delete;

    [[nodiscard]] ServerHandshakeStateMachine& handshake() noexcept { return m_handshake; }
    [[nodiscard]] ServerClientConnection* connection() const noexcept { return m_connection; }
    [[nodiscard]] u32 sessionId() const noexcept { return m_sessionId; }

    /// 握手完成后回填玩家ID（构造时占位 0）
    void setPlayerId(PlayerId playerId) noexcept { m_playerId = playerId; }
    [[nodiscard]] PlayerId playerId() const noexcept { return m_playerId; }

    /**
     * @brief 派发一个入站 IR 包
     *
     * 先交握手状态机（Handshake/Status/Login/Configuration 包在此消费），
     * 未被消费的 Play 包经守卫后交 `ServerPlayHandler::route`。
     * 所有丢弃路径都返回成功——它们是协议上的降级丢弃，不是调用方的错误。
     */
    [[nodiscard]] Result<void> handleInbound(const mc::network::ir::IrPacket& packet);

private:
    ServerClientConnection* m_connection; // 非拥有，所有权归 ServerNetwork
    ServerHandshakeStateMachine m_handshake;
    ServerPlayHandler& m_playHandler;
    PlayerId m_playerId;
    u32 m_sessionId;
};

} // namespace mc::server::net
