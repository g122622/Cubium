/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, without limitation the rights
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

#include "client/network/ClientNetwork.hpp"

#include "client/network/ClientPlayVisitor.hpp"
#include "common/network/backend/java/JavaBackend.hpp"
#include "common/network/backend/java/handshake/JavaLoginHandshaker.hpp"
#include "common/network/ir/packets/login/LoginPackets.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"

#include <spdlog/spdlog.h>

#include <variant>

namespace mc::client::net {

ClientNetwork::ClientNetwork()
{
    // 构建一次 Java 协议表（与 ServerNetwork 对称，表无状态可各自持有）
    mc::network::backend::java::JavaBackend backend;
    m_tables = backend.provideProtocolTables();
}

ClientNetwork::~ClientNetwork()
{
    if (m_state != ClientConnState::Disconnected) {
        disconnect("ClientNetwork destroyed");
    }
}

bool ClientNetwork::isConnected() const noexcept
{
    return m_conn != nullptr && m_conn->isConnected();
}

Result<void> ClientNetwork::connectLocal(
    std::unique_ptr<mc::network::transport::ILocalTransport> serverSideTransport, const std::string& username)
{
    m_username = username;
    m_conn = std::make_unique<ClientConn>(
        std::move(serverSideTransport), m_tables, mc::network::protocol::PacketFlow::Serverbound);
    m_conn->onPacket([this](const mc::network::ir::IrPacket& packet) { _handleInbound(packet); });

    _setState(ClientConnState::LoggingIn);
    return _sendHandshakeHello();
}

Result<void> ClientNetwork::connectTcp(const std::string& host, u16 port, const std::string& username)
{
    m_username = username;
    auto transport = std::make_unique<mc::network::transport::TcpTransport>();
    auto r = transport->connect(mc::network::transport::Endpoint{host, port});
    if (!r.success()) {
        return r;
    }
    m_conn =
        std::make_unique<ClientConn>(std::move(transport), m_tables, mc::network::protocol::PacketFlow::Serverbound);
    m_conn->onPacket([this](const mc::network::ir::IrPacket& packet) { _handleInbound(packet); });

    _setState(ClientConnState::Connecting);
    return _sendHandshakeHello();
}

void ClientNetwork::disconnect(const std::string& reason)
{
    _setState(ClientConnState::Disconnecting);
    if (m_conn) {
        m_conn->close();
    }
    m_conn.reset();
    _setState(ClientConnState::Disconnected);
    spdlog::info("ClientNetwork disconnected: {}", reason);
}

Result<void> ClientNetwork::send(mc::network::ir::IrPacket packet)
{
    if (m_conn == nullptr) {
        return Error(ErrorCode::InvalidState, "Not connected", "ClientNetwork::send");
    }
    auto r = m_conn->send(std::move(packet));
    if (r.success()) {
        m_packetsSent.fetch_add(1);
    }
    return r;
}

void ClientNetwork::tick()
{
    if (m_conn != nullptr && m_conn->isLocalMode()) {
        m_conn->pumpLocal();
    }
}

// ============================================================================
// 入站分发
// ============================================================================

void ClientNetwork::_handleInbound(const mc::network::ir::IrPacket& packet)
{
    m_packetsReceived.fetch_add(1);
    using CP = mc::network::protocol::ConnectionProtocol;

    switch (packet.phase) {
        case CP::Handshaking:
            // 客户端不应在握手阶段收包（ClientIntention 是客户端发出）
            spdlog::warn("ClientNetwork: unexpected handshake-phase inbound");
            break;
        case CP::Status:
            // TODO(Phase6): Status 查询响应处理
            break;
        case CP::Login: {
            const auto* login = std::get_if<mc::network::ir::LoginPacket>(&packet.packet);
            if (login == nullptr) {
                spdlog::error("ClientNetwork: Login phase variant missing");
                break;
            }
            auto r = _handleLoginPacket(*login);
            if (!r.success()) {
                spdlog::error("ClientNetwork: login handler failed: {}", r.error().toString());
            }
            break;
        }
        case CP::Configuration: {
            const auto* cfg = std::get_if<mc::network::ir::ConfigurationPacket>(&packet.packet);
            if (cfg == nullptr) {
                spdlog::error("ClientNetwork: Configuration phase variant missing");
                break;
            }
            auto r = _handleConfigurationPacket(*cfg);
            if (!r.success()) {
                spdlog::error("ClientNetwork: configuration handler failed: {}", r.error().toString());
            }
            break;
        }
        case CP::Play: {
            const auto* play = std::get_if<mc::network::ir::PlayPacket>(&packet.packet);
            if (play == nullptr) {
                spdlog::error("ClientNetwork: Play phase variant missing");
                break;
            }
            auto r = _handlePlayPacket(*play);
            if (!r.success()) {
                spdlog::error("ClientNetwork: play handler failed: {}", r.error().toString());
            }
            break;
        }
    }
}

// ============================================================================
// 握手状态机
// ============================================================================

Result<void> ClientNetwork::_sendHandshakeHello()
{
    // 发 ClientIntention(LOGIN)（terminal，Connection 自动切 Login 阶段）→ 发 Hello(username)
    auto intention = send(mc::network::backend::java::JavaLoginHandshaker::buildClientIntention("", 0));
    if (!intention.success()) {
        return intention;
    }
    auto hello = send(mc::network::backend::java::JavaLoginHandshaker::buildHello(m_username));
    if (!hello.success()) {
        return hello;
    }
    return Result<void>::ok();
}

Result<void> ClientNetwork::_handleLoginPacket(const mc::network::ir::LoginPacket& pkt)
{
    if (std::holds_alternative<mc::network::ir::login::LoginCompression>(pkt)) {
        const auto& comp = std::get<mc::network::ir::login::LoginCompression>(pkt);
        // 装压缩层（threshold<0 禁用）
        if (m_conn) {
            m_conn->setupCompression(comp.threshold);
        }
        return Result<void>::ok();
    }

    if (std::holds_alternative<mc::network::ir::login::LoginFinished>(pkt)) {
        const auto& lf = std::get<mc::network::ir::login::LoginFinished>(pkt);
        m_uuid = lf.uuid;
        m_username = lf.username; // 服务端权威用户名
        m_loginFinishedReceived = true;
        // 对齐 Java handleLoginFinished：先 setupInboundProtocol(Configuration)（入站先翻 Configuration，
        // 后续 S→C Configuration 包按 Configuration 入站表解码），再 send(LoginAcknowledged)（此时出站
        // 仍 Login，按 Login Sb id=3 编码）。发完 LoginAcknowledged(terminal) 后出站自动翻 Configuration。
        if (m_conn) {
            m_conn->setInboundPhase(mc::network::protocol::ConnectionProtocol::Configuration);
        }
        return send(mc::network::backend::java::JavaLoginHandshaker::buildLoginAcknowledged());
    }

    if (std::holds_alternative<mc::network::ir::login::HelloBound>(pkt)) {
        // 在线模式：收服务端 RSA 公钥 + verify token。
        // TODO(Phase6): handleHelloBound → 发 Key → setupEncryption。离线模式不应收到。
        spdlog::warn("ClientNetwork: HelloBound received (online mode TODO), ignoring");
        return Result<void>::ok();
    }

    if (std::holds_alternative<mc::network::ir::login::Disconnect>(pkt)) {
        const auto& dc = std::get<mc::network::ir::login::Disconnect>(pkt);
        spdlog::info("ClientNetwork: login disconnect: {}", dc.reason);
        disconnect("Login disconnect");
        return Result<void>::ok();
    }

    // Hello/Key/LoginAcknowledged 是客户端发出，不应收到；忽略
    return Result<void>::ok();
}

Result<void> ClientNetwork::_handleConfigurationPacket(const mc::network::ir::ConfigurationPacket& pkt)
{
    if (std::holds_alternative<mc::network::ir::configuration::SelectKnownPacks>(pkt)) {
        // 服务端发来已知数据包列表；客户端回命中集合（minecraft:core 命中）
        mc::network::ir::configuration::SelectKnownPacks reply;
        reply.knownPacks = {mc::network::ir::configuration::KnownPack{"minecraft", "core", "1.21.11"}};
        return send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Configuration,
            mc::network::ir::ConfigurationPacket{std::move(reply)}});
    }

    if (std::holds_alternative<mc::network::ir::configuration::FinishConfiguration>(pkt)) {
        // 服务端发完所有配置数据；客户端回 FinishConfiguration（terminal，Connection 自动切 Play）
        mc::network::ir::configuration::FinishConfiguration reply;
        m_configurationFinished = true;
        return send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Configuration,
            mc::network::ir::ConfigurationPacket{std::move(reply)}});
    }

    if (std::holds_alternative<mc::network::ir::configuration::Ping>(pkt)) {
        // S→C Ping(parameter)：客户端回 C→S Pong(同 parameter)（id=5 双向复用 Ping IR struct）。
        // 对齐 Java ClientConfigurationPacketListenerImpl#handlePing → send(ServerboundPongConfigurationPacket)。
        mc::network::ir::configuration::Ping pong = std::get<mc::network::ir::configuration::Ping>(pkt);
        return send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Configuration,
            mc::network::ir::ConfigurationPacket{std::move(pong)}});
    }

    // RegistryData/UpdateTags/UpdateEnabledFeatures：客户端 SelectKnownPacks 命中 minecraft:core，
    // 服务端以 RegistryEntry{data=nullopt}（声明"客户端已知"）发送，客户端依赖本地硬编码 vanilla
    // registry 即可，无需消费 NBT。真 Java 互通时我方服务端同样发 data=nullopt，真客户端用其本地
    // registry——故 NBT 消费路径在 core 命中前提下永不触发，刻意保留为占位（若未来支持非 core
    // 数据包协商再补 NBT 解析）。KeepAlive/CustomPayload/Disconnect：Step3 视需填充。
    if (m_visitor != nullptr) {
        // 委托 visitor 处理余下 Configuration 包（占位）
        mc::network::ir::IrPacket wrapper{
            mc::network::protocol::ConnectionProtocol::Configuration, mc::network::ir::ConfigurationPacket{pkt}};
        (void)m_visitor->handleConfiguration(wrapper);
    }
    return Result<void>::ok();
}

Result<void> ClientNetwork::_handlePlayPacket(const mc::network::ir::PlayPacket& pkt)
{
    // play::Login（post-config S→C）：携带本地玩家 id + 维度信息。
    // ClientNetwork 仅负责存 playerId + 切 Playing 状态 + 触发 onLoginReady 通知；
    // 本地玩家实体生成由 visitor 的 Login 分支完成（Step3 决策：visitor 直接调游戏方法）。
    // 故 Login 不在此 return，继续向下委托 visitor。
    if (std::holds_alternative<mc::network::ir::play::Login>(pkt)) {
        const auto& login = std::get<mc::network::ir::play::Login>(pkt);
        m_playerId = login.playerId;
        _setState(ClientConnState::Playing);
        if (m_onLoginReady) {
            m_onLoginReady(login.playerId, login.spawnInfo.dimension, m_uuid);
        }
    }

    // 所有 Play 包（含 Login）委托 visitor（Step3 填充 ~73 分支）
    if (m_visitor != nullptr) {
        mc::network::ir::IrPacket wrapper{
            mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{pkt}};
        return m_visitor->handle(wrapper);
    }
    // 无 visitor 时静默丢弃（Step2 骨架期）
    return Result<void>::ok();
}

} // namespace mc::client::net
