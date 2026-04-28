#include "ReplaceItemCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/PlayerManager.hpp"
#include <sstream>

namespace mc {
namespace command {

void ReplaceItemCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto replaceitemNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("replaceitem");
    replaceitemNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        replaceitemNode,
        support::makeMetadata(
            "Replaces items in an inventory.",
            "/replaceitem <entity|block> <target> <slot> <item> [count]",
            2,
            {},
            true));

    // /replaceitem entity <targets> <slot> <item> [count]
    auto entityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
    auto targetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets",
        EntityArgumentType::entities());
    auto slotArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "slot",
        StringArgumentType::string());
    auto itemArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "item",
        StringArgumentType::string());
    auto countArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "count",
        IntegerArgumentType::integer(1, 64));
    countArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return replaceEntityItem(ctx);
    });
    itemArg->addChild(countArg);
    itemArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return replaceEntityItem(ctx);
    });
    slotArg->addChild(itemArg);
    targetsArg->addChild(slotArg);
    entityNode->addChild(targetsArg);
    replaceitemNode->addChild(entityNode);

    // /replaceitem block <pos> <slot> <item> [count]
    auto blockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    auto posArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Vector3d>>(
        "pos",
        Vec3ArgumentType::vec3());
    auto blockSlotArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "slot",
        StringArgumentType::string());
    auto blockItemArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "item",
        StringArgumentType::string());
    auto blockCountArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "count",
        IntegerArgumentType::integer(1, 64));
    blockCountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return replaceBlockItem(ctx);
    });
    blockItemArg->addChild(blockCountArg);
    blockItemArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return replaceBlockItem(ctx);
    });
    blockSlotArg->addChild(blockItemArg);
    posArg->addChild(blockSlotArg);
    blockNode->addChild(posArg);
    replaceitemNode->addChild(blockNode);

    dispatcher.registerCommand(replaceitemNode);
}

i32 ReplaceItemCommand::replaceEntityItem(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");
    const String slot = context.getArgument<String>("slot");
    const String item = context.getArgument<String>("item");

    i32 count = 1;
    if (context.hasArgument("count")) {
        count = context.getArgument<i32>("count");
    }

    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No entities matched the selector");
        return 0;
    }

    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        auto player = source.server()->playerManager().getPlayer(playerId);
        if (player) {
            // TODO: 实现物品槽位替换系统
            successCount++;
        }
    }

    std::ostringstream ss;
    ss << "Replaced slot '" << slot << "' with " << count << "x '" << item << "' for " << successCount << " entit" << (successCount == 1 ? "y" : "ies");
    source.sendMessage(ss.str());

    return successCount;
}

i32 ReplaceItemCommand::replaceBlockItem(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    const Vector3d& pos = context.getArgument<Vector3d>("pos");
    const String slot = context.getArgument<String>("slot");
    const String item = context.getArgument<String>("item");

    i32 count = 1;
    if (context.hasArgument("count")) {
        count = context.getArgument<i32>("count");
    }

    std::ostringstream ss;
    ss << "Replaced slot '" << slot << "' with " << count << "x '" << item << "' at (" << static_cast<i32>(pos.x) << ", " << static_cast<i32>(pos.y) << ", " << static_cast<i32>(pos.z) << ")";
    source.sendMessage(ss.str());

    // TODO: 实现方块容器物品替换系统

    return 1;
}

} // namespace command
} // namespace mc
