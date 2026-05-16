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
#include "advancement/trigger/conditions/EntityPredicate.hpp"
#include "advancement/trigger/conditions/EntityFlagsPredicate.hpp"
#include "advancement/trigger/conditions/EntityEquipmentPredicate.hpp"
#include "advancement/trigger/conditions/NBTPredicate.hpp"
#include "advancement/trigger/conditions/LocationPredicate.hpp"
#include "advancement/MinMaxBounds.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <nlohmann/json.hpp>

using namespace mc::advancement;

// ========== EntityFlagsPredicate 测试 ==========

class EntityFlagsPredicateTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F(EntityFlagsPredicateTest, DefaultPredicateMatchesAny)
{
    EntityFlagsPredicate predicate;
    EXPECT_TRUE(predicate.isAny());
}

TEST_F(EntityFlagsPredicateTest, NullJsonReturnsAny)
{
    auto result = EntityFlagsPredicate::fromJson(nullptr);
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.value().isAny());
}

TEST_F(EntityFlagsPredicateTest, OnFireFlag)
{
    nlohmann::json json = {{"is_on_fire", true}};
    auto result = EntityFlagsPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    EXPECT_TRUE(result.value().isOnFire().has_value());
    EXPECT_TRUE(result.value().isOnFire().value());
}

TEST_F(EntityFlagsPredicateTest, MultipleFlags)
{
    nlohmann::json json = {
        {"is_on_fire", false},
        {"is_sneaking", true},
        {"is_sprinting", false},
        {"is_swimming", true},
        {"is_baby", true}
    };

    auto result = EntityFlagsPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());

    EXPECT_TRUE(result.value().isOnFire().has_value());
    EXPECT_FALSE(result.value().isOnFire().value());

    EXPECT_TRUE(result.value().isSneaking().has_value());
    EXPECT_TRUE(result.value().isSneaking().value());

    EXPECT_TRUE(result.value().isSprinting().has_value());
    EXPECT_FALSE(result.value().isSprinting().value());

    EXPECT_TRUE(result.value().isSwimming().has_value());
    EXPECT_TRUE(result.value().isSwimming().value());

    EXPECT_TRUE(result.value().isBaby().has_value());
    EXPECT_TRUE(result.value().isBaby().value());
}

TEST_F(EntityFlagsPredicateTest, ToJsonRoundTrip)
{
    nlohmann::json json = {
        {"is_on_fire", true},
        {"is_baby", false}
    };

    auto result = EntityFlagsPredicate::fromJson(json);
    EXPECT_TRUE(result.success());

    nlohmann::json serialized = result.value().toJson();
    EXPECT_FALSE(serialized.is_null());

    auto result2 = EntityFlagsPredicate::fromJson(serialized);
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(result.value().isOnFire(), result2.value().isOnFire());
    EXPECT_EQ(result.value().isBaby(), result2.value().isBaby());
}

// ========== EntityEquipmentPredicate 测试 ==========

class EntityEquipmentPredicateTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F(EntityEquipmentPredicateTest, DefaultPredicateMatchesAny)
{
    EntityEquipmentPredicate predicate;
    EXPECT_TRUE(predicate.isAny());
}

TEST_F(EntityEquipmentPredicateTest, NullJsonReturnsAny)
{
    auto result = EntityEquipmentPredicate::fromJson(nullptr);
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.value().isAny());
}

TEST_F(EntityEquipmentPredicateTest, EmptyObjectReturnsAny)
{
    nlohmann::json json = nlohmann::json::object();
    auto result = EntityEquipmentPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.value().isAny());
}

TEST_F(EntityEquipmentPredicateTest, WithHeadEquipment)
{
    nlohmann::json json = {
        {"head", {{"item", "minecraft:diamond_helmet"}}}
    };

    auto result = EntityEquipmentPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    EXPECT_FALSE(result.value().getHead().isAny());
    EXPECT_TRUE(result.value().getChest().isAny());
    EXPECT_TRUE(result.value().getLegs().isAny());
    EXPECT_TRUE(result.value().getFeet().isAny());
}

