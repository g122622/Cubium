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
 * @file EntityArgumentTest.cpp
 * @brief EntityArgument 和 EntitySelector 单元测试
 *
 * 测试实体选择器参数解析，包括：
 * - FloatRange 解析
 * - IntRange 解析
 * - 选择器类型解析
 * - x_rotation 和 y_rotation 角度范围解析
 */

#include <gtest/gtest.h>

#include "common/command/StringReader.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"

using mc::command::CommandException;
using mc::command::EntityArgumentType;
using mc::command::EntitySelector;
using mc::command::EntitySelectorSort;
using mc::command::EntitySelectorType;
using mc::command::FloatRange;
using mc::command::IntRange;
using mc::command::StringReader;

// ========== FloatRange 测试 ==========

class FloatRangeTest : public ::testing::Test {
protected:
    FloatRange range;
};

TEST_F(FloatRangeTest, DefaultUnbounded)
{
    EXPECT_TRUE(range.isUnbounded());
    EXPECT_FALSE(range.hasMin());
    EXPECT_FALSE(range.hasMax());
}

TEST_F(FloatRangeTest, SetMinOnly)
{
    range.setMin(10.0f);
    EXPECT_FALSE(range.isUnbounded());
    EXPECT_TRUE(range.hasMin());
    EXPECT_FALSE(range.hasMax());
    EXPECT_FLOAT_EQ(range.getMin(), 10.0f);
}

TEST_F(FloatRangeTest, SetMaxOnly)
{
    range.setMax(20.0f);
    EXPECT_FALSE(range.isUnbounded());
    EXPECT_FALSE(range.hasMin());
    EXPECT_TRUE(range.hasMax());
    EXPECT_FLOAT_EQ(range.getMax(), 20.0f);
}

TEST_F(FloatRangeTest, SetBothBounds)
{
    range.setMin(10.0f);
    range.setMax(20.0f);
    EXPECT_FALSE(range.isUnbounded());
    EXPECT_TRUE(range.hasMin());
    EXPECT_TRUE(range.hasMax());
    EXPECT_FLOAT_EQ(range.getMin(), 10.0f);
    EXPECT_FLOAT_EQ(range.getMax(), 20.0f);
}

TEST_F(FloatRangeTest, TestWithinRange)
{
    range.setMin(10.0f);
    range.setMax(20.0f);
    EXPECT_FALSE(range.test(9.9f));
    EXPECT_TRUE(range.test(10.0f));
    EXPECT_TRUE(range.test(15.0f));
    EXPECT_TRUE(range.test(20.0f));
    EXPECT_FALSE(range.test(20.1f));
}

TEST_F(FloatRangeTest, TestSquared)
{
    range.setMin(3.0f);
    range.setMax(5.0f);
    // 距离 4 在范围内，平方为 16
    EXPECT_TRUE(range.testSquared(16.0f));
    // 距离 2 不在范围内，平方为 4
    EXPECT_FALSE(range.testSquared(4.0f));
    // 距离 6 不在范围内，平方为 36
    EXPECT_FALSE(range.testSquared(36.0f));
}

// ========== IntRange 测试 ==========

class IntRangeTest : public ::testing::Test {
protected:
    IntRange range;
};

TEST_F(IntRangeTest, DefaultUnbounded)
{
    EXPECT_TRUE(range.isUnbounded());
    EXPECT_FALSE(range.hasMin());
    EXPECT_FALSE(range.hasMax());
}

TEST_F(IntRangeTest, SetBothBounds)
{
    range.setMin(10);
    range.setMax(20);
    EXPECT_FALSE(range.isUnbounded());
    EXPECT_TRUE(range.hasMin());
    EXPECT_TRUE(range.hasMax());
    EXPECT_EQ(range.getMin(), 10);
    EXPECT_EQ(range.getMax(), 20);
}

TEST_F(IntRangeTest, TestWithinRange)
{
    range.setMin(10);
    range.setMax(20);
    EXPECT_FALSE(range.test(9));
    EXPECT_TRUE(range.test(10));
    EXPECT_TRUE(range.test(15));
    EXPECT_TRUE(range.test(20));
    EXPECT_FALSE(range.test(21));
}

