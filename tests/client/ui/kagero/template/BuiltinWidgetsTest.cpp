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
 * @file BuiltinWidgetsTest.cpp
 * @brief BuiltinWidgets 工厂单元测试
 *
 * 覆盖 grid/slot 等内置 Widget 的属性解析与生效链路：
 * - grid 的 cols/rows/gap 属性正确写入 GridConfig
 * - slot 的 index 属性正确写入 SlotWidget::slotIndex()
 * - 属性缺失时保持默认值
 * - 非法值（负数、零、非数字）按 widget_attrs::parseInt 语义回退
 */

#include "client/ui/kagero/template/bindings/BuiltinWidgets.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/layout/algorithms/GridLayout.hpp"
#include "client/ui/kagero/widget/ContainerWidget.hpp"
#include "client/ui/kagero/widget/SlotWidget.hpp"
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::tpl::bindings;
using namespace mc::client::ui::kagero::widget;
using namespace mc::client::ui::kagero::layout;

// ==================== Slot Widget 属性解析测试 ====================

TEST(BuiltinWidgetsSlotTest, SlotIndexAttribute)
{
    BuiltinWidgets::instance().initialize();

    std::map<std::string, std::string> attrs{{"index", "5"}};
    auto widget = BuiltinWidgets::instance().create("slot", "slot_0", attrs);
    ASSERT_NE(widget, nullptr);

    auto* slot = dynamic_cast<SlotWidget*>(widget.get());
    ASSERT_NE(slot, nullptr);
    EXPECT_EQ(5, slot->slotIndex());
}

TEST(BuiltinWidgetsSlotTest, SlotDefaultIndexIsMinusOne)
{
    BuiltinWidgets::instance().initialize();

    auto widget = BuiltinWidgets::instance().create("slot", "slot_0", {});
    ASSERT_NE(widget, nullptr);

    auto* slot = dynamic_cast<SlotWidget*>(widget.get());
    ASSERT_NE(slot, nullptr);
    EXPECT_EQ(-1, slot->slotIndex());
}

TEST(BuiltinWidgetsSlotTest, SlotIndexZeroIsAllowed)
{
    BuiltinWidgets::instance().initialize();

    std::map<std::string, std::string> attrs{{"index", "0"}};
    auto widget = BuiltinWidgets::instance().create("slot", "slot_0", attrs);
    ASSERT_NE(widget, nullptr);

    auto* slot = dynamic_cast<SlotWidget*>(widget.get());
    ASSERT_NE(slot, nullptr);
    EXPECT_EQ(0, slot->slotIndex());
}

TEST(BuiltinWidgetsSlotTest, SlotIndexInvalidFallsBackToDefault)
{
    BuiltinWidgets::instance().initialize();

    // parseInt 对非数字字符串返回默认值（-1，来自 slot->slotIndex()）
    std::map<std::string, std::string> attrs{{"index", "abc"}};
    auto widget = BuiltinWidgets::instance().create("slot", "slot_0", attrs);
    ASSERT_NE(widget, nullptr);

    auto* slot = dynamic_cast<SlotWidget*>(widget.get());
    ASSERT_NE(slot, nullptr);
    EXPECT_EQ(-1, slot->slotIndex());
}

TEST(BuiltinWidgetsSlotTest, SlotIdIsApplied)
{
    BuiltinWidgets::instance().initialize();

    auto widget = BuiltinWidgets::instance().create("slot", "my_slot", {});
    ASSERT_NE(widget, nullptr);
    EXPECT_EQ("my_slot", widget->id());
}

// ==================== Grid Widget 属性解析测试 ====================

TEST(BuiltinWidgetsGridTest, GridColsAndRowsAttributes)
{
    BuiltinWidgets::instance().initialize();

    std::map<std::string, std::string> attrs{{"cols", "9"}, {"rows", "3"}};
    auto widget = BuiltinWidgets::instance().create("grid", "grid_0", attrs);
    ASSERT_NE(widget, nullptr);

    auto* container = dynamic_cast<ContainerWidget*>(widget.get());
    ASSERT_NE(container, nullptr);
    EXPECT_EQ(ContainerLayoutType::Grid, container->layoutType());

    const GridConfig& config = container->gridConfig();
    EXPECT_EQ(9, config.columns);
    EXPECT_EQ(3, config.rows);
}

