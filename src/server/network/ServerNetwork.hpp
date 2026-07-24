/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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
#include "common/network/buffer/RegistryByteBuf.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/pipeline/Connection.hpp"
#include "common/network/pipeline/ProtocolTableSet.hpp"
#include "common/network/protocol/PacketFlow.hpp"
#include "common/network/transport/LocalTransport.hpp"
#include "common/network/transport/TcpTransport.hpp"

#include <asio.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace mc::server::net {

/// Java 后端的连接实例类型（buffer 类型已固化为 RegistryByteBuf）
using ClientConn = mc::network::pipeline::Connection<mc::network::buffer::RegistryByteBuf>;
using ProtocolTables = mc::network::pipeline::ProtocolTableSet<mc::network::buffer::RegistryByteBuf>;

/// 单连接握手编排状态：与 Connection 的 phase 大体一致，但显式区分 Configuration 子进度
enum class HandshakeState : u8 {
    Handshaking,   ///< 等待 ClientIntention
    Login,         ///< 等待 Hello / 已发 LoginCompression+LoginFinished 等待 LoginAcknowledged
    Configuration, ///< 推送 RegistryData/UpdateTags/FinishConfiguration
    Play,          ///< 握手完成，交给 ServerPlayRouter
    Disconnected
};

/**
 * @brief 服务端单客户端连接：包裹 pipeline::Connection + 握手状态 + sessionId
 *
 * Local 模式（集成服）与 Wire 模式（独立服 TCP）共用同一套握手/Play 流程，
 * 仅构造时注入的 transport 不同。出站统一走 send(ir::IrPacket)；入站由
 * Connection::onPacket 单一监听器收，先路由握手/Configuration 包给
 * ServerHandshakeStateMachine，Play 包交给 ServerPlayRouter。
 *
 * 不持有玩家数据——玩家在握手完成（onPlayerReady）时由 IntegratedServer/StandaloneServer
 * 创建并回填 playerId；Play 路由器据此分发。
 */
class ServerClientConnection {
public:
    /// Local 模式构造（集成服）：注入一对 LocalTransport 的服务端侧
    ServerClientConnection(std::unique_ptr<mc::network::transport::ILocalTransport> localTransport,
        std::shared_ptr<ProtocolTables> tables,
        u32 sessionId);

    /// Wire 模式构造（独立服）：注入已 accept 的 TcpTransport
    ServerClientConnection(std::unique_ptr<mc::network::transport::TcpTransport> wireTransport,
        std::shared_ptr<ProtocolTables> tables,
        u32 sessionId);

    ~ServerClientConnection();

    ServerClientConnection(const ServerClientConnection&) = delete;
    ServerClientConnection& operator=(const ServerClientConnection&) = delete;

    /// 出站发送（线程安全：Local 走队列 mutex，Wire 走 socket send mutex）
    [[nodiscard]] Result<void> send(mc::network::ir::IrPacket packet);

    /// 注册入站监听器（收到任意阶段包都回调，由调用方分流握手/Play）
    void onPacket(ClientConn::PacketListener listener);

    /// Local 模式 tick 驱动：pump 对端投递的包（Wire 模式由接收线程异步驱动，无需调用）
    void pumpLocal();

    void close();

    [[nodiscard]] HandshakeState state() const noexcept { return m_state; }
    void setState(HandshakeState s) noexcept { m_state = s; }

    [[nodiscard]] u32 sessionId() const noexcept { return m_sessionId; }
    [[nodiscard]] bool isLocalMode() const noexcept { return m_conn.isLocalMode(); }
    [[nodiscard]] bool isConnected() const noexcept { return m_conn.isConnected(); }
    [[nodiscard]] mc::network::protocol::ConnectionProtocol phase() const noexcept { return m_conn.phase(); }
    void setPhase(mc::network::protocol::ConnectionProtocol p) noexcept { m_conn.setPhase(p); }

    /// 装入压缩层（收到 LoginCompression 阈值后）
    void setupCompression(i32 threshold) { m_conn.setupCompression(threshold); }

