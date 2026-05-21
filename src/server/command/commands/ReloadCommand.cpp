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

#include "ReloadCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/item/crafting/RecipeLoader.hpp"
#include "common/item/loot/LootTableLoader.hpp"
#include "common/resource/DataPackList.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include <spdlog/spdlog.h>

namespace mc {
namespace command {

void ReloadCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto reloadNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("reload");
    reloadNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(reloadNode,
        support::makeMetadata("Reloads loot tables, advancements, and functions from disk.", "/reload", 2, {}, true));

    reloadNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return reload(ctx); });

    dispatcher.registerCommand(reloadNode);
}

i32 ReloadCommand::reload(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    auto* server = source.server();
    if (!server) {
        source.sendMessage("Failed to reload: server not available");
        return 0;
    }

    source.sendMessage("Reloading server resources...");

    auto& dataPacks = server->dataPackList();

    // 1. 重新加载战利品表
    auto& lootTableManager = server->lootTableManager();
    loot::LootTableLoader lootLoader(lootTableManager);
    auto lootResult = lootLoader.loadFromDataPackList(dataPacks);
    if (lootResult.failed()) {
        source.sendMessage("Failed to reload loot tables: " + lootResult.error().toString());
        spdlog::error("Failed to reload loot tables: {}", lootResult.error().toString());
    } else {
        const auto& result = lootResult.value();
        source.sendMessage("Reloaded " + std::to_string(result.successCount) + " loot tables" +
            (result.failedCount > 0 ? (" (" + std::to_string(result.failedCount) + " failed)") : ""));
        for (const auto& err : result.errors) {
            spdlog::error("Loot table error: {}", err);
        }
    }

    // 2. 重新加载配方
    RecipeLoader recipeLoader;
    auto recipeResult = recipeLoader.loadFromDataPackList(dataPacks);
    if (recipeResult.failed()) {
        source.sendMessage("Failed to reload recipes: " + recipeResult.error().toString());
        spdlog::error("Failed to reload recipes: {}", recipeResult.error().toString());
    } else {
        const auto& result = recipeResult.value();
        source.sendMessage("Reloaded " + std::to_string(result.successCount) + " recipes" +
            (result.failedCount > 0 ? (" (" + std::to_string(result.failedCount) + " failed)") : ""));
        for (const auto& err : result.errors) {
            spdlog::error("Recipe error: {}", err);
        }
    }

    source.sendMessage("Reload complete!");
    return 1;
}

} // namespace command
} // namespace mc
