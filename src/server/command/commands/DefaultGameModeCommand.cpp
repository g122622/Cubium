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

#include "DefaultGameModeCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/core/Types.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include <memory>
#include <string>

namespace mc::command {

void DefaultGameModeCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto defaultGameModeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("defaultgamemode");
    defaultGameModeNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(defaultGameModeNode,
        support::makeMetadata(
            "Set the server default game mode.", "/defaultgamemode <survival|creative|adventure|spectator>", 2));

    auto modeArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, GameMode>>(
        "gamemode", GameModeArgumentType::gameMode());
    modeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _setDefaultMode(ctx); });
    defaultGameModeNode->addChild(modeArg);

    dispatcher.registerCommand(defaultGameModeNode);
}

i32 DefaultGameModeCommand::_setDefaultMode(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("Server not available");
        return 0;
    }

    const GameMode mode = context.getArgument<GameMode>("gamemode");
    server->setDefaultGameMode(mode);
    source.sendMessage("Set default game mode to " + std::string(support::getGameModeCommandName(mode)));
    return 1;
}

} // namespace mc::command