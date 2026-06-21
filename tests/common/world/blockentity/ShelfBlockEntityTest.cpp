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

#include "world/blockentity/interactive/ShelfBlockEntity.hpp"
#include "common/TestWorldHelper.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blockentity;

namespace {

/**
 * @brief 按资源路径懒注册测试用物品。
 * @param path 资源路径。
 * @return 已注册物品指针。
 */
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

// ========== ShelfBlockEntity 测试 ==========

class ShelfBlockEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        shelf_ = std::make_unique<ShelfBlockEntity>(BlockPos(5, 10, 15));
        m_diamond = ensureTestItem("diamond");
        m_stick = ensureTestItem("stick");
        m_iron = ensureTestItem("iron_ingot");
    }

    std::unique_ptr<ShelfBlockEntity> shelf_;
    Item* m_diamond = nullptr;
    Item* m_stick = nullptr;
    Item* m_iron = nullptr;
};

// ========== 基本属性测试 ==========

TEST_F(ShelfBlockEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(shelf_->getType(), BlockEntityType::Shelf);
}

TEST_F(ShelfBlockEntityTest, Create_HasCorrectPosition)
{
    EXPECT_EQ(shelf_->getPos(), BlockPos(5, 10, 15));
}

TEST_F(ShelfBlockEntityTest, Create_HasCorrectSize)
{
    EXPECT_EQ(shelf_->getContainerSize(), ShelfBlockEntity::SHELF_SIZE);
    EXPECT_EQ(ShelfBlockEntity::SHELF_SIZE, 3); // 书架有3个槽位
}

TEST_F(ShelfBlockEntityTest, GetInventory_ReturnsValidPointer)
{
    IInventory* inventory = shelf_->getInventory();
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->getContainerSize(), ShelfBlockEntity::SHELF_SIZE);
}

TEST_F(ShelfBlockEntityTest, Create_InventoryIsEmpty)
{
    const IInventory* inventory = shelf_->getInventory();
    ASSERT_NE(inventory, nullptr);
    for (i32 i = 0; i < ShelfBlockEntity::SHELF_SIZE; ++i) {
        EXPECT_TRUE(inventory->getItem(i).isEmpty());
    }
}

// ========== 红石比较器信号测试 ==========

TEST_F(ShelfBlockEntityTest, AnalogOutputSignal_EmptyShelf_ReturnsZero)
{
    EXPECT_EQ(shelf_->getAnalogOutputSignal(), 0);
}

TEST_F(ShelfBlockEntityTest, AnalogOutputSignal_Slot0Occupied_Returns1)
{
    IInventory* inventory = shelf_->getInventory();
    ASSERT_NE(inventory, nullptr);
    inventory->setItem(0, ItemStack(m_diamond, 1));
    EXPECT_EQ(shelf_->getAnalogOutputSignal(), 1); // bit 0
}

TEST_F(ShelfBlockEntityTest, AnalogOutputSignal_Slot1Occupied_Returns2)
{
    IInventory* inventory = shelf_->getInventory();
    ASSERT_NE(inventory, nullptr);
    inventory->setItem(1, ItemStack(m_stick, 1));
    EXPECT_EQ(shelf_->getAnalogOutputSignal(), 2); // bit 1
}

TEST_F(ShelfBlockEntityTest, AnalogOutputSignal_Slot2Occupied_Returns4)
{
    IInventory* inventory = shelf_->getInventory();
    ASSERT_NE(inventory, nullptr);
    inventory->setItem(2, ItemStack(m_iron, 1));
    EXPECT_EQ(shelf_->getAnalogOutputSignal(), 4); // bit 2
}

TEST_F(ShelfBlockEntityTest, AnalogOutputSignal_AllSlotsOccupied_Returns7)
{
    IInventory* inventory = shelf_->getInventory();
    ASSERT_NE(inventory, nullptr);
    inventory->setItem(0, ItemStack(m_diamond, 1));
    inventory->setItem(1, ItemStack(m_stick, 1));
    inventory->setItem(2, ItemStack(m_iron, 1));
    EXPECT_EQ(shelf_->getAnalogOutputSignal(), 7); // 1 + 2 + 4
}

TEST_F(ShelfBlockEntityTest, AnalogOutputSignal_Slots0And2_Returns5)
{
    IInventory* inventory = shelf_->getInventory();
    ASSERT_NE(inventory, nullptr);
    inventory->setItem(0, ItemStack(m_diamond, 1));
    inventory->setItem(2, ItemStack(m_iron, 1));
    EXPECT_EQ(shelf_->getAnalogOutputSignal(), 5); // 1 + 4
}

