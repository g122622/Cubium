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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND, any subsequent use,
 * modification, distribution, or any derivative works must retain the above
 * copyright notice and this permission notice.
 *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "common/TestWorldHelper.hpp"
#include "common/entity/inventory/PlayerEnderChestInventory.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerListener.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/nbt/Nbt.hpp"
#include <gtest/gtest.h>

using namespace mc;

namespace {

/// 测试用回调计数器，用于验证 onChanged 是否被调用
class ChangeCallbackCounter {
public:
    void operator()() { m_callCount++; }
    [[nodiscard]] i32 callCount() const { return m_callCount; }
    void reset() { m_callCount = 0; }

private:
    i32 m_callCount = 0;
};

} // namespace

class PlayerEnderChestInventoryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 确保物品注册表已初始化
        ItemRegistry::instance();
        diamondItem = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "diamond"));
        ironItem = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_ingot"));
    }

    PlayerEnderChestInventory inventory;
    ChangeCallbackCounter callback;
    Item* diamondItem = nullptr;
    Item* ironItem = nullptr;
};

// ========== 基本属性测试 ==========

TEST_F(PlayerEnderChestInventoryTest, ContainerSize)
{
    EXPECT_EQ(inventory.getContainerSize(), 27);
}

TEST_F(PlayerEnderChestInventoryTest, InitiallyEmpty)
{
    EXPECT_TRUE(inventory.isEmpty());
    for (i32 i = 0; i < 27; ++i) {
        EXPECT_TRUE(inventory.getItem(i).isEmpty());
    }
}

// ========== onChanged 回调测试 ==========

TEST_F(PlayerEnderChestInventoryTest, SetChangedCallsOnChangedCallback)
{
    bool called = false;
    inventory.setOnChanged([&called]() { called = true; });

    inventory.setChanged();
    EXPECT_TRUE(called);
}

TEST_F(PlayerEnderChestInventoryTest, SetChangedWithoutCallbackDoesNotCrash)
{
    // 未设置回调时，setChanged 不应崩溃
    EXPECT_NO_THROW(inventory.setChanged());
}

TEST_F(PlayerEnderChestInventoryTest, SetItemTriggersOnChanged)
{
    if (!diamondItem) {
        GTEST_SKIP() << "Diamond item not registered";
    }

    inventory.setOnChanged(std::ref(callback));
    ItemStack stack(*diamondItem, 1);
    inventory.setItem(0, stack);

    EXPECT_EQ(callback.callCount(), 1);
}

TEST_F(PlayerEnderChestInventoryTest, RemoveItemTriggersOnChanged)
{
    if (!diamondItem) {
        GTEST_SKIP() << "Diamond item not registered";
    }

    ItemStack stack(*diamondItem, 10);
    inventory.setItem(0, stack);

    inventory.setOnChanged(std::ref(callback));
    inventory.removeItem(0, 5);

    EXPECT_EQ(callback.callCount(), 1);
}

TEST_F(PlayerEnderChestInventoryTest, ClearTriggersOnChanged)
{
    if (!diamondItem) {
        GTEST_SKIP() << "Diamond item not registered";
    }

    inventory.setItem(0, ItemStack(*diamondItem, 1));

    inventory.setOnChanged(std::ref(callback));
    inventory.clear();

    EXPECT_EQ(callback.callCount(), 1);
}

TEST_F(PlayerEnderChestInventoryTest, RemoveItemNoUpdateDoesNotTriggerOnChanged)
{
    if (!diamondItem) {
        GTEST_SKIP() << "Diamond item not registered";
    }

    inventory.setItem(0, ItemStack(*diamondItem, 1));

    inventory.setOnChanged(std::ref(callback));
    inventory.removeItemNoUpdate(0);

    // removeItemNoUpdate 不应触发 setChanged
    EXPECT_EQ(callback.callCount(), 0);
}

