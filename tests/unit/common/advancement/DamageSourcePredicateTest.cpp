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
 * @file FrogFroglightDamageSourcePredicateTest.cpp
 * @brief 伤害源谓词（vanilla DamageSourcePredicate）单元测试
 *
 * 覆盖岩浆怪/史莱姆"被青蛙捕食时改掉蛙明灯"所依赖的
 * `damage_source_properties.source_entity` 条件：
 * 此前 DamageSourcePredicate 只解析布尔标志，source_entity / direct_entity / type / tags
 * 被静默忽略，谓词退化为"恒真"，青蛙捕食时蛙明灯条件失效。
 */

#include <gtest/gtest.h>

#include "common/TestWorldHelper.hpp"
#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/basic/SlimeEntity.hpp"
#include "common/entity/entities/passive/basic/SheepEntity.hpp"
#include "common/entity/entities/passive/frog/FrogEntity.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <nlohmann/json.hpp>

namespace mc {
namespace {

/// Entity 构造函数为 protected，测试用最小派生类实例化带类型 ID 的"攻击者"。
class FakeAttacker final : public Entity {
public:
    explicit FakeAttacker(EntityInstanceId id, ecs::EntityRegistry& registry)
        : Entity(id, nullptr, registry)
    {}
};

/// BaseTestWorld 构造函数为 protected，测试以派生类实例化。
class VehicleTestWorld final : public mc::test::BaseTestWorld {};

class FrogFroglightDamageSourcePredicateTest : public ::testing::Test {};

TEST_F(FrogFroglightDamageSourcePredicateTest, SourceEntity_MatchesFrogAttacker)
{
    EntityDamageSource source(DamageType::MobAttack, nullptr);
    // 用一个带类型 ID 的实体充当"青蛙"（谓词只看 entity.getTypeId()）
    FakeAttacker attacker(EntityInstanceId(1), mc::test::testEcsRegistry());
    attacker.setTypeId(entity::EntityTypeKeys::FROG);
    EntityDamageSource withAttacker(DamageType::MobAttack, &attacker);

    const nlohmann::json json = {{"source_entity", {{"type", entity::EntityTypeKeys::FROG}}}};
    const auto predicate = advancement::DamageSourcePredicate::fromJson(json);
    ASSERT_TRUE(predicate.success());
    EXPECT_FALSE(predicate.value().isAny());

    // 青蛙造成的伤害匹配；无造成者的伤害不匹配
    EXPECT_TRUE(predicate.value().test(withAttacker));
    EXPECT_FALSE(predicate.value().test(source));
}

TEST_F(FrogFroglightDamageSourcePredicateTest, SourceEntity_RejectsOtherAttacker)
{
    FakeAttacker zombie(EntityInstanceId(1), mc::test::testEcsRegistry());
    zombie.setTypeId(entity::EntityTypeKeys::ZOMBIE);
    EntityDamageSource source(DamageType::MobAttack, &zombie);

    const nlohmann::json json = {{"source_entity", {{"type", entity::EntityTypeKeys::FROG}}}};
    const auto predicate = advancement::DamageSourcePredicate::fromJson(json);
    ASSERT_TRUE(predicate.success());
    // vanilla 的岩浆怪掉落表用 inverted 包住该条件：非青蛙击杀掉黏液球
    EXPECT_FALSE(predicate.value().test(source));
}

TEST_F(FrogFroglightDamageSourcePredicateTest, DamageTypeId_MatchesMobAttack)
{
    const nlohmann::json json = {{"type", "minecraft:mob_attack"}};
    const auto predicate = advancement::DamageSourcePredicate::fromJson(json);
    ASSERT_TRUE(predicate.success());

    EntityDamageSource mobAttack(DamageType::MobAttack, nullptr);
    EntityDamageSource fall(DamageType::Fall, nullptr);
    EXPECT_TRUE(predicate.value().test(mobAttack));
    EXPECT_FALSE(predicate.value().test(fall));
}

TEST_F(FrogFroglightDamageSourcePredicateTest, SourceEntityComponents_MatchFrogVariant)
{
    // 对齐数据包岩浆怪掉落表：
    //   {"source_entity": {"type": "minecraft:frog",
    //                      "components": {"minecraft:frog/variant": "minecraft:warm"}}}
    FrogEntity warmFrog(EntityInstanceId(1), mc::test::testEcsRegistry());
    warmFrog.setTypeId(entity::EntityTypeKeys::FROG); // 绕过工厂直接构造时 m_typeId 为空
    warmFrog.setVariant(FrogEntity::Variant::Warm);
    EntityDamageSource warmAttack(DamageType::MobAttack, &warmFrog);

    FrogEntity coldFrog(EntityInstanceId(2), mc::test::testEcsRegistry());
    coldFrog.setTypeId(entity::EntityTypeKeys::FROG);
    coldFrog.setVariant(FrogEntity::Variant::Cold);
    EntityDamageSource coldAttack(DamageType::MobAttack, &coldFrog);

    const nlohmann::json json = {{"source_entity",
        {{"type", entity::EntityTypeKeys::FROG}, {"components", {{"minecraft:frog/variant", "minecraft:warm"}}}}}};
    const auto predicate = advancement::DamageSourcePredicate::fromJson(json);
    ASSERT_TRUE(predicate.success());

    // 温暖青蛙捕食掉珠光蛙明灯；寒冷青蛙不掉
    EXPECT_TRUE(predicate.value().test(warmAttack));
    EXPECT_FALSE(predicate.value().test(coldAttack));
}

TEST_F(FrogFroglightDamageSourcePredicateTest, TypeTags_ObjectFormMatches)
{
    EntityDamageSource mobAttack(DamageType::MobAttack, nullptr);
    EntityDamageSource fall(DamageType::Fall, nullptr);

    // vanilla tags 结构：[{"id": "minecraft:is_projectile", "expected": true}]
    const nlohmann::json projectileJson = {
        {"tags", nlohmann::json::array({{{"id", "minecraft:is_projectile"}, {"expected", true}}})}};
    const auto projectilePredicate = advancement::DamageSourcePredicate::fromJson(projectileJson);
    ASSERT_TRUE(projectilePredicate.success());
    EXPECT_FALSE(projectilePredicate.value().isAny());
    EXPECT_FALSE(projectilePredicate.value().test(fall));
}

TEST_F(FrogFroglightDamageSourcePredicateTest, TypeSpecific_SlimeSizeRange)
{
    // 对齐数据包岩浆怪掉落表：{"type_specific": {"type": "minecraft:slime", "size": {"min": 2}}}
    SlimeEntity bigSlime(EntityInstanceId(1), mc::test::testEcsRegistry());
    bigSlime.setTypeId(entity::EntityTypeKeys::SLIME);
    bigSlime.setSlimeSize(3, false);
    SlimeEntity smallSlime(EntityInstanceId(2), mc::test::testEcsRegistry());
    smallSlime.setTypeId(entity::EntityTypeKeys::SLIME);
    smallSlime.setSlimeSize(1, false);

    const nlohmann::json json = {{"type_specific", {{"type", "minecraft:slime"}, {"size", {{"min", 2}}}}}};
    const auto predicate = advancement::EntityPredicate::fromJson(json);
    ASSERT_TRUE(predicate.success());
    EXPECT_FALSE(predicate.value().isAny());
    EXPECT_TRUE(predicate.value().test(bigSlime));
    EXPECT_FALSE(predicate.value().test(smallSlime));
}

TEST_F(FrogFroglightDamageSourcePredicateTest, TypeSpecific_SheepSheared)
{
    SheepEntity sheep(EntityInstanceId(1), mc::test::testEcsRegistry());
    sheep.setTypeId(entity::EntityTypeKeys::SHEEP);
    sheep.setSheared(false);

    // 对齐数据包绵羊毛掉落表：{"type_specific": {"type": "minecraft:sheep", "sheared": false}}
    const nlohmann::json json = {{"type_specific", {{"type", "minecraft:sheep"}, {"sheared", false}}}};
    const auto predicate = advancement::EntityPredicate::fromJson(json);
    ASSERT_TRUE(predicate.success());
    EXPECT_TRUE(predicate.value().test(sheep));

    sheep.setSheared(true);
    EXPECT_FALSE(predicate.value().test(sheep));
}

TEST_F(FrogFroglightDamageSourcePredicateTest, Vehicle_MatchesRiddenEntity)
{
    // 对齐数据包：尸壳掉落表 {"vehicle": {"type": "minecraft:camel_husk"}}
    const nlohmann::json json = {{"vehicle", {{"type", "minecraft:camel_husk"}}}};
    const auto predicate = advancement::EntityPredicate::fromJson(json);
    ASSERT_TRUE(predicate.success());
    EXPECT_FALSE(predicate.value().isAny());
    ASSERT_NE(predicate.value().getVehicle(), nullptr);

    // 无载具的实体不匹配（世界桩 getEntity 返回 nullptr）
    FakeAttacker husk(EntityInstanceId(1), mc::test::testEcsRegistry());
    husk.setTypeId(entity::EntityTypeKeys::HUSK);
    VehicleTestWorld world;
    EXPECT_FALSE(predicate.value().test(world, 0.0, 64.0, 0.0, husk));
}

TEST_F(FrogFroglightDamageSourcePredicateTest, BooleanOnlyJson_StillAnyWhenNoFields)
{
    const auto predicate = advancement::DamageSourcePredicate::fromJson(nlohmann::json::object());
    ASSERT_TRUE(predicate.success());
    EXPECT_TRUE(predicate.value().isAny());
}

} // namespace
} // namespace mc