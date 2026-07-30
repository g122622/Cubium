/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without including limitation the rights
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

#include "server/network/ServerNetwork.hpp"

#include "common/network/backend/java/JavaBackend.hpp"
#include "common/network/ir/packets/configuration/ConfigurationPackets.hpp"
#include "common/network/ir/packets/login/LoginPackets.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>

namespace mc::server::net {

// ============================================================================
// ServerClientConnection
// ============================================================================

ServerClientConnection::ServerClientConnection(std::unique_ptr<mc::network::transport::ILocalTransport> localTransport,
    std::shared_ptr<ProtocolTables> tables,
    u32 sessionId)
    // 服务端发出方向 = Clientbound（对客户端而言是入站 Clientbound）
    : m_conn(std::move(localTransport), std::move(tables), mc::network::protocol::PacketFlow::Clientbound)
    , m_sessionId(sessionId)
{}

ServerClientConnection::ServerClientConnection(std::unique_ptr<mc::network::transport::TcpTransport> wireTransport,
    std::shared_ptr<ProtocolTables> tables,
    u32 sessionId)
    : m_conn(std::move(wireTransport), std::move(tables), mc::network::protocol::PacketFlow::Clientbound)
    , m_sessionId(sessionId)
{}

ServerClientConnection::~ServerClientConnection()
{
    close();
}

Result<void> ServerClientConnection::send(mc::network::ir::IrPacket packet)
{
    return m_conn.send(std::move(packet));
}

void ServerClientConnection::onPacket(ClientConn::PacketListener listener)
{
    m_conn.onPacket(std::move(listener));
}

void ServerClientConnection::pumpLocal()
{
    m_conn.pumpLocal();
}

void ServerClientConnection::drainInbound()
{
    // 锁内一次性快照整队 + 取 handler 引用，锁外派发：handler 可能递归触发入站
    // （如回包）或增删连接，锁外执行避免重入死锁。镜像 ServerNetwork::tick() 的
    // Local 快照-后-pump 模式。handler 仅由 onClientConnect 装配一次后不再改，
    // 拷贝一份本地副本后即可锁外安全调用。
    std::deque<mc::network::ir::IrPacket> local;
    decltype(m_inboundHandler) handler;
    {
        std::lock_guard<std::mutex> lock(m_inboundQueueMutex);
        local.swap(m_inboundQueue);
        handler = m_inboundHandler;
    }
    if (!handler || local.empty()) {
        return;
    }
    for (auto& pkt : local) {
        handler(pkt);
    }
}

void ServerClientConnection::close()
{
    m_conn.close();
    m_state = HandshakeState::Disconnected;
}

void ServerClientConnection::disconnect(const std::string& reason)
{
    // 幂等：已断开的连接不再发 Disconnect 包。
    if (!isConnected()) {
        return;
    }

    // reason 为纯文本，由 codec 编码为 NBT StringTag（vanilla Component.literal(text) 纯文本
    // 折叠路径）。此前误用 JSON 字符串 + writeString，与 vanilla Component NBT 线格式不符，
    // 客户端按 NBT 解码时把字符串首字节当 tag id，报 “Invalid tag id”。
    const std::string& textReason = reason;

    // 按当前阶段发对应 Clientbound Disconnect 包。Handshaking/Status 阶段无
    // Disconnect 包定义（协议未规定），直接跳过发包仅断连。
    const auto phase = m_conn.phase();
    if (phase == mc::network::protocol::ConnectionProtocol::Login) {
        mc::network::ir::login::Disconnect dc;
        dc.reason = textReason;
        (void)send(mc::network::ir::IrPacket{phase, mc::network::ir::LoginPacket{std::move(dc)}});
    } else if (phase == mc::network::protocol::ConnectionProtocol::Configuration) {
        mc::network::ir::configuration::Disconnect dc;
        dc.reason = textReason;
        (void)send(mc::network::ir::IrPacket{phase, mc::network::ir::ConfigurationPacket{std::move(dc)}});
    } else if (phase == mc::network::protocol::ConnectionProtocol::Play) {
        mc::network::ir::play::Disconnect dc;
        dc.reason = textReason;
        (void)send(mc::network::ir::IrPacket{phase, mc::network::ir::PlayPacket{std::move(dc)}});
    }

    close();
}

// ============================================================================
// ServerNetwork
// ============================================================================

ServerNetwork::ServerNetwork()
{
    // 构建一次 Java 协议表，所有连接共享（表本身无状态、线程安全）
    mc::network::backend::java::JavaBackend backend;
    m_tables = backend.provideProtocolTables();
}

ServerNetwork::~ServerNetwork()
{
    if (m_accepting.load()) {
        m_accepting.store(false);
        if (m_acceptor) {
            asio::error_code ec;
            m_acceptor->close(ec);
        }
    }
    if (m_acceptThread && m_acceptThread->joinable()) {
        m_acceptThread->join();
    }
    // 显式先清空所有连接：每个 ServerClientConnection 析构会 close 其 TcpTransport 并
    // join 接收线程，而接收线程在关闭时可能触发 onDisconnect→_notifyDisconnect（锁
    // m_disconnectedSessionsMutex）。此处所有 mutex 仍存活，安全。若交由成员按声明逆序
    // 销毁，m_connections 会在 m_disconnectedSessionsMutex 之后析构，触发已析构 mutex。
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        m_connections.clear();
    }
}

