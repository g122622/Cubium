#include "EnchantCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/application/IServer.hpp"
#include "server/core/PlayerManager.hpp"
#include <sstream>

namespace mc {
namespace command {

void EnchantCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    auto enchantNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("enchant");
    enchantNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        enchantNode,
        support::makeMetadata(
            "Adds an enchantment to a player's held item.",
            "/enchant <player> <enchantment> [<level>]",
            2,
            {},
            true));

    // /enchant <player>
    auto playerNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::player()
    );

    // /enchant <player> <enchantment>
    auto enchantmentNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, String>>(
        "enchantment",
        StringArgumentType::word()
    );
    enchantmentNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return enchantItem(ctx);
    });

    // /enchant <player> <enchantment> <level>
    auto levelNode = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "level",
        IntegerArgumentType::integer(1, 10)
    );
    levelNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return enchantItem(ctx);
    });

    enchantmentNode->addChild(levelNode);
    playerNode->addChild(enchantmentNode);
    enchantNode->addChild(playerNode);
    dispatcher.registerCommand(enchantNode);
}

i32 EnchantCommand::enchantItem(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    auto& selector = context.getArgument<EntitySelector>("player");
    auto playerIds = support::resolvePlayerIds(source, selector);

    if (playerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    const String& enchantmentName = context.getArgument<String>("enchantment");
    i32 level = 1;

    if (context.hasArgument("level")) {
        level = context.getArgument<i32>("level");
    }

    auto* server = source.server();
    auto& playerManager = server->playerManager();
    i32 successCount = 0;

    for (PlayerId playerId : playerIds) {
        auto* playerData = playerManager.getPlayer(playerId);
        if (!playerData) {
            continue;
        }

        // TODO: 实现附魔逻辑
        // 1. 获取玩家手持物品
        // 2. 检查附魔是否有效
        // 3. 应用附魔
        // 需要访问 PlayerInventory 和 EnchantmentRegistry

        successCount++;
    }

    std::ostringstream ss;
    ss << "Applied " << enchantmentName << " level " << level;
    source.sendMessage(ss.str());

    return successCount;
}

} // namespace command
} // namespace mc
