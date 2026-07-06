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

#pragma once

#include "client/renderer/trident/gui/GuiRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/special/bundle/BundleContents.hpp"

namespace mc::client::renderer::trident::item {
class ItemRenderer;
}

namespace mc::client::ui::screen::tooltip {

// 引入 BundleContents 类型别名，简化本文件中的引用
// BundleContents 实际定义在 mc::item::items 命名空间中
using mc::item::items::BundleContents;

/**
 * @brief 收纳袋物品专用 tooltip 渲染器
 *
 * 对应 MC 1.21.11 的 net.minecraft.client.gui.screens.inventory.tooltip.ClientBundleTooltip。
 *
 * 与普通文本 tooltip 不同，收纳袋 tooltip 包含：
 * - 内容物网格（4 列 × N 行，每格 24×24px，含 4px 边距）
 * - 选中项高亮（背景 + 前景两层）
 * - 满度进度条（96×13px，含 1px 边框）
 * - 满时/空时状态文本
 * - 内容物 > 12 项时的 "+N" 溢出指示
 *
 * 由于本项目的 tooltip 渲染不依赖 MC 的 ClientTooltipComponent 架构，
 * 此处将其作为独立工具类实现，由 AbstractContainerScreen / CreativeScreen
 * 在 renderItemTooltip 中调用。
 *
 * 渲染原语使用 GuiRenderer 的纯色矩形（fillRect/drawRect）和文本（drawText），
 * 与现有 AbstractContainerScreen::renderItemTooltip 风格一致。
 *
 * 布局常量与 MC 1.21.11 ClientBundleTooltip 对齐：
 * - SLOT_SIZE = 24（每格边长，含 4px 边距）
 * - GRID_WIDTH = 96（4 列 × 24px）
 * - PROGRESSBAR_HEIGHT = 13
 * - PROGRESSBAR_WIDTH = 96
 * - PROGRESSBAR_FILL_MAX = 94（去除 1px 边框后的最大填充宽度）
 * - PROGRESSBAR_MARGIN_Y = 4（网格与进度条之间的垂直间距）
 * - BOTTOM_PADDING = 8（进度条下方留白）
 * - TOP_PADDING = 4（顶部留白）
 * - SIDE_PADDING = 4（左右两侧留白，使总宽度从 96 扩展到 96+8=104，与 MC 一致）
 *
 * 参考: net.minecraft.client.gui.screens.inventory.tooltip.ClientBundleTooltip
 */
class BundleTooltipRenderer {
public:
    // ========== 布局常量（与 MC 1.21.11 ClientBundleTooltip 一致）==========

    /// 单格边长（含 4px 内边距，物品图标渲染在 16×16 区域内）
    static constexpr i32 SLOT_SIZE = 24;

    /// 网格列数（固定 4 列）
    static constexpr i32 GRID_COLUMNS = 4;

    /// 网格总宽度（4 列 × 24px = 96px）
    static constexpr i32 GRID_WIDTH = GRID_COLUMNS * SLOT_SIZE;

    /// 进度条高度
    static constexpr i32 PROGRESSBAR_HEIGHT = 13;

    /// 进度条宽度（与网格同宽）
    static constexpr i32 PROGRESSBAR_WIDTH = GRID_WIDTH;

    /// 进度条最大填充宽度（去除 1px 边框）
    static constexpr i32 PROGRESSBAR_FILL_MAX = PROGRESSBAR_WIDTH - 2;

    /// 网格与进度条之间的垂直间距
    static constexpr i32 PROGRESSBAR_MARGIN_Y = 4;

    /// 顶部留白
    static constexpr i32 TOP_PADDING = 4;

    /// 进度条下方留白
    static constexpr i32 BOTTOM_PADDING = 8;

    /// 左右两侧留白（各 4px，使总宽度为 GRID_WIDTH + SIDE_PADDING * 2 = 104）
    static constexpr i32 SIDE_PADDING = 4;

