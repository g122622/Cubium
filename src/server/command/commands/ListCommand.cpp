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

#include "ListCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/core/Types.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/core/PlayerManager.hpp"

#include <cstddef>
#include <utility>
#include <vector>
#include <memory>
#include <sstream>

namespace mc {
namespace command {

void ListCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    using namespace mc::command;

    auto listNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    listNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(0);
    });
    support::applyMetadata(listNode, support::makeMetadata("List online players.", "/list", 0));
    listNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return _listPlayers(ctx);
    });

    // /list uuids：原版把每个在线玩家的 UUID 也列出来。
    auto uuidsNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("uuids");
    uuidsNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _listPlayerUuids(ctx); });
    listNode->addChild(uuidsNode);

    dispatcher.registerCommand(listNode);
}

i32 ListCommand::_listPlayers(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    size_t playerCount = 0;

    if (auto* server = source.server()) {
        server->playerManager().forEachPlayer([&playerCount](auto&) {
            ++playerCount;
            return true;
        });
    }

    std::ostringstream ss;
    ss << "There are " << playerCount << " player(s) online";
    source.sendMessage(ss.str());

    return static_cast<i32>(playerCount);
}

i32 ListCommand::_listPlayerUuids(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    std::vector<std::pair<std::string, std::string>> players;

    if (auto* server = source.server()) {
        server->playerManager().forEachPlayer([&players](auto& data) {
            players.emplace_back(data.username, data.uuid);
            return true;
        });
    }

    std::ostringstream ss;
    ss << "There are " << players.size() << " of a max of " << players.size() << " players online:";
    for (const auto& [name, uuid] : players) {
        ss << " " << name << " (" << uuid << ")";
    }
    source.sendMessage(ss.str());

    return static_cast<i32>(players.size());
}

} // namespace command
} // namespace mc