TEST_F(PlayerEnderChestInventoryTest, MultipleChangesCallCallbackMultipleTimes)
{
    if (!diamondItem) {
        GTEST_SKIP() << "Diamond item not registered";
    }

    inventory.setOnChanged(std::ref(callback));

    inventory.setItem(0, ItemStack(*diamondItem, 1));
    inventory.setItem(1, ItemStack(*diamondItem, 2));
    inventory.setItem(2, ItemStack(*diamondItem, 3));

    EXPECT_EQ(callback.callCount(), 3);
}

TEST_F(PlayerEnderChestInventoryTest, OnChangedCallbackCanBeReplaced)
{
    if (!diamondItem) {
        GTEST_SKIP() << "Diamond item not registered";
    }

    i32 counter1 = 0;
    i32 counter2 = 0;

    inventory.setOnChanged([&counter1]() { counter1++; });
    inventory.setItem(0, ItemStack(*diamondItem, 1));
    EXPECT_EQ(counter1, 1);
    EXPECT_EQ(counter2, 0);

    inventory.setOnChanged([&counter2]() { counter2++; });
    inventory.setItem(1, ItemStack(*diamondItem, 1));
    EXPECT_EQ(counter1, 1); // 不再增加
    EXPECT_EQ(counter2, 1);
}

// ========== openInventory/closeInventory 测试 ==========

TEST_F(PlayerEnderChestInventoryTest, OpenInventoryWithNoActiveChestDoesNotCrash)
{
    // 没有 activeChest 时，openInventory → startOpen 不应崩溃
    // startOpen 在 m_activeChest == nullptr 时不调用 EnderChestEntity::openContainer
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    EXPECT_NO_THROW(inventory.openInventory(player));
}

TEST_F(PlayerEnderChestInventoryTest, CloseInventoryWithNoActiveChestDoesNotCrash)
{
    // 没有 activeChest 时，closeInventory → stopOpen 不应崩溃
    // stopOpen 在 m_activeChest == nullptr 时仅设置 m_activeChest = nullptr（无操作）
    Player player(1, "TestPlayer", mc::test::testEcsRegistry());
    EXPECT_NO_THROW(inventory.closeInventory(player));
}

// ========== NBT 序列化/反序列化测试 ==========

TEST_F(PlayerEnderChestInventoryTest, NbtRoundTripEmpty)
{
    nbt::tags::compound_tag tag;
    inventory.toNbt(tag);

    PlayerEnderChestInventory loaded;
    loaded.fromNbt(tag);

    EXPECT_TRUE(loaded.isEmpty());
    EXPECT_EQ(loaded.getContainerSize(), 27);
}

TEST_F(PlayerEnderChestInventoryTest, NbtRoundTripWithItems)
{
    if (!diamondItem || !ironItem) {
        GTEST_SKIP() << "Required items not registered";
    }

    inventory.setItem(0, ItemStack(*diamondItem, 32));
    inventory.setItem(13, ItemStack(*ironItem, 64));
    inventory.setItem(26, ItemStack(*diamondItem, 1));

    nbt::tags::compound_tag tag;
    inventory.toNbt(tag);

    PlayerEnderChestInventory loaded;
    loaded.fromNbt(tag);

    EXPECT_EQ(loaded.getItem(0).getCount(), 32);
    EXPECT_EQ(loaded.getItem(0).getItem(), diamondItem);
    EXPECT_EQ(loaded.getItem(13).getCount(), 64);
    EXPECT_EQ(loaded.getItem(13).getItem(), ironItem);
    EXPECT_EQ(loaded.getItem(26).getCount(), 1);
    EXPECT_EQ(loaded.getItem(26).getItem(), diamondItem);

    // 空槽位
    EXPECT_TRUE(loaded.getItem(1).isEmpty());
    EXPECT_TRUE(loaded.getItem(14).isEmpty());
}

// ========== setActiveChest / isActiveChestValid 测试 ==========

