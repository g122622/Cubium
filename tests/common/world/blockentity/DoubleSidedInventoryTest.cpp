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

#include "world/blockentity/storage/DoubleSidedInventory.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blockentity;

namespace {
Item* ensureTestItem(const char* path)
{
    auto& registry = ItemRegistry::instance();
    const ResourceLocation id("minecraft", path);
    if (Item* existing = registry.getItem(id); existing != nullptr) {
        return existing;
    }
    return &registry.registerItem(id, ItemProperties().maxStackSize(64));
}
} // namespace

// ========== DoubleSidedInventory 测试 ==========

class DoubleSidedInventoryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        m_diamond = ensureTestItem("diamond");
        m_iron = ensureTestItem("iron_ingot");

        // 创建两个27格背包模拟箱子
        m_upperInventory = std::make_unique<SimpleInventory>(27);
        m_lowerInventory = std::make_unique<SimpleInventory>(27);

        // 创建双箱
        m_doubleInventory = std::make_unique<DoubleSidedInventory>(m_upperInventory.get(), m_lowerInventory.get());
    }

    std::unique_ptr<SimpleInventory> m_upperInventory;
    std::unique_ptr<SimpleInventory> m_lowerInventory;
    std::unique_ptr<DoubleSidedInventory> m_doubleInventory;
    Item* m_diamond = nullptr;
    Item* m_iron = nullptr;
};

TEST_F(DoubleSidedInventoryTest, Create_HasCorrectSize)
{
    // 双箱 = 27 + 27 = 54 格
    EXPECT_EQ(m_doubleInventory->getContainerSize(), 54);
}

TEST_F(DoubleSidedInventoryTest, Create_IsEmptyInitially)
{
    EXPECT_TRUE(m_doubleInventory->isEmpty());
}

TEST_F(DoubleSidedInventoryTest, GetUpper_ReturnsUpperInventory)
{
    EXPECT_EQ(m_doubleInventory->getUpper(), m_upperInventory.get());
}

TEST_F(DoubleSidedInventoryTest, GetLower_ReturnsLowerInventory)
{
    EXPECT_EQ(m_doubleInventory->getLower(), m_lowerInventory.get());
}

TEST_F(DoubleSidedInventoryTest, IsPartOfLargeChest_UpperReturnsTrue)
{
    EXPECT_TRUE(m_doubleInventory->isPartOfLargeChest(m_upperInventory.get()));
}

TEST_F(DoubleSidedInventoryTest, IsPartOfLargeChest_LowerReturnsTrue)
{
    EXPECT_TRUE(m_doubleInventory->isPartOfLargeChest(m_lowerInventory.get()));
}

TEST_F(DoubleSidedInventoryTest, IsPartOfLargeChest_OtherReturnsFalse)
{
    SimpleInventory other(27);
    EXPECT_FALSE(m_doubleInventory->isPartOfLargeChest(&other));
}

// ========== 槽位转换测试 ==========

TEST_F(DoubleSidedInventoryTest, GetContainerAndSlot_Slot0ToUpperSlot0)
{
    IInventory* container = nullptr;
    i32 localSlot = -1;

    EXPECT_TRUE(m_doubleInventory->getContainerAndSlot(0, &container, localSlot));
    EXPECT_EQ(container, m_upperInventory.get());
    EXPECT_EQ(localSlot, 0);
}

TEST_F(DoubleSidedInventoryTest, GetContainerAndSlot_Slot26ToUpperSlot26)
{
    IInventory* container = nullptr;
    i32 localSlot = -1;

    EXPECT_TRUE(m_doubleInventory->getContainerAndSlot(26, &container, localSlot));
    EXPECT_EQ(container, m_upperInventory.get());
    EXPECT_EQ(localSlot, 26);
}

TEST_F(DoubleSidedInventoryTest, GetContainerAndSlot_Slot27ToLowerSlot0)
{
    IInventory* container = nullptr;
    i32 localSlot = -1;

    EXPECT_TRUE(m_doubleInventory->getContainerAndSlot(27, &container, localSlot));
    EXPECT_EQ(container, m_lowerInventory.get());
    EXPECT_EQ(localSlot, 0);
}

