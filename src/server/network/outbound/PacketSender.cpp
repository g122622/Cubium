/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "server/network/outbound/PacketSender.hpp"
#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/pipeline/OutboundPacket.hpp"
#include "common/util/UuidUtils.hpp"
#include "server/player/lifecycle/PlayerManager.hpp"
#include "server/player/runtime/ServerPlayerData.hpp"
#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::server {

PacketSender::PacketSender(PlayerManager& playerManager)
    : m_playerManager(playerManager)
{}

bool PacketSender::sendToPlayer(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_playerManager.getPlayer(playerId);
    if (!player) {
        return false;
    }
    // 复制一份：单玩家发送直接移动，但仍需保留原包供调用方
    return player->send(mc::network::ir::IrPacket{packet});
}

void PacketSender::broadcast(const mc::network::ir::IrPacket& packet)
{
    auto shared = std::make_shared<const mc::network::pipeline::OutboundPacket>(packet);
    m_playerManager.forEachPlayer([&](ServerPlayerData& player) {
        player.sendShared(shared);
    });
}

void PacketSender::broadcastExcept(PlayerId excludePlayerId, const mc::network::ir::IrPacket& packet)
{
    auto shared = std::make_shared<const mc::network::pipeline::OutboundPacket>(packet);
    m_playerManager.forEachPlayer([&](ServerPlayerData& player) {
        if (player.playerId != excludePlayerId) {
            player.sendShared(shared);
        }
    });
}

} // namespace mc::server
