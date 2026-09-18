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
#include "common/network/transport/LocalTransport.hpp"
#include "server/network/base/ServerClientConnection.hpp"

#include <asio.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

namespace mc::server::net {

/**
 * @brief 服务端网络门面：管理所有 ServerClientConnection、TCP accept、协议表
 *
 * 集成服（IntegratedServer）用 createLocalClientSide 建一对 LocalTransport，
 * 服务端侧包成 ServerClientConnection，客户端侧 ILocalTransport 交 ClientNetwork。
 * 独立服（StandaloneServer）用 startAccept 监听 TCP，accept 出 socket 经
 * TcpTransport::attachConnectedSocket 注入，包成 Wire 模式 ServerClientConnection。
 *
 * tick() 在服务端主循环调用：pump 所有 Local 模式连接（Wire 由各自接收线程驱动），
 * 并对 Wire 模式连接主线程 drain 入站队列（接收线程仅 enqueueInbound，游戏逻辑
 * 在此派发，避免跨线程进入非线程安全的 MinecraftServer 状态）。
 * 重要：handler 内禁止递归调用 tick/pumpLocal——会造成 inbox 重入死循环。
 */
class ServerNetwork {
public:
    ServerNetwork();
    ~ServerNetwork();

    ServerNetwork(const ServerNetwork&) = delete;
    ServerNetwork& operator=(const ServerNetwork&) = delete;

    /// 启动 TCP 监听（独立服/LAN 发布）；集成服本地客户端不调用本方法。
    [[nodiscard]] Result<void> startAccept(u16 port, u32 maxConnections);

    /// 新连接进入握手时回调（含 Local 与 TCP 两种来源）
    void onClientConnect(std::function<void(ServerClientConnection&)> cb) { m_onConnect = std::move(cb); }

    /// 连接断开时回调（由主线程在 tick() 内派发，非接收线程）。
    /// TcpTransport 接收线程检测到断开后只把 sessionId 入延迟队列，
    /// tick() 在主线程 swap 后回调本函数——所有 session map / 玩家清理在主线程进行。
    void onClientDisconnect(std::function<void(ServerClientConnection&)> cb) { m_onDisconnect = std::move(cb); }

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
    /// TCP accept 异步循环：async_accept 链 + io_context::run() 驱动
    void _beginAccept();

    /// 发起一次异步 accept，成功后递归调用自身继续接受下一个连接
    void _doAsyncAccept();

    /// Wire 连接 transport 断开时回调（接收线程触发）：仅把 sessionId 入延迟队列，
    /// 不跨线程碰连接/session map。tick() 在主线程 swap 后回调 m_onDisconnect。
    void _notifyDisconnect(u32 sessionId);

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
    std::function<void(ServerClientConnection&)> m_onDisconnect;
    // 接收线程 push、主线程 tick() drain 的延迟断开 sessionId 队列。
    std::vector<u32> m_disconnectedSessions;
    std::mutex m_disconnectedSessionsMutex;
    std::atomic<u32> m_nextSessionId{1}; // sessionId 0 保留给集成服本地客户端
};

} // namespace mc::server::net
