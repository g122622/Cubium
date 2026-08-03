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

#include "common/advancement/AdvancementLoader.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/core/Types.hpp"
#include "common/item/crafting/RecipeLoader.hpp"
#include "common/item/loot/LootPredicateLoader.hpp"
#include "common/item/loot/LootTableLoader.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/function/FunctionLoader.hpp"
#include "server/function/FunctionManager.hpp"
#include <memory>
#include <string>
#include <spdlog/spdlog.h>

namespace mc {
namespace command {

void ReloadCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto reloadNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("reload");
    reloadNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(reloadNode,
        support::makeMetadata("Reloads loot tables, recipes, functions, predicates, and advancements from data packs.",
            "/reload",
            2,
            {},
            true));

    reloadNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _reload(ctx); });

    dispatcher.registerCommand(reloadNode);
}

i32 ReloadCommand::_reload(CommandContext<ServerCommandSource>& context)
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
    auto lootResult = lootLoader.loadFromDataPackRepository(dataPacks);
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
    auto recipeResult = recipeLoader.loadFromDataPackRepository(dataPacks);
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

    // 3. 重新加载函数
    auto& functionManager = server->functionManager();
    function::FunctionLoader functionLoader(functionManager);
    auto funcResult = functionLoader.loadFromDataPackRepository(dataPacks);
    if (funcResult.failed()) {
        source.sendMessage("Failed to reload functions: " + funcResult.error().toString());
        spdlog::error("Failed to reload functions: {}", funcResult.error().toString());
    } else {
        const auto& result = funcResult.value();
        source.sendMessage("Reloaded " + std::to_string(result.successCount) + " functions" +
            (result.failedCount > 0 ? (" (" + std::to_string(result.failedCount) + " failed)") : ""));
        for (const auto& err : result.errors) {
            spdlog::error("Function error: {}", err);
        }
        // 通知函数管理器重新加载完成，下次 tick 时执行 minecraft:load 标签
        functionManager.notifyReload();
    }

    // 4. 重新加载战利品谓词
    {
        auto& predicateMgr = server->predicateManager();
        loot::LootPredicateLoader predicateLoader(predicateMgr);
        auto predicateResult = predicateLoader.loadFromDataPackRepository(dataPacks);
        if (predicateResult.failed()) {
            source.sendMessage("Failed to reload predicates: " + predicateResult.error().toString());
            spdlog::error("Failed to reload predicates: {}", predicateResult.error().toString());
        } else {
            const auto& result = predicateResult.value();
            source.sendMessage("Reloaded " + std::to_string(result.successCount) + " predicates" +
                (result.failedCount > 0 ? (" (" + std::to_string(result.failedCount) + " failed)") : ""));
            for (const auto& err : result.errors) {
                spdlog::error("Predicate error: {}", err);
            }
        }
        // 重新关联谓词管理器到掉落表管理器
        server->lootTableManager().setPredicateManager(&predicateMgr);
    }

    // 5. 重新加载进度
    {
        advancement::AdvancementLoader advancementLoader;
        auto advancementResult = advancementLoader.loadFromDataPackRepository(dataPacks);
        if (advancementResult.failed()) {
            source.sendMessage("Failed to reload advancements: " + advancementResult.error().toString());
            spdlog::error("Failed to reload advancements: {}", advancementResult.error().toString());
        } else {
            const auto& result = advancementResult.value();
            source.sendMessage("Reloaded " + std::to_string(result.successCount) + " advancements" +
                (result.failedCount > 0 ? (" (" + std::to_string(result.failedCount) + " failed)") : ""));
            for (const auto& err : result.errors) {
                spdlog::error("Advancement error: {}", err);
            }
        }
    }

    source.sendMessage("Reload complete!");
    return 1;
}

} // namespace command
} // namespace mc
