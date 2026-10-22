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

#include "NautilusInventoryMenu.hpp"

#include "common/entity/entities/passive/nautilus/AbstractNautilusEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/InventorySlotMapping.hpp"
#include "common/entity/inventory/PlayerInventory.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"

#include <memory>

namespace mc {

namespace {
/**
 * @brief 鹦鹉螺装备槽（对齐 vanilla NautilusInventoryMenu 裸插槽 + mayPlace 判定）
 *
 * vanilla NautilusInventoryMenu 把 EquipmentSlot 自身的 mayPlace 语义挂在 ArmorSlot 上：
 * 鞍槽只接受鞍，身体槽只接受鹦鹉螺铠甲。此处统一委托鹦鹉螺实体的 canEquip（槽 0 鞍、槽 1 铠甲）。
 * 槽位不可用（未驯服/幼年/已死亡）时 vanilla 的 `isActive()` 为假，对应 Cubium 的 isEnabled。
 */
class NautilusEquipmentSlot : public Slot {
public:
    NautilusEquipmentSlot(
        blockentity::SimpleInventory* inventory, i32 slotIndex, i32 x, i32 y, AbstractNautilusEntity& nautilus)
        : Slot(inventory, slotIndex, x, y)
        , m_nautilus(nautilus)
    {}

    [[nodiscard]] bool mayPlace(const ItemStack& stack) const override
    {
        // 对齐 vanilla Slot.mayPlace：空物品恒可放（用于清空槽位）。
        if (stack.isEmpty()) {
            return true;
        }
        return m_nautilus.canEquip(stack, getSlotNumber());
    }

    [[nodiscard]] bool isEnabled() const noexcept override
    {
        // 对齐 vanilla NautilusInventoryMenu 两个 ArmorSlot 的 isActive()（= canUseSlot）：
        // 鞍槽要求 canEquipSaddle()，身体槽要求 canEquipBodyArmor()；两者条件相同，
        // 故统一按"是否可装备"判定。
        return m_nautilus.canEquipSaddle();
    }

private:
    AbstractNautilusEntity& m_nautilus;
};
} // namespace

NautilusInventoryMenu::NautilusInventoryMenu(
    i32 id, PlayerInventory* playerInventory, AbstractNautilusEntity& nautilus, blockentity::SimpleInventory* inventory)
    : AbstractContainerMenu(id, playerInventory)
    , m_nautilus(nautilus)
    , m_nautilusInventory(inventory)
{
    MC_ASSERT(playerInventory != nullptr);
    MC_ASSERT(inventory != nullptr);

    // 鞍槽 (8, 18)
    addSlot(std::make_unique<NautilusEquipmentSlot>(inventory, SADDLE_SLOT, 8, 18, m_nautilus));
    // 身体装备槽 (8, 36)：vanilla NautilusInventoryMenu 的第二槽不与鞍槽同行，而是直接下移 18 像素。
    addSlot(std::make_unique<NautilusEquipmentSlot>(inventory, BODY_SLOT, 8, 36, m_nautilus));
    // TODO: vanilla 鹦鹉螺还会在 (sx, sy) 处按 getInventoryColumns() 附加宝藏库存槽
    // （AbstractMountInventoryMenu 的 mountContainer，槽位 2..2+N）。Cubium 的鹦鹉螺库存目前
    // 只有鞍 + 铠甲两格（getContainerSize()==2），补齐宝藏库存后在此追加槽位并展平为单槽索引。

    // 玩家主背包（3 行 × 9 列）与快捷栏，起点对齐 vanilla addStandardInventorySlots(8, 84)。
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

bool NautilusInventoryMenu::stillValid(const Player& player) const
{
    // 对齐 vanilla AbstractMountInventoryMenu.stillValid：
    // !hasInventoryChanged(container)（坐骑库存被重建即失效）&& 坐骑存活
    // && player.isWithinEntityInteractionRange(mount, 4.0)。
    // vanilla 中间还有 mountContainer.stillValid(player)——SimpleContainer 恒为真且无打开计数，
    // 故此处只需校验库存未被重建。
    if (m_nautilusInventory != m_nautilus.inventoryPtr()) {
        return false;
    }
    if (!m_nautilus.isAlive()) {
        return false;
    }
    return player.distanceSqTo(m_nautilus.x(), m_nautilus.y(), m_nautilus.z()) <= 16.0f;
}

ItemStack NautilusInventoryMenu::quickMoveStack(i32 slotIndex, Player& player)
{
    (void)player;

    Slot* slot = getSlot(slotIndex);
    if (slot == nullptr || slot->isEmpty()) {
        return ItemStack();
    }

    // 对齐 vanilla AbstractMountInventoryMenu.quickMoveStack：
    // i = 2（鞍 + 身体槽）+ 坐骑附加库存槽数；j/k 为玩家主背包与快捷栏的边界。
    const i32 equipmentEnd = BODY_SLOT + 1;
    const i32 mountSlotCount = m_nautilusInventory->getContainerSize();
    const i32 mountEnd = equipmentEnd + mountSlotCount;
    const i32 mainEnd = mountEnd + 27;
    const i32 hotbarEnd = mainEnd + 9;

    ItemStack stack = slot->getItem();
    const ItemStack result = stack.copy();

    if (slotIndex < mountEnd) {
        // 坐骑槽 → 玩家背包
        if (!moveItemToRange(stack, mountEnd, getSlotCount(), true)) {
            return ItemStack();
        }
    } else if (getSlot(BODY_SLOT)->mayPlace(stack) && !getSlot(BODY_SLOT)->hasItem()) {
        // 身体装备优先（vanilla 顺序：先槽 1，再槽 0）
        if (!moveItemToRange(stack, BODY_SLOT, BODY_SLOT + 1, false)) {
            return ItemStack();
        }
    } else if (getSlot(SADDLE_SLOT)->mayPlace(stack) && !getSlot(SADDLE_SLOT)->hasItem()) {
        if (!moveItemToRange(stack, SADDLE_SLOT, SADDLE_SLOT + 1, false)) {
            return ItemStack();
        }
    } else if (mountSlotCount == 0 || !moveItemToRange(stack, equipmentEnd, mountEnd, false)) {
        if (slotIndex >= mainEnd && slotIndex < hotbarEnd) {
            if (!moveItemToRange(stack, mountEnd, mainEnd, false)) {
                return ItemStack();
            }
        } else if (slotIndex >= mountEnd && slotIndex < mainEnd) {
            if (!moveItemToRange(stack, mainEnd, hotbarEnd, false)) {
                return ItemStack();
            }
        } else if (!moveItemToRange(stack, mainEnd, mainEnd, false)) {
            return ItemStack();
        }
        // 对齐 vanilla：这些分支一律返回空（物品去向由 moveItemStackTo 原地写回槽位）。
        if (stack.isEmpty()) {
            slot->set(ItemStack());
        } else {
            slot->setChanged();
        }
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