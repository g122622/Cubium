#include "SaveOnCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/application/IServer.hpp"

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

    // TODO: 实现自动保存控制
    // 需要：
    // 1. IServer::setAutoSaveEnabled(true)
    // 2. 或 WorldSaveManager::enableAutoSave()

    source.sendMessage("Automatic saving is now enabled");
    return 1;
}

} // namespace command
} // namespace mc
