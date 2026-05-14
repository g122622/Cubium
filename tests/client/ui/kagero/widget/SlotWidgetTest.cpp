/**
 * @file SlotWidgetTest.cpp
 * @brief SlotWidget单元测试
 */

#include "client/ui/kagero/widget/SlotWidget.hpp"
#include "client/ui/Glyph.hpp"
#include "client/ui/kagero/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero;
using namespace mc::client::ui::kagero::widget;
using namespace mc::client::Colors;
using namespace mc;

// ==================== 构造函数测试 ====================

TEST(SlotWidgetTest, DefaultConstructor)
{
    SlotWidget slot;
    EXPECT_TRUE(slot.id().empty());
    EXPECT_EQ(-1, slot.slotIndex());
    EXPECT_TRUE(slot.showBackground());
    EXPECT_TRUE(slot.isInteractive());
}

TEST(SlotWidgetTest, ConstructorWithPosition)
{
    SlotWidget slot("slot_0", 10, 20);

    EXPECT_EQ("slot_0", slot.id());
    EXPECT_EQ(10, slot.x());
    EXPECT_EQ(20, slot.y());
    EXPECT_EQ(16, slot.width()); // 默认尺寸
    EXPECT_EQ(16, slot.height());
    EXPECT_EQ(-1, slot.slotIndex());
}

TEST(SlotWidgetTest, ConstructorWithSize)
{
    SlotWidget slot("slot_1", 10, 20, 32);

    EXPECT_EQ("slot_1", slot.id());
    EXPECT_EQ(10, slot.x());
    EXPECT_EQ(20, slot.y());
    EXPECT_EQ(32, slot.width());
    EXPECT_EQ(32, slot.height());
}

// ==================== 槽位索引测试 ====================

TEST(SlotWidgetTest, SetSlotIndex)
{
    SlotWidget slot("test", 0, 0);

    slot.setSlotIndex(5);
    EXPECT_EQ(5, slot.slotIndex());

    slot.setSlotIndex(0);
    EXPECT_EQ(0, slot.slotIndex());
}

// ==================== 物品操作测试 ====================

TEST(SlotWidgetTest, SetItem)
{
    SlotWidget slot("test", 0, 0);

    ItemStack item;
    // ItemStack默认构造创建空栈
    slot.setItem(item);

    const ItemStack& retrieved = slot.item();
    EXPECT_TRUE(retrieved.isEmpty());
}

TEST(SlotWidgetTest, ClearItem)
{
    SlotWidget slot("test", 0, 0);

    slot.clearItem();
    EXPECT_TRUE(slot.isEmpty());
}

TEST(SlotWidgetTest, IsEmpty)
{
    SlotWidget slot("test", 0, 0);

    EXPECT_TRUE(slot.isEmpty()); // 默认为空

    slot.clearItem();
    EXPECT_TRUE(slot.isEmpty());
}

// ==================== 显示属性测试 ====================

TEST(SlotWidgetTest, SetShowBackground)
{
    SlotWidget slot("test", 0, 0);

    EXPECT_TRUE(slot.showBackground());

    slot.setShowBackground(false);
    EXPECT_FALSE(slot.showBackground());

    slot.setShowBackground(true);
    EXPECT_TRUE(slot.showBackground());
}

TEST(SlotWidgetTest, SetInteractive)
{
    SlotWidget slot("test", 0, 0);

    EXPECT_TRUE(slot.isInteractive());

    slot.setInteractive(false);
    EXPECT_FALSE(slot.isInteractive());

    slot.setInteractive(true);
    EXPECT_TRUE(slot.isInteractive());
}

TEST(SlotWidgetTest, SetShowCount)
{
    SlotWidget slot("test", 0, 0);

    EXPECT_TRUE(slot.showCount());

    slot.setShowCount(false);
    EXPECT_FALSE(slot.showCount());
}

TEST(SlotWidgetTest, SetHighlightColor)
{
    SlotWidget slot("test", 0, 0);

    u32 color = fromARGB(128, 255, 255, 255);
    slot.setHighlightColor(color);
    EXPECT_EQ(color, slot.highlightColor());
}