TEST(BuiltinWidgetsGridTest, GridDefaultConfig)
{
    BuiltinWidgets::instance().initialize();

    auto widget = BuiltinWidgets::instance().create("grid", "grid_0", {});
    ASSERT_NE(widget, nullptr);

    auto* container = dynamic_cast<ContainerWidget*>(widget.get());
    ASSERT_NE(container, nullptr);
    EXPECT_EQ(ContainerLayoutType::Grid, container->layoutType());

    // GridConfig 默认：columns=1, rows=0（自动推算）, gaps=0
    const GridConfig& config = container->gridConfig();
    EXPECT_EQ(1, config.columns);
    EXPECT_EQ(0, config.rows);
    EXPECT_EQ(0, config.columnGap);
    EXPECT_EQ(0, config.rowGap);
}

TEST(BuiltinWidgetsGridTest, GridColsClampedToOne)
{
    BuiltinWidgets::instance().initialize();

    // cols=0 或负数应被钳制为 1
    std::map<std::string, std::string> attrs{{"cols", "0"}};
    auto widget = BuiltinWidgets::instance().create("grid", "grid_0", attrs);
    ASSERT_NE(widget, nullptr);

    auto* container = dynamic_cast<ContainerWidget*>(widget.get());
    ASSERT_NE(container, nullptr);
    EXPECT_EQ(1, container->gridConfig().columns);
}

TEST(BuiltinWidgetsGridTest, GridRowsZeroMeansAuto)
{
    BuiltinWidgets::instance().initialize();

    // rows=0 表示自动推算，应保留为 0
    std::map<std::string, std::string> attrs{{"cols", "9"}, {"rows", "0"}};
    auto widget = BuiltinWidgets::instance().create("grid", "grid_0", attrs);
    ASSERT_NE(widget, nullptr);

    auto* container = dynamic_cast<ContainerWidget*>(widget.get());
    ASSERT_NE(container, nullptr);
    EXPECT_EQ(0, container->gridConfig().rows);
}

TEST(BuiltinWidgetsGridTest, GridGapAttributeSetsBothGaps)
{
    BuiltinWidgets::instance().initialize();

    // gap 属性同时设置 columnGap 与 rowGap
    std::map<std::string, std::string> attrs{{"cols", "3"}, {"gap", "8"}};
    auto widget = BuiltinWidgets::instance().create("grid", "grid_0", attrs);
    ASSERT_NE(widget, nullptr);

    auto* container = dynamic_cast<ContainerWidget*>(widget.get());
    ASSERT_NE(container, nullptr);
    EXPECT_EQ(8, container->gridConfig().columnGap);
    EXPECT_EQ(8, container->gridConfig().rowGap);
}

TEST(BuiltinWidgetsGridTest, GridGapClampedToZero)
{
    BuiltinWidgets::instance().initialize();

    // 负数 gap 应被钳制为 0
    std::map<std::string, std::string> attrs{{"gap", "-5"}};
    auto widget = BuiltinWidgets::instance().create("grid", "grid_0", attrs);
    ASSERT_NE(widget, nullptr);

    auto* container = dynamic_cast<ContainerWidget*>(widget.get());
    ASSERT_NE(container, nullptr);
    EXPECT_EQ(0, container->gridConfig().columnGap);
    EXPECT_EQ(0, container->gridConfig().rowGap);
}

TEST(BuiltinWidgetsGridTest, GridInvalidColsFallsBackToDefault)
{
    BuiltinWidgets::instance().initialize();

    // parseInt 对非数字返回默认值 1
    std::map<std::string, std::string> attrs{{"cols", "abc"}};
    auto widget = BuiltinWidgets::instance().create("grid", "grid_0", attrs);
    ASSERT_NE(widget, nullptr);

    auto* container = dynamic_cast<ContainerWidget*>(widget.get());
    ASSERT_NE(container, nullptr);
    EXPECT_EQ(1, container->gridConfig().columns);
}

TEST(BuiltinWidgetsGridTest, GridIdIsApplied)
{
    BuiltinWidgets::instance().initialize();

    auto widget = BuiltinWidgets::instance().create("grid", "my_grid", {});
    ASSERT_NE(widget, nullptr);
    EXPECT_EQ("my_grid", widget->id());
}

// ==================== Grid 布局端到端测试 ====================