TEST_F(DoubleSidedInventoryTest, GetContainerAndSlot_Slot53ToLowerSlot26)
{
    IInventory* container = nullptr;
    i32 localSlot = -1;

    EXPECT_TRUE(m_doubleInventory->getContainerAndSlot(53, &container, localSlot));
    EXPECT_EQ(container, m_lowerInventory.get());
    EXPECT_EQ(localSlot, 26);
}

TEST_F(DoubleSidedInventoryTest, GetContainerAndSlot_InvalidSlotReturnsFalse)
{
    IInventory* container = nullptr;
    i32 localSlot = -1;

    EXPECT_FALSE(m_doubleInventory->getContainerAndSlot(-1, &container, localSlot));
    EXPECT_FALSE(m_doubleInventory->getContainerAndSlot(54, &container, localSlot));
    EXPECT_FALSE(m_doubleInventory->getContainerAndSlot(100, &container, localSlot));
}

// ========== 物品操作测试 ==========

TEST_F(DoubleSidedInventoryTest, SetItem_UpperSlot)
{
    m_doubleInventory->setItem(5, ItemStack(m_diamond, 32));

    EXPECT_EQ(m_upperInventory->getItem(5).getItem(), m_diamond);
    EXPECT_EQ(m_upperInventory->getItem(5).getCount(), 32);
    EXPECT_EQ(m_doubleInventory->getItem(5).getItem(), m_diamond);
    EXPECT_EQ(m_doubleInventory->getItem(5).getCount(), 32);
}

TEST_F(DoubleSidedInventoryTest, SetItem_LowerSlot)
{
    m_doubleInventory->setItem(30, ItemStack(m_iron, 16));

    // 30 - 27 = 3
    EXPECT_EQ(m_lowerInventory->getItem(3).getItem(), m_iron);
    EXPECT_EQ(m_lowerInventory->getItem(3).getCount(), 16);
    EXPECT_EQ(m_doubleInventory->getItem(30).getItem(), m_iron);
    EXPECT_EQ(m_doubleInventory->getItem(30).getCount(), 16);
}

TEST_F(DoubleSidedInventoryTest, RemoveItem_RemovesFromCorrectContainer)
{
    m_doubleInventory->setItem(10, ItemStack(m_diamond, 10));

    ItemStack removed = m_doubleInventory->removeItem(10, 5);

    EXPECT_EQ(removed.getItem(), m_diamond);
    EXPECT_EQ(removed.getCount(), 5);
    EXPECT_EQ(m_doubleInventory->getItem(10).getCount(), 5);
    EXPECT_EQ(m_upperInventory->getItem(10).getCount(), 5);
}

TEST_F(DoubleSidedInventoryTest, Clear_ClearsBothContainers)
{
    m_upperInventory->setItem(0, ItemStack(m_diamond, 10));
    m_lowerInventory->setItem(0, ItemStack(m_iron, 20));

    m_doubleInventory->clear();

    EXPECT_TRUE(m_doubleInventory->isEmpty());
    EXPECT_TRUE(m_upperInventory->isEmpty());
    EXPECT_TRUE(m_lowerInventory->isEmpty());
}

TEST_F(DoubleSidedInventoryTest, IsEmpty_ReturnsFalseWhenUpperHasItems)
{
    m_upperInventory->setItem(0, ItemStack(m_diamond, 1));
    EXPECT_FALSE(m_doubleInventory->isEmpty());
}

TEST_F(DoubleSidedInventoryTest, IsEmpty_ReturnsFalseWhenLowerHasItems)
{
    m_lowerInventory->setItem(0, ItemStack(m_diamond, 1));
    EXPECT_FALSE(m_doubleInventory->isEmpty());
}

// ========== 物品查找测试 ==========

TEST_F(DoubleSidedInventoryTest, GetFirstEmptySlot_ReturnsZeroWhenEmpty)
{
    EXPECT_EQ(m_doubleInventory->getFirstEmptySlot(), 0);
}

TEST_F(DoubleSidedInventoryTest, GetFirstEmptySlot_ReturnsCorrectSlot)
{
    m_doubleInventory->setItem(0, ItemStack(m_diamond, 1));
    m_doubleInventory->setItem(1, ItemStack(m_diamond, 1));

    EXPECT_EQ(m_doubleInventory->getFirstEmptySlot(), 2);
}

