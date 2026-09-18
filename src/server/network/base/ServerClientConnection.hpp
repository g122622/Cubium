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
#include "common/network/buffer/RegistryByteBuf.hpp"
#include "common/network/crypto/Crypt.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/pipeline/Connection.hpp"
#include "common/network/pipeline/ProtocolTableSet.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/transport/LocalTransport.hpp"
#include "server/network/base/IServerClientConnection.hpp"

#include <array>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

// 前置声明而非 include：TcpTransport.hpp 是全项目唯一拖入 <asio.hpp> 的传输头，
// 而本头只需在 Wire 构造签名里出现 unique_ptr<TcpTransport>（声明处无需完整类型）。
// 真正的构造调用点在 ServerClientConnection.cpp 与 session/ServerNetwork.cpp，二者
// 自行 include TcpTransport.hpp。此举让 LoginFlow.hpp / ServerHandshake.hpp /
// ServerPlayer.hpp 等仅需连接类型的头不再被迫拉入 asio。
namespace mc::network::transport {
class TcpTransport;
}

namespace mc::server::net {

/// Java 后端的连接实例类型（buffer 类型已固化为 RegistryByteBuf）
using ClientConn = mc::network::pipeline::Connection<mc::network::buffer::RegistryByteBuf>;
using ProtocolTables = mc::network::pipeline::ProtocolTableSet<mc::network::buffer::RegistryByteBuf>;

/// 单连接握手编排状态：与 Connection 的 phase 大体一致，但显式区分 Configuration 子进度
enum class HandshakeState : u8 {
    Handshaking,   ///< 等待 ClientIntention
    Login,         ///< 等待 Hello / 已发 LoginCompression+LoginFinished 等待 LoginAcknowledged
    Configuration, ///< 推送 RegistryData/UpdateTags/FinishConfiguration
    Play,          ///< 握手完成，交给 ServerPlayHandler
    Disconnected
};

/**
 * @brief 服务端单客户端连接：包裹 pipeline::Connection + 握手状态 + sessionId
 *
 * Local 模式（集成服）与 Wire 模式（独立服 TCP）共用同一套握手/Play 流程，
 * 仅构造时注入的 transport 不同。出站统一走 send(ir::IrPacket)；入站由
 * Connection::onPacket 单一监听器收，先路由握手/Configuration 包给
 * ServerHandshakeStateMachine，Play 包交给 ServerPlayHandler。
 *
 * 不持有玩家数据——玩家在握手完成（onPlayerReady）时由 IntegratedServer/StandaloneServer
 * 创建并回填 playerId；Play 处理器据此分发。
 */
class ServerClientConnection : public IServerClientConnection {
public:
    /// Local 模式构造（集成服）：注入一对 LocalTransport 的服务端侧
    ServerClientConnection(std::unique_ptr<mc::network::transport::ILocalTransport> localTransport,
        std::shared_ptr<ProtocolTables> tables,
        u32 sessionId);

    /// Wire 模式构造（独立服）：注入已 accept 的 TcpTransport
    ServerClientConnection(std::unique_ptr<mc::network::transport::TcpTransport> wireTransport,
        std::shared_ptr<ProtocolTables> tables,
        u32 sessionId);

    ~ServerClientConnection() override;

    ServerClientConnection(const ServerClientConnection&) = delete;
    ServerClientConnection& operator=(const ServerClientConnection&) = delete;

    /// 出站发送（线程安全：Local 走队列 mutex，Wire 走 socket send mutex）
    [[nodiscard]] Result<void> send(mc::network::ir::IrPacket packet) override;

    /// 注册入站监听器（收到任意阶段包都回调，由调用方分流握手/Play）
    ///
    /// Local 模式：监听器由 pumpLocal() 在主线程直接触发，跑游戏逻辑即可。
    /// Wire 模式：transport 接收线程触发该监听器；若直接跑游戏逻辑会跨线程进入
    /// 非线程安全的 MinecraftServer 世界状态。Wire 连接应改为注册一个仅入队的
    /// 监听器（enqueueInbound），并经 setInboundHandler 设置游戏逻辑，由
    /// ServerNetwork::tick() 在主线程 drainInbound 派发。详见下三方法。
    void onPacket(ClientConn::PacketListener listener);

    /// 设置 Wire 模式入站游戏逻辑处理器（握手/Play 分流），由 onClientConnect 回调装配。
    /// 仅 drainInbound() 在主线程调用；Wire 连接的 onPacket 监听器只 enqueueInbound。
    void setInboundHandler(std::function<void(const mc::network::ir::IrPacket&)> handler)
    {
        std::lock_guard<std::mutex> lock(m_inboundQueueMutex);
        m_inboundHandler = std::move(handler);
    }

