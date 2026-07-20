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
#include "common/TestWorldHelper.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/hanging/HangingEntity.hpp"
#include "common/entity/entities/vehicle/MinecartEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include <memory>
#include <vector>
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

    // 单个物品占1/64槽位填充率，信号 = floor(1/64 / 27 * 14) + 1 = 0 + 1 = 1
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
    RideableMinecartEntity rideable(EntityInstanceId(1));
    EXPECT_EQ(rideable.getComparatorOutput(), 0);
}

TEST_F(MinecartComparatorOutputTest, FurnaceMinecart_ReturnsZero)
{
    // 熔炉矿车没有比较器信号输出
    FurnaceMinecartEntity furnace(EntityInstanceId(2));
    EXPECT_EQ(furnace.getComparatorOutput(), 0);
}

TEST_F(MinecartComparatorOutputTest, TNTMinecart_ReturnsZero)
{
    // TNT矿车没有比较器信号输出
    TNTMinecartEntity tnt(EntityInstanceId(3));
    EXPECT_EQ(tnt.getComparatorOutput(), 0);
}

TEST_F(MinecartComparatorOutputTest, ChestMinecart_Empty_ReturnsZero)
{
    ChestMinecartEntity chest(EntityInstanceId(10));
    EXPECT_EQ(chest.getComparatorOutput(), 0);
}

TEST_F(MinecartComparatorOutputTest, ChestMinecart_WithItems_ReturnsNonZero)
{
    ChestMinecartEntity chest(EntityInstanceId(11));
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);

    chest.setInventoryItem(0, ItemStack(*diamond, 1));
    EXPECT_GT(chest.getComparatorOutput(), 0);
    EXPECT_LE(chest.getComparatorOutput(), 15);
}

TEST_F(MinecartComparatorOutputTest, ChestMinecart_Full_ReturnsFifteen)
{
    ChestMinecartEntity chest(EntityInstanceId(12));
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
    HopperMinecartEntity hopper(EntityInstanceId(20));
    EXPECT_EQ(hopper.getComparatorOutput(), 0);
}

TEST_F(MinecartComparatorOutputTest, HopperMinecart_WithItems_ReturnsNonZero)
{
    HopperMinecartEntity hopper(EntityInstanceId(21));
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);

    hopper.setInventoryItem(0, ItemStack(*diamond, 1));
    EXPECT_GT(hopper.getComparatorOutput(), 0);
    EXPECT_LE(hopper.getComparatorOutput(), 15);
}

TEST_F(MinecartComparatorOutputTest, HopperMinecart_Full_ReturnsFifteen)
{
    HopperMinecartEntity hopper(EntityInstanceId(22));
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
    CommandBlockMinecartEntity command(EntityInstanceId(30));
    EXPECT_EQ(command.getComparatorOutput(), 0);
}

TEST_F(MinecartComparatorOutputTest, CommandBlockMinecart_SetSuccessCount_ReturnsCorrectValue)
{
    // 设置成功次数后返回对应值
    CommandBlockMinecartEntity command(EntityInstanceId(31));
    command.setSuccessCount(5);
    EXPECT_EQ(command.getComparatorOutput(), 5);
}

TEST_F(MinecartComparatorOutputTest, CommandBlockMinecart_SuccessCountCappedAtFifteen)
{
    // 成功次数上限为15
    CommandBlockMinecartEntity command(EntityInstanceId(32));
    command.setSuccessCount(100);
    EXPECT_EQ(command.getComparatorOutput(), 15);
}

TEST_F(MinecartComparatorOutputTest, CommandBlockMinecart_SuccessCountOne)
{
    CommandBlockMinecartEntity command(EntityInstanceId(33));
    command.setSuccessCount(1);
    EXPECT_EQ(command.getComparatorOutput(), 1);
}

// ========== ItemFrameEntity::getComparatorOutput 测试 ==========

class ItemFrameComparatorOutputTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(ItemFrameComparatorOutputTest, EmptyFrame_ReturnsZero)
{
    // 无物品的展示框返回0
    ItemFrameEntity frame;
    EXPECT_EQ(frame.getComparatorOutput(), 0);
}

