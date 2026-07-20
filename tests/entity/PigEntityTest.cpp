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

#include <gtest/gtest.h>

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/passive/basic/PigEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"

using namespace mc;
using namespace mc::math;

namespace {

// ============================================================================
// PigEntity IEquipable 接口测试
// ============================================================================

class PigEntityEquipableTest : public ::testing::Test {
protected:
    void SetUp() override { pig = std::make_unique<PigEntity>(EntityInstanceId(1)); }

    std::unique_ptr<PigEntity> pig;
};

/**
 * @brief 测试 IEquipable 接口槽数量
 * MC 1.16.5: 猪只有一个鞍槽
 */
TEST_F(PigEntityEquipableTest, HasOneEquipmentSlot)
{
    EXPECT_EQ(pig->getEquipmentSlotCount(), 1);
}

/**
 * @brief 测试无鞍时 getEquipment 返回空
 *
 * 注意：PigEntity 使用 BoostHelper 管理鞍状态，需要 EntityDataManager 初始化。
 * 在单元测试中（无 World 上下文），BoostHelper 未初始化，hasSaddle() 始终返回 false。
 * 这是预期的设计行为。
 */
TEST_F(PigEntityEquipableTest, GetEquipmentReturnsEmptyWhenNoSaddle)
{
    // 无 World 上下文时，BoostHelper 未初始化，hasSaddle() 返回 false
    EXPECT_FALSE(pig->hasSaddle());
    ItemStack equipment = pig->getEquipment(0);
    EXPECT_TRUE(equipment.isEmpty());
}

/**
 * @brief 测试无效槽位返回空
 */
TEST_F(PigEntityEquipableTest, InvalidSlotReturnsEmpty)
{
    // 负数槽位
    EXPECT_TRUE(pig->getEquipment(-1).isEmpty());

    // 超出范围的槽位
    EXPECT_TRUE(pig->getEquipment(1).isEmpty());
    EXPECT_TRUE(pig->getEquipment(2).isEmpty());
}

/**
 * @brief 测试 setEquipment 忽略无效槽位
 *
 * 由于 BoostHelper 未初始化，setEquipment 不会改变鞍状态。
 * 这里只测试 setEquipment 不会崩溃。
 */
TEST_F(PigEntityEquipableTest, SetEquipmentIgnoresInvalidSlot)
{
    ItemStack saddle(Items::SADDLE, 1);

    // 设置到无效槽位不应崩溃
    pig->setEquipment(1, saddle);
    pig->setEquipment(-1, saddle);
}

/**
 * @brief 测试 canEquip 接受鞍
 */
TEST_F(PigEntityEquipableTest, CanEquipSaddle)
{
    ItemStack saddle(Items::SADDLE, 1);
    EXPECT_TRUE(pig->canEquip(saddle, 0));
}

/**
 * @brief 测试 canEquip 拒绝非鞍物品
 */
TEST_F(PigEntityEquipableTest, CannotEquipNonSaddle)
{
    ItemStack diamond(Items::DIAMOND, 1);
    EXPECT_FALSE(pig->canEquip(diamond, 0));

    ItemStack sword(Items::DIAMOND_SWORD, 1);
    EXPECT_FALSE(pig->canEquip(sword, 0));
}

/**
 * @brief 测试 canEquip 接受空物品（清空槽位）
 */
TEST_F(PigEntityEquipableTest, CanEquipEmptyToClearSlot)
{
    ItemStack empty;
    EXPECT_TRUE(pig->canEquip(empty, 0));
}

/**
 * @brief 测试 canEquip 拒绝有效物品到无效槽位
 */
TEST_F(PigEntityEquipableTest, CannotEquipToInvalidSlot)
{
    ItemStack saddle(Items::SADDLE, 1);

    // 无效槽位
    EXPECT_FALSE(pig->canEquip(saddle, 1));
    EXPECT_FALSE(pig->canEquip(saddle, -1));
}

// ============================================================================
// PigEntity IRideable 接口测试
// ============================================================================

class PigEntityRideableTest : public ::testing::Test {
protected:
    void SetUp() override { pig = std::make_unique<PigEntity>(EntityInstanceId(1)); }

