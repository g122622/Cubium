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
 * @file SliderWidgetTest.cpp
 * @brief SliderWidget单元测试
 */

#include "client/ui/kagero/widget/SliderWidget.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "common/input/KeyBinding.hpp"
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::widget;
using namespace mc::client::Colors;
using namespace mc;

// ==================== SliderWidget测试 ====================

TEST(SliderWidgetTest, DefaultConstructor)
{
    SliderWidget slider;
    EXPECT_TRUE(slider.id().empty());
    EXPECT_DOUBLE_EQ(0.0, slider.value());
}

TEST(SliderWidgetTest, ConstructorWithParams)
{
    SliderWidget slider("sldr_volume", 10, 20, 200, 30, 0.0, 100.0, 50.0);

    EXPECT_EQ("sldr_volume", slider.id());
    EXPECT_EQ(10, slider.x());
    EXPECT_EQ(20, slider.y());
    EXPECT_EQ(200, slider.width());
    EXPECT_EQ(30, slider.height());
    EXPECT_DOUBLE_EQ(0.0, slider.minValue());
    EXPECT_DOUBLE_EQ(100.0, slider.maxValue());
    EXPECT_DOUBLE_EQ(50.0, slider.value());
}

// ==================== 值操作测试 ====================

TEST(SliderWidgetTest, SetValue)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);

    slider.setValue(75.0);
    EXPECT_DOUBLE_EQ(75.0, slider.value());

    // 超出范围限制
    slider.setValue(150.0);
    EXPECT_DOUBLE_EQ(100.0, slider.value());

    slider.setValue(-50.0);
    EXPECT_DOUBLE_EQ(0.0, slider.value());
}

TEST(SliderWidgetTest, SetRange)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);

    slider.setRange(-50.0, 50.0);
    EXPECT_DOUBLE_EQ(-50.0, slider.minValue());
    EXPECT_DOUBLE_EQ(50.0, slider.maxValue());
    // 值应该被约束到新范围
    EXPECT_DOUBLE_EQ(50.0, slider.value());
}

TEST(SliderWidgetTest, SetMinValue)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);

    slider.setMinValue(25.0);
    EXPECT_DOUBLE_EQ(25.0, slider.minValue());
}

TEST(SliderWidgetTest, SetMaxValue)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);

    slider.setMaxValue(75.0);
    EXPECT_DOUBLE_EQ(75.0, slider.maxValue());
}

TEST(SliderWidgetTest, SetStepSize)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);

    slider.setStepSize(5.0);
    EXPECT_DOUBLE_EQ(5.0, slider.stepSize());
}

TEST(SliderWidgetTest, GetRatio)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);

    EXPECT_DOUBLE_EQ(0.5, slider.getRatio());

    slider.setValue(0.0);
    EXPECT_DOUBLE_EQ(0.0, slider.getRatio());

    slider.setValue(100.0);
    EXPECT_DOUBLE_EQ(1.0, slider.getRatio());
}

TEST(SliderWidgetTest, SetFromRatio)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 0.0);

    slider.setFromRatio(0.5);
    EXPECT_DOUBLE_EQ(50.0, slider.value());

    slider.setFromRatio(0.0);
    EXPECT_DOUBLE_EQ(0.0, slider.value());

    slider.setFromRatio(1.0);
    EXPECT_DOUBLE_EQ(100.0, slider.value());

    // 超出范围约束
    slider.setFromRatio(1.5);
    EXPECT_DOUBLE_EQ(100.0, slider.value());

    slider.setFromRatio(-0.5);
    EXPECT_DOUBLE_EQ(0.0, slider.value());
}

// ==================== 回调测试 ====================

TEST(SliderWidgetTest, OnValueChangedCallback)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);

    f64 lastValue = 0.0;
    int callCount = 0;
    slider.setOnValueChanged([&lastValue, &callCount](f64 value) {
        lastValue = value;
        ++callCount;
    });

    slider.setValue(75.0);
    EXPECT_DOUBLE_EQ(75.0, lastValue);
    EXPECT_EQ(1, callCount);

    // 相同值不触发回调
    slider.setValue(75.0);
    EXPECT_EQ(1, callCount);

    slider.setValue(25.0);
    EXPECT_DOUBLE_EQ(25.0, lastValue);
    EXPECT_EQ(2, callCount);
}

// ==================== 步进测试 ====================

