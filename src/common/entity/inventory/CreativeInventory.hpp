#pragma once

#include "../../core/Types.hpp"
#include "../../item/core/ItemStack.hpp"
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
    String searchKey;
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