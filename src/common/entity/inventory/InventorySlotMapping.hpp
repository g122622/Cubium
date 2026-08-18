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
 * LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "Slot.hpp" // InventorySlots 常量
#include "common/core/Types.hpp"
#include "common/network/ir/packets/play/ItemStackView.hpp"
#include <vector>

namespace mc {

class PlayerInventory; // 前向声明，避免环依赖（实现放 .cpp）

/**
 * @brief vanilla InventoryMenu 菜单槽索引常量（46 槽布局）
 *
 * Java 1.21.11 玩家背包菜单（InventoryMenu）的槽位顺序，即
 * ContainerSetContent(containerId=0)/ContainerSetSlot(containerId=0) 包中 slot 的语义。
 * 与项目 PlayerInventory 内部索引（InventorySlots，41 槽）不同，需经 InventorySlotMapping 双向映射。
 *
 * 布局：
 * - 0: 合成结果 (craftResult)
 * - 1-4: 合成输入 (craftGrid)
 * - 5-8: 护甲 (HEAD/CHEST/LEGS/FEET)
 * - 9-35: 主背包 (main)
 * - 36-44: 快捷栏 (hotbar)
 * - 45: 副手 (offhand)
 */
namespace InventoryMenuSlots {
constexpr i32 CRAFT_RESULT = 0;
constexpr i32 CRAFT_START = 1;
constexpr i32 CRAFT_END = 4;
constexpr i32 ARMOR_HEAD = 5;
constexpr i32 ARMOR_CHEST = 6;
constexpr i32 ARMOR_LEGS = 7;
constexpr i32 ARMOR_FEET = 8;
constexpr i32 MAIN_START = 9;
constexpr i32 MAIN_END = 35;
constexpr i32 HOTBAR_START = 36;
constexpr i32 HOTBAR_END = 44;
constexpr i32 OFFHAND = 45;
constexpr i32 TOTAL_SIZE = 46;
} // namespace InventoryMenuSlots

/**
 * @brief PlayerInventory 内部索引(0-40) ↔ InventoryMenu 菜单索引(0-45) 双向映射
 *
 * 两套索引体系的对应关系：
 *
 * | PlayerInventory 内部 (InventorySlots) | InventoryMenu 菜单 (InventoryMenuSlots) |
 * |---------------------------------------|-----------------------------------------|
 * | 0-8  快捷栏 (HOTBAR)                  | 36-44 快捷栏                            |
 * | 9-35 主背包 (MAIN)                    | 9-35  主背包                            |
 * | 36-39 护甲 (ARMOR HEAD→FEET)          | 5-8   护甲 (HEAD→FEET)                  |
 * | 40   副手 (OFFHAND)                   | 45    副手                              |
 * | —                                     | 0/1-4 合成结果/合成输入（项目无合成格） |
 *
 * craftResult(0)/craftGrid(1-4) 在项目 PlayerInventory 中无对应槽，反向映射返回 -1。
 */
namespace InventorySlotMapping {

/// PlayerInventory 内部索引(0-40) → InventoryMenu 菜单索引(0-45)。越界返回 -1。
[[nodiscard]] constexpr i32 playerInvToMenuSlot(i32 playerInvSlot) noexcept
{
    using namespace InventorySlots;
    if (playerInvSlot >= HOTBAR_START && playerInvSlot <= HOTBAR_END) {
        return playerInvSlot + 36; // 0-8 → 36-44
    }
    if (playerInvSlot >= MAIN_START && playerInvSlot <= MAIN_END) {
        return playerInvSlot; // 9-35 → 9-35
    }
    if (playerInvSlot >= ARMOR_START && playerInvSlot <= ARMOR_END) {
        return playerInvSlot - 31; // 36-39 → 5-8
    }
    if (playerInvSlot == OFFHAND) {
        return InventoryMenuSlots::OFFHAND; // 40 → 45
    }
    return -1;
}

/// InventoryMenu 菜单索引(0-45) → PlayerInventory 内部索引(0-40)。合成槽(0/1-4)或越界返回 -1。
[[nodiscard]] constexpr i32 menuSlotToPlayerInvId(i32 menuSlot) noexcept
{
    using namespace InventoryMenuSlots;
    if (menuSlot == CRAFT_RESULT || (menuSlot >= CRAFT_START && menuSlot <= CRAFT_END)) {
        return -1; // 合成结果/合成输入：项目无对应槽
    }
    if (menuSlot >= ARMOR_HEAD && menuSlot <= ARMOR_FEET) {
        return menuSlot + 31; // 5-8 → 36-39
    }
    if (menuSlot >= MAIN_START && menuSlot <= MAIN_END) {
        return menuSlot; // 9-35 → 9-35
    }
    if (menuSlot >= HOTBAR_START && menuSlot <= HOTBAR_END) {
        return menuSlot - 36; // 36-44 → 0-8
    }
    if (menuSlot == OFFHAND) {
        return InventorySlots::OFFHAND; // 45 → 40
    }
    return -1;
}
} // namespace InventorySlotMapping

/**
 * @brief 构造 46 槽 InventoryMenu 布局的物品视图列表
 *
 * 用于 ContainerSetContent(containerId=0) 全量同步玩家物品栏。按 InventoryMenu 顺序填充：
 * 0=craftResult 空、1-4=craft 空、5-8=armor(helmet→boots)、9-35=main、36-44=hotbar、45=offhand。
 * 实现放 .cpp（需 PlayerInventory 完整定义 + toItemStackView）。
 *
 * @param inv 玩家背包
 * @return 46 个 ItemStackView，索引即 InventoryMenu 菜单槽索引
 */
[[nodiscard]] std::vector<mc::network::ir::play::ItemStackView> buildMenuContent(const PlayerInventory& inv);

} // namespace mc