TEST(SliderWidgetTest, StepSizeSnapping)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 0.0);
    slider.setStepSize(5.0);

    slider.setValue(12.0);
    EXPECT_DOUBLE_EQ(10.0, slider.value()); // 四舍五入到10

    slider.setValue(13.0);
    EXPECT_DOUBLE_EQ(15.0, slider.value()); // 四舍五入到15

    slider.setValue(17.5);
    EXPECT_DOUBLE_EQ(20.0, slider.value()); // 四舍五入到20
}

// ==================== 显示文本测试 ====================

TEST(SliderWidgetTest, SetDisplayText)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);

    slider.setDisplayText("Volume: {}");
    EXPECT_EQ("Volume: {}", slider.displayText());
}

TEST(SliderWidgetTest, FormatCallback)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);

    slider.setFormatCallback(
        [](f64 value) -> std::string { return "Value: " + std::to_string(static_cast<i32>(value)) + "%"; });

    EXPECT_EQ("Value: 50%", slider.displayText());

    slider.setValue(75.0);
    EXPECT_EQ("Value: 75%", slider.displayText());
}

// ==================== 拖动测试 ====================

TEST(SliderWidgetTest, ClickSetsFocus)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);
    slider.setActive(true);
    slider.setVisible(true);

    EXPECT_FALSE(slider.isFocused());

    slider.onClick(50, 10, 0, 0);
    EXPECT_TRUE(slider.isFocused());
    EXPECT_TRUE(slider.isDragging());
}

TEST(SliderWidgetTest, DraggingState)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);
    slider.setActive(true);
    slider.setVisible(true);

    EXPECT_FALSE(slider.isDragging());

    slider.onClick(50, 10, 0, 0);
    EXPECT_TRUE(slider.isDragging());

    slider.onRelease(50, 10, 0, 0);
    EXPECT_FALSE(slider.isDragging());
}

// ==================== IntSliderWidget测试 ====================

TEST(IntSliderWidgetTest, Constructor)
{
    IntSliderWidget slider("test", 0, 0, 100, 20, 0, 100, 50);

    EXPECT_EQ(50, slider.intValue());
}

TEST(IntSliderWidgetTest, SetIntValue)
{
    IntSliderWidget slider("test", 0, 0, 100, 20, 0, 100, 50);

    slider.setIntValue(75);
    EXPECT_EQ(75, slider.intValue());

    slider.setValue(25.7);
    EXPECT_EQ(26, slider.intValue()); // 四舍五入
}

TEST(IntSliderWidgetTest, FormatDisplay)
{
    IntSliderWidget slider("test", 0, 0, 100, 20, 0, 100, 50);

    EXPECT_EQ("50", slider.displayText());

    slider.setIntValue(75);
    EXPECT_EQ("75", slider.displayText());
}

// ==================== 滚轮测试 ====================

TEST(SliderWidgetTest, ScrollChangesValue)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);
    slider.setActive(true);
    slider.setVisible(true);

    f64 initialValue = slider.value();

    // 向下滚动减少值
    slider.onScroll(50, 10, -1.0);
    EXPECT_LT(slider.value(), initialValue);

    // 向上滚动增加值
    f64 afterScrollDown = slider.value();
    slider.onScroll(50, 10, 1.0);
    EXPECT_GT(slider.value(), afterScrollDown);
}

// ==================== 值显示测试 ====================

TEST(SliderWidgetTest, ShowValue_DefaultTrue)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);

    // 默认 m_showValue 为 true
    EXPECT_TRUE(slider.showValue());
}

TEST(SliderWidgetTest, ShowValue_SetFalse)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);

    slider.setShowValue(false);
    EXPECT_FALSE(slider.showValue());
}

TEST(SliderWidgetTest, ShowValue_SetTrue)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);

    slider.setShowValue(false);
    EXPECT_FALSE(slider.showValue());

    slider.setShowValue(true);
    EXPECT_TRUE(slider.showValue());
}

TEST(SliderWidgetTest, DisplayText_WithFormatCallback)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);

    slider.setFormatCallback([](f64 value) -> std::string { return std::to_string(static_cast<i32>(value)) + "%"; });

    EXPECT_EQ("50%", slider.displayText());

    slider.setValue(75.0);
    EXPECT_EQ("75%", slider.displayText());
}

TEST(SliderWidgetTest, DisplayText_WithCustomText)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);

    slider.setDisplayText("Custom Label");
    EXPECT_EQ("Custom Label", slider.displayText());
}

TEST(SliderWidgetTest, DisplayText_DefaultFormatValue)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 42.0);

    // 无 formatCallback 且无 displayText 时，使用默认 formatValue
    std::string text = slider.displayText();
    EXPECT_FALSE(text.empty());
}

