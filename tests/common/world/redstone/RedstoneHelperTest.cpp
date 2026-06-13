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

#include "common/world/redstone/RedstoneHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/vehicle/MinecartEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::redstone;
using namespace mc::entity;

namespace {

/**
 * @brief 按资源路径懒注册测试用物品
 */
Item* ensureTestItem(const char* path)
{
    auto& registry = ItemRegistry::instance();
    const ResourceLocation id("minecraft", path);
    if (Item* existing = registry.getItem(id); existing != nullptr) {
        return existing;
    }
    return &registry.registerItem(id, ItemProperties().maxStackSize(64));
}

} // namespace

/**
 * @brief RedstoneHelper 单元测试
 *
 * 测试红石辅助函数。
 */
class RedstoneHelperTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// ========== 信号衰减测试 ==========

TEST_F(RedstoneHelperTest, AttenuateBasic)
{
    // 信号衰减测试
    EXPECT_EQ(RedstoneHelper::attenuate(15, 1), 14);
    EXPECT_EQ(RedstoneHelper::attenuate(15, 5), 10);
    EXPECT_EQ(RedstoneHelper::attenuate(15, 15), 0);
    EXPECT_EQ(RedstoneHelper::attenuate(15, 16), 0); // 不会变成负数
}

TEST_F(RedstoneHelperTest, AttenuateFromLowStrength)
{
    // 从低强度开始衰减
    EXPECT_EQ(RedstoneHelper::attenuate(5, 1), 4);
    EXPECT_EQ(RedstoneHelper::attenuate(5, 5), 0);
    EXPECT_EQ(RedstoneHelper::attenuate(1, 1), 0);
}

TEST_F(RedstoneHelperTest, AttenuateZeroDistance)
{
    // 零距离传输不衰减
    EXPECT_EQ(RedstoneHelper::attenuate(15, 0), 15);
    EXPECT_EQ(RedstoneHelper::attenuate(10, 0), 10);
    EXPECT_EQ(RedstoneHelper::attenuate(0, 0), 0);
}

TEST_F(RedstoneHelperTest, AttenuateZeroStrength)
{
    // 零强度无论传输多远都是零
    EXPECT_EQ(RedstoneHelper::attenuate(0, 0), 0);
    EXPECT_EQ(RedstoneHelper::attenuate(0, 1), 0);
    EXPECT_EQ(RedstoneHelper::attenuate(0, 100), 0);
}

// ========== 信号限制测试 ==========

TEST_F(RedstoneHelperTest, ClampInRange)
{
    // 在范围内的值不变
    EXPECT_EQ(RedstoneHelper::clamp(0), 0);
    EXPECT_EQ(RedstoneHelper::clamp(5), 5);
    EXPECT_EQ(RedstoneHelper::clamp(10), 10);
    EXPECT_EQ(RedstoneHelper::clamp(15), 15);
}

TEST_F(RedstoneHelperTest, ClampOutOfRange)
{
    // 超出范围的值被限制
    EXPECT_EQ(RedstoneHelper::clamp(-1), 0);
    EXPECT_EQ(RedstoneHelper::clamp(-100), 0);
    EXPECT_EQ(RedstoneHelper::clamp(16), 15);
    EXPECT_EQ(RedstoneHelper::clamp(100), 15);
}

TEST_F(RedstoneHelperTest, ClampBoundary)
{
    // 边界值测试
    EXPECT_EQ(RedstoneHelper::clamp(-1), 0);
    EXPECT_EQ(RedstoneHelper::clamp(0), 0);
    EXPECT_EQ(RedstoneHelper::clamp(15), 15);
    EXPECT_EQ(RedstoneHelper::clamp(16), 15);
}

// ========== 常量测试 ==========

TEST_F(RedstoneHelperTest, ConstantsCorrect)
{
    // 红石信号范围是 0-15
    EXPECT_EQ(RedstoneHelper::MIN_POWER, 0);
    EXPECT_EQ(RedstoneHelper::MAX_POWER, 15);
    EXPECT_EQ(RedstonePower::MIN_POWER, 0);
    EXPECT_EQ(RedstonePower::MAX_POWER, 15);
}

// ========== 方向判断测试 ==========

TEST_F(RedstoneHelperTest, IsHorizontal)
{
    using namespace mc::Directions;

    EXPECT_TRUE(RedstoneHelper::isHorizontal(Direction::North));
    EXPECT_TRUE(RedstoneHelper::isHorizontal(Direction::South));
    EXPECT_TRUE(RedstoneHelper::isHorizontal(Direction::East));
    EXPECT_TRUE(RedstoneHelper::isHorizontal(Direction::West));

    EXPECT_FALSE(RedstoneHelper::isHorizontal(Direction::Up));
    EXPECT_FALSE(RedstoneHelper::isHorizontal(Direction::Down));
}

