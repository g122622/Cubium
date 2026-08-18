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

#include "GiveCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/ItemArgument.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/InventorySlotMapping.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/ItemStackBridge.hpp"
#include "common/network/ir/packets/play/ItemStackView.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "server/application/IServer.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/network/PacketBuilders.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

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

/**
 * @brief 获取物品的最大堆叠数量
 */
[[nodiscard]] i32 getMaxStackSize(const Item* item)
{
    if (item == nullptr) {
        return mc::item::DEFAULT_MAX_STACK_SIZE;
    }
    return item->maxStackSize();
}

/**
 * @brief 给予玩家物品
 *
 * 逻辑：
 * 1. 遍历目标玩家列表
 * 2. 对每个玩家，按堆叠大小分批给予物品
 * 3. 如果背包满了，掉落在地上
 * 4. 播放拾取音效
 *
 * @return 成功给予物品的玩家数量
 */
[[nodiscard]] i32 giveItemToPlayers(
    ServerCommandSource& source, const std::vector<PlayerId>& targetPlayerIds, const Item* item, i32 count)
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

        // 获取玩家实体用于位置和音效
        server::ServerWorld* playerWorld = server->getPlayerWorld(playerId);
        Player* player = playerWorld ? server->playerEntityManager().getPlayerEntity(playerId, *playerWorld) : nullptr;
        if (player == nullptr) {
            continue;
        }

        i32 remaining = count;
        i32 totalAdded = 0;

        // 按堆叠大小分批给予物品
        while (remaining > 0) {
            const i32 stackSize = std::min(remaining, maxStackSize);

            // 创建物品堆
            ItemStack stack(item, stackSize);

            // 尝试添加到背包
            const i32 notAdded = inventory->add(stack);

            const i32 added = stackSize - notAdded;
            totalAdded += added;

            // 如果有剩余，说明背包满了，掉落在地上
            if (notAdded > 0) {
                ItemStack dropStack(item, notAdded);
                // 在玩家位置掉落物品，设置 noPickupDelay 和 owner
                math::Random rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()));
                ItemEntity* droppedItem = ItemDropHelper::spawnItemEntity(playerWorld,
                    dropStack,
                    player->x(),
                    player->y() + 0.5,
                    player->z(),
                    rng,
                    0,             // noPickupDelay - 立即可拾取
                    player->uuid() // owner UUID
                );
                (void)droppedItem; // 避免未使用警告
            }

            remaining -= added;
        }

        // 如果有物品被添加，同步背包并播放音效
        if (totalAdded > 0) {
            // 同步背包到客户端
            syncInventoryToClient(source, playerId, *inventory);

            // 播放拾取音效（批5b：经 buildPlaySoundIr + connectionManager 投递，
            // 原 IServer::sendSoundToPlayer 纯虚已删）
            math::Random rng(static_cast<u64>(playerId) * static_cast<u64>(count));
            const f32 pitch = (rng.nextFloat() - rng.nextFloat()) * 0.7f + 1.0f;
            server->connectionManager().sendToPlayer(playerId,
                mc::server::net::buildPlaySoundIr(SoundEvents::ENTITY_ITEM_PICKUP,
                    sound::SoundCategory::Players,
                    Vector3(player->x(), player->y(), player->z()),
                    0.2f,
                    pitch * 2.0f));
        }

        successCount++;
    }

    return successCount;
}

/**
 * @brief 描述目标玩家
 */
[[nodiscard]] std::string describeTargets(
    const ServerCommandSource& source, const std::vector<PlayerId>& targetPlayerIds)
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

void GiveCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    using namespace mc::command;

    auto giveNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("give");
    giveNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(
        giveNode, support::makeMetadata("Give items to players.", "/give <targets> <item> [<count>]", 2, {}, false));

    // /give <targets> <item> [count]
    auto targetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::players());

    auto itemArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemInput>>("item", ItemArgumentType::item());
    itemArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _giveItem(ctx); });

    auto countArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("count", IntegerArgumentType::integer(1, 64));
    countArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _giveItem(ctx); });

    itemArg->addChild(countArg);
    targetsArg->addChild(itemArg);
    giveNode->addChild(targetsArg);

    dispatcher.registerCommand(giveNode);
}

i32 GiveCommand::_giveItem(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // 获取目标玩家
    EntitySelector selector = context.getArgument<EntitySelector>("targets");
    std::vector<PlayerId> targetPlayerIds = support::resolvePlayerIds(source, selector);

    if (targetPlayerIds.empty()) {
        source.sendError("commands.give.failed.noPlayer");
        return 0;
    }

    // 获取物品
    ItemInput itemInput = context.getArgument<ItemInput>("item");
    if (!itemInput.isValid()) {
        source.sendError("commands.give.failed.invalidItem");
        return 0;
    }

    const Item* item = itemInput.getItem();
    if (item == nullptr) {
        source.sendError("commands.give.failed.invalidItem");
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
        source.sendError("commands.give.failed.noSpace");
        return 0;
    }

    // 发送反馈消息
    // 格式：
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
