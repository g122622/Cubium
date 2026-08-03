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

#include "item/crafting/ShapedRecipe.hpp"
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

ShapedRecipe::ShapedRecipe(const ResourceLocation& id,
    i32 width,
    i32 height,
    std::vector<Ingredient> ingredients,
    ItemStack result,
    const std::string& group)
    : m_id(id)
    , m_width(width)
    , m_height(height)
    , m_ingredients(std::move(ingredients))
    , m_result(std::move(result))
    , m_group(group)
{}

bool ShapedRecipe::matches(const CraftingInventory& inventory) const noexcept
{
    // 遍历所有可能的偏移位置，先检查镜像，再检查正向
    for (i32 offsetY = 0; offsetY <= inventory.getHeight() - m_height; ++offsetY) {
        for (i32 offsetX = 0; offsetX <= inventory.getWidth() - m_width; ++offsetX) {
            if (_checkMatch(inventory, offsetX, offsetY, true)) {
                return true;
            }
            if (_checkMatch(inventory, offsetX, offsetY, false)) {
                return true;
            }
        }
    }
    return false;
}

ItemStack ShapedRecipe::assemble(const CraftingInventory& inventory) const
{
    (void)inventory;
    return m_result.copy();
}

bool ShapedRecipe::canFitIn(i32 width, i32 height) const noexcept
{
    return width >= m_width && height >= m_height;
}

std::vector<ItemStack> ShapedRecipe::getRemainingItems(const CraftingInventory& inventory) const
{
    return RecipeUtils::getDefaultRemainingItems(inventory);
}

bool ShapedRecipe::_checkMatch(
    const CraftingInventory& inventory, i32 offsetX, i32 offsetY, bool mirrored) const noexcept
{
    // 遍历整个网格，对于网格外的位置，使用 Ingredient.EMPTY 进行测试
    for (i32 y = 0; y < inventory.getHeight(); ++y) {
        for (i32 x = 0; x < inventory.getWidth(); ++x) {
            // 计算相对于配方左上角的坐标
            i32 relX = x - offsetX;
            i32 relY = y - offsetY;

            // 获取该位置对应的原料
            const Ingredient* ingredient = &Ingredient::EMPTY;

            if (relX >= 0 && relY >= 0 && relX < m_width && relY < m_height) {
                // 位置在配方范围内
                if (mirrored) {
                    // 镜像：水平翻转 X 坐标
                    ingredient = &m_ingredients[m_width - relX - 1 + relY * m_width];
                } else {
                    // 正向
                    ingredient = &m_ingredients[relX + relY * m_width];
                }
            }

            // 测试网格中的物品
            i32 slot = y * inventory.getWidth() + x;
            ItemStack stack = inventory.getItem(slot);

            if (!ingredient->test(stack)) {
                return false;
            }
        }
    }
    return true;
}

} // namespace crafting
} // namespace mc
