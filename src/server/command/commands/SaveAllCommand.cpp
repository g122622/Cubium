#include "SaveAllCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/application/IServer.hpp"
#include "server/world/ServerWorld.hpp"
#include "common/world/storage/WorldStorageService.hpp"

#include <sstream>
#include <spdlog/spdlog.h>

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
    auto* server = source.server();

    if (!server) {
        source.sendMessage("Error: Server not available");
        return 0;
    }

    source.sendMessage("Saving the game (this may take a moment)...");

    // 保存世界
    auto& world = server->world();
    auto* serverWorld = world.asServerWorld();

    size_t totalSections = 0;

    if (serverWorld && serverWorld->isStorageOpen()) {
        auto result = serverWorld->saveAll();
        if (result.success()) {
            totalSections += result.value();
        } else {
            source.sendMessage(fmt::format("Failed to save world: {}", result.error().message()));
            spdlog::error("Failed to save world: {}", result.error().message());
            return 0;
        }
    }

    // TODO: 保存玩家数据

    source.sendMessage(fmt::format("Saved the game ({} sections)", totalSections));
    spdlog::info("Game saved by {} ({} sections)", source.name(), totalSections);

    return 1;
}

i32 SaveAllCommand::saveAllFlush(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    auto* server = source.server();

    if (!server) {
        source.sendMessage("Error: Server not available");
        return 0;
    }

    source.sendMessage("Saving the game with flush (this may take a moment)...");

    // 保存世界
    auto& world = server->world();
    auto* serverWorld = world.asServerWorld();

    size_t totalSections = 0;

    if (serverWorld && serverWorld->isStorageOpen()) {
        // 保存所有缓存数据
        auto result = serverWorld->saveAll();
        if (!result.success()) {
            source.sendMessage(fmt::format("Failed to save world: {}", result.error().message()));
            return 0;
        }

        // 清除所有缓存（强制刷新到磁盘）
        serverWorld->storage().clearAllCaches();

        totalSections = result.value();
    }

    // TODO: 保存玩家数据

    source.sendMessage(fmt::format("Saved the game (flushed, {} sections)", totalSections));
    spdlog::info("Game saved with flush by {} ({} sections)", source.name(), totalSections);

    return 1;
}

} // namespace command
} // namespace mc
