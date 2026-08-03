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
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/CreativeInventory.hpp"
#include "core/Types.hpp"
#include <utility>
#include <vector>

namespace mc {

class PlayerInventory;
class Player;

/**
 * @brief 创造模式物品选择容器菜单
 *
 * 对齐原版 CreativeModeInventoryScreen 的容器协议：槽位=玩家背包（护甲/主背包/
 * 快捷栏/副手，共 41 槽，无合成网格），创造物品池**不**作为网络同步槽位（池约
 * 千项，做槽会同步上千 ItemStack 过线），而是用**虚拟调色板槽索引**承载取物：
 *
 * - 调色板点击以 `ClickAction::Clone` 发送，slotIndex = PALETTE_VIRTUAL_BASE +
 *   visibleIndex。`clicked` override 识别该区间，从**本地重建的物品池**取对应
 *   entry，clone 到光标（左键整组、右键单个），仅创造模式生效（`GameModeUtils::
 *   isCreative` 守卫）。
 * - 玩家背包槽位走基类 `AbstractContainerMenu::clicked` 的正常 Pick/QuickMove/
 *   Swap/Throw 逻辑。
 *
 * 服务端与客户端各持一个 ItemPickerMenu 实例（共享 containerId=0）。服务端侧
 * 持有 `buildCreativePaletteEntries()` 结果用于 clone 取物；客户端侧物品池仅用于
 * 调色板渲染，clone 的光标由服务端经 `ContainerContentPacket` 的 carried 字段回传。
 *
 * 槽位布局（GUI 内相对坐标，与 InventoryCraftingMenu 的玩家区一致）：
 * - 0-3:  护甲（头盔/胸甲/护腿/靴子）(8, 8..62)
 * - 4-30: 主背包 (3x9) (8, 84)
 * - 31-39: 快捷栏 (1x9) (8, 142)
 * - 40:   副手 (77, 62)
 */
class ItemPickerMenu : public AbstractContainerMenu {
public:
    /**
     * @brief 构造函数
     * @param id 容器ID（创造背包复用 PLAYER_CONTAINER_ID=0）
     * @param playerInventory 玩家背包
     * @param loadPalette 是否立即构建创造物品池（服务端需要用于 clone 取物；
     *                    客户端可传 false，由宿主屏幕自行持有池用于渲染）
     */
    ItemPickerMenu(ContainerId id, PlayerInventory* playerInventory, bool loadPalette = true);

    ItemStack clicked(i32 slotIndex, i32 button, ClickType clickType, Player& player) override;

    ItemStack quickMoveStack(i32 slotIndex, Player& player) override;

    [[nodiscard]] bool stillValid(const Player& player) const override { return true; }

    /**
     * @brief 创造物品池（供服务端 clone 取物 / 客户端调色板渲染）
     *
     * 若构造时未 loadPalette 则为空，需宿主自行 `setPaletteEntries` 或 `reloadPalette`。
     */
    [[nodiscard]] const std::vector<CreativeInventoryEntry>& paletteEntries() const { return m_paletteEntries; }
    void setPaletteEntries(std::vector<CreativeInventoryEntry> entries) { m_paletteEntries = std::move(entries); }

    /** @brief 重建创造物品池（创造模式打开时调用） */
    void reloadPalette() { m_paletteEntries = buildCreativePaletteEntries(); }

    /**
     * @brief 槽位索引常量（41 槽玩家背包，无合成网格）
     */
    static constexpr i32 ARMOR_SLOT_START = 0; ///< 护甲槽起始
    static constexpr i32 ARMOR_SLOT_COUNT = 4; ///< 护甲槽数量
    static constexpr i32 ARMOR_HEAD = 0;
    static constexpr i32 ARMOR_CHEST = 1;
    static constexpr i32 ARMOR_LEGS = 2;
    static constexpr i32 ARMOR_FEET = 3;
    static constexpr i32 PLAYER_INV_START = 4; ///< 主背包起始
    static constexpr i32 PLAYER_INV_COUNT = 27;
    static constexpr i32 PLAYER_INV_END = 30; ///< 主背包结束
    static constexpr i32 HOTBAR_START = 31;   ///< 快捷栏起始
    static constexpr i32 HOTBAR_COUNT = 9;
    static constexpr i32 HOTBAR_END = 39;       ///< 快捷栏结束
    static constexpr i32 OFFHAND_SLOT = 40;     ///< 副手
    static constexpr i32 TOTAL_SLOT_COUNT = 41; ///< 总槽位

    /**
     * @brief 虚拟调色板槽索引基址
     *
     * slotIndex >= 此值的点击被 `clicked` 识别为创造调色板取物（Clone 语义）。
     * 取值远大于真实槽位数，避免与背包槽冲突；`ContainerManager` 不预检
     * slotIndex 范围，故虚拟索引安全。
     */
    static constexpr i32 PALETTE_VIRTUAL_BASE = 10000;

private:
    /**
     * @brief 处理调色板虚拟槽 clone（创造取物）
     * @return clone 后的光标物品
     *
     * 仅创造模式生效；左键(button==0)取整组、右键(button==1)取单个。
     * visibleIndex 越界或非创造模式时返回当前光标不变。
     */
    ItemStack _handlePaletteClone(i32 slotIndex, i32 button, Player& player);

    std::vector<CreativeInventoryEntry> m_paletteEntries;
};

} // namespace mc
