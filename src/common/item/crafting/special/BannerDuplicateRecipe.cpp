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
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "item/crafting/special/BannerDuplicateRecipe.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/crafting/SpecialRecipe.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/color/DyeColor.hpp"
#include "item/items/block/BannerItem.hpp"
#include "world/blockentity/interactive/BannerEntity.hpp"
#include <optional>
#include <vector>

namespace mc {
namespace crafting {

BannerDuplicateRecipe::BannerDuplicateRecipe(const ResourceLocation& id)
    : SpecialRecipe(id)
{}

bool BannerDuplicateRecipe::matches(const CraftingInventory& inventory) const
{
    auto pair = _findBannerPair(inventory);
    return pair.has_value();
}

ItemStack BannerDuplicateRecipe::assemble(const CraftingInventory& inventory) const
{
    auto pair = _findBannerPair(inventory);
    if (!pair.has_value()) {
        return ItemStack();
    }

    // 复制源旗帜
    const ItemStack& sourceStack = inventory.getItem(pair->sourceIndex);
    ItemStack result = sourceStack.copy();
    result.setCount(1);
    return result;
}

std::vector<ItemStack> BannerDuplicateRecipe::getRemainingItems(const CraftingInventory& inventory) const
{
    std::vector<ItemStack> remaining(inventory.getContainerSize());

    auto pair = _findBannerPair(inventory);
    if (!pair.has_value()) {
        return remaining;
    }

    // 源旗帜（有图案的）保留返回
    const ItemStack& sourceStack = inventory.getItem(pair->sourceIndex);
    remaining[pair->sourceIndex] = sourceStack.copy();
    remaining[pair->sourceIndex].setCount(1);

    return remaining;
}

std::optional<BannerDuplicateRecipe::BannerPair> BannerDuplicateRecipe::_findBannerPair(
    const CraftingInventory& inventory) const
{
    DyeColor color = DyeColor::White;
    i32 sourceIndex = -1; // 有图案的旗帜
    i32 targetIndex = -1; // 无图案的旗帜
    i32 bannerCount = 0;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        const ItemStack& stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }

        const auto* item = stack.getItem();
        if (item == nullptr) {
            continue;
        }

        const auto* bannerItem = dynamic_cast<const item::BannerItem*>(item);
        if (bannerItem == nullptr) {
            // 有非旗帜物品，不匹配
            return std::nullopt;
        }

        // 两个旗帜必须是相同颜色
        if (bannerCount == 0) {
            color = bannerItem->getColor();
        } else if (bannerItem->getColor() != color) {
            return std::nullopt;
        }

        i32 patternCount = blockentity::BannerEntity::getPatternCount(stack);
        if (patternCount > blockentity::BannerEntity::MAX_PATTERNS) {
            // 图案超过6层，不匹配
            return std::nullopt;
        }

        if (patternCount > 0) {
            // 有图案的旗帜
            if (sourceIndex >= 0) {
                // 只能有一个有图案的旗帜
                return std::nullopt;
            }
            sourceIndex = i;
        } else {
            // 无图案的旗帜
            if (targetIndex >= 0) {
                // 只能有一个无图案的旗帜
                return std::nullopt;
            }
            targetIndex = i;
        }

        ++bannerCount;
    }

    // 必须恰好2个旗帜（一个有图案，一个无图案）
    if (sourceIndex < 0 || targetIndex < 0) {
        return std::nullopt;
    }

    return BannerPair{sourceIndex, targetIndex, color};
}

} // namespace crafting
} // namespace mc
