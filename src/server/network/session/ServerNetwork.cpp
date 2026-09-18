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

#include "server/network/session/ServerNetwork.hpp"

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/backend/java/JavaBackend.hpp"
#include "common/network/transport/LocalTransport.hpp"
#include "common/network/transport/TcpTransport.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>
#include <asio/error_code.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

namespace mc::server::net {

ServerNetwork::ServerNetwork()
{
    // 构建一次 Java 协议表，所有连接共享（表本身无状态、线程安全）
    mc::network::backend::java::JavaBackend backend;
    m_tables = backend.provideProtocolTables();
}

ServerNetwork::~ServerNetwork()
{
    // 通知 accept 循环退出：stop() 让 io_context::run() 在处理完当前回调后返回，
    // 从而可靠唤醒阻塞在 run() 中的 accept 线程。跨平台一致，避免 Linux 上
    // close() listen socket fd 无法中断阻塞 ::accept(fd) 的陷阱。
    if (m_acceptThread && m_acceptThread->joinable()) {
        if (m_ioContext) {
            m_ioContext->stop();
        }
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
    // 异步 accept 链：每次 accept 成功后递归发起新的 async_accept，直到 io_context 停止。
    // 析构时 m_ioContext->stop() 让 run() 在处理完当前回调后返回，可靠唤醒 accept 线程，
    // 跨平台一致，避免 Linux 上 close() listen socket fd 无法中断阻塞 ::accept(fd) 的陷阱。
    _doAsyncAccept();
    m_ioContext->run();
}

void ServerNetwork::_doAsyncAccept()
{
    if (!m_accepting.load() || !m_acceptor) {
        return;
    }

    m_acceptor->async_accept([this](asio::error_code ec, asio::ip::tcp::socket socket) {
        if (ec) {
            // io_context 被 stop() 后会触发 operation_aborted，此时静默退出即可
            if (m_accepting.load()) {
                spdlog::warn("ServerNetwork accept failed: {}", ec.message());
            }
            return;
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

        // 继续接受下一个连接
        _doAsyncAccept();
    });
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

} // namespace mc::server::net
