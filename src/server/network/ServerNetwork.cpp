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

void ServerClientConnection::close()
{
    m_conn.close();
    m_state = HandshakeState::Disconnected;
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
    // 仅 pump Local 模式连接（Wire 模式由各 TcpTransport 接收线程异步驱动 onBytes）。
    // 取快照后在锁外 pump，避免 handler 回调内增删连接时死锁。
    std::vector<ServerClientConnection*> locals;
    {
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        locals.reserve(m_connections.size());
        for (auto& c : m_connections) {
            if (c->isLocalMode() && c->isConnected()) {
                locals.push_back(c.get());
            }
        }
    }
    for (auto* c : locals) {
        c->pumpLocal();
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
