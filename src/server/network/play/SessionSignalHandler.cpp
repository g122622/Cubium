/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without including limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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

#include "server/network/play/SessionSignalHandler.hpp"

#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/util/TimeUtils.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/core/KeepAliveManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include <variant>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::server::net {

void SessionSignalHandler::handlePingRequestPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // 客户端主动 ping(ping 协议通道)，回 PongResponse(cb:60) 同 time。
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::ServerboundPingRequest>(&play);
    if (evt == nullptr) {
        return;
    }
    mc::network::ir::play::PongResponse resp;
    resp.time = evt->time;
    m_server.sendPacketToPlayer(playerId,
        mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(resp)}});
}

void SessionSignalHandler::handlePongPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // common 通道 cb:59 ping 的回声。对齐 Java ServerCommonPacketListenerImpl.handlePong：
    // 服务端不据此计算 RTT（vanilla 为空实现），仅确认回声链路可达。
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::ServerboundPong>(&play);
    if (evt == nullptr) {
        return;
    }
    spdlog::info("[Server] Pong from player {} id={}", playerId, evt->id);
}

void SessionSignalHandler::handleKeepAlivePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::KeepAlive>(&play);
    if (evt == nullptr) {
        return;
    }

    u64 currentTimeMs = util::TimeUtils::getCurrentTimeMs();
    m_server.keepAliveManager().handleKeepAliveResponse(playerId, static_cast<u64>(evt->id), currentTimeMs);
}

void SessionSignalHandler::handleConfigurationAcknowledgedPacket(
    PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // ConfigurationAcknowledged（C→S terminal）：客户端收到服务端 StartConfiguration(S→C) 后回此包，
    // 双方切回 Configuration 阶段。入站阶段由框架 ProtocolSwapHandler 自动切回 Configuration
    // （见 ProtocolSwapHandler.cpp Play 分支识别此 terminal），无需此处手动 setInboundPhase。
    //
    // TODO(Play→Configuration reconfiguration): 完整 reconfiguration 需服务端在此显式
    // setOutboundPhase(Configuration) 后重推 RegistryData/UpdateTags 等；但当前项目无
    // StartConfiguration(S→C) IR 结构体，服务端无发起 reconfiguration 的路径，此包运行时不被
    // 客户端触发。本处理仅确认接收，保证 terminal 自动阶段切换链路不被 route 兜底干扰。
    (void)packet;
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (player == nullptr || !player->loggedIn) {
        return;
    }
    spdlog::info("ConfigurationAcknowledged from player {}, switching back to Configuration phase", playerId);
}

void SessionSignalHandler::handleChunkBatchReceivedPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    // ChunkBatchReceived（C→S id=10）：客户端在区块批次结束后回发本批次实际接收速率
    // chunksPerTick(Float)，服务端据此动态调整下一批次区块发送配额。
    //
    // TODO(服务端区块批次流速控): 当前 ChunkSendManager::processPendingSends 每 tick 抽干
    // m_readyChunks 就绪队列，无批次划分与发送配额，亦不发 ChunkBatchStart/Finished(cb:11/12)。
    // 完整实现需在 ChunkSendManager 引入批次窗口（按 tick 攒批 + Start/Finished 配对下发），
    // 并以本包 chunksPerTick 反馈调整每批次发送上限。此处仅确认接收并记 warn。
    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::ChunkBatchReceived>(&play);
    if (evt == nullptr) {
        return;
    }
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (player == nullptr || !player->loggedIn) {
        return;
    }
    spdlog::warn("ChunkBatchReceived from player {} chunksPerTick={} not implemented (batch flow control pending)",
        playerId,
        evt->chunksPerTick);
}

} // namespace mc::server::net
