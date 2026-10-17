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

#include "DyeableArmorItem.hpp"
#include "common/core/Types.hpp"
#include "common/item/armor/ArmorMaterial.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/armor/ArmorItem.hpp"
#include <utility>

namespace mc {
namespace item::items {

namespace {

/// 颜色存储在物品的 display.color 子标签中（0xRRGGBB），与 vanilla 的 DYED_COLOR 组件对应。
constexpr const char* TAG_DISPLAY = "display";
constexpr const char* TAG_COLOR = "color";

} // namespace

DyeableArmorItem::DyeableArmorItem(
    const armor::ArmorMaterial& material, armor::ArmorSlot slot, ItemProperties properties)
    : ArmorItem(material, slot, std::move(properties))
{}

u32 DyeableArmorItem::getStoredColor(const ItemStack& stack, u32 defaultColor) noexcept
{
    const auto* displayTag = stack.getChildTag(TAG_DISPLAY);
    if (displayTag != nullptr) {
        const auto colorIter = displayTag->find(TAG_COLOR);
        if (colorIter != displayTag->end() && colorIter->is_number()) {
            return colorIter->get<u32>() & 0x00FFFFFFu;
        }
    }

    return defaultColor;
}

std::optional<u32> DyeableArmorItem::getStoredColorOrNone(const ItemStack& stack) noexcept
{
    const auto* displayTag = stack.getChildTag(TAG_DISPLAY);
    if (displayTag != nullptr) {
        const auto colorIter = displayTag->find(TAG_COLOR);
        if (colorIter != displayTag->end() && colorIter->is_number()) {
            return colorIter->get<u32>() & 0x00FFFFFFu;
        }
    }
    return std::nullopt;
}

u32 DyeableArmorItem::getColor(const ItemStack& stack) const noexcept
{
    return getStoredColor(stack, getDefaultColor());
}

void DyeableArmorItem::setColor(ItemStack& stack, u32 color) noexcept
{
    stack.getOrCreateChildTag(TAG_DISPLAY)[TAG_COLOR] = static_cast<u32>(color & 0x00FFFFFFu);
}

void DyeableArmorItem::clearColor(ItemStack& stack) noexcept
{
    auto* tag = stack.getTag();
    if (tag == nullptr || !tag->is_object()) {
        return;
    }

    auto displayIter = tag->find(TAG_DISPLAY);
    if (displayIter == tag->end() || !displayIter->is_object()) {
        return;
    }

    displayIter->erase(TAG_COLOR);
    if (displayIter->empty()) {
        stack.removeChildTag(TAG_DISPLAY);
    }
}

bool DyeableArmorItem::hasColor(const ItemStack& stack) noexcept
{
    return getStoredColorOrNone(stack).has_value();
}

} // namespace item::items
} // namespace mc