// ========== EntitySelector 基础测试 ==========

class EntitySelectorTest : public ::testing::Test {
protected:
    EntitySelector selector;
};

TEST_F(EntitySelectorTest, DefaultConstructor)
{
    EXPECT_EQ(selector.type(), EntitySelectorType::SinglePlayer);
    EXPECT_EQ(selector.limit(), INT32_MAX);
    EXPECT_FALSE(selector.isSelf());
    EXPECT_FALSE(selector.includesNonPlayers());
    EXPECT_TRUE(selector.isSingle());
}

TEST_F(EntitySelectorTest, SelfFactory)
{
    selector = EntitySelector::self();
    EXPECT_EQ(selector.type(), EntitySelectorType::Self);
    EXPECT_TRUE(selector.isSelf());
    EXPECT_TRUE(selector.isSingle());
}

TEST_F(EntitySelectorTest, NearestPlayerFactory)
{
    selector = EntitySelector::nearestPlayer();
    EXPECT_EQ(selector.type(), EntitySelectorType::SinglePlayer);
    EXPECT_EQ(selector.limit(), 1);
    EXPECT_EQ(selector.sort(), EntitySelectorSort::Nearest);
}

TEST_F(EntitySelectorTest, AllPlayersFactory)
{
    selector = EntitySelector::allPlayers();
    EXPECT_EQ(selector.type(), EntitySelectorType::AllPlayers);
    EXPECT_FALSE(selector.isSingle());
}

TEST_F(EntitySelectorTest, AllEntitiesFactory)
{
    selector = EntitySelector::allEntities();
    EXPECT_EQ(selector.type(), EntitySelectorType::AllEntities);
    EXPECT_TRUE(selector.includesNonPlayers());
}

TEST_F(EntitySelectorTest, RandomPlayerFactory)
{
    selector = EntitySelector::randomPlayer();
    EXPECT_EQ(selector.type(), EntitySelectorType::RandomPlayer);
    EXPECT_TRUE(selector.isSingle());
    EXPECT_EQ(selector.limit(), 1);
    EXPECT_EQ(selector.sort(), EntitySelectorSort::Random);
}

TEST_F(EntitySelectorTest, ByUsernameFactory)
{
    selector = EntitySelector::byUsername("Steve");
    EXPECT_EQ(selector.type(), EntitySelectorType::SinglePlayer);
    EXPECT_TRUE(selector.hasUsername());
    EXPECT_EQ(selector.username(), "Steve");
}

TEST_F(EntitySelectorTest, SetDistance)
{
    selector.distance().setMin(10.0f);
    selector.distance().setMax(50.0f);
    EXPECT_FALSE(selector.distance().isUnbounded());
    EXPECT_TRUE(selector.distance().hasMin());
    EXPECT_TRUE(selector.distance().hasMax());
}

TEST_F(EntitySelectorTest, SetLevel)
{
    selector.level().setMin(10);
    selector.level().setMax(30);
    EXPECT_FALSE(selector.level().isUnbounded());
    EXPECT_TRUE(selector.level().hasMin());
    EXPECT_TRUE(selector.level().hasMax());
}

TEST_F(EntitySelectorTest, SetCoordinates)
{
    selector.setX(100.0f);
    selector.setY(64.0f);
    selector.setZ(-200.0f);
    EXPECT_TRUE(selector.hasX());
    EXPECT_TRUE(selector.hasY());
    EXPECT_TRUE(selector.hasZ());
    EXPECT_FLOAT_EQ(selector.getX(), 100.0f);
    EXPECT_FLOAT_EQ(selector.getY(), 64.0f);
    EXPECT_FLOAT_EQ(selector.getZ(), -200.0f);
}

TEST_F(EntitySelectorTest, SetDimensions)
{
    selector.setDx(10.0f);
    selector.setDy(5.0f);
    selector.setDz(10.0f);
    EXPECT_TRUE(selector.hasDx());
    EXPECT_TRUE(selector.hasDy());
    EXPECT_TRUE(selector.hasDz());
}

TEST_F(EntitySelectorTest, SetSort)
{
    selector.setSort(EntitySelectorSort::Furthest);
    EXPECT_EQ(selector.sort(), EntitySelectorSort::Furthest);
}

