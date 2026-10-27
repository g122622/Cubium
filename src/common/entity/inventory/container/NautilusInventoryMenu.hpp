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

#include <memory>

namespace mc {

namespace blockentity {
class SimpleInventory;
}

class AbstractNautilusEntity;
class PlayerInventory;

/**
 * @brief 鹦鹉螺背包菜单（对齐 vanilla 1.21.11 NautilusInventoryMenu）
 *
 * vanilla `AbstractNautilus.openCustomInventoryScreen` 走
 * `player.openNautilusInventory(this, inventory)`，服务端由 `ServerPlayer.openNautilusInventory`
 * 构造 `NautilusInventoryMenu`（继承 AbstractMountInventoryMenu）并经
 * `ClientboundMountScreenOpenPacket` 下发——即鹦鹉螺与马共用"坐骑界面"网络类型，
 * 槽位为鞍(0)、身体装备(1)，其后为坐骑附加库存，再后为玩家背包。
 *
 * 槽位坐标对齐 vanilla：鞍槽 (8, 18)、鹦鹉螺铠甲槽 (8, 36)、玩家背包与快捷栏起点 (8, 84)。
 * 装备槽是否可放由鹦鹉螺自身的 canEquip 判定；槽位是否可用（灰显）对齐 vanilla
 * ArmorSlot.isActive() = canUseSlot(SADDLE/BODY)。
 */
class NautilusInventoryMenu : public AbstractContainerMenu {
public:
    /// 鞍槽索引（对齐 vanilla NautilusInventoryMenu.SLOT_SADDLE）
    static constexpr i32 SADDLE_SLOT = 0;
    /// 身体装备槽索引（对齐 vanilla NautilusInventoryMenu.SLOT_BODY_ARMOR）
    static constexpr i32 BODY_SLOT = 1;
    /// 玩家主背包起始 Y
    static constexpr i32 PLAYER_INV_Y = 84;
    /// 快捷栏 Y
    static constexpr i32 HOTBAR_Y = 142;
    /// 槽位边长
    static constexpr i32 SLOT_SIZE = 18;

    /**
     * @param id 容器 ID
     * @param playerInventory 玩家背包
     * @param nautilus 鹦鹉螺实体
     * @param inventory 鹦鹉螺库存（存活期不短于菜单）
     */
    NautilusInventoryMenu(i32 id,
        PlayerInventory* playerInventory,
        AbstractNautilusEntity& nautilus,
        blockentity::SimpleInventory* inventory);

    ~NautilusInventoryMenu() override = default;

    /// 玩家是否仍可访问（对齐 vanilla AbstractMountInventoryMenu.stillValid 的实体距离部分）
    [[nodiscard]] bool stillValid(const Player& player) const override;

protected:
    ItemStack quickMoveStack(i32 slotIndex, Player& player) override;

private:
    AbstractNautilusEntity& m_nautilus;
    blockentity::SimpleInventory* m_nautilusInventory;
};

} // namespace mc
