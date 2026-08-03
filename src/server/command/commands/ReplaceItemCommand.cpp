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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ReplaceItemCommand.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/arguments/ItemArgument.hpp"
#include "common/command/arguments/ItemSlotArgument.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/AbstractContainerMenu.hpp"
#include "common/entity/inventory/PlayerEnderChestInventory.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/ir/ItemStackBridge.hpp"
#include "common/network/ir/packets/play/ItemStackView.hpp"
#include "common/network/ir/packets/play/PlayPackets.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/ContainerBlockEntity.hpp"
#include "server/application/IServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/CommandMetadata.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/ServerPlayerData.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

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
    const i32 totalSlots = inventory.getContainerSize();
    pkt.items.reserve(static_cast<size_t>(totalSlots));
    for (i32 slot = 0; slot < totalSlots; ++slot) {
        pkt.items.push_back(mc::network::ir::toItemStackView(inventory.getItem(slot)));
    }
    pkt.carriedItem = mc::network::ir::play::ItemStackView{0, 0, {}}; // 空 carried

    mc::network::ir::IrPacket packet{
        mc::network::protocol::ConnectionProtocol::Play,
        mc::network::ir::PlayPacket{std::move(pkt)},
    };
    (void)server->connectionManager().sendToPlayer(playerId, packet);
}

/**
 * @brief 在实体物品栏中设置指定槽位的物品
 *
 * 处理玩家背包槽位、装备槽位、末影箱槽位和马匹槽位的映射，以及槽位有效性验证。
 *
 * @return 是否设置成功
 */
