#include <gtest/gtest.h>
#include "entity/inventory/CraftingInventory.hpp"
#include "item/core/ItemRegistry.hpp"

using namespace mc;

class CraftingInventoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 获取测试用的物品
        stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
        dirt = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "dirt"));
    }

    Item* stone = nullptr;
    Item* dirt = nullptr;
};

// ========== 构造函数测试 ==========

TEST_F(CraftingInventoryTest, Create_2x2) {
    CraftingInventory inv(2, 2);

    EXPECT_EQ(inv.getWidth(), 2);
    EXPECT_EQ(inv.getHeight(), 2);
    EXPECT_EQ(inv.getContainerSize(), 4);
    EXPECT_TRUE(inv.isEmpty());
}

TEST_F(CraftingInventoryTest, Create_3x3) {
    CraftingInventory inv(3, 3);

    EXPECT_EQ(inv.getWidth(), 3);
    EXPECT_EQ(inv.getHeight(), 3);
    EXPECT_EQ(inv.getContainerSize(), 9);
    EXPECT_TRUE(inv.isEmpty());
}

TEST_F(CraftingInventoryTest, Create_1x1) {
    CraftingInventory inv(1, 1);

    EXPECT_EQ(inv.getWidth(), 1);
    EXPECT_EQ(inv.getHeight(), 1);
    EXPECT_EQ(inv.getContainerSize(), 1);
    EXPECT_TRUE(inv.isEmpty());
}

// ========== 槽位索引测试 ==========

TEST_F(CraftingInventoryTest, PosToSlot_TopLeft) {
    CraftingInventory inv(3, 3);

    EXPECT_EQ(inv.posToSlot(0, 0), 0);
}

TEST_F(CraftingInventoryTest, PosToSlot_TopRight) {
    CraftingInventory inv(3, 3);

    EXPECT_EQ(inv.posToSlot(2, 0), 2);
}

TEST_F(CraftingInventoryTest, PosToSlot_BottomLeft) {
    CraftingInventory inv(3, 3);

    EXPECT_EQ(inv.posToSlot(0, 2), 6);
}

TEST_F(CraftingInventoryTest, PosToSlot_BottomRight) {
    CraftingInventory inv(3, 3);

    EXPECT_EQ(inv.posToSlot(2, 2), 8);
}

TEST_F(CraftingInventoryTest, PosToSlot_OutOfBounds_Negative) {
    CraftingInventory inv(3, 3);

    EXPECT_EQ(inv.posToSlot(-1, 0), -1);
    EXPECT_EQ(inv.posToSlot(0, -1), -1);
    EXPECT_EQ(inv.posToSlot(-1, -1), -1);
}

TEST_F(CraftingInventoryTest, PosToSlot_OutOfBounds_TooLarge) {
    CraftingInventory inv(3, 3);

    EXPECT_EQ(inv.posToSlot(3, 0), -1);
    EXPECT_EQ(inv.posToSlot(0, 3), -1);
    EXPECT_EQ(inv.posToSlot(3, 3), -1);
}

TEST_F(CraftingInventoryTest, SlotToPos_Valid) {
    CraftingInventory inv(3, 3);

    i32 x, y;

    EXPECT_TRUE(inv.slotToPos(0, x, y));
    EXPECT_EQ(x, 0);
    EXPECT_EQ(y, 0);

    EXPECT_TRUE(inv.slotToPos(4, x, y));
    EXPECT_EQ(x, 1);
    EXPECT_EQ(y, 1);

    EXPECT_TRUE(inv.slotToPos(8, x, y));
    EXPECT_EQ(x, 2);
    EXPECT_EQ(y, 2);
}

TEST_F(CraftingInventoryTest, SlotToPos_Invalid) {
    CraftingInventory inv(3, 3);

    i32 x, y;

    EXPECT_FALSE(inv.slotToPos(-1, x, y));
    EXPECT_FALSE(inv.slotToPos(9, x, y));
}

// ========== 物品操作测试 ==========

