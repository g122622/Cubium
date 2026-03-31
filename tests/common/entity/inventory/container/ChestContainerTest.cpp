#include <gtest/gtest.h>
#include "entity/inventory/container/ChestContainer.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include "item/core/ItemRegistry.hpp"

using namespace mc;
using namespace mc::blockentity;

// ========== ChestContainer 测试 ==========

class ChestContainerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建单箱背包容器
        singleChestInventory_ = std::make_unique<SimpleInventory>(27);
        // 创建双箱背包容器
        doubleChestInventory_ = std::make_unique<SimpleInventory>(54);
    }

    std::unique_ptr<SimpleInventory> singleChestInventory_;
    std::unique_ptr<SimpleInventory> doubleChestInventory_;
};

TEST_F(ChestContainerTest, CreateSingleChest_HasCorrectSlotCount) {
    auto container = ChestContainer::createSingle(
        ContainerId(1),
        nullptr,
        singleChestInventory_.get()
    );

    // 单箱: 27格箱子 + 27格主背包 + 9格快捷栏 = 63格
    // 注意: 如果没有玩家背包，只有箱子槽位
    EXPECT_NE(container, nullptr);
    EXPECT_EQ(container->getRowCount(), ChestContainer::SINGLE_CHEST_ROWS);
    EXPECT_EQ(container->getChestSlotCount(), 27);
}

TEST_F(ChestContainerTest, CreateDoubleChest_HasCorrectSlotCount) {
    auto container = ChestContainer::createDouble(
        ContainerId(1),
        nullptr,
        doubleChestInventory_.get()
    );

    EXPECT_NE(container, nullptr);
    EXPECT_EQ(container->getRowCount(), ChestContainer::DOUBLE_CHEST_ROWS);
    EXPECT_EQ(container->getChestSlotCount(), 54);
}

TEST_F(ChestContainerTest, GetChestInventory_ReturnsCorrectInventory) {
    auto container = ChestContainer::createSingle(
        ContainerId(1),
        nullptr,
        singleChestInventory_.get()
    );

    EXPECT_EQ(container->getChestInventory(), singleChestInventory_.get());
}

TEST_F(ChestContainerTest, ContainerType_IsCorrect) {
    auto container = ChestContainer::createSingle(
        ContainerId(1),
        nullptr,
        singleChestInventory_.get()
    );

    EXPECT_EQ(container->type(), ContainerType::Chest);
}

TEST_F(ChestContainerTest, SlotPerRow_IsNine) {
    EXPECT_EQ(ChestContainer::SLOTS_PER_ROW, 9);
}

TEST_F(ChestContainerTest, SingleChestRows_IsThree) {
    EXPECT_EQ(ChestContainer::SINGLE_CHEST_ROWS, 3);
}

TEST_F(ChestContainerTest, DoubleChestRows_IsSix) {
    EXPECT_EQ(ChestContainer::DOUBLE_CHEST_ROWS, 6);
}
