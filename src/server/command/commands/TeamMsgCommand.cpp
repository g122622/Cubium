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

#include "TeamMsgCommand.hpp"

#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/scoreboard/core/ScorePlayerTeam.hpp"
#include "common/util/text/ComponentNbtSerialization.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"

#include <memory>
#include <sstream>
#include <string>

namespace mc::command {
namespace {

/**
 * @brief 找出玩家所在的队伍（原版 Scoreboard.getPlayersTeam）
 *
 * 原版按成员名在全部队伍里查；Cubium 的成员集合存的是玩家名，故这里同样用名字匹配。
 */
scoreboard::ScorePlayerTeam* _playerTeam(server::ServerScoreboard& scoreboard, const std::string& playerName)
{
    for (auto* team : scoreboard.getTeams()) {
        if (team->getMembers().contains(playerName)) {
            return team;
        }
    }
    return nullptr;
}

/// 队伍显示名（原版 PlayerTeam.getFormattedDisplayName，带队伍颜色格式码）。
std::string _teamName(scoreboard::ScorePlayerTeam& team)
{
    return team.getFormattedDisplayName()->getFormattedText();
}

} // namespace

void TeamMsgCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto messageArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ComponentTextPtr>>(
        "message", MessageComponentArgumentType::message());
    messageArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _sendTeamMessage(ctx); });

    auto teamMsgNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("teammsg");
    teamMsgNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(0);
    });
    support::applyMetadata(
        teamMsgNode, support::makeMetadata("Sends a message to your team.", "/teammsg <message>", 0, {}, true));
    teamMsgNode->addChild(messageArg);

    // 原版把 /tm 注册成 teammsg 的重定向节点。
    auto tmNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("tm");
    tmNode->setRedirect(teamMsgNode);

    dispatcher.registerCommand(teamMsgNode);
    dispatcher.registerCommand(tmNode);
}

i32 TeamMsgCommand::_sendTeamMessage(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr || !source.isPlayer()) {
        source.sendError("Only players can use this command");
        return 0;
    }

    const std::string message = context.getArgument<ComponentTextPtr>("message")->text;
    const std::string senderName = source.name();

    auto& scoreboard = server->scoreboard();
    auto* team = _playerTeam(scoreboard, senderName);
    if (team == nullptr || team->getMembers().empty()) {
        source.sendError("You are not in a team");
        return 0;
    }

    // 原版 TeamMsgCommand：组成 "<sender> -> [team]: <message>" 发给队伍里的每个在线成员，
    // 包括发送者自己（发送者收到的是同样的文本，而不是额外的确认消息）。
    std::ostringstream line;
    line << senderName << " -> [" << _teamName(*team) << "]: " << message;

    auto& players = server->playerManager();
    i32 sent = 0;
    for (const auto& memberName : team->getMembers()) {
        auto* data = players.findByUsername(memberName);
        if (data == nullptr) {
            continue;
        }
        auto conn = data->getConnection();
        if (!conn || !conn->isConnected()) {
            continue;
        }
        mc::network::ir::play::SystemChat packet;
        packet.content = ::mc::text::plainTextToNbtBytes(line.str());
        packet.overlay = false;
        auto result = conn->send(mc::network::ir::IrPacket{
            mc::network::protocol::ConnectionProtocol::Play, mc::network::ir::PlayPacket{std::move(packet)}});
        if (result.failed()) {
            continue;
        }
        ++sent;
    }

    if (sent == 0) {
        source.sendError("No team members online");
        return 0;
    }
    return 1;
}

} // namespace mc::command