#include <gtest/gtest.h>
#include "entity/inventory/container/EnchantmentContainer.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "world/block/BlockPos.hpp"
#include "common/util/math/random/Random.hpp"

using namespace mc;

// ========== EnchantmentTableContainer 测试 ==========

class EnchantmentTableContainerTest : public ::testing::Test {
protected:
    void SetUp() override {
        playerInventory_ = std::make_unique<PlayerInventory>();
    }

    std::unique_ptr<PlayerInventory> playerInventory_;
};

TEST_F(EnchantmentTableContainerTest, Create_HasCorrectSlotCount) {
    // 容器实际槽位数量 = 附魔台槽位 + 玩家背包槽位 = 2 + 36 = 38
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    EXPECT_EQ(container.getSlotCount(), 38);
}

TEST_F(EnchantmentTableContainerTest, ContainerType_IsCorrect) {
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    EXPECT_EQ(container.getId(), ContainerId(1));
}

TEST_F(EnchantmentTableContainerTest, SlotIndices_AreCorrect) {
    EXPECT_EQ(EnchantmentContainer::SLOT_ITEM, 0);
    EXPECT_EQ(EnchantmentContainer::SLOT_LAPIS, 1);
    EXPECT_EQ(EnchantmentContainer::ENCHANTMENT_SLOTS, 2);
}

TEST_F(EnchantmentTableContainerTest, Constants_AreCorrect) {
    // 验证GUI布局常量存在
    EXPECT_GT(EnchantmentContainer::ITEM_SLOT_X, 0);
    EXPECT_GT(EnchantmentContainer::ITEM_SLOT_Y, 0);
    EXPECT_GT(EnchantmentContainer::LAPIS_SLOT_X, EnchantmentContainer::ITEM_SLOT_X);
    EXPECT_EQ(EnchantmentContainer::LAPIS_SLOT_Y, EnchantmentContainer::ITEM_SLOT_Y);
    EXPECT_GT(EnchantmentContainer::PLAYER_INV_Y, EnchantmentContainer::ITEM_SLOT_Y);
    EXPECT_GT(EnchantmentContainer::HOTBAR_Y, EnchantmentContainer::PLAYER_INV_Y);
    EXPECT_EQ(EnchantmentContainer::ENCHANTMENT_OPTIONS, 3);
}

TEST_F(EnchantmentTableContainerTest, GetItemSlot_ReturnsEmptyWhenEmpty) {
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);

    ItemStack item = container.getItemSlot();
    EXPECT_TRUE(item.isEmpty());
}

TEST_F(EnchantmentTableContainerTest, GetLapisSlot_ReturnsEmptyWhenEmpty) {
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);

    ItemStack lapis = container.getLapisSlot();
    EXPECT_TRUE(lapis.isEmpty());
}

TEST_F(EnchantmentTableContainerTest, GetEnchantPower_ReturnsZeroWithoutWorld) {
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);

    // 没有世界对象，书架力量应为0
    EXPECT_EQ(container.getEnchantPower(), 0);
}

TEST_F(EnchantmentTableContainerTest, GetEnchantmentLevel_ReturnsZeroForInvalidIndex) {
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);

    EXPECT_EQ(container.getEnchantmentLevel(-1), 0);
    EXPECT_EQ(container.getEnchantmentLevel(3), 0);
    EXPECT_EQ(container.getEnchantmentLevel(100), 0);
}

TEST_F(EnchantmentTableContainerTest, GetEnchantmentClue_ReturnsEmptyForInvalidIndex) {
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);

    EXPECT_EQ(container.getEnchantmentClue(-1), "");
    EXPECT_EQ(container.getEnchantmentClue(3), "");
    EXPECT_EQ(container.getEnchantmentClue(100), "");
}

TEST_F(EnchantmentTableContainerTest, GetEnchantmentWorldClue_ReturnsZeroForInvalidIndex) {
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);

    EXPECT_EQ(container.getEnchantmentWorldClue(-1), 0);
    EXPECT_EQ(container.getEnchantmentWorldClue(3), 0);
    EXPECT_EQ(container.getEnchantmentWorldClue(100), 0);
}

TEST_F(EnchantmentTableContainerTest, IsEnchantmentOptionAvailable_ReturnsFalseForInvalidIndex) {
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);

    EXPECT_FALSE(container.isEnchantmentOptionAvailable(-1));
    EXPECT_FALSE(container.isEnchantmentOptionAvailable(3));
    EXPECT_FALSE(container.isEnchantmentOptionAvailable(100));
}

TEST_F(EnchantmentTableContainerTest, IsEnchantmentOptionAvailable_ReturnsFalseWhenEmpty) {
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);

    // 没有物品时，所有选项不可用
    EXPECT_FALSE(container.isEnchantmentOptionAvailable(0));
    EXPECT_FALSE(container.isEnchantmentOptionAvailable(1));
    EXPECT_FALSE(container.isEnchantmentOptionAvailable(2));
}
