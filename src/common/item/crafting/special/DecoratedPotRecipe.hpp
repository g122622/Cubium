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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "item/crafting/SpecialRecipe.hpp"
#include <vector>

namespace mc {
namespace crafting {

/**
 * @brief 饰纹陶罐配方
 *
 * 在3x3合成台中使用十字形排列的陶片/砖块合成饰纹陶罐。
 *
 * 合成模式（十字形）：
 * ```
 *   .  B  .     B = back（背面）
 *   L  .  R     L = left（左面）
 *   .  F  .     R = right（右面）
 *               F = front（正面）
 * ```
 *
 * 四个位置的物品必须是 DECORATED_POT_INGREDIENTS 标签中的物品
 * （陶片或砖块），其余5个格子必须为空。
 *
 * 结果：根据四个位置的物品生成对应的 PotDecorations，
 * 调用 createDecoratedPotItem() 生成带有图案数据的饰纹陶罐物品。
 */
class DecoratedPotRecipe : public SpecialRecipe {
public:
    /**
     * @brief 构造函数
     * @param id 配方资源位置
     */
    explicit DecoratedPotRecipe(const ResourceLocation& id);

    /**
     * @brief 检查合成网格是否匹配饰纹陶罐配方
     * @param inventory 合成网格
     * @return 如果匹配返回 true
     *
     * 匹配条件：
     * 1. 合成网格为 3x3
     * 2. 恰好4个物品，位于十字形位置
     * 3. 四个物品都属于 DECORATED_POT_INGREDIENTS 标签
     */
    [[nodiscard]] bool matches(const CraftingInventory& inventory) const override;

    /**
     * @brief 根据输入物品生成饰纹陶罐
     * @param inventory 合成网格
     * @return 带有图案数据的饰纹陶罐物品堆
     */
    [[nodiscard]] ItemStack assemble(const CraftingInventory& inventory) const override;

    /**
     * @brief 获取剩余物品
     * @param inventory 合成网格
     * @return 空列表（所有输入物品都被消耗）
     */
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const CraftingInventory& inventory) const override;
};

} // namespace crafting
} // namespace mc
