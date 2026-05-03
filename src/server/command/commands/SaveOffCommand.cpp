#include "SaveOffCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/application/IServer.hpp"
#include "server/world/ServerWorld.hpp"

namespace mc {
namespace command {

void SaveOffCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto saveOffNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("save-off");
    saveOffNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(4);
    });
    support::applyMetadata(
        saveOffNode,
        support::makeMetadata(
            "Disables server automatic saving.",
            "/save-off",
            4,
            {},
            false));

    saveOffNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return disableAutoSave(ctx);
    });

    dispatcher.registerCommand(saveOffNode);
}

i32 SaveOffCommand::disableAutoSave(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    auto* server = source.server();
    auto* serverWorld = server ? server->world().asServerWorld() : nullptr;
    auto* saveManager = serverWorld ? serverWorld->saveManager() : nullptr;

    if (!saveManager) {
        source.sendMessage("Error: Save manager not available");
        return 0;
    }

    saveManager->stopAutoSave();
    source.sendMessage("Automatic saving is now disabled");
    return 1;
}

} // namespace command
} // namespace mc
