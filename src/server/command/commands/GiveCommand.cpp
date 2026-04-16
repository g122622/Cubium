#include "GiveCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/item/core/ItemStack.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/player/ServerPlayer.hpp"

#include <sstream>

namespace mc {
namespace command {

void GiveCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    using namespace mc::command;

    auto giveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("give");
    giveNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        giveNode,
        support::makeMetadata(
            "Give an item to players.",
            "/give <player> <item> [count]",
            2,
            {},
            false));

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
    EntitySelector selector = context.getArgument<EntitySelector>("player");
    (void)selector;

    ItemInput itemInput = context.getArgument<ItemInput>("item");

    i32 count = 1;
    if (context.hasArgument("count")) {
        count = context.getArgument<i32>("count");
    }

    if (!itemInput.isValid()) {
        source.sendMessage("Invalid item");
        return 0;
    }

    const Item* item = itemInput.getItem();
    if (!item) {
        source.sendMessage("Unknown item");
        return 0;
    }

    std::ostringstream ss;
    ss << "Gave " << count << " [" << item->getName() << "] to player";
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc