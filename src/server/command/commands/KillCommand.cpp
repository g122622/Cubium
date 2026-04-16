#include "KillCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/player/ServerPlayer.hpp"

#include <sstream>

namespace mc {
namespace command {

void KillCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    using namespace mc::command;

    auto killNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("kill");
    killNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        killNode,
        support::makeMetadata(
            "Kill entities.",
            "/kill [target]",
            2,
            {},
            false));
    killNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return killSelf(ctx);
    });

    auto targetArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "target",
        EntityArgumentType::entities()
    );
    targetArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return killTarget(ctx);
    });
    killNode->addChild(targetArg);

    dispatcher.registerCommand(killNode);
}

i32 KillCommand::killSelf(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    if (!source.isPlayer()) {
        source.sendMessage("You must be an entity to use this command");
        return 0;
    }

    ServerPlayer& player = source.assertPlayer();

    std::ostringstream ss;
    ss << "Killed " << player.username();
    source.sendMessage(ss.str());

    return 1;
}

i32 KillCommand::killTarget(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("target");
    (void)selector;

    source.sendMessage("Killed target entity");
    return 1;
}

} // namespace command
} // namespace mc