    /// 装入加密层（在线模式 RSA 握手后）
    [[nodiscard]] Result<void> setupEncryption(const std::array<u8, mc::network::crypto::kSharedSecretBytes>& secret)
    {
        return m_conn.setupEncryption(secret);
    }

    [[nodiscard]] ClientConn& raw() noexcept { return m_conn; }
    [[nodiscard]] const ClientConn& raw() const noexcept { return m_conn; }

private:
    ClientConn m_conn;
    u32 m_sessionId;
    HandshakeState m_state = HandshakeState::Handshaking;
};

/**
 * @brief 服务端网络门面：管理所有 ServerClientConnection、TCP accept、协议表
 *
 * 集成服（IntegratedServer）用 createLocalClientSide 建一对 LocalTransport，
 * 服务端侧包成 ServerClientConnection，客户端侧 ILocalTransport 交 ClientNetwork。
 * 独立服（StandaloneServer）用 startAccept 监听 TCP，accept 出 socket 经
 * TcpTransport::attachConnectedSocket 注入，包成 Wire 模式 ServerClientConnection。
 *
 * tick() 在服务端主循环调用：pump 所有 Local 模式连接（Wire 由各自接收线程驱动）。
 * 重要：handler 内禁止递归调用 tick/pumpLocal——会造成 inbox 重入死循环。
 */
class ServerNetwork {
public:
    ServerNetwork();
    ~ServerNetwork();

    ServerNetwork(const ServerNetwork&) = delete;
    ServerNetwork& operator=(const ServerNetwork&) = delete;

    /// 启动 TCP 监听（独立服）；集成服不调用
    [[nodiscard]] Result<void> startAccept(u16 port, u32 maxConnections);

    /// 新连接进入握手时回调（含 Local 与 TCP 两种来源）
    void onClientConnect(std::function<void(ServerClientConnection&)> cb) { m_onConnect = std::move(cb); }

    /**
     * @brief Local 模式：创建一对 LocalTransport，服务端侧包成 ServerClientConnection 并登记
     * @param outClientSide 输出客户端侧 ILocalTransport（交 ClientNetwork::connectLocal）
     * @return 服务端侧连接的裸指针（所有权归 ServerNetwork）
     */
    [[nodiscard]] ServerClientConnection* createLocalClientSide(
        std::unique_ptr<mc::network::transport::ILocalTransport>* outClientSide);

    void addConnection(std::unique_ptr<ServerClientConnection> conn);
    void removeConnection(u32 sessionId);
    [[nodiscard]] ServerClientConnection* find(u32 sessionId);

    /// tick：pump 所有 Local 模式连接；TCP 连接的接收由各 transport 接收线程驱动
    void tick();

    /// 向所有已进入 Play 阶段的连接广播一个 IR 包
    void broadcast(const mc::network::ir::IrPacket& packet);

    /// 协议表（供 ServerClientConnection 构造时共享）
    [[nodiscard]] std::shared_ptr<ProtocolTables> tables() const noexcept { return m_tables; }

private:
    /// TCP accept 异步循环：accept 出 socket → 包成 Wire 模式 ServerClientConnection → 回调 onConnect
    void _beginAccept();

    std::shared_ptr<ProtocolTables> m_tables;
    std::vector<std::unique_ptr<ServerClientConnection>> m_connections;
    std::mutex m_connectionsMutex;

    // TCP accept（独立服）
    std::unique_ptr<asio::io_context> m_ioContext;
    std::unique_ptr<asio::ip::tcp::acceptor> m_acceptor;
    std::unique_ptr<std::thread> m_acceptThread;
    std::atomic<bool> m_accepting{false};
    u16 m_listenPort = 0;
    u32 m_maxConnections = 0;

    std::function<void(ServerClientConnection&)> m_onConnect;
    std::atomic<u32> m_nextSessionId{1}; // sessionId 0 保留给集成服本地客户端
};

} // namespace mc::server::net
