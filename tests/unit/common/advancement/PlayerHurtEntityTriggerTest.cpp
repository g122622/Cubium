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
 * @file PlayerHurtEntityTriggerTest.cpp
 * @brief player_hurt_entity 触发器与 DamagePredicate 单元测试
 *
 * 对齐 vanilla PlayerHurtEntityTrigger / DamagePredicate。数据包
 * adventure/overoverkill（重锤单次 100 点伤害）、shoot_arrow、throw_trident
 * 依赖该触发器；此前未实现且 DamagePredicate 整类缺失。
 */

#include "common/TestWorldHelper.hpp"
#include "common/advancement/Criterion.hpp"
#include "common/advancement/trigger/CriterionTriggers.hpp"
#include "common/advancement/trigger/conditions/DamagePredicate.hpp"
#include "common/advancement/trigger/impl/EntityTriggers.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

namespace mc {
namespace {

class PlayerHurtEntityTriggerTest : public ::testing::Test {
protected:
    void SetUp() override { advancement::CriterionTriggers::instance().registerBuiltinTriggers(); }

    void TearDown() override { advancement::CriterionTriggers::instance().clear(); }
};

TEST_F(PlayerHurtEntityTriggerTest, IsRegistered)
{
    EXPECT_TRUE(
        advancement::CriterionTriggers::instance().hasTrigger(ResourceLocation("minecraft:player_hurt_entity")));
}

TEST_F(PlayerHurtEntityTriggerTest, DamagePredicate_DealtBounds)
{
    // adventure/overoverkill：{"damage": {"dealt": {"min": 100.0}}}
    auto result = advancement::DamagePredicate::fromJson(nlohmann::json{{"dealt", {{"min", 100.0}}}});
    ASSERT_TRUE(result.success());
    const advancement::DamagePredicate predicate = result.value();
    EXPECT_FALSE(predicate.isAny());

    EntityDamageSource source(DamageType::MobAttack, nullptr);
    EXPECT_TRUE(predicate.test(source, 100.0f, 100.0f, false));
    EXPECT_TRUE(predicate.test(source, 120.0f, 120.0f, false));
    EXPECT_FALSE(predicate.test(source, 99.0f, 99.0f, false));
}

TEST_F(PlayerHurtEntityTriggerTest, DamagePredicate_TakenAndDealtAreIndependent)
{
    auto result =
        advancement::DamagePredicate::fromJson(nlohmann::json{{"dealt", {{"min", 10.0}}}, {"taken", {{"max", 5.0}}}});
    ASSERT_TRUE(result.success());
    const advancement::DamagePredicate predicate = result.value();
    EntityDamageSource source(DamageType::MobAttack, nullptr);
    EXPECT_TRUE(predicate.test(source, 10.0f, 5.0f, false));
    EXPECT_FALSE(predicate.test(source, 10.0f, 5.5f, false));
    EXPECT_FALSE(predicate.test(source, 9.0f, 5.0f, false));
}

TEST_F(PlayerHurtEntityTriggerTest, DamagePredicate_BlockedFlag)
{
    auto result = advancement::DamagePredicate::fromJson(nlohmann::json{{"blocked", true}});
    ASSERT_TRUE(result.success());
    const advancement::DamagePredicate predicate = result.value();
    EntityDamageSource source(DamageType::MobAttack, nullptr);
    EXPECT_TRUE(predicate.test(source, 1.0f, 1.0f, true));
    EXPECT_FALSE(predicate.test(source, 1.0f, 1.0f, false));
}

TEST_F(PlayerHurtEntityTriggerTest, DamagePredicate_TypeMatchesMaseSmashTag)
{
    // adventure/overoverkill 的 type.tags 写法：[{"id": "minecraft:mace_smash", "expected": true}]
    const nlohmann::json json = {
        {"dealt", {{"min", 100.0}}},
        {"type", {{"tags", nlohmann::json::array({{{"id", "minecraft:mace_smash"}, {"expected", true}}})}}},
    };
    auto result = advancement::DamagePredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    const advancement::DamagePredicate predicate = result.value();
    EXPECT_FALSE(predicate.isAny());

    EntityDamageSource mobAttack(DamageType::MobAttack, nullptr);
    // 未打标签的 mob_attack 不属于 mace_smash
    EXPECT_FALSE(predicate.test(mobAttack, 100.0f, 100.0f, false));
}

TEST_F(PlayerHurtEntityTriggerTest, TriggerInstance_DamageAndEntity)
{
    // 条件同时含 damage 与 entity：两者都须满足
    const nlohmann::json json = {
        {"trigger", "minecraft:player_hurt_entity"},
        {"conditions", {{"damage", {{"dealt", {{"min", 4.0}}}}}, {"entity", {{"type", "minecraft:zombie"}}}}},
    };
    auto criterion = advancement::Criterion::fromJson("hurt_zombie", json);
    ASSERT_TRUE(criterion.success());
    auto instance =
        std::dynamic_pointer_cast<advancement::PlayerHurtEntityTriggerInstance>(criterion.value().getTriggerInstance());
    ASSERT_NE(instance, nullptr);

    ZombieEntity zombie(EntityInstanceId(1), mc::test::testEcsRegistry());
    zombie.setTypeId(entity::EntityTypeKeys::ZOMBIE);
    EntityDamageSource source(DamageType::MobAttack, nullptr);

    EXPECT_TRUE(instance->test(source, 5.0f, 5.0f, false, zombie));
    EXPECT_FALSE(instance->test(source, 3.0f, 3.0f, false, zombie));
}

TEST_F(PlayerHurtEntityTriggerTest, TriggerInstance_EntityMismatch)
{
    advancement::PlayerHurtEntityTriggerInstance instance;
    ASSERT_TRUE(instance.fromJson(nlohmann::json{{"entity", {{"type", "minecraft:creeper"}}}}).success());

    ZombieEntity zombie(EntityInstanceId(1), mc::test::testEcsRegistry());
    zombie.setTypeId(entity::EntityTypeKeys::ZOMBIE);
    EntityDamageSource source(DamageType::MobAttack, nullptr);
    EXPECT_FALSE(instance.test(source, 10.0f, 10.0f, false, zombie));
}

TEST_F(PlayerHurtEntityTriggerTest, ConditionsToJsonRoundTrip)
{
    advancement::PlayerHurtEntityTriggerInstance instance;
    ASSERT_TRUE(instance.fromJson(nlohmann::json{{"damage", {{"dealt", {{"min", 100.0}}}}}}).success());
    const nlohmann::json roundTrip = instance.conditionsToJson();
    ASSERT_TRUE(roundTrip.contains("damage"));
    ASSERT_TRUE(roundTrip["damage"].contains("dealt"));
}

TEST_F(PlayerHurtEntityTriggerTest, EntityHurtPlayer_Registered)
{
    EXPECT_TRUE(
        advancement::CriterionTriggers::instance().hasTrigger(ResourceLocation("minecraft:entity_hurt_player")));
}

TEST_F(PlayerHurtEntityTriggerTest, EntityHurtPlayer_DamageCondition)
{
    // 与 player_hurt_entity 成对，条件只有可选的 damage 谓词
    const nlohmann::json json = {
        {"trigger", "minecraft:entity_hurt_player"},
        {"conditions", {{"damage", {{"dealt", {{"min", 4.0}}}}}}},
    };
    auto criterion = advancement::Criterion::fromJson("hurt_by_zombie", json);
    ASSERT_TRUE(criterion.success());
    auto instance =
        std::dynamic_pointer_cast<advancement::EntityHurtPlayerTriggerInstance>(criterion.value().getTriggerInstance());
    ASSERT_NE(instance, nullptr);

    EntityDamageSource source(DamageType::MobAttack, nullptr);
    EXPECT_TRUE(instance->test(source, 5.0f, 5.0f, false));
    EXPECT_FALSE(instance->test(source, 3.0f, 3.0f, false));
}

TEST_F(PlayerHurtEntityTriggerTest, EntityHurtPlayer_NoDamageMatchesAny)
{
    auto* trigger = advancement::CriterionTriggers::instance().getTrigger<advancement::EntityHurtPlayerTrigger>();
    ASSERT_NE(trigger, nullptr);
    auto result = trigger->fromJson(nlohmann::json::object());
    ASSERT_TRUE(result.success());
    auto instance = std::dynamic_pointer_cast<advancement::EntityHurtPlayerTriggerInstance>(result.value());
    ASSERT_NE(instance, nullptr);
    EntityDamageSource source(DamageType::Fall, nullptr);
    EXPECT_TRUE(instance->test(source, 1.0f, 1.0f, false));
}

TEST_F(PlayerHurtEntityTriggerTest, ChangedDimension_Registered)
{
    EXPECT_TRUE(advancement::CriterionTriggers::instance().hasTrigger(ResourceLocation("minecraft:changed_dimension")));
}

TEST_F(PlayerHurtEntityTriggerTest, ChangedDimension_ToOnly)
{
    // 数据包 end/root：{"to": "minecraft:the_end"}
    const nlohmann::json json = {
        {"trigger", "minecraft:changed_dimension"},
        {"conditions", {{"to", "minecraft:the_end"}}},
    };
    auto criterion = advancement::Criterion::fromJson("entered_end", json);
    ASSERT_TRUE(criterion.success());
    auto instance =
        std::dynamic_pointer_cast<advancement::ChangedDimensionTriggerInstance>(criterion.value().getTriggerInstance());
    ASSERT_NE(instance, nullptr);

    const ResourceLocation overworld("minecraft:overworld");
    const ResourceLocation nether("minecraft:the_nether");
    const ResourceLocation end("minecraft:the_end");
    EXPECT_TRUE(instance->test(overworld, end));
    EXPECT_TRUE(instance->test(nether, end));
    EXPECT_FALSE(instance->test(overworld, nether));
}

TEST_F(PlayerHurtEntityTriggerTest, ChangedDimension_FromAndTo)
{
    advancement::ChangedDimensionTriggerInstance instance;
    ASSERT_TRUE(
        instance.fromJson(nlohmann::json{{"from", "minecraft:the_nether"}, {"to", "minecraft:overworld"}}).success());

    const ResourceLocation overworld("minecraft:overworld");
    const ResourceLocation nether("minecraft:the_nether");
    EXPECT_TRUE(instance.test(nether, overworld));
    EXPECT_FALSE(instance.test(overworld, nether));
}

TEST_F(PlayerHurtEntityTriggerTest, ChangedDimension_NoConditionsMatchesAny)
{
    advancement::ChangedDimensionTriggerInstance instance;
    ASSERT_TRUE(instance.fromJson(nlohmann::json::object()).success());
    const ResourceLocation overworld("minecraft:overworld");
    EXPECT_TRUE(instance.test(overworld, overworld));
    const nlohmann::json roundTrip = instance.conditionsToJson();
    EXPECT_TRUE(roundTrip.empty());
}

TEST_F(PlayerHurtEntityTriggerTest, FishingRodHooked_Registered)
{
    EXPECT_TRUE(
        advancement::CriterionTriggers::instance().hasTrigger(ResourceLocation("minecraft:fishing_rod_hooked")));
}

TEST_F(PlayerHurtEntityTriggerTest, FishingRodHooked_MatchesCaughtItem)
{
    // 对齐数据包 husbandry/fishish_business 的写法，此处用内置 #decorated_pot_sherds 标签
    // （#fishes 由数据包注入，单元测试不加载数据包）
    const nlohmann::json json = {
        {"trigger", "minecraft:fishing_rod_hooked"},
        {"conditions", {{"item", {{"tag", "minecraft:decorated_pot_sherds"}}}}},
    };
    auto criterion = advancement::Criterion::fromJson("fish_caught", json);
    ASSERT_TRUE(criterion.success());
    auto instance =
        std::dynamic_pointer_cast<advancement::FishingRodHookedTriggerInstance>(criterion.value().getTriggerInstance());
    ASSERT_NE(instance, nullptr);

    Items::initialize();
    item::tag::ItemTags::initialize();
    ASSERT_NE(Items::FISHING_ROD, nullptr);
    const ItemStack rod(*Items::FISHING_ROD, 1);

    const Item* sherd = ItemRegistry::instance().getItem(ResourceLocation("minecraft:heart_pottery_sherd"));
    ASSERT_NE(sherd, nullptr);
    EXPECT_TRUE(instance->test(rod, nullptr, {ItemStack(*sherd, 1)}));

    ASSERT_NE(Items::DIAMOND, nullptr);
    EXPECT_FALSE(instance->test(rod, nullptr, {ItemStack(*Items::DIAMOND, 1)}));
    // 什么都没钓到（钩住非物品实体）不匹配
    EXPECT_FALSE(instance->test(rod, nullptr, {}));
}

TEST_F(PlayerHurtEntityTriggerTest, FishingRodHooked_RodCondition)
{
    // 条件限定钓竿时，需与使用中的钓竿一致；无 item 条件时空掉落也匹配
    advancement::FishingRodHookedTriggerInstance instance;
    ASSERT_TRUE(instance.fromJson(nlohmann::json{{"rod", {{"items", "minecraft:fishing_rod"}}}}).success());
    Items::initialize();
    ASSERT_NE(Items::FISHING_ROD, nullptr);
    ASSERT_NE(Items::DIAMOND, nullptr);
    EXPECT_TRUE(instance.test(ItemStack(*Items::FISHING_ROD, 1), nullptr, {}));
    EXPECT_FALSE(instance.test(ItemStack(*Items::DIAMOND, 1), nullptr, {}));
}

TEST_F(PlayerHurtEntityTriggerTest, FishingRodHooked_EntityCondition)
{
    advancement::FishingRodHookedTriggerInstance instance;
    ASSERT_TRUE(instance.fromJson(nlohmann::json{{"entity", {{"type", "minecraft:zombie"}}}}).success());
    ASSERT_NE(Items::FISHING_ROD, nullptr);
    const ItemStack rod(*Items::FISHING_ROD, 1);

    ZombieEntity zombie(EntityInstanceId(1), mc::test::testEcsRegistry());
    zombie.setTypeId(entity::EntityTypeKeys::ZOMBIE);
    EXPECT_TRUE(instance.test(rod, &zombie, {}));
    EXPECT_FALSE(instance.test(rod, nullptr, {}));
}

TEST_F(PlayerHurtEntityTriggerTest, KilledByArrow_Registered)
{
    EXPECT_TRUE(advancement::CriterionTriggers::instance().hasTrigger(ResourceLocation("minecraft:killed_by_arrow")));
}

TEST_F(PlayerHurtEntityTriggerTest, KilledByArrow_UniqueEntityTypesAndWeapon)
{
    // 弩多杀进度写法：{"victims": [], "unique_entity_types": {"min": 2},
    //                 "fired_from_weapon": {"items": "minecraft:crossbow"}}
    const nlohmann::json json = {
        {"trigger", "minecraft:killed_by_arrow"},
        {"conditions",
            {{"unique_entity_types", {{"min", 2}}}, {"fired_from_weapon", {{"items", "minecraft:crossbow"}}}}},
    };
    auto criterion = advancement::Criterion::fromJson("crossbow_multi_kill", json);
    ASSERT_TRUE(criterion.success());
    auto instance =
        std::dynamic_pointer_cast<advancement::KilledByArrowTriggerInstance>(criterion.value().getTriggerInstance());
    ASSERT_NE(instance, nullptr);

    Items::initialize();
    ASSERT_NE(Items::CROSSBOW, nullptr);
    ASSERT_NE(Items::BOW, nullptr);
    const ItemStack crossbow(*Items::CROSSBOW, 1);
    const ItemStack bow(*Items::BOW, 1);

    ZombieEntity zombie(EntityInstanceId(1), mc::test::testEcsRegistry());
    zombie.setTypeId(entity::EntityTypeKeys::ZOMBIE);
    ZombieEntity zombie2(EntityInstanceId(2), mc::test::testEcsRegistry());
    zombie2.setTypeId(entity::EntityTypeKeys::ZOMBIE);
    ZombieEntity skeleton(EntityInstanceId(3), mc::test::testEcsRegistry());
    skeleton.setTypeId(entity::EntityTypeKeys::SKELETON);

    // 两种不同实体类型 → 满足 min 2
    EXPECT_TRUE(instance->test({&zombie, &skeleton}, crossbow));
    // 同种类型仅计 1 → 不满足
    EXPECT_FALSE(instance->test({&zombie, &zombie2}, crossbow));
    // 武器不是弩 → 不匹配
    EXPECT_FALSE(instance->test({&zombie, &skeleton}, bow));
    // 武器缺失（直接放置的箭矢）→ 带 fired_from_weapon 的条件不匹配
    EXPECT_FALSE(instance->test({&zombie, &skeleton}, ItemStack()));
}

TEST_F(PlayerHurtEntityTriggerTest, KilledByArrow_VictimsMultisetMatch)
{
    advancement::KilledByArrowTriggerInstance instance;
    ASSERT_TRUE(
        instance.fromJson(nlohmann::json{{"victims", nlohmann::json::array({{{"type", "minecraft:skeleton"}}})}})
            .success());
    ZombieEntity zombie(EntityInstanceId(1), mc::test::testEcsRegistry());
    zombie.setTypeId(entity::EntityTypeKeys::ZOMBIE);
    ZombieEntity skeleton(EntityInstanceId(3), mc::test::testEcsRegistry());
    skeleton.setTypeId(entity::EntityTypeKeys::SKELETON);
    // 无武器条件时任意武器（含空栈）都通过
    EXPECT_TRUE(instance.test({&skeleton}, ItemStack()));
    EXPECT_FALSE(instance.test({&zombie}, ItemStack()));
}

} // namespace
} // namespace mc