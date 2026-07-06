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

#include "BundleTooltipRenderer.hpp"

#include <algorithm>

namespace mc::client::ui::screen::tooltip {

// ============================================================================
// 布局计算（无渲染依赖，可在测试中独立链接）
// ============================================================================

i32 BundleTooltipRenderer::slotCount(const BundleContents& contents) noexcept
{
    // 对应 MC 1.21.11 ClientBundleTooltip#slotCount：min(12, contents.size())
    const auto total = static_cast<i32>(contents.size());
    return std::min(MAX_VISIBLE_SLOTS, total);
}

i32 BundleTooltipRenderer::gridSizeY(const BundleContents& contents) noexcept
{
    // 对应 MC 1.21.11 ClientBundleTooltip#gridSizeY：positiveCeilDiv(slotCount, 4)
    const i32 slots = slotCount(contents);
    // positiveCeilDiv(a, b) = (a + b - 1) / b （a >= 0）
    return (slots + GRID_COLUMNS - 1) / GRID_COLUMNS;
}

i32 BundleTooltipRenderer::itemGridHeight(const BundleContents& contents) noexcept
{
    // 对应 MC 1.21.11 ClientBundleTooltip#itemGridHeight：gridSizeY * 24
    return gridSizeY(contents) * SLOT_SIZE;
}

i32 BundleTooltipRenderer::progressBarFill(const BundleContents& contents) noexcept
{
    // 对应 MC 1.21.11 ClientBundleTooltip#getProgressBarFill：
    //   Mth.clamp(Mth.mulAndTruncate(weight, 94), 0, 94)
    // MC 使用 Fraction，本项目使用整数权重（weight / MAX_WEIGHT）。
    // mulAndTruncate(frac, 94) = (numerator * 94) / denominator
    // 本项目：weight * 94 / MAX_WEIGHT
    const i64 weight = contents.weight();
    i64 fill = (weight * PROGRESSBAR_FILL_MAX) / BundleContents::MAX_WEIGHT;
    fill = std::clamp(fill, static_cast<i64>(0), static_cast<i64>(PROGRESSBAR_FILL_MAX));
    return static_cast<i32>(fill);
}

i32 BundleTooltipRenderer::amountOfHiddenItems(const BundleContents& contents) noexcept
{
    // 对应 MC 1.21.11 ClientBundleTooltip#getAmountOfHiddenItems：
    //   contents.itemCopyStream().skip(list.size()).mapToInt(ItemStack::getCount).sum()
    // 其中 list = getShownItems(numberOfItemsToShow)，list.size() = min(contents.size(), numberOfItemsToShow)
    // 注意：list.size() 不等于 slotCount（slotCount = min(12, size)），
    // 当 size > 12 时 list.size() = numberOfItemsToShow（可能小于 12）
    const i32 shownItemsCount = std::min(static_cast<i32>(contents.size()), contents.numberOfItemsToShow());
    const auto& items = contents.items();
    i32 sum = 0;
    for (Size i = static_cast<Size>(shownItemsCount); i < items.size(); ++i) {
        sum += items[i].getCount();
    }
    return sum;
}

i32 BundleTooltipRenderer::tooltipHeight(const BundleContents& contents, i32 emptyDescriptionHeight) noexcept
{
    // 对应 MC 1.21.11 ClientBundleTooltip#getHeight：
    // - 空：getEmptyBundleBackgroundHeight = emptyDescHeight + 13 + 8
    // - 非空：backgroundHeight = itemGridHeight + 13 + 8
    if (contents.isEmpty()) {
        return emptyDescriptionHeight + PROGRESSBAR_HEIGHT + BOTTOM_PADDING;
    }
    return itemGridHeight(contents) + PROGRESSBAR_HEIGHT + BOTTOM_PADDING;
}

std::pair<i32, i32> BundleTooltipRenderer::positionTooltip(
    i32 mouseX, i32 mouseY, i32 tooltipWidth, i32 tooltipHeight, i32 screenWidth, i32 screenHeight) noexcept
{
    // 与 AbstractContainerScreen::renderItemTooltip / TooltipRenderer::positionTooltip 一致
    f64 x = static_cast<f64>(mouseX) + MOUSE_OFFSET;
    f64 y = static_cast<f64>(mouseY) + MOUSE_OFFSET;

    if (x + static_cast<f64>(tooltipWidth) > static_cast<f64>(screenWidth)) {
        x = static_cast<f64>(mouseX) - MOUSE_OFFSET - static_cast<f64>(tooltipWidth);
    }
    if (y + static_cast<f64>(tooltipHeight) > static_cast<f64>(screenHeight)) {
        y = static_cast<f64>(mouseY) - MOUSE_OFFSET - static_cast<f64>(tooltipHeight);
    }

    x = std::max(MIN_POSITION, x);
    y = std::max(MIN_POSITION, y);

    return {static_cast<i32>(x), static_cast<i32>(y)};
}

} // namespace mc::client::ui::screen::tooltip
