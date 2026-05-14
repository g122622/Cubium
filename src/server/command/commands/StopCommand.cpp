#include "StopCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"

namespace mc::command {

void StopCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto stopNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("stop");
    stopNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(4); });
    support::applyMetadata(stopNode, support::makeMetadata("Stop the server.", "/stop", 4));
    stopNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return stop(ctx); });

    dispatcher.registerCommand(stopNode);
}

i32 StopCommand::stop(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("Server not available");
        return 0;
    }

    source.sendMessage("Stopping server");
    server->requestStop();
    return 1;
}

} // namespace mc::command