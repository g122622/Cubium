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

#include "ItemSlotAccess.hpp"

#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/passive/horse/AbstractHorseEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/entity/inventory/AbstractContainerMenu.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/entity/inventory/InventorySlotMapping.hpp"
#include "common/entity/inventory/PlayerEnderChestInventory.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/ContainerBlockEntity.hpp"
#include "server/application/IServer.hpp"
#include "server/command/support/PlayerResolver.hpp"
#include "server/interaction/InventoryManager.hpp"
#include "server/player/ServerPlayer.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/player/ServerPlayerEntityManager.hpp"

namespace mc::command::support {

namespace {

/// 按槽位编号取玩家实体（光标/末影箱槽位需要）。
Player* _playerEntity(ServerCommandSource& source, PlayerId playerId)
{
    auto* server = source.server();
    auto* world = source.world();
    if (server == nullptr || world == nullptr) {
        return nullptr;
    }
    return server->playerEntityManager().getPlayerEntity(playerId, *world);
}

/// 装备槽位映射到玩家背包内的实际下标。
bool _equipmentInventorySlot(PlayerInventory& inventory, const ItemSlot& slot, i32& outSlot)
{
    const i32 invSlot = slot.toPlayerInventorySlot(inventory.getSelectedSlot());
    if (invSlot < 0 || invSlot >= InventorySlots::TOTAL_SIZE) {
        return false;
    }
    outSlot = invSlot;
    return true;
}

} // namespace

void syncPlayerInventoryToClient(ServerCommandSource& source, PlayerId playerId)
{
    auto* server = source.server();
    if (server == nullptr) {
        return;
    }
    server->inventoryManager().syncToClient(playerId);
}

bool getPlayerSlotItem(ServerCommandSource& source, PlayerId playerId, const ItemSlot& slot, ItemStack& outItem)
{
    PlayerInventory* inventory = resolvePlayerInventory(source, playerId);
    if (inventory == nullptr) {
        return false;
    }

    if (slot.isEquipmentSlot()) {
        i32 invSlot = 0;
        if (!_equipmentInventorySlot(*inventory, slot, invSlot)) {
            return false;
        }
        outItem = inventory->getItem(invSlot);
        return true;
    }

    if (slot.isPlayerInventorySlot()) {
        const i32 invSlot = slot.slotIndex();
        if (invSlot < 0 || invSlot >= InventorySlots::TOTAL_SIZE) {
            return false;
        }
        outItem = inventory->getItem(invSlot);
        return true;
    }

    if (slot.isEnderChestSlot()) {
        Player* player = _playerEntity(source, playerId);
        if (player == nullptr) {
            return false;
        }
        const i32 enderSlot = slot.toEnderChestSlot();
        if (enderSlot < 0 || enderSlot >= player->enderChestInventory().getContainerSize()) {
            return false;
        }
        outItem = player->enderChestInventory().getItem(enderSlot);
        return true;
    }

    if (slot.isCursorSlot()) {
        Player* player = _playerEntity(source, playerId);
        if (player == nullptr) {
            return false;
        }
        if (auto* openMenu = player->openContainerMenu(); openMenu != nullptr) {
            outItem = openMenu->getCarriedItem();
        } else {
            outItem = player->inventory().getCarriedItem();
        }
        return true;
    }

    // 合成槽位尚未接入永久合成菜单，其余槽位在玩家上不存在。
    return false;
}

bool setPlayerSlotItem(ServerCommandSource& source, PlayerId playerId, const ItemSlot& slot, const ItemStack& stack)
{
    PlayerInventory* inventory = resolvePlayerInventory(source, playerId);
    if (inventory == nullptr) {
        return false;
    }

    if (slot.isEquipmentSlot()) {
        i32 invSlot = 0;
        if (!_equipmentInventorySlot(*inventory, slot, invSlot)) {
            return false;
        }
        inventory->setItem(invSlot, stack);
        return true;
    }

    if (slot.isPlayerInventorySlot()) {
        const i32 invSlot = slot.slotIndex();
        if (invSlot < 0 || invSlot >= InventorySlots::TOTAL_SIZE) {
            return false;
        }
        inventory->setItem(invSlot, stack);
        return true;
    }

    if (slot.isEnderChestSlot()) {
        Player* player = _playerEntity(source, playerId);
        if (player == nullptr) {
            return false;
        }
        const i32 enderSlot = slot.toEnderChestSlot();
        if (enderSlot < 0 || enderSlot >= player->enderChestInventory().getContainerSize()) {
            return false;
        }
        player->enderChestInventory().setItem(enderSlot, stack);
        return true;
    }

    if (slot.isCursorSlot()) {
        Player* player = _playerEntity(source, playerId);
        if (player == nullptr) {
            return false;
        }
        if (auto* openMenu = player->openContainerMenu(); openMenu != nullptr) {
            openMenu->setCarriedItem(stack);
        } else {
            player->inventory().setCarriedItem(stack);
        }
        return true;
    }

    // 合成槽位与未知槽位：玩家身上没有对应槽位。
    // TODO: 永久合成菜单（InventoryCraftingMenu）接入 Player 后再支持 500-503。
    return false;
}

bool getBlockContainerSlot(server::ServerWorld& world, const BlockPos& pos, i32 slotIndex, ItemStack& outItem)
{
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    auto* container = dynamic_cast<ContainerBlockEntity*>(blockEntity);
    if (container == nullptr) {
        return false;
    }
    IInventory* inventory = container->getInventory();
    if (inventory == nullptr || slotIndex < 0 || slotIndex >= inventory->getContainerSize()) {
        return false;
    }
    outItem = inventory->getItem(slotIndex);
    return true;
}

bool setBlockContainerSlot(
    server::ServerWorld& world, const BlockPos& pos, i32 slotIndex, const ItemStack& stack)
{
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    auto* container = dynamic_cast<ContainerBlockEntity*>(blockEntity);
    if (container == nullptr) {
        return false;
    }
    IInventory* inventory = container->getInventory();
    if (inventory == nullptr || slotIndex < 0 || slotIndex >= inventory->getContainerSize()) {
        return false;
    }
    inventory->setItem(slotIndex, stack);
    inventory->setChanged();
    return true;
}

bool getNonPlayerEntitySlotItem(Entity& entity, const ItemSlot& slot, ItemStack& outItem)
{
    if (slot.isEquipmentSlot()) {
        auto* living = dynamic_cast<LivingEntity*>(&entity);
        if (living == nullptr) {
            return false;
        }
        const i32 enumIndex = slot.toEquipmentSlotIndex();
        if (enumIndex < 0 || enumIndex >= static_cast<i32>(EquipmentSlot::Count)) {
            return false;
        }
        outItem = living->getEquipment(static_cast<EquipmentSlot>(enumIndex));
        return true;
    }

    if (auto* horse = dynamic_cast<AbstractHorseEntity*>(&entity)) {
        const i32 horseSlot = slot.isHorseSlot() ? slot.toHorseSlot() : -1;
        if (horseSlot >= 0 && horseSlot < horse->getInventorySize()) {
            outItem = horse->getEquipment(horseSlot);
            return true;
        }
    }

    if (slot.isVillagerSlot()) {
        auto* villager = dynamic_cast<entity::AbstractVillagerEntity*>(&entity);
        if (villager == nullptr) {
            return false;
        }
        const i32 villagerSlot = slot.toVillagerSlot();
        if (villagerSlot < 0 || villagerSlot >= villager->inventory().getContainerSize()) {
            return false;
        }
        outItem = villager->inventory().getItem(villagerSlot);
        return true;
    }

    return false;
}

bool setNonPlayerEntitySlotItem(Entity& entity, const ItemSlot& slot, const ItemStack& stack)
{
    if (slot.isEquipmentSlot()) {
        auto* living = dynamic_cast<LivingEntity*>(&entity);
        if (living == nullptr) {
            return false;
        }
        const i32 enumIndex = slot.toEquipmentSlotIndex();
        if (enumIndex < 0 || enumIndex >= static_cast<i32>(EquipmentSlot::Count)) {
            // 100/101/102/103/105 之外的装备槽（如 104 不存在）在 vanilla 无对应槽位。
            return false;
        }
        living->setEquipment(static_cast<EquipmentSlot>(enumIndex), stack);
        return true;
    }

    if (auto* horse = dynamic_cast<AbstractHorseEntity*>(&entity)) {
        const i32 horseSlot = slot.isHorseSlot() ? slot.toHorseSlot() : -1;
        if (horseSlot >= 0 && horseSlot < horse->getInventorySize()) {
            // 空物品表示清空槽位；非空时按鞍/马铠规则校验。
            if (!stack.isEmpty() && !horse->canEquip(stack, horseSlot)) {
                return false;
            }
            horse->setEquipment(horseSlot, stack);
            return true;
        }
    }

    if (slot.isVillagerSlot()) {
        auto* villager = dynamic_cast<entity::AbstractVillagerEntity*>(&entity);
        if (villager == nullptr) {
            return false;
        }
        const i32 villagerSlot = slot.toVillagerSlot();
        if (villagerSlot < 0 || villagerSlot >= villager->inventory().getContainerSize()) {
            return false;
        }
        villager->inventory().setItem(villagerSlot, stack);
        return true;
    }

    return false;
}

std::string describeSlot(const ItemSlot& slot)
{
    const i32 idx = slot.slotIndex();
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
    if (slot.isEnderChestSlot()) {
        return "enderchest." + std::to_string(idx - 200);
    }
    if (slot.isHorseSlot()) {
        return "horse." + std::to_string(idx - 500);
    }
    if (slot.isHorseChestSlot()) {
        return "horse.chest";
    }
    if (slot.isCraftingSlot()) {
        return "player.crafting." + std::to_string(idx - 500);
    }
    if (slot.isVillagerSlot()) {
        return "villager." + std::to_string(idx - 300);
    }
    return std::to_string(idx);
}

} // namespace mc::command::support
