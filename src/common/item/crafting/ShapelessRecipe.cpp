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

#include "item/crafting/ShapelessRecipe.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/IRecipe.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace crafting {

ShapelessRecipe::ShapelessRecipe(
    const ResourceLocation& id, std::vector<Ingredient> ingredients, ItemStack result, const std::string& group)
    : m_id(id)
    , m_ingredients(std::move(ingredients))
    , m_result(std::move(result))
    , m_group(group)
{
    // 计算是否为简单配方
    m_isSimple = true;
    for (const Ingredient& ingredient : m_ingredients) {
        if (!ingredient.isSimple()) {
            m_isSimple = false;
            break;
        }
    }
}

bool ShapelessRecipe::matches(const CraftingInventory& inventory) const
{
    // 统计网格中的非空槽位数量
    i32 nonEmptySlots = 0;
    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        if (!inventory.getItem(i).isEmpty()) {
            ++nonEmptySlots;
        }
    }

    // 原料数量必须等于非空槽位数量
    if (static_cast<i32>(m_ingredients.size()) != nonEmptySlots) {
        return false;
    }

    // 跟踪已使用的槽位
    std::vector<bool> used(inventory.getContainerSize(), false);

    // 使用回溯算法进行匹配
    return _matchWithBacktracking(inventory, used, 0);
}

bool ShapelessRecipe::_matchWithBacktracking(
    const CraftingInventory& inventory, std::vector<bool>& used, i32 ingredientIndex) const
{
    // 所有原料都已匹配
    if (ingredientIndex >= static_cast<i32>(m_ingredients.size())) {
        return true;
    }

    const Ingredient& ingredient = m_ingredients[ingredientIndex];

    // 尝试为当前原料找一个匹配的槽位
    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        if (!used[i] && ingredient.test(inventory.getItem(i))) {
            used[i] = true;

            // 递归匹配下一个原料
            if (_matchWithBacktracking(inventory, used, ingredientIndex + 1)) {
                return true;
            }

            // 回溯，尝试其他槽位
            used[i] = false;
        }
    }

    // 没有找到匹配
    return false;
}

ItemStack ShapelessRecipe::assemble(const CraftingInventory& inventory) const
{
    (void)inventory;
    return m_result.copy();
}

bool ShapelessRecipe::canFitIn(i32 width, i32 height) const
{
    return static_cast<i32>(m_ingredients.size()) <= width * height;
}

std::vector<ItemStack> ShapelessRecipe::getRemainingItems(const CraftingInventory& inventory) const
{
    return RecipeUtils::getDefaultRemainingItems(inventory);
}

} // namespace crafting
} // namespace mc
