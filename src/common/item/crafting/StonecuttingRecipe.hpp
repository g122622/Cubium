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

#include "IRecipe.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "entity/inventory/IInventory.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace crafting {

/**
 * @brief 切石机配方
 *
 * 切石机配方将一个输入物品转换为输出物品。
 * 与熔炼配方类似，但不需要熔炼时间和经验值。
 *
 * JSON 格式示例：
 * @code
 * {
 *   "type": "minecraft:stonecutting",
 *   "ingredient": { "item": "minecraft:stone" },
 *   "result": "minecraft:stone_bricks",
 *   "count": 1
 * }
 * @endcode
 */
class StonecuttingRecipe : public IRecipe<IInventory> {
public:
    /**
     * @brief 构造函数
     * @param id 配方ID
     * @param group 分组名（切石机配方通常不使用分组）
     * @param ingredient 输入原料
     * @param result 结果物品
     * @param count 结果数量
     */
    StonecuttingRecipe(const ResourceLocation& id,
        const std::string& group,
        const Ingredient& ingredient,
        const ItemStack& result,
        i32 count = 1);

    ~StonecuttingRecipe() override = default;

    // ========== IRecipe 接口实现 ==========

    [[nodiscard]] bool matches(const IInventory& inventory) const override;
    [[nodiscard]] ItemStack assemble(const IInventory& inventory) const override;
    [[nodiscard]] ItemStack getResultItem() const override { return m_result; }
    [[nodiscard]] const std::vector<Ingredient>& getIngredients() const override;
    [[nodiscard]] const std::string& getGroup() const override { return m_group; }
    [[nodiscard]] ResourceLocation getId() const override { return m_id; }
    [[nodiscard]] RecipeType getType() const override { return RecipeType::Stonecutting; }

    /**
     * @brief 切石机配方始终可以适应（只有一个输入槽）
     */
    [[nodiscard]] bool canFitIn(i32 width, i32 height) const override
    {
        (void)width;
        (void)height;
        return true;
    }

    /**
     * @brief 获取合成后剩余的物品堆
     * @param inventory 容器
     * @return 每个槽位的剩余物品堆列表
     */
    [[nodiscard]] std::vector<ItemStack> getRemainingItems(const IInventory& inventory) const override;

    // ========== 切石机特有方法 ==========

    /**
     * @brief 获取输入原料
     * @return 输入原料
     */
    [[nodiscard]] const Ingredient& getIngredient() const { return m_ingredient; }

    /**
     * @brief 获取结果数量
     * @return 结果数量
     */
    [[nodiscard]] i32 getCount() const { return m_count; }

private:
    ResourceLocation m_id;
    std::string m_group;
    Ingredient m_ingredient;
    ItemStack m_result;
    i32 m_count;
    mutable std::vector<Ingredient> m_ingredients; ///< 缓存的原料列表
};

} // namespace crafting
} // namespace mc
