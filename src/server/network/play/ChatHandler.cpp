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

#include "server/network/play/ChatHandler.hpp"

#include "common/core/Types.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/util/math/Vector2.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/dimension/Dimension.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <string>
#include <variant>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::server::net {

void ChatHandler::handleChatMessagePacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::Chat>(&play);
    if (evt == nullptr) {
        return;
    }

    const std::string& message = evt->message;

    if (!message.empty() && message[0] == '/') {
        // 命令：message 含 '/' 前缀，CommandDispatcher::parse 自动剥离。
        _executePlayerCommand(playerId, message);
        return;
    }

    spdlog::info("[Chat] {}: {}", player->username, message);
}

void ChatHandler::handleChatCommandPacket(PlayerId playerId, const mc::network::ir::IrPacket& packet)
{
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    const auto& play = std::get<mc::network::ir::PlayPacket>(packet.packet);
    const auto* evt = std::get_if<mc::network::ir::play::ChatCommand>(&play);
    if (evt == nullptr) {
        return;
    }

    // ChatCommand.command 不含 '/' 前缀（对齐 vanilla ServerboundChatCommandPacket）。
    // CommandDispatcher::parse 对有无 '/' 均自动剥离，故直接传 evt->command，无需补 '/'。
    _executePlayerCommand(playerId, evt->command);
}

void ChatHandler::_executePlayerCommand(PlayerId playerId, const std::string& commandInput)
{
    auto* player = m_server.playerManager().getPlayer(playerId);
    if (!player || !player->loggedIn) {
        return;
    }

    DimensionId commandDimension = 0;
    if (auto* playerWorld = m_server.getPlayerWorld(playerId)) {
        commandDimension = playerWorld->dimension();
    }
    // 从玩家管理器查找 Player 实体作为命令执行实体。
    mc::Entity* commandEntity = nullptr;
    if (auto* cmdDim = m_server.dimensionManager().getDimension(commandDimension)) {
        if (auto* cmdWorld = cmdDim->world()) {
            commandEntity = m_server.playerEntityManager().getPlayerEntity(playerId, *cmdWorld);
        }
    }

    mc::command::ServerCommandSource source(&m_server,
        nullptr,
        commandDimension,
        Vector3d(player->x, player->y, player->z),
        Vector2f(player->yaw, player->pitch),
        static_cast<i32>(m_server.resolveOpLevel(player->uuid)),
        playerId,
        player->username,
        commandEntity);
    // CommandDispatcher::parse 自动剥离前导 '/'，commandInput 含或不含 '/' 均可。
    auto cmdResult = m_server.commandRegistry().execute(commandInput, source);
    if (cmdResult.failed()) {
        spdlog::warn("Command '{}' failed for {}: {}", commandInput, player->username, cmdResult.error().toString());
    } else {
        spdlog::info("Command '{}' executed for {} with result {}", commandInput, player->username, cmdResult.value());
    }
}

} // namespace mc::server::net
