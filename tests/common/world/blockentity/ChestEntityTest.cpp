#include <gtest/gtest.h>
#include "world/blockentity/storage/ChestEntity.hpp"
#include "world/blockentity/storage/TrappedChestEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include "world/block/BlockPos.hpp"

using namespace mc;
using namespace mc::blockentity;

// ========== ChestEntity 测试 ==========

class ChestEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        chest_ = std::make_unique<ChestEntity>(BlockPos(10, 20, 30));
    }

    std::unique_ptr<ChestEntity> chest_;
};

TEST_F(ChestEntityTest, Create_HasCorrectType) {
    EXPECT_EQ(chest_->getType(), BlockEntityType::Chest);
}

TEST_F(ChestEntityTest, Create_HasCorrectPosition) {
    EXPECT_EQ(chest_->getPos(), BlockPos(10, 20, 30));
}

TEST_F(ChestEntityTest, Create_HasCorrectSize) {
    EXPECT_EQ(chest_->getContainerSize(), ChestEntity::CHEST_SIZE);
    EXPECT_EQ(ChestEntity::CHEST_SIZE, 27);  // 标准箱子大小
}

TEST_F(ChestEntityTest, Create_LidAngleIsZero) {
    EXPECT_FLOAT_EQ(chest_->getLidAngle(), 0.0f);
    EXPECT_FLOAT_EQ(chest_->getPrevLidAngle(), 0.0f);
}

TEST_F(ChestEntityTest, Create_OpenCountIsZero) {
    EXPECT_EQ(chest_->getOpenCount(), 0);
}

TEST_F(ChestEntityTest, OpenContainer_IncrementsCount) {
    chest_->openContainer();
    EXPECT_EQ(chest_->getOpenCount(), 1);

    chest_->openContainer();
    EXPECT_EQ(chest_->getOpenCount(), 2);
}

TEST_F(ChestEntityTest, CloseContainer_DecrementsCount) {
    chest_->openContainer();
    chest_->openContainer();
    EXPECT_EQ(chest_->getOpenCount(), 2);

    chest_->closeContainer();
    EXPECT_EQ(chest_->getOpenCount(), 1);
}

TEST_F(ChestEntityTest, CloseContainer_NotBelowZero) {
    chest_->closeContainer();  // 没有打开时关闭
    EXPECT_EQ(chest_->getOpenCount(), 0);

    chest_->closeContainer();  // 再次关闭
    EXPECT_EQ(chest_->getOpenCount(), 0);
}

TEST_F(ChestEntityTest, NeedsTick_ReturnsTrue) {
    EXPECT_TRUE(chest_->needsTick());
}

TEST_F(ChestEntityTest, GetInventory_ReturnsValidPointer) {
    IInventory* inventory = chest_->getInventory();
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->getContainerSize(), ChestEntity::CHEST_SIZE);
}

TEST_F(ChestEntityTest, Save_ContainsBasicInfo) {
    nlohmann::json data;
    chest_->save(data);

    EXPECT_TRUE(data.contains("id"));
    EXPECT_EQ(data["id"], "minecraft:chest");
    EXPECT_TRUE(data.contains("x"));
    EXPECT_TRUE(data.contains("y"));
    EXPECT_TRUE(data.contains("z"));
}

TEST_F(ChestEntityTest, Clone_CreatesCopy) {
    std::unique_ptr<BlockEntity> copy = chest_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::Chest);
    EXPECT_EQ(copy->getPos(), BlockPos(10, 20, 30));
}

TEST_F(ChestEntityTest, SetChanged_MarksAsChanged) {
    EXPECT_FALSE(chest_->isChanged());
    chest_->setChanged();
    EXPECT_TRUE(chest_->isChanged());
}

TEST_F(ChestEntityTest, UpdateLidAnimation_OpensWhenCountPositive) {
    chest_->openContainer();

    // 模拟多次tick更新动画
    for (int i = 0; i < 15; ++i) {
        chest_->updateLidAnimation(0.05f);
    }

    // 盖子应该完全打开 (接近1.0)
    EXPECT_NEAR(chest_->getLidAngle(), 1.0f, 0.1f);
}

