#include "SetIdleTimeoutCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"

#include <sstream>

namespace mc::command {

void SetIdleTimeoutCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto timeoutNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("setidletimeout");
    timeoutNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(3);
    });
    support::applyMetadata(
        timeoutNode,
        support::makeMetadata(
            "Set the player idle timeout in minutes.",
            "/setidletimeout <minutes>",
            3));

    auto minutesArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "minutes",
        IntegerArgumentType::integer(0, 1440));
    minutesArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return setTimeout(ctx);
    });
    timeoutNode->addChild(minutesArg);

    dispatcher.registerCommand(timeoutNode);
}

i32 SetIdleTimeoutCommand::setTimeout(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("Server not available");
        return 0;
    }

    const i32 timeout = context.getArgument<i32>("minutes");
    server->setPlayerIdleTimeoutMinutes(timeout);

    std::ostringstream ss;
    ss << "Set idle timeout to " << timeout << " minute(s)";
    source.sendMessage(ss.str());
    return 1;
}

} // namespace mc::command