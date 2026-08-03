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

#include "MapExtendingRecipe.hpp"
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

MapExtendingRecipe::MapExtendingRecipe(const ResourceLocation& id)
    : SpecialRecipe(id)
{}

bool MapExtendingRecipe::matches(const CraftingInventory& inventory) const
{
    bool hasFilledMap = false;
    bool hasPaper = false;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }

        if (_isExtendableMap(stack)) {
            if (hasFilledMap) {
                return false;
            }
            hasFilledMap = true;
        } else if (_isPaper(stack)) {
            hasPaper = true;
        } else {
            return false;
        }
    }

    return hasFilledMap && hasPaper;
}

ItemStack MapExtendingRecipe::assemble(const CraftingInventory& inventory) const
{
    ItemStack filledMap;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (_isExtendableMap(stack)) {
            filledMap = stack;
            break;
        }
    }

    if (filledMap.isEmpty()) {
        return ItemStack::EMPTY;
    }

    // 创建缩放级别+1的新地图
    // 设置 map_scale_direction NBT 标签，在合成结果取出时处理缩放
    ItemStack result = filledMap.copy();
    result.setCount(1);
    auto& tag = result.getOrCreateTag();
    tag["map_scale_direction"] = 1;

    return result;
}

std::vector<ItemStack> MapExtendingRecipe::getRemainingItems(const CraftingInventory& inventory) const
{
    // 原地图被消耗，无剩余
    return std::vector<ItemStack>(inventory.getContainerSize());
}

bool MapExtendingRecipe::_isExtendableMap(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return false;
    }

    if (!item::items::FilledMapItem::isFilledMap(stack)) {
        return false;
    }

    // 探险地图不可扩展
    if (item::items::FilledMapItem::isExplorationMap(stack)) {
        return false;
    }

    // 检查缩放级别是否已达到上限
    const auto* tag = stack.getTag();
    if (tag != nullptr && tag->contains("map_scale_direction")) {
        // 已经在缩放过程中的地图不能再扩展
        return false;
    }

    return true;
}

bool MapExtendingRecipe::_isPaper(const ItemStack& stack)
{
    // isEmpty() 已经包含了 m_item == nullptr 的检查，无需重复检查
    if (stack.isEmpty()) {
        return false;
    }
    return stack.getItem() == Items::PAPER;
}

} // namespace crafting
} // namespace mc
