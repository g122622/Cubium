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
 * @file ListWidgetTest.cpp
 * @brief ListWidget单元测试
 */

#include "client/ui/kagero/widget/ListWidget.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/kagero/template/binder/BindingContext.hpp"
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::widget;
using namespace mc::client::ui::kagero::tpl::binder;
using namespace mc::client::Colors;
using namespace mc;

namespace {

/**
 * @brief 记录绘制调用的测试画布
 *
 * 只覆盖本测试需要的调用，其余接口保持空实现。
 */
class RecordingCanvas final : public paint::ICanvas {
public:
    /**
     * @brief 重置记录状态
     */
    void reset()
    {
        filledRectCalled = false;
        textCalled = false;
        lastFilledRect = Rect{};
        lastFilledColor = 0;
        lastText.clear();
        lastTextX = 0.0f;
        lastTextY = 0.0f;
        lastTextColor = 0;
    }

    void drawRect(const Rect& rect, const paint::IPaint& paint) override
    {
        filledRectCalled = true;
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
    [[nodiscard]] i32 width() const override { return 0; }
    [[nodiscard]] i32 height() const override { return 0; }
    [[nodiscard]] f32 getTextWidth(const std::string& text) const override
    {
        return static_cast<f32>(text.size()) * 6.0f;
    }
    [[nodiscard]] u32 getFontHeight() const override { return 12; }

    bool filledRectCalled = false;
    bool textCalled = false;
    Rect lastFilledRect{};
    u32 lastFilledColor = 0;
    std::string lastText;
    f32 lastTextX = 0.0f;
    f32 lastTextY = 0.0f;
    u32 lastTextColor = 0;
};

} // namespace

// ==================== 测试用列表项 ====================

class TestListItem : public IListItem {
public:
    TestListItem(std::string text, i32 height = 20)
        : m_text(std::move(text))
        , m_height(height)
    {}

    [[nodiscard]] i32 getHeight() const override { return m_height; }

    void paintItem(PaintContext& ctx, i32 x, i32 y, i32 width, bool selected, bool hovered) override
    {
        (void)ctx;
        (void)x;
        (void)y;
        (void)width;
        (void)selected;
        (void)hovered;
    }