TEST(BuiltinWidgetsGridLayoutTest, GridColsRowsTakeEffectAfterRelayout)
{
    // 验证 BuiltinWidgets::create 创建的 grid 容器，relayout 后子项按 cols 排列。
    // 这覆盖了 BuiltinWidgets 属性解析 -> GridConfig -> ContainerWidget::relayout
    //   -> LayoutEngine::layoutGrid -> GridLayout::compute 的完整链路。
    BuiltinWidgets::instance().initialize();

    std::map<std::string, std::string> attrs{{"cols", "3"}, {"rows", "1"}};
    auto widget = BuiltinWidgets::instance().create("grid", "grid_0", attrs);
    ASSERT_NE(widget, nullptr);

    auto* container = dynamic_cast<ContainerWidget*>(widget.get());
    ASSERT_NE(container, nullptr);

    // 设置容器尺寸为 300x100，3列1行，每格 100x100
    container->setBounds(Rect(0, 0, 300, 100));

    // 添加 3 个子 widget
    auto child0 = std::make_unique<Widget>("c0");
    auto child1 = std::make_unique<Widget>("c1");
    auto child2 = std::make_unique<Widget>("c2");
    child0->setBounds(Rect(0, 0, 100, 100));
    child1->setBounds(Rect(0, 0, 100, 100));
    child2->setBounds(Rect(0, 0, 100, 100));
    container->addChild(std::move(child0));
    container->addChild(std::move(child1));
    container->addChild(std::move(child2));

    // 触发布局
    container->requestLayout();
    container->relayout();

    // 子项应被均匀排成 3 列：x = 0, 100, 200
    const auto& children = container->widgets();
    ASSERT_EQ(3u, children.size());
    EXPECT_EQ(0, children[0]->x());
    EXPECT_EQ(100, children[1]->x());
    EXPECT_EQ(200, children[2]->x());
}

TEST(BuiltinWidgetsGridLayoutTest, GridRowsTakesEffectAfterRelayout)
{
    // 验证 rows 属性在 relayout 后实际生效：cols=2, rows=2，4 个子项排成 2x2 网格。
    BuiltinWidgets::instance().initialize();

    std::map<std::string, std::string> attrs{{"cols", "2"}, {"rows", "2"}};
    auto widget = BuiltinWidgets::instance().create("grid", "grid_0", attrs);
    ASSERT_NE(widget, nullptr);

    auto* container = dynamic_cast<ContainerWidget*>(widget.get());
    ASSERT_NE(container, nullptr);

    // 容器 200x200，2x2 网格，每格 100x100
    container->setBounds(Rect(0, 0, 200, 200));

    auto child0 = std::make_unique<Widget>("c0");
    auto child1 = std::make_unique<Widget>("c1");
    auto child2 = std::make_unique<Widget>("c2");
    auto child3 = std::make_unique<Widget>("c3");
    child0->setBounds(Rect(0, 0, 100, 100));
    child1->setBounds(Rect(0, 0, 100, 100));
    child2->setBounds(Rect(0, 0, 100, 100));
    child3->setBounds(Rect(0, 0, 100, 100));
    container->addChild(std::move(child0));
    container->addChild(std::move(child1));
    container->addChild(std::move(child2));
    container->addChild(std::move(child3));

    container->requestLayout();
    container->relayout();

    // 2x2 排列：
    //   (0,0) (100,0)
    //   (0,100) (100,100)
    const auto& children = container->widgets();
    ASSERT_EQ(4u, children.size());
    EXPECT_EQ(0, children[0]->x());
    EXPECT_EQ(0, children[0]->y());
    EXPECT_EQ(100, children[1]->x());
    EXPECT_EQ(0, children[1]->y());
    EXPECT_EQ(0, children[2]->x());
    EXPECT_EQ(100, children[2]->y());
    EXPECT_EQ(100, children[3]->x());
    EXPECT_EQ(100, children[3]->y());
}

