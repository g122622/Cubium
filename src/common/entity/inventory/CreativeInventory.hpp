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

#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include <string>
#include <vector>

namespace mc {

class PlayerInventory;

/**
 * @brief 创造模式物品条目
 *
 * 用于创造模式物品库的本地展示与搜索过滤。
 */
struct CreativeInventoryEntry {
    ItemStack stack;
    std::string searchKey;
};

/**
 * @brief 构建创造模式物品库条目
 *
 * 返回可在创造屏幕中展示的物品列表。条目按物品 ID 排序，
 * 方便搜索与滚动浏览。
 */
[[nodiscard]] std::vector<CreativeInventoryEntry> buildCreativePaletteEntries();

/**
 * @brief 填充创造模式初始背包
 *
 * 当前实现会保留一个工作台入口，并使用原版方块物品填充其余槽位。
 * 这是玩家初次进入创造模式时的默认物品栏布局。
 */
void fillCreativeModeInventory(PlayerInventory& inventory);

} // namespace mc