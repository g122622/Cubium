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

#include "client/renderer/trident/item/ItemRenderer.hpp"

#include <algorithm>
#include <string>

namespace mc::client::ui::screen::tooltip {

// ============================================================================
// 渲染主入口（依赖 GuiRenderer + ItemRenderer，仅在客户端构建中链接）
// ============================================================================

void BundleTooltipRenderer::render(renderer::trident::gui::GuiRenderer& gui,
    renderer::trident::item::ItemRenderer& itemRenderer,
    const ItemStack& stack,
    i32 mouseX,
    i32 mouseY,
    i32 screenWidth,
    i32 screenHeight,
    u32 borderColor)
{
    const BundleContents contents = BundleContents::fromItemStack(stack);

    if (contents.isEmpty()) {
        // 空收纳袋：渲染描述文本 + 空进度条
        // 对应 MC 1.21.11 ClientBundleTooltip#renderEmptyBundleTooltip
        const std::string emptyDesc = "Bundle is empty";
        // 文本高度 = font 高度（默认 9px）
        const u32 fontHeight = gui.getFontHeight();
        const i32 emptyDescHeight = static_cast<i32>(fontHeight);

        const i32 totalHeight = tooltipHeight(contents, emptyDescHeight);
        const auto [x, y] = positionTooltip(mouseX, mouseY, TOOLTIP_WIDTH, totalHeight, screenWidth, screenHeight);

        // 背景 + 边框
        gui.fillRect(static_cast<f64>(x),
            static_cast<f64>(y),
            static_cast<f64>(TOOLTIP_WIDTH),
            static_cast<f64>(totalHeight),
            BACKGROUND_COLOR);
        gui.drawRect(static_cast<f64>(x),
            static_cast<f64>(y),
            static_cast<f64>(TOOLTIP_WIDTH),
            static_cast<f64>(totalHeight),
            borderColor);

        // 空描述文本（居中）
        const f64 textX = static_cast<f64>(x) + static_cast<f64>(TOOLTIP_WIDTH) / 2.0;
        const f64 textY = static_cast<f64>(y) + static_cast<f64>(TOP_PADDING);
        gui.drawTextCentered(emptyDesc, textX, textY, EMPTY_TEXT_COLOR);

        // 进度条（空）
        const i32 progressBarY = y + TOP_PADDING + emptyDescHeight + PROGRESSBAR_MARGIN_Y;
        const i32 progressBarX = x + SIDE_PADDING;
        // 进度条背景
        gui.fillRect(static_cast<f64>(progressBarX),
            static_cast<f64>(progressBarY),
            static_cast<f64>(PROGRESSBAR_WIDTH),
            static_cast<f64>(PROGRESSBAR_HEIGHT),
            PROGRESSBAR_BG_COLOR);
        // 进度条边框
        gui.drawRect(static_cast<f64>(progressBarX),
            static_cast<f64>(progressBarY),
            static_cast<f64>(PROGRESSBAR_WIDTH),
            static_cast<f64>(PROGRESSBAR_HEIGHT),
            PROGRESSBAR_BORDER_COLOR);

        // 空进度条文本（居中）
        const std::string emptyText = "Empty";
        const f64 barTextX = static_cast<f64>(progressBarX) + static_cast<f64>(PROGRESSBAR_WIDTH) / 2.0;
        const f64 barTextY = static_cast<f64>(progressBarY) + 3.0;
        gui.drawTextCentered(emptyText, barTextX, barTextY, TEXT_COLOR);
        return;
    }

    // 非空收纳袋：渲染物品网格 + 进度条
    // 对应 MC 1.21.11 ClientBundleTooltip#renderBundleWithItemsTooltip
    const i32 gridH = itemGridHeight(contents);
    const i32 totalHeight = tooltipHeight(contents);
    const auto [x, y] = positionTooltip(mouseX, mouseY, TOOLTIP_WIDTH, totalHeight, screenWidth, screenHeight);

    // 背景 + 边框
    gui.fillRect(static_cast<f64>(x),
        static_cast<f64>(y),
        static_cast<f64>(TOOLTIP_WIDTH),
        static_cast<f64>(totalHeight),
        BACKGROUND_COLOR);
    gui.drawRect(static_cast<f64>(x),
        static_cast<f64>(y),
        static_cast<f64>(TOOLTIP_WIDTH),
        static_cast<f64>(totalHeight),
        borderColor);

    // 物品网格
    // MC 算法：从右下角开始填充，i 从 1 到 gridSizeY，j 从 1 到 4
    //   slotX = (x + contentXOffset + GRID_WIDTH) - j * SLOT_SIZE
    //   slotY = (y + gridHeight) - i * SLOT_SIZE
    // 由于本项目 contentXOffset = (TOOLTIP_WIDTH - GRID_WIDTH) / 2 = SIDE_PADDING，
    // 网格起始 X = x + SIDE_PADDING，结束 X = x + SIDE_PADDING + GRID_WIDTH
    const bool hasOverflow = static_cast<i32>(contents.size()) > MAX_VISIBLE_SLOTS;

    // 对应 MC: List<ItemStack> list = getShownItems(getNumberOfItemsToShow())
    // shownItemsCount = min(contents.size(), numberOfItemsToShow)
    const i32 shownItemsCount = std::min(static_cast<i32>(contents.size()), contents.numberOfItemsToShow());
    const auto& items = contents.items();

    // 收纳袋可能选中的项索引（在完整 items 中的位置，-1 表示无）
    // MC 中 selectedItem 是相对于完整列表的索引（最新插入为 0）
    const i32 selectedIndex = contents.selectedItem();

    // 渲染网格
    // MC 使用 1-based 索引从右下角向上填充：
    //   for i in 1..=gridSizeY:
    //     for j in 1..=4:
    //       slotX = right - j * SLOT_SIZE
    //       slotY = bottom - i * SLOT_SIZE
    //       k = (i-1) * 4 + (j-1) + 1  （1-based slot 序号）
    //       if shouldRenderSurplusText: renderCount
    //       elif shouldRenderItemSlot(k): renderSlot(k)
    //
    // 在 renderSlot 中：
    //   itemIndex = list.size() - k  （0-based，最新插入在 list.size()-1）
    //   itemstack = list.get(itemIndex)
    //   isSelected = (itemIndex == contents.selectedItem())
    const i32 gridRight = x + SIDE_PADDING + GRID_WIDTH;
    const i32 gridBottom = y + gridH;

    i32 slotIndex = 1; // 1-based
    for (i32 row = 1; row <= gridSizeY(contents); ++row) {
        for (i32 col = 1; col <= GRID_COLUMNS; ++col) {
            const i32 slotX = gridRight - col * SLOT_SIZE;
            const i32 slotY = gridBottom - row * SLOT_SIZE;

            // 检查是否应该渲染 "+N" 溢出文本
            // MC: shouldRenderSurplusText(flag, i1, l) = flag && i1 * l == 1
            //   即 col * row == 1（左下角第一格）
            if (hasOverflow && col == 1 && row == 1) {
                // 渲染 "+N" 居中文本
                // 对应 MC: renderCount(j1, k1, getAmountOfHiddenItems(list), ...)
                //   drawCenteredString(font, "+" + count, x + 12, y + 10, -1)
                const i32 hiddenCount = amountOfHiddenItems(contents);
                const std::string overflowText = "+" + std::to_string(hiddenCount);
                const f64 textCenterX = static_cast<f64>(slotX) + static_cast<f64>(SLOT_SIZE) / 2.0;
                const f64 textY = static_cast<f64>(slotY) + 10.0;
                gui.drawTextCentered(overflowText, textCenterX, textY, TEXT_COLOR);
                continue;
            }

            // 检查是否应该渲染物品槽位
            // MC: shouldRenderItemSlot(list, k) = list.size() >= k
            if (shownItemsCount >= slotIndex) {
                // 对应 MC: i = list.size() - slotIndex（items 索引，0-based）
                const i32 itemIndex = shownItemsCount - slotIndex;
                if (itemIndex < 0 || itemIndex >= static_cast<i32>(items.size())) {
                    ++slotIndex;
                    continue;
                }

                const bool isSelected = (itemIndex == selectedIndex);

                // 槽位背景
                if (isSelected) {
                    gui.fillRect(static_cast<f64>(slotX),
                        static_cast<f64>(slotY),
                        static_cast<f64>(SLOT_SIZE),
                        static_cast<f64>(SLOT_SIZE),
                        SELECTED_BACK_COLOR);
                } else {
                    gui.fillRect(static_cast<f64>(slotX),
                        static_cast<f64>(slotY),
                        static_cast<f64>(SLOT_SIZE),
                        static_cast<f64>(SLOT_SIZE),
                        SLOT_BACKGROUND_COLOR);
                }

                // 渲染物品图标
                // 对应 MC: renderItem(itemstack, x + 4, y + 4, k)
                const ItemStack& itemStack = items[static_cast<Size>(itemIndex)];
                if (!itemStack.isEmpty()) {
                    itemRenderer.renderItem(gui,
                        itemStack,
                        static_cast<f64>(slotX + SLOT_ICON_OFFSET),
                        static_cast<f64>(slotY + SLOT_ICON_OFFSET),
                        16.0);

                    // 渲染物品数量（对应 MC renderItemDecorations）
                    if (itemStack.getCount() > 1) {
                        const std::string countText = std::to_string(itemStack.getCount());
                        const f64 countX = static_cast<f64>(slotX + SLOT_SIZE - 2);
                        const f64 countY = static_cast<f64>(slotY + SLOT_SIZE - 8);
                        gui.drawText(countText, countX, countY, TEXT_COLOR, true);
                    }
                }

                // 选中项前景高亮（对应 MC blitSprite SLOT_HIGHLIGHT_FRONT_SPRITE）
                if (isSelected) {
                    gui.drawRect(static_cast<f64>(slotX),
                        static_cast<f64>(slotY),
                        static_cast<f64>(SLOT_SIZE),
                        static_cast<f64>(SLOT_SIZE),
                        SELECTED_FRONT_COLOR);
                }
            }

            ++slotIndex;
        }
    }

    // 进度条
    const i32 progressBarY = y + gridH + PROGRESSBAR_MARGIN_Y;
    const i32 progressBarX = x + SIDE_PADDING;

    // 进度条背景
    gui.fillRect(static_cast<f64>(progressBarX),
        static_cast<f64>(progressBarY),
        static_cast<f64>(PROGRESSBAR_WIDTH),
        static_cast<f64>(PROGRESSBAR_HEIGHT),
        PROGRESSBAR_BG_COLOR);

    // 进度条填充
    const i32 fillWidth = progressBarFill(contents);
    const i64 weight = contents.weight();
    const bool isFull = weight >= BundleContents::MAX_WEIGHT;
    const u32 fillColor = isFull ? PROGRESSBAR_FULL_COLOR : PROGRESSBAR_FILL_COLOR;
    if (fillWidth > 0) {
        // MC: blitSprite(getProgressBarTexture(), x+1, y, getProgressBarFill(), 13)
        // 即填充从 progressBarX + 1 开始，宽度为 fillWidth
        gui.fillRect(static_cast<f64>(progressBarX + 1),
            static_cast<f64>(progressBarY),
            static_cast<f64>(fillWidth),
            static_cast<f64>(PROGRESSBAR_HEIGHT),
            fillColor);
    }

    // 进度条边框
    gui.drawRect(static_cast<f64>(progressBarX),
        static_cast<f64>(progressBarY),
        static_cast<f64>(PROGRESSBAR_WIDTH),
        static_cast<f64>(PROGRESSBAR_HEIGHT),
        PROGRESSBAR_BORDER_COLOR);

    // 满时文本（居中）
    if (isFull) {
        const std::string fullText = "Full";
        const f64 barTextX = static_cast<f64>(progressBarX) + static_cast<f64>(PROGRESSBAR_WIDTH) / 2.0;
        const f64 barTextY = static_cast<f64>(progressBarY) + 3.0;
        gui.drawTextCentered(fullText, barTextX, barTextY, TEXT_COLOR);
    }
}

} // namespace mc::client::ui::screen::tooltip
