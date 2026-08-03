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
#include "item/core/Item.hpp"
#include "item/crafting/SpecialRecipe.hpp"
#include <vector>

namespace mc {
namespace crafting {

/**
 * @brief 物品修复配方
 *
 * 允许玩家在工作台中修复两个相同类型的可损坏物品。
 * 修复后的物品耐久度 = (剩余耐久1 + 剩余耐久2) + 5%最大耐久度。
 * 诅咒附魔会被合并（取最高等级）。
 *
 * 使用示例：
 * @code
 * // 玩家在工作台中放入两把损坏的钻石剑
 * // 结果：一把修复后的钻石剑，耐久度增加
 * @endcode
 */
class RepairItemRecipe : public SpecialRecipe {
public:
    explicit RepairItemRecipe(const ResourceLocation& id);

    /**
     * @brief 检查是否匹配修复配方
     * @param inventory 合成网格
     * @return 如果有两个相同类型的可修复物品返回 true
     *
     * 匹配条件：
     * 1. 只有恰好两个物品
     * 2. 两个物品类型相同
     * 3. 物品数量各为 1
     * 4. 物品可修复（isRepairable()）
     */
    [[nodiscard]] bool matches(const CraftingInventory& inventory) const override;

    /**
     * @brief 生成修复后的物品
     * @param inventory 合成网格
     * @return 修复后的物品
     *
     * 修复计算：
     * 1. 计算修复后耐久度 = (剩余耐久1 + 剩余耐久2) + 5%最大耐久度
     * 2. 合并诅咒附魔（取最高等级）
     */
    [[nodiscard]] ItemStack assemble(const CraftingInventory& inventory) const override;

    /**
     * @brief 获取剩余物品
     * @param inventory 合成网格
     * @return 空列表（修复配方消耗所有输入物品）
     */
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const CraftingInventory& inventory) const override;
};

} // namespace crafting
} // namespace mc
