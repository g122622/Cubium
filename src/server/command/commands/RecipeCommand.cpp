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

#include "RecipeCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/core/Types.hpp"
#include "common/item/crafting/RecipeManager.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace mc {
namespace command {

void RecipeCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto recipeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("recipe");
    recipeNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(recipeNode,
        support::makeMetadata(
            "Gives or takes recipes from players.", "/recipe <give|take> <targets> <recipe|*>", 2, {}, true));

    // 原版：/recipe <give|take> <targets> <recipe|*>，先给字面量再收 targets。
    auto giveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("give");
    auto takeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("take");
    const auto addTargets = [](CommandNode<ServerCommandSource>& verbNode, bool give) {
        auto targetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
            "targets", EntityArgumentType::players());
        auto recipeArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ResourceLocation>>(
            "recipe", RecipeIdArgumentType::recipe());
        recipeArg->setCommand([give](CommandContext<ServerCommandSource>& ctx) {
            return give ? _giveRecipe(ctx) : _takeRecipe(ctx);
        });
        auto allNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("*");
        allNode->setCommand([give](CommandContext<ServerCommandSource>& ctx) {
            return give ? _giveRecipe(ctx) : _takeRecipe(ctx);
        });
        targetsArg->addChild(recipeArg);
        targetsArg->addChild(allNode);
        verbNode.addChild(targetsArg);
    };
    addTargets(*giveNode, true);
    addTargets(*takeNode, false);

    recipeNode->addChild(giveNode);
    recipeNode->addChild(takeNode);
    dispatcher.registerCommand(recipeNode);
}

i32 RecipeCommand::_giveRecipe(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");

    std::string recipeStr = "*";
    if (context.hasArgument("recipe")) {
        recipeStr = context.getArgument<ResourceLocation>("recipe").toString();
    }

    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No players matched the selector");
        return 0;
    }

    // 收集要解锁的配方ID列表
    std::vector<ResourceLocation> recipesToGive;
    if (recipeStr == "*") {
        // 给予所有已注册的配方
        auto allRecipes = crafting::RecipeManager::instance().getAllRecipes();
        recipesToGive.reserve(allRecipes.size());
        for (const auto* recipe : allRecipes) {
            if (recipe != nullptr && !recipe->isDynamic()) {
                recipesToGive.push_back(recipe->getId());
            }
        }
    } else {
        // 给予指定配方
        ResourceLocation recipeId(recipeStr);
        const auto* recipe = crafting::RecipeManager::instance().getRecipe(recipeId);
        if (recipe == nullptr) {
            source.sendError("Unknown recipe: " + recipeStr);
            return 0;
        }
        if (recipe->isDynamic()) {
            source.sendError("Cannot give dynamic recipe: " + recipeStr);
            return 0;
        }
        recipesToGive.push_back(recipeId);
    }

    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        // 通过 ServerPlayerEntityManager 获取 Player 实体
        Player* player = source.server()->playerEntityManager().getPlayerEntity(playerId, *source.world());
        if (player == nullptr) {
            continue;
        }

        ServerPlayer* serverPlayer = player->asServerPlayer();
        if (serverPlayer == nullptr) {
            continue;
        }

        // 解锁配方
        size_t unlocked = serverPlayer->unlockRecipes(recipesToGive);
        if (unlocked > 0) {
            successCount++;
        }
    }

    if (recipeStr == "*") {
        std::ostringstream ss;
        ss << "Gave all recipes to " << successCount << " player(s)";
        source.sendMessage(ss.str());
    } else {
        std::ostringstream ss;
        ss << "Gave recipe '" << recipeStr << "' to " << successCount << " player(s)";
        source.sendMessage(ss.str());
    }

    return successCount;
}

i32 RecipeCommand::_takeRecipe(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");

    std::string recipeStr = "*";
    if (context.hasArgument("recipe")) {
        recipeStr = context.getArgument<ResourceLocation>("recipe").toString();
    }

    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No players matched the selector");
        return 0;
    }

    // 收集要锁定的配方ID列表
    std::vector<ResourceLocation> recipesToTake;
    if (recipeStr == "*") {
        // 从所有已注册配方中收集（需要锁定玩家已解锁的所有配方）
        auto allRecipes = crafting::RecipeManager::instance().getAllRecipes();
        recipesToTake.reserve(allRecipes.size());
        for (const auto* recipe : allRecipes) {
            if (recipe != nullptr && !recipe->isDynamic()) {
                recipesToTake.push_back(recipe->getId());
            }
        }
    } else {
        // 锁定指定配方
        ResourceLocation recipeId(recipeStr);
        const auto* recipe = crafting::RecipeManager::instance().getRecipe(recipeId);
        if (recipe == nullptr) {
            source.sendError("Unknown recipe: " + recipeStr);
            return 0;
        }
        recipesToTake.push_back(recipeId);
    }

    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        // 通过 ServerPlayerEntityManager 获取 Player 实体
        Player* player = source.server()->playerEntityManager().getPlayerEntity(playerId, *source.world());
        if (player == nullptr) {
            continue;
        }

        ServerPlayer* serverPlayer = player->asServerPlayer();
        if (serverPlayer == nullptr) {
            continue;
        }

        // 锁定配方
        size_t locked = serverPlayer->lockRecipes(recipesToTake);
        if (locked > 0) {
            successCount++;
        }
    }

    if (recipeStr == "*") {
        std::ostringstream ss;
        ss << "Took all recipes from " << successCount << " player(s)";
        source.sendMessage(ss.str());
    } else {
        std::ostringstream ss;
        ss << "Took recipe '" << recipeStr << "' from " << successCount << " player(s)";
        source.sendMessage(ss.str());
    }

    return successCount;
}

} // namespace command
} // namespace mc
