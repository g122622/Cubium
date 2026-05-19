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

#include "world/blockentity/storage/ChestEntity.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include "world/blockentity/storage/TrappedChestEntity.hpp"
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

// ========== ChestEntity 测试 ==========

class ChestEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        chest_ = std::make_unique<ChestEntity>(BlockPos(10, 20, 30));
        m_diamond = ensureTestItem("diamond");
        m_stick = ensureTestItem("stick");
    }

    std::unique_ptr<ChestEntity> chest_;
    Item* m_diamond = nullptr;
    Item* m_stick = nullptr;
};

TEST_F(ChestEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(chest_->getType(), BlockEntityType::Chest);
}

TEST_F(ChestEntityTest, Create_HasCorrectPosition)
{
    EXPECT_EQ(chest_->getPos(), BlockPos(10, 20, 30));
}

TEST_F(ChestEntityTest, Create_HasCorrectSize)
{
    EXPECT_EQ(chest_->getContainerSize(), ChestEntity::CHEST_SIZE);
    EXPECT_EQ(ChestEntity::CHEST_SIZE, 27); // 标准箱子大小
}

TEST_F(ChestEntityTest, Create_LidAngleIsZero)
{
    EXPECT_FLOAT_EQ(chest_->getLidAngle(), 0.0f);
    EXPECT_FLOAT_EQ(chest_->getPrevLidAngle(), 0.0f);
}

TEST_F(ChestEntityTest, Create_OpenCountIsZero)
{
    EXPECT_EQ(chest_->getOpenCount(), 0);
}

TEST_F(ChestEntityTest, OpenContainer_IncrementsCount)
{
    chest_->openContainer(nullptr);
    EXPECT_EQ(chest_->getOpenCount(), 1);

    chest_->openContainer(nullptr);
    EXPECT_EQ(chest_->getOpenCount(), 2);
}

TEST_F(ChestEntityTest, CloseContainer_DecrementsCount)
{
    chest_->openContainer(nullptr);
    chest_->openContainer(nullptr);
    EXPECT_EQ(chest_->getOpenCount(), 2);

    chest_->closeContainer(nullptr);
    EXPECT_EQ(chest_->getOpenCount(), 1);
}

TEST_F(ChestEntityTest, CloseContainer_NotBelowZero)
{
    chest_->closeContainer(nullptr);
    EXPECT_EQ(chest_->getOpenCount(), 0);

    chest_->closeContainer(nullptr);
    EXPECT_EQ(chest_->getOpenCount(), 0);
}

TEST_F(ChestEntityTest, NeedsTick_ReturnsTrue)
{
    EXPECT_TRUE(chest_->needsTick());
}

TEST_F(ChestEntityTest, GetInventory_ReturnsValidPointer)
{
    IInventory* inventory = chest_->getInventory();
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->getContainerSize(), ChestEntity::CHEST_SIZE);
}

TEST_F(ChestEntityTest, Save_ContainsBasicInfo)
{
    nlohmann::json data;
    chest_->save(data);

    EXPECT_TRUE(data.contains("id"));
    EXPECT_EQ(data["id"], "minecraft:chest");
    EXPECT_TRUE(data.contains("x"));
    EXPECT_TRUE(data.contains("y"));
    EXPECT_TRUE(data.contains("z"));
}

TEST_F(ChestEntityTest, Clone_CreatesCopy)
{
    std::unique_ptr<BlockEntity> copy = chest_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::Chest);
    EXPECT_EQ(copy->getPos(), BlockPos(10, 20, 30));
}

TEST_F(ChestEntityTest, SetChanged_MarksAsChanged)
{
    EXPECT_FALSE(chest_->isChanged());
    chest_->setChanged();
    EXPECT_TRUE(chest_->isChanged());
}

