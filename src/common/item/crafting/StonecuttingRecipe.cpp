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

#include "StonecuttingRecipe.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/IRecipe.hpp"
#include "common/item/crafting/Ingredient.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <string>
#include <vector>

namespace mc {
namespace crafting {

StonecuttingRecipe::StonecuttingRecipe(const ResourceLocation& id,
    const std::string& group,
    const Ingredient& ingredient,
    const ItemStack& result,
    i32 count)
    : m_id(id)
    , m_group(group)
    , m_ingredient(ingredient)
    , m_result(result)
    , m_count(count)
{
    // 缓存原料列表
    m_ingredients.push_back(m_ingredient);
}

bool StonecuttingRecipe::matches(const IInventory& inventory) const
{
    // 检查输入槽（槽位0）是否匹配原料
    if (inventory.getContainerSize() == 0) {
        return false;
    }
    return m_ingredient.test(inventory.getItem(0));
}

ItemStack StonecuttingRecipe::assemble(const IInventory& inventory) const
{
    (void)inventory;
    ItemStack result = m_result;
    result.setCount(m_count);
    return result;
}

const std::vector<Ingredient>& StonecuttingRecipe::getIngredients() const
{
    return m_ingredients;
}

std::vector<ItemStack> StonecuttingRecipe::getRemainingItems(const IInventory& inventory) const
{
    return RecipeUtils::getDefaultRemainingItems(inventory);
}

} // namespace crafting
} // namespace mc
