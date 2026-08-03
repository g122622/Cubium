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

#include "item/crafting/special/TippedArrowRecipe.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/SpecialRecipe.hpp"
#include "common/item/potion/Potion.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "item/Items.hpp"
#include "item/items/weapon/TippedArrowItem.hpp"
#include "item/potion/PotionUtils.hpp"
#include <algorithm>
#include <vector>

namespace mc {
namespace crafting {

TippedArrowRecipe::TippedArrowRecipe(const ResourceLocation& id)
    : SpecialRecipe(id)
{}

bool TippedArrowRecipe::matches(const CraftingInventory& inventory) const
{
    i32 lingeringPotionSlot = -1;
    i32 arrowCount = 0;
    bool hasOtherItems = false;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }

        const Item* item = stack.getItem();

        // 检查是否是滞留药水
        if (item == Items::LINGERING_POTION) {
            // 只能有一个滞留药水
            if (lingeringPotionSlot != -1) {
                return false;
            }
            lingeringPotionSlot = i;
        }
        // 检查是否是箭
        else if (item == Items::ARROW) {
            ++arrowCount;
        }
        // 其他物品
        else {
            hasOtherItems = true;
            break;
        }
    }

    // 必须有一个滞留药水和至少一支箭，不能有其他物品
    return lingeringPotionSlot != -1 && arrowCount > 0 && !hasOtherItems;
}

ItemStack TippedArrowRecipe::assemble(const CraftingInventory& inventory) const
{
    // 找到滞留药水
    i32 lingeringPotionSlot = _findLingeringPotion(inventory);
    if (lingeringPotionSlot == -1) {
        return ItemStack::EMPTY;
    }

    // 统计箭的数量
    i32 arrowCount = _countArrows(inventory, lingeringPotionSlot);
    if (arrowCount <= 0) {
        return ItemStack::EMPTY;
    }

    // 限制最多8支药水箭
    arrowCount = std::min(arrowCount, 8);

    // 获取滞留药水的药水效果
    ItemStack lingeringPotion = inventory.getItem(lingeringPotionSlot);
    const potion::Potion* potion = potion::PotionUtils::getPotion(lingeringPotion);
    if (potion == nullptr) {
        return ItemStack::EMPTY;
    }

    // 创建药水箭
    ItemStack result(Items::TIPPED_ARROW, arrowCount);
    item::TippedArrowItem::setPotion(result, potion);

    // 复制自定义药水效果
    auto customEffects = potion::PotionUtils::getCustomEffects(lingeringPotion);
    if (!customEffects.empty()) {
        potion::PotionUtils::setCustomEffects(result, customEffects);
    }

    // 复制自定义药水颜色
    auto customColor = potion::PotionUtils::getCustomPotionColor(lingeringPotion);
    if (customColor.has_value()) {
        potion::PotionUtils::setCustomPotionColor(result, customColor);
    }

    return result;
}

std::vector<ItemStack> TippedArrowRecipe::getRemainingItems(const CraftingInventory& inventory) const
{
    // 滞留药水和箭都被消耗，没有剩余物品
    return std::vector<ItemStack>(inventory.getContainerSize());
}

i32 TippedArrowRecipe::_findLingeringPotion(const CraftingInventory& inventory) const noexcept
{
    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (!stack.isEmpty() && stack.getItem() == Items::LINGERING_POTION) {
            return i;
        }
    }
    return -1;
}

i32 TippedArrowRecipe::_countArrows(const CraftingInventory& inventory, i32 excludeSlot) const noexcept
{
    i32 count = 0;
    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        if (i == excludeSlot) {
            continue;
        }
        ItemStack stack = inventory.getItem(i);
        if (!stack.isEmpty() && stack.getItem() == Items::ARROW) {
            count += stack.getCount();
        }
    }
    return count;
}

} // namespace crafting
} // namespace mc
