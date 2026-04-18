/**
 * @file ListWidgetTest.cpp
 * @brief ListWidget单元测试
 */

#include <gtest/gtest.h>
#include "client/ui/kagero/widget/ListWidget.hpp"
#include "client/ui/kagero/Types.hpp"
#include "client/ui/kagero/paint/PaintContext.hpp"
#include "client/ui/Glyph.hpp"

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::widget;
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
    void reset() {
        filledRectCalled = false;
        textCalled = false;
        lastFilledRect = Rect{};
        lastFilledColor = 0;
        lastText.clear();
        lastTextX = 0.0f;
        lastTextY = 0.0f;
        lastTextColor = 0;
    }

    void drawRect(const Rect& rect, const paint::IPaint& paint) override {
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

    void drawText(const String& text, f32 x, f32 y, const paint::IPaint& paint) override {
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
    [[nodiscard]] f32 getTextWidth(const String& text) const override { return static_cast<f32>(text.size()) * 6.0f; }
    [[nodiscard]] u32 getFontHeight() const override { return 12; }

    bool filledRectCalled = false;
    bool textCalled = false;
    Rect lastFilledRect{};
    u32 lastFilledColor = 0;
    String lastText;
    f32 lastTextX = 0.0f;
    f32 lastTextY = 0.0f;
    u32 lastTextColor = 0;
};

} // namespace

// ==================== 测试用列表项 ====================

class TestListItem : public IListItem {
public:
    TestListItem(String text, i32 height = 20)
        : m_text(std::move(text))
        , m_height(height) {}

    [[nodiscard]] i32 getHeight() const override { return m_height; }

    void paintItem(PaintContext& ctx, i32 x, i32 y, i32 width, bool selected, bool hovered) override {
        (void)ctx;
        (void)x;
        (void)y;
        (void)width;
        (void)selected;
        (void)hovered;
    }

    void setText(const String& text) { m_text = text; }
    [[nodiscard]] const String& text() const { return m_text; }

private:
    String m_text;
    i32 m_height;
};

// ==================== 构造函数测试 ====================

TEST(ListWidgetTest, DefaultConstructor) {
    ListWidget list;
    EXPECT_TRUE(list.id().empty());
    EXPECT_EQ(0u, list.itemCount());
}

TEST(ListWidgetTest, ConstructorWithBounds) {
    ListWidget list("list", 10, 20, 200, 300);

    EXPECT_EQ("list", list.id());
    EXPECT_EQ(10, list.x());
    EXPECT_EQ(20, list.y());
    EXPECT_EQ(200, list.width());
    EXPECT_EQ(300, list.height());
}

// ==================== 项目操作测试 ====================

TEST(ListWidgetTest, AddItem) {
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    EXPECT_EQ(1u, list.itemCount());

    list.addItem(std::make_unique<TestListItem>("Item 2"));
    EXPECT_EQ(2u, list.itemCount());
}

TEST(ListWidgetTest, InsertItem) {
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.addItem(std::make_unique<TestListItem>("Item 3"));
    list.insertItem(1, std::make_unique<TestListItem>("Item 2"));

    EXPECT_EQ(3u, list.itemCount());
    EXPECT_EQ("Item 2", dynamic_cast<TestListItem*>(list.getItem(1))->text());
}

TEST(ListWidgetTest, RemoveItem) {
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

TEST(ListWidgetTest, ClearItems) {
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.addItem(std::make_unique<TestListItem>("Item 2"));

    EXPECT_EQ(2u, list.itemCount());

    list.clearItems();
    EXPECT_EQ(0u, list.itemCount());
}

TEST(ListWidgetTest, GetItem) {
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

TEST(ListWidgetTest, SelectItem) {
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

TEST(ListWidgetTest, ClearSelection) {
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.selectItem(0);

    EXPECT_EQ(0, list.selectedIndex());

    list.clearSelection();
    EXPECT_EQ(-1, list.selectedIndex());
}

TEST(ListWidgetTest, SelectedItem) {
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.addItem(std::make_unique<TestListItem>("Item 2"));

    list.selectItem(0);
    IListItem* item = list.selectedItem();
    ASSERT_NE(item, nullptr);

    list.clearSelection();
    EXPECT_EQ(nullptr, list.selectedItem());
}

TEST(ListWidgetTest, SelectionModeNone) {
    ListWidget list("list");

    list.addItem(std::make_unique<TestListItem>("Item 1"));
    list.setSelectionMode(ListWidget::SelectionMode::None);

    list.selectItem(0);
    EXPECT_EQ(-1, list.selectedIndex());
}

TEST(ListWidgetTest, SelectionModeSingle) {
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

TEST(ListWidgetTest, MultiSelect) {
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

TEST(ListWidgetTest, SetSelectedIndices) {
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

TEST(ListWidgetTest, SelectedIndices) {
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

TEST(ListWidgetTest, OnSelectCallback) {
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

TEST(ListWidgetTest, OnSelectionChangedCallback) {
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

TEST(ListWidgetTest, FixedItemHeight) {
    ListWidget list("list");
    list.setItemHeight(30);

    EXPECT_EQ(30, list.itemHeight());

    list.addItem(std::make_unique<TestListItem>("Item 1", 20));
    list.addItem(std::make_unique<TestListItem>("Item 2", 25));

    // 使用固定高度
    EXPECT_EQ(60, list.contentHeight()); // 2 * 30
}

TEST(ListWidgetTest, VariableItemHeight) {
    ListWidget list("list");
    list.setItemHeight(0); // 0表示使用项目自己的高度

    list.addItem(std::make_unique<TestListItem>("Item 1", 20));
    list.addItem(std::make_unique<TestListItem>("Item 2", 30));
    list.addItem(std::make_unique<TestListItem>("Item 3", 25));

    EXPECT_EQ(75, list.contentHeight()); // 20 + 30 + 25
}

// ==================== 双击检测测试 ====================

TEST(ListWidgetTest, DoubleClickTime) {
    ListWidget list("list");

    EXPECT_EQ(500, list.doubleClickTime());

    list.setDoubleClickTime(300);
    EXPECT_EQ(300, list.doubleClickTime());
}

// ==================== TextListItem测试 ====================

TEST(TextListItemTest, Constructor) {
    TextListItem item("Test Item", 25);

    EXPECT_EQ(25, item.getHeight());
    EXPECT_EQ("Test Item", item.text());
}

TEST(TextListItemTest, SetText) {
    TextListItem item("Initial");

    item.setText("Updated");
    EXPECT_EQ("Updated", item.text());
}

TEST(TextListItemTest, SetTextColor) {
    TextListItem item("Test");

    item.setTextColor(RED);
    EXPECT_EQ(RED, item.textColor());
}

TEST(TextListItemTest, SetSelectedColor) {
    TextListItem item("Test");

    item.setSelectedColor(BLUE);
    // 无法直接验证，但不崩溃即可
}

TEST(TextListItemTest, SetHoveredColor) {
    TextListItem item("Test");

    item.setHoveredColor(GREEN);
    // 无法直接验证，但不崩溃即可
}

TEST(TextListItemTest, PaintItem_DrawsTextAndSelectedBackground) {
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