TEST_F(EntityEquipmentPredicateTest, WithAllEquipment)
{
    nlohmann::json json = {
        {"head", {{"item", "minecraft:diamond_helmet"}}},
        {"chest", {{"item", "minecraft:diamond_chestplate"}}},
        {"legs", {{"item", "minecraft:diamond_leggings"}}},
        {"feet", {{"item", "minecraft:diamond_boots"}}},
        {"mainhand", {{"item", "minecraft:diamond_sword"}}},
        {"offhand", {{"item", "minecraft:shield"}}}
    };

    auto result = EntityEquipmentPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());

    EXPECT_FALSE(result.value().getHead().isAny());
    EXPECT_FALSE(result.value().getChest().isAny());
    EXPECT_FALSE(result.value().getLegs().isAny());
    EXPECT_FALSE(result.value().getFeet().isAny());
    EXPECT_FALSE(result.value().getMainHand().isAny());
    EXPECT_FALSE(result.value().getOffHand().isAny());
}

// ========== NBTPredicate 测试 ==========

class NBTPredicateTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F(NBTPredicateTest, DefaultPredicateMatchesAny)
{
    NBTPredicate predicate;
    EXPECT_TRUE(predicate.isAny());
}

TEST_F(NBTPredicateTest, NullJsonReturnsAny)
{
    auto result = NBTPredicate::fromJson(nullptr);
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.value().isAny());
}

TEST_F(NBTPredicateTest, StringJsonReturnsAny)
{
    // 由于 Mojangson 解析器尚未实现，字符串格式暂时返回空的谓词
    nlohmann::json json = "{CustomName:'{\"text\":\"Test\"}'}";
    auto result = NBTPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    // 暂时返回空的 NBTPredicate
}

// ========== DistancePredicate 测试 ==========

class DistancePredicateTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F(DistancePredicateTest, DefaultPredicateMatchesAny)
{
    DistancePredicate predicate;
    EXPECT_TRUE(predicate.isAny());
}

TEST_F(DistancePredicateTest, NullJsonReturnsAny)
{
    auto result = DistancePredicate::fromJson(nullptr);
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.value().isAny());
}

TEST_F(DistancePredicateTest, ExactDistance)
{
    nlohmann::json json = 10.0;
    auto result = DistancePredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());

    // 测试精确距离
    EXPECT_TRUE(result.value().test(0, 0, 0, 10, 0, 0));  // 距离 10
    EXPECT_FALSE(result.value().test(0, 0, 0, 9, 0, 0));  // 距离 9
    EXPECT_FALSE(result.value().test(0, 0, 0, 11, 0, 0)); // 距离 11
}

TEST_F(DistancePredicateTest, RangeDistance)
{
    nlohmann::json json = {{"min", 5.0}, {"max", 10.0}};
    auto result = DistancePredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());

    // 测试范围
    EXPECT_FALSE(result.value().test(0, 0, 0, 4, 0, 0));  // 距离 4
    EXPECT_TRUE(result.value().test(0, 0, 0, 5, 0, 0));   // 距离 5
    EXPECT_TRUE(result.value().test(0, 0, 0, 7.5, 0, 0)); // 距离 7.5
    EXPECT_TRUE(result.value().test(0, 0, 0, 10, 0, 0));  // 距离 10
    EXPECT_FALSE(result.value().test(0, 0, 0, 11, 0, 0)); // 距离 11
}

TEST_F(DistancePredicateTest, AtLeast)
{
    auto predicate = DistancePredicate::atLeast(10.0);
    EXPECT_FALSE(predicate.isAny());

    EXPECT_FALSE(predicate.test(0, 0, 0, 9, 0, 0));   // 距离 9
    EXPECT_TRUE(predicate.test(0, 0, 0, 10, 0, 0));  // 距离 10
    EXPECT_TRUE(predicate.test(0, 0, 0, 15, 0, 0));  // 距离 15
}