bool setEntitySlotItem(ServerCommandSource& source, PlayerId playerId, const ItemSlot& slot, const ItemStack& stack)
{
    auto* server = source.server();
    if (server == nullptr) {
        return false;
    }

    PlayerInventory* inventory = server->playerInventory(playerId);
    if (inventory == nullptr) {
        return false;
    }

    // 装备槽位 (98-106)：需要映射到玩家背包的装备区
    if (slot.isEquipmentSlot()) {
        // weapon.mainhand (98) 需要获取当前选中的快捷栏槽位
        i32 selectedSlot = inventory->getSelectedSlot();
        i32 invSlot = slot.toPlayerInventorySlot(selectedSlot);
        if (invSlot < 0 || invSlot >= InventorySlots::TOTAL_SIZE) {
            return false;
        }
        inventory->setItem(invSlot, stack);
        return true;
    }

    // 玩家背包槽位 (0-40)：直接设置
    if (slot.isPlayerInventorySlot()) {
        i32 invSlot = slot.slotIndex();
        if (invSlot < 0 || invSlot >= InventorySlots::TOTAL_SIZE) {
            return false;
        }
        inventory->setItem(invSlot, stack);
        return true;
    }

    // 末影箱槽位 (200-226)：通过 Player 实体访问末影箱物品栏
    if (slot.isEnderChestSlot()) {
        auto* world = source.world();
        if (world == nullptr) {
            source.sendError("No world available for ender chest access");
            return false;
        }
        auto* playerEntity = server->playerEntityManager().getPlayerEntity(playerId, *world);
        if (playerEntity == nullptr) {
            source.sendError("Player entity not found for ender chest access");
            return false;
        }
        auto& enderChest = playerEntity->enderChestInventory();
        i32 enderSlot = slot.toEnderChestSlot();
        if (enderSlot < 0 || enderSlot >= enderChest.getContainerSize()) {
            source.sendError("Ender chest slot index out of range");
            return false;
        }
        enderChest.setItem(enderSlot, stack);
        return true;
    }

    // 马匹槽位 (500-514)：需要解析目标马匹实体
    // TODO: 当前 /replaceitem entity 仅支持玩家选择器，马匹槽位需要实体选择器支持
    if (slot.isHorseSlot()) {
        source.sendError("Horse inventory slots require entity targeting (not yet supported in player-only mode)");
        return false;
    }

    // 玩家光标槽位 (499)：访问当前打开的容器菜单的 carried item，
    // 若无打开的容器菜单则使用 PlayerInventory 的 carried item。
    // 注意：player.cursor (499) 与 horse.chest (499) 编号重叠，
    // 当前 /replaceitem entity 仅支持玩家选择器，因此优先匹配光标槽位。
    // isHorseChestSlot() 也对 499 返回 true，但在玩家上下文中光标槽位优先。
    if (slot.isCursorSlot()) {
        auto* world = source.world();
        if (world == nullptr) {
            source.sendError("No world available for cursor slot access");
            return false;
        }
        auto* playerEntity = server->playerEntityManager().getPlayerEntity(playerId, *world);
        if (playerEntity == nullptr) {
            source.sendError("Player entity not found for cursor slot access");
            return false;
        }

        // 优先使用当前打开的容器菜单的 carried item（与 MC 原版 containerMenu.getCarried() 一致）
        auto* openMenu = playerEntity->openContainerMenu();
        if (openMenu != nullptr) {
            openMenu->setCarriedItem(stack);
        } else {
            // 无打开的容器菜单时，使用 PlayerInventory 的 carried item
            playerEntity->inventory().setCarriedItem(stack);
        }
        return true;
    }

    // 合成槽位 (500-503)：需要访问玩家的合成容器
    // TODO: 当永久合成菜单（InventoryCraftingMenu）集成到 Player 后实现
    if (slot.isCraftingSlot()) {
        source.sendError("Crafting slots are not yet supported");
        return false;
    }

    // 村民槽位 (300-307)：需要访问村民交易界面
    // TODO: 当村民交易系统集成后实现
    if (slot.isVillagerSlot()) {
        source.sendError("Villager trade slots are not yet supported");
        return false;
    }

    // 未知槽位
    return false;
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

/**
 * @brief 描述槽位名称（用于反馈消息）
 */
[[nodiscard]] std::string describeSlot(const ItemSlot& slot)
{
    i32 idx = slot.slotIndex();
    // 常用槽位提供友好名称
    switch (idx) {
        case 98:
            return "weapon.mainhand";
        case 99:
            return "weapon.offhand";
        case 100:
            return "armor.head";
        case 101:
            return "armor.chest";
        case 102:
            return "armor.legs";
        case 103:
            return "armor.feet";
        case 105:
            return "armor.body";
        case 106:
            return "saddle";
        default:
            break;
    }
    // 末影箱槽位
    if (slot.isEnderChestSlot()) {
        return "enderchest." + std::to_string(idx - 200);
    }
    // 马匹槽位
    if (slot.isHorseSlot()) {
        return "horse." + std::to_string(idx - 500);
    }
    // 马匹箱子槽位
    if (slot.isHorseChestSlot()) {
        return "horse.chest";
    }
    // 合成槽位
    if (slot.isCraftingSlot()) {
        return "player.crafting." + std::to_string(idx - 500);
    }
    // 村民槽位
    if (slot.isVillagerSlot()) {
        return "villager." + std::to_string(idx - 300);
    }
    return std::to_string(idx);
}

/**
 * @brief 在方块容器中设置指定槽位的物品
 *
 * @return 是否设置成功
 */
bool setBlockSlotItem(
    server::ServerWorld* world, const BlockPos& pos, i32 slotIndex, const ItemStack& stack, ServerCommandSource& source)
{
    if (world == nullptr) {
        return false;
    }

    BlockEntity* blockEntity = world->getBlockEntity(pos);
    if (blockEntity == nullptr) {
        source.sendError("No block entity at the specified position");
        return false;
    }

    auto* container = dynamic_cast<ContainerBlockEntity*>(blockEntity);
    if (container == nullptr) {
        source.sendError("The block at the specified position is not a container");
        return false;
    }

    IInventory* inventory = container->getInventory();
    if (inventory == nullptr) {
        source.sendError("The container has no inventory");
        return false;
    }

    // 验证槽位范围
    if (slotIndex < 0 || slotIndex >= inventory->getContainerSize()) {
        std::ostringstream ss;
        ss << "Slot " << slotIndex << " is out of range (0-" << (inventory->getContainerSize() - 1) << ")";
        source.sendError(ss.str());
        return false;
    }

    inventory->setItem(slotIndex, stack);
    inventory->setChanged();
    return true;
}

} // namespace

