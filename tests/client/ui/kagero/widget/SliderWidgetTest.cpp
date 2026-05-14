/**
 * @file SliderWidgetTest.cpp
 * @brief SliderWidget单元测试
 */

#include "client/ui/kagero/widget/SliderWidget.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::widget;
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

    slider.onClick(50, 10, 0);
    EXPECT_TRUE(slider.isFocused());
    EXPECT_TRUE(slider.isDragging());
}

TEST(SliderWidgetTest, DraggingState)
{
    SliderWidget slider("test", 0, 0, 100, 20, 0.0, 100.0, 50.0);
    slider.setActive(true);
    slider.setVisible(true);

    EXPECT_FALSE(slider.isDragging());

    slider.onClick(50, 10, 0);
    EXPECT_TRUE(slider.isDragging());

    slider.onRelease(50, 10, 0);
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
    slider.onKey(262, 0, 1, 0); // GLFW_KEY_RIGHT
    EXPECT_GT(slider.value(), initialValue);

    // 左键减少值
    f64 afterRight = slider.value();
    slider.onKey(263, 0, 1, 0); // GLFW_KEY_LEFT
    EXPECT_LT(slider.value(), afterRight);
}
