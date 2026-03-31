#include "GiveCommand.hpp"
#include "common/command/CommandContext.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "server/player/ServerPlayer.hpp"
#include "common/item/core/ItemStack.hpp"
#include <sstream>

namespace mc {
namespace command {

void GiveCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    using namespace mc::command;

    auto giveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("give");
    giveNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });

    // /give <player> <item> [count]
    auto playerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::player()
    );

    auto itemArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemInput>>(
        "item",
        ItemArgumentType::item()
    );

    auto countArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "count",
        IntegerArgumentType::integer(1, 64)
    );
    countArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return giveItem(ctx);
    });

    // 没有count参数时默认给1个
    itemArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return giveItem(ctx);
    });

    itemArg->addChild(countArg);
    playerArg->addChild(itemArg);
    giveNode->addChild(playerArg);

    dispatcher.registerCommand(giveNode);
}

i32 GiveCommand::giveItem(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    // 获取目标玩家
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    // 获取物品
    ItemInput itemInput = context.getArgument<ItemInput>("item");

    // 获取数量（可选，默认1）
    i32 count = 1;
    if (context.hasArgument("count")) {
        count = context.getArgument<i32>("count");
    }

    // 验证物品
    if (!itemInput.isValid()) {
        source.sendMessage("Invalid item");
        return 0;
    }

    const Item* item = itemInput.getItem();
    if (!item) {
        source.sendMessage("Unknown item");
        return 0;
    }

    // TODO: 解析选择器获取玩家
    // ServerPlayer* player = resolvePlayerSelector(selector, source);

    // TODO: 给予物品
    // auto stack = itemInput.createStack(count);
    // player->giveItem(stack);

    std::ostringstream ss;
    ss << "Gave " << count << " [" << item->getName() << "] to player";
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