    /// 显示的最大物品槽位数（超过则显示 "+N" 溢出指示）
    static constexpr i32 MAX_VISIBLE_SLOTS = 12;

    /// tooltip 总宽度
    static constexpr i32 TOOLTIP_WIDTH = GRID_WIDTH + SIDE_PADDING * 2;

    /// 物品图标在槽位内的偏移（(SLOT_SIZE - 16) / 2 = 4）
    static constexpr i32 SLOT_ICON_OFFSET = (SLOT_SIZE - 16) / 2;

    // ========== 颜色常量（与 AbstractContainerScreen::renderItemTooltip 一致）==========

    /// 半透明深色背景
    static constexpr u32 BACKGROUND_COLOR = 0xF0100010;

    /// 紫色边框（默认容器屏幕）
    static constexpr u32 BORDER_COLOR = 0x505000FF;

    /// 创造屏幕边框色
    static constexpr u32 CREATIVE_BORDER_COLOR = 0xFF4DA3FF;

    /// 白色文本
    static constexpr u32 TEXT_COLOR = 0xFFFFFFFF;

    /// 选中项背景高亮（半透明白）
    static constexpr u32 SELECTED_BACK_COLOR = 0x80FFFFFF;

    /// 选中项前景高亮（半透明黑边）
    static constexpr u32 SELECTED_FRONT_COLOR = 0x60000000;

    /// 普通槽位背景
    static constexpr u32 SLOT_BACKGROUND_COLOR = 0x40FFFFFF;

    /// 进度条背景（空）
    static constexpr u32 PROGRESSBAR_BG_COLOR = 0x40000000;

    /// 进度条填充色（未满）
    static constexpr u32 PROGRESSBAR_FILL_COLOR = 0xFFE0E0E0;

    /// 进度条填充色（已满）
    static constexpr u32 PROGRESSBAR_FULL_COLOR = 0xFFFFFF80;

    /// 进度条边框
    static constexpr u32 PROGRESSBAR_BORDER_COLOR = 0xFF000000;

    /// 空收纳袋描述文本颜色（与 MC 一致，灰褐色 -5592406 = 0xFFAAAA56）
    static constexpr u32 EMPTY_TEXT_COLOR = 0xFFAAAA56;

    // ========== 鼠标偏移（与普通 tooltip 一致）==========

    static constexpr f64 MOUSE_OFFSET = 12.0;
    static constexpr f64 MIN_POSITION = 4.0;

    // ========== 主接口 ==========

    /**
     * @brief 渲染收纳袋 tooltip
     *
     * 根据内容物状态选择渲染路径：
     * - 空：渲染空描述文本 + 空进度条
     * - 非空：渲染物品网格 + 进度条（+ 满时文本 / +N 溢出指示）
     *
     * TODO: 当前使用 GuiRenderer 纯色矩形渲染所有视觉元素，与 MC 1.21.11 原版
     * ClientBundleTooltip 的 sprite 纹理渲染有视觉差异。待 GuiTextureManager 支持
     * 加载相关 sprite 后应升级为纹理化渲染。详见 README.md 中的
     * "TODO：升级到纹理化渲染" 章节。
     *
     * @param gui GUI 渲染器
     * @param itemRenderer 物品渲染器（用于渲染网格中的物品图标）
     * @param stack 收纳袋物品堆（已通过 BundleItem::isBundleItem 校验）
     * @param mouseX 鼠标 X 坐标
     * @param mouseY 鼠标 Y 坐标
     * @param screenWidth 屏幕宽度
     * @param screenHeight 屏幕高度
     * @param borderColor 边框颜色（默认紫色，创造屏幕为蓝色）
     */
    static void render(renderer::trident::gui::GuiRenderer& gui,
        renderer::trident::item::ItemRenderer& itemRenderer,
        const ItemStack& stack,
        i32 mouseX,
        i32 mouseY,
        i32 screenWidth,
        i32 screenHeight,
        u32 borderColor = BORDER_COLOR);

