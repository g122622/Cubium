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

#include "InventorySlotMapping.hpp"

#include "PlayerInventory.hpp"
#include "common/network/ir/ItemStackBridge.hpp"

namespace mc {

std::vector<mc::network::ir::play::ItemStackView> buildMenuContent(const PlayerInventory& inv)
{
    using namespace mc::network::ir::play;
    std::vector<ItemStackView> out;
    out.reserve(static_cast<size_t>(InventoryMenuSlots::TOTAL_SIZE));

    // 按 InventoryMenu 菜单槽索引顺序填充。craftResult(0)/craftGrid(1-4) 在项目 PlayerInventory
    // 中无对应槽，填空 ItemStackView；其余槽经 menuSlotToPlayerInvId 反查内部索引后取物品。
    for (i32 menuSlot = 0; menuSlot < InventoryMenuSlots::TOTAL_SIZE; ++menuSlot) {
        const i32 playerInvSlot = InventorySlotMapping::menuSlotToPlayerInvId(menuSlot);
        if (playerInvSlot < 0) {
            out.push_back(ItemStackView{}); // 合成结果/合成输入：空
            continue;
        }
        out.push_back(mc::network::ir::toItemStackView(inv.getItem(playerInvSlot)));
    }
    return out;
}

} // namespace mc
