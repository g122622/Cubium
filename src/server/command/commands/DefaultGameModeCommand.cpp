#include "DefaultGameModeCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"

namespace mc::command {

void DefaultGameModeCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto defaultGameModeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("defaultgamemode");
    defaultGameModeNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        defaultGameModeNode,
        support::makeMetadata(
            "Set the server default game mode.",
            "/defaultgamemode <survival|creative|adventure|spectator>",
            2));

    auto modeArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, GameMode>>(
        "gamemode",
        GameModeArgumentType::gameMode());
    modeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setDefaultMode(ctx);
    });
    defaultGameModeNode->addChild(modeArg);

    dispatcher.registerCommand(defaultGameModeNode);
}

i32 DefaultGameModeCommand::setDefaultMode(CommandContext<ServerCommandSource>& context)
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