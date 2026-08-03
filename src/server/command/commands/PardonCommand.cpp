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

#include "PardonCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/core/Types.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/BannedPlayerList.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"

#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {
namespace command {

void PardonCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto pardonNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("pardon");
    pardonNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(3); });
    support::applyMetadata(
        pardonNode, support::makeMetadata("Removes a player from the ban list.", "/pardon <player>", 3, {}, false));

    // /pardon <player>
    auto playerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::player());
    playerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _pardonPlayer(ctx); });

    pardonNode->addChild(playerArg);
    dispatcher.registerCommand(pardonNode);
}

i32 PardonCommand::_pardonPlayer(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    // 获取玩家名
    std::string playerName;

    // 首先尝试从选择器获取玩家名
    if (selector.hasUsername()) {
        playerName = selector.username();
    } else {
        // 尝试从在线玩家获取
        std::vector<PlayerId> playerIds = support::resolvePlayerIds(source, selector);
        if (!playerIds.empty()) {
            auto* server = source.server();
            if (server != nullptr) {
                auto* playerData = server->playerManager().getPlayer(playerIds.front());
                if (playerData != nullptr) {
                    playerName = playerData->username;
                }
            }
        }
    }

    if (playerName.empty()) {
        source.sendError("commands.pardon.failed.noPlayer");
        return 0;
    }

    auto* server = source.server();
    if (server == nullptr) {
        source.sendError("commands.pardon.failed.noServer");
        return 0;
    }

    auto& banList = server->bannedPlayerList();

    // 检查玩家是否被封禁
    if (!banList.isNameBanned(playerName)) {
        std::ostringstream ss;
        ss << "Could not unban " << playerName << " (not banned)";
        source.sendError(ss.str());
        return 0;
    }

    // 移除封禁
    if (!banList.removeEntryByName(playerName)) {
        std::ostringstream ss;
        ss << "Failed to remove " << playerName << " from ban list";
        source.sendError(ss.str());
        return 0;
    }

    // 保存封禁列表
    auto saveResult = banList.save();
    if (saveResult.failed()) {
        spdlog::error("Failed to save banned players list: {}", saveResult.error().message());
    }

    // 发送成功消息
    std::ostringstream ss;
    ss << "Unbanned " << playerName;
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
