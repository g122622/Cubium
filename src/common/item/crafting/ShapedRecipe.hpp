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
 * @brief 有序合成配方
 *
 * 有序合成要求原料按照特定图案放置在合成网格中。
 * 配方可以镜像翻转（水平翻转）。
 *
 * 匹配算法：
 * 1. 计算输入网格中物品的边界框
 * 2. 检查边界框尺寸是否匹配配方尺寸
 * 3. 尝试所有可能的位置偏移
 * 4. 对于每个位置，检查正向和镜像两种模式
 * 5. 返回第一个匹配结果
 *
 * JSON 格式示例：
 * @code
 * {
 *   "type": "minecraft:crafting_shaped",
 *   "pattern": ["##", "##"],
 *   "key": {
 *     "#": { "item": "minecraft:oak_planks" }
 *   },
 *   "result": {
 *     "item": "minecraft:crafting_table",
 *     "count": 1
 *   }
 * }
 * @endcode
 */
class ShapedRecipe : public CraftingRecipe {
public:
    /**
     * @brief 构造有序合成配方
     * @param id 配方ID
     * @param width 配方宽度
     * @param height 配方高度
     * @param ingredients 原料列表（按行优先顺序）
     * @param result 结果物品堆
     * @param group 配方分组（可选）
     */
    ShapedRecipe(const ResourceLocation& id,
        i32 width,
        i32 height,
        std::vector<Ingredient> ingredients,
        ItemStack result,
        const std::string& group = "");

    /**
     * @brief 检查配方是否匹配给定容器
     * @param inventory 合成网格
     * @return 如果匹配返回true
     */
    [[nodiscard]] bool matches(const CraftingInventory& inventory) const noexcept override;

    /**
     * @brief 生成结果物品堆
     * @param inventory 合成网格（用于获取NBT数据）
     * @return 结果物品堆
     */
    [[nodiscard]] ItemStack assemble(const CraftingInventory& inventory) const override;

    /**
     * @brief 获取结果物品
     * @return 结果物品堆（数量始终为配方定义的数量）
     */
    [[nodiscard]] ItemStack getResultItem() const noexcept override { return m_result; }

    /**
     * @brief 获取原料列表
     * @return 原料列表（按行优先顺序）
     */
    [[nodiscard]] const std::vector<Ingredient>& getIngredients() const noexcept override { return m_ingredients; }

    /**
     * @brief 获取配方分组
     * @return 分组名，如果无分组返回空字符串
     */
    [[nodiscard]] const std::string& getGroup() const noexcept override { return m_group; }

    /**
     * @brief 获取配方宽度
     * @return 宽度
     */
    [[nodiscard]] i32 getRecipeWidth() const noexcept override { return m_width; }

    /**
     * @brief 获取配方高度
     * @return 高度
     */
    [[nodiscard]] i32 getRecipeHeight() const noexcept override { return m_height; }

    /**
     * @brief 获取配方ID
     * @return 配方ID
     */
    [[nodiscard]] ResourceLocation getId() const noexcept override { return m_id; }

    /**
     * @brief 获取配方类型
     * @return RecipeType::ShapedCrafting
     */
    [[nodiscard]] RecipeType getType() const noexcept override { return RecipeType::ShapedCrafting; }

    /**
     * @brief 检查配方是否适合给定尺寸的网格
     * @param width 网格宽度
     * @param height 网格高度
     * @return 如果适合返回true
     */
    [[nodiscard]] bool canFitIn(i32 width, i32 height) const noexcept override;

    /**
     * @brief 获取合成后剩余的物品堆
     * @param inventory 合成网格
     * @return 每个槽位的剩余物品堆列表
     */
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const CraftingInventory& inventory) const override;

private:
    /**
     * @brief 检查指定位置的匹配
     * @param inventory 合成网格
     * @param offsetX X偏移
     * @param offsetY Y偏移
     * @param mirrored 是否镜像（true=水平翻转）
     * @return 如果匹配返回true
     *
     * 匹配算法：
     * - 遍历整个网格（而不是只遍历配方区域）
     * - 对于配方外的位置，使用 Ingredient.EMPTY 进行测试
     * - Ingredient.EMPTY.test(stack) 只对空物品堆返回 true
     */
    bool _checkMatch(const CraftingInventory& inventory, i32 offsetX, i32 offsetY, bool mirrored) const noexcept;

    ResourceLocation m_id;
    i32 m_width;
    i32 m_height;
    std::vector<Ingredient> m_ingredients;
    ItemStack m_result;
    std::string m_group;
};

} // namespace crafting
} // namespace mc
