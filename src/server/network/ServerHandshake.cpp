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

#include "server/network/ServerHandshake.hpp"

#include "common/util/UuidUtils.hpp"
#include "server/network/RegistryDataBuilder.hpp"

#include <spdlog/spdlog.h>

#include <variant>

namespace mc::server::net {

ServerHandshakeStateMachine::ServerHandshakeStateMachine(
    ServerClientConnection& conn, bool isOfflineMode, i32 compressionThreshold)
    : m_conn(conn)
    , m_isOfflineMode(isOfflineMode)
    , m_compressionThreshold(compressionThreshold)
{}

Result<void> ServerHandshakeStateMachine::_send(mc::network::ir::IrPacket packet)
{
    return m_conn.send(std::move(packet));
}

Result<bool> ServerHandshakeStateMachine::handleInbound(const mc::network::ir::IrPacket& packet)
{
    using CP = mc::network::protocol::ConnectionProtocol;

    // packet.packet 是 variant-of-variants（5 个阶段变体）。先按 packet.phase 取出对应
    // 阶段变体，再在内层变体上取具体包。
    if (packet.phase == CP::Handshaking) {
        const auto* hs = std::get_if<mc::network::ir::HandshakePacket>(&packet.packet);
        if (hs == nullptr) {
            return Error(ErrorCode::InvalidData, "握手阶段变体缺失", "ServerHandshake::handleInbound");
        }
        const auto* intention = std::get_if<mc::network::ir::handshake::ClientIntention>(hs);
        if (intention == nullptr) {
            return Error(ErrorCode::InvalidData, "握手阶段非 ClientIntention 包", "ServerHandshake::handleInbound");
        }
        auto r = _handleHandshake(*intention);
        if (!r.success()) {
            return r.error();
        }
        return true;
    }

    if (packet.phase == CP::Login) {
        const auto* login = std::get_if<mc::network::ir::LoginPacket>(&packet.packet);
        if (login == nullptr) {
            return Error(ErrorCode::InvalidData, "登录阶段变体缺失", "ServerHandshake::handleInbound");
        }
        auto r = _handleLoginPacket(*login);
        if (!r.success()) {
            return r.error();
        }
        return true;
    }

    if (packet.phase == CP::Configuration) {
        const auto* cfg = std::get_if<mc::network::ir::ConfigurationPacket>(&packet.packet);
        if (cfg == nullptr) {
            return Error(ErrorCode::InvalidData, "配置阶段变体缺失", "ServerHandshake::handleInbound");
        }
        // Configuration 包一律由握手状态机消费（SelectKnownPacks/FinishConfiguration/RegistryData 等），
        // 不交 Play 路由器。FinishConfiguration(C→S) 处理后 m_playReady=true 并触发 onPlayerReady，
        // 此后若再收到迟到的 Configuration 包（重发/竞态），_handleConfigurationPacket 内部对已就绪
        // 状态幂等处理，仍返回 true 吞掉，绝不转发 Play 路由器。
        auto r = _handleConfigurationPacket(*cfg);
        if (!r.success()) {
            return r.error();
        }
        return true; // Configuration 包始终已消费
    }

    // Play 阶段包：交给 ServerPlayRouter
    return false;
}

Result<void> ServerHandshakeStateMachine::_handleHandshake(const mc::network::ir::handshake::ClientIntention& intention)
{
    if (intention.intendedState != 2) {
        // 仅支持 Login（intent=2）；Status(1) 留 TODO
        // TODO(Phase6): Status 查询握手未实现
        spdlog::warn("ServerHandshake: unsupported intendedState={}", intention.intendedState);
        return Error(ErrorCode::InvalidArgument, "仅支持 Login 意图", "ServerHandshake::_handleHandshake");
    }
    // Connection 收到 ClientIntention(terminal) 后自动切 Login 阶段
    m_conn.setState(HandshakeState::Login);
    spdlog::info("ServerHandshake: client intention protocol={} host={}:{}",
        intention.protocolVersion,
        intention.hostName,
        intention.port);
    return Result<void>::ok();
}

Result<void> ServerHandshakeStateMachine::_handleLoginPacket(const mc::network::ir::LoginPacket& pkt)
{
    // std::visit 处理 LoginPacket 变体
    if (std::holds_alternative<mc::network::ir::login::Hello>(pkt)) {
        const auto& hello = std::get<mc::network::ir::login::Hello>(pkt);
        m_username = hello.username;
        // 离线 UUID 由用户名生成
        const Uuid offline = util::generateOfflineUuid(m_username);
        m_offlineUuid = offline;

        if (m_isOfflineMode) {
            return _advanceAfterHelloOffline();
        }
        return _advanceAfterHelloOnline();
    }

    if (std::holds_alternative<mc::network::ir::login::Key>(pkt)) {
        return _handleKey(std::get<mc::network::ir::login::Key>(pkt));
    }

    if (std::holds_alternative<mc::network::ir::login::LoginAcknowledged>(pkt)) {
        // 客户端确认登录完成，Connection 自动切 Configuration；开始推送配置
        m_conn.setState(HandshakeState::Configuration);
        return _beginConfiguration();
    }

    // LoginCompression/LoginFinished 是 S→C（服务端发出），不应收到
    // Disconnect 由上层处理；其余忽略
    return Result<void>::ok();
}

Result<void> ServerHandshakeStateMachine::_advanceAfterHelloOffline()
{
    // 离线模式：无 RSA。发 LoginCompression（threshold=-1 集成服禁用，独立服真阈值）→ 发 LoginFinished
    if (m_compressionThreshold >= 0) {
        auto r = _send(mc::network::backend::java::JavaLoginHandshaker::buildLoginCompression(m_compressionThreshold));
        if (!r.success()) {
            return r;
        }
        m_conn.setupCompression(m_compressionThreshold);
    }
    // threshold<0 时不发 LoginCompression（Java 协议：未发即禁用压缩）
    auto lf = _send(mc::network::backend::java::JavaLoginHandshaker::buildLoginFinished(m_offlineUuid, m_username));
    if (!lf.success()) {
        return lf;
    }
    m_loginFinishedSent = true;
    return Result<void>::ok();
}

Result<void> ServerHandshakeStateMachine::_advanceAfterHelloOnline()
{
    // 在线模式：发 HelloBound（含 RSA 公钥 + verify token），等客户端 Key
    auto start = mc::network::backend::java::JavaLoginHandshaker::buildHelloBound(true);
    if (!start.success()) {
        return start.error();
    }
    m_serverPrivateKeyDer = start.value().serverPrivateKeyDer;
    m_verifyToken = start.value().verifyToken;
    return _send(std::move(start.value().helloBound));
}

Result<void> ServerHandshakeStateMachine::_handleKey(const mc::network::ir::login::Key& key)
{
    auto secret = mc::network::backend::java::JavaLoginHandshaker::handleKey(key, m_serverPrivateKeyDer, m_verifyToken);
    if (!secret.success()) {
        return secret.error();
    }
    auto enc = m_conn.setupEncryption(secret.value());
    if (!enc.success()) {
        return enc;
    }
    // 加密层装好后，发 LoginCompression + LoginFinished
    if (m_compressionThreshold >= 0) {
        auto r = _send(mc::network::backend::java::JavaLoginHandshaker::buildLoginCompression(m_compressionThreshold));
        if (!r.success()) {
            return r;
        }
        m_conn.setupCompression(m_compressionThreshold);
    }
    auto lf = _send(mc::network::backend::java::JavaLoginHandshaker::buildLoginFinished(m_offlineUuid, m_username));
    if (!lf.success()) {
        return lf;
    }
    m_loginFinishedSent = true;
    return Result<void>::ok();
}

Result<void> ServerHandshakeStateMachine::_beginConfiguration()
{
    if (m_configurationStarted) {
        return Result<void>::ok();
    }
    m_configurationStarted = true;

    // 发 SelectKnownPacks(S→C)，告知客户端服务端已知的原版数据包；等客户端回 SelectKnownPacks(C→S)
    mc::network::ir::configuration::SelectKnownPacks skp;
    skp.knownPacks = buildServerKnownPacks();
    return _send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Configuration,
        mc::network::ir::ConfigurationPacket{std::move(skp)}});
}

