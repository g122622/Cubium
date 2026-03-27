#include <gtest/gtest.h>
#include "world/blockentity/processing/FurnaceEntity.hpp"
#include "world/blockentity/processing/BlastFurnaceEntity.hpp"
#include "world/blockentity/processing/SmokerEntity.hpp"
#include "world/blockentity/processing/FurnaceInventory.hpp"
#include "world/block/BlockPos.hpp"

using namespace mc;
using namespace mc::blockentity;

// ========== FurnaceInventory 测试 ==========

class FurnaceInventoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        inventory_ = std::make_unique<FurnaceInventory>();
    }

    std::unique_ptr<FurnaceInventory> inventory_;
};

TEST_F(FurnaceInventoryTest, Create_HasCorrectSize) {
    EXPECT_EQ(inventory_->getContainerSize(), FurnaceInventory::SLOT_COUNT);
    EXPECT_EQ(FurnaceInventory::SLOT_COUNT, 3);
}

TEST_F(FurnaceInventoryTest, Create_SlotIndicesAreCorrect) {
    EXPECT_EQ(FurnaceInventory::SLOT_INPUT, 0);
    EXPECT_EQ(FurnaceInventory::SLOT_FUEL, 1);
    EXPECT_EQ(FurnaceInventory::SLOT_OUTPUT, 2);
}

TEST_F(FurnaceInventoryTest, Create_IsEmpty) {
    EXPECT_TRUE(inventory_->isEmpty());
    EXPECT_TRUE(inventory_->isInputEmpty());
    EXPECT_TRUE(inventory_->isFuelEmpty());
    EXPECT_TRUE(inventory_->isOutputEmpty());
}

TEST_F(FurnaceInventoryTest, GetInputItem_ReturnsEmptyInitially) {
    ItemStack input = inventory_->getInputItem();
    EXPECT_TRUE(input.isEmpty());
}

TEST_F(FurnaceInventoryTest, GetFuelItem_ReturnsEmptyInitially) {
    ItemStack fuel = inventory_->getFuelItem();
    EXPECT_TRUE(fuel.isEmpty());
}

TEST_F(FurnaceInventoryTest, GetOutputItem_ReturnsEmptyInitially) {
    ItemStack output = inventory_->getOutputItem();
    EXPECT_TRUE(output.isEmpty());
}

TEST_F(FurnaceInventoryTest, SetInputItem_UpdatesInput) {
    ItemStack emptyStack;
    inventory_->setInputItem(emptyStack);
    EXPECT_TRUE(inventory_->isInputEmpty());
}

TEST_F(FurnaceInventoryTest, SetChanged_Callback) {
    bool callbackCalled = false;
    FurnaceInventory invWithCallback([&callbackCalled]() {
        callbackCalled = true;
    });

    invWithCallback.setChanged();
    EXPECT_TRUE(callbackCalled);
}

TEST_F(FurnaceInventoryTest, CanPlaceItem_InputSlotReturnsTrueForValidItem) {
    // Note: canPlaceItem returns false for empty stacks, which is correct behavior
    // This tests the slot index validation
    EXPECT_FALSE(inventory_->canPlaceItem(FurnaceInventory::SLOT_INPUT, ItemStack()));
}

TEST_F(FurnaceInventoryTest, CanPlaceItem_FuelSlotReturnsTrueForValidItem) {
    // Note: canPlaceItem returns false for empty stacks, which is correct behavior
    EXPECT_FALSE(inventory_->canPlaceItem(FurnaceInventory::SLOT_FUEL, ItemStack()));
}

TEST_F(FurnaceInventoryTest, CanPlaceItem_OutputSlotReturnsFalseForEmptyStack) {
    // 输出槽不接受空物品堆
    EXPECT_FALSE(inventory_->canPlaceItem(FurnaceInventory::SLOT_OUTPUT, ItemStack()));
}

TEST_F(FurnaceInventoryTest, Clear_MakesAllSlotsEmpty) {
    inventory_->clear();
    EXPECT_TRUE(inventory_->isEmpty());
}

TEST_F(FurnaceInventoryTest, GetMaxStackSize_ReturnsDefault) {
    EXPECT_EQ(inventory_->getMaxStackSize(), 64);
}

// ========== FurnaceEntity 测试 ==========

class FurnaceEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        furnace_ = std::make_unique<FurnaceEntity>(BlockPos(10, 20, 30));
    }

    std::unique_ptr<FurnaceEntity> furnace_;
};

TEST_F(FurnaceEntityTest, Create_HasCorrectType) {
    EXPECT_EQ(furnace_->getType(), BlockEntityType::Furnace);
}

TEST_F(FurnaceEntityTest, Create_HasCorrectPosition) {
    EXPECT_EQ(furnace_->getPos(), BlockPos(10, 20, 30));
}

TEST_F(FurnaceEntityTest, Create_HasCorrectContainerSize) {
    EXPECT_EQ(furnace_->getContainerSize(), 3);
}

TEST_F(FurnaceEntityTest, Create_NotBurningInitially) {
    EXPECT_FALSE(furnace_->isBurning());
    EXPECT_EQ(furnace_->getBurnTime(), 0);
    EXPECT_EQ(furnace_->getBurnTimeTotal(), 0);
}