TEST_F(ItemFrameComparatorOutputTest, FrameWithItem_Rotation0_Returns1)
{
    // 有物品、旋转0 → 0 % 8 + 1 = 1
    ItemFrameEntity frame;
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);
    frame.setDisplayedItem(ItemStack(*diamond, 1));
    EXPECT_EQ(frame.getItemRotation(), 0); // setDisplayedItem 重置旋转为0
    EXPECT_EQ(frame.getComparatorOutput(), 1);
}

TEST_F(ItemFrameComparatorOutputTest, FrameWithItem_Rotation4_Returns5)
{
    // 有物品、旋转4 → 4 % 8 + 1 = 5
    ItemFrameEntity frame;
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);
    frame.setDisplayedItem(ItemStack(*diamond, 1));
    frame.setItemRotation(4);
    EXPECT_EQ(frame.getComparatorOutput(), 5);
}

TEST_F(ItemFrameComparatorOutputTest, FrameWithItem_Rotation7_Returns8)
{
    // 有物品、旋转7（最大值）→ 7 % 8 + 1 = 8
    ItemFrameEntity frame;
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);
    frame.setDisplayedItem(ItemStack(*diamond, 1));
    frame.setItemRotation(7);
    EXPECT_EQ(frame.getComparatorOutput(), 8);
}

TEST_F(ItemFrameComparatorOutputTest, FrameWithItem_RotateItemCyclesThroughValues)
{
    // rotateItem 逐次旋转
    ItemFrameEntity frame;
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);
    frame.setDisplayedItem(ItemStack(*diamond, 1));

    // 初始旋转0 → 信号1
    EXPECT_EQ(frame.getComparatorOutput(), 1);

    // 旋转1次 → 旋转1 → 信号2
    frame.rotateItem();
    EXPECT_EQ(frame.getComparatorOutput(), 2);

    // 旋转到7 → 信号8
    for (int i = 0; i < 6; ++i) {
        frame.rotateItem();
    }
    EXPECT_EQ(frame.getItemRotation(), 7);
    EXPECT_EQ(frame.getComparatorOutput(), 8);

    // 再旋转一次回到0 → 信号1
    frame.rotateItem();
    EXPECT_EQ(frame.getItemRotation(), 0);
    EXPECT_EQ(frame.getComparatorOutput(), 1);
}

TEST_F(ItemFrameComparatorOutputTest, ComparatorOutputMatchesAnalogOutput)
{
    // getComparatorOutput() 应该等于 getAnalogOutput()
    ItemFrameEntity frame;
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);
    frame.setDisplayedItem(ItemStack(*diamond, 1));

    for (i32 rot = 0; rot < 8; ++rot) {
        frame.setItemRotation(rot);
        EXPECT_EQ(frame.getComparatorOutput(), frame.getAnalogOutput())
            << "Rotation " << rot << ": getComparatorOutput() != getAnalogOutput()";
    }
}

TEST_F(ItemFrameComparatorOutputTest, ComparatorOutputNeverExceeds8)
{
    // 展示框信号范围是0-8，不会超过8
    ItemFrameEntity frame;
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);
    frame.setDisplayedItem(ItemStack(*diamond, 1));

    for (i32 rot = 0; rot < 8; ++rot) {
        frame.setItemRotation(rot);
        EXPECT_LE(frame.getComparatorOutput(), 8);
        EXPECT_GE(frame.getComparatorOutput(), 1);
    }
}

// ========== getEntitySignal 测试 ==========

namespace {

/**
 * @brief 支持 getEntitiesInAABB 的测试世界
 *
 * 继承 BaseTestWorld，允许在测试中手动添加实体供 getEntitiesInAABB 查询。
 * 实体会根据其位置进行 AABB 过滤。
 */
class EntityTestWorld : public mc::test::BaseTestWorld {
public:
    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB& aabb, const Entity*) const override
    {
        std::vector<Entity*> result;
        for (auto* entity : m_entities) {
            if (entity == nullptr || entity->isRemoved()) {
                continue;
            }
            // 使用实体位置进行简单的 AABB 包含检查
            // 实体中心点在 AABB 内即视为相交
            f64 ex = static_cast<f64>(entity->x());
            f64 ey = static_cast<f64>(entity->y());
            f64 ez = static_cast<f64>(entity->z());
            if (aabb.contains(Vector3(static_cast<f32>(ex), static_cast<f32>(ey), static_cast<f32>(ez)))) {
                result.push_back(entity);
            }
        }
        return result;
    }

