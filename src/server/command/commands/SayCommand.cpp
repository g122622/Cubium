#include "SayCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"

namespace mc::command {

void SayCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto sayNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("say");
    sayNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        sayNode,
        support::makeMetadata(
            "Broadcast a server message.",
            "/say <message>",
            2));

    auto messageArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "message",
        StringArgumentType::greedyString());
    messageArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return say(ctx);
    });
    sayNode->addChild(messageArg);

    dispatcher.registerCommand(sayNode);
}

i32 SayCommand::say(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("Server not available");
        return 0;
    }

    const String message = context.getArgument<String>("message");
    server->broadcastServerMessage("[" + source.name() + "] " + message);
    source.sendMessage("Broadcasted message");
    return 1;
}

} // namespace mc::command