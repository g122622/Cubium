#include "GiveCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/packet/InventoryPackets.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/player/ServerPlayer.hpp"

#include <algorithm>
#include <sstream>

namespace mc {
namespace command {

namespace {

/**
 * @brief 同步玩家背包到客户端
 */
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

/**
 * @brief 获取物品的最大堆叠数量
 */
[[nodiscard]] i32 getMaxStackSize(const Item* item)
{
    if (item == nullptr) {
        return 64;  // 默认最大堆叠数
    }
    return item->maxStackSize();
}

/**
 * @brief 给予玩家物品
 *
 * 参考 MC 1.16.5 GiveCommand.giveItem()
 * 逻辑：
 * 1. 遍历目标玩家列表
 * 2. 对每个玩家，按堆叠大小分批给予物品
 * 3. 如果背包满了，掉落在地上
 * 4. 播放拾取音效
 *
 * @return 成功给予物品的玩家数量
 */
[[nodiscard]] i32 giveItemToPlayers(
    ServerCommandSource& source,
    const std::vector<PlayerId>& targetPlayerIds,
    const Item* item,
    i32 count)
{
    if (item == nullptr || count <= 0) {
        return 0;
    }

    auto* server = source.server();
    if (server == nullptr) {
        return 0;
    }

    i32 successCount = 0;
    const i32 maxStackSize = getMaxStackSize(item);

    for (PlayerId playerId : targetPlayerIds) {
        if (playerId == 0) {
            continue;
        }

        PlayerInventory* inventory = server->playerInventory(playerId);
        if (inventory == nullptr) {
            continue;
        }

        i32 remaining = count;

        // 按堆叠大小分批给予物品
        while (remaining > 0) {
            const i32 stackSize = std::min(remaining, maxStackSize);

            // 创建物品堆
            ItemStack stack(item, stackSize);

            // 尝试添加到背包
            const i32 notAdded = inventory->add(stack);

            // 如果有剩余，说明背包满了
            if (notAdded > 0) {
                // TODO: 在玩家位置掉落物品实体
                // ItemEntity* droppedItem = player.dropItem(stack, false);
                // if (droppedItem != nullptr) {
                //     droppedItem->setNoPickupDelay();
                //     droppedItem->setOwnerId(player.getUniqueID());
                // }
                // 目前简化实现：剩余物品被丢弃
            }

            remaining -= (stackSize - notAdded);
        }

        // 同步背包到客户端
        syncInventoryToClient(source, playerId, *inventory);

        // TODO: 播放拾取音效
        // server->world().playSound(
        //     nullptr,
        //     player.x(), player.y(), player.z(),
        //     SoundEvents::ENTITY_ITEM_PICKUP,
        //     SoundCategory::PLAYERS,
        //     0.2f,
        //     (random.nextFloat() - random.nextFloat()) * 0.7f + 1.0f) * 2.0f
        // );

        successCount++;
    }

    return successCount;
}

/**
 * @brief 描述目标玩家
 */
[[nodiscard]] String describeTargets(const ServerCommandSource& source, const std::vector<PlayerId>& targetPlayerIds)
{
    if (targetPlayerIds.size() == 1 && source.server() != nullptr) {
        const auto* playerData = source.server()->playerManager().getPlayer(targetPlayerIds.front());
        if (playerData != nullptr) {
            return playerData->username;
        }
    }

    return std::to_string(targetPlayerIds.size()) + " players";
}

} // namespace

void GiveCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher) {
    using namespace mc::command;

    auto giveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("give");
    giveNode->setRequirement([](const ServerCommandSource& source) {
        return source.hasPermission(2);
    });
    support::applyMetadata(
        giveNode,
        support::makeMetadata(
            "Give items to players.",
            "/give <targets> <item> [<count>]",
            2,
            {},
            false));

    // /give <targets> <item> [count]
    auto targetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets",
        EntityArgumentType::players()
    );

    auto itemArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemInput>>(
        "item",
        ItemArgumentType::item()
    );
    itemArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return giveItem(ctx);
    });

    auto countArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>(
        "count",
        IntegerArgumentType::integer(1, 64)  // MC 1.16.5 限制为 1-64
    );
    countArg->setCommand([](CommandContext<ServerCommandSource>& ctx) {
        return giveItem(ctx);
    });

    itemArg->addChild(countArg);
    targetsArg->addChild(itemArg);
    giveNode->addChild(targetsArg);

    dispatcher.registerCommand(giveNode);
}

i32 GiveCommand::giveItem(CommandContext<ServerCommandSource>& context) {
    auto& source = context.getSource();

    // 获取目标玩家
    EntitySelector selector = context.getArgument<EntitySelector>("targets");
    std::vector<PlayerId> targetPlayerIds = support::resolvePlayerIds(source, selector);

    if (targetPlayerIds.empty()) {
        source.sendMessage("commands.give.failed.noPlayer");
        return 0;
    }

    // 获取物品
    ItemInput itemInput = context.getArgument<ItemInput>("item");
    if (!itemInput.isValid()) {
        source.sendMessage("commands.give.failed.invalidItem");
        return 0;
    }

    const Item* item = itemInput.getItem();
    if (item == nullptr) {
        source.sendMessage("commands.give.failed.invalidItem");
        return 0;
    }

    // 获取数量（默认为 1）
    i32 count = 1;
    if (context.hasArgument("count")) {
        count = context.getArgument<i32>("count");
    }

    // 给予物品
    const i32 successCount = giveItemToPlayers(source, targetPlayerIds, item, count);

    if (successCount <= 0) {
        source.sendMessage("commands.give.failed.noSpace");
        return 0;
    }

    // 发送反馈消息
    // MC 1.16.5 格式：
    // - 单个玩家: "Gave 64 [Stone] to Steve"
    // - 多个玩家: "Gave 64 [Stone] to 3 players"
    std::ostringstream ss;
    ss << "Gave " << count << " [" << item->getName() << "] to ";

    if (targetPlayerIds.size() == 1) {
        ss << describeTargets(source, targetPlayerIds);
    } else {
        ss << targetPlayerIds.size() << " players";
    }

    source.sendMessage(ss.str());

    return successCount;
}

} // namespace command
} // namespace mc
