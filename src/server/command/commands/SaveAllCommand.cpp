#include "SaveAllCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/application/IServer.hpp"

#include <sstream>

namespace mc {
namespace command {

void SaveAllCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto saveAllNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("save-all");
    saveAllNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(4);
    });
    support::applyMetadata(
        saveAllNode,
        support::makeMetadata(
            "Saves the server to disk.",
            "/save-all [flush]",
            4,
            {},
            false));

    // /save-all
    saveAllNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return saveAll(ctx);
    });

    // /save-all flush
    auto flushNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("flush");
    flushNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return saveAllFlush(ctx);
    });

    saveAllNode->addChild(flushNode);
    dispatcher.registerCommand(saveAllNode);
}

i32 SaveAllCommand::saveAll(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    // TODO: 实现世界保存
    // 需要：
    // 1. ServerWorld::saveAll()
    // 2. PlayerManager::saveAll()
    // 3. 保存区块数据、实体数据、玩家数据

    source.sendMessage("Saving the game (this may take a moment)");
    source.sendMessage("Saved the game");

    return 1;
}

i32 SaveAllCommand::saveAllFlush(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    // TODO: 实现强制刷新保存
    // 强制刷新所有待处理的区块和实体数据到磁盘

    source.sendMessage("Saving the game (this may take a moment)");
    source.sendMessage("Saved the game (flushed)");

    return 1;
}

} // namespace command
} // namespace mc
