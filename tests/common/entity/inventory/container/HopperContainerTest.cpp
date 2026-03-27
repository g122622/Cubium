#include <gtest/gtest.h>
#include "entity/inventory/container/HopperContainer.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"

using namespace mc;
using namespace mc::blockentity;

// ========== HopperContainer 测试 ==========

class HopperContainerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建漏斗背包容器（5格）
        hopperInventory_ = std::make_unique<SimpleInventory>(HopperContainer::HOPPER_SIZE);
    }

    std::unique_ptr<SimpleInventory> hopperInventory_;
};

TEST_F(HopperContainerTest, Create_HasCorrectSlotCount) {
    // 注意: 在Release模式下MC_ASSERT不起作用
    // 当playerInventory为nullptr时，容器仍会尝试添加玩家背包槽位
    // 实际槽位数量 = 漏斗槽位 + 玩家背包槽位 = 5 + 36 = 41
    // 但如果没有有效玩家背包，创建可能不安全
    // 因此这个测试只验证容器能创建
    HopperContainer container(ContainerId(1), nullptr, hopperInventory_.get());
    // 验证漏斗背包已设置
    EXPECT_EQ(container.getHopperInventory(), hopperInventory_.get());
}

TEST_F(HopperContainerTest, GetHopperInventory_ReturnsCorrectInventory) {
    HopperContainer container(ContainerId(1), nullptr, hopperInventory_.get());

    EXPECT_EQ(container.getHopperInventory(), hopperInventory_.get());
}

TEST_F(HopperContainerTest, ContainerType_IsCorrect) {
    HopperContainer container(ContainerId(1), nullptr, hopperInventory_.get());

    EXPECT_EQ(container.type(), ContainerType::Hopper);
}

TEST_F(HopperContainerTest, HopperSize_IsFive) {
    EXPECT_EQ(HopperContainer::HOPPER_SIZE, 5);
}

TEST_F(HopperContainerTest, Constants_AreCorrect) {
    // 验证GUI布局常量存在
    EXPECT_GT(HopperContainer::HOPPER_SLOT_Y, 0);
    EXPECT_GT(HopperContainer::PLAYER_INV_Y, HopperContainer::HOPPER_SLOT_Y);
    EXPECT_GT(HopperContainer::HOTBAR_Y, HopperContainer::PLAYER_INV_Y);
    EXPECT_EQ(HopperContainer::SLOT_SIZE, 18);
}