    // ========== 布局计算（公开用于测试）==========

    /**
     * @brief 计算内容物网格的行数
     *
     * 对应 MC 1.21.11 ClientBundleTooltip#gridSizeY：
     * positiveCeilDiv(slotCount, 4)，其中 slotCount = min(12, contents.size())。
     *
     * @param contents 内容物
     * @return 网格行数（1~3）
     */
    [[nodiscard]] static i32 gridSizeY(const BundleContents& contents) noexcept;

    /**
     * @brief 计算内容物网格的高度（像素）
     *
     * 对应 MC 1.21.11 ClientBundleTooltip#itemGridHeight：gridSizeY * 24。
     *
     * @param contents 内容物
     * @return 网格高度（24、48 或 72）
     */
    [[nodiscard]] static i32 itemGridHeight(const BundleContents& contents) noexcept;

    /**
     * @brief 计算要显示的槽位数
     *
     * 对应 MC 1.21.11 ClientBundleTooltip#slotCount：min(12, contents.size())。
     *
     * @param contents 内容物
     * @return 槽位数（0~12）
     */
    [[nodiscard]] static i32 slotCount(const BundleContents& contents) noexcept;

    /**
     * @brief 计算进度条填充宽度
     *
     * 对应 MC 1.21.11 ClientBundleTooltip#getProgressBarFill：
     * clamp(mulAndTruncate(weight, 94), 0, 94)。
     * 本项目使用整数权重：weight * 94 / MAX_WEIGHT。
     *
     * @param contents 内容物
     * @return 填充宽度（0~94）
     */
    [[nodiscard]] static i32 progressBarFill(const BundleContents& contents) noexcept;

    /**
     * @brief 计算溢出物品数量
     *
     * 对应 MC 1.21.11 ClientBundleTooltip#getAmountOfHiddenItems：
     * 内容物中第 slotCount 之后的物品数量之和。
     *
     * @param contents 内容物
     * @return 溢出物品数量
     */
    [[nodiscard]] static i32 amountOfHiddenItems(const BundleContents& contents) noexcept;

    /**
     * @brief 计算 tooltip 总高度
     *
     * 对应 MC 1.21.11 ClientBundleTooltip#getHeight：
     * - 空：getEmptyBundleBackgroundHeight = emptyDescriptionHeight + 13 + 8
     * - 非空：backgroundHeight = itemGridHeight + 13 + 8
     *
     * 本项目使用 TOP_PADDING + gridHeight + PROGRESSBAR_MARGIN_Y + PROGRESSBAR_HEIGHT + BOTTOM_PADDING。
     *
     * @param contents 内容物
     * @param emptyDescriptionHeight 空描述文本高度（仅空状态使用，传 0 表示非空或忽略）
     * @return tooltip 总高度
     */
    [[nodiscard]] static i32 tooltipHeight(const BundleContents& contents, i32 emptyDescriptionHeight = 0) noexcept;

    /**
     * @brief 计算 tooltip 渲染位置
     *
     * 与 AbstractContainerScreen::renderItemTooltip 中的位置算法一致：
     * - 默认在鼠标右下偏移 12px
     * - 超出右边界时翻转到鼠标左方
     * - 超出下边界时翻转到鼠标上方
     * - 不低于 4px（避免贴边）
     *
     * @param mouseX 鼠标 X 坐标
     * @param mouseY 鼠标 Y 坐标
     * @param tooltipWidth tooltip 宽度
     * @param tooltipHeight tooltip 高度
     * @param screenWidth 屏幕宽度
     * @param screenHeight 屏幕高度
     * @return 左上角坐标 (x, y)
     */
    [[nodiscard]] static std::pair<i32, i32> positionTooltip(
        i32 mouseX, i32 mouseY, i32 tooltipWidth, i32 tooltipHeight, i32 screenWidth, i32 screenHeight) noexcept;
};

} // namespace mc::client::ui::screen::tooltip