void ReplaceItemCommand::registerTo(CommandDispatcher<ServerCommandSource>& dispatcher)
{
    auto replaceitemNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("replaceitem");
    replaceitemNode->setRequirement([](const ServerCommandSource& source) { return source.hasPermission(2); });
    support::applyMetadata(replaceitemNode,
        support::makeMetadata("Replaces items in an inventory.",
            "/replaceitem <entity|block> <target> <slot> <item> [count]",
            2,
            {},
            true));

    // /replaceitem entity <targets> <slot> <item> [count]
    auto entityNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("entity");
    auto targetsArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, EntitySelector>>(
        "targets", EntityArgumentType::players());
    auto slotArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemSlot>>("slot", ItemSlotArgumentType::itemSlot());
    auto itemArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemInput>>("item", ItemArgumentType::item());
    auto countArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("count", IntegerArgumentType::integer(1, 99));
    countArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _replaceEntityItem(ctx); });
    itemArg->addChild(countArg);
    itemArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _replaceEntityItem(ctx); });
    slotArg->addChild(itemArg);
    targetsArg->addChild(slotArg);
    entityNode->addChild(targetsArg);
    replaceitemNode->addChild(entityNode);

    // /replaceitem block <pos> <slot> <item> [count]
    auto blockNode = std::make_shared<LiteralCommandNode<ServerCommandSource>>("block");
    auto posArg = std::make_shared<ArgumentCommandNode<ServerCommandSource, Coordinates::Ptr>>(
        "pos", BlockPosArgumentType::blockPos());
    auto blockSlotArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemSlot>>("slot", ItemSlotArgumentType::itemSlot());
    auto blockItemArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, ItemInput>>("item", ItemArgumentType::item());
    auto blockCountArg =
        std::make_shared<ArgumentCommandNode<ServerCommandSource, i32>>("count", IntegerArgumentType::integer(1, 99));
    blockCountArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _replaceBlockItem(ctx); });
    blockItemArg->addChild(blockCountArg);
    blockItemArg->setCommand([](CommandContext<ServerCommandSource>& ctx) { return _replaceBlockItem(ctx); });
    blockSlotArg->addChild(blockItemArg);
    posArg->addChild(blockSlotArg);
    blockNode->addChild(posArg);
    replaceitemNode->addChild(blockNode);

    dispatcher.registerCommand(replaceitemNode);
}

i32 ReplaceItemCommand::_replaceEntityItem(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // 解析参数
    const EntitySelector& selector = context.getArgument<EntitySelector>("targets");
    const ItemSlot slot = context.getArgument<ItemSlot>("slot");
    const ItemInput itemInput = context.getArgument<ItemInput>("item");

    // 验证物品
    if (!itemInput.isValid()) {
        source.sendError("Invalid item");
        return 0;
    }
    const Item* item = itemInput.getItem();
    if (item == nullptr) {
        source.sendError("Unknown item");
        return 0;
    }

    // 获取数量（默认为 1）
    i32 count = 1;
    if (context.hasArgument("count")) {
        count = context.getArgument<i32>("count");
    }

    // 限制数量不超过物品最大堆叠
    const i32 maxStack = item->maxStackSize();
    if (count > maxStack) {
        count = maxStack;
    }

    // 解析目标玩家
    auto playerIds = support::resolvePlayerIds(source, selector);
    if (playerIds.empty()) {
        source.sendError("No entities matched the selector");
        return 0;
    }

    // 验证槽位有效性
    if (!slot.isValid()) {
        source.sendError("Invalid slot");
        return 0;
    }

    // 检查槽位类型是否支持实体操作
    if (!slot.isPlayerInventorySlot() && !slot.isEquipmentSlot() && !slot.isEnderChestSlot() && !slot.isCursorSlot()) {
        if (slot.isHorseSlot()) {
            source.sendError("Horse inventory slots require entity targeting with @e selector");
            return 0;
        }
        if (slot.isHorseChestSlot()) {
            source.sendError("Horse chest slot requires entity targeting with @e selector");
            return 0;
        }
        if (slot.isCraftingSlot()) {
            source.sendError("Crafting slots are not yet supported for entity replacement");
            return 0;
        }
        if (slot.isVillagerSlot()) {
            source.sendError("Villager trade slots are not yet supported");
            return 0;
        }
        source.sendError("Slot " + describeSlot(slot) + " is not valid for entities");
        return 0;
    }

    i32 successCount = 0;
    for (PlayerId playerId : playerIds) {
        if (playerId == 0) {
            continue;
        }

        // 获取玩家背包
        auto* server = source.server();
        if (server == nullptr) {
            continue;
        }

        PlayerInventory* inventory = server->playerInventory(playerId);
        if (inventory == nullptr) {
            continue;
        }

        // 创建物品堆
        ItemStack stack(item, count);

        // 设置物品到指定槽位
        bool success = setEntitySlotItem(source, playerId, slot, stack);
        if (!success) {
            continue;
        }

        // 同步背包到客户端
        syncInventoryToClient(source, playerId, *inventory);
        successCount++;
    }

    // 发送反馈消息
    if (successCount <= 0) {
        source.sendError("No items were replaced");
        return 0;
    }

    std::ostringstream ss;
    ss << "Replaced slot " << describeSlot(slot) << " with " << count << "x " << item->getName() << " for "
       << describeTargets(source, playerIds);
    source.sendMessage(ss.str());

    return successCount;
}