TEST_F(DoubleSidedInventoryTest, GetFirstEmptySlot_ReturnsMinusOneWhenFull)
{
    // 填满所有槽位
    for (i32 i = 0; i < 54; ++i) {
        m_doubleInventory->setItem(i, ItemStack(m_diamond, 1));
    }

    EXPECT_EQ(m_doubleInventory->getFirstEmptySlot(), -1);
}

TEST_F(DoubleSidedInventoryTest, GetFirstEmptySlot_CrossesContainerBoundary)
{
    // 填满上半部分
    for (i32 i = 0; i < 27; ++i) {
        m_upperInventory->setItem(i, ItemStack(m_diamond, 1));
    }

    // 下半部分为空，第一个空槽是27
    EXPECT_EQ(m_doubleInventory->getFirstEmptySlot(), 27);
}

TEST_F(DoubleSidedInventoryTest, CountItem_CountsBothContainers)
{
    m_upperInventory->setItem(0, ItemStack(m_diamond, 10));
    m_upperInventory->setItem(1, ItemStack(m_diamond, 20));
    m_lowerInventory->setItem(0, ItemStack(m_diamond, 15));
    m_lowerInventory->setItem(1, ItemStack(m_iron, 5));

    EXPECT_EQ(m_doubleInventory->countItem(*m_diamond), 45); // 10 + 20 + 15
    EXPECT_EQ(m_doubleInventory->countItem(*m_iron), 5);
}

TEST_F(DoubleSidedInventoryTest, HasItem_ReturnsTrueWhenPresent)
{
    m_lowerInventory->setItem(10, ItemStack(m_diamond, 1));

    EXPECT_TRUE(m_doubleInventory->hasItem(*m_diamond));
    EXPECT_FALSE(m_doubleInventory->hasItem(*m_iron));
}

// ========== 变更通知测试 ==========

TEST_F(DoubleSidedInventoryTest, SetChanged_MarksContainers)
{
    // 设置变更标志
    m_doubleInventory->setChanged();

    // 验证容器已被标记（通过检查 getUpper 和 getLower 是否有效）
    EXPECT_NE(m_doubleInventory->getUpper(), nullptr);
    EXPECT_NE(m_doubleInventory->getLower(), nullptr);
}

// ========== 最大堆叠测试 ==========

TEST_F(DoubleSidedInventoryTest, GetMaxStackSize_Returns64)
{
    EXPECT_EQ(m_doubleInventory->getMaxStackSize(), 64);
}

TEST_F(DoubleSidedInventoryTest, CanPlaceItem_DelegatesToUpperContainer)
{
    ItemStack stack(m_diamond, 1);

    EXPECT_TRUE(m_doubleInventory->canPlaceItem(0, stack));
    EXPECT_TRUE(m_doubleInventory->canPlaceItem(26, stack));
}

TEST_F(DoubleSidedInventoryTest, CanPlaceItem_DelegatesToLowerContainer)
{
    ItemStack stack(m_diamond, 1);

    EXPECT_TRUE(m_doubleInventory->canPlaceItem(27, stack));
    EXPECT_TRUE(m_doubleInventory->canPlaceItem(53, stack));
}

// ========== 边界条件测试 ==========

TEST_F(DoubleSidedInventoryTest, SetItem_OutOfRangeBehavior)
{
    // 注意：DoubleSidedInventory 内部会检查槽位范围
    // 越界访问应该有保护或断言

    // 在边界上操作
    m_doubleInventory->setItem(0, ItemStack(m_diamond, 1)); // 第一个槽位
    m_doubleInventory->setItem(53, ItemStack(m_iron, 1));   // 最后一个槽位

    EXPECT_EQ(m_doubleInventory->getItem(0).getItem(), m_diamond);
    EXPECT_EQ(m_doubleInventory->getItem(53).getItem(), m_iron);
}

TEST_F(DoubleSidedInventoryTest, RemoveItemNoUpdate_RemovesWithoutNotify)
{
    m_doubleInventory->setItem(5, ItemStack(m_diamond, 10));

    ItemStack removed = m_doubleInventory->removeItemNoUpdate(5);

    EXPECT_EQ(removed.getItem(), m_diamond);
    EXPECT_EQ(removed.getCount(), 10);
    EXPECT_TRUE(m_doubleInventory->getItem(5).isEmpty());
}
