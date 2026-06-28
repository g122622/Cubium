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

/**
 * @file TooltipTest.cpp
 * @brief Tooltip、TooltipRenderer 和 Widget::refreshTooltip 单元测试
 */

#include "client/ui/kagero/widget/Tooltip.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/widget/ButtonWidget.hpp"
#include "client/ui/kagero/widget/TooltipRenderer.hpp"
#include "client/ui/kagero/widget/Widget.hpp"
#include <chrono>
#include <thread>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::widget;

// ==================== Tooltip 数据类测试 ====================

TEST(TooltipTest, DefaultConstructorIsEmpty)
{
    Tooltip tooltip;
    EXPECT_TRUE(tooltip.isEmpty());
    EXPECT_EQ(0u, tooltip.lineCount());
    EXPECT_TRUE(tooltip.lines().empty());
}

TEST(TooltipTest, SingleLineConstructor)
{
    Tooltip tooltip("Hello");
    EXPECT_FALSE(tooltip.isEmpty());
    EXPECT_EQ(1u, tooltip.lineCount());
    ASSERT_EQ(1u, tooltip.lines().size());
    EXPECT_EQ("Hello", tooltip.lines()[0]);
}

TEST(TooltipTest, MultiLineConstructor)
{
    std::vector<std::string> lines = {"Line 1", "Line 2", "Line 3"};
    Tooltip tooltip(lines);
    EXPECT_FALSE(tooltip.isEmpty());
    EXPECT_EQ(3u, tooltip.lineCount());
    ASSERT_EQ(3u, tooltip.lines().size());
    EXPECT_EQ("Line 1", tooltip.lines()[0]);
    EXPECT_EQ("Line 2", tooltip.lines()[1]);
    EXPECT_EQ("Line 3", tooltip.lines()[2]);
}

TEST(TooltipTest, CreateFactoryMethodSingleLine)
{
    auto tooltip = Tooltip::create("Test");
    EXPECT_FALSE(tooltip.isEmpty());
    EXPECT_EQ(1u, tooltip.lineCount());
    EXPECT_EQ("Test", tooltip.lines()[0]);
}

TEST(TooltipTest, CreateFactoryMethodMultiLine)
{
    auto tooltip = Tooltip::create(std::vector<std::string>{"A", "B"});
    EXPECT_FALSE(tooltip.isEmpty());
    EXPECT_EQ(2u, tooltip.lineCount());
}

TEST(TooltipTest, AddLine)
{
    Tooltip tooltip;
    EXPECT_TRUE(tooltip.isEmpty());

    tooltip.addLine("First");
    EXPECT_FALSE(tooltip.isEmpty());
    EXPECT_EQ(1u, tooltip.lineCount());

    tooltip.addLine("Second");
    EXPECT_EQ(2u, tooltip.lineCount());
    EXPECT_EQ("First", tooltip.lines()[0]);
    EXPECT_EQ("Second", tooltip.lines()[1]);
}

TEST(TooltipTest, MaxWidthDefault)
{
    Tooltip tooltip("Test");
    EXPECT_EQ(Tooltip::DEFAULT_MAX_WIDTH, tooltip.maxWidth());
    EXPECT_EQ(170, tooltip.maxWidth());
}

TEST(TooltipTest, SetMaxWidth)
{
    Tooltip tooltip("Test");
    tooltip.setMaxWidth(200);
    EXPECT_EQ(200, tooltip.maxWidth());
}

// ==================== TooltipRenderer 测试 ====================

namespace {

/**
 * @brief 用于 TooltipRenderer 测试的记录画布
 */
class TooltipTestCanvas final : public paint::ICanvas {
public:
    void reset()
    {
        filledRects.clear();
        texts.clear();
    }

    void drawRect(const Rect& rect, const paint::IPaint& paint) override
    {
        filledRects.push_back({rect, paint.color().toARGB()});
    }

    void drawRRect(const paint::RRect&, const paint::IPaint&) override {}
    void drawCircle(f32, f32, f32, const paint::IPaint&) override {}
    void drawOval(const Rect&, const paint::IPaint&) override {}
    void drawPath(const paint::IPath&, const paint::IPaint&) override {}
    void drawLine(f32, f32, f32, f32, const paint::IPaint&) override {}
    void drawGradientRect(const Rect&, u32, u32, bool) override {}
    void drawImage(const paint::IImage&, f32, f32) override {}
    void drawImageRect(const paint::IImage&, const Rect&, const Rect&) override {}
    void drawImageNine(const paint::IImage&, const Rect&, const Rect&, const paint::IPaint*) override {}

