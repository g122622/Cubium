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

#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "item/core/ItemStack.hpp"
#include "item/crafting/SpecialRecipe.hpp"
#include <vector>

namespace mc {
namespace crafting {

/**
 * @brief 地图扩展配方
 *
 * 允许玩家使用已填充地图和纸张扩展地图（缩小比例）。
 * 扩展后的地图覆盖范围更大，但细节更少。
 *
 * 参考: net.minecraft.item.crafting.MapExtendingRecipe
 *
 * 合成台匹配条件：
 * - 1张已填充地图（FilledMapItem），且缩放级别 < 4
 * - 8张纸（Paper），围绕地图放置
 *
 * 制图台匹配条件：
 * - 1张已填充地图 + 1张纸
 *
 * 结果：
 * - 输出缩放级别+1的新地图
 * - 原地图被消耗
 */
class MapExtendingRecipe : public SpecialRecipe {
public:
    explicit MapExtendingRecipe(const ResourceLocation& id);

    /**
     * @brief 检查是否匹配地图扩展配方
     * @param inventory 合成网格
     * @return 如果有1张已填充地图（缩放级别<4）和纸张返回 true
     */
    [[nodiscard]] bool matches(const CraftingInventory& inventory) const override;

    /**
     * @brief 生成扩展后的地图
     * @param inventory 合成网格
     * @return 缩放级别+1的新地图
     */
    [[nodiscard]] ItemStack assemble(const CraftingInventory& inventory) const override;

    /**
     * @brief 获取剩余物品
     * @param inventory 合成网格
     * @return 无剩余（原地图被消耗）
     */
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const CraftingInventory& inventory) const override;

private:
    /**
     * @brief 检查物品是否为已填充地图（且可扩展）
     * @param stack 物品堆
     * @return 如果是已填充地图且缩放级别 < 4 返回 true
     */
    [[nodiscard]] static bool _isExtendableMap(const ItemStack& stack);

    /**
     * @brief 检查物品是否为纸
     * @param stack 物品堆
     * @return 如果是纸返回 true
     */
    [[nodiscard]] static bool _isPaper(const ItemStack& stack);
};

} // namespace crafting
} // namespace mc
