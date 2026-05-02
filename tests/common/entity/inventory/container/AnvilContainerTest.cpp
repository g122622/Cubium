#include <gtest/gtest.h>
#include "entity/inventory/container/AnvilContainer.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "world/block/BlockPos.hpp"

using namespace mc;

// ========== AnvilContainer 测试 ==========

class AnvilContainerTest : public ::testing::Test {
protected:
    void SetUp() override {
        playerInventory_ = std::make_unique<PlayerInventory>();
    }

    std::unique_ptr<PlayerInventory> playerInventory_;
};

TEST_F(AnvilContainerTest, Create_HasCorrectSlotCount) {
    // 容器实际槽位数量 = 铁砧槽位 + 玩家背包槽位 = 3 + 36 = 39
    BlockPos pos(0, 0, 0);
    AnvilContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    EXPECT_EQ(container.getSlotCount(), 39);
}

TEST_F(AnvilContainerTest, ContainerType_IsCorrect) {
    BlockPos pos(0, 0, 0);
    AnvilContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    EXPECT_EQ(container.getId(), ContainerId(1));
}

TEST_F(AnvilContainerTest, SlotIndices_AreCorrect) {
    EXPECT_EQ(AnvilContainer::SLOT_INPUT_1, 0);
    EXPECT_EQ(AnvilContainer::SLOT_INPUT_2, 1);
    EXPECT_EQ(AnvilContainer::SLOT_OUTPUT, 2);
    EXPECT_EQ(AnvilContainer::ANVIL_SLOTS, 3);
}

TEST_F(AnvilContainerTest, Constants_AreCorrect) {
    // 验证GUI布局常量存在
    EXPECT_EQ(AnvilContainer::INPUT_SLOT_X[0], 27);
    EXPECT_EQ(AnvilContainer::INPUT_SLOT_X[1], 76);
    EXPECT_EQ(AnvilContainer::INPUT_SLOT_Y, 47);
    EXPECT_EQ(AnvilContainer::OUTPUT_SLOT_X, 134);
    EXPECT_EQ(AnvilContainer::OUTPUT_SLOT_Y, 47);
    EXPECT_GT(AnvilContainer::PLAYER_INV_Y, AnvilContainer::INPUT_SLOT_Y);
    EXPECT_GT(AnvilContainer::HOTBAR_Y, AnvilContainer::PLAYER_INV_Y);
    EXPECT_EQ(AnvilContainer::MAX_REPAIR_COST, 40);
}

TEST_F(AnvilContainerTest, GetRepairCost_ReturnsZeroInitially) {
    BlockPos pos(0, 0, 0);
    AnvilContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    EXPECT_EQ(container.getRepairCost(), 0);
}

TEST_F(AnvilContainerTest, GetMaterialCost_ReturnsZeroInitially) {
    BlockPos pos(0, 0, 0);
    AnvilContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    EXPECT_EQ(container.getMaterialCost(), 0);
}

TEST_F(AnvilContainerTest, IsTooExpensive_ReturnsFalseInitially) {
    BlockPos pos(0, 0, 0);
    AnvilContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    EXPECT_FALSE(container.isTooExpensive());
}

TEST_F(AnvilContainerTest, GetItemName_ReturnsEmptyInitially) {
    BlockPos pos(0, 0, 0);
    AnvilContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    EXPECT_EQ(container.getItemName(), "");
}

TEST_F(AnvilContainerTest, SetItemName_UpdatesName) {
    BlockPos pos(0, 0, 0);
    AnvilContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    container.setItemName("Test Item");
    EXPECT_EQ(container.getItemName(), "Test Item");
}

TEST_F(AnvilContainerTest, GetInputSlot1_ReturnsEmptyWhenEmpty) {
    BlockPos pos(0, 0, 0);
    AnvilContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    EXPECT_TRUE(container.getInputSlot1().isEmpty());
}

TEST_F(AnvilContainerTest, GetInputSlot2_ReturnsEmptyWhenEmpty) {
    BlockPos pos(0, 0, 0);
    AnvilContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    EXPECT_TRUE(container.getInputSlot2().isEmpty());
}

TEST_F(AnvilContainerTest, GetOutputSlot_ReturnsEmptyWhenEmpty) {
    BlockPos pos(0, 0, 0);
    AnvilContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    EXPECT_TRUE(container.getOutputSlot().isEmpty());
}

TEST_F(AnvilContainerTest, IsRenameOnly_ReturnsFalseInitially) {
    BlockPos pos(0, 0, 0);
    AnvilContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    EXPECT_FALSE(container.isRenameOnly());
}

TEST_F(AnvilContainerTest, StillValid_ReturnsTrue) {
    BlockPos pos(0, 0, 0);
    AnvilContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    EXPECT_TRUE(container.stillValid(*playerInventory_->getPlayer()));
}

TEST_F(AnvilContainerTest, MaxRepairCost_IsCorrect) {
    EXPECT_EQ(AnvilContainer::MAX_REPAIR_COST, 40);
}
