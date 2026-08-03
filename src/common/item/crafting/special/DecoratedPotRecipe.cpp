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

#include "item/crafting/special/DecoratedPotRecipe.hpp"

#include "common/core/Types.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/SpecialRecipe.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "item/tag/ItemTags.hpp"
#include "world/blockentity/interactive/DecoratedPotBlockEntity.hpp"
#include <vector>

namespace mc {
namespace crafting {

using item::tag::ItemTags;

DecoratedPotRecipe::DecoratedPotRecipe(const ResourceLocation& id)
    : SpecialRecipe(id)
{}

bool DecoratedPotRecipe::matches(const CraftingInventory& inventory) const
{
    // 必须是 3x3 合成网格
    if (inventory.getWidth() != 3 || inventory.getHeight() != 3) {
        return false;
    }

    // 十字形四个位置的坐标：(1,0)=back, (0,1)=left, (2,1)=right, (1,2)=front
    constexpr struct {
        i32 x;
        i32 y;
    } crossPositions[] = {{1, 0}, {0, 1}, {2, 1}, {1, 2}};

    // 检查十字形位置的物品
    i32 ingredientCount = 0;
    for (const auto& pos : crossPositions) {
        const ItemStack& stack = inventory.getItemAt(pos.x, pos.y);
        if (stack.isEmpty()) {
            return false;
        }
        if (!ItemTags::DECORATED_POT_INGREDIENTS().contains(stack)) {
            return false;
        }
        ++ingredientCount;
    }

    // 检查非十字形位置必须为空
    // 四个角：(0,0), (2,0), (0,2), (2,2)
    // 中心：(1,1)
    constexpr struct {
        i32 x;
        i32 y;
    } emptyPositions[] = {{0, 0}, {2, 0}, {1, 1}, {0, 2}, {2, 2}};

    for (const auto& pos : emptyPositions) {
        if (!inventory.getItemAt(pos.x, pos.y).isEmpty()) {
            return false;
        }
    }

    return ingredientCount == 4;
}

ItemStack DecoratedPotRecipe::assemble(const CraftingInventory& inventory) const
{
    // 获取十字形四个位置的物品，转换为图案
    const Item* backItem = inventory.getItemAt(1, 0).getItem();
    const Item* leftItem = inventory.getItemAt(0, 1).getItem();
    const Item* rightItem = inventory.getItemAt(2, 1).getItem();
    const Item* frontItem = inventory.getItemAt(1, 2).getItem();

    // 将物品转换为对应的图案
    auto backPattern = blockentity::getPatternFromItem(backItem);
    auto leftPattern = blockentity::getPatternFromItem(leftItem);
    auto rightPattern = blockentity::getPatternFromItem(rightItem);
    auto frontPattern = blockentity::getPatternFromItem(frontItem);

    // 创建图案数据并生成饰纹陶罐物品
    blockentity::PotDecorations decorations(backPattern, leftPattern, rightPattern, frontPattern);
    return blockentity::createDecoratedPotItem(decorations);
}

std::vector<ItemStack> DecoratedPotRecipe::getRemainingItems(const CraftingInventory& inventory) const
{
    // 所有输入物品都被消耗，没有剩余物品
    return std::vector<ItemStack>(inventory.getContainerSize());
}

} // namespace crafting
} // namespace mc