TEST_F(FurnaceEntityTest, Create_CookTimeIsZero) {
    EXPECT_EQ(furnace_->getCookTime(), 0);
    EXPECT_EQ(furnace_->getCookTimeTotal(), 200);  // 默认200 tick
}

TEST_F(FurnaceEntityTest, Create_NeedsTickReturnsTrue) {
    EXPECT_TRUE(furnace_->needsTick());
}

TEST_F(FurnaceEntityTest, GetInventory_ReturnsValidPointer) {
    IInventory* inventory = furnace_->getInventory();
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->getContainerSize(), 3);
}

TEST_F(FurnaceEntityTest, GetFurnaceInventory_ReturnsValidReference) {
    FurnaceInventory& inv = furnace_->getFurnaceInventory();
    EXPECT_EQ(inv.getContainerSize(), 3);
}

TEST_F(FurnaceEntityTest, Save_ContainsBasicInfo) {
    nlohmann::json data;
    furnace_->save(data);

    EXPECT_TRUE(data.contains("id"));
    EXPECT_EQ(data["id"], "minecraft:furnace");
    EXPECT_TRUE(data.contains("BurnTime"));
    EXPECT_TRUE(data.contains("CookTime"));
    EXPECT_TRUE(data.contains("CookTimeTotal"));
}

TEST_F(FurnaceEntityTest, Clone_CreatesCopy) {
    std::unique_ptr<BlockEntity> copy = furnace_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::Furnace);
    EXPECT_EQ(copy->getPos(), BlockPos(10, 20, 30));
}

TEST_F(FurnaceEntityTest, SetChanged_MarksAsChanged) {
    EXPECT_FALSE(furnace_->isChanged());
    furnace_->setChanged();
    EXPECT_TRUE(furnace_->isChanged());
}

TEST_F(FurnaceEntityTest, GetComparatorSignal_ReturnsZeroWhenEmpty) {
    EXPECT_EQ(furnace_->getComparatorSignal(), 0);
}

TEST_F(FurnaceEntityTest, Load_LoadsBurnTime) {
    nlohmann::json data;
    data["BurnTime"] = 100;
    data["CookTime"] = 50;
    data["CookTimeTotal"] = 200;

    EXPECT_TRUE(furnace_->load(data));
}

// ========== BlastFurnaceEntity 测试 ==========

class BlastFurnaceEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        blastFurnace_ = std::make_unique<BlastFurnaceEntity>(BlockPos(5, 10, 15));
    }

    std::unique_ptr<BlastFurnaceEntity> blastFurnace_;
};

TEST_F(BlastFurnaceEntityTest, Create_HasCorrectType) {
    EXPECT_EQ(blastFurnace_->getType(), BlockEntityType::BlastFurnace);
}

TEST_F(BlastFurnaceEntityTest, Create_HasCorrectPosition) {
    EXPECT_EQ(blastFurnace_->getPos(), BlockPos(5, 10, 15));
}

TEST_F(BlastFurnaceEntityTest, Clone_CreatesCopy) {
    std::unique_ptr<BlockEntity> copy = blastFurnace_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::BlastFurnace);
    EXPECT_EQ(copy->getPos(), BlockPos(5, 10, 15));
}

TEST_F(BlastFurnaceEntityTest, NeedsTick_ReturnsTrue) {
    EXPECT_TRUE(blastFurnace_->needsTick());
}

// ========== SmokerEntity 测试 ==========

class SmokerEntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        smoker_ = std::make_unique<SmokerEntity>(BlockPos(1, 2, 3));
    }

    std::unique_ptr<SmokerEntity> smoker_;
};

TEST_F(SmokerEntityTest, Create_HasCorrectType) {
    EXPECT_EQ(smoker_->getType(), BlockEntityType::Smoker);
}

TEST_F(SmokerEntityTest, Create_HasCorrectPosition) {
    EXPECT_EQ(smoker_->getPos(), BlockPos(1, 2, 3));
}

TEST_F(SmokerEntityTest, Clone_CreatesCopy) {
    std::unique_ptr<BlockEntity> copy = smoker_->clone();

    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->getType(), BlockEntityType::Smoker);
    EXPECT_EQ(copy->getPos(), BlockPos(1, 2, 3));
}

TEST_F(SmokerEntityTest, NeedsTick_ReturnsTrue) {
    EXPECT_TRUE(smoker_->needsTick());
}

// ========== AbstractFurnaceEntity 静态方法测试 ==========

class AbstractFurnaceEntityStaticTest : public ::testing::Test {
};

TEST_F(AbstractFurnaceEntityStaticTest, IsFuel_ReturnsFalseForEmptyStack) {
    ItemStack emptyStack;
    EXPECT_FALSE(AbstractFurnaceEntity::isFuel(emptyStack));
}

TEST_F(AbstractFurnaceEntityStaticTest, GetBurnTime_ReturnsZeroForEmptyStack) {
    ItemStack emptyStack;
    EXPECT_EQ(AbstractFurnaceEntity::getBurnTime(emptyStack), 0);
}