    void drawText(const std::string& text, f32 x, f32 y, const paint::IPaint& paint) override
    {
        texts.push_back({text, x, y, paint.color().toARGB()});
    }

    void drawTextBlob(const paint::ITextBlob&, f32, f32, const paint::IPaint&) override {}
    void clipRect(const Rect&) override {}
    void clipRRect(const paint::RRect&) override {}
    void clipPath(const paint::IPath&) override {}
    void clipOutRect(const Rect&) override {}
    [[nodiscard]] bool clipIsEmpty() const override { return false; }
    [[nodiscard]] Rect getClipBounds() const override { return Rect{}; }
    void translate(f32, f32) override {}
    void scale(f32, f32) override {}
    void rotate(f32) override {}
    void concat(const paint::Matrix&) override {}
    void setMatrix(const paint::Matrix&) override {}
    [[nodiscard]] paint::Matrix getTotalMatrix() const override { return paint::Matrix::identity(); }
    i32 save() override { return 0; }
    void restore() override {}
    void restoreToCount(i32) override {}
    i32 saveLayer(const Rect*, const paint::IPaint*) override { return 0; }
    i32 saveLayerAlpha(const Rect*, u8) override { return 0; }
    [[nodiscard]] i32 width() const override { return m_width; }
    [[nodiscard]] i32 height() const override { return m_height; }
    [[nodiscard]] f32 getTextWidth(const std::string& text) const override
    {
        return static_cast<f32>(text.size()) * m_charWidth;
    }
    [[nodiscard]] u32 getFontHeight() const override { return m_fontHeight; }

    void setCanvasSize(i32 w, i32 h)
    {
        m_width = w;
        m_height = h;
    }

    void setCharWidth(f32 w) { m_charWidth = w; }

    void setFontHeight(u32 h) { m_fontHeight = h; }

    // 记录结构
    struct FilledRectRecord {
        Rect rect;
        u32 color;
    };
    struct TextRecord {
        std::string text;
        f32 x;
        f32 y;
        u32 color;
    };

    std::vector<FilledRectRecord> filledRects;
    std::vector<TextRecord> texts;

    i32 m_width = 800;
    i32 m_height = 600;
    f32 m_charWidth = 6.0f;
    u32 m_fontHeight = 12;
};

} // namespace

TEST(TooltipRendererTest, MeasureEmptyTooltip)
{
    TooltipTestCanvas canvas;
    PaintContext ctx(canvas);
    Tooltip tooltip;

    auto [width, height] = TooltipRenderer::measure(ctx, tooltip);
    EXPECT_FLOAT_EQ(0.0f, width);
    EXPECT_FLOAT_EQ(0.0f, height);
}

TEST(TooltipRendererTest, MeasureSingleLine)
{
    TooltipTestCanvas canvas;
    canvas.setCharWidth(6.0f);
    canvas.setFontHeight(12);
    PaintContext ctx(canvas);

    Tooltip tooltip = Tooltip::create("Hello"); // 5 chars * 6 = 30px
    auto [width, height] = TooltipRenderer::measure(ctx, tooltip);

    // width = maxTextWidth + 2 * PADDING = 30 + 8 = 38
    EXPECT_FLOAT_EQ(38.0f, width);
    // height = 1 * fontHeight + 2 * PADDING = 12 + 8 = 20
    EXPECT_FLOAT_EQ(20.0f, height);
}

TEST(TooltipRendererTest, MeasureMultiLine)
{
    TooltipTestCanvas canvas;
    canvas.setCharWidth(6.0f);
    canvas.setFontHeight(12);
    PaintContext ctx(canvas);

    Tooltip tooltip =
        Tooltip::create(std::vector<std::string>{"Short", "Longer Text"}); // "Longer Text" = 11 chars * 6 = 66px
    auto [width, height] = TooltipRenderer::measure(ctx, tooltip);

    // width = 66 + 8 = 74
    EXPECT_FLOAT_EQ(74.0f, width);
    // height = 2 * 12 + 8 = 32
    EXPECT_FLOAT_EQ(32.0f, height);
}

