#include <gtest/gtest.h>
#include "entity/inventory/container/FurnaceContainer.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"

using namespace mc;
using namespace mc::blockentity;

// ========== FurnaceContainer 测试 ==========

class FurnaceContainerTest : public ::testing::Test {
protected:
    void SetUp() override {
        playerInventory_ = std::make_unique<PlayerInventory>();
        // 创建熔炉背包容器（3格：输入、燃料、输出）
        furnaceInventory_ = std::make_unique<SimpleInventory>(FurnaceContainer::FURNACE_SLOTS);
    }

    std::unique_ptr<PlayerInventory> playerInventory_;
    std::unique_ptr<SimpleInventory> furnaceInventory_;
};

TEST_F(FurnaceContainerTest, Create_HasCorrectSlotCount) {
    // 注意: 在Release模式下MC_ASSERT不起作用
    // 容器实际槽位数量 = 熔炉槽位 + 玩家背包槽位 = 3 + 36 = 39
    // 测试验证熔炉背包已正确设置
    FurnaceContainer container(ContainerId(1), playerInventory_.get(), furnaceInventory_.get());
    EXPECT_EQ(container.getFurnaceInventory(), furnaceInventory_.get());
    EXPECT_EQ(container.getSlotCount(), 39);
}

TEST_F(FurnaceContainerTest, GetFurnaceInventory_ReturnsCorrectInventory) {
    FurnaceContainer container(ContainerId(1), playerInventory_.get(), furnaceInventory_.get());

    EXPECT_EQ(container.getFurnaceInventory(), furnaceInventory_.get());
}

TEST_F(FurnaceContainerTest, ContainerType_IsCorrect) {
    FurnaceContainer container(ContainerId(1), playerInventory_.get(), furnaceInventory_.get());

    EXPECT_EQ(container.getId(), ContainerId(1));
}

TEST_F(FurnaceContainerTest, SlotIndices_AreCorrect) {
    EXPECT_EQ(FurnaceContainer::SLOT_INPUT, 0);
    EXPECT_EQ(FurnaceContainer::SLOT_FUEL, 1);
    EXPECT_EQ(FurnaceContainer::SLOT_OUTPUT, 2);
}

TEST_F(FurnaceContainerTest, FurnaceSlots_IsThree) {
    EXPECT_EQ(FurnaceContainer::FURNACE_SLOTS, 3);
}

TEST_F(FurnaceContainerTest, Constants_AreCorrect) {
    // 验证GUI布局常量存在
    EXPECT_GT(FurnaceContainer::FURNACE_SLOT_Y, 0);
    EXPECT_GT(FurnaceContainer::PLAYER_INV_Y, FurnaceContainer::FURNACE_SLOT_Y);
    EXPECT_GT(FurnaceContainer::HOTBAR_Y, FurnaceContainer::PLAYER_INV_Y);
    EXPECT_EQ(FurnaceContainer::SLOT_SIZE, 18);
}