Result<void> ServerNetwork::startAccept(u16 port, u32 maxConnections)
{
    m_listenPort = port;
    m_maxConnections = maxConnections;

    m_ioContext = std::make_unique<asio::io_context>();
    asio::error_code ec;
    auto endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port);
    m_acceptor = std::make_unique<asio::ip::tcp::acceptor>(*m_ioContext, endpoint);
    m_acceptor->set_option(asio::ip::tcp::acceptor::reuse_address(true), ec);

    m_accepting.store(true);
    m_acceptThread = std::make_unique<std::thread>([this] { _beginAccept(); });
    spdlog::info("ServerNetwork listening on port {} (max {})", port, maxConnections);
    return Result<void>::ok();
}

void ServerNetwork::_beginAccept()
{
    while (m_accepting.load()) {
        asio::ip::tcp::socket socket(*m_ioContext);
        asio::error_code ec;
        m_acceptor->accept(socket, ec);
        if (ec) {
            if (m_accepting.load()) {
                spdlog::warn("ServerNetwork accept failed: {}", ec.message());
            }
            continue;
        }

        const u32 sessionId = m_nextSessionId.fetch_add(1);
        auto transport = std::make_unique<mc::network::transport::TcpTransport>();
        transport->attachConnectedSocket(std::move(socket));
        // 接收线程检测到对端断开时仅把 sessionId 入延迟队列，主线程 tick() 再回调清理。
        // 在 transport move 进 Connection 前安装（move 后无法再触及 transport）。
        transport->onDisconnect([this, sessionId] { _notifyDisconnect(sessionId); });

        auto conn = std::make_unique<ServerClientConnection>(std::move(transport), m_tables, sessionId);
        ServerClientConnection* raw = conn.get();
        {
            std::lock_guard<std::mutex> lock(m_connectionsMutex);
            m_connections.push_back(std::move(conn));
        }
        if (m_onConnect) {
            m_onConnect(*raw);
        }
    }
}

void ServerNetwork::_notifyDisconnect(u32 sessionId)
{
    // 接收线程调用：仅入队 sessionId，不碰连接/session map（跨线程不安全）。
    std::lock_guard<std::mutex> lock(m_disconnectedSessionsMutex);
    m_disconnectedSessions.push_back(sessionId);
}

ServerClientConnection* ServerNetwork::createLocalClientSide(
    std::unique_ptr<mc::network::transport::ILocalTransport>* outClientSide)
{
    auto pair = mc::network::transport::LocalTransportPair::create();
    *outClientSide = std::move(pair.client);

    // 集成服本地客户端固定 sessionId == 0
    auto conn = std::make_unique<ServerClientConnection>(std::move(pair.server), m_tables, 0u);
    ServerClientConnection* raw = conn.get();
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        m_connections.push_back(std::move(conn));
    }
    if (m_onConnect) {
        m_onConnect(*raw);
    }
    return raw;
}