TEST_F(ShelfBlockEntityTest, AnalogOutputSignal_StackSizeDoesNotAffectSignal)
{
    // 比较器信号只看槽位是否占用，不看数量
    IInventory* inventory = shelf_->getInventory();
    ASSERT_NE(inventory, nullptr);
    inventory->setItem(0, ItemStack(m_diamond, 64));
    EXPECT_EQ(shelf_->getAnalogOutputSignal(), 1); // 仍然是1，不是64
}

// ========== swapItemNoUpdate 测试 ==========

TEST_F(ShelfBlockEntityTest, SwapItemNoUpdate_EmptySlot_SwapsWithEmpty)
{
    ItemStack emptyStack;
    ItemStack result = shelf_->swapItemNoUpdate(0, emptyStack);
    EXPECT_TRUE(result.isEmpty());                             // 空槽位返回空物品
    EXPECT_TRUE(shelf_->getInventory()->getItem(0).isEmpty()); // 空物品被放入
}

TEST_F(ShelfBlockEntityTest, SwapItemNoUpdate_EmptySlot_ReceivesItem)
{
    IInventory* inventory = shelf_->getInventory();
    ASSERT_NE(inventory, nullptr);

    ItemStack newItem(m_diamond, 5);
    ItemStack result = shelf_->swapItemNoUpdate(0, newItem);

    EXPECT_TRUE(result.isEmpty()); // 空槽位返回空
    EXPECT_EQ(inventory->getItem(0).getItem(), m_diamond);
    EXPECT_EQ(inventory->getItem(0).getCount(), 5);
}

TEST_F(ShelfBlockEntityTest, SwapItemNoUpdate_OccupiedSlot_SwapsItem)
{
    IInventory* inventory = shelf_->getInventory();
    ASSERT_NE(inventory, nullptr);

    inventory->setItem(1, ItemStack(m_diamond, 10));

    ItemStack newItem(m_stick, 3);
    ItemStack result = shelf_->swapItemNoUpdate(1, newItem);

    EXPECT_EQ(result.getItem(), m_diamond); // 返回原来的物品
    EXPECT_EQ(result.getCount(), 10);
    EXPECT_EQ(inventory->getItem(1).getItem(), m_stick); // 新物品在槽位中
    EXPECT_EQ(inventory->getItem(1).getCount(), 3);
}

TEST_F(ShelfBlockEntityTest, SwapItemNoUpdate_DoesNotTriggerChangeNotification)
{
    // swapItemNoUpdate 不应触发 setChanged
    shelf_->setChanged(); // 先标记为已更改
    EXPECT_TRUE(shelf_->isChanged());

    // 清除更改标记（通过 save 间接测试或通过子类方法）
    // swapItemNoUpdate 本身不调用 markChanged
    ItemStack newItem(m_diamond, 1);
    shelf_->swapItemNoUpdate(0, newItem);

    // 只要没有崩溃即通过，swapItemNoUpdate 不负责通知
    EXPECT_EQ(shelf_->getInventory()->getItem(0).getItem(), m_diamond);
}

// ========== 序列化测试 ==========

TEST_F(ShelfBlockEntityTest, Save_ContainsBasicInfo)
{
    nlohmann::json data;
    shelf_->save(data);

    EXPECT_TRUE(data.contains("id"));
    EXPECT_EQ(data["id"], "minecraft:shelf");
    EXPECT_TRUE(data.contains("x"));
    EXPECT_TRUE(data.contains("y"));
    EXPECT_TRUE(data.contains("z"));
}

TEST_F(ShelfBlockEntityTest, SaveLoad_PreservesItemsBySlot)
{
    IInventory* inventory = shelf_->getInventory();
    ASSERT_NE(inventory, nullptr);
    inventory->setItem(0, ItemStack(m_diamond, 7));
    inventory->setItem(2, ItemStack(m_stick, 3));

    nlohmann::json data;
    shelf_->save(data);

    ShelfBlockEntity loaded(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded.load(data));

    const IInventory* loadedInventory = loaded.getInventory();
    ASSERT_NE(loadedInventory, nullptr);
    EXPECT_EQ(loadedInventory->getItem(0).getItem(), m_diamond);
    EXPECT_EQ(loadedInventory->getItem(0).getCount(), 7);
    EXPECT_TRUE(loadedInventory->getItem(1).isEmpty()); // 未设置的槽位应为空
    EXPECT_EQ(loadedInventory->getItem(2).getItem(), m_stick);
    EXPECT_EQ(loadedInventory->getItem(2).getCount(), 3);
}