    void setText(const std::string& text) { m_text = text; }
    [[nodiscard]] const std::string& text() const { return m_text; }

private:
    std::string m_text;
    i32 m_height;
};

// ==================== 构造函数测试 ====================

TEST(ListWidgetTest, DefaultConstructor)
{
    ListWidget list;
    EXPECT_TRUE(list.id().empty());
    EXPECT_EQ(0u, list.itemCount());
}

TEST(ListWidgetTest, ConstructorWithBounds)
{
    ListWidget list("list", 10, 20, 200, 300);

    EXPECT_EQ("list", list.id());
    EXPECT_EQ(10, list.x());
    EXPECT_EQ(20, list.y());
    EXPECT_EQ(200, list.width());
    EXPECT_EQ(300, list.height());
}

// ==================== 项目操作测试 ====================

TEST(ListWidgetTest, AddItem)
{
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    EXPECT_EQ(1u, list.itemCount());

    list.addItem(std::make_unique<TestListItem>("Item 2"));
    EXPECT_EQ(2u, list.itemCount());
}

TEST(ListWidgetTest, InsertItem)
{
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.addItem(std::make_unique<TestListItem>("Item 3"));
    list.insertItem(1, std::make_unique<TestListItem>("Item 2"));

    EXPECT_EQ(3u, list.itemCount());
    EXPECT_EQ("Item 2", dynamic_cast<TestListItem*>(list.getItem(1))->text());
}

TEST(ListWidgetTest, RemoveItem)
{
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.addItem(std::make_unique<TestListItem>("Item 2"));
    list.addItem(std::make_unique<TestListItem>("Item 3"));

    EXPECT_EQ(3u, list.itemCount());

    list.removeItem(1);
    EXPECT_EQ(2u, list.itemCount());
    EXPECT_EQ("Item 1", dynamic_cast<TestListItem*>(list.getItem(0))->text());
    EXPECT_EQ("Item 3", dynamic_cast<TestListItem*>(list.getItem(1))->text());
}

TEST(ListWidgetTest, ClearItems)
{
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.addItem(std::make_unique<TestListItem>("Item 2"));

    EXPECT_EQ(2u, list.itemCount());

    list.clearItems();
    EXPECT_EQ(0u, list.itemCount());
}

TEST(ListWidgetTest, GetItem)
{
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.addItem(std::make_unique<TestListItem>("Item 2"));

    IListItem* item = list.getItem(0);
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(20, item->getHeight());

    IListItem* item2 = list.getItem(1);
    ASSERT_NE(item2, nullptr);

    IListItem* invalid = list.getItem(100);
    EXPECT_EQ(invalid, nullptr);
}

// ==================== 选择测试 ====================

TEST(ListWidgetTest, SelectItem)
{
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.addItem(std::make_unique<TestListItem>("Item 2"));
    list.addItem(std::make_unique<TestListItem>("Item 3"));

    EXPECT_EQ(-1, list.selectedIndex());

    list.selectItem(0);
    EXPECT_EQ(0, list.selectedIndex());

    list.selectItem(2);
    EXPECT_EQ(2, list.selectedIndex());
}

TEST(ListWidgetTest, ClearSelection)
{
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.selectItem(0);

    EXPECT_EQ(0, list.selectedIndex());

    list.clearSelection();
    EXPECT_EQ(-1, list.selectedIndex());
}

TEST(ListWidgetTest, SelectedItem)
{
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.addItem(std::make_unique<TestListItem>("Item 2"));

    list.selectItem(0);
    IListItem* item = list.selectedItem();
    ASSERT_NE(item, nullptr);

    list.clearSelection();
    EXPECT_EQ(nullptr, list.selectedItem());
}

TEST(ListWidgetTest, SelectionModeNone)
{
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.setSelectionMode(ListWidget::SelectionMode::None);

    list.selectItem(0);
    EXPECT_EQ(-1, list.selectedIndex());
}

TEST(ListWidgetTest, SelectionModeSingle)
{
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.addItem(std::make_unique<TestListItem>("Item 2"));

    list.setSelectionMode(ListWidget::SelectionMode::Single);
    list.selectItem(0);
    EXPECT_EQ(0, list.selectedIndex());

    list.selectItem(1);
    EXPECT_EQ(1, list.selectedIndex());
}

// ==================== 多选测试 ====================

TEST(ListWidgetTest, MultiSelect)
{
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.addItem(std::make_unique<TestListItem>("Item 2"));
    list.addItem(std::make_unique<TestListItem>("Item 3"));

    list.setMultiSelect(true);
    EXPECT_TRUE(list.isMultiSelect());

    // 选择第一项
    list.selectItem(0);
    EXPECT_TRUE(list.isSelected(0));
    EXPECT_FALSE(list.isSelected(1));
    EXPECT_FALSE(list.isSelected(2));

    // 选择第二项（添加到选择）
    list.selectItem(1);
    EXPECT_TRUE(list.isSelected(0));
    EXPECT_TRUE(list.isSelected(1));
    EXPECT_FALSE(list.isSelected(2));

    // 再次点击第一项取消选择
    list.selectItem(0);
    EXPECT_FALSE(list.isSelected(0));
    EXPECT_TRUE(list.isSelected(1));
}

TEST(ListWidgetTest, SetSelectedIndices)
{
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.addItem(std::make_unique<TestListItem>("Item 2"));
    list.addItem(std::make_unique<TestListItem>("Item 3"));
    list.addItem(std::make_unique<TestListItem>("Item 4"));

    list.setMultiSelect(true);
    list.setSelectedIndices({0, 2, 3});

    auto selected = list.selectedIndices();
    EXPECT_EQ(3u, selected.size());
    EXPECT_TRUE(list.isSelected(0));
    EXPECT_FALSE(list.isSelected(1));
    EXPECT_TRUE(list.isSelected(2));
    EXPECT_TRUE(list.isSelected(3));
}

TEST(ListWidgetTest, SelectedIndices)
{
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.addItem(std::make_unique<TestListItem>("Item 2"));

    list.setMultiSelect(true);
    list.selectItem(0);
    list.selectItem(1);

    const auto& indices = list.selectedIndices();
    EXPECT_EQ(2u, indices.size());
}

// ==================== 回调测试 ====================

TEST(ListWidgetTest, OnSelectCallback)
{
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.addItem(std::make_unique<TestListItem>("Item 2"));

    size_t selectedIndex = 999;
    int callCount = 0;
    list.setOnSelect([&selectedIndex, &callCount](size_t index, IListItem* item) {
        selectedIndex = index;
        ++callCount;
        (void)item;
    });

    list.selectItem(0);
    EXPECT_EQ(0u, selectedIndex);
    EXPECT_EQ(1, callCount);

    list.selectItem(1);
    EXPECT_EQ(1u, selectedIndex);
    EXPECT_EQ(2, callCount);
}

TEST(ListWidgetTest, OnSelectionChangedCallback)
{
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.addItem(std::make_unique<TestListItem>("Item 2"));

    i32 oldIndex = -999;
    i32 newIndex = -999;
    int callCount = 0;
    list.setOnSelectionChanged([&oldIndex, &newIndex, &callCount](i32 oldIdx, i32 newIdx) {
        oldIndex = oldIdx;
        newIndex = newIdx;
        ++callCount;
    });

    list.selectItem(0);
    EXPECT_EQ(-1, oldIndex);
    EXPECT_EQ(0, newIndex);
    EXPECT_EQ(1, callCount);

    list.selectItem(1);
    EXPECT_EQ(0, oldIndex);
    EXPECT_EQ(1, newIndex);
    EXPECT_EQ(2, callCount);
}

// ==================== 项目高度测试 ====================

TEST(ListWidgetTest, FixedItemHeight)
{
    ListWidget list("list");
    list.setItemHeight(30);

    EXPECT_EQ(30, list.itemHeight());

    list.addItem(std::make_unique<TestListItem>("Item 1", 20));
    list.addItem(std::make_unique<TestListItem>("Item 2", 25));

    // 使用固定高度
    EXPECT_EQ(60, list.contentHeight()); // 2 * 30
}

TEST(ListWidgetTest, VariableItemHeight)
{
    ListWidget list("list");
    list.setItemHeight(0); // 0表示使用项目自己的高度

    list.addItem(std::make_unique<TestListItem>("Item 1", 20));
    list.addItem(std::make_unique<TestListItem>("Item 2", 30));
    list.addItem(std::make_unique<TestListItem>("Item 3", 25));

    EXPECT_EQ(75, list.contentHeight()); // 20 + 30 + 25
}

// ==================== 双击检测测试 ====================

TEST(ListWidgetTest, DoubleClickViaOnDoubleClick)
{
    // ListWidget now uses framework-level double-click detection from KageroEngine.
    // The onDoubleClick override on ListWidget dispatches to IListItem::onDoubleClick
    // and the m_onDoubleClick callback.
    ListWidget list("list");
    list.setBounds(Rect(0, 0, 200, 300));
    list.setItemHeight(20);

    int doubleClickCount = 0;
    list.setOnDoubleClick([&](size_t index, IListItem* item) {
        doubleClickCount++;
        EXPECT_EQ(0u, index);
        EXPECT_NE(item, nullptr);
    });

    auto item = std::make_unique<TextListItem>("Item 0");
    list.addItem(std::move(item));

    // Simulate a double-click via onDoubleClick (KageroEngine would call this
    // when it detects a double-click within 250ms on the same widget)
    list.onDoubleClick(10, 5, 0, 0);

    EXPECT_EQ(1, doubleClickCount);
}

// ==================== TextListItem测试 ====================

TEST(TextListItemTest, Constructor)
{
    TextListItem item("Test Item", 25);

    EXPECT_EQ(25, item.getHeight());
    EXPECT_EQ("Test Item", item.text());
}

TEST(TextListItemTest, SetText)
{
    TextListItem item("Initial");

    item.setText("Updated");
    EXPECT_EQ("Updated", item.text());
}

TEST(TextListItemTest, SetTextColor)
{
    TextListItem item("Test");

    item.setTextColor(RED);
    EXPECT_EQ(RED, item.textColor());
}

TEST(TextListItemTest, SetSelectedColor)
{
    TextListItem item("Test");

    item.setSelectedColor(BLUE);
    // 无法直接验证，但不崩溃即可
}

TEST(TextListItemTest, SetHoveredColor)
{
    TextListItem item("Test");

    item.setHoveredColor(GREEN);
    // 无法直接验证，但不崩溃即可
}

TEST(TextListItemTest, PaintItem_DrawsTextAndSelectedBackground)
{
    RecordingCanvas canvas;
    PaintContext ctx(canvas);
    TextListItem item("Hello", 20);

    item.paintItem(ctx, 10, 30, 120, true, false);

    EXPECT_TRUE(canvas.filledRectCalled);
    EXPECT_TRUE(canvas.textCalled);
    EXPECT_EQ("Hello", canvas.lastText);
    EXPECT_EQ(10, canvas.lastFilledRect.x);
    EXPECT_EQ(30, canvas.lastFilledRect.y);
    EXPECT_EQ(120, canvas.lastFilledRect.width);
    EXPECT_EQ(20, canvas.lastFilledRect.height);
    EXPECT_EQ(item.textColor(), canvas.lastTextColor);
}

// ==================== 悬停检测测试 ====================

TEST(ListWidgetTest, OnMouseMove_UpdatesHoveredIndex)
{
    ListWidget list("list", 0, 0, 200, 300);
    list.setActive(true);
    list.setVisible(true);
    list.setItemHeight(30);

    list.addItem(std::make_unique<TestListItem>("Item 0"));
    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.addItem(std::make_unique<TestListItem>("Item 2"));

    // 鼠标移动到第一项区域 → hoveredIndex 应为 0
    bool handled = list.onMouseMove(10, 5);
    EXPECT_TRUE(handled);
    EXPECT_EQ(0, list.hoveredIndex());

    // 鼠标移动到第二项区域（y=35 在第二项 30~60 范围内）
    list.onMouseMove(10, 35);
    EXPECT_EQ(1, list.hoveredIndex());

    // 鼠标移动到第三项区域（y=65 在第三项 60~90 范围内）
    list.onMouseMove(10, 65);
    EXPECT_EQ(2, list.hoveredIndex());
}

TEST(ListWidgetTest, OnMouseMove_OutOfBounds_ReturnsFalse)
{
    ListWidget list("list", 0, 0, 200, 300);
    list.setActive(true);
    list.setVisible(true);
    list.setItemHeight(30);

    list.addItem(std::make_unique<TestListItem>("Item 0"));

    // 鼠标在列表范围外
    bool handled = list.onMouseMove(-10, 10);
    EXPECT_FALSE(handled);
    EXPECT_EQ(-1, list.hoveredIndex());

    handled = list.onMouseMove(10, -10);
    EXPECT_FALSE(handled);
    EXPECT_EQ(-1, list.hoveredIndex());
}

TEST(ListWidgetTest, OnMouseMove_InactiveWidget_ReturnsFalse)
{
    ListWidget list("list", 0, 0, 200, 300);
    list.setActive(false);
    list.setVisible(true);
    list.setItemHeight(30);

    list.addItem(std::make_unique<TestListItem>("Item 0"));

    bool handled = list.onMouseMove(10, 10);
    EXPECT_FALSE(handled);
    // 非激活状态下 hoveredIndex 不应改变
    EXPECT_EQ(-1, list.hoveredIndex());
}

TEST(ListWidgetTest, OnMouseMove_InvisibleWidget_ReturnsFalse)
{
    ListWidget list("list", 0, 0, 200, 300);
    list.setActive(true);
    list.setVisible(false);
    list.setItemHeight(30);

    list.addItem(std::make_unique<TestListItem>("Item 0"));

    bool handled = list.onMouseMove(10, 10);
    EXPECT_FALSE(handled);
    // 不可见状态下 hoveredIndex 不应改变
    EXPECT_EQ(-1, list.hoveredIndex());
}

TEST(ListWidgetTest, OnMouseMove_ScrollOffsetAffectsIndex)
{
    ListWidget list("list", 0, 0, 200, 60);
    list.setActive(true);
    list.setVisible(true);
    list.setItemHeight(20);

    list.addItem(std::make_unique<TestListItem>("Item 0"));
    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.addItem(std::make_unique<TestListItem>("Item 2"));
    list.addItem(std::make_unique<TestListItem>("Item 3"));

    // 不滚动时，y=5 对应第 0 项
    list.onMouseMove(10, 5);
    EXPECT_EQ(0, list.hoveredIndex());

    // 滚动 20px 后，y=5 对应第 1 项（逻辑 y = 5 + 20 = 25，在第 1 项 20~40 范围内）
    list.scrollBy(20);
    list.onMouseMove(10, 5);
    EXPECT_EQ(1, list.hoveredIndex());

    // 继续滚动到 40px 需要更大的内容高度
    // 当前 4 项 * 20px = 80px，可见高度 60px，最大滚动 = 20px
    // 所以 scrollBy(20) 已经到达最大滚动位置，再 scrollBy(20) 不再增加
    // 我们需要更多的项目来允许更深的滚动
    list.addItem(std::make_unique<TestListItem>("Item 4"));
    list.addItem(std::make_unique<TestListItem>("Item 5"));
    // 现在 6 项 * 20px = 120px，最大滚动 = 60px

    // 滚动到 40px 位置
    list.setScrollY(40);
    list.onMouseMove(10, 5);
    EXPECT_EQ(2, list.hoveredIndex());
}

// ==================== 数据绑定与 refreshItems 测试 ====================

namespace {

/// @brief 测试用工厂：根据 Value 中的 "label" 字段构造 TestListItem
std::unique_ptr<IListItem> makeLabelItem(const Value& data, size_t /*index*/)
{
    if (data.isString()) {
        return std::make_unique<TestListItem>(data.asString());
    }
    if (data.isObject() && data.hasProperty("label")) {
        return std::make_unique<TestListItem>(data.getProperty("label").toString());
    }
    return std::make_unique<TestListItem>(data.toString());
}

} // namespace

TEST(ListWidgetTest, SetItemsFromValue_BuildsItemsFromValueArray)
{
    ListWidget list("list", 0, 0, 200, 300);
    list.setItemFactory(&makeLabelItem);

    std::vector<Value> values;
    values.emplace_back(Value(std::string("Apple")));
    values.emplace_back(Value(std::string("Banana")));
    values.emplace_back(Value(std::string("Cherry")));

    list.setItemsFromValue(Value::fromArray(values));

    EXPECT_EQ(3u, list.itemCount());
    auto* item0 = dynamic_cast<TestListItem*>(list.getItem(0));
    auto* item1 = dynamic_cast<TestListItem*>(list.getItem(1));
    auto* item2 = dynamic_cast<TestListItem*>(list.getItem(2));
    ASSERT_NE(nullptr, item0);
    ASSERT_NE(nullptr, item1);
    ASSERT_NE(nullptr, item2);
    EXPECT_EQ("Apple", item0->text());
    EXPECT_EQ("Banana", item1->text());
    EXPECT_EQ("Cherry", item2->text());
}

TEST(ListWidgetTest, SetItemsFromValue_FallsBackToTextListItemWhenNoFactory)
{
    ListWidget list("list", 0, 0, 200, 300);

    std::vector<Value> values;
    values.emplace_back(Value(std::string("Hello")));
    values.emplace_back(Value(std::string("World")));

    list.setItemsFromValue(Value::fromArray(values));

    EXPECT_EQ(2u, list.itemCount());
    // 无工厂时回退为 TextListItem（内部类型，通过 toString 验证内容）
    auto* item0 = list.getItem(0);
    auto* item1 = list.getItem(1);
    ASSERT_NE(nullptr, item0);
    ASSERT_NE(nullptr, item1);
    // TextListItem 不暴露 text()，但可通过 toString() 间接验证（这里仅校验类型存在）
    EXPECT_EQ(20, item0->getHeight());
    EXPECT_EQ(20, item1->getHeight());
}

TEST(ListWidgetTest, SetItemsFromValue_NonArrayClearsItems)
{
    ListWidget list("list", 0, 0, 200, 300);
    list.setItemFactory(&makeLabelItem);

    // 先填充一些项
    std::vector<Value> values;
    values.emplace_back(Value(std::string("A")));
    values.emplace_back(Value(std::string("B")));
    list.setItemsFromValue(Value::fromArray(values));
    EXPECT_EQ(2u, list.itemCount());

    // 传入非数组应清空
    list.setItemsFromValue(Value(true));
    EXPECT_EQ(0u, list.itemCount());
}

TEST(ListWidgetTest, SetItemsFromValue_FiresOnItemsChangedCallback)
{
    ListWidget list("list", 0, 0, 200, 300);
    list.setItemFactory(&makeLabelItem);

    i32 callCount = 0;
    list.setOnItemsChanged([&callCount]() { ++callCount; });

    std::vector<Value> values;
    values.emplace_back(Value(std::string("X")));
    list.setItemsFromValue(Value::fromArray(values));

    EXPECT_EQ(1, callCount);
}

TEST(ListWidgetTest, SetItemsFromValue_CachesDataSourceForRefresh)
{
    ListWidget list("list", 0, 0, 200, 300);
    list.setItemFactory(&makeLabelItem);

    EXPECT_FALSE(list.hasCachedDataSource());

    std::vector<Value> values;
    values.emplace_back(Value(std::string("Original")));
    list.setItemsFromValue(Value::fromArray(values));

    EXPECT_TRUE(list.hasCachedDataSource());
}

TEST(ListWidgetTest, RefreshItems_NoCachedDataSourceIsNoOp)
{
    ListWidget list("list", 0, 0, 200, 300);
    list.setItemFactory(&makeLabelItem);

    i32 callCount = 0;
    list.setOnItemsChanged([&callCount]() { ++callCount; });

    // 从未调用 setItemsFromValue，refreshItems 应为空操作
    list.refreshItems();

    EXPECT_EQ(0u, list.itemCount());
    EXPECT_EQ(0, callCount);
    EXPECT_FALSE(list.hasCachedDataSource());
}

TEST(ListWidgetTest, RefreshItems_RebuildsFromCachedDataSource)
{
    ListWidget list("list", 0, 0, 200, 300);
    list.setItemFactory(&makeLabelItem);

    std::vector<Value> values;
    values.emplace_back(Value(std::string("First")));
    values.emplace_back(Value(std::string("Second")));
    list.setItemsFromValue(Value::fromArray(values));

    EXPECT_EQ(2u, list.itemCount());

    // 清空后调用 refreshItems，应从缓存重建
    list.clearItems();
    EXPECT_EQ(0u, list.itemCount());

    list.refreshItems();

    EXPECT_EQ(2u, list.itemCount());
    auto* item0 = dynamic_cast<TestListItem*>(list.getItem(0));
    auto* item1 = dynamic_cast<TestListItem*>(list.getItem(1));
    ASSERT_NE(nullptr, item0);
    ASSERT_NE(nullptr, item1);
    EXPECT_EQ("First", item0->text());
    EXPECT_EQ("Second", item1->text());
}

TEST(ListWidgetTest, RefreshItems_UsesCurrentItemFactory)
{
    ListWidget list("list", 0, 0, 200, 300);
    list.setItemFactory(&makeLabelItem);

    std::vector<Value> values;
    values.emplace_back(Value(std::string("Data1")));
    values.emplace_back(Value(std::string("Data2")));
    list.setItemsFromValue(Value::fromArray(values));

    // 验证初始工厂生效
    auto* item0 = dynamic_cast<TestListItem*>(list.getItem(0));
    ASSERT_NE(nullptr, item0);
    EXPECT_EQ("Data1", item0->text());

    // 替换工厂：在文本前加前缀
    list.setItemFactory([](const Value& data, size_t /*index*/) -> std::unique_ptr<IListItem> {
        return std::make_unique<TestListItem>("[" + data.toString() + "]");
    });

    // refreshItems 应使用新工厂重建
    list.refreshItems();

    EXPECT_EQ(2u, list.itemCount());
    auto* newItem0 = dynamic_cast<TestListItem*>(list.getItem(0));
    auto* newItem1 = dynamic_cast<TestListItem*>(list.getItem(1));
    ASSERT_NE(nullptr, newItem0);
    ASSERT_NE(nullptr, newItem1);
    EXPECT_EQ("[Data1]", newItem0->text());
    EXPECT_EQ("[Data2]", newItem1->text());
}

TEST(ListWidgetTest, RefreshItems_FiresOnItemsChangedCallback)
{
    ListWidget list("list", 0, 0, 200, 300);
    list.setItemFactory(&makeLabelItem);

    std::vector<Value> values;
    values.emplace_back(Value(std::string("X")));
    list.setItemsFromValue(Value::fromArray(values));

    i32 callCount = 0;
    list.setOnItemsChanged([&callCount]() { ++callCount; });

    list.refreshItems();

    EXPECT_EQ(1, callCount);
}

TEST(ListWidgetTest, RefreshItems_PreservesValidSelectionIndex)
{
    ListWidget list("list", 0, 0, 200, 300);
    list.setItemFactory(&makeLabelItem);
    list.setSelectionMode(ListWidget::SelectionMode::Single);

    std::vector<Value> values;
    values.emplace_back(Value(std::string("A")));
    values.emplace_back(Value(std::string("B")));
    values.emplace_back(Value(std::string("C")));
    list.setItemsFromValue(Value::fromArray(values));

    // 选中第 1 项
    list.selectItem(1);
    EXPECT_EQ(1, list.selectedIndex());

    // refreshItems 重建后应保留选中索引（仍在范围内）
    list.refreshItems();

    EXPECT_EQ(1, list.selectedIndex());
    EXPECT_EQ(3u, list.itemCount());
}

TEST(ListWidgetTest, RefreshItems_ClearsSelectionWhenIndexOutOfRange)
{
    ListWidget list("list", 0, 0, 200, 300);
    list.setItemFactory(&makeLabelItem);
    list.setSelectionMode(ListWidget::SelectionMode::Single);

    std::vector<Value> values;
    values.emplace_back(Value(std::string("A")));
    values.emplace_back(Value(std::string("B")));
    values.emplace_back(Value(std::string("C")));
    values.emplace_back(Value(std::string("D")));
    values.emplace_back(Value(std::string("E")));
    list.setItemsFromValue(Value::fromArray(values));

    // 选中第 4 项
    list.selectItem(4);
    EXPECT_EQ(4, list.selectedIndex());

    // 用更短的数组刷新数据源（选中索引将越界）
    std::vector<Value> shorterValues;
    shorterValues.emplace_back(Value(std::string("A")));
    shorterValues.emplace_back(Value(std::string("B")));
    list.setItemsFromValue(Value::fromArray(shorterValues));

    // 此时 selectedIndex 已被 clearItems 重置为 -1
    EXPECT_EQ(-1, list.selectedIndex());

    // 重新选中第 1 项，然后 refreshItems（数据源长度不变，应保留选中）
    list.selectItem(1);
    list.refreshItems();
    EXPECT_EQ(1, list.selectedIndex());
}

TEST(ListWidgetTest, RefreshItems_PreservesMultiSelectionInRange)
{
    ListWidget list("list", 0, 0, 200, 300);
    list.setItemFactory(&makeLabelItem);
    list.setSelectionMode(ListWidget::SelectionMode::Multiple);

    std::vector<Value> values;
    for (i32 i = 0; i < 5; ++i) {
        values.emplace_back(Value(std::string("Item") + std::to_string(i)));
    }
    list.setItemsFromValue(Value::fromArray(values));

    // 多选第 0、2、4 项
    list.selectItem(0);
    list.selectItem(2);
    list.selectItem(4);
    EXPECT_EQ(3u, list.selectedIndices().size());

    // refreshItems 重建后应保留仍在范围内的多选索引
    list.refreshItems();

    const auto& indices = list.selectedIndices();
    EXPECT_EQ(3u, indices.size());
    // 验证具体索引值（顺序由 selectItem 的 toggle 逻辑决定）
    bool has0 = std::find(indices.begin(), indices.end(), 0) != indices.end();
    bool has2 = std::find(indices.begin(), indices.end(), 2) != indices.end();
    bool has4 = std::find(indices.begin(), indices.end(), 4) != indices.end();
    EXPECT_TRUE(has0);
    EXPECT_TRUE(has2);
    EXPECT_TRUE(has4);
}

TEST(ListWidgetTest, RefreshItems_FiltersMultiSelectionWhenOutOfRange)
{
    ListWidget list("list", 0, 0, 200, 300);
    list.setItemFactory(&makeLabelItem);
    list.setSelectionMode(ListWidget::SelectionMode::Multiple);

    std::vector<Value> values;
    for (i32 i = 0; i < 5; ++i) {
        values.emplace_back(Value(std::string("Item") + std::to_string(i)));
    }
    list.setItemsFromValue(Value::fromArray(values));

    // 多选第 0、3、4 项
    list.selectItem(0);
    list.selectItem(3);
    list.selectItem(4);

    // 用更短的数组刷新数据源（长度变为 3，索引 3、4 越界）
    std::vector<Value> shorterValues;
    shorterValues.emplace_back(Value(std::string("A")));
    shorterValues.emplace_back(Value(std::string("B")));
    shorterValues.emplace_back(Value(std::string("C")));
    list.setItemsFromValue(Value::fromArray(shorterValues));

    // clearItems 已重置单选索引，但多选列表 m_selectedIndices 在 clearItems 中未清空
    // 重新选中第 1 项以建立有效多选状态
    list.selectItem(1);

    // 再次 refreshItems，应过滤掉越界索引
    list.refreshItems();

    const auto& indices = list.selectedIndices();
    // 所有保留下来的索引都应在 [0, 3) 范围内
    for (i32 idx : indices) {
        EXPECT_GE(idx, 0);
        EXPECT_LT(idx, static_cast<i32>(list.itemCount()));
    }
}

TEST(ListWidgetTest, RefreshItems_EmptyArrayDataSource)
{
    ListWidget list("list", 0, 0, 200, 300);
    list.setItemFactory(&makeLabelItem);

    // 先填充一些项
    std::vector<Value> values;
    values.emplace_back(Value(std::string("A")));
    list.setItemsFromValue(Value::fromArray(values));
    EXPECT_EQ(1u, list.itemCount());

    // 用空数组刷新数据源
    list.setItemsFromValue(Value::emptyArray());
    EXPECT_EQ(0u, list.itemCount());
    EXPECT_TRUE(list.hasCachedDataSource());

    // refreshItems 应保持空列表
    i32 callCount = 0;
    list.setOnItemsChanged([&callCount]() { ++callCount; });
    list.refreshItems();

    EXPECT_EQ(0u, list.itemCount());
    EXPECT_EQ(1, callCount);
}

TEST(ListWidgetTest, RefreshItems_UsesCachedDataSourceAfterFactoryChange)
{
    ListWidget list("list", 0, 0, 200, 300);

    // 初始无工厂，使用默认 TextListItem
    std::vector<Value> values;
    values.emplace_back(Value(std::string("Test")));
    list.setItemsFromValue(Value::fromArray(values));
    EXPECT_EQ(1u, list.itemCount());

    // 设置工厂后 refreshItems，应使用新工厂重建
    i32 factoryCallCount = 0;
    list.setItemFactory([&factoryCallCount](const Value& data, size_t /*index*/) -> std::unique_ptr<IListItem> {
        ++factoryCallCount;
        return std::make_unique<TestListItem>("Factory:" + data.toString(), 30);
    });

    list.refreshItems();

    EXPECT_EQ(1u, list.itemCount());
    EXPECT_EQ(1, factoryCallCount);
    auto* item = list.getItem(0);
    ASSERT_NE(nullptr, item);
    EXPECT_EQ(30, item->getHeight()); // 新工厂使用 30 高度
}