// ==================== 键盘测试 ====================

TEST(SliderWidgetTest, KeyLeftRight)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);
    slider.setActive(true);
    slider.setVisible(true);
    slider.setFocused(true);
    slider.setStepSize(5.0);

    f64 initialValue = slider.value();

    // 右键增加值
    slider.onKey(mc::Keys::Right, 0, 1, 0);
    EXPECT_GT(slider.value(), initialValue);

    // 左键减少值
    f64 afterRight = slider.value();
    slider.onKey(mc::Keys::Left, 0, 1, 0);
    EXPECT_LT(slider.value(), afterRight);
}

// ==================== Paint 渲染测试 ====================

namespace {

/**
 * @brief 记录绘制调用的测试画布
 *
 * 只覆盖 SliderWidget paint 测试需要的调用，其余接口保持空实现。
 */
class SliderTestCanvas final : public paint::ICanvas {
public:
    void reset()
    {
        filledRectCount = 0;
        borderCount = 0;
        textCalled = false;
        lastText.clear();
        lastTextColor = 0;
        lastTextX = 0.0f;
        lastTextY = 0.0f;
    }

    void drawRect(const Rect& rect, const paint::IPaint& paint) override
    {
        ++filledRectCount;
        lastFilledRect = rect;
        lastFilledColor = paint.color().toARGB();
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
        textCalled = true;
        lastText = text;
        lastTextX = x;
        lastTextY = y;
        lastTextColor = paint.color().toARGB();
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
    [[nodiscard]] i32 width() const override { return 800; }
    [[nodiscard]] i32 height() const override { return 600; }
    [[nodiscard]] f32 getTextWidth(const std::string& text) const override
    {
        return static_cast<f32>(text.size()) * 6.0f;
    }
    [[nodiscard]] u32 getFontHeight() const override { return 12; }

    int filledRectCount = 0;
    int borderCount = 0;
    bool textCalled = false;
    std::string lastText;
    f32 lastTextX = 0.0f;
    f32 lastTextY = 0.0f;
    u32 lastTextColor = 0;
    Rect lastFilledRect{};
    u32 lastFilledColor = 0;
};

} // namespace

TEST(SliderWidgetTest, Paint_ShowValueTrue_DrawsText)
{
    SliderTestCanvas canvas;
    PaintContext ctx(canvas);

    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);
    slider.setVisible(true);
    ASSERT_TRUE(slider.showValue()); // 默认 m_showValue = true

    slider.paint(ctx);

    // 验证 drawText 被调用，文本为 displayText() 的结果
    EXPECT_TRUE(canvas.textCalled);
    EXPECT_EQ(slider.displayText(), canvas.lastText);
    // 文本颜色应为 WHITE
    EXPECT_EQ(mc::client::Colors::WHITE, canvas.lastTextColor);
}

TEST(SliderWidgetTest, Paint_ShowValueFalse_NoTextDrawn)
{
    SliderTestCanvas canvas;
    PaintContext ctx(canvas);

    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);
    slider.setVisible(true);
    slider.setShowValue(false);

    slider.paint(ctx);

    // m_showValue = false 时不应绘制文本
    EXPECT_FALSE(canvas.textCalled);
}

TEST(SliderWidgetTest, Paint_DrawsTrackAndKnob)
{
    SliderTestCanvas canvas;
    PaintContext ctx(canvas);

    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);
    slider.setVisible(true);

    slider.paint(ctx);

    // paint() 应至少绘制轨道背景和手柄（两次 drawRect 调用）
    // 以及值文本（一次 drawText）
    EXPECT_GE(canvas.filledRectCount, 2);
    EXPECT_TRUE(canvas.textCalled);
}

TEST(SliderWidgetTest, Paint_FormatCallbackText)
{
    SliderTestCanvas canvas;
    PaintContext ctx(canvas);

    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);
    slider.setVisible(true);
    slider.setFormatCallback([](f64 value) -> std::string { return std::to_string(static_cast<i32>(value)) + "%"; });

    slider.paint(ctx);

    EXPECT_TRUE(canvas.textCalled);
    EXPECT_EQ("50%", canvas.lastText);
}

TEST(SliderWidgetTest, Paint_InvisibleWidget_NoDrawing)
{
    SliderTestCanvas canvas;
    PaintContext ctx(canvas);

    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);
    slider.setVisible(false);

    slider.paint(ctx);

    // 不可见时不应绘制任何内容
    EXPECT_FALSE(canvas.textCalled);
    EXPECT_EQ(0, canvas.filledRectCount);
}
