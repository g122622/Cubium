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

#include "item/items/BannerPatternItem.hpp"
#include "text/TranslationTextComponent.hpp"

namespace mc {
namespace item {

BannerPatternItem::BannerPatternItem(blockentity::BannerPatternType pattern, ItemProperties properties)
    : Item(std::move(properties))
    , m_pattern(pattern)
{}

void BannerPatternItem::appendHoverText(
    const ItemStack& stack, std::vector<std::unique_ptr<text::ITextComponent>>& tooltip) const
{
    Item::appendHoverText(stack, tooltip);

    // 显示图案翻译名称
    std::string translationKey = "item.minecraft.banner_pattern." + blockentity::BannerPatterns::getFileName(m_pattern);
    tooltip.push_back(std::make_unique<text::TranslationTextComponent>(translationKey));
}

} // namespace item
} // namespace mc
