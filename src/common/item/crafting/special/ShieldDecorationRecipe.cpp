/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software being
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

#include "item/crafting/special/ShieldDecorationRecipe.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/crafting/SpecialRecipe.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "item/Items.hpp"
#include "item/items/block/BannerItem.hpp"
#include <optional>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace crafting {

ShieldDecorationRecipe::ShieldDecorationRecipe(const ResourceLocation& id)
    : SpecialRecipe(id)
{}

bool ShieldDecorationRecipe::matches(const CraftingInventory& inventory) const
{
    auto pair = _findPair(inventory);
    return pair.has_value();
}

ItemStack ShieldDecorationRecipe::assemble(const CraftingInventory& inventory) const
{
    auto pair = _findPair(inventory);
    if (!pair.has_value()) {
        return ItemStack();
    }

    const ItemStack& shieldStack = inventory.getItem(pair->shieldIndex);
    const ItemStack& bannerStack = inventory.getItem(pair->bannerIndex);

    // 复制盾牌作为结果
    ItemStack result = shieldStack.copy();
    result.setCount(1);

    // 获取旗帜的BlockEntityTag
    const nlohmann::json* bannerTag = bannerStack.getChildTag("BlockEntityTag");

    // 设置BlockEntityTag到盾牌
    nlohmann::json& resultTag = result.getOrCreateChildTag("BlockEntityTag");

    if (bannerTag != nullptr) {
        // 复制旗帜的图案数据
        if (bannerTag->contains("Patterns")) {
            resultTag["Patterns"] = (*bannerTag)["Patterns"];
        }
    }

    // 添加Base字段（旗帜底色）
    const auto* bannerItem = dynamic_cast<const item::BannerItem*>(bannerStack.getItem());
    if (bannerItem != nullptr) {
        resultTag["Base"] = static_cast<i32>(bannerItem->getColor());
    }

    return result;
}

std::vector<ItemStack> ShieldDecorationRecipe::getRemainingItems(const CraftingInventory& inventory) const
{
    // 旗帜保留返回（不消耗）
    std::vector<ItemStack> remaining(inventory.getContainerSize());

    auto pair = _findPair(inventory);
    if (!pair.has_value()) {
        return remaining;
    }

    const ItemStack& bannerStack = inventory.getItem(pair->bannerIndex);
    remaining[pair->bannerIndex] = bannerStack.copy();
    remaining[pair->bannerIndex].setCount(1);

    return remaining;
}

std::optional<ShieldDecorationRecipe::ShieldBannerPair> ShieldDecorationRecipe::_findPair(
    const CraftingInventory& inventory) const
{
    i32 shieldIndex = -1;
    i32 bannerIndex = -1;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        const ItemStack& stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }

        const auto* item = stack.getItem();
        if (item == nullptr) {
            continue;
        }

        // 检查是否是旗帜
        const auto* bannerItem = dynamic_cast<const item::BannerItem*>(item);
        if (bannerItem != nullptr) {
            if (bannerIndex >= 0) {
                // 只能有一个旗帜
                return std::nullopt;
            }
            bannerIndex = i;
            continue;
        }

        // 检查是否是盾牌
        if (item == Items::SHIELD) {
            if (shieldIndex >= 0) {
                // 只能有一个盾牌
                return std::nullopt;
            }
            // 盾牌不能已有图案
            if (stack.getChildTag("BlockEntityTag") != nullptr) {
                return std::nullopt;
            }
            shieldIndex = i;
            continue;
        }

        // 有其他物品，不匹配
        return std::nullopt;
    }

    // 必须有盾牌和旗帜
    if (shieldIndex < 0 || bannerIndex < 0) {
        return std::nullopt;
    }

    return ShieldBannerPair{shieldIndex, bannerIndex};
}

} // namespace crafting
} // namespace mc