TEST_F(DistancePredicateTest, AtMost)
{
    auto predicate = DistancePredicate::atMost(10.0);
    EXPECT_FALSE(predicate.isAny());

    EXPECT_TRUE(predicate.test(0, 0, 0, 9, 0, 0));    // 距离 9
    EXPECT_TRUE(predicate.test(0, 0, 0, 10, 0, 0));   // 距离 10
    EXPECT_FALSE(predicate.test(0, 0, 0, 11, 0, 0));  // 距离 11
}

TEST_F(DistancePredicateTest, Between)
{
    auto predicate = DistancePredicate::between(5.0, 10.0);
    EXPECT_FALSE(predicate.isAny());

    EXPECT_FALSE(predicate.test(0, 0, 0, 4, 0, 0));   // 距离 4
    EXPECT_TRUE(predicate.test(0, 0, 0, 5, 0, 0));    // 距离 5
    EXPECT_TRUE(predicate.test(0, 0, 0, 7.5, 0, 0));  // 距离 7.5
    EXPECT_TRUE(predicate.test(0, 0, 0, 10, 0, 0));   // 距离 10
    EXPECT_FALSE(predicate.test(0, 0, 0, 11, 0, 0));  // 距离 11
}

TEST_F(DistancePredicateTest, ToJsonRoundTrip)
{
    nlohmann::json json = {{"min", 5.0}, {"max", 10.0}};
    auto result = DistancePredicate::fromJson(json);
    EXPECT_TRUE(result.success());

    nlohmann::json serialized = result.value().toJson();
    EXPECT_FALSE(serialized.is_null());

    auto result2 = DistancePredicate::fromJson(serialized);
    EXPECT_TRUE(result2.success());

    // 验证序列化往返一致性
    EXPECT_TRUE(result2.value().test(0, 0, 0, 7.5, 0, 0));
    EXPECT_FALSE(result2.value().test(0, 0, 0, 4, 0, 0));
    EXPECT_FALSE(result2.value().test(0, 0, 0, 11, 0, 0));
}

// ========== EntityPredicate 测试 ==========

class EntityPredicateTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F(EntityPredicateTest, DefaultPredicateMatchesAny)
{
    EntityPredicate predicate;
    EXPECT_TRUE(predicate.isAny());
}

TEST_F(EntityPredicateTest, NullJsonReturnsAny)
{
    auto result = EntityPredicate::fromJson(nullptr);
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.value().isAny());
}

TEST_F(EntityPredicateTest, TypeOnly)
{
    nlohmann::json json = {{"type", "minecraft:zombie"}};

    auto result = EntityPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    EXPECT_TRUE(result.value().getType().has_value());
    EXPECT_EQ(result.value().getType().value().toString(), "minecraft:zombie");
}

TEST_F(EntityPredicateTest, WithDistance)
{
    nlohmann::json json = {
        {"type", "minecraft:zombie"},
        {"distance", {{"min", 5.0}, {"max", 20.0}}}
    };

    auto result = EntityPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    EXPECT_FALSE(result.value().getDistance().isAny());
}

TEST_F(EntityPredicateTest, WithFlags)
{
    nlohmann::json json = {
        {"type", "minecraft:zombie"},
        {"flags", {{"is_on_fire", true}, {"is_baby", false}}}
    };

    auto result = EntityPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    EXPECT_FALSE(result.value().getFlags().isAny());
    EXPECT_TRUE(result.value().getFlags().isOnFire().has_value());
    EXPECT_TRUE(result.value().getFlags().isOnFire().value());
    EXPECT_TRUE(result.value().getFlags().isBaby().has_value());
    EXPECT_FALSE(result.value().getFlags().isBaby().value());
}

TEST_F(EntityPredicateTest, WithEquipment)
{
    nlohmann::json json = {
        {"type", "minecraft:skeleton"},
        {"equipment", {
            {"mainhand", {{"item", "minecraft:bow"}}}
        }}
    };

    auto result = EntityPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    EXPECT_FALSE(result.value().getEquipment().isAny());
    EXPECT_FALSE(result.value().getEquipment().getMainHand().isAny());
}