    /**
     * @brief 添加实体到世界中（用于测试）
     * 注意：测试负责确保实体生命周期覆盖测试范围
     */
    void addEntity(Entity* entity) { m_entities.push_back(entity); }

private:
    std::vector<Entity*> m_entities;
};

} // namespace

class GetEntitySignalTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(GetEntitySignalTest, EmptyWorld_ReturnsZero)
{
    // 没有实体时返回0
    EntityTestWorld world;
    BlockPos pos(0, 0, 0);
    EXPECT_EQ(RedstoneHelper::getEntitySignal(world, pos), 0);
}

TEST_F(GetEntitySignalTest, SingleChestMinecart_ReturnsSignal)
{
    // 一个箱子矿车在位置上，应返回非零信号
    EntityTestWorld world;
    ChestMinecartEntity chest(EntityInstanceId(1));
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);
    chest.setInventoryItem(0, ItemStack(*diamond, 64));

    world.addEntity(&chest);

    BlockPos pos(0, 0, 0);
    i32 signal = RedstoneHelper::getEntitySignal(world, pos);
    EXPECT_GT(signal, 0);
    EXPECT_LE(signal, 15);
}

TEST_F(GetEntitySignalTest, RideableMinecart_ReturnsZero)
{
    // 普通矿车比较器输出为0
    EntityTestWorld world;
    RideableMinecartEntity rideable(EntityInstanceId(1));
    world.addEntity(&rideable);

    BlockPos pos(0, 0, 0);
    EXPECT_EQ(RedstoneHelper::getEntitySignal(world, pos), 0);
}

TEST_F(GetEntitySignalTest, CommandBlockMinecart_ReturnsSuccessCount)
{
    // 命令方块矿车返回成功次数
    EntityTestWorld world;
    CommandBlockMinecartEntity command(EntityInstanceId(1));
    command.setSuccessCount(10);
    world.addEntity(&command);

    BlockPos pos(0, 0, 0);
    EXPECT_EQ(RedstoneHelper::getEntitySignal(world, pos), 10);
}

TEST_F(GetEntitySignalTest, MultipleEntities_ReturnsMaxSignal)
{
    // 多个实体时返回最大信号
    EntityTestWorld world;

    ChestMinecartEntity chest(EntityInstanceId(1));
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);
    chest.setInventoryItem(0, ItemStack(*diamond, 1)); // 信号1

    CommandBlockMinecartEntity command(EntityInstanceId(2));
    command.setSuccessCount(7); // 信号7

    world.addEntity(&chest);
    world.addEntity(&command);

    BlockPos pos(0, 0, 0);
    EXPECT_EQ(RedstoneHelper::getEntitySignal(world, pos), 7);
}

TEST_F(GetEntitySignalTest, FullChestMinecart_ReturnsMaxSignal)
{
    // 满的箱子矿车返回15
    EntityTestWorld world;
    ChestMinecartEntity chest(EntityInstanceId(1));
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);
    for (i32 i = 0; i < ChestMinecartEntity::INVENTORY_SIZE; ++i) {
        chest.setInventoryItem(i, ItemStack(*diamond, 64));
    }
    world.addEntity(&chest);

    BlockPos pos(0, 0, 0);
    EXPECT_EQ(RedstoneHelper::getEntitySignal(world, pos), 15);
}

TEST_F(GetEntitySignalTest, FullHopperMinecart_ReturnsMaxSignal)
{
    // 满的漏斗矿车返回15
    EntityTestWorld world;
    HopperMinecartEntity hopper(EntityInstanceId(1));
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);
    for (i32 i = 0; i < HopperMinecartEntity::INVENTORY_SIZE; ++i) {
        hopper.setInventoryItem(i, ItemStack(*diamond, 64));
    }
    world.addEntity(&hopper);

    BlockPos pos(0, 0, 0);
    EXPECT_EQ(RedstoneHelper::getEntitySignal(world, pos), 15);
}

