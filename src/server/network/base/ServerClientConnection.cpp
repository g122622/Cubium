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

#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

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

    // reason 必须是纯文本：codec 会把它编码为 NBT StringTag（vanilla
    // Component.literal(text) 的纯文本折叠路径）。若误传 JSON 字符串走 writeString，
    // 客户端按 NBT 解码时会把首字节当 tag id，报 "Invalid tag id"。
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

} // namespace mc::server::net
