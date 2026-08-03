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
#include "common/item/crafting/IRecipe.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "item/crafting/RecipeManager.hpp"
#include <string>
#include <vector>

namespace mc {
namespace crafting {

/**
 * @brief 无序合成配方
 *
 * 无序合成只要求原料存在，不要求特定位置。
 * 原料顺序不影响匹配结果。
 *
 * 匹配算法：
 * 1. 检查原料数量是否匹配
 * 2. 对于简单原料（不包含可损坏物品）：使用贪心匹配
 * 3. 对于复杂原料：使用回溯算法确保正确匹配
 *
 * JSON 格式示例：
 * @code
 * {
 *   "type": "minecraft:crafting_shapeless",
 *   "ingredients": [
 *     { "item": "minecraft:iron_ingot" },
 *     { "item": "minecraft:stick" }
 *   ],
 *   "result": {
 *     "item": "minecraft:iron_ingot",
 *     "count": 1
 *   }
 * }
 * @endcode
 */
class ShapelessRecipe : public CraftingRecipe {
public:
    /**
     * @brief 构造无序合成配方
     * @param id 配方ID
     * @param ingredients 原料列表
     * @param result 结果物品堆
     * @param group 配方分组（可选）
     */
    ShapelessRecipe(const ResourceLocation& id,
        std::vector<Ingredient> ingredients,
        ItemStack result,
        const std::string& group = "");

    /**
     * @brief 检查配方是否匹配给定容器
     * @param inventory 合成网格
     * @return 如果匹配返回true
     */
    [[nodiscard]] bool matches(const CraftingInventory& inventory) const override;

    /**
     * @brief 生成结果物品堆
     * @param inventory 合成网格（用于获取NBT数据）
     * @return 结果物品堆
     */
    [[nodiscard]] ItemStack assemble(const CraftingInventory& inventory) const override;

    /**
     * @brief 获取结果物品
     * @return 结果物品堆
     */
    [[nodiscard]] ItemStack getResultItem() const override { return m_result; }

    /**
     * @brief 获取原料列表
     * @return 原料列表
     */
    [[nodiscard]] const std::vector<Ingredient>& getIngredients() const override { return m_ingredients; }

    /**
     * @brief 获取配方分组
     * @return 分组名，如果无分组返回空字符串
     */
    [[nodiscard]] const std::string& getGroup() const override { return m_group; }

    /**
     * @brief 获取配方ID
     * @return 配方ID
     */
    [[nodiscard]] ResourceLocation getId() const override { return m_id; }

    /**
     * @brief 获取配方类型
     * @return RecipeType::ShapelessCrafting
     */
    [[nodiscard]] RecipeType getType() const override { return RecipeType::ShapelessCrafting; }

    /**
     * @brief 检查配方是否适合给定尺寸的网格
     * @param width 网格宽度
     * @param height 网格高度
     * @return 如果原料数量不超过网格大小返回true
     */
    [[nodiscard]] bool canFitIn(i32 width, i32 height) const override;

    /**
     * @brief 获取合成后剩余的物品堆
     * @param inventory 合成网格
     * @return 每个槽位的剩余物品堆列表
     */
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const CraftingInventory& inventory) const override;

    /**
     * @brief 检查配方是否为简单配方
     * @return 如果所有原料都不包含可损坏物品返回true
     *
     * 简单配方可以使用更高效的贪心匹配算法。
     */
    [[nodiscard]] bool isSimple() const { return m_isSimple; }

private:
    /**
     * @brief 使用回溯算法匹配原料
     * @param inventory 合成网格
     * @param used 已使用的槽位标记
     * @param ingredientIndex 当前要匹配的原料索引
     * @return 如果所有原料都能匹配返回true
     *
     * 回溯算法确保在贪心算法可能失败的情况下也能找到正确的匹配。
     */
    bool _matchWithBacktracking(const CraftingInventory& inventory, std::vector<bool>& used, i32 ingredientIndex) const;

    ResourceLocation m_id;
    std::vector<Ingredient> m_ingredients;
    ItemStack m_result;
    std::string m_group;
    bool m_isSimple = true; ///< 是否为简单配方（所有原料都不包含可损坏物品）
};

} // namespace crafting
} // namespace mc
