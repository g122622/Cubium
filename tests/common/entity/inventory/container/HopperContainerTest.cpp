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

#include "entity/inventory/container/HopperContainer.hpp"
#include "entity/entities/player/Player.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include "world/blockentity/transport/HopperEntity.hpp"
#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"

using namespace mc;

// ========== HopperContainer 测试 ==========

class HopperContainerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        player_ = std::make_unique<Player>(1, "HopperTestPlayer", mc::test::testEcsRegistry());
        playerInventory_ = std::make_unique<PlayerInventory>(player_.get());
        // 创建漏斗背包容器（5格）
        hopperInventory_ = std::make_unique<blockentity::SimpleInventory>(HopperContainer::HOPPER_SIZE);
    }

    std::unique_ptr<Player> player_;
    std::unique_ptr<PlayerInventory> playerInventory_;
    std::unique_ptr<blockentity::SimpleInventory> hopperInventory_;
};

TEST_F(HopperContainerTest, Create_HasCorrectSlotCount)
{
    // 容器实际槽位数量 = 漏斗槽位 + 玩家背包槽位 = 5 + 36 = 41
    HopperContainer container(ContainerId(1), playerInventory_.get(), hopperInventory_.get());
    EXPECT_EQ(container.getSlotCount(), 41);
}

TEST_F(HopperContainerTest, GetHopperInventory_ReturnsCorrectInventory)
{
    HopperContainer container(ContainerId(1), playerInventory_.get(), hopperInventory_.get());
    EXPECT_EQ(container.getHopperInventory(), hopperInventory_.get());
}

TEST_F(HopperContainerTest, ContainerId_IsCorrect)
{
    HopperContainer container(ContainerId(1), playerInventory_.get(), hopperInventory_.get());
    EXPECT_EQ(container.getId(), ContainerId(1));
}

TEST_F(HopperContainerTest, HopperSize_IsFive)
{
    EXPECT_EQ(HopperContainer::HOPPER_SIZE, 5);
}

TEST_F(HopperContainerTest, Constants_AreCorrect_MC1165)
{
    // 验证GUI布局常量 - MC 1.16.5坐标
    EXPECT_EQ(HopperContainer::HOPPER_SLOT_START_X, 44);
    EXPECT_EQ(HopperContainer::HOPPER_SLOT_Y, 20);
    EXPECT_EQ(HopperContainer::PLAYER_INV_Y, 51);
    EXPECT_EQ(HopperContainer::HOTBAR_Y, 109);
    EXPECT_EQ(HopperContainer::SLOT_SIZE, 18);
}

TEST_F(HopperContainerTest, StillValid_WithoutEntity_ReturnsTrue)
{
    // 当没有关联 HopperEntity 时，使用 IInventory::isUsableByPlayer()
    // SimpleInventory 的默认实现返回 true
    HopperContainer container(ContainerId(1), playerInventory_.get(), hopperInventory_.get());
    EXPECT_TRUE(container.stillValid(*player_));
}

TEST_F(HopperContainerTest, StillValid_WithEntity_WhenPlayerIsNearHopper_ReturnsTrue)
{
    // 创建漏斗实体
    auto hopperEntity = std::make_unique<blockentity::HopperEntity>(BlockPos(0, 64, 0));

    HopperContainer container(ContainerId(1), playerInventory_.get(), hopperEntity->getInventory(), hopperEntity.get());

    // 玩家在漏斗附近（距离小于8格）
    // 漏斗中心在 (0.5, 64.5, 0.5)，玩家在 (0.5, 64.0, 0.5)
    // 距离平方 = 0 + 0.25 + 0 = 0.25 < 64
    player_->setPosition(0.5f, 64.0f, 0.5f);
    EXPECT_TRUE(container.stillValid(*player_));

    // 玩家在漏斗8格边缘（距离刚好等于8格）
    // 漏斗中心在 (0.5, 64.5, 0.5)，玩家在 (8.5, 64.5, 0.5)
    // 距离平方 = 64 + 0 + 0 = 64 <= 64（边界情况，应该返回true）
    player_->setPosition(8.5f, 64.5f, 0.5f);
    EXPECT_TRUE(container.stillValid(*player_));
}

TEST_F(HopperContainerTest, StillValid_WithEntity_WhenPlayerIsTooFar_ReturnsFalse)
{
    // 创建漏斗实体
    auto hopperEntity = std::make_unique<blockentity::HopperEntity>(BlockPos(0, 64, 0));

    HopperContainer container(ContainerId(1), playerInventory_.get(), hopperEntity->getInventory(), hopperEntity.get());

    // 玩家距离漏斗超过8格
    // 漏斗中心在 (0.5, 64.5, 0.5)，玩家在 (12.5, 64.0, 0.5)
    // 距离平方 = 144 + 0.25 + 0 = 144.25 > 64
    player_->setPosition(12.5f, 64.0f, 0.5f);
    EXPECT_FALSE(container.stillValid(*player_));

    // 玩家距离漏斗刚好超过8格（8.01格）
    // 漏斗中心在 (0.5, 64.5, 0.5)，玩家在 (8.51, 64.5, 0.5)
    // 距离平方 = 64.1601 + 0 + 0 = 64.1601 > 64
    player_->setPosition(8.51f, 64.5f, 0.5f);
    EXPECT_FALSE(container.stillValid(*player_));
}