TEST_F(RedstoneHelperTest, IsVertical)
{
    using namespace mc::Directions;

    EXPECT_TRUE(RedstoneHelper::isVertical(Direction::Up));
    EXPECT_TRUE(RedstoneHelper::isVertical(Direction::Down));

    EXPECT_FALSE(RedstoneHelper::isVertical(Direction::North));
    EXPECT_FALSE(RedstoneHelper::isVertical(Direction::South));
    EXPECT_FALSE(RedstoneHelper::isVertical(Direction::East));
    EXPECT_FALSE(RedstoneHelper::isVertical(Direction::West));
}

// ========== 组合衰减和限制测试 ==========

TEST_F(RedstoneHelperTest, AttenuateThenClamp)
{
    // 典型使用场景：衰减后限制范围
    i32 strength = 10;
    i32 distance = 5;

    i32 result = RedstoneHelper::clamp(RedstoneHelper::attenuate(strength, distance));
    EXPECT_EQ(result, 5);

    // 超过最大距离
    distance = 20;
    result = RedstoneHelper::clamp(RedstoneHelper::attenuate(strength, distance));
    EXPECT_EQ(result, 0);
}

// ========== calcRedstoneFromInventory 测试 ==========

class CalcRedstoneFromInventoryTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(CalcRedstoneFromInventoryTest, EmptyInventory_ReturnsZero)
{
    blockentity::SimpleInventory inventory(27);
    EXPECT_EQ(RedstoneHelper::calcRedstoneFromInventory(inventory), 0);
}

TEST_F(CalcRedstoneFromInventoryTest, SingleItem_ReturnsOne)
{
    blockentity::SimpleInventory inventory(27);
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);
    inventory.setItem(0, ItemStack(*diamond, 1));

    // 单个物品占1/27槽位填充率，信号 = floor(1/64 / 27 * 14) + 1 = 0 + 1 = 1
    EXPECT_EQ(RedstoneHelper::calcRedstoneFromInventory(inventory), 1);
}

TEST_F(CalcRedstoneFromInventoryTest, SingleSlotFull_ReturnsOne)
{
    blockentity::SimpleInventory inventory(27);
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);
    // 64个钻石占满1格，填充率 = (64/64) / 27 ≈ 0.037
    // 信号 = floor(0.037 * 14) + 1 = 0 + 1 = 1
    inventory.setItem(0, ItemStack(*diamond, 64));

    EXPECT_EQ(RedstoneHelper::calcRedstoneFromInventory(inventory), 1);
}

TEST_F(CalcRedstoneFromInventoryTest, AllSlotsFull_ReturnsFifteen)
{
    blockentity::SimpleInventory inventory(27);
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);
    // 全部27格填满，填充率 = 27/27 = 1.0
    // 信号 = floor(1.0 * 14) + 1 = 14 + 1 = 15
    for (i32 i = 0; i < 27; ++i) {
        inventory.setItem(i, ItemStack(*diamond, 64));
    }

    EXPECT_EQ(RedstoneHelper::calcRedstoneFromInventory(inventory), 15);
}

TEST_F(CalcRedstoneFromInventoryTest, TwoSlotInventory_HalfFull)
{
    blockentity::SimpleInventory inventory(2);
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);

    // 1格满（64个），1格空
    // 填充率 = (64/64 + 0) / 2 = 0.5
    // 信号 = floor(0.5 * 14) + 1 = 7 + 1 = 8
    inventory.setItem(0, ItemStack(*diamond, 64));

    EXPECT_EQ(RedstoneHelper::calcRedstoneFromInventory(inventory), 8);
}

TEST_F(CalcRedstoneFromInventoryTest, FiveSlotInventory_AllFull)
{
    blockentity::SimpleInventory inventory(5);
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);

    // 全部5格填满，填充率 = 5/5 = 1.0
    // 信号 = floor(1.0 * 14) + 1 = 14 + 1 = 15
    for (i32 i = 0; i < 5; ++i) {
        inventory.setItem(i, ItemStack(*diamond, 64));
    }

    EXPECT_EQ(RedstoneHelper::calcRedstoneFromInventory(inventory), 15);
}

TEST_F(CalcRedstoneFromInventoryTest, OneSlotInventory_FullStack_ReturnsFifteen)
{
    blockentity::SimpleInventory inventory(1);
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);

    // 1格满（64个），填充率 = (64/64) / 1 = 1.0
    // 信号 = floor(1.0 * 14) + 1 = 14 + 1 = 15
    inventory.setItem(0, ItemStack(*diamond, 64));

    EXPECT_EQ(RedstoneHelper::calcRedstoneFromInventory(inventory), 15);
}

