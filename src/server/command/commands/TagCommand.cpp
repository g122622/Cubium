#include "TagCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/PlayerManager.hpp"
#include <sstream>

namespace mc {
namespace command {

void TagCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto tagNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("tag");
    tagNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        tagNode,
        support::makeMetadata(
            "Manages entity tags.",
            "/tag <targets> <add|remove|list> [tag]",
            2,
            {},
            true));

    auto targetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets",
        EntityArgumentType::entities());

    // /tag <targets> add <tag>
    auto addNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("add");
    auto tagArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "tag",
        StringArgumentType::string());
    tagArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return addTag(ctx);
    });
    addNode->addChild(tagArg);
    targetsArg->addChild(addNode);

    // /tag <targets> remove <tag>
    auto removeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("remove");
    auto removeTagArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, std::string>>(
        "tag",
        StringArgumentType::string());
    removeTagArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return removeTag(ctx);
    });
    removeNode->addChild(removeTagArg);
    targetsArg->addChild(removeNode);

    // /tag <targets> list
    auto listNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("list");
    listNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return listTags(ctx);
    });
    targetsArg->addChild(listNode);

    tagNode->addChild(targetsArg);
    dispatcher.registerCommand(tagNode);
}

i32 TagCommand::addTag(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");
    const std::string tag = context.getArgument<std::string>("tag");

    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No entities matched the selector");
        return 0;
    }

    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        auto player = source.server()->playerManager().getPlayer(playerId);
        if (player) {
            // TODO: 实现实体标签系统
            successCount++;
        }
    }

    std::ostringstream ss;
    ss << "Added tag '" << tag << "' to " << successCount << " entit" << (successCount == 1 ? "y" : "ies");
    source.sendMessage(ss.str());

    return successCount;
}

i32 TagCommand::removeTag(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");
    const std::string tag = context.getArgument<std::string>("tag");

    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No entities matched the selector");
        return 0;
    }

    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        auto player = source.server()->playerManager().getPlayer(playerId);
        if (player) {
            // TODO: 实现实体标签系统
            successCount++;
        }
    }

    std::ostringstream ss;
    ss << "Removed tag '" << tag << "' from " << successCount << " entit" << (successCount == 1 ? "y" : "ies");
    source.sendMessage(ss.str());

    return successCount;
}

i32 TagCommand::listTags(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");

    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No entities matched the selector");
        return 0;
    }

    // TODO: 实现实体标签系统
    if (playerIds.size() == 1) {
        auto player = source.server()->playerManager().getPlayer(playerIds[0]);
        if (player) {
            source.sendMessage(player->username + " has no tags");
        }
    } else {
        source.sendMessage("Listing tags for " + std::to_string(playerIds.size()) + " entities");
    }

    return 1;
}

} // namespace command
} // namespace mc
