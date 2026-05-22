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
 * @file PlayerResolverTest.cpp
 * @brief PlayerResolver 单元测试
 *
 * 测试玩家选择器的解析和过滤功能，特别是经验等级过滤。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/command/arguments/EntityArgument.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/support/PlayerResolver.hpp"

namespace mc::command {

class PlayerResolverTestServer final : public test::BaseTestServer {};

} // namespace mc::command

using mc::Difficulty;
using mc::GameMode;
using mc::PlayerId;
using mc::command::EntitySelector;
using mc::command::EntitySelectorSort;
using mc::command::EntitySelectorType;
using mc::command::FloatRange;
using mc::command::IntRange;
using mc::command::ServerCommandSource;
using mc::command::support::getDifficultyCommandName;
using mc::command::support::getGameModeCommandName;
using mc::command::support::resolvePlayerIds;
using mc::command::support::resolveSinglePlayerId;

class IntRangeTest : public ::testing::Test {
protected:
    mc::command::IntRange range;
};

TEST_F(IntRangeTest, UnboundedRangeAcceptsAnyValue)
{
    EXPECT_TRUE(range.isUnbounded());
    EXPECT_TRUE(range.test(0));
    EXPECT_TRUE(range.test(100));
    EXPECT_TRUE(range.test(-50));
}

TEST_F(IntRangeTest, MinBoundRejectsLowerValues)
{
    range.setMin(10);
    EXPECT_FALSE(range.isUnbounded());
    EXPECT_FALSE(range.test(5));
    EXPECT_FALSE(range.test(9));
    EXPECT_TRUE(range.test(10));
    EXPECT_TRUE(range.test(100));
}

TEST_F(IntRangeTest, MaxBoundRejectsHigherValues)
{
    range.setMax(20);
    EXPECT_FALSE(range.isUnbounded());
    EXPECT_TRUE(range.test(0));
    EXPECT_TRUE(range.test(20));
    EXPECT_FALSE(range.test(21));
    EXPECT_FALSE(range.test(100));
}

TEST_F(IntRangeTest, BoundedRangeOnlyAcceptsInRange)
{
    range.setMin(10);
    range.setMax(20);
    EXPECT_FALSE(range.isUnbounded());
    EXPECT_FALSE(range.test(5));
    EXPECT_FALSE(range.test(9));
    EXPECT_TRUE(range.test(10));
    EXPECT_TRUE(range.test(15));
    EXPECT_TRUE(range.test(20));
    EXPECT_FALSE(range.test(21));
    EXPECT_FALSE(range.test(100));
}

TEST_F(IntRangeTest, ExactValueRange)
{
    range.setMin(15);
    range.setMax(15);
    EXPECT_FALSE(range.isUnbounded());
    EXPECT_FALSE(range.test(14));
    EXPECT_TRUE(range.test(15));
    EXPECT_FALSE(range.test(16));
}

TEST_F(IntRangeTest, ZeroLevelHandling)
{
    range.setMin(0);
    range.setMax(5);
    EXPECT_TRUE(range.test(0));
    EXPECT_TRUE(range.test(3));
    EXPECT_TRUE(range.test(5));
    EXPECT_FALSE(range.test(6));
}

TEST_F(IntRangeTest, HighLevelHandling)
{
    range.setMin(100);
    range.setMax(200);
    EXPECT_FALSE(range.test(99));
    EXPECT_TRUE(range.test(100));
    EXPECT_TRUE(range.test(150));
    EXPECT_TRUE(range.test(200));
    EXPECT_FALSE(range.test(201));
}

class PlayerResolverTest : public ::testing::Test {
protected:
    mc::command::PlayerResolverTestServer m_server;
};

TEST_F(PlayerResolverTest, ResolveSinglePlayerWithNoPlayersReturnsZero)
{
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::SinglePlayer);

    PlayerId result = resolveSinglePlayerId(source, selector);
    EXPECT_EQ(result, 0);
}

TEST_F(PlayerResolverTest, ResolveAllPlayersWithNoPlayersReturnsEmpty)
{
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);

    auto result = resolvePlayerIds(source, selector);
    EXPECT_TRUE(result.empty());
}

TEST_F(PlayerResolverTest, ResolveAllPlayersReturnsAll)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");
    m_server.addTestPlayer(3, "Charlie");

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);

    auto result = resolvePlayerIds(source, selector);
    EXPECT_EQ(result.size(), 3);
}