TEST_F(GetEntitySignalTest, ItemFrameEntity_ReturnsAnalogOutput)
{
    // 物品展示框返回比较器输出信号
    EntityTestWorld world;
    ItemFrameEntity frame;
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);
    frame.setDisplayedItem(ItemStack(*diamond, 1));
    frame.setItemRotation(3); // 3 % 8 + 1 = 4
    world.addEntity(&frame);

    BlockPos pos(0, 0, 0);
    EXPECT_EQ(RedstoneHelper::getEntitySignal(world, pos), 4);
}

TEST_F(GetEntitySignalTest, AABBOverload_ReturnsSameAsBlockPosOverload)
{
    // AABB 重载应与 BlockPos 重载返回相同结果
    EntityTestWorld world;
    ChestMinecartEntity chest(EntityInstanceId(1));
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);
    for (i32 i = 0; i < ChestMinecartEntity::INVENTORY_SIZE; ++i) {
        chest.setInventoryItem(i, ItemStack(*diamond, 64));
    }
    world.addEntity(&chest);

    BlockPos pos(0, 0, 0);
    AxisAlignedBB aabb = AxisAlignedBB::fromBlock(0, 0, 0);
    EXPECT_EQ(RedstoneHelper::getEntitySignal(world, pos), RedstoneHelper::getEntitySignal(world, aabb));
}

TEST_F(GetEntitySignalTest, AABBOverload_EmptyWorld_ReturnsZero)
{
    // AABB 重载在空世界中返回0
    EntityTestWorld world;
    AxisAlignedBB aabb(static_cast<f64>(0),
        static_cast<f64>(0),
        static_cast<f64>(0),
        static_cast<f64>(1),
        static_cast<f64>(1),
        static_cast<f64>(1));
    EXPECT_EQ(RedstoneHelper::getEntitySignal(world, aabb), 0);
}

TEST_F(GetEntitySignalTest, AABBOverload_FiltersByPosition)
{
    // AABB 重载应仅返回搜索区域内的实体信号
    EntityTestWorld world;

    // 创建两个矿车，一个在原点 (0,0,0)，一个在远处 (100,0,0)
    ChestMinecartEntity nearbyChest(EntityInstanceId(1));
    Item* diamond = ensureTestItem("diamond");
    ASSERT_NE(diamond, nullptr);
    // 填满全部27格，信号15
    for (i32 i = 0; i < ChestMinecartEntity::INVENTORY_SIZE; ++i) {
        nearbyChest.setInventoryItem(i, ItemStack(*diamond, 64));
    }
    nearbyChest.setPosition(0.5f, 0.5f, 0.5f); // 在 AABB 内

    CommandBlockMinecartEntity farCommand(EntityInstanceId(2));
    farCommand.setSuccessCount(10);             // 信号10
    farCommand.setPosition(100.0f, 0.0f, 0.0f); // 在 AABB 外

    world.addEntity(&nearbyChest);
    world.addEntity(&farCommand);

    // 搜索 (0,0,0)-(1,1,1) 范围，应只找到 nearbyChest
    AxisAlignedBB nearbyBox(static_cast<f64>(0),
        static_cast<f64>(0),
        static_cast<f64>(0),
        static_cast<f64>(1),
        static_cast<f64>(1),
        static_cast<f64>(1));
    EXPECT_EQ(RedstoneHelper::getEntitySignal(world, nearbyBox), 15);

    // 搜索 (99,0,0)-(101,1,1) 范围，应只找到 farCommand
    AxisAlignedBB farBox(static_cast<f64>(99),
        static_cast<f64>(0),
        static_cast<f64>(0),
        static_cast<f64>(101),
        static_cast<f64>(1),
        static_cast<f64>(1));
    EXPECT_EQ(RedstoneHelper::getEntitySignal(world, farBox), 10);

    // 搜索远离所有实体的区域，应返回0
    AxisAlignedBB emptyBox(static_cast<f64>(50),
        static_cast<f64>(0),
        static_cast<f64>(0),
        static_cast<f64>(51),
        static_cast<f64>(1),
        static_cast<f64>(1));
    EXPECT_EQ(RedstoneHelper::getEntitySignal(world, emptyBox), 0);
}