TEST_F(EntityPredicateTest, WithEffects)
{
    nlohmann::json json = {
        {"type", "minecraft:zombie"},
        {"effects", {
            {"minecraft:regeneration", {{"amplifier", {{"min", 0}}}}}
        }}
    };

    auto result = EntityPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    EXPECT_FALSE(result.value().getEffects().isAny());
}

TEST_F(EntityPredicateTest, ComplexPredicate)
{
    nlohmann::json json = {
        {"type", "minecraft:zombie"},
        {"distance", {{"max", 30.0}}},
        {"flags", {{"is_baby", true}}},
        {"equipment", {
            {"head", {{"item", "minecraft:diamond_helmet"}}}
        }}
    };

    auto result = EntityPredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());

    // 验证所有组件都正确解析
    EXPECT_TRUE(result.value().getType().has_value());
    EXPECT_FALSE(result.value().getDistance().isAny());
    EXPECT_FALSE(result.value().getFlags().isAny());
    EXPECT_FALSE(result.value().getEquipment().isAny());
    EXPECT_TRUE(result.value().getEffects().isAny()); // 未指定
    EXPECT_TRUE(result.value().getNbt().isAny());     // 未指定
    EXPECT_TRUE(result.value().getLocation().isAny()); // 未指定
}

TEST_F(EntityPredicateTest, ToJsonRoundTrip)
{
    nlohmann::json json = {
        {"type", "minecraft:skeleton"},
        {"distance", {{"min", 10.0}, {"max", 50.0}}},
        {"flags", {{"is_on_fire", false}}}
    };

    auto result = EntityPredicate::fromJson(json);
    EXPECT_TRUE(result.success());

    nlohmann::json serialized = result.value().toJson();
    EXPECT_FALSE(serialized.is_null());

    auto result2 = EntityPredicate::fromJson(serialized);
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(result.value().getType().value().toString(),
              result2.value().getType().value().toString());
    EXPECT_FALSE(result2.value().getDistance().isAny());
    EXPECT_FALSE(result2.value().getFlags().isAny());
}

// ========== DamageSourcePredicate 测试 ==========

class DamageSourcePredicateTest : public ::testing::Test
{
protected:
    void SetUp() override {}
};

TEST_F(DamageSourcePredicateTest, DefaultPredicateMatchesAny)
{
    DamageSourcePredicate predicate;
    EXPECT_TRUE(predicate.isAny());
}

TEST_F(DamageSourcePredicateTest, NullJsonReturnsAny)
{
    auto result = DamageSourcePredicate::fromJson(nullptr);
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(result.value().isAny());
}

TEST_F(DamageSourcePredicateTest, IsProjectile)
{
    nlohmann::json json = {{"is_projectile", true}};
    auto result = DamageSourcePredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());
    EXPECT_TRUE(result.value().isProjectile().has_value());
    EXPECT_TRUE(result.value().isProjectile().value());
}

TEST_F(DamageSourcePredicateTest, MultipleFlags)
{
    nlohmann::json json = {
        {"is_fire", true},
        {"is_explosion", false},
        {"bypasses_armor", true}
    };

    auto result = DamageSourcePredicate::fromJson(json);
    EXPECT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());

    EXPECT_TRUE(result.value().isFire().has_value());
    EXPECT_TRUE(result.value().isFire().value());

    EXPECT_TRUE(result.value().isExplosion().has_value());
    EXPECT_FALSE(result.value().isExplosion().value());

    EXPECT_TRUE(result.value().bypassesArmor().has_value());
    EXPECT_TRUE(result.value().bypassesArmor().value());
}

TEST_F(DamageSourcePredicateTest, ToJsonRoundTrip)
{
    nlohmann::json json = {
        {"is_magic", true},
        {"is_lightning", false}
    };

    auto result = DamageSourcePredicate::fromJson(json);
    EXPECT_TRUE(result.success());

    nlohmann::json serialized = result.value().toJson();
    EXPECT_FALSE(serialized.is_null());

    auto result2 = DamageSourcePredicate::fromJson(serialized);
    EXPECT_TRUE(result2.success());

    EXPECT_EQ(result.value().isMagic(), result2.value().isMagic());
    EXPECT_EQ(result.value().isLightning(), result2.value().isLightning());
}
