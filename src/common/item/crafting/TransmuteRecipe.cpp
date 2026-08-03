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

#include "item/crafting/TransmuteRecipe.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/IRecipe.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace crafting {

TransmuteRecipe::TransmuteRecipe(
    const ResourceLocation& id, Ingredient input, Ingredient material, ItemStack result, const std::string& group)
    : m_id(id)
    , m_input(std::move(input))
    , m_material(std::move(material))
    , m_result(std::move(result))
    , m_group(group)
{
    // 构造 getIngredients() 返回的列表（input + material）
    m_ingredients.reserve(2);
    m_ingredients.push_back(m_input);
    m_ingredients.push_back(m_material);
}

bool TransmuteRecipe::matches(const CraftingInventory& inventory) const
{
    // 对应 MC 1.21.11 TransmuteRecipe#matches
    // 统计非空物品数量
    i32 nonEmptyCount = 0;
    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        if (!inventory.getItem(i).isEmpty()) {
            ++nonEmptyCount;
        }
    }
    if (nonEmptyCount != 2) {
        return false;
    }

    bool inputFound = false;
    bool materialFound = false;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }

        if (!inputFound && m_input.test(stack)) {
            // input 物品必须满足：转化结果不能与原物品相同
            if (_isResultUnchanged(stack)) {
                return false;
            }
            inputFound = true;
        } else {
            // 不能是 input（已找到 input），必须是 material
            if (materialFound || !m_material.test(stack)) {
                return false;
            }
            materialFound = true;
        }
    }

    return inputFound && materialFound;
}

ItemStack TransmuteRecipe::assemble(const CraftingInventory& inventory) const
{
    // 对应 MC 1.21.11 TransmuteRecipe#assemble
    // 找到匹配 input 的物品堆，使用 transmuteCopy 转化
    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (!stack.isEmpty() && m_input.test(stack)) {
            const Item* resultItem = m_result.getItem();
            if (resultItem == nullptr) {
                return ItemStack::EMPTY;
            }
            return stack.transmuteCopy(*resultItem, m_result.getCount());
        }
    }
    return ItemStack::EMPTY;
}

std::vector<ItemStack> TransmuteRecipe::getRemainingItems(const CraftingInventory& inventory) const
{
    // 转化配方消耗 input 和 material，无剩余物品
    // input 物品被转化为结果物品（NBT 保留），material 被消耗
    return RecipeUtils::getDefaultRemainingItems(inventory);
}

bool TransmuteRecipe::_isResultUnchanged(const ItemStack& stack) const
{
    // 对应 MC 1.21.11 TransmuteResult#isResultUnchanged
    // 如果转化结果与原物品类型相同，则视为无意义转化
    // （NBT 会被保留，所以相同物品类型意味着结果与输入完全相同）
    const Item* resultItem = m_result.getItem();
    if (resultItem == nullptr) {
        return true;
    }
    return stack.getItem() == resultItem;
}

} // namespace crafting
} // namespace mc