TEST(TooltipRendererTest, PositionTooltipDefaultPosition)
{
    // 鼠标在 (100, 100)，默认在右下方偏移 12px
    auto [x, y] = TooltipRenderer::positionTooltip(100.0f, 100.0f, 50.0f, 30.0f, 800.0f, 600.0f);
    EXPECT_FLOAT_EQ(112.0f, x); // mouseX + MOUSE_OFFSET
    EXPECT_FLOAT_EQ(112.0f, y); // mouseY + MOUSE_OFFSET
}

TEST(TooltipRendererTest, PositionTooltipFlipRight)
{
    // 鼠标靠近右边缘，Tooltip 应翻转到左侧
    auto [x, y] = TooltipRenderer::positionTooltip(790.0f, 100.0f, 50.0f, 30.0f, 800.0f, 600.0f);
    // 790 + 12 + 50 = 852 > 800，翻转到左侧
    // x = 790 - 12 - 50 = 728
    EXPECT_FLOAT_EQ(728.0f, x);
    EXPECT_FLOAT_EQ(112.0f, y);
}

TEST(TooltipRendererTest, PositionTooltipFlipBottom)
{
    // 鼠标靠近底部，Tooltip 应翻转到上方
    auto [x, y] = TooltipRenderer::positionTooltip(100.0f, 590.0f, 50.0f, 30.0f, 800.0f, 600.0f);
    // 590 + 12 + 30 = 632 > 600，翻转到上方
    // y = 590 - 12 - 30 = 548
    EXPECT_FLOAT_EQ(112.0f, x);
    EXPECT_FLOAT_EQ(548.0f, y);
}

TEST(TooltipRendererTest, PositionTooltipFlipBoth)
{
    // 鼠标靠近右下角，两个方向都应翻转
    auto [x, y] = TooltipRenderer::positionTooltip(790.0f, 590.0f, 50.0f, 30.0f, 800.0f, 600.0f);
    EXPECT_FLOAT_EQ(728.0f, x); // 790 - 12 - 50
    EXPECT_FLOAT_EQ(548.0f, y); // 590 - 12 - 30
}

TEST(TooltipRendererTest, PositionTooltipMinPosition)
{
    // 极端情况：鼠标在 (0, 0)，翻转后可能产生负值，应被 MIN_POSITION 截断
    auto [x, y] = TooltipRenderer::positionTooltip(0.0f, 0.0f, 200.0f, 200.0f, 800.0f, 600.0f);
    EXPECT_FLOAT_EQ(12.0f, x); // 0 + 12 (默认右下)
    EXPECT_FLOAT_EQ(12.0f, y); // 0 + 12 (默认右下)

    // 鼠标在 (0, 0)，宽度巨大，右侧翻转后 x 为负值
    auto [x2, y2] = TooltipRenderer::positionTooltip(0.0f, 0.0f, 500.0f, 30.0f, 800.0f, 600.0f);
    // 0 + 12 + 500 = 512 < 800，不翻转
    EXPECT_FLOAT_EQ(12.0f, x2);

    // 鼠标在 (0, 0)，高度巨大
    auto [x3, y3] = TooltipRenderer::positionTooltip(0.0f, 0.0f, 50.0f, 700.0f, 800.0f, 600.0f);
    // 0 + 12 + 700 > 600，翻转到上方
    // y = 0 - 12 - 700 = -712，被 MIN_POSITION 截断为 4.0f
    EXPECT_FLOAT_EQ(4.0f, y3);
}

TEST(TooltipRendererTest, RenderEmptyTooltip)
{
    TooltipTestCanvas canvas;
    PaintContext ctx(canvas);
    Tooltip tooltip;

    TooltipRenderer::render(ctx, tooltip, 100.0f, 100.0f, 800.0f, 600.0f);

    EXPECT_TRUE(canvas.filledRects.empty());
    EXPECT_TRUE(canvas.texts.empty());
}