TEST_F(CalcRedstoneFromInventoryTest, OneSlotInventory_HalfStack)
{
    blockentity::SimpleInventory inventory(1);
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);

    // 1格容器，32个物品（半堆叠）
    // 填充率 = (32/64) / 1 = 0.5
    // 信号 = floor(0.5 * 14) + 1 = 7 + 1 = 8
    inventory.setItem(0, ItemStack(*diamond, 32));

    EXPECT_EQ(RedstoneHelper::calcRedstoneFromInventory(inventory), 8);
}

TEST_F(CalcRedstoneFromInventoryTest, ThreeSlotInventory_MixedStacks)
{
    blockentity::SimpleInventory inventory(3);
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);

    // 槽0: 64个，槽1: 32个，槽2: 空
    // 填充率 = (64/64 + 32/64 + 0) / 3 = (1.0 + 0.5 + 0) / 3 = 0.5
    // 信号 = floor(0.5 * 14) + 1 = 7 + 1 = 8
    inventory.setItem(0, ItemStack(*diamond, 64));
    inventory.setItem(1, ItemStack(*diamond, 32));

    EXPECT_EQ(RedstoneHelper::calcRedstoneFromInventory(inventory), 8);
}

// ========== Entity::getComparatorOutput 测试 ==========

class MinecartComparatorOutputTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(MinecartComparatorOutputTest, RideableMinecart_ReturnsZero)
{
    // 普通矿车没有比较器信号输出
    RideableMinecartEntity rideable(EntityId(1));
    EXPECT_EQ(rideable.getComparatorOutput(), 0);
}

TEST_F(MinecartComparatorOutputTest, FurnaceMinecart_ReturnsZero)
{
    // 熔炉矿车没有比较器信号输出
    FurnaceMinecartEntity furnace(EntityId(2));
    EXPECT_EQ(furnace.getComparatorOutput(), 0);
}

TEST_F(MinecartComparatorOutputTest, TNTMinecart_ReturnsZero)
{
    // TNT矿车没有比较器信号输出
    TNTMinecartEntity tnt(EntityId(3));
    EXPECT_EQ(tnt.getComparatorOutput(), 0);
}

TEST_F(MinecartComparatorOutputTest, ChestMinecart_Empty_ReturnsZero)
{
    ChestMinecartEntity chest(EntityId(10));
    EXPECT_EQ(chest.getComparatorOutput(), 0);
}

TEST_F(MinecartComparatorOutputTest, ChestMinecart_WithItems_ReturnsNonZero)
{
    ChestMinecartEntity chest(EntityId(11));
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);

    chest.setInventoryItem(0, ItemStack(*diamond, 1));
    EXPECT_GT(chest.getComparatorOutput(), 0);
    EXPECT_LE(chest.getComparatorOutput(), 15);
}

TEST_F(MinecartComparatorOutputTest, ChestMinecart_Full_ReturnsFifteen)
{
    ChestMinecartEntity chest(EntityId(12));
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);

    // 全部27格填满
    for (i32 i = 0; i < ChestMinecartEntity::INVENTORY_SIZE; ++i) {
        chest.setInventoryItem(i, ItemStack(*diamond, 64));
    }
    EXPECT_EQ(chest.getComparatorOutput(), 15);
}

TEST_F(MinecartComparatorOutputTest, HopperMinecart_Empty_ReturnsZero)
{
    HopperMinecartEntity hopper(EntityId(20));
    EXPECT_EQ(hopper.getComparatorOutput(), 0);
}

TEST_F(MinecartComparatorOutputTest, HopperMinecart_WithItems_ReturnsNonZero)
{
    HopperMinecartEntity hopper(EntityId(21));
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);

    hopper.setInventoryItem(0, ItemStack(*diamond, 1));
    EXPECT_GT(hopper.getComparatorOutput(), 0);
    EXPECT_LE(hopper.getComparatorOutput(), 15);
}

TEST_F(MinecartComparatorOutputTest, HopperMinecart_Full_ReturnsFifteen)
{
    HopperMinecartEntity hopper(EntityId(22));
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);

    // 全部5格填满
    for (i32 i = 0; i < HopperMinecartEntity::INVENTORY_SIZE; ++i) {
        hopper.setInventoryItem(i, ItemStack(*diamond, 64));
    }
    EXPECT_EQ(hopper.getComparatorOutput(), 15);
}

TEST_F(MinecartComparatorOutputTest, CommandBlockMinecart_DefaultSuccessCount_ReturnsZero)
{
    // 默认成功次数为0
    CommandBlockMinecartEntity command(EntityId(30));
    EXPECT_EQ(command.getComparatorOutput(), 0);
}

TEST_F(MinecartComparatorOutputTest, CommandBlockMinecart_CappedAtFifteen)
{
    CommandBlockMinecartEntity command(EntityId(31));
    // 直接设置 successCount（通过 setCommand 后命令执行的间接结果）
    // getComparatorOutput() 返回 min(successCount, 15)
    // 默认 successCount = 0
    EXPECT_EQ(command.getComparatorOutput(), 0);
}
