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

#include "entity/inventory/container/BrewingStandContainer.hpp"
#include "entity/inventory/PlayerInventory.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::blockentity;

// ========== BrewingStandContainer 测试 ==========

class BrewingStandContainerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        playerInventory_ = std::make_unique<PlayerInventory>();
        // 创建酿造台背包容器（5格：3药水 + 材料 + 燃料）
        brewingInventory_ = std::make_unique<SimpleInventory>(BrewingStandContainer::BREWING_SLOTS);
    }

    std::unique_ptr<PlayerInventory> playerInventory_;
    std::unique_ptr<SimpleInventory> brewingInventory_;
};

TEST_F(BrewingStandContainerTest, Create_HasCorrectSlotCount)
{
    // 容器实际槽位数量 = 酿造台槽位 + 玩家背包槽位 = 5 + 36 = 41
    BrewingStandContainer container(ContainerId(1), playerInventory_.get(), brewingInventory_.get());
    EXPECT_EQ(container.getSlotCount(), 41);
}

TEST_F(BrewingStandContainerTest, ContainerType_IsCorrect)
{
    BrewingStandContainer container(ContainerId(1), playerInventory_.get(), brewingInventory_.get());
    EXPECT_EQ(container.getId(), ContainerId(1));
}

TEST_F(BrewingStandContainerTest, SlotIndices_AreCorrect)
{
    EXPECT_EQ(BrewingStandContainer::SLOT_POTION_START, 0);
    EXPECT_EQ(BrewingStandContainer::POTION_SLOTS, 3);
    EXPECT_EQ(BrewingStandContainer::SLOT_INGREDIENT, 3);
    EXPECT_EQ(BrewingStandContainer::SLOT_FUEL, 4);
    EXPECT_EQ(BrewingStandContainer::BREWING_SLOTS, 5);
}

TEST_F(BrewingStandContainerTest, Constants_AreCorrect)
{
    // 验证GUI布局常量存在 - MC 1.16.5坐标
    EXPECT_EQ(BrewingStandContainer::POTION_SLOT_X[0], 56);
    EXPECT_EQ(BrewingStandContainer::POTION_SLOT_X[1], 79);
    EXPECT_EQ(BrewingStandContainer::POTION_SLOT_X[2], 102);
    EXPECT_EQ(BrewingStandContainer::POTION_SLOT_Y[0], 51);
    EXPECT_EQ(BrewingStandContainer::POTION_SLOT_Y[1], 58);
    EXPECT_EQ(BrewingStandContainer::POTION_SLOT_Y[2], 51);
    EXPECT_EQ(BrewingStandContainer::INGREDIENT_SLOT_X, 79);
    EXPECT_EQ(BrewingStandContainer::INGREDIENT_SLOT_Y, 17);
    EXPECT_EQ(BrewingStandContainer::FUEL_SLOT_X, 17);
    EXPECT_EQ(BrewingStandContainer::FUEL_SLOT_Y, 17);
    EXPECT_EQ(BrewingStandContainer::PLAYER_INV_Y, 84);
    EXPECT_EQ(BrewingStandContainer::HOTBAR_Y, 142);
}

TEST_F(BrewingStandContainerTest, GetBrewingStandInventory_ReturnsCorrectInventory)
{
    BrewingStandContainer container(ContainerId(1), playerInventory_.get(), brewingInventory_.get());
    EXPECT_EQ(container.getBrewingStandInventory(), brewingInventory_.get());
}

TEST_F(BrewingStandContainerTest, GetBrewTime_ReturnsZeroInitially)
{
    BrewingStandContainer container(ContainerId(1), playerInventory_.get(), brewingInventory_.get());
    EXPECT_EQ(container.getBrewTime(), 0);
}

TEST_F(BrewingStandContainerTest, GetFuelLevel_ReturnsZeroInitially)
{
    BrewingStandContainer container(ContainerId(1), playerInventory_.get(), brewingInventory_.get());
    EXPECT_EQ(container.getFuelLevel(), 0);
}

TEST_F(BrewingStandContainerTest, SetBrewTime_UpdatesValue)
{
    BrewingStandContainer container(ContainerId(1), playerInventory_.get(), brewingInventory_.get());
    container.setBrewTime(200);
    EXPECT_EQ(container.getBrewTime(), 200);
}

TEST_F(BrewingStandContainerTest, SetFuel_UpdatesValue)
{
    BrewingStandContainer container(ContainerId(1), playerInventory_.get(), brewingInventory_.get());
    container.setFuel(15);
    EXPECT_EQ(container.getFuelLevel(), 15);
}

TEST_F(BrewingStandContainerTest, StillValid_ReturnsTrue)
{
    BrewingStandContainer container(ContainerId(1), playerInventory_.get(), brewingInventory_.get());
    EXPECT_TRUE(container.stillValid(*playerInventory_->getPlayer()));
}
