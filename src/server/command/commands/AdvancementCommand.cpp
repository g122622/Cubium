#include "AdvancementCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/PlayerManager.hpp"
#include <sstream>

namespace mc {
namespace command {

void AdvancementCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto advancementNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("advancement");
    advancementNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        advancementNode,
        support::makeMetadata(
            "Grants, revokes, or tests advancements.",
            "/advancement <grant|revoke|test> <targets> ...",
            2,
            {},
            true));

    auto targetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets",
        EntityArgumentType::entities());

    // /advancement grant <targets> everything
    auto grantNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("grant");
    auto everythingNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("everything");
    everythingNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return grantAdvancement(ctx);
    });

    // /advancement grant <targets> only <advancement>
    auto onlyNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("only");
    auto advancementArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "advancement",
        StringArgumentType::string());
    advancementArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return grantAdvancement(ctx);
    });
    onlyNode->addChild(advancementArg);

    grantNode->addChild(everythingNode);
    grantNode->addChild(onlyNode);
    targetsArg->addChild(grantNode);

    // /advancement revoke <targets> everything
    auto revokeNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("revoke");
    auto revokeEverythingNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("everything");
    revokeEverythingNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return revokeAdvancement(ctx);
    });

    // /advancement revoke <targets> only <advancement>
    auto revokeOnlyNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("only");
    auto revokeAdvArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "advancement",
        StringArgumentType::string());
    revokeAdvArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return revokeAdvancement(ctx);
    });
    revokeOnlyNode->addChild(revokeAdvArg);

    revokeNode->addChild(revokeEverythingNode);
    revokeNode->addChild(revokeOnlyNode);
    targetsArg->addChild(revokeNode);

    // /advancement test <targets> <advancement>
    auto testNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("test");
    auto testTargetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets",
        EntityArgumentType::entities());
    auto testAdvArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "advancement",
        StringArgumentType::string());
    testAdvArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return testAdvancement(ctx);
    });
    testTargetsArg->addChild(testAdvArg);
    testNode->addChild(testTargetsArg);
    advancementNode->addChild(testNode);

    advancementNode->addChild(targetsArg);
    dispatcher.registerCommand(advancementNode);
}

i32 AdvancementCommand::grantAdvancement(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");

    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No players matched the selector");
        return 0;
    }

    String advancement = "everything";
    if (context.hasArgument("advancement")) {
        advancement = context.getArgument<String>("advancement");
    }

    std::ostringstream ss;
    ss << "Granted advancement '" << advancement << "' to " << playerIds.size() << " player(s)";
    source.sendMessage(ss.str());

    // TODO: 实现进度系统

    return static_cast<i32>(playerIds.size());
}

i32 AdvancementCommand::revokeAdvancement(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");

    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No players matched the selector");
        return 0;
    }

    String advancement = "everything";
    if (context.hasArgument("advancement")) {
        advancement = context.getArgument<String>("advancement");
    }

    std::ostringstream ss;
    ss << "Revoked advancement '" << advancement << "' from " << playerIds.size() << " player(s)";
    source.sendMessage(ss.str());

    // TODO: 实现进度系统

    return static_cast<i32>(playerIds.size());
}

i32 AdvancementCommand::testAdvancement(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");
    const String advancement = context.getArgument<String>("advancement");

    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No players matched the selector");
        return 0;
    }

    // TODO: 实现进度系统
    if (playerIds.size() == 1) {
        auto player = source.server()->playerManager().getPlayer(playerIds[0]);
        if (player) {
            source.sendMessage(player->username + " has not completed advancement '" + advancement + "'");
        }
    } else {
        source.sendMessage("Testing advancement '" + advancement + "' for " + std::to_string(playerIds.size()) + " players");
    }

    return 1;
}

} // namespace command
} // namespace mc
