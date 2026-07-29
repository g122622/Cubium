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

#include "common/network/backend/java/JavaBackend.hpp"
#include "common/network/ir/packets/login/LoginPackets.hpp"
#include "common/network/ir/packets/status/StatusPackets.hpp"
#include "common/util/UuidUtils.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TranslationTextComponent.hpp"
#include "server/network/RegistryDataBuilder.hpp"

#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>

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
            return Error(ErrorCode::InvalidData, "Handshake phase variant missing", "ServerHandshake::handleInbound");
        }
        const auto* intention = std::get_if<mc::network::ir::handshake::ClientIntention>(hs);
        if (intention == nullptr) {
            return Error(ErrorCode::InvalidData,
                "Handshake phase is not a ClientIntention packet",
                "ServerHandshake::handleInbound");
        }
        auto r = _handleHandshake(*intention);
        if (!r.success()) {
            return r.error();
        }
        return true;
    }

    if (packet.phase == CP::Status) {
        // ClientIntention(intent=1) 是 terminal，Connection 在 handleInbound 返回后自动切到
        // Status 阶段。后续 StatusRequest/PingRequest 以 phase==Status 到达，由此分支消费，
        // 绝不漏到 ServerPlayRouter（否则触发 "dropping non-Play packet (phase=1)" 告警）。
        const auto* st = std::get_if<mc::network::ir::StatusPacket>(&packet.packet);
        if (st == nullptr) {
            return Error(ErrorCode::InvalidData, "Status phase variant missing", "ServerHandshake::handleInbound");
        }
        auto r = _handleStatusPacket(*st);
        if (!r.success()) {
            return r.error();
        }
        return true; // Status 包始终已消费
    }

    if (packet.phase == CP::Login) {
        const auto* login = std::get_if<mc::network::ir::LoginPacket>(&packet.packet);
        if (login == nullptr) {
            return Error(ErrorCode::InvalidData, "Login phase variant missing", "ServerHandshake::handleInbound");
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
            return Error(
                ErrorCode::InvalidData, "Configuration phase variant missing", "ServerHandshake::handleInbound");
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
    using mc::network::backend::java::kJavaProtocolVersion;

    if (intention.intendedState == 1) {
        // STATUS：对齐 Java handleIntention 的 STATUS 分支——显式 setupOutboundProtocol(Status)。
        // 入站阶段由框架收 ClientIntention(terminal) 自动翻 Status（ProtocolSwapHandler
        // intendedState==1→Status）。随后的 StatusRequest/PingRequest 由 handleInbound 的
        // Status 分支处理，StatusResponse 按 Status 出站表编码。
        m_conn.setOutboundPhase(mc::network::protocol::ConnectionProtocol::Status);
        spdlog::info("ServerHandshake: status ping protocol={} host={}:{}",
            intention.protocolVersion,
            intention.hostName,
            intention.port);
        return Result<void>::ok();
    }

    if (intention.intendedState == 2 || intention.intendedState == 3) {
        // LOGIN(2) 或 TRANSFER(3)：项目未单独实现 transfer，ProtocolSwapHandler 亦将 3→Login，
        // 故此处统一按 Login 处理。对齐 Java beginLogin：先 setupOutboundProtocol(Login)。
        m_conn.setOutboundPhase(mc::network::protocol::ConnectionProtocol::Login);
        if (intention.protocolVersion != kJavaProtocolVersion) {
            // 协议版本不匹配：对齐 Java ServerHandshakePacketListenerImpl#beginLogin——
            // 先 setupOutboundProtocol(Login)（上面已设），再发 Login Disconnect（可翻译文本组件）
            // 后断连。入站阶段由框架收 ClientIntention(terminal) 自动翻 Login。
            m_conn.setState(HandshakeState::Login);
            const bool outdated = intention.protocolVersion < 754;
            const char* key =
                outdated ? "multiplayer.disconnect.outdated_client" : "multiplayer.disconnect.incompatible";
            mc::text::TranslationTextComponent comp(key);
            comp.addParam(std::make_unique<mc::text::StringTextComponent>("1.21.11"));
            mc::network::ir::login::Disconnect dc;
            dc.reason = comp.toJson().dump();
            (void)m_conn.send(mc::network::ir::IrPacket{
                mc::network::protocol::ConnectionProtocol::Login, mc::network::ir::LoginPacket{std::move(dc)}});
            spdlog::info("ServerHandshake: rejected protocol {} (expected {}): {}",
                intention.protocolVersion,
                kJavaProtocolVersion,
                key);
            m_conn.close();
            return Result<void>::ok();
        }
        // 入站阶段由框架收 ClientIntention(terminal) 自动切 Login 阶段
        m_conn.setState(HandshakeState::Login);
        spdlog::info("ServerHandshake: client intention protocol={} host={}:{}",
            intention.protocolVersion,
            intention.hostName,
            intention.port);
        return Result<void>::ok();
    }

    spdlog::warn("ServerHandshake: unsupported intendedState={}", intention.intendedState);
    return Error(ErrorCode::InvalidArgument, "Unsupported intendedState", "ServerHandshake::_handleHandshake");
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
        // 客户端确认登录完成。对齐 Java handleLoginAcknowledgement：先 setupOutboundProtocol
        // (Configuration)，使后续 SelectKnownPacks 等按 Configuration 出站表编码。入站阶段由
        // 框架收 LoginAcknowledged(terminal) 自动切 Configuration。
        m_conn.setOutboundPhase(mc::network::protocol::ConnectionProtocol::Configuration);
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

    // UpdateTags：推送 buildConfigurationUpdateTags()。网络同步的 registry 完全替换客户端
    // 本地 registry，本地 core 包 tags/ 目录不被读取，故 timeline 的 in_overworld/
    // in_nether/in_end tag 必须由此显式 bindTag，否则 freeze() 报 "Unbound tags"。
    mc::network::ir::configuration::UpdateTags tags;
    tags.registries = buildConfigurationUpdateTags();
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
        // 对齐 Java handleConfigurationFinished：先 setupOutboundProtocol(Play)，使 onPlayerReady
        // 回调里发的 play::Login 按 Play 出站表编码。入站阶段由框架收 FinishConfiguration(terminal)
        // 自动切 Play。
        // 幂等守卫：迟到的重发不应二次触发 onPlayerReady（否则重复创建玩家/重发 play::Login）。
        if (m_playReady) {
            spdlog::debug("ServerHandshake: duplicate FinishConfiguration ignored (already Play ready)");
            return Result<void>::ok();
        }
        m_conn.setOutboundPhase(mc::network::protocol::ConnectionProtocol::Play);
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

Result<void> ServerHandshakeStateMachine::_handleStatusPacket(const mc::network::ir::StatusPacket& pkt)
{
    // ping 后已 close，忽略此后到达的迟到包
    if (!m_conn.isConnected()) {
        return Result<void>::ok();
    }

    if (std::holds_alternative<mc::network::ir::status::StatusRequest>(pkt)) {
        // 对齐 Java ServerStatusPacketListenerImpl#handleStatusRequest：单次守卫，二次请求断连。
        if (m_hasRequestedStatus) {
            m_conn.close();
            return Result<void>::ok();
        }
        m_hasRequestedStatus = true;
        // 无状态提供者：对齐 Java "disconnect.ignoring_status_request"（静默断连，无包）。
        if (!m_statusProvider) {
            m_conn.close();
            return Result<void>::ok();
        }
        const StatusInfo info = m_statusProvider();
        mc::network::ir::status::StatusResponse resp;
        resp.json = _buildStatusJson(info);
        auto r = m_conn.send(mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Status, mc::network::ir::StatusPacket{std::move(resp)}});
        if (!r.success()) {
            return r;
        }
        // 注意：响应后不断连——留通道等客户端 PingRequest（对齐 Java）。
        return Result<void>::ok();
    }

    if (std::holds_alternative<mc::network::ir::status::PingRequest>(pkt)) {
        // 对齐 Java handlePingRequest：回 Pong（原样回显 payload）后断连（无 Disconnect 包）。
        const auto& ping = std::get<mc::network::ir::status::PingRequest>(pkt);
        mc::network::ir::status::PingResponse pong;
        pong.payload = ping.payload;
        (void)m_conn.send(mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Status, mc::network::ir::StatusPacket{std::move(pong)}});
        m_conn.close();
        return Result<void>::ok();
    }

    // StatusResponse/PingResponse 是 S→C，不应收到；忽略。
    return Result<void>::ok();
}

std::string ServerHandshakeStateMachine::_buildStatusJson(const StatusInfo& info)
{
    // 对齐 MC Java 1.21.11 ServerStatus Codec（经 JsonOps 序列化）。字段"省略而非 null"：
    // favicon/enforcesSecureChat 在离线默认下省略；1.21.11 已无 previewsChat。
    // nlohmann::json::dump() 默认不转义 HTML，等价 Java Gson disableHtmlEscaping()。
    nlohmann::json j;
    j["description"] = nlohmann::json::object({{"text", info.motd}});
    j["players"] = nlohmann::json::object({
        {"max", info.maxPlayers},
        {"online", info.onlinePlayers},
        {"sample", nlohmann::json::array()}, // TODO: 真实玩家样本（name + offline uuid，上限 12）
    });
    j["version"] = nlohmann::json::object({
        {"name", info.versionName},
        {"protocol", info.protocolVersion},
    });
    // TODO: favicon（server-icon.png 加载为
    // data:image/png;base64,...）；enforcesSecureChat（在线模式+enforce-secure-profile）
    return j.dump();
}

} // namespace mc::server::net
