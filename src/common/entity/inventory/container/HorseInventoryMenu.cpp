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

#include "common/entity/inventory/container/HorseInventoryMenu.hpp"

#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/entities/passive/horse/AbstractHorseEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"

#include <cstddef>
#include <utility>

namespace mc {

// ============================================================================
// HorseEquipmentSlot
// ============================================================================

HorseEquipmentSlot::HorseEquipmentSlot(
    blockentity::SimpleInventory* inventory, i32 slotIndex, i32 x, i32 y, AbstractHorseEntity& horse)
    : Slot(inventory, slotIndex, x, y)
    , m_horse(horse)
{}

bool HorseEquipmentSlot::mayPlace(const ItemStack& stack) const
{
    // 对齐 vanilla Slot.mayPlace：空物品恒可放（用于清空槽位）。
    if (stack.isEmpty()) {
        return true;
    }
    const Item* item = stack.getItem();
    if (item == nullptr) {
        return false;
    }

    const i32 slotIndex = getSlotNumber();
    switch (slotIndex) {
        case 0:
            // 鞍槽：只接受鞍（对齐 vanilla SaddleSlot）。
            return item == Items::SADDLE;
        case 1:
            // 马铠/装饰槽：由马实体自身判定（马铠 / 地毯 / 猪鞍等）。
            return m_horse.isValidArmorForSlot(stack);
        default:
            // 箱马箱子槽：箱内只放非鞍、非马铠物品，避免与装备槽语义混淆。
            return item != Items::SADDLE && !m_horse.isValidArmorForSlot(stack);
    }
}

// ============================================================================
// HorseInventoryMenu
// ============================================================================

HorseInventoryMenu::HorseInventoryMenu(
    i32 id, PlayerInventory* playerInventory, AbstractHorseEntity& horse, blockentity::SimpleInventory* inventory)
    : AbstractContainerMenu(id, playerInventory)
    , m_horse(horse)
    , m_horseInventory(inventory)
{
    MC_ASSERT(playerInventory != nullptr);
    MC_ASSERT(inventory != nullptr);

    const bool chested = inventory->getContainerSize() > HORSE_INVENTORY_ROW;
    // 对齐 vanilla HorseInventoryMenu 槽位布局：未装箱的马两列（鞍、马铠）居中，装箱的马按
    // 5 列网格式排布鞍/马铠与箱子物品。
    const i32 columns = chested ? 5 : 2;
    for (i32 index = 0; index < inventory->getContainerSize(); ++index) {
        const i32 row = index / columns;
        const i32 column = index % columns;
        const i32 x = 8 + column * SLOT_SIZE;
        const i32 y = 18 + row * SLOT_SIZE;
        addSlot(std::make_unique<HorseEquipmentSlot>(inventory, index, x, y, m_horse));
    }

    // 玩家主背包（3 行 × 9 列）与快捷栏。
    for (i32 row = 0; row < 3; ++row) {
        for (i32 column = 0; column < 9; ++column) {
            addSlot(std::make_unique<Slot>(playerInventory,
                InventorySlots::MAIN_START + row * 9 + column,
                8 + column * SLOT_SIZE,
                PLAYER_INV_Y + row * SLOT_SIZE));
        }
    }
    for (i32 column = 0; column < 9; ++column) {
        addSlot(std::make_unique<Slot>(playerInventory, column, 8 + column * SLOT_SIZE, HOTBAR_Y));
    }
}

bool HorseInventoryMenu::stillValid(const Player& player) const
{
    // 对齐 vanilla HorseInventoryMenu.stillValid：马存活、与玩家同世界、且未受伤后逃离过远。
    if (m_horse.isRemoved() || m_horse.isChild()) {
        return false;
    }
    if (m_horse.world() != player.world()) {
        return false;
    }
    return player.distanceSqTo(m_horse.x(), m_horse.y(), m_horse.z()) <= 8.0f * 8.0f;
}

void HorseInventoryMenu::slotsChanged(IInventory* inventory)
{
    // 装备槽变化需要同步马的鞍/马铠状态（setEquipment 已在实体侧处理），此处仅转发基类逻辑。
    AbstractContainerMenu::slotsChanged(inventory);
}

ItemStack HorseInventoryMenu::quickMoveStack(i32 slotIndex, Player& player)
{
    (void)player;

    Slot* slot = getSlot(slotIndex);
    if (slot == nullptr || slot->isEmpty()) {
        return ItemStack();
    }

    ItemStack stack = slot->getItem();
    const ItemStack result = stack.copy();
    const i32 horseSlotCount = m_horseInventory->getContainerSize();

    if (slotIndex < horseSlotCount) {
        // 马库存 → 玩家背包
        if (!moveItemToRange(stack, horseSlotCount, getSlotCount(), true)) {
            return ItemStack();
        }
    } else if (stack.getItem() == Items::SADDLE) {
        // 玩家背包 → 鞍槽（对齐 vanilla quickMoveStack 优先把鞍放进空鞍槽）
        if (!getSlot(0)->hasItem() && !moveItemToRange(stack, 0, 1, false)) {
            return ItemStack();
        }
    } else if (m_horseInventory->getContainerSize() > 1 && m_horse.isValidArmorForSlot(stack)) {
        if (!getSlot(1)->hasItem() && !moveItemToRange(stack, 1, 2, false)) {
            return ItemStack();
        }
    } else if (!moveItemToRange(stack, 0, horseSlotCount, false)) {
        return ItemStack();
    }

    if (stack.isEmpty()) {
        slot->set(ItemStack());
    } else {
        slot->setChanged();
    }
    return result;
}

} // namespace mc
