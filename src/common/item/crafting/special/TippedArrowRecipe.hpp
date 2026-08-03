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
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "item/crafting/SpecialRecipe.hpp"
#include <vector>

namespace mc {
namespace crafting {

/**
 * @brief 药水箭配方
 *
 * 允许玩家使用滞留药水和箭合成药水箭。
 *
 * 合成模式：
 * - 中心格子：滞留药水
 * - 周围格子：箭（至少1支，最多8支）
 *
 * 结果：
 * - 每支箭变成一支药水箭，继承滞留药水的效果
 * - 最多8支药水箭
 */
class TippedArrowRecipe : public SpecialRecipe {
public:
    explicit TippedArrowRecipe(const ResourceLocation& id);

    /**
     * @brief 检查是否匹配药水箭配方
     * @param inventory 合成网格
     * @return 如果有滞留药水且周围有箭返回 true
     *
     * 匹配条件：
     * 1. 恰好有一个滞留药水
     * 2. 至少有一个箭
     * 3. 没有其他物品
     */
    [[nodiscard]] bool matches(const CraftingInventory& inventory) const override;

    /**
     * @brief 生成药水箭
     * @param inventory 合成网格
     * @return 药水箭物品堆（数量等于输入箭的数量）
     */
    [[nodiscard]] ItemStack assemble(const CraftingInventory& inventory) const override;

    /**
     * @brief 获取剩余物品
     * @param inventory 合成网格
     * @return 空列表（滞留药水和箭都被消耗）
     */
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const CraftingInventory& inventory) const override;

private:
    /**
     * @brief 查找滞留药水的位置
     * @param inventory 合成网格
     * @return 滞留药水的槽位索引，如果没找到返回 -1
     */
    [[nodiscard]] i32 _findLingeringPotion(const CraftingInventory& inventory) const noexcept;

    /**
     * @brief 统计箭的数量
     * @param inventory 合成网格
     * @param excludeSlot 排除的槽位（滞留药水所在位置）
     * @return 箭的总数量
     */
    [[nodiscard]] i32 _countArrows(const CraftingInventory& inventory, i32 excludeSlot) const noexcept;
};

} // namespace crafting
} // namespace mc