Result<void> ServerHandshakeStateMachine::_pushConfigurationData()
{
    // 客户端已回 SelectKnownPacks(C→S) 命中 minecraft:core。
    // 依次推送 RegistryData×N → UpdateTags → UpdateEnabledFeatures → FinishConfiguration
    auto registries = buildConfigurationRegistryData();
    for (auto& reg : registries) {
        auto r = _send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Configuration,
            mc::network::ir::ConfigurationPacket{std::move(reg)}});
        if (!r.success()) {
            return r;
        }
    }

    // UpdateTags：暂发空（客户端命中 core 后标签数据可省）。TODO(Phase6): 真实标签同步
    mc::network::ir::configuration::UpdateTags tags;
    auto rt = _send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Configuration,
        mc::network::ir::ConfigurationPacket{std::move(tags)}});
    if (!rt.success()) {
        return rt;
    }

    // UpdateEnabledFeatures：原版默认启用特性集。TODO(Phase6): 真实特性集
    mc::network::ir::configuration::UpdateEnabledFeatures features;
    features.features = {std::string("minecraft:vanilla")};
    auto rf = _send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Configuration,
        mc::network::ir::ConfigurationPacket{std::move(features)}});
    if (!rf.success()) {
        return rf;
    }

    // FinishConfiguration(S→C, terminal)：Connection 自动切 Play
    mc::network::ir::configuration::FinishConfiguration finish;
    return _send(mc::network::ir::IrPacket{mc::network::protocol::ConnectionProtocol::Configuration,
        mc::network::ir::ConfigurationPacket{std::move(finish)}});
}

Result<void> ServerHandshakeStateMachine::_handleConfigurationPacket(const mc::network::ir::ConfigurationPacket& pkt)
{
    if (std::holds_alternative<mc::network::ir::configuration::SelectKnownPacks>(pkt)) {
        // 客户端回 SelectKnownPacks(C→S)：推送剩余配置数据
        return _pushConfigurationData();
    }

    if (std::holds_alternative<mc::network::ir::configuration::FinishConfiguration>(pkt)) {
        // 客户端回 FinishConfiguration(C→S)：配置完成，进入 Play。
        // 幂等守卫：迟到的重发不应二次触发 onPlayerReady（否则重复创建玩家/重发 play::Login）。
        if (m_playReady) {
            spdlog::debug("ServerHandshake: duplicate FinishConfiguration ignored (already Play ready)");
            return Result<void>::ok();
        }
        m_conn.setState(HandshakeState::Play);
        m_playReady = true;
        if (m_onReady) {
            m_onReady(m_username, m_offlineUuid);
        }
        return Result<void>::ok();
    }

    // ClientInformation / CustomPayload / Ping / KeepAlive / RegistryData(不应收到) / Disconnect
    // 暂忽略；TODO(Phase6): ClientInformation 存储、Ping 回 Pong
    return Result<void>::ok();
}

} // namespace mc::server::net