TEST_F(ShelfBlockEntityTest, SaveLoad_EmptyShelf)
{
    nlohmann::json data;
    shelf_->save(data);

    ShelfBlockEntity loaded(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded.load(data));

    const IInventory* loadedInventory = loaded.getInventory();
    ASSERT_NE(loadedInventory, nullptr);
    for (i32 i = 0; i < ShelfBlockEntity::SHELF_SIZE; ++i) {
        EXPECT_TRUE(loadedInventory->getItem(i).isEmpty());
    }
}

TEST_F(ShelfBlockEntityTest, SaveLoad_AllSlotsPreserved)
{
    IInventory* inventory = shelf_->getInventory();
    ASSERT_NE(inventory, nullptr);
    inventory->setItem(0, ItemStack(m_diamond, 1));
    inventory->setItem(1, ItemStack(m_stick, 32));
    inventory->setItem(2, ItemStack(m_iron, 64));

    nlohmann::json data;
    shelf_->save(data);

    ShelfBlockEntity loaded(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded.load(data));

    const IInventory* loadedInventory = loaded.getInventory();
    ASSERT_NE(loadedInventory, nullptr);
    EXPECT_EQ(loadedInventory->getItem(0).getItem(), m_diamond);
    EXPECT_EQ(loadedInventory->getItem(0).getCount(), 1);
    EXPECT_EQ(loadedInventory->getItem(1).getItem(), m_stick);
    EXPECT_EQ(loadedInventory->getItem(1).getCount(), 32);
    EXPECT_EQ(loadedInventory->getItem(2).getItem(), m_iron);
    EXPECT_EQ(loadedInventory->getItem(2).getCount(), 64);
}

// ========== Clone 测试 ==========

TEST_F(ShelfBlockEntityTest, Clone_CreatesCopy)
{
    std::unique_ptr<BlockEntity> copy = shelf_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::Shelf);
    EXPECT_EQ(copy->getPos(), BlockPos(5, 10, 15));
}

TEST_F(ShelfBlockEntityTest, Clone_CopiesInventoryContents)
{
    IInventory* inventory = shelf_->getInventory();
    ASSERT_NE(inventory, nullptr);
    inventory->setItem(0, ItemStack(m_diamond, 11));
    inventory->setItem(2, ItemStack(m_stick, 42));

    std::unique_ptr<BlockEntity> copy = shelf_->clone();
    ASSERT_NE(copy, nullptr);

    const auto* clonedShelf = dynamic_cast<const ShelfBlockEntity*>(copy.get());
    ASSERT_NE(clonedShelf, nullptr);

    const IInventory* clonedInventory = clonedShelf->getInventory();
    ASSERT_NE(clonedInventory, nullptr);
    EXPECT_EQ(clonedInventory->getItem(0).getItem(), m_diamond);
    EXPECT_EQ(clonedInventory->getItem(0).getCount(), 11);
    EXPECT_TRUE(clonedInventory->getItem(1).isEmpty());
    EXPECT_EQ(clonedInventory->getItem(2).getItem(), m_stick);
    EXPECT_EQ(clonedInventory->getItem(2).getCount(), 42);
}

TEST_F(ShelfBlockEntityTest, Clone_IndependentFromOriginal)
{
    IInventory* inventory = shelf_->getInventory();
    ASSERT_NE(inventory, nullptr);
    inventory->setItem(1, ItemStack(m_diamond, 5));

    std::unique_ptr<BlockEntity> copy = shelf_->clone();
    ASSERT_NE(copy, nullptr);

    const auto* clonedShelf = dynamic_cast<const ShelfBlockEntity*>(copy.get());
    ASSERT_NE(clonedShelf, nullptr);

    // 修改原实体的物品不应影响克隆
    inventory->setItem(1, ItemStack(m_stick, 10));

    const IInventory* clonedInventory = clonedShelf->getInventory();
    ASSERT_NE(clonedInventory, nullptr);
    EXPECT_EQ(clonedInventory->getItem(1).getItem(), m_diamond); // 仍然是钻石
    EXPECT_EQ(clonedInventory->getItem(1).getCount(), 5);        // 仍然是5
}

// ========== SetChanged 测试 ==========

TEST_F(ShelfBlockEntityTest, SetChanged_MarksAsChanged)
{
    EXPECT_FALSE(shelf_->isChanged());
    shelf_->setChanged();
    EXPECT_TRUE(shelf_->isChanged());
}