TEST_F(EntitySelectorTest, SetEntityType)
{
    selector.setEntityType("minecraft:zombie");
    EXPECT_TRUE(selector.hasEntityType());
    EXPECT_EQ(selector.entityType(), "minecraft:zombie");
    EXPECT_FALSE(selector.entityTypeNegated());
}

TEST_F(EntitySelectorTest, SetEntityTypeNegated)
{
    selector.setEntityType("minecraft:player", true);
    EXPECT_TRUE(selector.hasEntityType());
    EXPECT_TRUE(selector.entityTypeNegated());
}

TEST_F(EntitySelectorTest, AddTags)
{
    selector.addTag("foo", false);
    selector.addTag("bar", true);
    ASSERT_EQ(selector.tags().size(), 1);
    ASSERT_EQ(selector.tagsNegated().size(), 1);
    EXPECT_EQ(selector.tags()[0], "foo");
    EXPECT_EQ(selector.tagsNegated()[0], "bar");
}

TEST_F(EntitySelectorTest, SetGameMode)
{
    selector.setGameMode("survival");
    EXPECT_TRUE(selector.hasGameMode());
    EXPECT_EQ(selector.gameMode(), "survival");
    EXPECT_FALSE(selector.gameModeNegated());
}

TEST_F(EntitySelectorTest, SetTeam)
{
    selector.setTeam("red");
    EXPECT_TRUE(selector.hasTeam());
    EXPECT_EQ(selector.team(), "red");
    EXPECT_FALSE(selector.teamNegated());
}

// ========== EntitySelector 角度范围测试 ==========

class EntitySelectorRotationTest : public ::testing::Test {
protected:
    EntitySelector selector;
};

TEST_F(EntitySelectorRotationTest, DefaultRotationRangesAreUnbounded)
{
    EXPECT_TRUE(selector.xRotation().isUnbounded());
    EXPECT_TRUE(selector.yRotation().isUnbounded());
}

TEST_F(EntitySelectorRotationTest, SetXRotation)
{
    selector.xRotation().setMin(-45.0f);
    selector.xRotation().setMax(45.0f);

    EXPECT_FALSE(selector.xRotation().isUnbounded());
    EXPECT_TRUE(selector.xRotation().hasMin());
    EXPECT_TRUE(selector.xRotation().hasMax());
    EXPECT_FLOAT_EQ(selector.xRotation().getMin(), -45.0f);
    EXPECT_FLOAT_EQ(selector.xRotation().getMax(), 45.0f);
}

TEST_F(EntitySelectorRotationTest, SetYRotation)
{
    selector.yRotation().setMin(170.0f);
    selector.yRotation().setMax(-170.0f);

    EXPECT_FALSE(selector.yRotation().isUnbounded());
    EXPECT_TRUE(selector.yRotation().hasMin());
    EXPECT_TRUE(selector.yRotation().hasMax());
    EXPECT_FLOAT_EQ(selector.yRotation().getMin(), 170.0f);
    EXPECT_FLOAT_EQ(selector.yRotation().getMax(), -170.0f);
}

// ========== EntityArgumentType 解析测试 ==========

class EntityArgumentParseTest : public ::testing::Test {
protected:
    std::shared_ptr<EntityArgumentType> playerArg = EntityArgumentType::player();
    std::shared_ptr<EntityArgumentType> playersArg = EntityArgumentType::players();
    std::shared_ptr<EntityArgumentType> entityArg = EntityArgumentType::entity();
    std::shared_ptr<EntityArgumentType> entitiesArg = EntityArgumentType::entities();
};

TEST_F(EntityArgumentParseTest, ParseByUsername)
{
    StringReader reader("Steve");
    EntitySelector selector = playerArg->parse(reader);
    EXPECT_TRUE(selector.hasUsername());
    EXPECT_EQ(selector.username(), "Steve");
}

TEST_F(EntityArgumentParseTest, ParseAtP)
{
    StringReader reader("@p");
    EntitySelector selector = playerArg->parse(reader);
    EXPECT_EQ(selector.type(), EntitySelectorType::SinglePlayer);
    EXPECT_TRUE(selector.isSingle());
}

