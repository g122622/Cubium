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

#include "item/crafting/special/MapCloningRecipe.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/SpecialRecipe.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "item/Items.hpp"
#include "item/items/map/FilledMapItem.hpp"
#include <vector>

namespace mc {
namespace crafting {

MapCloningRecipe::MapCloningRecipe(const ResourceLocation& id)
    : SpecialRecipe(id)
{}

bool MapCloningRecipe::matches(const CraftingInventory& inventory) const
{
    bool hasFilledMap = false;
    bool hasEmptyMap = false;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }

        if (_isFilledMap(stack)) {
            if (hasFilledMap) {
                return false;
            }
            hasFilledMap = true;
        } else if (_isEmptyMap(stack)) {
            hasEmptyMap = true;
        } else {
            return false;
        }
    }

    return hasFilledMap && hasEmptyMap;
}

ItemStack MapCloningRecipe::assemble(const CraftingInventory& inventory) const
{
    ItemStack filledMap;
    i32 emptyMapCount = 0;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }

        if (_isFilledMap(stack)) {
            filledMap = stack;
        } else if (_isEmptyMap(stack)) {
            emptyMapCount += stack.getCount();
        }
    }

    if (filledMap.isEmpty() || emptyMapCount == 0) {
        return ItemStack::EMPTY;
    }

    // 创建复制的地图（数量 = 空地图数量 + 1）
    // 注意：原地图不会被消耗，所以结果包含原地图
    ItemStack result = filledMap.copy();
    result.setCount(emptyMapCount + 1);

    return result;
}

std::vector<ItemStack> MapCloningRecipe::getRemainingItems(const CraftingInventory& inventory) const
{
    std::vector<ItemStack> remaining(inventory.getContainerSize());

    // 保留原地图
    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (_isFilledMap(stack)) {
            remaining[i] = stack.copy();
            break;
        }
    }

    return remaining;
}

bool MapCloningRecipe::_isFilledMap(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return false;
    }
    return item::items::FilledMapItem::isFilledMap(stack);
}

bool MapCloningRecipe::_isEmptyMap(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return false;
    }
    return stack.getItem() != nullptr && stack.getItem() == Items::MAP;
}

} // namespace crafting
} // namespace mc