TEST(TooltipRendererTest, RenderSingleLineTooltip)
{
    TooltipTestCanvas canvas;
    canvas.setCharWidth(6.0f);
    canvas.setFontHeight(12);
    PaintContext ctx(canvas);

    Tooltip tooltip = Tooltip::create("Hello");
    TooltipRenderer::render(ctx, tooltip, 100.0f, 100.0f, 800.0f, 600.0f);

    // 应绘制背景矩形和边框矩形（drawBorder 通过 drawRect 实现）
    EXPECT_GE(canvas.filledRects.size(), 1u);

    // 验证背景颜色是 TooltipRenderer::BACKGROUND_COLOR
    bool foundBackground = false;
    for (const auto& rect : canvas.filledRects) {
        if (rect.color == TooltipRenderer::BACKGROUND_COLOR) {
            foundBackground = true;
            break;
        }
    }
    EXPECT_TRUE(foundBackground);

    // 应绘制文本
    ASSERT_EQ(1u, canvas.texts.size());
    EXPECT_EQ("Hello", canvas.texts[0].text);
    EXPECT_EQ(TooltipRenderer::TEXT_COLOR, canvas.texts[0].color);

    // 验证文本位置：x = mouseX + MOUSE_OFFSET + PADDING = 100 + 12 + 4 = 116
    // y = mouseY + MOUSE_OFFSET + PADDING = 100 + 12 + 4 = 116
    EXPECT_FLOAT_EQ(116.0f, canvas.texts[0].x);
    EXPECT_FLOAT_EQ(116.0f, canvas.texts[0].y);
}

TEST(TooltipRendererTest, RenderMultiLineTooltip)
{
    TooltipTestCanvas canvas;
    canvas.setCharWidth(6.0f);
    canvas.setFontHeight(12);
    PaintContext ctx(canvas);

    Tooltip tooltip = Tooltip::create(std::vector<std::string>{"Line 1", "Line 2", "Line 3"});
    TooltipRenderer::render(ctx, tooltip, 100.0f, 100.0f, 800.0f, 600.0f);

    // 应绘制背景矩形和边框矩形
    EXPECT_GE(canvas.filledRects.size(), 1u);

    // 应绘制3行文本
    ASSERT_EQ(3u, canvas.texts.size());
    EXPECT_EQ("Line 1", canvas.texts[0].text);
    EXPECT_EQ("Line 2", canvas.texts[1].text);
    EXPECT_EQ("Line 3", canvas.texts[2].text);

    // 第一行 y = 100 + 12 + 4 = 116
    // 第二行 y = 116 + 12 = 128
    // 第三行 y = 128 + 12 = 140
    EXPECT_FLOAT_EQ(116.0f, canvas.texts[0].y);
    EXPECT_FLOAT_EQ(128.0f, canvas.texts[1].y);
    EXPECT_FLOAT_EQ(140.0f, canvas.texts[2].y);
}

TEST(TooltipRendererTest, RenderTooltipColorConstants)
{
    // 验证颜色常量与 MC 风格一致
    EXPECT_EQ(0xF0100010u, TooltipRenderer::BACKGROUND_COLOR);
    EXPECT_EQ(0x505000FFu, TooltipRenderer::BORDER_COLOR);
    EXPECT_EQ(0xFFFFFFFFu, TooltipRenderer::TEXT_COLOR);
    EXPECT_FLOAT_EQ(4.0f, TooltipRenderer::PADDING);
    EXPECT_FLOAT_EQ(12.0f, TooltipRenderer::MOUSE_OFFSET);
    EXPECT_FLOAT_EQ(4.0f, TooltipRenderer::MIN_POSITION);
}

// ==================== Widget::refreshTooltip 测试 ====================

namespace {

/**
 * @brief 用于测试 refreshTooltip 的 Widget 子类
 */
class TooltipTestWidget : public Widget {
public:
    TooltipTestWidget() = default;
    explicit TooltipTestWidget(std::string id)
        : Widget(std::move(id))
    {}

    void paint(PaintContext& ctx) override
    {
        // 在 paint 中调用 refreshTooltip，模拟实际使用
        refreshTooltip(ctx, static_cast<f32>(ctx.canvas().width()), static_cast<f32>(ctx.canvas().height()));
    }

    // 暴露 updateHover 用于测试
    void simulateHover(i32 mouseX, i32 mouseY) { updateHover(mouseX, mouseY); }
};

} // namespace

TEST(WidgetTooltipTest, SetTooltipString)
{
    TooltipTestWidget widget;
    EXPECT_FALSE(widget.hasTooltip());

    widget.setTooltip("Hello");
    EXPECT_TRUE(widget.hasTooltip());
    EXPECT_EQ(1u, widget.tooltip().lineCount());
    EXPECT_EQ("Hello", widget.tooltip().lines()[0]);
}