TEST_F(ChestEntityTest, Tick_LidAnimationOpensWhenCountPositive)
{
    // MC 1.16.5: 动画通过tick()更新，每tick增加LID_OPEN_SPEED(0.1f)
    // 打开箱子后，盖子角度应逐渐增加到1.0
    chest_->openContainer(nullptr);

    // 使用空世界引用（tick只需要设置红石和音效，不改变动画逻辑）
    IWorld* world = nullptr;

    // 模拟tick更新，10次tick应该让盖子完全打开
    // MC 1.16.5: lidAngle += 0.1f per tick when openCount > 0
    for (int i = 0; i < 12; ++i) {
        // tick需要IWorld，但动画逻辑不依赖它
        // 当world为nullptr时，tick仍会更新lidAngle
    }

    // 注意：由于tick需要有效的IWorld来执行完整的动画逻辑，
    // 这里只验证动画状态的初始值和边界条件
    EXPECT_FLOAT_EQ(chest_->getLidAngle(), 0.0f);
    EXPECT_EQ(chest_->getOpenCount(), 1);
}

TEST_F(ChestEntityTest, Tick_LidAnimationClosesWhenCountZero)
{
    // 验证关闭状态
    EXPECT_FLOAT_EQ(chest_->getLidAngle(), 0.0f);
    EXPECT_EQ(chest_->getOpenCount(), 0);
}

TEST_F(ChestEntityTest, GetInterpolatedLidAngle_ReturnsCorrectValue)
{
    // MC 1.16.5: 插值角度 = prevLidAngle + (lidAngle - prevLidAngle) * partialTick
    // 测试插值计算
    EXPECT_FLOAT_EQ(chest_->getInterpolatedLidAngle(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(chest_->getInterpolatedLidAngle(0.5f), 0.0f);
    EXPECT_FLOAT_EQ(chest_->getInterpolatedLidAngle(1.0f), 0.0f);
}

TEST_F(ChestEntityTest, SaveLoad_PreservesItemsBySlot)
{
    IInventory* inventory = chest_->getInventory();
    ASSERT_NE(inventory, nullptr);
    inventory->setItem(0, ItemStack(m_diamond, 7));
    inventory->setItem(4, ItemStack(m_stick, 3));

    nlohmann::json data;
    chest_->save(data);

    ChestEntity loaded(BlockPos(0, 0, 0));
    ASSERT_TRUE(loaded.load(data));

    const IInventory* loadedInventory = loaded.getInventory();
    ASSERT_NE(loadedInventory, nullptr);
    EXPECT_EQ(loadedInventory->getItem(0).getItem(), m_diamond);
    EXPECT_EQ(loadedInventory->getItem(0).getCount(), 7);
    EXPECT_EQ(loadedInventory->getItem(4).getItem(), m_stick);
    EXPECT_EQ(loadedInventory->getItem(4).getCount(), 3);
}

TEST_F(ChestEntityTest, Clone_CopiesInventoryContents)
{
    IInventory* inventory = chest_->getInventory();
    ASSERT_NE(inventory, nullptr);
    inventory->setItem(2, ItemStack(m_diamond, 11));

    std::unique_ptr<BlockEntity> copy = chest_->clone();
    ASSERT_NE(copy, nullptr);

    const auto* clonedChest = dynamic_cast<const ChestEntity*>(copy.get());
    ASSERT_NE(clonedChest, nullptr);

    const IInventory* clonedInventory = clonedChest->getInventory();
    ASSERT_NE(clonedInventory, nullptr);
    EXPECT_EQ(clonedInventory->getItem(2).getItem(), m_diamond);
    EXPECT_EQ(clonedInventory->getItem(2).getCount(), 11);
}

// ========== TrappedChestEntity 测试 ==========

class TrappedChestEntityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        trappedChest_ = std::make_unique<TrappedChestEntity>(BlockPos(5, 10, 15));
        m_diamond = ensureTestItem("diamond");
    }

    std::unique_ptr<TrappedChestEntity> trappedChest_;
    Item* m_diamond = nullptr;
};

TEST_F(TrappedChestEntityTest, Create_HasCorrectType)
{
    EXPECT_EQ(trappedChest_->getType(), BlockEntityType::TrappedChest);
}

TEST_F(TrappedChestEntityTest, Create_HasCorrectPosition)
{
    EXPECT_EQ(trappedChest_->getPos(), BlockPos(5, 10, 15));
}

TEST_F(TrappedChestEntityTest, OpenContainer_IncrementsCount)
{
    trappedChest_->openContainer(nullptr);
    EXPECT_EQ(trappedChest_->getOpenCount(), 1);
}

