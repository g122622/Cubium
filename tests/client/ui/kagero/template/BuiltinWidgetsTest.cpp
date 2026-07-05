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