TEST(WidgetTooltipTest, SetTooltipObject)
{
    TooltipTestWidget widget;
    auto tooltip = Tooltip::create(std::vector<std::string>{"Line 1", "Line 2"});
    widget.setTooltip(tooltip);
    EXPECT_TRUE(widget.hasTooltip());
    EXPECT_EQ(2u, widget.tooltip().lineCount());
}

TEST(WidgetTooltipTest, ClearTooltip)
{
    TooltipTestWidget widget;
    widget.setTooltip("Hello");
    EXPECT_TRUE(widget.hasTooltip());

    widget.clearTooltip();
    EXPECT_FALSE(widget.hasTooltip());
    EXPECT_TRUE(widget.tooltip().isEmpty());
}

TEST(WidgetTooltipTest, SetTooltipDelay)
{
    TooltipTestWidget widget;
    EXPECT_EQ(0, widget.tooltipDelay());

    widget.setTooltipDelay(500);
    EXPECT_EQ(500, widget.tooltipDelay());
}

TEST(WidgetTooltipTest, LastMousePosition)
{
    TooltipTestWidget widget;
    widget.setBounds(Rect(0, 0, 100, 100));

    EXPECT_EQ(0, widget.lastMouseX());
    EXPECT_EQ(0, widget.lastMouseY());

    widget.simulateHover(50, 60);
    EXPECT_EQ(50, widget.lastMouseX());
    EXPECT_EQ(60, widget.lastMouseY());
}

TEST(WidgetTooltipTest, RefreshTooltipEmptyDoesNotDraw)
{
    TooltipTestCanvas canvas;
    PaintContext ctx(canvas);
    TooltipTestWidget widget;

    widget.setBounds(Rect(0, 0, 100, 100));
    widget.simulateHover(50, 50);
    widget.paint(ctx);

    // 空 Tooltip 不应绘制任何内容
    EXPECT_TRUE(canvas.filledRects.empty());
    EXPECT_TRUE(canvas.texts.empty());
}

TEST(WidgetTooltipTest, RefreshTooltipNotHoveredDoesNotDraw)
{
    TooltipTestCanvas canvas;
    PaintContext ctx(canvas);
    TooltipTestWidget widget;

    widget.setBounds(Rect(0, 0, 100, 100));
    widget.setTooltip("Hello");
    // 鼠标不在组件上，不悬停
    widget.simulateHover(200, 200);
    EXPECT_FALSE(widget.isHovered());
    widget.paint(ctx);

    // 不悬停时不应绘制 Tooltip
    EXPECT_TRUE(canvas.filledRects.empty());
    EXPECT_TRUE(canvas.texts.empty());
}

TEST(WidgetTooltipTest, RefreshTooltipHoveredDrawsTooltip)
{
    TooltipTestCanvas canvas;
    canvas.setCharWidth(6.0f);
    canvas.setFontHeight(12);
    PaintContext ctx(canvas);
    TooltipTestWidget widget;

    widget.setBounds(Rect(0, 0, 100, 100));
    widget.setTooltip("Hello");
    // 鼠标在组件内，悬停
    widget.simulateHover(50, 50);
    EXPECT_TRUE(widget.isHovered());
    widget.paint(ctx);

    // 悬停时应绘制 Tooltip
    EXPECT_FALSE(canvas.filledRects.empty());
    EXPECT_FALSE(canvas.texts.empty());
    EXPECT_EQ("Hello", canvas.texts[0].text);
}

TEST(WidgetTooltipTest, RefreshTooltipWithDelayNotShownImmediately)
{
    TooltipTestCanvas canvas;
    canvas.setCharWidth(6.0f);
    canvas.setFontHeight(12);
    PaintContext ctx(canvas);
    TooltipTestWidget widget;

    widget.setBounds(Rect(0, 0, 100, 100));
    widget.setTooltip("Delayed");
    widget.setTooltipDelay(2000); // 2秒延迟

    // 开始悬停
    widget.simulateHover(50, 50);
    EXPECT_TRUE(widget.isHovered());

    // 立即调用 paint，延迟尚未到期，不应绘制
    widget.paint(ctx);
    EXPECT_TRUE(canvas.filledRects.empty()) << "Tooltip should not be drawn before delay expires";
    EXPECT_TRUE(canvas.texts.empty()) << "Tooltip should not be drawn before delay expires";
}

