/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "SaveAllCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/core/Types.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "common/world/storage/player/PlayerDataManager.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"

#include <cstddef>
#include <memory>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace mc {
namespace command {

namespace {

[[nodiscard]] world::storage::SingleLevelStorageManager* getSharedStorage(server::IServer& server)
{
    auto* storage = server.sharedStorage();
    if (storage == nullptr || !storage->isOpen()) {
        return nullptr;
    }
    return storage;
}

} // namespace

void SaveAllCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto saveAllNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("save-all");
    saveAllNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(4); });
    support::applyMetadata(
        saveAllNode, support::makeMetadata("Saves the server to disk.", "/save-all [flush]", 4, {}, false));

    // /save-all
    saveAllNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _saveAll(ctx); });

    // /save-all flush
    auto flushNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("flush");
    flushNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _saveAllFlush(ctx); });

    saveAllNode->addChild(flushNode);
    dispatcher.registerCommand(saveAllNode);
}

i32 SaveAllCommand::_saveAll(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();

    if (!server) {
        source.sendMessage("Error: Server not available");
        return 0;
    }

    source.sendMessage("Saving the game (this may take a moment)...");

    auto* storage = getSharedStorage(*server);
    if (storage == nullptr) {
        source.sendMessage("Failed to save world: shared storage not available");
        return 0;
    }

    if (server->isSharedStorageReadonlyForeignWorld()) {
        source.sendMessage(fmt::format("Current world is a readonly foreign save (format: {}); no data will be written",
            storage->formatInfo().formatName));
        return 0;
    }

    size_t totalSections = 0;
    auto result = storage->saveAll();
    if (result.success()) {
        totalSections += result.value();
    } else {
        source.sendMessage(fmt::format("Failed to save world: {}", result.error().message()));
        spdlog::error("Failed to save world: {}", result.error().message());
        return 0;
    }

    const size_t savedPlayers = server->playerManager().playerCount();

    source.sendMessage(fmt::format("Saved the game ({} sections, {} players)", totalSections, savedPlayers));
    spdlog::info("Game saved by {} ({} sections, {} players)", source.name(), totalSections, savedPlayers);

    return 1;
}

i32 SaveAllCommand::_saveAllFlush(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    auto* server = source.server();

    if (!server) {
        source.sendMessage("Error: Server not available");
        return 0;
    }

    source.sendMessage("Saving the game with flush (this may take a moment)...");

    auto* storage = getSharedStorage(*server);
    if (storage == nullptr) {
        source.sendMessage("Failed to save world: shared storage not available");
        return 0;
    }

    if (server->isSharedStorageReadonlyForeignWorld()) {
        source.sendMessage(fmt::format("Current world is a readonly foreign save (format: {}); flush was skipped",
            storage->formatInfo().formatName));
        return 0;
    }

    size_t totalSections = 0;
    auto result = storage->saveAll();
    if (!result.success()) {
        source.sendMessage(fmt::format("Failed to save world: {}", result.error().message()));
        return 0;
    }
    storage->clearAllCaches();
    totalSections = result.value();

    if (auto* playerDataManager = storage->playerDataManager()) {
        playerDataManager->clearCache();
    }
    const size_t savedPlayers = server->playerManager().playerCount();

    source.sendMessage(fmt::format("Saved the game (flushed, {} sections, {} players)", totalSections, savedPlayers));
    spdlog::info("Game saved with flush by {} ({} sections, {} players)", source.name(), totalSections, savedPlayers);

    return 1;
}

} // namespace command
} // namespace mc
