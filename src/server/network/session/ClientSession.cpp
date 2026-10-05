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

#include "server/network/session/ClientSession.hpp"

#include "common/core/Result.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "server/network/handshake/ServerHandshake.hpp"
#include "server/network/play/ServerPlayHandler.hpp"

#include <variant>
#include <spdlog/spdlog.h>

namespace mc::server::net {

Result<void> ClientSession::handleInbound(const mc::network::ir::IrPacket& packet)
{
    // 重配置确认拦截：Play 阶段的 ConfigurationAcknowledged 是本会话对重配置请求的响应，
    // 不应走通用 Play 路由（SessionSignalHandler 只做确认）。此处就地处理并发起新一轮配置。
    if (packet.phase == mc::network::protocol::ConnectionProtocol::Play) {
        if (const auto* play = std::get_if<mc::network::ir::PlayPacket>(&packet.packet);
            play != nullptr && std::holds_alternative<mc::network::ir::play::ConfigurationAcknowledged>(*play)) {
            handleConfigurationAcknowledged();
            return Result<void>::ok();
        }
    }

    // 握手状态机优先：Handshake/Status/Login/Configuration 包在此消费。
    auto handled = m_handshake.handleInbound(packet);
    if (!handled.success()) {
        return handled.error();
    }
    if (handled.value()) {
        return Result<void>::ok();
    }

    // 守卫①：phase 必须为 Play。握手状态机应已消费所有非 Play 包，正常不会走到这里；
    // 若因竞态或迟到包漏至此处，降级丢弃并告警，不用断言阻塞端到端。
    if (packet.phase != mc::network::protocol::ConnectionProtocol::Play) {
        spdlog::warn(
            "ClientSession: dropping non-Play packet (phase={}) reached Play handler", static_cast<i32>(packet.phase));
        return Result<void>::ok();
    }

    // 守卫②：playerId 已回填。握手未完成时 playerId 仍为占位 0（onPlayerReady 才回填），
    // 此时到达的 Play 包无处可派发，丢弃并告警。
    if (m_playerId == 0) {
        spdlog::warn("ClientSession: play packet before playerId bound, dropping");
        return Result<void>::ok();
    }

    m_playHandler.route(m_playerId, packet);
    return Result<void>::ok();
}

bool ClientSession::handleConfigurationAcknowledged()
{
    // 对齐 vanilla ServerGamePacketListenerImpl#handleConfigurationAcknowledged：仅当此前
    // 已发起重配置（waitingForSwitchToConfig）时有效，否则忽略（vanilla 抛异常，此处降级告警）。
    if (!m_waitingForConfig) {
        spdlog::warn(
            "ClientSession: ConfigurationAcknowledged but no switch was requested (sessionId={})", m_sessionId);
        return false;
    }
    m_waitingForConfig = false;

    // 重置握手状态机并发起新一轮配置阶段（重发 SelectKnownPacks → 重推 RegistryData 等）。
    auto result = m_handshake.restartConfiguration();
    if (!result.success()) {
        spdlog::error("ClientSession: failed to restart configuration: {}", result.error().toString());
        return false;
    }
    spdlog::info("ClientSession: restarted configuration phase for player {} (sessionId={})", m_playerId, m_sessionId);
    return true;
}

Result<void> ClientSession::startConfiguration()
{
    if (m_connection == nullptr || !m_connection->isConnected()) {
        return Error(ErrorCode::InvalidState, "Connection unavailable", "ClientSession::startConfiguration");
    }

    // 对齐 vanilla ServerGamePacketListenerImpl#switchToConfig：
    //   waitingForSwitchToConfig = true; send(ClientboundStartConfigurationPacket.INSTANCE);
    //   connection.setupOutboundProtocol(Configuration)
    // 先切出站阶段（使 StartConfiguration 之后发的 Configuration 包按 Configuration 表编码），
    // 再发 StartConfiguration（terminal，发送侧自动切出站阶段，此处显式设置保证顺序确定）。
    m_waitingForConfig = true;
    m_connection->setOutboundPhase(mc::network::protocol::ConnectionProtocol::Configuration);

    mc::network::ir::play::StartConfiguration start;
    auto result = m_connection->send(mc::network::ir::IrPacket{
        mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(start)}});
    if (!result.success()) {
        m_waitingForConfig = false;
        return result;
    }
    spdlog::info("ClientSession: sent StartConfiguration to player {} (sessionId={})", m_playerId, m_sessionId);
    return Result<void>::ok();
}

} // namespace mc::server::net