TEST_F(EntityArgumentParseTest, ParseAtA)
{
    StringReader reader("@a");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_EQ(selector.type(), EntitySelectorType::AllPlayers);
}

TEST_F(EntityArgumentParseTest, ParseAtE)
{
    StringReader reader("@e");
    EntitySelector selector = entitiesArg->parse(reader);
    EXPECT_EQ(selector.type(), EntitySelectorType::AllEntities);
    EXPECT_TRUE(selector.includesNonPlayers());
}

TEST_F(EntityArgumentParseTest, ParseAtR)
{
    StringReader reader("@r");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_EQ(selector.type(), EntitySelectorType::RandomPlayer);
}

TEST_F(EntityArgumentParseTest, ParseAtS)
{
    StringReader reader("@s");
    EntitySelector selector = playerArg->parse(reader);
    EXPECT_EQ(selector.type(), EntitySelectorType::Self);
    EXPECT_TRUE(selector.isSelf());
}

TEST_F(EntityArgumentParseTest, ParseInvalidSelectorThrows)
{
    StringReader reader("@x");
    EXPECT_THROW(playerArg->parse(reader), CommandException);
}

TEST_F(EntityArgumentParseTest, ParseDistanceRange)
{
    StringReader reader("@a[distance=10..20]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_FALSE(selector.distance().isUnbounded());
    EXPECT_TRUE(selector.distance().hasMin());
    EXPECT_TRUE(selector.distance().hasMax());
    EXPECT_FLOAT_EQ(selector.distance().getMin(), 10.0f);
    EXPECT_FLOAT_EQ(selector.distance().getMax(), 20.0f);
}

TEST_F(EntityArgumentParseTest, ParseDistanceMinOnly)
{
    StringReader reader("@a[distance=10..]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_TRUE(selector.distance().hasMin());
    EXPECT_FALSE(selector.distance().hasMax());
    EXPECT_FLOAT_EQ(selector.distance().getMin(), 10.0f);
}

TEST_F(EntityArgumentParseTest, ParseDistanceMaxOnly)
{
    StringReader reader("@a[distance=..20]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_FALSE(selector.distance().hasMin());
    EXPECT_TRUE(selector.distance().hasMax());
    EXPECT_FLOAT_EQ(selector.distance().getMax(), 20.0f);
}

TEST_F(EntityArgumentParseTest, ParseLevelRange)
{
    StringReader reader("@a[level=10..30]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_FALSE(selector.level().isUnbounded());
    EXPECT_EQ(selector.level().getMin(), 10);
    EXPECT_EQ(selector.level().getMax(), 30);
}

TEST_F(EntityArgumentParseTest, ParseLimit)
{
    StringReader reader("@a[limit=5]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_EQ(selector.limit(), 5);
}

TEST_F(EntityArgumentParseTest, ParseSort)
{
    StringReader reader("@a[sort=nearest]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_EQ(selector.sort(), EntitySelectorSort::Nearest);
}

TEST_F(EntityArgumentParseTest, ParseCoordinates)
{
    StringReader reader("@a[x=100,y=64,z=-200]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_TRUE(selector.hasX());
    EXPECT_TRUE(selector.hasY());
    EXPECT_TRUE(selector.hasZ());
    EXPECT_FLOAT_EQ(selector.getX(), 100.0f);
    EXPECT_FLOAT_EQ(selector.getY(), 64.0f);
    EXPECT_FLOAT_EQ(selector.getZ(), -200.0f);
}

TEST_F(EntityArgumentParseTest, ParseDimensions)
{
    StringReader reader("@a[dx=10,dy=5,dz=10]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_TRUE(selector.hasDx());
    EXPECT_TRUE(selector.hasDy());
    EXPECT_TRUE(selector.hasDz());
}

TEST_F(EntityArgumentParseTest, ParseName)
{
    StringReader reader("@a[name=Steve]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_TRUE(selector.hasUsername());
    EXPECT_EQ(selector.username(), "Steve");
}

TEST_F(EntityArgumentParseTest, ParseNameNegated)
{
    StringReader reader("@a[name=!Steve]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_TRUE(selector.hasUsernameNegated());
    EXPECT_EQ(selector.usernameNegated(), "Steve");
}

TEST_F(EntityArgumentParseTest, ParseGameMode)
{
    StringReader reader("@a[gamemode=survival]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_TRUE(selector.hasGameMode());
    EXPECT_EQ(selector.gameMode(), "survival");
}

TEST_F(EntityArgumentParseTest, ParseGameModeNegated)
{
    StringReader reader("@a[gamemode=!creative]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_TRUE(selector.hasGameMode());
    EXPECT_TRUE(selector.gameModeNegated());
}

TEST_F(EntityArgumentParseTest, ParseType)
{
    StringReader reader("@e[type=minecraft:zombie]");
    EntitySelector selector = entitiesArg->parse(reader);
    EXPECT_TRUE(selector.hasEntityType());
    EXPECT_EQ(selector.entityType(), "minecraft:zombie");
}

TEST_F(EntityArgumentParseTest, ParseTypeNegated)
{
    StringReader reader("@e[type=!minecraft:player]");
    EntitySelector selector = entitiesArg->parse(reader);
    EXPECT_TRUE(selector.hasEntityType());
    EXPECT_TRUE(selector.entityTypeNegated());
}

TEST_F(EntityArgumentParseTest, ParseTag)
{
    StringReader reader("@e[tag=foo]");
    EntitySelector selector = entitiesArg->parse(reader);
    ASSERT_EQ(selector.tags().size(), 1);
    EXPECT_EQ(selector.tags()[0], "foo");
}

TEST_F(EntityArgumentParseTest, ParseTagNegated)
{
    StringReader reader("@e[tag=!bar]");
    EntitySelector selector = entitiesArg->parse(reader);
    ASSERT_EQ(selector.tagsNegated().size(), 1);
    EXPECT_EQ(selector.tagsNegated()[0], "bar");
}

TEST_F(EntityArgumentParseTest, ParseTeam)
{
    StringReader reader("@a[team=red]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_TRUE(selector.hasTeam());
    EXPECT_EQ(selector.team(), "red");
}

// ========== x_rotation 和 y_rotation 解析测试 ==========

class EntityArgumentRotationParseTest : public ::testing::Test {
protected:
    std::shared_ptr<EntityArgumentType> playerArg = EntityArgumentType::player();
    std::shared_ptr<EntityArgumentType> playersArg = EntityArgumentType::players();
    std::shared_ptr<EntityArgumentType> entitiesArg = EntityArgumentType::entities();
};

TEST_F(EntityArgumentRotationParseTest, ParseXRotationRange)
{
    StringReader reader("@a[x_rotation=-45..45]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_FALSE(selector.xRotation().isUnbounded());
    EXPECT_TRUE(selector.xRotation().hasMin());
    EXPECT_TRUE(selector.xRotation().hasMax());
    EXPECT_FLOAT_EQ(selector.xRotation().getMin(), -45.0f);
    EXPECT_FLOAT_EQ(selector.xRotation().getMax(), 45.0f);
}

TEST_F(EntityArgumentRotationParseTest, ParseXRotationMinOnly)
{
    StringReader reader("@a[x_rotation=-30..]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_TRUE(selector.xRotation().hasMin());
    EXPECT_FALSE(selector.xRotation().hasMax());
    EXPECT_FLOAT_EQ(selector.xRotation().getMin(), -30.0f);
}

TEST_F(EntityArgumentRotationParseTest, ParseXRotationMaxOnly)
{
    StringReader reader("@a[x_rotation=..30]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_FALSE(selector.xRotation().hasMin());
    EXPECT_TRUE(selector.xRotation().hasMax());
    EXPECT_FLOAT_EQ(selector.xRotation().getMax(), 30.0f);
}

TEST_F(EntityArgumentRotationParseTest, ParseXRotationExactValue)
{
    StringReader reader("@a[x_rotation=0]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_TRUE(selector.xRotation().hasMin());
    EXPECT_TRUE(selector.xRotation().hasMax());
    EXPECT_FLOAT_EQ(selector.xRotation().getMin(), 0.0f);
    EXPECT_FLOAT_EQ(selector.xRotation().getMax(), 0.0f);
}

TEST_F(EntityArgumentRotationParseTest, ParseYRotationRange)
{
    StringReader reader("@a[y_rotation=170..-170]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_FALSE(selector.yRotation().isUnbounded());
    EXPECT_TRUE(selector.yRotation().hasMin());
    EXPECT_TRUE(selector.yRotation().hasMax());
    EXPECT_FLOAT_EQ(selector.yRotation().getMin(), 170.0f);
    EXPECT_FLOAT_EQ(selector.yRotation().getMax(), -170.0f);
}

TEST_F(EntityArgumentRotationParseTest, ParseYRotationWraparound)
{
    // 跨越正北方向的范围
    StringReader reader("@a[y_rotation=90..-90]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_FALSE(selector.yRotation().isUnbounded());
    EXPECT_FLOAT_EQ(selector.yRotation().getMin(), 90.0f);
    EXPECT_FLOAT_EQ(selector.yRotation().getMax(), -90.0f);
}

TEST_F(EntityArgumentRotationParseTest, ParseYRotationMinOnly)
{
    StringReader reader("@a[y_rotation=180..]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_TRUE(selector.yRotation().hasMin());
    EXPECT_FALSE(selector.yRotation().hasMax());
    EXPECT_FLOAT_EQ(selector.yRotation().getMin(), 180.0f);
}

TEST_F(EntityArgumentRotationParseTest, ParseYRotationMaxOnly)
{
    StringReader reader("@a[y_rotation=..-90]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_FALSE(selector.yRotation().hasMin());
    EXPECT_TRUE(selector.yRotation().hasMax());
    EXPECT_FLOAT_EQ(selector.yRotation().getMax(), -90.0f);
}

TEST_F(EntityArgumentRotationParseTest, ParseBothRotations)
{
    StringReader reader("@a[x_rotation=-30..30,y_rotation=-90..90]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_FALSE(selector.xRotation().isUnbounded());
    EXPECT_FALSE(selector.yRotation().isUnbounded());
    EXPECT_FLOAT_EQ(selector.xRotation().getMin(), -30.0f);
    EXPECT_FLOAT_EQ(selector.xRotation().getMax(), 30.0f);
    EXPECT_FLOAT_EQ(selector.yRotation().getMin(), -90.0f);
    EXPECT_FLOAT_EQ(selector.yRotation().getMax(), 90.0f);
}

TEST_F(EntityArgumentRotationParseTest, ParseRotationWithOtherParams)
{
    StringReader reader("@a[distance=10..50,x_rotation=-20..20,y_rotation=-45..45,limit=3]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_FALSE(selector.distance().isUnbounded());
    EXPECT_FALSE(selector.xRotation().isUnbounded());
    EXPECT_FALSE(selector.yRotation().isUnbounded());
    EXPECT_EQ(selector.limit(), 3);
}

TEST_F(EntityArgumentRotationParseTest, XRotationAngleTestMatchesParsed)
{
    StringReader reader("@a[x_rotation=-30..30]");
    EntitySelector selector = playersArg->parse(reader);

    // 测试角度过滤
    EXPECT_TRUE(selector.xRotation().testAngle(0.0f));
    EXPECT_TRUE(selector.xRotation().testAngle(15.0f));
    EXPECT_TRUE(selector.xRotation().testAngle(-15.0f));
    EXPECT_TRUE(selector.xRotation().testAngle(30.0f));
    EXPECT_TRUE(selector.xRotation().testAngle(-30.0f));

    EXPECT_FALSE(selector.xRotation().testAngle(45.0f));
    EXPECT_FALSE(selector.xRotation().testAngle(-45.0f));
    EXPECT_FALSE(selector.xRotation().testAngle(90.0f));
    EXPECT_FALSE(selector.xRotation().testAngle(-90.0f));
}

TEST_F(EntityArgumentRotationParseTest, YRotationWraparoundAngleTest)
{
    // 测试跨越边界的 y_rotation 范围：[170..-170]
    // 这表示从 170 度到 -170 度，跨越正北方向
    // 规范化后：min=170, max=-170, min > max，使用 OR 逻辑
    // 匹配范围：[170, 180) ∪ [-180, -170]
    StringReader reader("@a[y_rotation=170..-170]");
    EntitySelector selector = playersArg->parse(reader);

    // 在范围内（接近正北）
    EXPECT_TRUE(selector.yRotation().testAngle(175.0f));
    EXPECT_TRUE(selector.yRotation().testAngle(180.0f)); // 规范化为 -180
    EXPECT_TRUE(selector.yRotation().testAngle(-180.0f));
    EXPECT_TRUE(selector.yRotation().testAngle(-175.0f));
    EXPECT_TRUE(selector.yRotation().testAngle(170.0f));
    EXPECT_TRUE(selector.yRotation().testAngle(-170.0f));

    // 不在范围内（远离正北）
    // 范围是 [170, 180) ∪ [-180, -170]
    // -169 到 169 不在范围内
    EXPECT_FALSE(selector.yRotation().testAngle(0.0f));
    EXPECT_FALSE(selector.yRotation().testAngle(90.0f));
    EXPECT_FALSE(selector.yRotation().testAngle(-90.0f));
    EXPECT_FALSE(selector.yRotation().testAngle(169.0f));
    EXPECT_FALSE(selector.yRotation().testAngle(-169.0f));
    EXPECT_FALSE(selector.yRotation().testAngle(169.999f));
}

// ========== scores 参数解析测试 ==========

class EntityArgumentScoresParseTest : public ::testing::Test {
protected:
    std::shared_ptr<EntityArgumentType> playersArg = EntityArgumentType::players();
};

TEST_F(EntityArgumentScoresParseTest, ParseSingleScore)
{
    StringReader reader("@a[scores={deaths=5}]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_TRUE(selector.hasScoreConditions());
    const auto& scores = selector.scoreConditions();
    ASSERT_EQ(scores.size(), 1);
    EXPECT_TRUE(scores.count("deaths") > 0);
    EXPECT_EQ(scores.at("deaths").getMin(), 5);
    EXPECT_EQ(scores.at("deaths").getMax(), 5);
}

TEST_F(EntityArgumentScoresParseTest, ParseScoreRange)
{
    StringReader reader("@a[scores={kills=10..50}]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_TRUE(selector.hasScoreConditions());
    const auto& scores = selector.scoreConditions();
    ASSERT_EQ(scores.size(), 1);
    EXPECT_TRUE(scores.count("kills") > 0);
    EXPECT_EQ(scores.at("kills").getMin(), 10);
    EXPECT_EQ(scores.at("kills").getMax(), 50);
}

TEST_F(EntityArgumentScoresParseTest, ParseMultipleScores)
{
    StringReader reader("@a[scores={deaths=1..5,kills=10..,level=20}]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_TRUE(selector.hasScoreConditions());
    const auto& scores = selector.scoreConditions();
    ASSERT_EQ(scores.size(), 3);

    EXPECT_TRUE(scores.count("deaths") > 0);
    EXPECT_EQ(scores.at("deaths").getMin(), 1);
    EXPECT_EQ(scores.at("deaths").getMax(), 5);

    EXPECT_TRUE(scores.count("kills") > 0);
    EXPECT_TRUE(scores.at("kills").hasMin());
    EXPECT_FALSE(scores.at("kills").hasMax());
    EXPECT_EQ(scores.at("kills").getMin(), 10);

    EXPECT_TRUE(scores.count("level") > 0);
    EXPECT_EQ(scores.at("level").getMin(), 20);
    EXPECT_EQ(scores.at("level").getMax(), 20);
}

TEST_F(EntityArgumentScoresParseTest, ScoresSetsIncludeNonPlayersFalse)
{
    // scores 参数应该自动设置 includesNonPlayers 为 false
    StringReader reader("@e[scores={deaths=1}]");
    EntitySelector selector = EntityArgumentType::entities()->parse(reader);
    EXPECT_FALSE(selector.includesNonPlayers());
}

// ========== advancements 参数解析测试 ==========

class EntityArgumentAdvancementsParseTest : public ::testing::Test {
protected:
    std::shared_ptr<EntityArgumentType> playersArg = EntityArgumentType::players();
};

TEST_F(EntityArgumentAdvancementsParseTest, ParseAdvancementComplete)
{
    StringReader reader("@a[advancements={minecraft:story/root=true}]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_TRUE(selector.hasAdvancementConditions());
    const auto& adv = selector.advancementConditions();
    ASSERT_EQ(adv.size(), 1);

    mc::ResourceLocation expectedId("minecraft:story/root");
    EXPECT_TRUE(adv.count(expectedId) > 0);
    EXPECT_TRUE(adv.at(expectedId).isComplete.has_value());
    EXPECT_TRUE(adv.at(expectedId).isComplete.value());
}

TEST_F(EntityArgumentAdvancementsParseTest, ParseAdvancementNotComplete)
{
    StringReader reader("@a[advancements={minecraft:story/root=false}]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_TRUE(selector.hasAdvancementConditions());
    const auto& adv = selector.advancementConditions();

    mc::ResourceLocation expectedId("minecraft:story/root");
    EXPECT_TRUE(adv.count(expectedId) > 0);
    EXPECT_TRUE(adv.at(expectedId).isComplete.has_value());
    EXPECT_FALSE(adv.at(expectedId).isComplete.value());
}

TEST_F(EntityArgumentAdvancementsParseTest, ParseAdvancementWithCriteria)
{
    StringReader reader("@a[advancements={minecraft:story/mine_stone={got_stone=true}}]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_TRUE(selector.hasAdvancementConditions());
    const auto& adv = selector.advancementConditions();

    mc::ResourceLocation expectedId("minecraft:story/mine_stone");
    EXPECT_TRUE(adv.count(expectedId) > 0);
    EXPECT_FALSE(adv.at(expectedId).isComplete.has_value()); // 使用准则条件，不是整体完成状态
    EXPECT_EQ(adv.at(expectedId).criteriaConditions.size(), 1);
    EXPECT_TRUE(adv.at(expectedId).criteriaConditions.count("got_stone") > 0);
    EXPECT_TRUE(adv.at(expectedId).criteriaConditions.at("got_stone"));
}

TEST_F(EntityArgumentAdvancementsParseTest, ParseMultipleAdvancements)
{
    StringReader reader("@a[advancements={minecraft:story/root=true,minecraft:nether/root=false}]");
    EntitySelector selector = playersArg->parse(reader);
    EXPECT_TRUE(selector.hasAdvancementConditions());
    const auto& adv = selector.advancementConditions();
    ASSERT_EQ(adv.size(), 2);
}

TEST_F(EntityArgumentAdvancementsParseTest, AdvancementsSetsIncludeNonPlayersFalse)
{
    // advancements 参数应该自动设置 includesNonPlayers 为 false
    StringReader reader("@e[advancements={minecraft:story/root=true}]");
    EntitySelector selector = EntityArgumentType::entities()->parse(reader);
    EXPECT_FALSE(selector.includesNonPlayers());
}

// ========== predicate 参数解析测试 ==========

class EntityArgumentPredicateParseTest : public ::testing::Test {
protected:
    std::shared_ptr<EntityArgumentType> entitiesArg = EntityArgumentType::entities();
};

TEST_F(EntityArgumentPredicateParseTest, ParsePredicate)
{
    StringReader reader("@e[predicate=minecraft:example_predicate]");
    EntitySelector selector = entitiesArg->parse(reader);
    EXPECT_TRUE(selector.hasPredicateCondition());
    EXPECT_EQ(selector.predicateCondition().predicate.toString(), "minecraft:example_predicate");
    EXPECT_FALSE(selector.predicateCondition().negated);
}

TEST_F(EntityArgumentPredicateParseTest, ParsePredicateNegated)
{
    StringReader reader("@e[predicate=!minecraft:example_predicate]");
    EntitySelector selector = entitiesArg->parse(reader);
    EXPECT_TRUE(selector.hasPredicateCondition());
    EXPECT_EQ(selector.predicateCondition().predicate.toString(), "minecraft:example_predicate");
    EXPECT_TRUE(selector.predicateCondition().negated);
}

TEST_F(EntityArgumentPredicateParseTest, ParsePredicateWithoutNamespace)
{
    StringReader reader("@e[predicate=example_predicate]");
    EntitySelector selector = entitiesArg->parse(reader);
    EXPECT_TRUE(selector.hasPredicateCondition());
    EXPECT_EQ(selector.predicateCondition().predicate.toString(), "minecraft:example_predicate");
}
