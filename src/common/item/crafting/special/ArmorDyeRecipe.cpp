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
#include "common/resource/ResourceLocation.hpp"
#include "common/util/color/DyeColor.hpp"
#include "item/Items.hpp"
#include "item/items/armor/DyeableArmorItem.hpp"
#include <vector>

namespace mc {
namespace crafting {

namespace {

/**
 * @brief 将 DyeColor 转换为 RGB 整数值
 *
 * @param color 染料颜色
 * @return RGB 整数值（0xRRGGBB 格式）
 */
u32 _dyeColorToRGB(DyeColor color)
{
    // 盔甲染色的颜色混合取纹理漫反射色（与烟花色不同）
    switch (color) {
        case DyeColor::White:
            return 0xF9FFFE; // #F9FFFE
        case DyeColor::Orange:
            return 0xF9801D; // #F9801D
        case DyeColor::Magenta:
            return 0xC74EBD; // #C74EBD
        case DyeColor::LightBlue:
            return 0x3AB3DA; // #3AB3DA
        case DyeColor::Yellow:
            return 0xFED83D; // #FED83D
        case DyeColor::Lime:
            return 0x80C71F; // #80C71F
        case DyeColor::Pink:
            return 0xF38BAA; // #F38BAA
        case DyeColor::Gray:
            return 0x474F52; // #474F52
        case DyeColor::LightGray:
            return 0x9D9D97; // #9D9D97
        case DyeColor::Cyan:
            return 0x169C9C; // #169C9C
        case DyeColor::Purple:
            return 0x8932B8; // #8932B8
        case DyeColor::Blue:
            return 0x3C44AA; // #3C44AA
        case DyeColor::Brown:
            return 0x835432; // #835432
        case DyeColor::Green:
            return 0x5E7C16; // #5E7C16
        case DyeColor::Red:
            return 0xB02E26; // #B02E26
        case DyeColor::Black:
            return 0x1D1D21; // #1D1D21
        default:
            return 0xF9FFFE; // 默认白色
    }
}

} // namespace

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
            // 从染料物品获取颜色
            const auto dyeColor = item::DyeItemHelper::dyeColorOfItem(stack.getItem());
            if (dyeColor.has_value()) {
                colors.push_back(_dyeColorToRGB(*dyeColor));
            }
        }
    }

    if (armorStack.isEmpty() || colors.empty()) {
        return ItemStack::EMPTY;
    }

    // 获取当前颜色（如果有）
    u32 currentColor = 0xFFFFFF; // 默认白色
    const Item* item = armorStack.getItem();
    if (item != nullptr) {
        const auto* dyeableItem = dynamic_cast<const item::items::DyeableArmorItem*>(item);
        if (dyeableItem != nullptr) {
            if (item::items::DyeableArmorItem::hasColor(armorStack)) {
                currentColor = dyeableItem->getColor(armorStack);
            } else {
                currentColor = dyeableItem->getDefaultColor();
            }
        }
    }

    // 混合颜色
    for (u32 dyeColor : colors) {
        currentColor = _mixColors(currentColor, dyeColor);
    }

    // 设置颜色
    item::items::DyeableArmorItem::setColor(armorStack, currentColor);

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
    return dynamic_cast<const item::items::DyeableArmorItem*>(item) != nullptr;
}

bool ArmorDyeRecipe::_isDye(const ItemStack& stack)
{
    if (stack.isEmpty()) {
        return false;
    }
    return item::DyeItemHelper::isDyeItem(stack.getItem());
}

u32 ArmorDyeRecipe::_mixColors(u32 color1, u32 color2)
{
    // 颜色混合算法：将 RGB 分量分别取平均
    i32 r1 = static_cast<i32>((color1 >> 16) & 0xFF);
    i32 g1 = static_cast<i32>((color1 >> 8) & 0xFF);
    i32 b1 = static_cast<i32>(color1 & 0xFF);

    i32 r2 = static_cast<i32>((color2 >> 16) & 0xFF);
    i32 g2 = static_cast<i32>((color2 >> 8) & 0xFF);
    i32 b2 = static_cast<i32>(color2 & 0xFF);

    i32 r = (r1 + r2) / 2;
    i32 g = (g1 + g2) / 2;
    i32 b = (b1 + b2) / 2;

    return (0xFF << 24) | (static_cast<u32>(r) << 16) | (static_cast<u32>(g) << 8) | static_cast<u32>(b);
}

} // namespace crafting
} // namespace mc
