#include "MeCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"

namespace mc {
namespace command {

void MeCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto meNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("me");
    meNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(0); });
    support::applyMetadata(
        meNode, support::makeMetadata("Displays a message about yourself in chat.", "/me <action>", 0, {}, true));

    auto actionArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "action", StringArgumentType::greedyString());
    actionArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return performAction(ctx); });
    meNode->addChild(actionArg);

    dispatcher.registerCommand(meNode);
}

i32 MeCommand::performAction(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();
    if (server == nullptr) {
        source.sendMessage("Server not available");
        return 0;
    }

    const std::string action = context.getArgument<std::string>("action");
    const std::string& name = source.name();

    // 广播 "* playername action" 格式的消息
    server->broadcastServerMessage("* " + name + " " + action);
    return 1;
}

} // namespace command
} // namespace mc
