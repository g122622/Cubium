#include <gtest/gtest.h>
#include "entity/inventory/container/ChestContainer.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "entity/entities/player/Player.hpp"
#include "world/blockentity/storage/ChestEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"

using namespace mc;

// ========== ChestContainer 测试 ==========

class ChestContainerTest : public ::testing::Test {
protected:
    void SetUp() override {
        player_ = std::make_unique<Player>(1, "ChestTestPlayer");
        playerInventory_ = std::make_unique<PlayerInventory>(player_.get());
        // 创建箱子背包容器（27格）
        chestInventory_ = std::make_unique<blockentity::SimpleInventory>(27);
        // 创建双箱背包容器（54格）
        doubleChestInventory_ = std::make_unique<blockentity::SimpleInventory>(54);
    }

    std::unique_ptr<Player> player_;
    std::unique_ptr<PlayerInventory> playerInventory_;
    std::unique_ptr<blockentity::SimpleInventory> chestInventory_;
    std::unique_ptr<blockentity::SimpleInventory> doubleChestInventory_;
};

TEST_F(ChestContainerTest, CreateSingle_HasCorrectSlotCount) {
    // 单箱容器槽位数量 = 箱子槽位 + 玩家背包槽位 = 27 + 36 = 63
    auto container = blockentity::ChestContainer::createSingle(
        ContainerId(1), playerInventory_.get(), chestInventory_.get());
    EXPECT_EQ(container->getSlotCount(), 63);
    EXPECT_EQ(container->getRowCount(), 3);
    EXPECT_EQ(container->getChestSlotCount(), 27);
}

TEST_F(ChestContainerTest, CreateDouble_HasCorrectSlotCount) {
    // 双箱容器槽位数量 = 双箱槽位 + 玩家背包槽位 = 54 + 36 = 90
    auto container = blockentity::ChestContainer::createDouble(
        ContainerId(1), playerInventory_.get(), doubleChestInventory_.get());
    EXPECT_EQ(container->getSlotCount(), 90);
    EXPECT_EQ(container->getRowCount(), 6);
    EXPECT_EQ(container->getChestSlotCount(), 54);
}

TEST_F(ChestContainerTest, GetChestInventory_ReturnsCorrectInventory) {
    auto container = blockentity::ChestContainer::createSingle(
        ContainerId(1), playerInventory_.get(), chestInventory_.get());
    EXPECT_EQ(container->getChestInventory(), chestInventory_.get());
}

TEST_F(ChestContainerTest, ContainerId_IsCorrect) {
    auto container = blockentity::ChestContainer::createSingle(
        ContainerId(42), playerInventory_.get(), chestInventory_.get());
    EXPECT_EQ(container->getId(), ContainerId(42));
}

TEST_F(ChestContainerTest, StillValid_WithoutEntity_ReturnsTrue) {
    // 当没有关联 ChestEntity 时，使用 IInventory::isUsableByPlayer()
    // SimpleInventory 的默认实现返回 true
    auto container = blockentity::ChestContainer::createSingle(
        ContainerId(1), playerInventory_.get(), chestInventory_.get());
    EXPECT_TRUE(container->stillValid(*player_));
}

TEST_F(ChestContainerTest, StillValid_SingleChest_WhenPlayerIsNear_ReturnsTrue) {
    // 创建箱子实体
    auto chestEntity = std::make_unique<blockentity::ChestEntity>(BlockPos(0, 64, 0));

    auto container = blockentity::ChestContainer::createSingle(
        ContainerId(1), playerInventory_.get(), chestEntity->getInventory(), chestEntity.get());

    // 玩家在箱子附近（距离小于8格）
    // 箱子中心在 (0.5, 64.5, 0.5)，玩家在 (0.5, 64.0, 0.5)
    // 距离平方 = 0 + 0.25 + 0 = 0.25 < 64
    player_->setPosition(0.5f, 64.0f, 0.5f);
    EXPECT_TRUE(container->stillValid(*player_));

    // 玩家在箱子8格边缘（距离刚好等于8格）
    // 箱子中心在 (0.5, 64.5, 0.5)，玩家在 (8.5, 64.5, 0.5)
    // 距离平方 = 64 + 0 + 0 = 64 <= 64（边界情况，应该返回true）
    player_->setPosition(8.5f, 64.5f, 0.5f);
    EXPECT_TRUE(container->stillValid(*player_));
}

TEST_F(ChestContainerTest, StillValid_SingleChest_WhenPlayerIsTooFar_ReturnsFalse) {
    // 创建箱子实体
    auto chestEntity = std::make_unique<blockentity::ChestEntity>(BlockPos(0, 64, 0));

    auto container = blockentity::ChestContainer::createSingle(
        ContainerId(1), playerInventory_.get(), chestEntity->getInventory(), chestEntity.get());

    // 玩家距离箱子超过8格
    // 箱子中心在 (0.5, 64.5, 0.5)，玩家在 (12.5, 64.0, 0.5)
    // 距离平方 = 144 + 0.25 + 0 = 144.25 > 64
    player_->setPosition(12.5f, 64.0f, 0.5f);
    EXPECT_FALSE(container->stillValid(*player_));

    // 玩家距离箱子刚好超过8格（8.01格）
    // 箱子中心在 (0.5, 64.5, 0.5)，玩家在 (8.51, 64.5, 0.5)
    // 距离平方 = 64.1601 + 0 + 0 = 64.1601 > 64
    player_->setPosition(8.51f, 64.5f, 0.5f);
    EXPECT_FALSE(container->stillValid(*player_));
}

TEST_F(ChestContainerTest, StillValid_DoubleChest_WhenPlayerNearEitherChest_ReturnsTrue) {
    // 创建双箱实体
    auto chestEntityA = std::make_unique<blockentity::ChestEntity>(BlockPos(0, 64, 0));
    auto chestEntityB = std::make_unique<blockentity::ChestEntity>(BlockPos(1, 64, 0));

    auto container = blockentity::ChestContainer::createDouble(
        ContainerId(1), playerInventory_.get(), doubleChestInventory_.get(),
        chestEntityA.get(), chestEntityB.get());

    // 玩家在第一个箱子附近
    player_->setPosition(0.5f, 64.0f, 0.5f);
    EXPECT_TRUE(container->stillValid(*player_));

    // 玩家在第二个箱子附近
    // 第二个箱子中心在 (1.5, 64.5, 0.5)
    player_->setPosition(1.5f, 64.0f, 0.5f);
    EXPECT_TRUE(container->stillValid(*player_));

    // 玩家在两个箱子中间
    player_->setPosition(1.0f, 64.0f, 0.5f);
    EXPECT_TRUE(container->stillValid(*player_));
}

TEST_F(ChestContainerTest, StillValid_DoubleChest_WhenPlayerTooFarFromBoth_ReturnsFalse) {
    // 创建双箱实体
    auto chestEntityA = std::make_unique<blockentity::ChestEntity>(BlockPos(0, 64, 0));
    auto chestEntityB = std::make_unique<blockentity::ChestEntity>(BlockPos(1, 64, 0));

    auto container = blockentity::ChestContainer::createDouble(
        ContainerId(1), playerInventory_.get(), doubleChestInventory_.get(),
        chestEntityA.get(), chestEntityB.get());

    // 玩家距离两个箱子都超过8格
    player_->setPosition(20.0f, 64.0f, 0.5f);
    EXPECT_FALSE(container->stillValid(*player_));
}