TEST(SlotWidgetTest, SetBackgroundTexture)
{
    SlotWidget slot("test", 0, 0);

    slot.setBackgroundTexture("textures/gui/slot.png");
    EXPECT_EQ("textures/gui/slot.png", slot.backgroundTexture());
}

// ==================== 回调测试 ====================

TEST(SlotWidgetTest, OnSlotClickCallback)
{
    SlotWidget slot("test", 0, 0);
    slot.setActive(true);
    slot.setVisible(true);
    slot.setSlotIndex(3);

    i32 clickedIndex = -1;
    i32 clickedButton = -1;
    bool shiftHeld = false;

    slot.setOnSlotClick([&](i32 index, i32 button, bool shift) {
        clickedIndex = index;
        clickedButton = button;
        shiftHeld = shift;
    });

    slot.onClick(5, 5, 0); // 左键点击

    EXPECT_EQ(3, clickedIndex);
    EXPECT_EQ(0, clickedButton);
    EXPECT_FALSE(shiftHeld);
}

TEST(SlotWidgetTest, OnSlotClickRightButton)
{
    SlotWidget slot("test", 0, 0);
    slot.setActive(true);
    slot.setVisible(true);
    slot.setSlotIndex(2);

    i32 clickedButton = -1;

    slot.setOnSlotClick([&](i32, i32 button, bool) { clickedButton = button; });

    slot.onClick(5, 5, 1); // 右键点击

    EXPECT_EQ(1, clickedButton);
}

TEST(SlotWidgetTest, OnSlotClickInactive)
{
    SlotWidget slot("test", 0, 0);
    slot.setActive(false); // 禁用
    slot.setSlotIndex(3);

    bool callbackCalled = false;
    slot.setOnSlotClick([&](i32, i32, bool) { callbackCalled = true; });

    // onClick 在 inactive 时返回 false，不调用回调
    bool result = slot.onClick(5, 5, 0);

    EXPECT_FALSE(result);         // inactive 时返回 false
    EXPECT_FALSE(callbackCalled); // 回调不被调用
}

TEST(SlotWidgetTest, OnReleaseCallback)
{
    SlotWidget slot("test", 0, 0);
    slot.setActive(true);
    slot.setVisible(true);

    bool releaseCalled = false;
    i32 releaseButton = -1;

    slot.setOnRelease([&](SlotWidget&, i32 button) {
        releaseCalled = true;
        releaseButton = button;
    });

    slot.onRelease(5, 5, 0);

    EXPECT_TRUE(releaseCalled);
    EXPECT_EQ(0, releaseButton);
}

// ==================== 状态测试 ====================

TEST(SlotWidgetTest, SetVisible)
{
    SlotWidget slot("test", 0, 0);

    EXPECT_TRUE(slot.isVisible());

    slot.setVisible(false);
    EXPECT_FALSE(slot.isVisible());
}

TEST(SlotWidgetTest, SetActive)
{
    SlotWidget slot("test", 0, 0);

    EXPECT_TRUE(slot.isActive());

    slot.setActive(false);
    EXPECT_FALSE(slot.isActive());
}

TEST(SlotWidgetTest, ContainsPoint)
{
    SlotWidget slot("test", 10, 10, 32);

    EXPECT_TRUE(slot.contains(20, 20));  // 内部
    EXPECT_TRUE(slot.contains(10, 10));  // 边界（左上角）
    EXPECT_TRUE(slot.contains(41, 41));  // 边界内（right-1, bottom-1）
    EXPECT_FALSE(slot.contains(5, 5));   // 外部（左上）
    EXPECT_FALSE(slot.contains(50, 50)); // 外部（右下）
    EXPECT_FALSE(slot.contains(42, 42)); // 边界外（right=42, bottom=42, 不包含）
}

// ==================== 可变物品引用测试 ====================

TEST(SlotWidgetTest, MutableItemReference)
{
    SlotWidget slot("test", 0, 0);

    ItemStack& item = slot.item();
    EXPECT_TRUE(item.isEmpty());

    // 可以通过可变引用修改
    // 注意：ItemStack 的实际修改方法取决于其实现
    (void)item; // 避免未使用警告
}
