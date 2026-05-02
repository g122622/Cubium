#include <gtest/gtest.h>
#include "entity/inventory/container/BrewingStandContainer.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"

using namespace mc;
using namespace mc::blockentity;

// ========== BrewingStandContainer 测试 ==========

class BrewingStandContainerTest : public ::testing::Test {
protected:
    void SetUp() override {
        playerInventory_ = std::make_unique<PlayerInventory>();
        // 创建酿造台背包容器（5格：3药水 + 材料 + 燃料）
        brewingInventory_ = std::make_unique<SimpleInventory>(BrewingStandContainer::BREWING_SLOTS);
    }

    std::unique_ptr<PlayerInventory> playerInventory_;
    std::unique_ptr<SimpleInventory> brewingInventory_;
};

TEST_F(BrewingStandContainerTest, Create_HasCorrectSlotCount) {
    // 容器实际槽位数量 = 酿造台槽位 + 玩家背包槽位 = 5 + 36 = 41
    BrewingStandContainer container(ContainerId(1), playerInventory_.get(), brewingInventory_.get());
    EXPECT_EQ(container.getSlotCount(), 41);
}

TEST_F(BrewingStandContainerTest, ContainerType_IsCorrect) {
    BrewingStandContainer container(ContainerId(1), playerInventory_.get(), brewingInventory_.get());
    EXPECT_EQ(container.getId(), ContainerId(1));
}

TEST_F(BrewingStandContainerTest, SlotIndices_AreCorrect) {
    EXPECT_EQ(BrewingStandContainer::SLOT_POTION_START, 0);
    EXPECT_EQ(BrewingStandContainer::POTION_SLOTS, 3);
    EXPECT_EQ(BrewingStandContainer::SLOT_INGREDIENT, 3);
    EXPECT_EQ(BrewingStandContainer::SLOT_FUEL, 4);
    EXPECT_EQ(BrewingStandContainer::BREWING_SLOTS, 5);
}

TEST_F(BrewingStandContainerTest, Constants_AreCorrect) {
    // 验证GUI布局常量存在
    EXPECT_GT(BrewingStandContainer::POTION_SLOT_X, 0);
    EXPECT_EQ(BrewingStandContainer::POTION_SLOT_Y[0], 51);
    EXPECT_EQ(BrewingStandContainer::POTION_SLOT_Y[1], 69);
    EXPECT_EQ(BrewingStandContainer::POTION_SLOT_Y[2], 87);
    EXPECT_GT(BrewingStandContainer::INGREDIENT_SLOT_X, 0);
    EXPECT_GT(BrewingStandContainer::INGREDIENT_SLOT_Y, 0);
    EXPECT_GT(BrewingStandContainer::FUEL_SLOT_X, 0);
    EXPECT_GT(BrewingStandContainer::FUEL_SLOT_Y, 0);
    EXPECT_GT(BrewingStandContainer::PLAYER_INV_Y, BrewingStandContainer::INGREDIENT_SLOT_Y);
    EXPECT_GT(BrewingStandContainer::HOTBAR_Y, BrewingStandContainer::PLAYER_INV_Y);
}

TEST_F(BrewingStandContainerTest, GetBrewingStandInventory_ReturnsCorrectInventory) {
    BrewingStandContainer container(ContainerId(1), playerInventory_.get(), brewingInventory_.get());
    EXPECT_EQ(container.getBrewingStandInventory(), brewingInventory_.get());
}

TEST_F(BrewingStandContainerTest, GetBrewTime_ReturnsZeroInitially) {
    BrewingStandContainer container(ContainerId(1), playerInventory_.get(), brewingInventory_.get());
    EXPECT_EQ(container.getBrewTime(), 0);
}

TEST_F(BrewingStandContainerTest, GetFuelLevel_ReturnsZeroInitially) {
    BrewingStandContainer container(ContainerId(1), playerInventory_.get(), brewingInventory_.get());
    EXPECT_EQ(container.getFuelLevel(), 0);
}

TEST_F(BrewingStandContainerTest, SetBrewTime_UpdatesValue) {
    BrewingStandContainer container(ContainerId(1), playerInventory_.get(), brewingInventory_.get());
    container.setBrewTime(200);
    EXPECT_EQ(container.getBrewTime(), 200);
}

TEST_F(BrewingStandContainerTest, SetFuel_UpdatesValue) {
    BrewingStandContainer container(ContainerId(1), playerInventory_.get(), brewingInventory_.get());
    container.setFuel(15);
    EXPECT_EQ(container.getFuelLevel(), 15);
}

TEST_F(BrewingStandContainerTest, StillValid_ReturnsTrue) {
    BrewingStandContainer container(ContainerId(1), playerInventory_.get(), brewingInventory_.get());
    EXPECT_TRUE(container.stillValid(*playerInventory_->getPlayer()));
}
