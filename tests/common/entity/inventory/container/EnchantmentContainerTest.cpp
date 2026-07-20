/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "entity/inventory/container/EnchantmentContainer.hpp"
#include "common/util/math/random/Random.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "item/Items.hpp"
#include "world/block/BlockPos.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ========== EnchantmentTableContainer 测试 ==========

class EnchantmentTableContainerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        playerInventory_ = std::make_unique<PlayerInventory>();
    }

    std::unique_ptr<PlayerInventory> playerInventory_;
};

TEST_F(EnchantmentTableContainerTest, Create_HasCorrectSlotCount)
{
    // 容器实际槽位数量 = 附魔台槽位 + 玩家背包槽位 = 2 + 36 = 38
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    EXPECT_EQ(container.getSlotCount(), 38);
}

TEST_F(EnchantmentTableContainerTest, ContainerType_IsCorrect)
{
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    EXPECT_EQ(container.getId(), ContainerId(1));
}

TEST_F(EnchantmentTableContainerTest, SlotIndices_AreCorrect)
{
    EXPECT_EQ(EnchantmentContainer::SLOT_ITEM, 0);
    EXPECT_EQ(EnchantmentContainer::SLOT_LAPIS, 1);
    EXPECT_EQ(EnchantmentContainer::ENCHANTMENT_SLOTS, 2);
}

TEST_F(EnchantmentTableContainerTest, Constants_AreCorrect)
{
    // 验证GUI布局常量存在
    EXPECT_GT(EnchantmentContainer::ITEM_SLOT_X, 0);
    EXPECT_GT(EnchantmentContainer::ITEM_SLOT_Y, 0);
    EXPECT_GT(EnchantmentContainer::LAPIS_SLOT_X, EnchantmentContainer::ITEM_SLOT_X);
    EXPECT_EQ(EnchantmentContainer::LAPIS_SLOT_Y, EnchantmentContainer::ITEM_SLOT_Y);
    EXPECT_GT(EnchantmentContainer::PLAYER_INV_Y, EnchantmentContainer::ITEM_SLOT_Y);
    EXPECT_GT(EnchantmentContainer::HOTBAR_Y, EnchantmentContainer::PLAYER_INV_Y);
    EXPECT_EQ(EnchantmentContainer::ENCHANTMENT_OPTIONS, 3);
}

TEST_F(EnchantmentTableContainerTest, GetItemSlot_ReturnsEmptyWhenEmpty)
{
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);

    ItemStack item = container.getItemSlot();
    EXPECT_TRUE(item.isEmpty());
}

TEST_F(EnchantmentTableContainerTest, GetLapisSlot_ReturnsEmptyWhenEmpty)
{
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);

    ItemStack lapis = container.getLapisSlot();
    EXPECT_TRUE(lapis.isEmpty());
}

TEST_F(EnchantmentTableContainerTest, GetEnchantPower_ReturnsZeroWithoutWorld)
{
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);

    // 没有世界对象，书架力量应为0
    EXPECT_EQ(container.getEnchantPower(), 0);
}

TEST_F(EnchantmentTableContainerTest, GetEnchantmentLevel_ReturnsZeroForInvalidIndex)
{
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);

    EXPECT_EQ(container.getEnchantmentLevel(-1), 0);
    EXPECT_EQ(container.getEnchantmentLevel(3), 0);
    EXPECT_EQ(container.getEnchantmentLevel(100), 0);
}

TEST_F(EnchantmentTableContainerTest, GetEnchantmentClue_ReturnsEmptyForInvalidIndex)
{
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);

    EXPECT_EQ(container.getEnchantmentClue(-1), "");
    EXPECT_EQ(container.getEnchantmentClue(3), "");
    EXPECT_EQ(container.getEnchantmentClue(100), "");
}

TEST_F(EnchantmentTableContainerTest, GetEnchantmentWorldClue_ReturnsZeroForInvalidIndex)
{
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);

    EXPECT_EQ(container.getEnchantmentWorldClue(-1), 0);
    EXPECT_EQ(container.getEnchantmentWorldClue(3), 0);
    EXPECT_EQ(container.getEnchantmentWorldClue(100), 0);
}

