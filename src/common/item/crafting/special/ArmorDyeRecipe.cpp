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

#include "item/crafting/special/ArmorDyeRecipe.hpp"
#include "common/core/Types.hpp"
#include "common/entity/inventory/CraftingInventory.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/crafting/SpecialRecipe.hpp"
#include "common/item/items/DyeItemHelper.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/color/DyedItemColor.hpp"
#include "item/Items.hpp"
#include "item/items/armor/DyeableArmorItem.hpp"
#include <vector>

namespace mc {
namespace crafting {

ArmorDyeRecipe::ArmorDyeRecipe(const ResourceLocation& id)
    : SpecialRecipe(id)
{}

bool ArmorDyeRecipe::matches(const CraftingInventory& inventory) const
{
    i32 armorCount = 0;
    i32 dyeCount = 0;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }

        if (_isDyeableArmor(stack)) {
            ++armorCount;
        } else if (_isDye(stack)) {
            ++dyeCount;
        } else {
            // 有其他物品，不匹配
            return false;
        }
    }

    // 必须恰好有一个可染色盔甲和至少一个染料
    return armorCount == 1 && dyeCount >= 1;
}

ItemStack ArmorDyeRecipe::assemble(const CraftingInventory& inventory) const
{
    ItemStack armorStack;
    std::vector<u32> colors;

    // 收集盔甲和染料颜色
    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }

        if (_isDyeableArmor(stack)) {
            armorStack = stack.copy();
        } else if (_isDye(stack)) {
            // 从染料物品获取纹理漫反射色（对齐 vanilla DyeItem 的 getTextureDiffuseColor）
            const auto dyeColor = item::DyeItemHelper::dyeColorOfItem(stack.getItem());
            if (dyeColor.has_value()) {
                colors.push_back(dyeTextureDiffuseColor(*dyeColor));
            }
        }
    }

    if (armorStack.isEmpty() || colors.empty()) {
        return ItemStack::EMPTY;
    }

    // 已有颜色（曾经染过色）也参与平均；从未染色时以 nullopt 表示该份不占平均份额。
    const std::optional<u32> currentColor = item::items::DyeableArmorItem::getStoredColorOrNone(armorStack);
    const u32 mixedColor = DyedItemColor::applyDyes(currentColor, colors);

    // 颜色统一写回 display.color（DyeableArmorItem 与皮革马铠共用同一存储位置）
    item::items::DyeableArmorItem::setColor(armorStack, mixedColor);

    return armorStack;
}

std::vector<ItemStack> ArmorDyeRecipe::getRemainingItems(const CraftingInventory& inventory) const
{
    // 染色配方消耗所有染料，不消耗盔甲（盔甲变成染色结果）
    std::vector<ItemStack> remaining(inventory.getContainerSize());
    // 所有染料都被消耗
    return remaining;
}

bool ArmorDyeRecipe::_isDyeableArmor(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return false;
    }
    const Item* item = stack.getItem();
    if (item == nullptr) {
        return false;
    }
    // 用 ItemTags::DYEABLE 判定（含 4 件皮革盔甲、皮革马铠、狼铠），而非 dynamic_cast<DyeableArmorItem>。
    // 皮革马铠是独立的 HorseArmorItem，不是 DyeableArmorItem 子类，用类型判定会漏掉它。
    return item::tag::ItemTags::DYEABLE().contains(item);
}

bool ArmorDyeRecipe::_isDye(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return false;
    }
    return item::DyeItemHelper::isDyeItem(stack.getItem());
}

} // namespace crafting
} // namespace mc