TEST_F(PlayerResolverTest, ResolveByUsername)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");
    m_server.addTestPlayer(3, "Charlie");

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::byUsername("Bob");

    PlayerId result = resolveSinglePlayerId(source, selector);
    EXPECT_EQ(result, 2);
}

TEST_F(PlayerResolverTest, ResolveByUsernameNotFound)
{
    m_server.addTestPlayer(1, "Alice");

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector = EntitySelector::byUsername("UnknownPlayer");

    PlayerId result = resolveSinglePlayerId(source, selector);
    EXPECT_EQ(result, 0);
}

TEST_F(PlayerResolverTest, ResolveNearestPlayer)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");
    m_server.addTestPlayer(3, "Charlie");

    auto* data1 = m_server.playerManager().getPlayer(1);
    auto* data2 = m_server.playerManager().getPlayer(2);
    auto* data3 = m_server.playerManager().getPlayer(3);
    ASSERT_NE(data1, nullptr);
    ASSERT_NE(data2, nullptr);
    ASSERT_NE(data3, nullptr);
    data1->x = 100.0f;
    data2->x = 10.0f;
    data3->x = 200.0f;

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::SinglePlayer);
    selector.setSort(EntitySelectorSort::Nearest);

    PlayerId result = resolveSinglePlayerId(source, selector);
    EXPECT_EQ(result, 2);
}

TEST_F(PlayerResolverTest, ResolveFurthestPlayer)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");
    m_server.addTestPlayer(3, "Charlie");

    auto* data1 = m_server.playerManager().getPlayer(1);
    auto* data2 = m_server.playerManager().getPlayer(2);
    auto* data3 = m_server.playerManager().getPlayer(3);
    ASSERT_NE(data1, nullptr);
    ASSERT_NE(data2, nullptr);
    ASSERT_NE(data3, nullptr);
    data1->x = 100.0f;
    data2->x = 10.0f;
    data3->x = 200.0f;

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.setSort(EntitySelectorSort::Furthest);
    selector.setLimit(1);

    auto result = resolvePlayerIds(source, selector);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], 3);
}

TEST_F(PlayerResolverTest, ResolveWithDistanceFilter)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");
    m_server.addTestPlayer(3, "Charlie");

    auto* data1 = m_server.playerManager().getPlayer(1);
    auto* data2 = m_server.playerManager().getPlayer(2);
    auto* data3 = m_server.playerManager().getPlayer(3);
    ASSERT_NE(data1, nullptr);
    ASSERT_NE(data2, nullptr);
    ASSERT_NE(data3, nullptr);
    data1->x = 5.0f;
    data2->x = 15.0f;
    data3->x = 50.0f;

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.distance().setMin(10);
    selector.distance().setMax(30);

    auto result = resolvePlayerIds(source, selector);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], 2);
}

TEST_F(PlayerResolverTest, ResolveWithLimit)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");
    m_server.addTestPlayer(3, "Charlie");
    m_server.addTestPlayer(4, "Dave");
    m_server.addTestPlayer(5, "Eve");

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.setLimit(3);

    auto result = resolvePlayerIds(source, selector);
    EXPECT_EQ(result.size(), 3);
}

TEST_F(PlayerResolverTest, GameModeFilterSurvival)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");

    auto* data1 = m_server.playerManager().getPlayer(1);
    auto* data2 = m_server.playerManager().getPlayer(2);
    ASSERT_NE(data1, nullptr);
    ASSERT_NE(data2, nullptr);
    data1->gameMode = GameMode::Survival;
    data2->gameMode = GameMode::Creative;

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.setGameMode("survival");

    auto result = resolvePlayerIds(source, selector);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], 1);
}

TEST_F(PlayerResolverTest, GameModeFilterCreative)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");
    m_server.addTestPlayer(3, "Charlie");

    auto* data1 = m_server.playerManager().getPlayer(1);
    auto* data2 = m_server.playerManager().getPlayer(2);
    auto* data3 = m_server.playerManager().getPlayer(3);
    ASSERT_NE(data1, nullptr);
    ASSERT_NE(data2, nullptr);
    ASSERT_NE(data3, nullptr);
    data1->gameMode = GameMode::Survival;
    data2->gameMode = GameMode::Creative;
    data3->gameMode = GameMode::Adventure;

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.setGameMode("creative");

    auto result = resolvePlayerIds(source, selector);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], 2);
}

