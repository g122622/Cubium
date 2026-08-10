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

#include "common/TestWorldHelper.hpp"
#include "common/core/Types.hpp"
#include "entity/core/Entity.hpp"
#include "entity/entities/effect/EffectEntities.hpp"
#include "entity/entities/misc/OminousItemSpawnerEntity.hpp"
#include "entity/entities/passive/ambient/BatEntity.hpp"
#include "entity/entities/vehicle/BoatEntity.hpp"

using namespace mc;
using namespace mc::entity;

// ============================================================================
// couldAcceptPassenger / canAddPassenger 默认行为测试
// ============================================================================

TEST(PassengerSystemTest, DefaultCouldAcceptPassengerReturnsTrue)
{
    // Entity 基类默认 couldAcceptPassenger() 返回 true
    BoatEntity boat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());
    EXPECT_TRUE(boat.couldAcceptPassenger());
}

TEST(PassengerSystemTest, DefaultCanAddPassengerReturnsTrueWhenEmpty)
{
    // Entity 基类默认 canAddPassenger() 在乘客未满时返回 true
    // 使用 ArmorStandEntity 测试（默认 getMaxPassengers() == 1）
    ArmorStandEntity armorStand{mc::test::testEcsRegistry()};
    // canAddPassenger 不检查 passenger 参数，仅检查乘客数量
    EXPECT_TRUE(armorStand.canAddPassenger(armorStand));
}

TEST(PassengerSystemTest, DefaultGetMaxPassengersIsOne)
{
    // Entity 基类默认 getMaxPassengers() 返回 1
    ArmorStandEntity armorStand{mc::test::testEcsRegistry()};
    EXPECT_EQ(armorStand.getMaxPassengers(), 1);
}

// ============================================================================
// OminousItemSpawner 乘客系统测试
// ============================================================================

TEST(PassengerSystemTest, OminousItemSpawnerDoesNotTriggerPressurePlate)
{
    // OminousItemSpawner 不触发压力板
    OminousItemSpawnerEntity spawner(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_TRUE(spawner.doesEntityNotTriggerPressurePlate());
}

TEST(PassengerSystemTest, OminousItemSpawnerCannotAcceptPassengers)
{
    // OminousItemSpawner 不能接受任何乘客
    OminousItemSpawnerEntity spawner(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(spawner.couldAcceptPassenger());
}

TEST(PassengerSystemTest, OminousItemSpawnerCannotAddPassenger)
{
    // OminousItemSpawner 不能添加任何乘客
    OminousItemSpawnerEntity spawner(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_FALSE(spawner.canAddPassenger(spawner));
}

// ============================================================================
// ArmorStand 压力板行为测试
// ============================================================================

TEST(PassengerSystemTest, ArmorStandNormalModeTriggersPressurePlate)
{
    // 普通模式盔甲架触发压力板
    ArmorStandEntity armorStand{mc::test::testEcsRegistry()};
    EXPECT_FALSE(armorStand.isMarker());
    EXPECT_FALSE(armorStand.doesEntityNotTriggerPressurePlate());
}

TEST(PassengerSystemTest, ArmorStandMarkerModeDoesNotTriggerPressurePlate)
{
    // 标记模式盔甲架不触发压力板
    ArmorStandEntity armorStand{mc::test::testEcsRegistry()};
    armorStand.setMarker(true);
    EXPECT_TRUE(armorStand.isMarker());
    EXPECT_TRUE(armorStand.doesEntityNotTriggerPressurePlate());
}

// ============================================================================
// Bat 压力板行为测试
// ============================================================================

TEST(PassengerSystemTest, BatDoesNotTriggerPressurePlate)
{
    // 蝙蝠不触发压力板
    BatEntity bat(EntityInstanceId(1), mc::test::testEcsRegistry());
    EXPECT_TRUE(bat.doesEntityNotTriggerPressurePlate());
}

// ============================================================================
// Boat 多乘客测试
// ============================================================================

TEST(PassengerSystemTest, BoatCanAddPassengerWhenNotFull)
{
    // 船未满时可以添加乘客
    BoatEntity boat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());
    EXPECT_EQ(boat.getMaxPassengers(), 2);
    // 空船应该可以添加乘客
    EXPECT_TRUE(boat.canAddPassenger(boat));
}

TEST(PassengerSystemTest, BoatCouldAcceptPassengers)
{
    // 船可以接受乘客
    BoatEntity boat(BoatEntity::Type::OAK, mc::test::testEcsRegistry());
    EXPECT_TRUE(boat.couldAcceptPassenger());
}

// ============================================================================
// Entity 基类默认行为测试
// ============================================================================

TEST(PassengerSystemTest, DefaultEntityTriggersPressurePlate)
{
    // ArmorStand（非 marker 模式）默认触发压力板
    ArmorStandEntity armorStand{mc::test::testEcsRegistry()};
    EXPECT_FALSE(armorStand.doesEntityNotTriggerPressurePlate());
}

TEST(PassengerSystemTest, DefaultEntityCanAcceptPassengers)
{
    // Entity 基类默认可以接受乘客
    ArmorStandEntity armorStand{mc::test::testEcsRegistry()};
    EXPECT_TRUE(armorStand.couldAcceptPassenger());
}
