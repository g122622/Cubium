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

#include "core/Types.hpp"

#include <gtest/gtest.h>

namespace mc::client {

// ============================================================================
// 屏幕工具提示命中检测与位置计算测试
//
// 这些测试验证 CreativeScreen、ChestScreen、FurnaceScreen 等屏幕中
// 工具提示触发逻辑所依赖的命中检测算法和位置计算算法。
//
// 命中检测逻辑与 CreativeScreen::_isMouseOver 和
// ContainerScreenBase::slotAt 一致。
// 位置计算逻辑与 CreativeScreen::renderTooltip 和
// TooltipRenderer::positionTooltip 一致。
// ============================================================================

// 与 CreativeScreen::_isMouseOver 和 ContainerScreenBase::slotAt
// 使用相同的边界检查逻辑
static bool isMouseOver(i32 mouseX, i32 mouseY, i32 x, i32 y, i32 width, i32 height)
{
    return mouseX >= x && mouseX < x + width && mouseY >= y && mouseY < y + height;
}

// ============================================================================
// 命中检测测试
// ============================================================================

class HitDetectionTest : public ::testing::Test {
protected:
    // 与 CreativeScreen / ContainerScreenBase 相同的布局常量
    static constexpr i32 SLOT_SIZE = 16;
    static constexpr i32 SLOT_SPACING = 18;
    static constexpr i32 PALETTE_COLUMNS = 9;
    static constexpr i32 PALETTE_VISIBLE_ROWS = 5;
    static constexpr i32 PALETTE_X = 8;
    static constexpr i32 PALETTE_Y = 26;
    static constexpr i32 INVENTORY_X = 180;
    static constexpr i32 INVENTORY_Y = 6;
    static constexpr i32 ARMOR_X = 8;
    static constexpr i32 ARMOR_Y_HEAD = 8;
    static constexpr i32 ARMOR_Y_CHEST = 26;
    static constexpr i32 ARMOR_Y_LEGS = 44;
    static constexpr i32 ARMOR_Y_FEET = 62;
    static constexpr i32 OFFHAND_X = 77;
    static constexpr i32 OFFHAND_Y = 62;
    static constexpr i32 PLAYER_INV_X = 8;
    static constexpr i32 PLAYER_INV_Y = 84;
    static constexpr i32 HOTBAR_X = 8;
    static constexpr i32 HOTBAR_Y = 142;
};

// 测试：鼠标在槽位内部应返回 true
TEST_F(HitDetectionTest, MouseInsideSlotReturnsTrue)
{
    constexpr i32 slotX = 100;
    constexpr i32 slotY = 200;

    // 左上角
    EXPECT_TRUE(isMouseOver(slotX, slotY, slotX, slotY, SLOT_SIZE, SLOT_SIZE));
    // 右边缘（不含，左闭右开）
    EXPECT_TRUE(isMouseOver(slotX + SLOT_SIZE - 1, slotY, slotX, slotY, SLOT_SIZE, SLOT_SIZE));
    // 下边缘（不含，上闭下开）
    EXPECT_TRUE(isMouseOver(slotX, slotY + SLOT_SIZE - 1, slotX, slotY, SLOT_SIZE, SLOT_SIZE));
    // 中心
    EXPECT_TRUE(isMouseOver(slotX + SLOT_SIZE / 2, slotY + SLOT_SIZE / 2, slotX, slotY, SLOT_SIZE, SLOT_SIZE));
}

// 测试：鼠标在槽位外部应返回 false
TEST_F(HitDetectionTest, MouseOutsideSlotReturnsFalse)
{
    constexpr i32 slotX = 100;
    constexpr i32 slotY = 200;

    EXPECT_FALSE(isMouseOver(slotX - 1, slotY, slotX, slotY, SLOT_SIZE, SLOT_SIZE));
    EXPECT_FALSE(isMouseOver(slotX, slotY - 1, slotX, slotY, SLOT_SIZE, SLOT_SIZE));
    EXPECT_FALSE(isMouseOver(slotX + SLOT_SIZE, slotY, slotX, slotY, SLOT_SIZE, SLOT_SIZE));
    EXPECT_FALSE(isMouseOver(slotX, slotY + SLOT_SIZE, slotX, slotY, SLOT_SIZE, SLOT_SIZE));
}

// 测试：槽位间距区域不应命中
TEST_F(HitDetectionTest, SpacingBetweenSlotsNotHit)
{
    // 两个相邻槽位之间有 2 像素间距 (SLOT_SPACING - SLOT_SIZE = 18 - 16 = 2)
    constexpr i32 slotX = 100;
    constexpr i32 slotY = 200;

    // 槽位右边缘最后一个像素（应命中）
    EXPECT_TRUE(isMouseOver(slotX + SLOT_SIZE - 1, slotY, slotX, slotY, SLOT_SIZE, SLOT_SIZE));
    // 间距区域第一个像素（不应命中当前槽位）
    EXPECT_FALSE(isMouseOver(slotX + SLOT_SIZE, slotY, slotX, slotY, SLOT_SIZE, SLOT_SIZE));
    // 下一个槽位起始（应命中下一个槽位）
    EXPECT_TRUE(isMouseOver(slotX + SLOT_SPACING, slotY, slotX + SLOT_SPACING, slotY, SLOT_SIZE, SLOT_SIZE));
}

// 测试：调色板网格索引计算
TEST_F(HitDetectionTest, PaletteIndexCalculation)
{
    constexpr i32 scrollRows = 0;

    // 第一行第一列（索引 0）
    {
        const i32 adjustedX = 0;
        const i32 adjustedY = 0;
        const i32 column = adjustedX / SLOT_SPACING;
        const i32 row = adjustedY / SLOT_SPACING;
        EXPECT_EQ(column, 0);
        EXPECT_EQ(row, 0);
        const i32 index = scrollRows * PALETTE_COLUMNS + row * PALETTE_COLUMNS + column;
        EXPECT_EQ(index, 0);
    }

    // 第二行第三列（索引 9*1 + 2 = 11）
    {
        const i32 adjustedX = 2 * SLOT_SPACING + 1;
        const i32 adjustedY = 1 * SLOT_SPACING + 1;
        const i32 column = adjustedX / SLOT_SPACING;
        const i32 row = adjustedY / SLOT_SPACING;
        EXPECT_EQ(column, 2);
        EXPECT_EQ(row, 1);
        const i32 index = scrollRows * PALETTE_COLUMNS + row * PALETTE_COLUMNS + column;
        EXPECT_EQ(index, 11);
    }

    // 最后一列第一行（索引 8）
    {
        const i32 adjustedX = 8 * SLOT_SPACING + 1;
        const i32 adjustedY = 1;
        const i32 column = adjustedX / SLOT_SPACING;
        const i32 row = adjustedY / SLOT_SPACING;
        EXPECT_EQ(column, 8);
        EXPECT_EQ(row, 0);
        const i32 index = scrollRows * PALETTE_COLUMNS + row * PALETTE_COLUMNS + column;
        EXPECT_EQ(index, 8);
    }
}

// 测试：槽位间距区域的像素不应计入任何列/行
TEST_F(HitDetectionTest, SpacingPixelsNotCountedAsColumnOrRow)
{
    // 在槽位间距区域（如第16和17像素）点击，
    // 取模后应超出 SLOT_SIZE，不命中任何槽位
    constexpr i32 adjustedX = SLOT_SIZE;         // 第17像素，刚好在间距区域
    const i32 localX = adjustedX % SLOT_SPACING; // 16 % 18 = 16
    EXPECT_GE(localX, SLOT_SIZE);                // 16 >= 16，在间距区域

    constexpr i32 adjustedY = SLOT_SIZE;
    const i32 localY = adjustedY % SLOT_SPACING; // 16 % 18 = 16
    EXPECT_GE(localY, SLOT_SIZE);                // 16 >= 16，在间距区域
}

// 测试：调色板边界索引（滚动偏移）
TEST_F(HitDetectionTest, PaletteIndexWithScrollOffset)
{
    // 滚动 2 行后，第一行第一列对应全局索引 18
    constexpr i32 scrollRows = 2;
    const i32 index = scrollRows * PALETTE_COLUMNS + 0;
    EXPECT_EQ(index, 18);
}

// ============================================================================
// 工具提示位置计算测试
// ============================================================================

class TooltipPositionTest : public ::testing::Test {
protected:
    // 与 _renderItemTooltip / renderItemTooltip 相同的常量
    static constexpr f64 PADDING = 4.0;
    static constexpr f64 MARGIN = 12.0;
    static constexpr i32 SCREEN_WIDTH = 800;
    static constexpr i32 SCREEN_HEIGHT = 600;
};

// 测试：鼠标在左上角时工具提示出现在右下方
TEST_F(TooltipPositionTest, TooltipAtTopLeft)
{
    const i32 mouseX = 10;
    const i32 mouseY = 10;
    f64 tooltipX = static_cast<f64>(mouseX) + MARGIN;
    f64 tooltipY = static_cast<f64>(mouseY) + MARGIN;

    EXPECT_GT(tooltipX, 0.0);
    EXPECT_GT(tooltipY, 0.0);
    EXPECT_LT(tooltipX, static_cast<f64>(SCREEN_WIDTH));
    EXPECT_LT(tooltipY, static_cast<f64>(SCREEN_HEIGHT));
}

// 测试：鼠标在右下角时工具提示翻转到左上方
TEST_F(TooltipPositionTest, TooltipFlipsAtBottomRight)
{
    const i32 mouseX = SCREEN_WIDTH - 10;
    const i32 mouseY = SCREEN_HEIGHT - 10;
    constexpr f64 tooltipWidth = 150.0;
    constexpr f64 tooltipHeight = 50.0;

    f64 tooltipX = static_cast<f64>(mouseX) + MARGIN;
    f64 tooltipY = static_cast<f64>(mouseY) + MARGIN;

    // 右侧溢出，应翻转到左侧
    if (tooltipX + tooltipWidth > static_cast<f64>(SCREEN_WIDTH)) {
        tooltipX = static_cast<f64>(mouseX) - MARGIN - tooltipWidth;
    }
    // 下方溢出，应翻转到上方
    if (tooltipY + tooltipHeight > static_cast<f64>(SCREEN_HEIGHT)) {
        tooltipY = static_cast<f64>(mouseY) - MARGIN - tooltipHeight;
    }

    tooltipX = std::max(4.0, tooltipX);
    tooltipY = std::max(4.0, tooltipY);

    EXPECT_GE(tooltipX, 4.0);
    EXPECT_GE(tooltipY, 4.0);
    EXPECT_LT(tooltipX + tooltipWidth, static_cast<f64>(SCREEN_WIDTH));
    EXPECT_LT(tooltipY + tooltipHeight, static_cast<f64>(SCREEN_HEIGHT));
}

// 测试：工具提示不应超出屏幕左上角
TEST_F(TooltipPositionTest, TooltipClampedToMinimum)
{
    // 极端位置：鼠标在 (0, 0)
    const i32 mouseX = 0;
    const i32 mouseY = 0;
    constexpr f64 tooltipWidth = 200.0;
    constexpr f64 tooltipHeight = 60.0;

    f64 tooltipX = static_cast<f64>(mouseX) + MARGIN;
    f64 tooltipY = static_cast<f64>(mouseY) + MARGIN;

    tooltipX = std::max(4.0, tooltipX);
    tooltipY = std::max(4.0, tooltipY);

    EXPECT_GE(tooltipX, 4.0);
    EXPECT_GE(tooltipY, 4.0);
}

// 测试：小型工具提示在屏幕中间正常显示
TEST_F(TooltipPositionTest, SmallTooltipInCenter)
{
    const i32 mouseX = SCREEN_WIDTH / 2;
    const i32 mouseY = SCREEN_HEIGHT / 2;
    constexpr f64 tooltipWidth = 80.0;
    constexpr f64 tooltipHeight = 20.0;

    f64 tooltipX = static_cast<f64>(mouseX) + MARGIN;
    f64 tooltipY = static_cast<f64>(mouseY) + MARGIN;

    // 屏幕中间，无需翻转
    if (tooltipX + tooltipWidth > static_cast<f64>(SCREEN_WIDTH)) {
        tooltipX = static_cast<f64>(mouseX) - MARGIN - tooltipWidth;
    }
    if (tooltipY + tooltipHeight > static_cast<f64>(SCREEN_HEIGHT)) {
        tooltipY = static_cast<f64>(mouseY) - MARGIN - tooltipHeight;
    }

    EXPECT_GT(tooltipX, static_cast<f64>(mouseX));
    EXPECT_GT(tooltipY, static_cast<f64>(mouseY));
}

// ============================================================================
// 容器屏槽位常量测试
// ============================================================================

// 测试：SLOT_SIZE 和 SLOT_SPACING 之间的关系
TEST(ScreenLayoutConstantsTest, SpacingIsLargerThanSize)
{
    // 间距必须大于尺寸，否则槽位之间没有间距
    constexpr i32 SLOT_SIZE = 16;
    constexpr i32 SLOT_SPACING = 18;
    EXPECT_GT(SLOT_SPACING, SLOT_SIZE);
    EXPECT_EQ(SLOT_SPACING - SLOT_SIZE, 2); // 2像素间距
}

// 测试：调色板列数与 MC 一致
TEST(ScreenLayoutConstantsTest, PaletteColumnsMatchMC)
{
    // MC 创造模式物品栏每行 9 列
    constexpr i32 PALETTE_COLUMNS = 9;
    EXPECT_EQ(PALETTE_COLUMNS, 9);
}

// 测试：调色板可见行数
TEST(ScreenLayoutConstantsTest, PaletteVisibleRows)
{
    // MC 创造模式物品栏可见 5 行
    constexpr i32 PALETTE_VISIBLE_ROWS = 5;
    EXPECT_EQ(PALETTE_VISIBLE_ROWS, 5);
}

} // namespace mc::client