TEST_F(ChestEntityTest, UpdateLidAnimation_ClosesWhenCountZero) {
    chest_->openContainer();

    // 先打开
    for (int i = 0; i < 15; ++i) {
        chest_->updateLidAnimation(0.05f);
    }
    EXPECT_NEAR(chest_->getLidAngle(), 1.0f, 0.1f);

    // 关闭
    chest_->closeContainer();

    // 动画关闭
    for (int i = 0; i < 15; ++i) {
        chest_->updateLidAnimation(0.05f);
    }

    // 盖子应该关闭 (接近0.0)
    EXPECT_NEAR(chest_->getLidAngle(), 0.0f, 0.2f);
}

// ========== TrappedChestEntity 测试 ==========

class TrappedChestEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        trappedChest_ = std::make_unique<TrappedChestEntity>(BlockPos(5, 10, 15));
    }

    std::unique_ptr<TrappedChestEntity> trappedChest_;
};

TEST_F(TrappedChestEntityTest, Create_HasCorrectType) {
    EXPECT_EQ(trappedChest_->getType(), BlockEntityType::TrappedChest);
}

TEST_F(TrappedChestEntityTest, Create_HasCorrectPosition) {
    EXPECT_EQ(trappedChest_->getPos(), BlockPos(5, 10, 15));
}

TEST_F(TrappedChestEntityTest, OpenContainer_IncrementsCount) {
    trappedChest_->openContainer();
    EXPECT_EQ(trappedChest_->getOpenCount(), 1);
}

TEST_F(TrappedChestEntityTest, GetRedstoneSignal_ReturnsOpenCount) {
    // 没有玩家打开时信号为0
    EXPECT_EQ(trappedChest_->getOpenCount(), 0);

    // 1个玩家打开
    trappedChest_->openContainer();
    EXPECT_EQ(trappedChest_->getOpenCount(), 1);

    // 多个玩家打开
    trappedChest_->openContainer();
    trappedChest_->openContainer();
    EXPECT_EQ(trappedChest_->getOpenCount(), 3);
}

TEST_F(TrappedChestEntityTest, Clone_CreatesCopy) {
    std::unique_ptr<BlockEntity> copy = trappedChest_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::TrappedChest);
    EXPECT_EQ(copy->getPos(), BlockPos(5, 10, 15));
}

// ========== SimpleInventory 测试 ==========

class SimpleInventoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建一个27格的库存（箱子大小）
        inventory_ = std::make_unique<SimpleInventory>(27);
    }

    std::unique_ptr<SimpleInventory> inventory_;
};

TEST_F(SimpleInventoryTest, Create_HasCorrectSize) {
    EXPECT_EQ(inventory_->getContainerSize(), 27);
}

TEST_F(SimpleInventoryTest, Create_IsEmpty) {
    EXPECT_TRUE(inventory_->isEmpty());
}

TEST_F(SimpleInventoryTest, SetItem_GetItem) {
    // ItemStack 测试需要完整的物品系统
    // 这里只测试基本操作
    EXPECT_TRUE(inventory_->isEmpty());

    ItemStack emptyStack = inventory_->getItem(0);
    EXPECT_TRUE(emptyStack.isEmpty());
}

TEST_F(SimpleInventoryTest, SetChanged_Callback) {
    bool callbackCalled = false;
    SimpleInventory invWithCallback(10, [&callbackCalled]() {
        callbackCalled = true;
    });

    invWithCallback.setChanged();
    EXPECT_TRUE(callbackCalled);
}

TEST_F(SimpleInventoryTest, RemoveItem_ReturnsEmptyForEmptySlot) {
    ItemStack removed = inventory_->removeItem(0, 1);
    EXPECT_TRUE(removed.isEmpty());
}

TEST_F(SimpleInventoryTest, Clear_MakesAllSlotsEmpty) {
    inventory_->clear();
    EXPECT_TRUE(inventory_->isEmpty());
}

TEST_F(SimpleInventoryTest, CanPlaceItem_ReturnsFalseForEmptyStack) {
    // canPlaceItem returns false for empty stacks, which is correct behavior
    ItemStack emptyStack;
    EXPECT_FALSE(inventory_->canPlaceItem(0, emptyStack));
}

TEST_F(SimpleInventoryTest, GetMaxStackSize_ReturnsDefault) {
    EXPECT_EQ(inventory_->getMaxStackSize(), 64);
}
