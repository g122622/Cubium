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

#include "ClearCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/ItemArgument.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/InventorySlotMapping.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/ItemStackBridge.hpp"
#include "common/network/ir/packets/play/ItemStackView.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/player/ServerPlayer.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
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

    // 用 ContainerSetContent(containerId=0) 同步完整玩家物品栏。
    // stateId 取自玩家数据（containerId=0 在服务端无独立 AbstractContainerMenu 实例）。
    mc::network::ir::play::ContainerSetContent pkt;
    pkt.containerId = 0; // 玩家物品栏
    const auto* playerData = server->playerManager().getPlayer(playerId);
    pkt.stateId = (playerData != nullptr) ? playerData->incrementPlayerInventoryStateId() : 0;
    // items 按 InventoryMenu 46 槽布局构造，对齐 Java 客户端 containerId=0 期望。
    pkt.items = mc::buildMenuContent(inventory);
    pkt.carriedItem = mc::network::ir::play::ItemStackView{0, 0, {}}; // 空 carried

    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    };
    (void)server->connectionManager().sendToPlayer(playerId, packet);
}

[[nodiscard]] i32 clearInventory(
    PlayerInventory& inventory, const ItemPredicateInput& predicate, std::optional<i32> maxCount)
{
    i32 remaining = maxCount.has_value() ? *maxCount : std::numeric_limits<i32>::max();
    i32 removedCount = 0;
    const i32 totalSlots = inventory.getContainerSize();

    for (i32 slot = 0; slot < totalSlots && remaining > 0; ++slot) {
        ItemStack stack = inventory.getItem(slot);
        if (stack.isEmpty()) {
            continue;
        }

        // 使用 ItemPredicateInput::test() 进行匹配，支持物品ID、标签和通配符
        if (!predicate.test(stack)) {
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

[[nodiscard]] std::string describeTargets(
    const ServerCommandSource& source, const std::vector<PlayerId>& targetPlayerIds)
{
    if (targetPlayerIds.size() == 1 && source.server() != nullptr) {
        const auto* playerData = source.server()->playerManager().getPlayer(targetPlayerIds.front());
        if (playerData != nullptr) {
            return playerData->username;
        }
    }

    return "target player(s)";
}

[[nodiscard]] i32 clearTargets(ServerCommandSource& source,
    const std::vector<PlayerId>& targetPlayerIds,
    const ItemPredicateInput& predicate,
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

        const i32 removedCount = clearInventory(*inventory, predicate, maxCount);
        if (removedCount <= 0) {
            continue;
        }

        removedTotal += removedCount;
        syncInventoryToClient(source, playerId, *inventory);
    }

    return removedTotal;
}

void sendClearMessage(ServerCommandSource& source,
    const std::vector<PlayerId>& targetPlayerIds,
    const ItemPredicateInput& predicate,
    std::optional<i32> maxCount,
    i32 removedCount)
{
    if (removedCount <= 0) {
        source.sendError("No matching items were found");
        return;
    }

    std::ostringstream ss;
    if (predicate.isAny()) {
        ss << "Cleared the inventory of ";
    } else {
        ss << "Cleared " << predicate.displayName();
        if (maxCount.has_value()) {
            ss << " (max " << *maxCount << ")";
        }
        ss << " from ";
    }

    ss << describeTargets(source, targetPlayerIds) << ", removing " << removedCount << " items";
    source.sendMessage(ss.str());
}

} // namespace

void ClearCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    using namespace mc::command;

    auto clearNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("clear");
    clearNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(clearNode,
        support::makeMetadata("Clear player inventories.", "/clear [player] [item] [maxCount]", 2, {}, false));

    clearNode->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _clearSelf(ctx); });

    auto playerArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "player", EntityArgumentType::players());
    playerArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _clearPlayer(ctx); });

    auto itemArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemPredicateInput>>(
        "item", ItemPredicateArgumentType::itemPredicate());
    itemArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _clearPlayerItem(ctx); });

    auto maxCountArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("maxCount", IntegerArgumentType::integer(0));
    maxCountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _clearPlayerItemCount(ctx); });

    itemArg->addChild(maxCountArg);
    playerArg->addChild(itemArg);
    clearNode->addChild(playerArg);

    dispatcher.registerCommand(clearNode);
}

i32 ClearCommand::_clearSelf(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    const PlayerId playerId = resolveSourcePlayerId(source);
    if (playerId == 0) {
        source.sendError("You must be a player to use this command");
        return 0;
    }

    const std::vector<PlayerId> targetPlayerIds{playerId};
    // /clear 不带参数时，清除所有物品（通配符谓词）
    const ItemPredicateInput anyPredicate;
    const i32 removedCount = clearTargets(source, targetPlayerIds, anyPredicate, std::nullopt);
    sendClearMessage(source, targetPlayerIds, anyPredicate, std::nullopt, removedCount);
    return removedCount;
}

i32 ClearCommand::_clearPlayer(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    const std::vector<PlayerId> targetPlayerIds = support::resolvePlayerIds(source, selector);
    if (targetPlayerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // /clear <player> 不带物品参数时，清除所有物品（通配符谓词）
    const ItemPredicateInput anyPredicate;
    const i32 removedCount = clearTargets(source, targetPlayerIds, anyPredicate, std::nullopt);
    sendClearMessage(source, targetPlayerIds, anyPredicate, std::nullopt, removedCount);
    return removedCount;
}

i32 ClearCommand::_clearPlayerItem(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    const std::vector<PlayerId> targetPlayerIds = support::resolvePlayerIds(source, selector);
    if (targetPlayerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // 使用 ItemPredicateArgumentType 支持物品ID、标签和通配符
    ItemPredicateInput predicate = context.getArgument<ItemPredicateInput>("item");

    const i32 removedCount = clearTargets(source, targetPlayerIds, predicate, std::nullopt);
    sendClearMessage(source, targetPlayerIds, predicate, std::nullopt, removedCount);
    return removedCount;
}

i32 ClearCommand::_clearPlayerItemCount(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();
    EntitySelector selector = context.getArgument<EntitySelector>("player");

    const std::vector<PlayerId> targetPlayerIds = support::resolvePlayerIds(source, selector);
    if (targetPlayerIds.empty()) {
        source.sendError("No matching players were found");
        return 0;
    }

    // 使用 ItemPredicateArgumentType 支持物品ID、标签和通配符
    ItemPredicateInput predicate = context.getArgument<ItemPredicateInput>("item");
    i32 maxCount = context.getArgument<i32>("maxCount");

    const i32 removedCount = clearTargets(source, targetPlayerIds, predicate, maxCount);
    sendClearMessage(source, targetPlayerIds, predicate, maxCount, removedCount);
    return removedCount;
}

} // namespace command
} // namespace mc