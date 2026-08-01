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

#include "server/network/ServerPlayRouter.hpp"

#include "common/network/protocol/ConnectionProtocol.hpp"
#include "server/network/ServerPlayHandler.hpp"

#include <spdlog/spdlog.h>

#include <variant>

namespace mc::server::net {

Result<void> ServerPlayRouter::handle(const mc::network::ir::IrPacket& packet)
{
    if (packet.phase != mc::network::protocol::ConnectionProtocol::Play) {
        // 防御：握手状态机 handleInbound 应已消费所有非 Play 包（Handshake/Login/Configuration），
        // 正常不会到达此处。若因竞态/迟到包漏至此，降级丢弃并记日志，避免 MC_ASSERT_RELEASE 崩溃
        // 阻塞端到端（早期 FinishConfiguration 误转发 bug 已在 ServerHandshake::handleInbound 修复）。
        spdlog::warn("ServerPlayRouter: dropping non-Play packet (phase={}) reached Play router",
            static_cast<int>(packet.phase));
        return Result<void>::ok();
    }

    // 统一交 ServerPlayHandler::route 按 ir::PlayPacket 变体分发。
    // sessionId 仅用于远程玩家路由；Local 模式 router 持有绑定后的 playerId。
    (void)m_sessionId;
    if (m_playerId == 0) {
        // 握手未完成、玩家ID尚未回填：丢弃（正常不应发生，onPlayerReady 后才进 Play）。
        spdlog::debug("ServerPlayRouter: play packet before playerId bound, dropping");
        return Result<void>::ok();
    }

    m_playHandler.route(m_playerId, packet);

    // TODO(Step4): 死 PacketHandler 迁入的 MoveVehicle/UseEntity/SteerBoat/EntityAction/
    //   PlayerInput 处理体尚未接入（上述 route 暂未覆盖这些 C→S 变体）。
    return Result<void>::ok();
}

} // namespace mc::server::net