TEST_F(PlayerResolverTest, GameModeFilterByNumber)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");

    auto* data1 = m_server.playerManager().getPlayer(1);
    auto* data2 = m_server.playerManager().getPlayer(2);
    ASSERT_NE(data1, nullptr);
    ASSERT_NE(data2, nullptr);
    data1->gameMode = GameMode::Survival;
    data2->gameMode = GameMode::Creative;

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.setGameMode("1");

    auto result = resolvePlayerIds(source, selector);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], 2);
}

TEST_F(PlayerResolverTest, GameModeFilterNegated)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");
    m_server.addTestPlayer(3, "Charlie");

    auto* data1 = m_server.playerManager().getPlayer(1);
    auto* data2 = m_server.playerManager().getPlayer(2);
    auto* data3 = m_server.playerManager().getPlayer(3);
    ASSERT_NE(data1, nullptr);
    ASSERT_NE(data2, nullptr);
    ASSERT_NE(data3, nullptr);
    data1->gameMode = GameMode::Survival;
    data2->gameMode = GameMode::Creative;
    data3->gameMode = GameMode::Survival;

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.setGameMode("creative", true);

    auto result = resolvePlayerIds(source, selector);

    ASSERT_EQ(result.size(), 2);
}

TEST_F(PlayerResolverTest, GameModeFilterAdventure)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");

    auto* data1 = m_server.playerManager().getPlayer(1);
    auto* data2 = m_server.playerManager().getPlayer(2);
    ASSERT_NE(data1, nullptr);
    ASSERT_NE(data2, nullptr);
    data1->gameMode = GameMode::Adventure;
    data2->gameMode = GameMode::Spectator;

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.setGameMode("adventure");

    auto result = resolvePlayerIds(source, selector);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], 1);
}

TEST_F(PlayerResolverTest, GameModeFilterSpectator)
{
    m_server.addTestPlayer(1, "Alice");
    m_server.addTestPlayer(2, "Bob");

    auto* data1 = m_server.playerManager().getPlayer(1);
    auto* data2 = m_server.playerManager().getPlayer(2);
    ASSERT_NE(data1, nullptr);
    ASSERT_NE(data2, nullptr);
    data1->gameMode = GameMode::Survival;
    data2->gameMode = GameMode::Spectator;

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    EntitySelector selector(EntitySelectorType::AllPlayers);
    selector.setGameMode("spectator");

    auto result = resolvePlayerIds(source, selector);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0], 2);
}

TEST(PlayerResolverUtilTest, GetGameModeCommandName)
{
    EXPECT_STREQ(getGameModeCommandName(GameMode::Survival), "survival");
    EXPECT_STREQ(getGameModeCommandName(GameMode::Creative), "creative");
    EXPECT_STREQ(getGameModeCommandName(GameMode::Adventure), "adventure");
    EXPECT_STREQ(getGameModeCommandName(GameMode::Spectator), "spectator");
    EXPECT_STREQ(getGameModeCommandName(GameMode::NotSet), "not_set");
}

TEST(PlayerResolverUtilTest, GetDifficultyCommandName)
{
    EXPECT_STREQ(getDifficultyCommandName(Difficulty::Peaceful), "peaceful");
    EXPECT_STREQ(getDifficultyCommandName(Difficulty::Easy), "easy");
    EXPECT_STREQ(getDifficultyCommandName(Difficulty::Normal), "normal");
    EXPECT_STREQ(getDifficultyCommandName(Difficulty::Hard), "hard");
}

class FloatRangeAngleTest : public ::testing::Test {
protected:
    mc::command::FloatRange range;
};

TEST_F(FloatRangeAngleTest, UnboundedRangeAcceptsAnyAngle)
{
    EXPECT_TRUE(range.isUnbounded());
    EXPECT_TRUE(range.testAngle(0.0f));
    EXPECT_TRUE(range.testAngle(90.0f));
    EXPECT_TRUE(range.testAngle(-90.0f));
    EXPECT_TRUE(range.testAngle(180.0f));
    EXPECT_TRUE(range.testAngle(-180.0f));
    EXPECT_TRUE(range.testAngle(270.0f));
    EXPECT_TRUE(range.testAngle(-270.0f));
}

TEST_F(FloatRangeAngleTest, NormalRangeNoWraparound)
{
    range.setMin(10.0f);
    range.setMax(30.0f);

    EXPECT_TRUE(range.testAngle(10.0f));
    EXPECT_TRUE(range.testAngle(20.0f));
    EXPECT_TRUE(range.testAngle(30.0f));
    EXPECT_FALSE(range.testAngle(9.0f));
    EXPECT_FALSE(range.testAngle(31.0f));
    EXPECT_FALSE(range.testAngle(0.0f));
    EXPECT_FALSE(range.testAngle(-10.0f));
}

