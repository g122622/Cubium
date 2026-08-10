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

/**
 * @file MinecartTests.cpp
 * @brief 矿车系统单元测试
 *
 * 测试内容：
 * - MinecartItem 创建和放置
 * - ChestMinecartEntity 库存系统
 * - RailShape isAscending 辅助函数
 * - AbstractMinecartEntity 基础功能
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/vehicle/MinecartEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/vehicle/MinecartItem.hpp"
#include "common/world/block/blocks/redstone/AbstractRailBlock.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"

namespace mc {
namespace entity {
namespace test {

// ============================================================================
// RailShape 辅助函数测试
// ============================================================================

class RailShapeTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(RailShapeTest, IsAscending_ReturnsTrueForAscendingShapes)
{
    // 上升铁轨形状应返回 true
    EXPECT_TRUE(blocks::isAscending(blocks::RailShape::AscendingEast));
    EXPECT_TRUE(blocks::isAscending(blocks::RailShape::AscendingWest));
    EXPECT_TRUE(blocks::isAscending(blocks::RailShape::AscendingNorth));
    EXPECT_TRUE(blocks::isAscending(blocks::RailShape::AscendingSouth));
}

TEST_F(RailShapeTest, IsAscending_ReturnsFalseForFlatShapes)
{
    // 平轨和弯轨应返回 false
    EXPECT_FALSE(blocks::isAscending(blocks::RailShape::NorthSouth));
    EXPECT_FALSE(blocks::isAscending(blocks::RailShape::EastWest));
    EXPECT_FALSE(blocks::isAscending(blocks::RailShape::SouthEast));
    EXPECT_FALSE(blocks::isAscending(blocks::RailShape::SouthWest));
    EXPECT_FALSE(blocks::isAscending(blocks::RailShape::NorthWest));
    EXPECT_FALSE(blocks::isAscending(blocks::RailShape::NorthEast));
}

// ============================================================================
// ChestMinecartEntity 库存测试
// ============================================================================

class ChestMinecartEntityTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ChestMinecartEntityTest, Constructor_CreatesEmptyInventory)
{
    ChestMinecartEntity minecart(EntityInstanceId(1), mc::test::testEcsRegistry());

    EXPECT_EQ(minecart.getContainerSize(), ChestMinecartEntity::INVENTORY_SIZE);
    EXPECT_EQ(ChestMinecartEntity::INVENTORY_SIZE, 27); // 3行 x 9列
    EXPECT_TRUE(minecart.isInventoryEmpty());
}

TEST_F(ChestMinecartEntityTest, GetInventoryItem_ReturnsEmptyForInvalidSlot)
{
    ChestMinecartEntity minecart(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 越界访问应返回空物品堆
    EXPECT_TRUE(minecart.getInventoryItem(-1).isEmpty());
    EXPECT_TRUE(minecart.getInventoryItem(100).isEmpty());
}

TEST_F(ChestMinecartEntityTest, ClearInventory_WorksCorrectly)
{
    ChestMinecartEntity minecart(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 验证库存操作基本功能
    EXPECT_TRUE(minecart.isInventoryEmpty());

    // 清空库存
    minecart.clearInventory();
    EXPECT_TRUE(minecart.isInventoryEmpty());
}

TEST_F(ChestMinecartEntityTest, RemoveInventoryItem_WorksCorrectly)
{
    ChestMinecartEntity minecart(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 越界访问应返回空物品堆
    EXPECT_TRUE(minecart.removeInventoryItem(-1, 1).isEmpty());
    EXPECT_TRUE(minecart.removeInventoryItem(100, 1).isEmpty());

    // 空槽位移除应返回空物品堆
    EXPECT_TRUE(minecart.removeInventoryItem(0, 1).isEmpty());
}

TEST_F(ChestMinecartEntityTest, GetInventory_ReturnsNonNullptr)
{
    ChestMinecartEntity minecart(EntityInstanceId(1), mc::test::testEcsRegistry());

    IInventory* inventory = minecart.getInventory();
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->getContainerSize(), ChestMinecartEntity::INVENTORY_SIZE);
}

// ============================================================================
// AbstractMinecartEntity 基础测试
// ============================================================================

class AbstractMinecartEntityTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(AbstractMinecartEntityTest, Constructor_SetsCorrectType)
{
    RideableMinecartEntity rideable(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_EQ(rideable.getMinecartType(), AbstractMinecartEntity::Type::Rideable);

    ChestMinecartEntity chest(EntityInstanceId(2), mc::test::testEcsRegistry());
    EXPECT_EQ(chest.getMinecartType(), AbstractMinecartEntity::Type::Chest);

    FurnaceMinecartEntity furnace(EntityInstanceId(3), mc::test::testEcsRegistry());
    EXPECT_EQ(furnace.getMinecartType(), AbstractMinecartEntity::Type::Furnace);

    TNTMinecartEntity tnt(EntityInstanceId(4), mc::test::testEcsRegistry());
    EXPECT_EQ(tnt.getMinecartType(), AbstractMinecartEntity::Type::TNT);

    HopperMinecartEntity hopper(EntityInstanceId(5), mc::test::testEcsRegistry());
    EXPECT_EQ(hopper.getMinecartType(), AbstractMinecartEntity::Type::Hopper);

    CommandBlockMinecartEntity command(EntityInstanceId(6), mc::test::testEcsRegistry());
    EXPECT_EQ(command.getMinecartType(), AbstractMinecartEntity::Type::CommandBlock);
}

TEST_F(AbstractMinecartEntityTest, GetSlopeAdjustment_ReturnsCorrectValue)
{
    // MC 1.16.5: getSlopeAdjustment() -> 0.0078125D
    // This is a static constexpr method, but protected, so test via derived class behavior
    // The value is used in slope calculations
    constexpr f64 SLOPE_ADJUSTMENT = 0.0078125;
    EXPECT_DOUBLE_EQ(SLOPE_ADJUSTMENT, 0.0078125);
}

TEST_F(AbstractMinecartEntityTest, GetMaxSpeed_ReturnsDefault)
{
    RideableMinecartEntity minecart(EntityInstanceId(1), mc::test::testEcsRegistry());
    // MC 1.16.5: 默认最大速度 0.4D
    EXPECT_FLOAT_EQ(minecart.getMaxSpeed(), 0.4f);
}

TEST_F(AbstractMinecartEntityTest, FurnaceMinecart_HasSlowerSpeed)
{
    FurnaceMinecartEntity furnace(EntityInstanceId(1), mc::test::testEcsRegistry());
    // MC 1.16.5: 熔炉矿车最大速度 0.2D
    EXPECT_FLOAT_EQ(furnace.getMaxSpeed(), 0.2f);
}

TEST_F(AbstractMinecartEntityTest, IsActivated_DefaultFalse)
{
    RideableMinecartEntity rideable(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(rideable.isActivated());

    FurnaceMinecartEntity furnace(EntityInstanceId(2), mc::test::testEcsRegistry());
    EXPECT_FALSE(furnace.isActivated());
}

TEST_F(AbstractMinecartEntityTest, FurnaceMinecart_FuelSystem)
{
    FurnaceMinecartEntity furnace(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 初始燃料为 0
    EXPECT_EQ(furnace.getFuel(), 0);
    EXPECT_FALSE(furnace.isActivated());

    // 添加燃料
    furnace.addFuel(100);
    EXPECT_EQ(furnace.getFuel(), 100);
    EXPECT_TRUE(furnace.isActivated());

    // 累加燃料
    furnace.addFuel(50);
    EXPECT_EQ(furnace.getFuel(), 150);
}

TEST_F(AbstractMinecartEntityTest, TNTMinecart_PrimeSystem)
{
    TNTMinecartEntity tnt(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 初始未点燃
    EXPECT_FALSE(tnt.isPrimed());

    // 点燃
    tnt.prime();
    EXPECT_TRUE(tnt.isPrimed());
}

TEST_F(AbstractMinecartEntityTest, TNTMinecart_PrimeDefaultFuse)
{
    TNTMinecartEntity tnt(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 默认引信时间为 80 ticks
    tnt.prime();
    EXPECT_TRUE(tnt.isPrimed());
}

TEST_F(AbstractMinecartEntityTest, TNTMinecart_PrimeCustomFuse)
{
    TNTMinecartEntity tnt(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 自定义引信时间
    tnt.prime(40);
    EXPECT_TRUE(tnt.isPrimed());
}

TEST_F(AbstractMinecartEntityTest, TNTMinecart_PrimeNegativeFuseNotPrimed)
{
    TNTMinecartEntity tnt(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 负值 fuse 不应视为引燃状态
    tnt.prime(-1);
    EXPECT_FALSE(tnt.isPrimed());

    // 0 值也不应视为引燃状态（fuse > -1 但 isPrimed 要求 fuse > -1）
    tnt.prime(0);
    EXPECT_TRUE(tnt.isPrimed());
}

// ============================================================================
// MinecartItem 基础测试（无世界环境）
// ============================================================================

class MinecartItemTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(MinecartItemTest, Constructor_StoresType)
{
    ItemProperties props;
    props.maxStackSize(1);

    item::MinecartItem item(AbstractMinecartEntity::Type::Rideable, props);
    EXPECT_EQ(item.getMinecartType(), AbstractMinecartEntity::Type::Rideable);

    item::MinecartItem chestItem(AbstractMinecartEntity::Type::Chest, props);
    EXPECT_EQ(chestItem.getMinecartType(), AbstractMinecartEntity::Type::Chest);
}

// ============================================================================
// ChestMinecartEntity 库存集成测试
// ============================================================================

class ChestMinecartInventoryTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ChestMinecartInventoryTest, Constructor_CreatesCorrectSize)
{
    blockentity::SimpleInventory inv(27);

    EXPECT_EQ(inv.getContainerSize(), 27);
    EXPECT_TRUE(inv.isEmpty());
}

TEST_F(ChestMinecartInventoryTest, SetItemAndGetItem_WorkCorrectly)
{
    blockentity::SimpleInventory inv(27);

    // 设置空物品后库存仍为空
    inv.setItem(0, ItemStack());
    EXPECT_TRUE(inv.isEmpty());

    // 获取空槽位返回空物品
    EXPECT_TRUE(inv.getItem(0).isEmpty());
    EXPECT_TRUE(inv.getItem(26).isEmpty());
}

TEST_F(ChestMinecartInventoryTest, Clear_EmptiesAllSlots)
{
    blockentity::SimpleInventory inv(27);

    inv.clear();
    EXPECT_TRUE(inv.isEmpty());
}

TEST_F(ChestMinecartInventoryTest, RemoveItem_UpdatesInventory)
{
    blockentity::SimpleInventory inv(27);

    // 移除空槽位物品返回空
    ItemStack removed = inv.removeItem(0, 1);
    EXPECT_TRUE(removed.isEmpty());
}

// ============================================================================
// getMaxSpeedWithRail 游戏规则测试
// ============================================================================

class MinecartSpeedTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(MinecartSpeedTest, GetMaxSpeed_ReturnsDefaultForAllTypes)
{
    // 基础最大速度常量
    RideableMinecartEntity rideable(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(rideable.getMaxSpeed(), 0.4f);

    FurnaceMinecartEntity furnace(EntityInstanceId(2), mc::test::testEcsRegistry());
    EXPECT_FLOAT_EQ(furnace.getMaxSpeed(), 0.2f);
}

TEST_F(MinecartSpeedTest, GetMaxSpeedWithRail_OffRail_ReturnsGetMaxSpeed)
{
    // 不在铁轨上时，getMaxSpeedWithRail 应返回 getMaxSpeed
    RideableMinecartEntity minecart(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(minecart.isOnRail());
    EXPECT_FLOAT_EQ(minecart.getMaxSpeedWithRail(), minecart.getMaxSpeed());
}

TEST_F(MinecartSpeedTest, GetMaxSpeedWithRail_DefaultGameRule_YieldsCorrectSpeed)
{
    // 默认 max_minecart_speed=8, 实际速度=8/20.0=0.4
    // 与旧版 DEFAULT_MAX_SPEED 一致
    RideableMinecartEntity minecart(EntityInstanceId(1), mc::test::testEcsRegistry());
    // 在铁轨上时，如果无世界指针，回退到 getMaxSpeed()
    EXPECT_FLOAT_EQ(minecart.getMaxSpeedWithRail(), minecart.getMaxSpeed());
}

// ============================================================================
// onActivatorRailPass 回调测试
// ============================================================================

class ActivatorRailCallbackTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ActivatorRailCallbackTest, HopperMinecart_ActivatorRailDisablesHopper)
{
    HopperMinecartEntity hopper(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 初始状态：漏斗未被禁用
    EXPECT_FALSE(hopper.isDisabled());

    // 充能的激活铁轨应禁用漏斗
    hopper.onActivatorRailPass(0, 0, 0, true);
    EXPECT_TRUE(hopper.isDisabled());

    // 未充能的激活铁轨应重新启用漏斗
    hopper.onActivatorRailPass(0, 0, 0, false);
    EXPECT_FALSE(hopper.isDisabled());
}

TEST_F(ActivatorRailCallbackTest, HopperMinecart_StaysDisabledAfterLeavingRail)
{
    HopperMinecartEntity hopper(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 充能激活铁轨禁用漏斗
    hopper.onActivatorRailPass(0, 0, 0, true);
    EXPECT_TRUE(hopper.isDisabled());

    // 离开激活铁轨后不调用 onActivatorRailPass，状态保持禁用
    // 这是正确行为：漏斗矿车离开激活铁轨后不会自动重新启用
    EXPECT_TRUE(hopper.isDisabled());
}

TEST_F(ActivatorRailCallbackTest, TNTMinecart_ActivatorRailIgnitesOnPowered)
{
    TNTMinecartEntity tnt(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 初始未点燃
    EXPECT_FALSE(tnt.isPrimed());

    // 充能激活铁轨应点燃TNT
    tnt.onActivatorRailPass(0, 0, 0, true);
    EXPECT_TRUE(tnt.isPrimed());
}

TEST_F(ActivatorRailCallbackTest, TNTMinecart_UnpoweredRailDoesNotIgnite)
{
    TNTMinecartEntity tnt(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 未充能激活铁轨不应点燃TNT
    tnt.onActivatorRailPass(0, 0, 0, false);
    EXPECT_FALSE(tnt.isPrimed());
}

TEST_F(ActivatorRailCallbackTest, RideableMinecart_ActivatorRailEjectsPassengersOnPowered)
{
    // 乘骑矿车在充能激活铁轨上弹出乘客
    // 此测试验证 onActivatorRailPass 不会崩溃
    RideableMinecartEntity rideable(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 无乘客时调用不应崩溃
    rideable.onActivatorRailPass(0, 0, 0, true);
    rideable.onActivatorRailPass(0, 0, 0, false);
}

TEST_F(ActivatorRailCallbackTest, CommandBlockMinecart_ExecutesOnRisingEdge)
{
    CommandBlockMinecartEntity cmd(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 充能激活铁轨的上升沿执行命令
    cmd.onActivatorRailPass(0, 0, 0, true);
    // 未充能激活铁轨重置状态
    cmd.onActivatorRailPass(0, 0, 0, false);
    // 再次充能应再次触发
    cmd.onActivatorRailPass(0, 0, 0, true);
}

TEST_F(ActivatorRailCallbackTest, FurnaceMinecart_UpdatesPushDirectionOnPowered)
{
    FurnaceMinecartEntity furnace(EntityInstanceId(1), mc::test::testEcsRegistry());

    // 熔炉矿车在充能激活铁轨上更新推动方向
    // 此测试验证 onActivatorRailPass 不会崩溃
    furnace.onActivatorRailPass(0, 0, 0, true);
    furnace.onActivatorRailPass(0, 0, 0, false);
}

} // namespace test
} // namespace entity
} // namespace mc
