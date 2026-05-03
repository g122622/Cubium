#include "SaveOnCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/application/IServer.hpp"
#include "server/world/ServerWorld.hpp"

namespace mc {
namespace command {

void SaveOnCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto saveOnNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("save-on");
    saveOnNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(4);
    });
    support::applyMetadata(
        saveOnNode,
        support::makeMetadata(
            "Enables server automatic saving.",
            "/save-on",
            4,
            {},
            false));

    saveOnNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return enableAutoSave(ctx);
    });

    dispatcher.registerCommand(saveOnNode);
}

i32 SaveOnCommand::enableAutoSave(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    auto* server = source.server();
    auto* serverWorld = server ? server->world().asServerWorld() : nullptr;
    auto* saveManager = serverWorld ? serverWorld->saveManager() : nullptr;

    if (!saveManager) {
        source.sendMessage("Error: Save manager not available");
        return 0;
    }

    saveManager->startAutoSave();
    source.sendMessage("Automatic saving is now enabled");
    return 1;
}

} // namespace command
} // namespace mc
