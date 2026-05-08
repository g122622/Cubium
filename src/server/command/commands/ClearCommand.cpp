#include "ClearCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/ItemArgument.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "server/application/IServer.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/player/ServerPlayer.hpp"

#include <algorithm>
#include <limits>
#include <optional>
#include <sstream>
#include <vector>

namespace mc {
namespace command {

namespace {

[[nodiscard]] PlayerId resolveSourcePlayerId(const ServerCommandSource& source)
{
    if (const auto* player = source.player()) {
        return player->playerId();
    }

    return source.playerId();
}

[[nodiscard]] PlayerInventory* resolveInventory(ServerCommandSource& source, PlayerId playerId)
{
    auto* server = source.server();
    if (server == nullptr) {
        return nullptr;
    }

    return server->playerInventory(playerId);
}

void syncInventoryToClient(ServerCommandSource& source, PlayerId playerId, const PlayerInventory& inventory)
{
    auto* server = source.server();
    if (server == nullptr) {
        return;
    }

    PlayerInventoryPacket packet(inventory);
    network::PacketSerializer payload;
    packet.serialize(payload);

    (void)server->connectionManager().sendPacketToPlayer(
        playerId,
        network::PacketType::PlayerInventory,
        payload.buffer());
}

[[nodiscard]] i32 clearInventory(PlayerInventory& inventory, const Item* item, std::optional<i32> maxCount)
{
    i32 remaining = maxCount.has_value() ? *maxCount : std::numeric_limits<i32>::max();
    i32 removedCount = 0;
    const i32 totalSlots = inventory.getContainerSize();

    for (i32 slot = 0; slot < totalSlots && remaining > 0; ++slot) {
        ItemStack stack = inventory.getItem(slot);
        if (stack.isEmpty()) {
            continue;
        }

        if (item != nullptr && stack.getItem() != item) {
            continue;
        }

        const i32 toRemove = std::min(remaining, stack.getCount());
        if (toRemove <= 0) {
            continue;
        }

        ItemStack removedStack = inventory.removeItem(slot, toRemove);
        removedCount += removedStack.getCount();
        remaining -= removedStack.getCount();
    }

    return removedCount;
}

[[nodiscard]] std::string describeTargets(const ServerCommandSource& source, const std::vector<PlayerId>& targetPlayerIds)
{
    if (targetPlayerIds.size() == 1 && source.server() != nullptr) {
        const auto* playerData = source.server()->playerManager().getPlayer(targetPlayerIds.front());
        if (playerData != nullptr) {
            return playerData->username;
        }
    }

    return "target player(s)";
}

[[nodiscard]] i32 clearTargets(
    ServerCommandSource& source,
    const std::vector<PlayerId>& targetPlayerIds,
    const Item* item,
    std::optional<i32> maxCount)
{
    i32 removedTotal = 0;

    for (PlayerId playerId : targetPlayerIds) {
        if (playerId == 0) {
            continue;
        }

        PlayerInventory* inventory = resolveInventory(source, playerId);
        if (inventory == nullptr) {
            continue;
        }

        const i32 removedCount = clearInventory(*inventory, item, maxCount);
        if (removedCount <= 0) {
            continue;
        }

        removedTotal += removedCount;
        syncInventoryToClient(source, playerId, *inventory);
    }

    return removedTotal;
}

void sendClearMessage(
    ServerCommandSource& source,
    const std::vector<PlayerId>& targetPlayerIds,
    const Item* item,
    std::optional<i32> maxCount,
    i32 removedCount)
{
    if (removedCount <= 0) {
        source.sendError("No matching items were found");
        return;
    }

    std::ostringstream ss;
    if (item == nullptr) {
        ss << "Cleared the inventory of ";
    } else {
        ss << "Cleared " << item->getName();
        if (maxCount.has_value()) {
            ss << " (max " << *maxCount << ")";
        }
        ss << " from ";
    }

    ss << describeTargets(source, targetPlayerIds)
       << ", removing " << removedCount << " items";
    source.sendMessage(ss.str());
}

} // namespace

void ClearCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    using namespace mc::command;

    auto clearNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("clear");
    clearNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        clearNode,
        support::makeMetadata(
            "Clear player inventories.",
            "/clear [player] [item] [maxCount]",
            2,
            {},
            false));

    clearNode->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return clearSelf(ctx);
    });

    auto playerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player",
        EntityArgumentType::players()
    );
    playerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return clearPlayer(ctx);
    });

    auto itemArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemInput>>(
        "item",
        ItemArgumentType::item()
    );
    itemArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return clearPlayerItem(ctx);
    });

    auto maxCountArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "maxCount",
        IntegerArgumentType::integer(0)
    );
    maxCountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return clearPlayerItemCount(ctx);
    });

    itemArg->addChild(maxCountArg);
    playerArg->addChild(itemArg);
    clearNode->addChild(playerArg);

    dispatcher.registerCommand(clearNode);
}

i32 ClearCommand::clearSelf(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    const PlayerId playerId = resolveSourcePlayerId(source);
    if (playerId == 0) {
        source.sendError("You must be a player to use this command");
        return 0;
    }

    const std::vector<PlayerId> targetPlayerIds{playerId};
    const i32 removedCount = clearTargets(source, targetPlayerIds, nullptr, std::nullopt);
    sendClearMessage(source, targetPlayerIds, nullptr, std::nullopt, removedCount);
    return removedCount;
}

i32 ClearCommand::clearPlayer(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    const std::vector<PlayerId> targetPlayerIds = support::resolvePlayerIds(source, selector);
    if (targetPlayerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    const i32 removedCount = clearTargets(source, targetPlayerIds, nullptr, std::nullopt);
    sendClearMessage(source, targetPlayerIds, nullptr, std::nullopt, removedCount);
    return removedCount;
}

i32 ClearCommand::clearPlayerItem(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    const std::vector<PlayerId> targetPlayerIds = support::resolvePlayerIds(source, selector);
    if (targetPlayerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    ItemInput itemInput = context.getArgument<ItemInput>("item");

    if (!itemInput.isValid()) {
        source.sendError("Invalid item");
        return 0;
    }

    const Item* item = itemInput.getItem();
    if (!item) {
        source.sendError("Unknown item");
        return 0;
    }

    const i32 removedCount = clearTargets(source, targetPlayerIds, item, std::nullopt);
    sendClearMessage(source, targetPlayerIds, item, std::nullopt, removedCount);
    return removedCount;
}

i32 ClearCommand::clearPlayerItemCount(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    const std::vector<PlayerId> targetPlayerIds = support::resolvePlayerIds(source, selector);
    if (targetPlayerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    ItemInput itemInput = context.getArgument<ItemInput>("item");
    i32 maxCount = context.getArgument<i32>("maxCount");

    if (!itemInput.isValid()) {
        source.sendError("Invalid item");
        return 0;
    }

    const Item* item = itemInput.getItem();
    if (!item) {
        source.sendError("Unknown item");
        return 0;
    }

    const i32 removedCount = clearTargets(source, targetPlayerIds, item, maxCount);
    sendClearMessage(source, targetPlayerIds, item, maxCount, removedCount);
    return removedCount;
}

} // namespace command
} // namespace mc