void ServerNetwork::addConnection(std::unique_ptr<ServerClientConnection> conn)
{
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    m_connections.push_back(std::move(conn));
}

void ServerNetwork::removeConnection(u32 sessionId)
{
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    m_connections.erase(
        std::remove_if(m_connections.begin(),
            m_connections.end(),
            [sessionId](const std::unique_ptr<ServerClientConnection>& c) { return c->sessionId() == sessionId; }),
        m_connections.end());
}

ServerClientConnection* ServerNetwork::find(u32 sessionId)
{
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    for (auto& c : m_connections) {
        if (c->sessionId() == sessionId) {
            return c.get();
        }
    }
    return nullptr;
}

void ServerNetwork::tick()
{
    // 取快照后在锁外处理，避免 handler 回调内增删连接时死锁。
    std::vector<ServerClientConnection*> locals;
    std::vector<ServerClientConnection*> wires;
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        locals.reserve(m_connections.size());
        wires.reserve(m_connections.size());
        for (auto& c : m_connections) {
            if (!c->isConnected()) {
                continue;
            }
            if (c->isLocalMode()) {
                locals.push_back(c.get());
            } else {
                wires.push_back(c.get()); // Wire 模式：接收线程已 enqueueInbound，此处主线程出队派发
            }
        }
    }
    // Local：pump 对端投递的包（主线程直接触发监听器）。
    for (auto* c : locals) {
        c->pumpLocal();
    }
    // Wire：主线程 drain 入站队列（消除接收线程跨线程跑游戏逻辑的隐患）。
    for (auto* c : wires) {
        c->drainInbound();
    }

    // 处理延迟断开：接收线程已把断开的 sessionId 入队，此处主线程回调 m_onDisconnect
    // 做玩家/session 清理，再从 m_connections 移除连接。快照-后-处理避免持锁回调死锁。
    std::vector<u32> disconnected;
    {
        std::lock_guard<std::mutex> lock(m_disconnectedSessionsMutex);
        disconnected.swap(m_disconnectedSessions);
    }
    if (!disconnected.empty() && m_onDisconnect) {
        std::vector<ServerClientConnection*> toNotify;
        {
            std::lock_guard<std::mutex> lock(m_connectionsMutex);
            for (auto& c : m_connections) {
                if (std::find(disconnected.begin(), disconnected.end(), c->sessionId()) != disconnected.end()) {
                    toNotify.push_back(c.get());
                }
            }
        }
        for (auto* c : toNotify) {
            m_onDisconnect(*c);
        }
    }
    if (!disconnected.empty()) {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        m_connections.erase(std::remove_if(m_connections.begin(),
                                m_connections.end(),
                                [&disconnected](const std::unique_ptr<ServerClientConnection>& c) {
                                    return std::find(disconnected.begin(), disconnected.end(), c->sessionId()) !=
                                        disconnected.end();
                                }),
            m_connections.end());
    }
}

void ServerNetwork::broadcast(const mc::network::ir::IrPacket& packet)
{
    std::vector<ServerClientConnection*> playConns;
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        for (auto& c : m_connections) {
            if (c->state() == HandshakeState::Play && c->isConnected()) {
                playConns.push_back(c.get());
            }
        }
    }
    for (auto* c : playConns) {
        // 广播时每个连接拷贝一份 IrPacket（IrPacket 含 variant，拷贝开销可接受；
        // Local 模式 send 内部按移动入队，Wire 模式 encode 消费）。
        // 单连接发送失败不中断其余广播，仅记日志。
        auto r = c->send(packet);
        if (!r.success()) {
            spdlog::warn(
                "ServerNetwork::broadcast: send failed sessionId={} : {}", c->sessionId(), r.error().toString());
        }
    }
}

} // namespace mc::server::net
