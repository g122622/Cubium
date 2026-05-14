/**
 * @file CheckboxWidgetTest.cpp
 * @brief CheckboxWidget单元测试
 */

#include "client/ui/kagero/widget/CheckboxWidget.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::widget;
using namespace mc::client::Colors;
using namespace mc;

// ==================== 构造函数测试 ====================

TEST(CheckboxWidgetTest, DefaultConstructor)
{
    CheckboxWidget checkbox;
    EXPECT_TRUE(checkbox.id().empty());
    EXPECT_TRUE(checkbox.text().empty());
    EXPECT_FALSE(checkbox.isChecked());
}

TEST(CheckboxWidgetTest, ConstructorWithText)
{
    CheckboxWidget checkbox("chk_option", 10, 20, "Enable Feature");

    EXPECT_EQ("chk_option", checkbox.id());
    EXPECT_EQ("Enable Feature", checkbox.text());
    EXPECT_EQ(10, checkbox.x());
    EXPECT_EQ(20, checkbox.y());
    EXPECT_FALSE(checkbox.isChecked());
}

TEST(CheckboxWidgetTest, ConstructorWithSize)
{
    CheckboxWidget checkbox("chk_option", 10, 20, 150, 30, "Enable Feature");

    EXPECT_EQ("chk_option", checkbox.id());
    EXPECT_EQ("Enable Feature", checkbox.text());
    EXPECT_EQ(10, checkbox.x());
    EXPECT_EQ(20, checkbox.y());
    EXPECT_EQ(150, checkbox.width());
    EXPECT_EQ(30, checkbox.height());
}

// ==================== 选中状态测试 ====================

TEST(CheckboxWidgetTest, SetChecked)
{
    CheckboxWidget checkbox("chk_test", 0, 0, "Test");

    EXPECT_FALSE(checkbox.isChecked());

    checkbox.setChecked(true);
    EXPECT_TRUE(checkbox.isChecked());

    checkbox.setChecked(false);
    EXPECT_FALSE(checkbox.isChecked());
}

TEST(CheckboxWidgetTest, Toggle)
{
    CheckboxWidget checkbox("chk_test", 0, 0, "Test");

    EXPECT_FALSE(checkbox.isChecked());

    checkbox.toggle();
    EXPECT_TRUE(checkbox.isChecked());

    checkbox.toggle();
    EXPECT_FALSE(checkbox.isChecked());
}

// ==================== 回调测试 ====================

TEST(CheckboxWidgetTest, OnChangedCallback)
{
    CheckboxWidget checkbox("chk_test", 0, 0, "Test");

    bool lastValue = false;
    int callCount = 0;
    checkbox.setOnChanged([&lastValue, &callCount](bool checked) {
        lastValue = checked;
        ++callCount;
    });

    checkbox.setChecked(true);
    EXPECT_TRUE(lastValue);
    EXPECT_EQ(1, callCount);

    checkbox.setChecked(false);
    EXPECT_FALSE(lastValue);
    EXPECT_EQ(2, callCount);

    // 相同值不触发回调
    checkbox.setChecked(false);
    EXPECT_EQ(2, callCount);
}

TEST(CheckboxWidgetTest, ToggleCallback)
{
    CheckboxWidget checkbox("chk_test", 0, 0, "Test");

    bool lastValue = false;
    int callCount = 0;
    checkbox.setOnChanged([&lastValue, &callCount](bool checked) {
        lastValue = checked;
        ++callCount;
    });

    checkbox.toggle();
    EXPECT_TRUE(lastValue);
    EXPECT_EQ(1, callCount);

    checkbox.toggle();
    EXPECT_FALSE(lastValue);
    EXPECT_EQ(2, callCount);
}

// ==================== 点击测试 ====================

TEST(CheckboxWidgetTest, ClickToggles)
{
    CheckboxWidget checkbox("chk_test", 0, 0, 100, 20, "Test");
    checkbox.setActive(true);
    checkbox.setVisible(true);

    EXPECT_FALSE(checkbox.isChecked());

    // 左键点击
    bool result = checkbox.onClick(10, 10, 0);
    EXPECT_TRUE(result);
    EXPECT_TRUE(checkbox.isChecked());

    result = checkbox.onClick(10, 10, 0);
    EXPECT_TRUE(result);
    EXPECT_FALSE(checkbox.isChecked());
}

TEST(CheckboxWidgetTest, ClickDisabled)
{
    CheckboxWidget checkbox("chk_test", 0, 0, 100, 20, "Test");
    checkbox.setActive(false);

    EXPECT_FALSE(checkbox.isChecked());

    bool result = checkbox.onClick(10, 10, 0);
    EXPECT_FALSE(result);
    EXPECT_FALSE(checkbox.isChecked());
}

TEST(CheckboxWidgetTest, ClickRightButtonIgnored)
{
    CheckboxWidget checkbox("chk_test", 0, 0, 100, 20, "Test");
    checkbox.setActive(true);
    checkbox.setVisible(true);

    // 右键点击不触发
    bool result = checkbox.onClick(10, 10, 1);
    EXPECT_FALSE(result);
    EXPECT_FALSE(checkbox.isChecked());
}

// ==================== 颜色测试 ====================

TEST(CheckboxWidgetTest, SetTextColor)
{
    CheckboxWidget checkbox("chk_test", 0, 0, "Test");

    checkbox.setTextColor(RED);
    EXPECT_EQ(RED, checkbox.textColor());
}

TEST(CheckboxWidgetTest, SetCheckColor)
{
    CheckboxWidget checkbox("chk_test", 0, 0, "Test");

    checkbox.setCheckColor(MC_GREEN);
    EXPECT_EQ(MC_GREEN, checkbox.checkColor());
}

// ==================== BoxSize测试 ====================

TEST(CheckboxWidgetTest, BoxSize)
{
    CheckboxWidget checkbox("chk_test", 0, 0, 100, 50, "Test");

    // Box size should be minimum of width and height
    EXPECT_EQ(50, checkbox.boxSize());

    checkbox.setSize(30, 100);
    EXPECT_EQ(30, checkbox.boxSize());
}

// ==================== 文本测试 ====================

TEST(CheckboxWidgetTest, SetText)
{
    CheckboxWidget checkbox("chk_test", 0, 0, "Initial");
    EXPECT_EQ("Initial", checkbox.text());

    checkbox.setText("Updated");
    EXPECT_EQ("Updated", checkbox.text());
}
