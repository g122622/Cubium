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

#include "item/crafting/special/FireworkStarFadeRecipe.hpp"

#include "common/core/Types.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/component/FireworkExplosion.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/SpecialRecipe.hpp"
#include "common/item/items/DyeItemHelper.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/color/DyeColor.hpp"
#include "item/Items.hpp"
#include <utility>
#include <vector>

namespace mc {
namespace crafting {

FireworkStarFadeRecipe::FireworkStarFadeRecipe(const ResourceLocation& id)
    : SpecialRecipe(id)
{}

bool FireworkStarFadeRecipe::matches(const CraftingInventory& inventory) const
{
    i32 ingredientCount = 0;
    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        if (!inventory.getItem(i).isEmpty()) {
            ++ingredientCount;
        }
    }
    if (ingredientCount < 2) {
        return false;
    }

    bool hasDye = false;
    bool hasStar = false;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        const ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }
        const Item* item = stack.getItem();

        if (item::DyeItemHelper::isDyeItem(item)) {
            hasDye = true;
        } else {
            if (item != Items::FIREWORK_STAR) {
                return false;
            }
            if (hasStar) {
                return false; // 至多一个烟花之星
            }
            hasStar = true;
        }
    }

    return hasStar && hasDye;
}

ItemStack FireworkStarFadeRecipe::assemble(const CraftingInventory& inventory) const
{
    std::vector<i32> fadeColors;
    ItemStack star;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        const ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }
        const Item* item = stack.getItem();

        if (const auto dyeColor = item::DyeItemHelper::dyeColorOfItem(item); dyeColor.has_value()) {
            fadeColors.push_back(static_cast<i32>(dyeFireworkColor(*dyeColor)));
        } else if (item == Items::FIREWORK_STAR) {
            star = stack.copy();
            star.setCount(1);
        }
    }

    if (star.isEmpty() || fadeColors.empty()) {
        return ItemStack::EMPTY;
    }

    // vanilla `itemstack.update(FIREWORK_EXPLOSION, DEFAULT, list, withFadeColors)`：
    // 未携带组件时以 DEFAULT 为初值，仅替换 fade_colors。
    item::component::FireworkExplosion explosion = star.getFireworkExplosion() != nullptr
        ? *star.getFireworkExplosion()
        : item::component::FireworkExplosion::defaultExplosion();
    star.setFireworkExplosion(explosion.withFadeColors(std::move(fadeColors)));
    return star;
}

std::vector<ItemStack> FireworkStarFadeRecipe::getRemainingItems(const CraftingInventory& inventory) const
{
    return RecipeUtils::getDefaultRemainingItems(inventory);
}

} // namespace crafting
} // namespace mc