TEST(BuiltinWidgetsGridLayoutTest, GridGapTakesEffectAfterRelayout)
{
    // 验证 gap 属性在 relayout 后实际生效：cols=3, gap=8，子项间距为 8。
    // 容器宽 316，3 列 + 2 个 gap：cellWidth = (316 - 2*8) / 3 = 100
    // 子项 x = 0, 108, 216
    BuiltinWidgets::instance().initialize();

    std::map<std::string, std::string> attrs{{"cols", "3"}, {"rows", "1"}, {"gap", "8"}};
    auto widget = BuiltinWidgets::instance().create("grid", "grid_0", attrs);
    ASSERT_NE(widget, nullptr);

    auto* container = dynamic_cast<ContainerWidget*>(widget.get());
    ASSERT_NE(container, nullptr);

    container->setBounds(Rect(0, 0, 316, 100));

    auto child0 = std::make_unique<Widget>("c0");
    auto child1 = std::make_unique<Widget>("c1");
    auto child2 = std::make_unique<Widget>("c2");
    child0->setBounds(Rect(0, 0, 100, 100));
    child1->setBounds(Rect(0, 0, 100, 100));
    child2->setBounds(Rect(0, 0, 100, 100));
    container->addChild(std::move(child0));
    container->addChild(std::move(child1));
    container->addChild(std::move(child2));

    container->requestLayout();
    container->relayout();

    const auto& children = container->widgets();
    ASSERT_EQ(3u, children.size());
    // cellWidth = (316 - 2*8) / 3 = 100
    // x = 0, 0+100+8=108, 108+100+8=216
    EXPECT_EQ(0, children[0]->x());
    EXPECT_EQ(108, children[1]->x());
    EXPECT_EQ(216, children[2]->x());
    // 间距确实为 8（cellWidth + gap = 108）
    EXPECT_EQ(8, children[1]->x() - children[0]->x() - children[0]->width());
    EXPECT_EQ(8, children[2]->x() - children[1]->x() - children[1]->width());
}

TEST(BuiltinWidgetsGridLayoutTest, GridGapAffectsRowSpacingAfterRelayout)
{
    // 验证 gap 属性同时影响行间距：cols=1, rows=2, gap=10
    // 容器高 210，2 行 + 1 个 rowGap：cellHeight = (210 - 10) / 2 = 100
    // 子项 y = 0, 110
    BuiltinWidgets::instance().initialize();

    std::map<std::string, std::string> attrs{{"cols", "1"}, {"rows", "2"}, {"gap", "10"}};
    auto widget = BuiltinWidgets::instance().create("grid", "grid_0", attrs);
    ASSERT_NE(widget, nullptr);

    auto* container = dynamic_cast<ContainerWidget*>(widget.get());
    ASSERT_NE(container, nullptr);

    container->setBounds(Rect(0, 0, 100, 210));

    auto child0 = std::make_unique<Widget>("c0");
    auto child1 = std::make_unique<Widget>("c1");
    child0->setBounds(Rect(0, 0, 100, 100));
    child1->setBounds(Rect(0, 0, 100, 100));
    container->addChild(std::move(child0));
    container->addChild(std::move(child1));

    container->requestLayout();
    container->relayout();

    const auto& children = container->widgets();
    ASSERT_EQ(2u, children.size());
    // cellHeight = (210 - 10) / 2 = 100
    // y = 0, 0+100+10=110
    EXPECT_EQ(0, children[0]->y());
    EXPECT_EQ(110, children[1]->y());
    // 行间距确实为 10
    EXPECT_EQ(10, children[1]->y() - children[0]->y() - children[0]->height());
}

// ==================== parseInt 负数字符串显式断言测试 ====================

TEST(BuiltinWidgetsParseIntTest, ParseIntNegativeStringReturnsNegativeValue)
{
    // 显式验证 widget_attrs::parseInt 对负数字符串的返回值
    // 这是 GridGapClampedToZero 用例假设的基石：parseInt("-5") 返回 -5，
    // 然后 BuiltinWidgets 的 std::max(0, ...) 才会将其钳制为 0。
    using namespace mc::client::ui::kagero::tpl::bindings::widget_attrs;
    EXPECT_EQ(-5, parseInt("-5", 0));
    EXPECT_EQ(-1, parseInt("-1", 0));
    EXPECT_EQ(-100, parseInt("-100", 7));
}

TEST(BuiltinWidgetsParseIntTest, ParseIntNegativeStringAppliedToGridGap)
{
    // 验证完整链路：gap="-5" -> parseInt 返回 -5 -> std::max(0, -5) = 0 -> columnGap/rowGap = 0
    BuiltinWidgets::instance().initialize();

    std::map<std::string, std::string> attrs{{"gap", "-5"}};
    auto widget = BuiltinWidgets::instance().create("grid", "grid_0", attrs);
    ASSERT_NE(widget, nullptr);

    auto* container = dynamic_cast<ContainerWidget*>(widget.get());
    ASSERT_NE(container, nullptr);
    // -5 经过 parseInt 解析为 -5，再经 std::max(0, -5) 钳制为 0
    EXPECT_EQ(0, container->gridConfig().columnGap);
    EXPECT_EQ(0, container->gridConfig().rowGap);
}

