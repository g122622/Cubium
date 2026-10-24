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

#include "server/network/base/ServerClientConnection.hpp"

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/configuration/ConfigurationPackets.hpp"
#include "common/network/ir/packets/login/LoginPackets.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/protocol/PacketFlow.hpp"
#include "common/network/transport/LocalTransport.hpp"
// Wire 构造需要 TcpTransport 完整类型（unique_ptr 派生类 → 基类的转换与析构）。
// 头文件只前置声明它，以免所有仅需连接类型的头被迫拉入 <asio.hpp>。
#include "common/network/transport/TcpTransport.hpp"

#include <algorithm>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc::server::net {

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
    // 初始化列表按成员声明顺序求值：m_peerAddress 先于 m_conn。故先在 m_peerAddress
    // 取 wireTransport->remoteAddress()（此时 wireTransport 未 move），随后 m_conn
    // 才 move wireTransport。move 后 wireTransport 为空指针，不可再用。
    // socket close 后 remote_endpoint 失效，故地址须在构造时一次性快照缓存。
    : m_peerAddress(wireTransport ? wireTransport->remoteAddress() : std::string{})
    , m_conn(std::move(wireTransport), std::move(tables), mc::network::protocol::PacketFlow::Clientbound)
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

Result<void> ServerClientConnection::sendShared(std::shared_ptr<const mc::network::pipeline::OutboundPacket> packet)
{
    return m_conn.sendShared(packet);
}

void ServerClientConnection::enqueueInbound(mc::network::ir::IrPacket packet)
{
    {
        std::lock_guard lock(m_inboundQueueMutex);
        if (m_inboundQueue.size() < MAX_INBOUND_PACKETS) {
            m_inboundQueue.push_back(std::move(packet));
            return;
        }
    }
    spdlog::warn("Inbound packet queue limit exceeded for session {}", m_sessionId);
    m_conn.close();
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
    // 按每 tick 的预算取包，避免单个玩家的积压阻塞整个主循环；业务回调在锁外执行。
    std::deque<mc::network::ir::IrPacket> local;
    decltype(m_inboundHandler) handler;
    {
        std::lock_guard<std::mutex> lock(m_inboundQueueMutex);
        const auto count = std::min(m_inboundQueue.size(), INBOUND_PACKETS_PER_TICK);
        for (usize index = 0; index < count; ++index) {
            local.push_back(std::move(m_inboundQueue.front()));
            m_inboundQueue.pop_front();
        }
        handler = m_inboundHandler;
    }
    if (!handler || local.empty()) {
        return;
    }
    for (auto& pkt : local) {
        handler(pkt);
        if (m_state == HandshakeState::Disconnected) {
            break;
        }
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

    // 原因包与阶段切换共用有序队列，使用执行时的出站阶段编码。
    auto result = m_conn.sendDisconnect(reason);
    if (!result.success()) {
        spdlog::warn("Cannot enqueue disconnect for session {}: {}", m_sessionId, result.error().toString());
    }

    closeAfterFlush();
}

} // namespace mc::server::net