TEST_F(CraftingInventoryTest, SetGetItem_BySlot) {
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    CraftingInventory inv(3, 3);

    ItemStack stack(*stone, 32);
    inv.setItem(4, stack);

    EXPECT_TRUE(inv.getItem(4).isSameItem(stack));
    EXPECT_EQ(inv.getItem(4).getCount(), 32);
    EXPECT_FALSE(inv.isEmpty());
}

TEST_F(CraftingInventoryTest, SetGetItem_ByPosition) {
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    CraftingInventory inv(3, 3);

    ItemStack stack(*stone, 16);
    inv.setItemAt(1, 1, stack);

    // 位置(1,1)应该是槽位4
    EXPECT_EQ(inv.posToSlot(1, 1), 4);
    EXPECT_TRUE(inv.getItemAt(1, 1).isSameItem(stack));
    EXPECT_EQ(inv.getItemAt(1, 1).getCount(), 16);
}

TEST_F(CraftingInventoryTest, GetItem_EmptySlot) {
    CraftingInventory inv(3, 3);

    EXPECT_TRUE(inv.getItem(0).isEmpty());
    EXPECT_TRUE(inv.getItemAt(0, 0).isEmpty());
}

TEST_F(CraftingInventoryTest, RemoveItem_Partial) {
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    CraftingInventory inv(3, 3);

    inv.setItem(0, ItemStack(*stone, 64));
    ItemStack removed = inv.removeItem(0, 16);

    EXPECT_EQ(removed.getCount(), 16);
    EXPECT_EQ(inv.getItem(0).getCount(), 48);
}

TEST_F(CraftingInventoryTest, RemoveItem_All) {
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    CraftingInventory inv(3, 3);

    inv.setItem(0, ItemStack(*stone, 32));
    ItemStack removed = inv.removeItem(0, 32);

    EXPECT_EQ(removed.getCount(), 32);
    EXPECT_TRUE(inv.getItem(0).isEmpty());
}

TEST_F(CraftingInventoryTest, RemoveItemNoUpdate) {
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    CraftingInventory inv(3, 3);

    inv.setItem(0, ItemStack(*stone, 64));
    ItemStack removed = inv.removeItemNoUpdate(0);

    EXPECT_EQ(removed.getCount(), 64);
    EXPECT_TRUE(inv.getItem(0).isEmpty());
}

TEST_F(CraftingInventoryTest, Clear) {
    if (!stone || !dirt) {
        GTEST_SKIP() << "Required items not registered";
    }

    CraftingInventory inv(3, 3);

    inv.setItem(0, ItemStack(*stone, 10));
    inv.setItem(4, ItemStack(*dirt, 20));
    inv.setItem(8, ItemStack(*stone, 30));

    inv.clear();

    EXPECT_TRUE(inv.isEmpty());
    EXPECT_TRUE(inv.getItem(0).isEmpty());
    EXPECT_TRUE(inv.getItem(4).isEmpty());
    EXPECT_TRUE(inv.getItem(8).isEmpty());
}

// ========== 回调测试 ==========

TEST_F(CraftingInventoryTest, ContentChangedCallback) {
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    CraftingInventory inv(3, 3);

    bool callbackCalled = false;
    inv.setContentChangedCallback([&callbackCalled]() {
        callbackCalled = true;
    });

    inv.setItem(0, ItemStack(*stone, 1));

    EXPECT_TRUE(callbackCalled);
}

// ========== 边界计算测试 ==========

TEST_F(CraftingInventoryTest, ContentBounds_Empty) {
    CraftingInventory inv(3, 3);

    i32 minX, minY, maxX, maxY;
    bool hasContent = inv.getContentBounds(minX, minY, maxX, maxY);

    EXPECT_FALSE(hasContent);
}

TEST_F(CraftingInventoryTest, ContentBounds_SingleItem) {
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    CraftingInventory inv(3, 3);

    inv.setItemAt(1, 1, ItemStack(*stone, 1));

    i32 minX, minY, maxX, maxY;
    bool hasContent = inv.getContentBounds(minX, minY, maxX, maxY);

    EXPECT_TRUE(hasContent);
    EXPECT_EQ(minX, 1);
    EXPECT_EQ(minY, 1);
    EXPECT_EQ(maxX, 1);
    EXPECT_EQ(maxY, 1);
}