TEST(BuiltinWidgetsParseIntTest, ParseIntNegativeStringAppliedToSlotIndex)
{
    // 验证 slot 的 index 属性可以接受负数（parseInt 直接返回负数，slot 不钳制）
    BuiltinWidgets::instance().initialize();

    std::map<std::string, std::string> attrs{{"index", "-3"}};
    auto widget = BuiltinWidgets::instance().create("slot", "slot_0", attrs);
    ASSERT_NE(widget, nullptr);

    auto* slot = dynamic_cast<SlotWidget*>(widget.get());
    ASSERT_NE(slot, nullptr);
    EXPECT_EQ(-3, slot->slotIndex());
}

TEST(BuiltinWidgetsParseIntTest, ParseIntNegativeStringAppliedToGridCols)
{
    // 验证 cols 负数经 parseInt 解析为负数，再被 std::max(1, ...) 钳制为 1
    BuiltinWidgets::instance().initialize();

    std::map<std::string, std::string> attrs{{"cols", "-3"}};
    auto widget = BuiltinWidgets::instance().create("grid", "grid_0", attrs);
    ASSERT_NE(widget, nullptr);

    auto* container = dynamic_cast<ContainerWidget*>(widget.get());
    ASSERT_NE(container, nullptr);
    EXPECT_EQ(1, container->gridConfig().columns);
}

TEST(BuiltinWidgetsParseIntTest, ParseIntNegativeStringAppliedToGridRows)
{
    // 验证 rows 负数经 parseInt 解析为负数，再被 std::max(0, ...) 钳制为 0（自动推算）
    BuiltinWidgets::instance().initialize();

    std::map<std::string, std::string> attrs{{"rows", "-2"}};
    auto widget = BuiltinWidgets::instance().create("grid", "grid_0", attrs);
    ASSERT_NE(widget, nullptr);

    auto* container = dynamic_cast<ContainerWidget*>(widget.get());
    ASSERT_NE(container, nullptr);
    EXPECT_EQ(0, container->gridConfig().rows);
}

// ==================== GridLayout auto-placement 验证测试 ====================

TEST(BuiltinWidgetsGridLayoutTest, AutoPlacementFillsRowFirst)
{
    // 直接验证 GridLayout auto-placement 行为：3 列 5 个子项，应排成 2 行
    // 第 0/1/2 子项在第 0 行（col 0/1/2），第 3/4 子项在第 1 行（col 0/1）
    BuiltinWidgets::instance().initialize();

    std::map<std::string, std::string> attrs{{"cols", "3"}}; // rows 默认 0 = 自动推算
    auto widget = BuiltinWidgets::instance().create("grid", "grid_0", attrs);
    ASSERT_NE(widget, nullptr);

    auto* container = dynamic_cast<ContainerWidget*>(widget.get());
    ASSERT_NE(container, nullptr);

    // rows 配置仍为 0（自动推算）；GridLayout::compute 内部 _resolveRows(5) = ceil(5/3) = 2
    EXPECT_EQ(0, container->gridConfig().rows);

    container->setBounds(Rect(0, 0, 300, 200));

    for (int i = 0; i < 5; ++i) {
        auto child = std::make_unique<Widget>("c" + std::to_string(i));
        child->setBounds(Rect(0, 0, 100, 100));
        container->addChild(std::move(child));
    }

    container->requestLayout();
    container->relayout();

    const auto& children = container->widgets();
    ASSERT_EQ(5u, children.size());

    // 容器 300x200，3 列 2 行，cellWidth = 100, cellHeight = 100
    // 行优先填充：
    //   c0(0,0)    c1(100,0)  c2(200,0)
    //   c3(0,100)  c4(100,100)
    EXPECT_EQ(0, children[0]->x());
    EXPECT_EQ(0, children[0]->y());

    EXPECT_EQ(100, children[1]->x());
    EXPECT_EQ(0, children[1]->y());

    EXPECT_EQ(200, children[2]->x());
    EXPECT_EQ(0, children[2]->y());

    EXPECT_EQ(0, children[3]->x());
    EXPECT_EQ(100, children[3]->y());

    EXPECT_EQ(100, children[4]->x());
    EXPECT_EQ(100, children[4]->y());
}