    /// Wire 模式入站：仅由 transport 接收线程调用，把解码后的 IR 包入队，不跑游戏逻辑。
    void enqueueInbound(mc::network::ir::IrPacket packet)
    {
        std::lock_guard<std::mutex> lock(m_inboundQueueMutex);
        m_inboundQueue.push_back(std::move(packet));
    }

    /// Wire 模式入站出队：仅由 ServerNetwork::tick() 在主线程调用。
    /// 锁内快照整个队列后清空，锁外逐个调 m_inboundHandler，避免 handler 重入死锁
    /// （镜像 tick() 现有 Local 快照-后-pump 模式）。Local 连接不调用本方法。
    void drainInbound();

    /// Local 模式 tick 驱动：pump 对端投递的包（Wire 模式由接收线程异步驱动，无需调用）
    void pumpLocal();

    void close() override;

    /// IServerClientConnection::disconnect：发当前阶段 Clientbound Disconnect 包后 close()。
    /// reason 为纯文本，内部包成 NBT 文本组件。已断开时为幂等 no-op。
    void disconnect(const std::string& reason) override;

    [[nodiscard]] HandshakeState state() const noexcept { return m_state; }
    void setState(HandshakeState s) noexcept { m_state = s; }

    [[nodiscard]] u32 sessionId() const noexcept { return m_sessionId; }
    [[nodiscard]] bool isLocalMode() const noexcept { return m_conn.isLocalMode(); }
    [[nodiscard]] bool isConnected() const noexcept override { return m_conn.isConnected(); }
    [[nodiscard]] mc::network::protocol::ConnectionProtocol phase() const noexcept { return m_conn.phase(); }
    void setPhase(mc::network::protocol::ConnectionProtocol p) noexcept { m_conn.setPhase(p); }
    /// 入站阶段（解码表）。对齐 MC Java setupInboundProtocol。
    [[nodiscard]] mc::network::protocol::ConnectionProtocol inboundPhase() const noexcept
    {
        return m_conn.inboundPhase();
    }
    void setInboundPhase(mc::network::protocol::ConnectionProtocol p) noexcept { m_conn.setInboundPhase(p); }
    /// 出站阶段（编码表）。对齐 MC Java setupOutboundProtocol。监听器在发新阶段包前显式切换。
    [[nodiscard]] mc::network::protocol::ConnectionProtocol outboundPhase() const noexcept
    {
        return m_conn.outboundPhase();
    }
    void setOutboundPhase(mc::network::protocol::ConnectionProtocol p) noexcept { m_conn.setOutboundPhase(p); }

    /// 装入压缩层（收到 LoginCompression 阈值后）
    void setupCompression(i32 threshold) { m_conn.setupCompression(threshold); }

    /// 装入加密层（在线模式 RSA 握手后）
    [[nodiscard]] Result<void> setupEncryption(const std::array<u8, mc::network::crypto::kSharedSecretBytes>& secret)
    {
        return m_conn.setupEncryption(secret);
    }

    [[nodiscard]] ClientConn& raw() noexcept { return m_conn; }
    [[nodiscard]] const ClientConn& raw() const noexcept { return m_conn; }

    /// IServerClientConnection::peerAddress：返回构造时快照的对端地址（Wire 模式
    /// "host:port"，Local 模式空串）。socket close 后仍可用（地址在构造时缓存）。
    [[nodiscard]] std::string peerAddress() const override { return m_peerAddress; }

private:
    // 注意声明顺序：m_peerAddress 须先于 m_conn，使初始化列表中先取 wireTransport
    // 对端地址再 move wireTransport 进 m_conn（move 后 wireTransport 为空）。
    std::string m_peerAddress;
    ClientConn m_conn;
    u32 m_sessionId;
    HandshakeState m_state = HandshakeState::Handshaking;

    // Wire 模式入站队列：transport 接收线程 enqueueInbound，主线程 drainInbound 派发。
    // Local 模式不使用（pumpLocal 主线程直驱）。消除跨线程游戏逻辑隐患。
    std::deque<mc::network::ir::IrPacket> m_inboundQueue;
    std::mutex m_inboundQueueMutex;
    std::function<void(const mc::network::ir::IrPacket&)> m_inboundHandler;
};

} // namespace mc::server::net
