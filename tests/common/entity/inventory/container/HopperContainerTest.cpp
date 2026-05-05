#include <gtest/gtest.h>
#include "entity/inventory/container/HopperContainer.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"

using namespace mc;

// ========== HopperContainer 测试 ==========

class HopperContainerTest : public ::testing::Test {
protected:
    void SetUp() override {
        playerInventory_ = std::make_unique<PlayerInventory>();
        // 创建漏斗背包容器（5格）
        hopperInventory_ = std::make_unique<blockentity::SimpleInventory>(HopperContainer::HOPPER_SIZE);
    }

    std::unique_ptr<PlayerInventory> playerInventory_;
    std::unique_ptr<blockentity::SimpleInventory> hopperInventory_;
};

TEST_F(HopperContainerTest, Create_HasCorrectSlotCount) {
    // 容器实际槽位数量 = 漏斗槽位 + 玩家背包槽位 = 5 + 36 = 41
    HopperContainer container(ContainerId(1), playerInventory_.get(), hopperInventory_.get());
    EXPECT_EQ(container.getSlotCount(), 41);
}

TEST_F(HopperContainerTest, GetHopperInventory_ReturnsCorrectInventory) {
    HopperContainer container(ContainerId(1), playerInventory_.get(), hopperInventory_.get());
    EXPECT_EQ(container.getHopperInventory(), hopperInventory_.get());
}

TEST_F(HopperContainerTest, ContainerId_IsCorrect) {
    HopperContainer container(ContainerId(1), playerInventory_.get(), hopperInventory_.get());
    EXPECT_EQ(container.getId(), ContainerId(1));
}

TEST_F(HopperContainerTest, HopperSize_IsFive) {
    EXPECT_EQ(HopperContainer::HOPPER_SIZE, 5);
}

TEST_F(HopperContainerTest, Constants_AreCorrect_MC1165) {
    // 验证GUI布局常量 - MC 1.16.5坐标
    EXPECT_EQ(HopperContainer::HOPPER_SLOT_START_X, 44);
    EXPECT_EQ(HopperContainer::HOPPER_SLOT_Y, 20);
    EXPECT_EQ(HopperContainer::PLAYER_INV_Y, 51);
    EXPECT_EQ(HopperContainer::HOTBAR_Y, 109);
    EXPECT_EQ(HopperContainer::SLOT_SIZE, 18);
}

TEST_F(HopperContainerTest, StillValid_ReturnsTrue) {
    HopperContainer container(ContainerId(1), playerInventory_.get(), hopperInventory_.get());
    // 由于SimpleInventory默认实现isUsableByPlayer返回true
    EXPECT_TRUE(container.stillValid(*playerInventory_->getPlayer()));
}