TEST_F(PlayerEnderChestInventoryTest, ActiveChestInitiallyNull)
{
    EXPECT_EQ(inventory.getActiveChest(), nullptr);
    EXPECT_FALSE(inventory.isActiveChestValid()); // null activeChest 时返回 false
}

// ========== SlotIndex 常量测试 ==========

TEST_F(PlayerEnderChestInventoryTest, SlotIndexStart)
{
    EXPECT_EQ(PlayerEnderChestInventory::SLOT_INDEX_START, 200);
}

TEST_F(PlayerEnderChestInventoryTest, EnderChestSize)
{
    EXPECT_EQ(PlayerEnderChestInventory::ENDER_CHEST_SIZE, 27);
}

// ========== ContainerListener 测试 ==========

/// 测试用 ContainerListener，记录 containerChanged 调用次数和引用
class TestContainerListener : public ContainerListener {
public:
    void containerChanged(IInventory& inventory) override
    {
        m_callCount++;
        m_lastInventory = &inventory;
    }

    [[nodiscard]] i32 callCount() const { return m_callCount; }
    [[nodiscard]] IInventory* lastInventory() const { return m_lastInventory; }
    void reset()
    {
        m_callCount = 0;
        m_lastInventory = nullptr;
    }

private:
    i32 m_callCount = 0;
    IInventory* m_lastInventory = nullptr;
};

TEST_F(PlayerEnderChestInventoryTest, AddListener_ReceivesNotifications)
{
    TestContainerListener listener;
    inventory.addListener(&listener);

    // setItem 应触发 containerChanged
    inventory.setItem(0, ItemStack());
    EXPECT_EQ(listener.callCount(), 1);
    EXPECT_EQ(listener.lastInventory(), &inventory);
}

TEST_F(PlayerEnderChestInventoryTest, RemoveListener_StopsNotifications)
{
    TestContainerListener listener;
    inventory.addListener(&listener);
    inventory.removeListener(&listener);

    inventory.setItem(0, ItemStack());
    EXPECT_EQ(listener.callCount(), 0);
}

TEST_F(PlayerEnderChestInventoryTest, MultipleListeners_AllReceiveNotifications)
{
    TestContainerListener listener1;
    TestContainerListener listener2;
    TestContainerListener listener3;

    inventory.addListener(&listener1);
    inventory.addListener(&listener2);
    inventory.addListener(&listener3);

    inventory.setItem(0, ItemStack());

    EXPECT_EQ(listener1.callCount(), 1);
    EXPECT_EQ(listener2.callCount(), 1);
    EXPECT_EQ(listener3.callCount(), 1);
}

TEST_F(PlayerEnderChestInventoryTest, AddListener_DuplicateIgnored)
{
    TestContainerListener listener;
    inventory.addListener(&listener);
    inventory.addListener(&listener); // 重复添加

    inventory.setItem(0, ItemStack());
    EXPECT_EQ(listener.callCount(), 1); // 只通知一次，不会重复调用
}

TEST_F(PlayerEnderChestInventoryTest, ListenerAndOnChangedCallback_BothCalled)
{
    ChangeCallbackCounter callback;
    TestContainerListener listener;

    inventory.setOnChanged([&callback]() { callback(); });
    inventory.addListener(&listener);

    inventory.setItem(0, ItemStack());

    // 两者都应该被调用
    EXPECT_EQ(callback.callCount(), 1);
    EXPECT_EQ(listener.callCount(), 1);
}

TEST_F(PlayerEnderChestInventoryTest, Clear_TriggersListeners)
{
    TestContainerListener listener;
    inventory.addListener(&listener);

    inventory.clear();
    EXPECT_EQ(listener.callCount(), 1);
}

TEST_F(PlayerEnderChestInventoryTest, RemoveItemNoUpdate_DoesNotTriggerListeners)
{
    TestContainerListener listener;
    inventory.addListener(&listener);

    inventory.removeItemNoUpdate(0);
    EXPECT_EQ(listener.callCount(), 0);
}
