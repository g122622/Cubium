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

#include "entity/inventory/container/LoomContainer.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"

using namespace mc;
using namespace mc::entity::inventory::container;

// ========== LoomContainer 测试 ==========

class LoomContainerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        player_ = std::make_unique<Player>(1, "LoomTestPlayer", mc::test::testEcsRegistry());
        playerInventory_ = std::make_unique<PlayerInventory>(player_.get());
    }

    std::unique_ptr<Player> player_;
    std::unique_ptr<PlayerInventory> playerInventory_;
};

TEST_F(LoomContainerTest, ClientConstruction)
{
    // 客户端构造函数不应崩溃
    LoomContainer container(0, playerInventory_.get(), BlockPos(0, 0, 0));
    EXPECT_EQ(container.getId(), 0);
}

TEST_F(LoomContainerTest, SlotConstants)
{
    EXPECT_EQ(LoomContainer::SLOT_BANNER, 0);
    EXPECT_EQ(LoomContainer::SLOT_DYE, 1);
    EXPECT_EQ(LoomContainer::SLOT_PATTERN, 2);
    EXPECT_EQ(LoomContainer::SLOT_RESULT, 3);
    EXPECT_EQ(LoomContainer::LOOM_SLOTS, 4);
}

TEST_F(LoomContainerTest, SlotCoordinates)
{
    // 验证GUI坐标常量合理
    EXPECT_GT(LoomContainer::BANNER_SLOT_X, 0);
    EXPECT_GT(LoomContainer::BANNER_SLOT_Y, 0);
    EXPECT_GT(LoomContainer::DYE_SLOT_X, 0);
    EXPECT_GT(LoomContainer::DYE_SLOT_Y, 0);
    EXPECT_GT(LoomContainer::RESULT_SLOT_X, 0);
    EXPECT_GT(LoomContainer::RESULT_SLOT_Y, 0);
}

TEST_F(LoomContainerTest, StillValid)
{
    LoomContainer container(0, playerInventory_.get(), BlockPos(0, 0, 0));
    // isWithinDistance检查玩家与方块距离，玩家在原点附近应返回true
    EXPECT_TRUE(container.stillValid(*player_));
}

TEST_F(LoomContainerTest, GetSelectedPatternDefault)
{
    LoomContainer container(0, playerInventory_.get(), BlockPos(0, 0, 0));
    // 默认选中图案为0
    EXPECT_EQ(container.getSelectedPattern(), 0);
}

TEST_F(LoomContainerTest, ClickMenuButtonInvalidPattern)
{
    LoomContainer container(0, playerInventory_.get(), BlockPos(0, 0, 0));
    // 索引0无效
    EXPECT_FALSE(container.clickMenuButton(*player_, 0));
    // 超出范围无效
    EXPECT_FALSE(container.clickMenuButton(*player_, 100));
}

TEST_F(LoomContainerTest, PatternItemIndexConstant)
{
    // PATTERN_ITEM_INDEX应该小于PATTERN_COUNT
    EXPECT_LT(LoomContainer::PATTERN_ITEM_INDEX, LoomContainer::PATTERN_COUNT);
    // 应该有6种需要图案物品的特殊图案
    EXPECT_EQ(LoomContainer::PATTERNS_WITH_ITEMS, 6);
}

// ========== LoomBannerSlot 测试 ==========

TEST(LoomSlotTest, BannerSlotRejectsEmpty)
{
    // 空物品不可放入
    LoomBannerSlot slot(nullptr, 0, 0, 0);
    ItemStack emptyStack;
    EXPECT_FALSE(slot.mayPlace(emptyStack));
}

// ========== LoomPatternSlot 测试 ==========

TEST(LoomSlotTest, PatternSlotRejectsEmpty)
{
    LoomPatternSlot slot(nullptr, 0, 0, 0);
    ItemStack emptyStack;
    EXPECT_FALSE(slot.mayPlace(emptyStack));
}

// ========== LoomResultSlot 测试 ==========

TEST_F(LoomContainerTest, ResultSlotRejectsPlacement)
{
    LoomContainer container(0, playerInventory_.get(), BlockPos(0, 0, 0));
    LoomResultSlot slot(nullptr, 0, 0, 0, &container);
    // 输出槽禁止放入任何物品
    EXPECT_FALSE(slot.mayPlace(ItemStack()));
}