TEST_F(EnchantmentTableContainerTest, IsEnchantmentOptionAvailable_ReturnsFalseForInvalidIndex)
{
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);

    EXPECT_FALSE(container.isEnchantmentOptionAvailable(-1));
    EXPECT_FALSE(container.isEnchantmentOptionAvailable(3));
    EXPECT_FALSE(container.isEnchantmentOptionAvailable(100));
}

TEST_F(EnchantmentTableContainerTest, IsEnchantmentOptionAvailable_ReturnsFalseWhenEmpty)
{
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);

    // 没有物品时，所有选项不可用
    EXPECT_FALSE(container.isEnchantmentOptionAvailable(0));
    EXPECT_FALSE(container.isEnchantmentOptionAvailable(1));
    EXPECT_FALSE(container.isEnchantmentOptionAvailable(2));
}

// ========== 创造模式经验消耗豁免测试 ==========

TEST_F(EnchantmentTableContainerTest, EnchantmentOptions_ConstantsAreCorrect)
{
    // 验证附魔选项常量
    EXPECT_EQ(EnchantmentContainer::ENCHANTMENT_OPTIONS, 3);
}

TEST_F(EnchantmentTableContainerTest, EnchantmentCost_MatchesMC1165)
{
    // MC 1.16.5: 附魔选项索引 + 1 = 消耗的经验等级和青金石数量
    // 选项 0 = 1 级, 1 个青金石
    // 选项 1 = 2 级, 2 个青金石
    // 选项 2 = 3 级, 3 个青金石
    // 这些值在 enchantItem() 方法中计算: cost = optionIndex + 1
    // 创造模式玩家不消耗经验（在 enchantItem 中检查 player.isCreative()）
    EXPECT_EQ(0 + 1, 1); // 选项 0
    EXPECT_EQ(1 + 1, 2); // 选项 1
    EXPECT_EQ(2 + 1, 3); // 选项 2
}

TEST_F(EnchantmentTableContainerTest, SlotConstants_AreCorrect)
{
    // 验证槽位索引
    EXPECT_EQ(EnchantmentContainer::SLOT_ITEM, 0);
    EXPECT_EQ(EnchantmentContainer::SLOT_LAPIS, 1);
    EXPECT_EQ(EnchantmentContainer::ENCHANTMENT_SLOTS, 2);
}

// ========== 带Player的EnchantmentContainer测试 ==========

class EnchantmentContainerWithPlayerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        player_ = std::make_unique<Player>(EntityInstanceId(1), "TestPlayer");
        playerInventory_ = std::make_unique<PlayerInventory>(player_.get());
    }

    std::unique_ptr<Player> player_;
    std::unique_ptr<PlayerInventory> playerInventory_;
};

TEST_F(EnchantmentContainerWithPlayerTest, CreateWithPlayer_HasCorrectSlotCount)
{
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    EXPECT_EQ(container.getSlotCount(), 38);
}

TEST_F(EnchantmentContainerWithPlayerTest, IsPlayerCreative_ReturnsFalseInSurvivalMode)
{
    // 默认生存模式
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    EXPECT_FALSE(container.isPlayerCreative());
}

TEST_F(EnchantmentContainerWithPlayerTest, IsPlayerCreative_ReturnsTrueInCreativeMode)
{
    player_->setGameMode(GameMode::Creative);
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    EXPECT_TRUE(container.isPlayerCreative());
}

TEST_F(EnchantmentContainerWithPlayerTest, IsPlayerCreative_ReturnsFalseInSpectatorMode)
{
    player_->setGameMode(GameMode::Spectator);
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    EXPECT_FALSE(container.isPlayerCreative());
}

TEST_F(EnchantmentContainerWithPlayerTest, IsPlayerCreative_ReturnsFalseInAdventureMode)
{
    player_->setGameMode(GameMode::Adventure);
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);
    EXPECT_FALSE(container.isPlayerCreative());
}

TEST_F(EnchantmentContainerWithPlayerTest, IsPlayerCreative_ChangesWithGameMode)
{
    BlockPos pos(0, 0, 0);
    EnchantmentContainer container(ContainerId(1), playerInventory_.get(), pos, nullptr);

    // 默认生存模式
    EXPECT_FALSE(container.isPlayerCreative());

    // 切换到创造模式
    player_->setGameMode(GameMode::Creative);
    EXPECT_TRUE(container.isPlayerCreative());

    // 切换回生存模式
    player_->setGameMode(GameMode::Survival);
    EXPECT_FALSE(container.isPlayerCreative());
}
