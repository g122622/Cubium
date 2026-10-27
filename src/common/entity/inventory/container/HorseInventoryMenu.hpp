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

#pragma once

#include "common/entity/inventory/AbstractContainerMenu.hpp"
#include "common/entity/inventory/InventorySlotMapping.hpp"
#include "common/entity/inventory/Slot.hpp"

#include <memory>

namespace mc {

namespace blockentity {
class SimpleInventory;
}

class AbstractHorseEntity;
class PlayerInventory;
class IWorld;

/**
 * @brief 马匹装备槽（对齐 vanilla HorseInventoryMenu.SaddleSlot / ArmorSlot / nonEquineSlot）
 *
 * 鞍槽只接受鞍；马铠/装饰槽交由马实体自身的 isValidArmorForSlot 判定（马铠/地毯/猪鞍等）；
 * 箱马箱子槽只接受非鞍、非马铠物品。空物品（清空槽位）恒可放入。
 */
class HorseEquipmentSlot : public Slot {
public:
    /**
     * @param inventory 马匹库存
     * @param slotIndex 马匹库存内部槽位索引（0=鞍，1=马铠/装饰，2+=箱子）
     * @param x 槽位屏幕 X
     * @param y 槽位屏幕 Y
     * @param horse 马实体（提供 isValidArmorForSlot）
     */
    HorseEquipmentSlot(
        blockentity::SimpleInventory* inventory, i32 slotIndex, i32 x, i32 y, AbstractHorseEntity& horse);

    /// 该槽位是否可放入指定物品（对齐 vanilla Slot.mayPlace）
    [[nodiscard]] bool mayPlace(const ItemStack& stack) const override;

private:
    AbstractHorseEntity& m_horse;
};

/**
 * @brief 马匹背包菜单（对齐 vanilla HorseInventoryMenu）
 *
 * 槽位布局：
 * - 0 鞍
 * - 1 马铠/装饰（仅 hasArmorSlot 的马）
 * - 2.. 箱马箱子内容（仅 chested horse）
 * - 之后为玩家主背包（3 行 × 9 列）与快捷栏（1 行 × 9 列）
 */
class HorseInventoryMenu : public AbstractContainerMenu {
public:
    /// 玩家主背包起始 Y
    static constexpr i32 PLAYER_INV_Y = 84;
    /// 快捷栏 Y
    static constexpr i32 HOTBAR_Y = 142;
    /// 槽位边长
    static constexpr i32 SLOT_SIZE = 18;
    /// 马匹库存前导槽（鞍 + 马铠）数量
    static constexpr i32 HORSE_INVENTORY_ROW = 2;

    /**
     * @param id 容器 ID
     * @param playerInventory 玩家背包
     * @param horse 马实体
     * @param inventory 马匹库存（存活期不短于菜单）
     */
    HorseInventoryMenu(
        i32 id, PlayerInventory* playerInventory, AbstractHorseEntity& horse, blockentity::SimpleInventory* inventory);

    ~HorseInventoryMenu() override = default;

    /// 玩家是否仍可访问（对齐 vanilla HorseInventoryMenu.stillValid）
    [[nodiscard]] bool stillValid(const Player& player) const override;

    /// 菜单打开期间的库存变化回调（对齐 vanilla slotsChanged）
    void slotsChanged(IInventory* inventory) override;

protected:
    ItemStack quickMoveStack(i32 slotIndex, Player& player) override;

private:
    AbstractHorseEntity& m_horse;
    blockentity::SimpleInventory* m_horseInventory;
};

} // namespace mc