TEST_F(FloatRangeAngleTest, WraparoundRange)
{
    range.setMin(170.0f);
    range.setMax(-170.0f);

    EXPECT_TRUE(range.testAngle(175.0f));
    EXPECT_TRUE(range.testAngle(180.0f));
    EXPECT_TRUE(range.testAngle(-180.0f));
    EXPECT_TRUE(range.testAngle(-175.0f));
    EXPECT_TRUE(range.testAngle(-170.0f));
    EXPECT_TRUE(range.testAngle(170.0f));

    EXPECT_FALSE(range.testAngle(0.0f));
    EXPECT_FALSE(range.testAngle(90.0f));
    EXPECT_FALSE(range.testAngle(-90.0f));
    EXPECT_FALSE(range.testAngle(169.0f));
    EXPECT_FALSE(range.testAngle(-169.0f));
}

TEST_F(FloatRangeAngleTest, WraparoundRangeLarge)
{
    range.setMin(90.0f);
    range.setMax(-90.0f);

    EXPECT_TRUE(range.testAngle(90.0f));
    EXPECT_TRUE(range.testAngle(180.0f));
    EXPECT_TRUE(range.testAngle(-180.0f));
    EXPECT_TRUE(range.testAngle(-90.0f));
    EXPECT_TRUE(range.testAngle(120.0f));
    EXPECT_TRUE(range.testAngle(-120.0f));

    EXPECT_FALSE(range.testAngle(0.0f));
    EXPECT_FALSE(range.testAngle(45.0f));
    EXPECT_FALSE(range.testAngle(-45.0f));
    EXPECT_FALSE(range.testAngle(89.0f));
    EXPECT_FALSE(range.testAngle(-89.0f));
}

TEST_F(FloatRangeAngleTest, PitchRangeNegative90To90)
{
    range.setMin(-45.0f);
    range.setMax(45.0f);

    EXPECT_TRUE(range.testAngle(0.0f));
    EXPECT_TRUE(range.testAngle(30.0f));
    EXPECT_TRUE(range.testAngle(-30.0f));
    EXPECT_TRUE(range.testAngle(45.0f));
    EXPECT_TRUE(range.testAngle(-45.0f));

    EXPECT_FALSE(range.testAngle(60.0f));
    EXPECT_FALSE(range.testAngle(-60.0f));
    EXPECT_FALSE(range.testAngle(90.0f));
    EXPECT_FALSE(range.testAngle(-90.0f));
}

TEST_F(FloatRangeAngleTest, OnlyMinBound)
{
    range.setMin(45.0f);

    EXPECT_TRUE(range.testAngle(45.0f));
    EXPECT_TRUE(range.testAngle(90.0f));
    EXPECT_TRUE(range.testAngle(180.0f));
    EXPECT_TRUE(range.testAngle(-180.0f));
    EXPECT_TRUE(range.testAngle(-1.0f));
    EXPECT_TRUE(range.testAngle(170.0f));
    EXPECT_TRUE(range.testAngle(-170.0f));
    EXPECT_TRUE(range.testAngle(-2.0f));
    EXPECT_TRUE(range.testAngle(-90.0f));
    EXPECT_TRUE(range.testAngle(-179.0f));

    EXPECT_FALSE(range.testAngle(0.0f));
    EXPECT_FALSE(range.testAngle(-0.5f));
    EXPECT_FALSE(range.testAngle(44.0f));
    EXPECT_FALSE(range.testAngle(1.0f));
    EXPECT_FALSE(range.testAngle(44.999f));
}

TEST_F(FloatRangeAngleTest, OnlyMaxBound)
{
    range.setMax(90.0f);

    EXPECT_TRUE(range.testAngle(0.0f));
    EXPECT_TRUE(range.testAngle(45.0f));
    EXPECT_TRUE(range.testAngle(90.0f));

    EXPECT_FALSE(range.testAngle(-1.0f));
    EXPECT_FALSE(range.testAngle(91.0f));
    EXPECT_FALSE(range.testAngle(-45.0f));
    EXPECT_FALSE(range.testAngle(180.0f));
}