TEST_F(CraftingInventoryTest, ContentBounds_MultipleItems) {
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    CraftingInventory inv(3, 3);

    // 放置物品形成2x2的网格
    inv.setItemAt(0, 0, ItemStack(*stone, 1)); // 左上
    inv.setItemAt(2, 2, ItemStack(*stone, 1)); // 右下

    i32 minX, minY, maxX, maxY;
    bool hasContent = inv.getContentBounds(minX, minY, maxX, maxY);

    EXPECT_TRUE(hasContent);
    EXPECT_EQ(minX, 0);
    EXPECT_EQ(minY, 0);
    EXPECT_EQ(maxX, 2);
    EXPECT_EQ(maxY, 2);
}

TEST_F(CraftingInventoryTest, IsAllEmpty) {
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    CraftingInventory inv(3, 3);

    EXPECT_TRUE(inv.isAllEmpty());

    inv.setItemAt(0, 0, ItemStack(*stone, 1));

    EXPECT_FALSE(inv.isAllEmpty());

    inv.removeItem(0, 1);

    EXPECT_TRUE(inv.isAllEmpty());
}

// ========== SetItems测试 ==========

TEST_F(CraftingInventoryTest, SetItems_CorrectSize) {
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    CraftingInventory inv(2, 2);

    std::vector<ItemStack> items(4);
    items[0] = ItemStack(*stone, 1);
    items[3] = ItemStack(*stone, 2);

    inv.setItems(items);

    EXPECT_TRUE(inv.getItemAt(0, 0).isSameItem(items[0]));
    EXPECT_TRUE(inv.getItemAt(1, 1).isSameItem(items[3]));
}

TEST_F(CraftingInventoryTest, SetItems_WrongSize) {
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    CraftingInventory inv(2, 2);

    // 先设置一个物品
    inv.setItem(0, ItemStack(*stone, 10));

    // 尝试设置错误大小的数组
    std::vector<ItemStack> items(5); // 错误大小

    inv.setItems(items);

    // 应该保持原状态
    EXPECT_EQ(inv.getItem(0).getCount(), 10);
}

// ========== CraftResultInventory测试 ==========

class CraftResultInventoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        stone = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stone"));
    }

    Item* stone = nullptr;
};

TEST_F(CraftResultInventoryTest, Create) {
    CraftResultInventory inv;

    EXPECT_EQ(inv.getContainerSize(), 1);
    EXPECT_TRUE(inv.isEmpty());
    EXPECT_FALSE(inv.hasResult());
}

TEST_F(CraftResultInventoryTest, SetResultItem) {
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    CraftResultInventory inv;

    ItemStack result(*stone, 4);
    inv.setResultItem(result);

    EXPECT_TRUE(inv.hasResult());
    EXPECT_TRUE(inv.getResultItem().isSameItem(result));
    EXPECT_EQ(inv.getResultItem().getCount(), 4);
}

TEST_F(CraftResultInventoryTest, GetItem_ValidSlot) {
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    CraftResultInventory inv;

    ItemStack result(*stone, 1);
    inv.setItem(0, result);

    EXPECT_TRUE(inv.getItem(0).isSameItem(result));
}

TEST_F(CraftResultInventoryTest, GetItem_InvalidSlot) {
    CraftResultInventory inv;

    EXPECT_TRUE(inv.getItem(-1).isEmpty());
    EXPECT_TRUE(inv.getItem(1).isEmpty());
    EXPECT_TRUE(inv.getItem(100).isEmpty());
}

TEST_F(CraftResultInventoryTest, RemoveItem) {
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    CraftResultInventory inv;

    inv.setResultItem(ItemStack(*stone, 1));
    ItemStack removed = inv.removeItem(0, 1);

    EXPECT_EQ(removed.getCount(), 1);
    EXPECT_FALSE(inv.hasResult());
}

TEST_F(CraftResultInventoryTest, Clear) {
    if (!stone) {
        GTEST_SKIP() << "Stone item not registered";
    }

    CraftResultInventory inv;

    inv.setResultItem(ItemStack(*stone, 1));
    inv.clear();

    EXPECT_TRUE(inv.isEmpty());
    EXPECT_FALSE(inv.hasResult());
}