i32 ReplaceItemCommand::_replaceBlockItem(CommandContext<ServerCommandSource>& context)
{
    auto& source = context.getSource();

    // 解析参数
    const Vector3i blockPos = BlockPosArgumentType::getBlockPos(context, "pos", source);
    const ItemSlot slot = context.getArgument<ItemSlot>("slot");
    const ItemInput itemInput = context.getArgument<ItemInput>("item");

    // 验证物品
    if (!itemInput.isValid()) {
        source.sendError("Invalid item");
        return 0;
    }
    const Item* item = itemInput.getItem();
    if (item == nullptr) {
        source.sendError("Unknown item");
        return 0;
    }

    // 获取数量（默认为 1）
    i32 count = 1;
    if (context.hasArgument("count")) {
        count = context.getArgument<i32>("count");
    }

    // 限制数量不超过物品最大堆叠
    const i32 maxStack = item->maxStackSize();
    if (count > maxStack) {
        count = maxStack;
    }

    // 验证槽位有效性
    if (!slot.isValid()) {
        source.sendError("Invalid slot");
        return 0;
    }

    // 对于方块容器，只接受 container.N 格式或纯数字格式的槽位
    i32 slotIndex = slot.slotIndex();
    // 允许的槽位范围：0-53 (container slots), 0-40 也可以（如果槽位编号小于容器大小）
    // 装备槽位、末影箱槽位、马匹槽位、合成槽位、光标槽位、村民槽位不适用于方块容器
    if (slot.isEquipmentSlot() || slot.isEnderChestSlot() || slot.isHorseSlot() || slot.isHorseChestSlot() ||
        slot.isCraftingSlot() || slot.isCursorSlot() || slot.isVillagerSlot()) {
        source.sendError("Slot " + describeSlot(slot) + " is not valid for block containers");
        return 0;
    }

    // 槽位索引必须是非负整数
    if (slotIndex < 0) {
        source.sendError("Invalid slot index for block container");
        return 0;
    }

    // 获取世界和方块实体
    server::ServerWorld* world = source.world();
    if (world == nullptr) {
        source.sendError("No world available");
        return 0;
    }

    BlockPos pos(blockPos.x, blockPos.y, blockPos.z);

    // 创建物品堆
    ItemStack stack(item, count);

    // 设置物品到方块容器
    bool success = setBlockSlotItem(world, pos, slotIndex, stack, source);
    if (!success) {
        return 0;
    }

    // 发送反馈消息
    std::ostringstream ss;
    ss << "Replaced slot " << slotIndex << " with " << count << "x " << item->getName() << " at (" << blockPos.x << ", "
       << blockPos.y << ", " << blockPos.z << ")";
    source.sendMessage(ss.str());

    return 1;
}

} // namespace command
} // namespace mc