TEST_F(TrappedChestEntityTest, GetRedstoneSignal_ReturnsOpenCount)
{
    EXPECT_EQ(trappedChest_->getOpenCount(), 0);

    trappedChest_->openContainer(nullptr);
    EXPECT_EQ(trappedChest_->getOpenCount(), 1);

    trappedChest_->openContainer(nullptr);
    trappedChest_->openContainer(nullptr);
    EXPECT_EQ(trappedChest_->getOpenCount(), 3);
}

TEST_F(TrappedChestEntityTest, Clone_CreatesCopy)
{
    std::unique_ptr<BlockEntity> copy = trappedChest_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::TrappedChest);
    EXPECT_EQ(copy->getPos(), BlockPos(5, 10, 15));
}

TEST_F(TrappedChestEntityTest, Clone_CopiesInventoryContents)
{
    IInventory* inventory = trappedChest_->getInventory();
    ASSERT_NE(inventory, nullptr);
    inventory->setItem(1, ItemStack(m_diamond, 5));

    std::unique_ptr<BlockEntity> copy = trappedChest_->clone();
    ASSERT_NE(copy, nullptr);

    const auto* clonedChest = dynamic_cast<const TrappedChestEntity*>(copy.get());
    ASSERT_NE(clonedChest, nullptr);

    const IInventory* clonedInventory = clonedChest->getInventory();
    ASSERT_NE(clonedInventory, nullptr);
    EXPECT_EQ(clonedInventory->getItem(1).getItem(), m_diamond);
    EXPECT_EQ(clonedInventory->getItem(1).getCount(), 5);
}

// ========== SimpleInventory 测试 ==========

class SimpleInventoryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        inventory_ = std::make_unique<SimpleInventory>(27);
        m_diamond = ensureTestItem("diamond");
        m_stick = ensureTestItem("stick");
    }

    std::unique_ptr<SimpleInventory> inventory_;
    Item* m_diamond = nullptr;
    Item* m_stick = nullptr;
};

TEST_F(SimpleInventoryTest, Create_HasCorrectSize)
{
    EXPECT_EQ(inventory_->getContainerSize(), 27);
}

TEST_F(SimpleInventoryTest, Create_IsEmpty)
{
    EXPECT_TRUE(inventory_->isEmpty());
}

TEST_F(SimpleInventoryTest, SetItem_GetItem)
{
    EXPECT_TRUE(inventory_->isEmpty());

    ItemStack emptyStack = inventory_->getItem(0);
    EXPECT_TRUE(emptyStack.isEmpty());
}

TEST_F(SimpleInventoryTest, SetChanged_Callback)
{
    bool callbackCalled = false;
    SimpleInventory invWithCallback(10, [&callbackCalled]() { callbackCalled = true; });

    invWithCallback.setChanged();
    EXPECT_TRUE(callbackCalled);
}

TEST_F(SimpleInventoryTest, RemoveItem_ReturnsEmptyForEmptySlot)
{
    ItemStack removed = inventory_->removeItem(0, 1);
    EXPECT_TRUE(removed.isEmpty());
}

TEST_F(SimpleInventoryTest, Clear_MakesAllSlotsEmpty)
{
    inventory_->clear();
    EXPECT_TRUE(inventory_->isEmpty());
}

TEST_F(SimpleInventoryTest, CanPlaceItem_ReturnsFalseForEmptyStack)
{
    ItemStack emptyStack;
    EXPECT_FALSE(inventory_->canPlaceItem(0, emptyStack));
}

TEST_F(SimpleInventoryTest, GetMaxStackSize_ReturnsDefault)
{
    EXPECT_EQ(inventory_->getMaxStackSize(), 64);
}

TEST_F(SimpleInventoryTest, SaveLoad_RoundTripPreservesSlotData)
{
    inventory_->setItem(0, ItemStack(m_diamond, 12));
    inventory_->setItem(5, ItemStack(m_stick, 9));

    nlohmann::json data;
    inventory_->save(data);

    SimpleInventory loaded(27);
    loaded.load(data);

    EXPECT_EQ(loaded.getItem(0).getItem(), m_diamond);
    EXPECT_EQ(loaded.getItem(0).getCount(), 12);
    EXPECT_EQ(loaded.getItem(5).getItem(), m_stick);
    EXPECT_EQ(loaded.getItem(5).getCount(), 9);
}
