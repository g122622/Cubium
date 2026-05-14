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
#include "common/command/arguments/ArgumentType.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/PlayerManager.hpp"
#include <sstream>

namespace mc {
namespace command {

void RecipeCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto recipeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("recipe");
    recipeNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(recipeNode,
        support::makeMetadata(
            "Gives or takes recipes from players.", "/recipe <give|take> <targets> <recipe|*>", 2, {}, true));

    auto targetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::players());

    // /recipe give <targets> <recipe|*>
    auto giveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("give");
    auto giveRecipeArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("recipe", StringArgumentType::string());
    giveRecipeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return giveRecipe(ctx); });
    giveNode->addChild(giveRecipeArg);

    auto giveAllNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("*");
    giveAllNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return giveRecipe(ctx); });
    giveNode->addChild(giveAllNode);
    targetsArg->addChild(giveNode);

    // /recipe take <targets> <recipe|*>
    auto takeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("take");
    auto takeRecipeArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>("recipe", StringArgumentType::string());
    takeRecipeArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return takeRecipe(ctx); });
    takeNode->addChild(takeRecipeArg);

    auto takeAllNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("*");
    takeAllNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return takeRecipe(ctx); });
    takeNode->addChild(takeAllNode);
    targetsArg->addChild(takeNode);

    recipeNode->addChild(targetsArg);
    dispatcher.registerCommand(recipeNode);
}

i32 RecipeCommand::giveRecipe(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");

    std::string recipe = "*";
    if (context.hasArgument("recipe")) {
        recipe = context.getArgument<std::string>("recipe");
    }

    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No players matched the selector");
        return 0;
    }

    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        auto player = source.server()->playerManager().getPlayer(playerId);
        if (player) {
            // TODO: 实现配方解锁系统
            successCount++;
        }
    }

    if (recipe == "*") {
        std::ostringstream ss;
        ss << "Gave all recipes to " << successCount << " player(s)";
        source.sendMessage(ss.str());
    } else {
        std::ostringstream ss;
        ss << "Gave recipe '" << recipe << "' to " << successCount << " player(s)";
        source.sendMessage(ss.str());
    }

    return successCount;
}

i32 RecipeCommand::takeRecipe(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");

    std::string recipe = "*";
    if (context.hasArgument("recipe")) {
        recipe = context.getArgument<std::string>("recipe");
    }

    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No players matched the selector");
        return 0;
    }

    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        auto player = source.server()->playerManager().getPlayer(playerId);
        if (player) {
            // TODO: 实现配方解锁系统
            successCount++;
        }
    }

    if (recipe == "*") {
        std::ostringstream ss;
        ss << "Took all recipes from " << successCount << " player(s)";
        source.sendMessage(ss.str());
    } else {
        std::ostringstream ss;
        ss << "Took recipe '" << recipe << "' from " << successCount << " player(s)";
        source.sendMessage(ss.str());
    }

    return successCount;
}

} // namespace command
} // namespace mc
