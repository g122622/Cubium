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
 * @file CriterionContextTest.cpp
 * @brief ContextAwarePredicate / CriterionContext 单元测试
 *
 * 数据包进度条件大量使用 `[{"condition": "minecraft:entity_properties", ...}]` 写法
 * （wolf_armor、ride_a_boat_with_a_goat、lightning_rod_with_villager_no_fire 等）。
 * 此前项目完全没有该上下文层，条件被静默忽略、进度在错误场景下被达成。
 */

#include "common/advancement/trigger/conditions/CriterionContext.hpp"
#include "common/TestWorldHelper.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/passive/basic/SheepEntity.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

namespace mc {
namespace {

class CriterionContextTest : public ::testing::Test {
protected:
    void SetUp() override { Items::initialize(); }
};

TEST_F(CriterionContextTest, NullJsonMatchesAny)
{
    auto result = advancement::ContextAwarePredicate::fromJson(nlohmann::json());
    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().isAny());
    EXPECT_TRUE(result.value().test(advancement::CriterionContext{}));
}

TEST_F(CriterionContextTest, EntityPropertiesArray_MatchesThisEntityType)
{
    // 对齐数据包 husbandry/remove_wolf_armor：
    //   [{"condition": "minecraft:entity_properties", "entity": "this",
    //     "predicate": {"type": "minecraft:wolf"}}]
    const nlohmann::json json = nlohmann::json::array({{{"condition", "minecraft:entity_properties"},
        {"entity", "this"},
        {"predicate", {{"type", "minecraft:wolf"}}}}});
    auto result = advancement::ContextAwarePredicate::fromJson(json);
    ASSERT_TRUE(result.success());
    EXPECT_FALSE(result.value().isAny());

    // 普通僵尸不是狼 → 不匹配（旧实现忽略整个条件，任何实体都会通过）
    ZombieEntity zombie(EntityInstanceId(1), mc::test::testEcsRegistry());
    zombie.setTypeId(entity::EntityTypeKeys::ZOMBIE);
    advancement::CriterionContext context;
    context.entity = &zombie;
    EXPECT_FALSE(result.value().test(context));

    // 无实体上下文 → 不匹配
    EXPECT_FALSE(result.value().test(advancement::CriterionContext{}));
}

TEST_F(CriterionContextTest, EntityProperties_MatchesCorrectEntity)
{
    const nlohmann::json json = nlohmann::json::array({{{"condition", "minecraft:entity_properties"},
        {"entity", "this"},
        {"predicate", {{"type", "minecraft:zombie"}}}}});
    auto result = advancement::ContextAwarePredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    ZombieEntity zombie(EntityInstanceId(1), mc::test::testEcsRegistry());
    zombie.setTypeId(entity::EntityTypeKeys::ZOMBIE);
    advancement::CriterionContext context;
    context.entity = &zombie;
    EXPECT_TRUE(result.value().test(context));

    SheepEntity sheep(EntityInstanceId(2), mc::test::testEcsRegistry());
    sheep.setTypeId(entity::EntityTypeKeys::SHEEP);
    advancement::CriterionContext sheepContext;
    sheepContext.entity = &sheep;
    EXPECT_FALSE(result.value().test(sheepContext));
}

TEST_F(CriterionContextTest, EntityProperties_KillerTarget)
{
    const nlohmann::json json = nlohmann::json::array({{{"condition", "minecraft:entity_properties"},
        {"entity", "killer"},
        {"predicate", {{"type", "minecraft:zombie"}}}}});
    auto result = advancement::ContextAwarePredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    SheepEntity victim(EntityInstanceId(2), mc::test::testEcsRegistry());
    victim.setTypeId(entity::EntityTypeKeys::SHEEP);
    ZombieEntity killer(EntityInstanceId(1), mc::test::testEcsRegistry());
    killer.setTypeId(entity::EntityTypeKeys::ZOMBIE);
    advancement::CriterionContext context;
    context.entity = &victim;
    context.killer = &killer;
    EXPECT_TRUE(result.value().test(context));
}

TEST_F(CriterionContextTest, MatchTool_UsesContextItem)
{
    // 对齐数据包 allay_deliver_cake_to_note_block 中的 match_tool 条件
    const nlohmann::json json =
        nlohmann::json::array({{{"condition", "minecraft:match_tool"}, {"predicate", {{"items", "minecraft:cake"}}}}});
    auto result = advancement::ContextAwarePredicate::fromJson(json);
    ASSERT_TRUE(result.success());

    ASSERT_NE(Items::CAKE, nullptr);
    ASSERT_NE(Items::BRICK, nullptr);
    const ItemStack cake(*Items::CAKE, 1);
    const ItemStack brick(*Items::BRICK, 1);

    advancement::CriterionContext context;
    context.item = &cake;
    EXPECT_TRUE(result.value().test(context));

    advancement::CriterionContext wrong;
    wrong.item = &brick;
    EXPECT_FALSE(result.value().test(wrong));

    // 无物品上下文 → 不匹配
    EXPECT_FALSE(result.value().test(advancement::CriterionContext{}));
}

TEST_F(CriterionContextTest, AllOfAndInverted)
{
    nlohmann::json zombieTerm = nlohmann::json{{"condition", "minecraft:entity_properties"},
        {"entity", "this"},
        {"predicate", {{"type", "minecraft:zombie"}}}};
    nlohmann::json toolTerm =
        nlohmann::json{{"condition", "minecraft:match_tool"}, {"predicate", {{"items", "minecraft:brick"}}}};
    nlohmann::json allOf = nlohmann::json::array();
    allOf.push_back(
        nlohmann::json{{"condition", "minecraft:all_of"}, {"terms", nlohmann::json::array({zombieTerm, toolTerm})}});
    auto result = advancement::ContextAwarePredicate::fromJson(allOf);
    ASSERT_TRUE(result.success());

    ZombieEntity zombie(EntityInstanceId(1), mc::test::testEcsRegistry());
    zombie.setTypeId(entity::EntityTypeKeys::ZOMBIE);
    ASSERT_NE(Items::BRICK, nullptr);
    const ItemStack brick(*Items::BRICK, 1);
    advancement::CriterionContext context;
    context.entity = &zombie;
    context.item = &brick;
    EXPECT_TRUE(result.value().test(context));

    // 缺少砖头 → all_of 失败
    advancement::CriterionContext withoutItem;
    withoutItem.entity = &zombie;
    EXPECT_FALSE(result.value().test(withoutItem));

    nlohmann::json inverted = nlohmann::json::array();
    inverted.push_back(nlohmann::json{{"condition", "minecraft:inverted"},
        {"term",
            nlohmann::json{{"condition", "minecraft:entity_properties"},
                {"entity", "this"},
                {"predicate", {{"type", "minecraft:zombie"}}}}}});
    auto invertedResult = advancement::ContextAwarePredicate::fromJson(inverted);
    ASSERT_TRUE(invertedResult.success());
    EXPECT_FALSE(invertedResult.value().test(context));
    advancement::CriterionContext empty;
    EXPECT_TRUE(invertedResult.value().test(empty));
}

TEST_F(CriterionContextTest, UnsupportedConditionIsRejected)
{
    // 未知条件类型不得按"永真"放行（否则进度在错误场景下达成）
    const nlohmann::json json = nlohmann::json::array({{{"condition", "minecraft:unknown_condition"}}});
    auto result = advancement::ContextAwarePredicate::fromJson(json);
    EXPECT_TRUE(result.failed());
}

} // namespace
} // namespace mc