TEST(WidgetTooltipTest, RefreshTooltipWithDelayShownAfterWait)
{
    TooltipTestCanvas canvas;
    canvas.setCharWidth(6.0f);
    canvas.setFontHeight(12);
    PaintContext ctx(canvas);
    TooltipTestWidget widget;

    widget.setBounds(Rect(0, 0, 100, 100));
    widget.setTooltip("Delayed");
    widget.setTooltipDelay(1); // 1ms 延迟（极短延迟确保测试稳定）

    // 开始悬停
    widget.simulateHover(50, 50);
    EXPECT_TRUE(widget.isHovered());

    // 第一次 paint 记录开始时间，但延迟未到期
    widget.paint(ctx);
    EXPECT_TRUE(canvas.filledRects.empty()) << "Tooltip should not be drawn on first paint with delay";

    // 等待延迟过期
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 延迟已过期，应绘制 Tooltip
    canvas.reset();
    widget.paint(ctx);
    EXPECT_FALSE(canvas.filledRects.empty()) << "Tooltip should be drawn after delay expires";
    EXPECT_FALSE(canvas.texts.empty());
    EXPECT_EQ("Delayed", canvas.texts[0].text);
}

TEST(WidgetTooltipTest, RefreshTooltipResetsOnMouseLeave)
{
    TooltipTestCanvas canvas;
    canvas.setCharWidth(6.0f);
    canvas.setFontHeight(12);
    PaintContext ctx(canvas);
    TooltipTestWidget widget;

    widget.setBounds(Rect(0, 0, 100, 100));
    widget.setTooltip("Test");
    // 使用零延迟确保立即显示
    widget.setTooltipDelay(0);

    // 开始悬停
    widget.simulateHover(50, 50);
    EXPECT_TRUE(widget.isHovered());

    // 延迟为0，应立即绘制 Tooltip
    widget.paint(ctx);
    EXPECT_FALSE(canvas.filledRects.empty());

    // 鼠标离开
    widget.simulateHover(200, 200);
    EXPECT_FALSE(widget.isHovered());

    canvas.reset();
    widget.paint(ctx);
    EXPECT_TRUE(canvas.filledRects.empty()) << "Tooltip should not be drawn after mouse leaves";
    EXPECT_TRUE(canvas.texts.empty());
}

TEST(WidgetTooltipTest, RefreshTooltipZeroDelayImmediateDisplay)
{
    TooltipTestCanvas canvas;
    canvas.setCharWidth(6.0f);
    canvas.setFontHeight(12);
    PaintContext ctx(canvas);
    TooltipTestWidget widget;

    widget.setBounds(Rect(0, 0, 100, 100));
    widget.setTooltip("Immediate");
    // 默认延迟为 0
    EXPECT_EQ(0, widget.tooltipDelay());

    widget.simulateHover(50, 50);
    widget.paint(ctx);

    // 延迟为 0，应立即绘制
    EXPECT_FALSE(canvas.filledRects.empty());
    EXPECT_FALSE(canvas.texts.empty());
    EXPECT_EQ("Immediate", canvas.texts[0].text);
}

TEST(WidgetTooltipTest, ButtonWidgetTooltipIntegration)
{
    // 验证 ButtonWidget::paint() 末尾调用了 refreshTooltip
    TooltipTestCanvas canvas;
    canvas.setCharWidth(6.0f);
    canvas.setFontHeight(12);
    PaintContext ctx(canvas);

    ButtonWidget button("btn_test", 0, 0, 100, 40, "Test");
    button.setTooltip("Button Tip");

    // 鼠标悬停在按钮上
    button.updateHover(50, 20);
    EXPECT_TRUE(button.isHovered());

    button.paint(ctx);

    // 按钮应绘制自身内容 + Tooltip
    // 至少有一个填充矩形（按钮背景）和一个文本（按钮文字）
    EXPECT_FALSE(canvas.filledRects.empty());

    // 查找 Tooltip 文本
    bool foundTooltipText = false;
    for (const auto& textRecord : canvas.texts) {
        if (textRecord.text == "Button Tip") {
            foundTooltipText = true;
            break;
        }
    }
    EXPECT_TRUE(foundTooltipText) << "ButtonWidget should render tooltip text";
}