TEST_F(FloatRangeAngleTest, AngleNormalization)
{
    range.setMin(0.0f);
    range.setMax(90.0f);

    EXPECT_FALSE(range.testAngle(270.0f));
    EXPECT_TRUE(range.testAngle(-270.0f));
    EXPECT_TRUE(range.testAngle(360.0f));
    EXPECT_TRUE(range.testAngle(-360.0f));
    EXPECT_TRUE(range.testAngle(450.0f));
}

TEST_F(FloatRangeAngleTest, ExactAngleMatch)
{
    range.setMin(45.0f);
    range.setMax(45.0f);

    EXPECT_TRUE(range.testAngle(45.0f));
    EXPECT_FALSE(range.testAngle(44.9f));
    EXPECT_FALSE(range.testAngle(45.1f));
    EXPECT_FALSE(range.testAngle(0.0f));
    EXPECT_FALSE(range.testAngle(-45.0f));
}

TEST_F(FloatRangeAngleTest, FullCircleRange)
{
    range.setMin(-180.0f);
    range.setMax(180.0f);

    EXPECT_TRUE(range.testAngle(-180.0f));
    EXPECT_TRUE(range.testAngle(180.0f));
    EXPECT_TRUE(range.testAngle(-180.0f));
    EXPECT_TRUE(range.testAngle(540.0f));

    EXPECT_FALSE(range.testAngle(-90.0f));
    EXPECT_FALSE(range.testAngle(0.0f));
    EXPECT_FALSE(range.testAngle(90.0f));
}

TEST_F(FloatRangeAngleTest, FullCircleRangeWraparound)
{
    range.setMin(-179.0f);
    range.setMax(179.0f);

    EXPECT_TRUE(range.testAngle(-179.0f));
    EXPECT_TRUE(range.testAngle(-90.0f));
    EXPECT_TRUE(range.testAngle(0.0f));
    EXPECT_TRUE(range.testAngle(90.0f));
    EXPECT_TRUE(range.testAngle(179.0f));

    EXPECT_FALSE(range.testAngle(180.0f));
    EXPECT_FALSE(range.testAngle(-180.0f));
}

class EntitySelectorAngleTest : public ::testing::Test {
protected:
    mc::command::EntitySelector selector;
};

TEST_F(EntitySelectorAngleTest, DefaultRotationRangesAreUnbounded)
{
    EXPECT_TRUE(selector.xRotation().isUnbounded());
    EXPECT_TRUE(selector.yRotation().isUnbounded());
}

TEST_F(EntitySelectorAngleTest, SetXRotation)
{
    selector.xRotation().setMin(-45.0f);
    selector.xRotation().setMax(45.0f);

    EXPECT_FALSE(selector.xRotation().isUnbounded());
    EXPECT_TRUE(selector.xRotation().hasMin());
    EXPECT_TRUE(selector.xRotation().hasMax());
    EXPECT_FLOAT_EQ(selector.xRotation().getMin(), -45.0f);
    EXPECT_FLOAT_EQ(selector.xRotation().getMax(), 45.0f);
}

TEST_F(EntitySelectorAngleTest, SetYRotation)
{
    selector.yRotation().setMin(170.0f);
    selector.yRotation().setMax(-170.0f);

    EXPECT_FALSE(selector.yRotation().isUnbounded());
    EXPECT_TRUE(selector.yRotation().hasMin());
    EXPECT_TRUE(selector.yRotation().hasMax());
    EXPECT_FLOAT_EQ(selector.yRotation().getMin(), 170.0f);
    EXPECT_FLOAT_EQ(selector.yRotation().getMax(), -170.0f);
}

TEST_F(EntitySelectorAngleTest, XRotationFilterMatchesPitch)
{
    selector.xRotation().setMin(-30.0f);
    selector.xRotation().setMax(30.0f);

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

TEST_F(EntitySelectorAngleTest, YRotationFilterHandlesWraparound)
{
    selector.yRotation().setMin(170.0f);
    selector.yRotation().setMax(-170.0f);

    EXPECT_TRUE(selector.yRotation().testAngle(175.0f));
    EXPECT_TRUE(selector.yRotation().testAngle(180.0f));
    EXPECT_TRUE(selector.yRotation().testAngle(-180.0f));
    EXPECT_TRUE(selector.yRotation().testAngle(-175.0f));

    EXPECT_FALSE(selector.yRotation().testAngle(0.0f));
    EXPECT_FALSE(selector.yRotation().testAngle(90.0f));
    EXPECT_FALSE(selector.yRotation().testAngle(-90.0f));
}