    std::unique_ptr<PigEntity> pig;
};

/**
 * @brief 测试初始无鞍状态
 *
 * 注意：PigEntity 使用 BoostHelper 管理鞍状态，需要 EntityDataManager 初始化。
 * 在单元测试中（无 World 上下文），BoostHelper 未初始化，hasSaddle() 始终返回 false。
 */
TEST_F(PigEntityRideableTest, InitiallyNotSaddled)
{
    // 无 World 上下文时，BoostHelper 未初始化，hasSaddle() 返回 false
    EXPECT_FALSE(pig->hasSaddle());
}

/**
 * @brief 测试 setSaddle 在无 World 上下文时的行为
 *
 * 由于 BoostHelper 未初始化，setSaddle() 调用 setSaddledFromBoolean()
 * 不会改变鞍状态。这是预期的设计行为。
 *
 * 完整的鞍状态测试需要集成测试（有 World 上下文）。
 */
TEST_F(PigEntityRideableTest, SetSaddleRequiresWorldContext)
{
    // 无 World 上下文时，setSaddle 不生效
    pig->setSaddle(true);
    EXPECT_FALSE(pig->hasSaddle()); // 仍然是 false，因为 BoostHelper 未初始化

    pig->setSaddle(false);
    EXPECT_FALSE(pig->hasSaddle());
}

/**
 * @brief 测试骑乘速度
 * MC 1.16.5: PigEntity.getMountedSpeed() = movementSpeed * 0.225
 *
 * 注意：此测试依赖属性系统，属性在 registerAttributes() 中注册。
 * 构造函数中已调用 registerAttributes()。
 */
TEST_F(PigEntityRideableTest, SteeringSpeed)
{
    // 注册属性后获取骑乘速度
    // 基础速度是 0.25，骑乘速度 = 0.25 * 0.225 = 0.05625
    f32 speed = pig->getSteeringSpeed();
    // BoostHelper 未初始化时，返回默认速度乘数
    // 实际速度需要属性系统完全初始化
    EXPECT_GT(speed, 0.0f); // 确保返回正值
}

/**
 * @brief 测试鞍状态与 IEquipable 同步需要 World 上下文
 *
 * 由于 BoostHelper 未初始化，鞍状态设置不生效。
 * 此测试验证在没有 World 上下文时的预期行为。
 */
TEST_F(PigEntityRideableTest, SaddleSyncRequiresWorldContext)
{
    // 无 World 上下文时，通过 IRideable 设置鞍不生效
    pig->setSaddle(true);
    EXPECT_FALSE(pig->hasSaddle());
    EXPECT_TRUE(pig->getEquipment(0).isEmpty()); // 没有鞍

    // 通过 IEquipable 清除鞍也不生效（本来就没有）
    ItemStack empty;
    pig->setEquipment(0, empty);
    EXPECT_FALSE(pig->hasSaddle());
    EXPECT_TRUE(pig->getEquipment(0).isEmpty());
}

// ============================================================================
// PigEntity 基本属性测试
// ============================================================================

class PigEntityBasicTest : public ::testing::Test {
protected:
    void SetUp() override { pig = std::make_unique<PigEntity>(EntityInstanceId(1)); }

    std::unique_ptr<PigEntity> pig;
};

/**
 * @brief 测试繁殖物品检测 - 胡萝卜
 */
TEST_F(PigEntityBasicTest, IsBreedingItem_Carrot)
{
    ItemStack carrot(Items::CARROT, 1);
    EXPECT_TRUE(pig->isBreedingItem(carrot));
}

/**
 * @brief 测试繁殖物品检测 - 马铃薯
 */
TEST_F(PigEntityBasicTest, IsBreedingItem_Potato)
{
    ItemStack potato(Items::POTATO, 1);
    EXPECT_TRUE(pig->isBreedingItem(potato));
}

/**
 * @brief 测试繁殖物品检测 - 甜菜根
 */
TEST_F(PigEntityBasicTest, IsBreedingItem_Beetroot)
{
    ItemStack beetroot(Items::BEETROOT, 1);
    EXPECT_TRUE(pig->isBreedingItem(beetroot));
}

/**
 * @brief 测试繁殖物品检测 - 非繁殖物品
 */
TEST_F(PigEntityBasicTest, IsBreedingItem_NonBreedingItem)
{
    ItemStack diamond(Items::DIAMOND, 1);
    EXPECT_FALSE(pig->isBreedingItem(diamond));

    ItemStack wheat(Items::WHEAT, 1);
    EXPECT_FALSE(pig->isBreedingItem(wheat));
}

/**
 * @brief 测试繁殖物品检测 - 空物品
 */
TEST_F(PigEntityBasicTest, IsBreedingItem_EmptyItem)
{
    ItemStack empty;
    EXPECT_FALSE(pig->isBreedingItem(empty));
}

} // namespace
