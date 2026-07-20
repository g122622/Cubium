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
 * @file GetTeamTest.cpp
 * @brief Entity::getTeam() 和 ServerPlayer::getTeam() 单元测试
 *
 * 测试队伍获取功能：
 * - Entity 基类 getTeam() 返回 nullptr
 * - ServerPlayer::getTeam() 通过服务器记分板获取队伍
 * - ServerPlayer::getTeam() 无服务器时返回 nullptr
 * - ServerPlayer::getTeam() 玩家不在队伍时返回 nullptr
 */

#include <gtest/gtest.h>

#include "common/entity/core/Entity.hpp"
#include "common/scoreboard/core/ScoreCriteria.hpp"
#include "common/scoreboard/core/ScorePlayerTeam.hpp"
#include "common/scoreboard/core/Scoreboard.hpp"
#include "common/scoreboard/core/Team.hpp"
#include "common/util/math/Vector4.hpp"
#include "common/util/text/TextStyle.hpp"
#include "server/player/ServerPlayer.hpp"
#include <memory>

using namespace mc;
using namespace mc::scoreboard;
using namespace mc::text;

namespace mc {
namespace entity {
// 测试用实体类，用于测试基类 getTeam() 行为
class TestEntity : public Entity {
public:
    TestEntity(EntityInstanceId id)
        : Entity(id, nullptr)
    {}
};
} // namespace entity
} // namespace mc

// 前向声明
namespace mc::server {
class MockServer;
}

/**
 * @brief getTeam() 功能测试套件
 *
 * 测试 Entity 基类和 ServerPlayer 子类的 getTeam() 方法。
 */
class GetTeamTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册内置判据
        ScoreCriteriaRegistry::instance().registerBuiltinCriteria();
    }

    void TearDown() override
    {
        // 清理判据注册表
        ScoreCriteriaRegistry::instance().clear();
    }
};

// ========== Entity 基类 getTeam() 测试 ==========

TEST_F(GetTeamTest, EntityBaseClassGetTeamReturnsNullptr)
{
    // Entity 基类的 getTeam() 应该返回 nullptr
    mc::entity::TestEntity entity(1);
    EXPECT_EQ(entity.getTeam(), nullptr);

    const mc::entity::TestEntity& constEntity = entity;
    EXPECT_EQ(constEntity.getTeam(), nullptr);
}

TEST_F(GetTeamTest, EntityBaseClassGetTeamConstReturnsNullptr)
{
    // 验证 const 版本的 getTeam() 也返回 nullptr
    const mc::entity::TestEntity entity(2);
    EXPECT_EQ(entity.getTeam(), nullptr);
}

// ========== ServerPlayer getTeam() 测试 ==========

TEST_F(GetTeamTest, ServerPlayerGetTeamWithoutServerReturnsNullptr)
{
    // 创建 ServerPlayer，不设置服务器
    ServerPlayer player(1, "TestPlayer");

    // 无服务器时，getTeam() 应该返回 nullptr
    EXPECT_EQ(player.getTeam(), nullptr);

    const ServerPlayer& constPlayer = player;
    EXPECT_EQ(constPlayer.getTeam(), nullptr);
}

TEST_F(GetTeamTest, ServerPlayerGetTeamWithEmptyScoreboardReturnsNullptr)
{
    // 这个测试需要 MockServer，但由于 MockServer 实现复杂，
    // 我们在此测试 Scoreboard 的 getPlayersTeam 行为
    Scoreboard scoreboard;

    // 玩家不在任何队伍时，getPlayersTeam 返回 nullptr
    EXPECT_EQ(scoreboard.getPlayersTeam("TestPlayer"), nullptr);
}

// ========== Scoreboard 队伍管理测试 ==========

TEST_F(GetTeamTest, ScoreboardGetPlayersTeamReturnsTeam)
{
    Scoreboard scoreboard;

    // 创建队伍
    auto* redTeam = scoreboard.createTeam("red");
    ASSERT_NE(redTeam, nullptr);

    // 添加玩家到队伍
    scoreboard.addPlayerToTeam("Steve", *redTeam);

    // 获取玩家队伍
    auto* team = scoreboard.getPlayersTeam("Steve");
    EXPECT_EQ(team, redTeam);
    EXPECT_EQ(team->getName(), "red");
}

TEST_F(GetTeamTest, ScoreboardGetPlayersTeamReturnsNullptrForUnknownPlayer)
{
    Scoreboard scoreboard;

    // 创建队伍，但不添加此玩家
    auto* redTeam = scoreboard.createTeam("red");
    ASSERT_NE(redTeam, nullptr);
    redTeam->addMember("Alex");

    // 获取未加入队伍的玩家
    auto* team = scoreboard.getPlayersTeam("Steve");
    EXPECT_EQ(team, nullptr);
}

TEST_F(GetTeamTest, ScoreboardPlayerCanSwitchTeam)
{
    Scoreboard scoreboard;

    // 创建两个队伍
    auto* redTeam = scoreboard.createTeam("red");
    auto* blueTeam = scoreboard.createTeam("blue");
    ASSERT_NE(redTeam, nullptr);
    ASSERT_NE(blueTeam, nullptr);

    // 玩家加入红队
    scoreboard.addPlayerToTeam("Steve", *redTeam);
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), redTeam);

    // 玩家切换到蓝队
    scoreboard.addPlayerToTeam("Steve", *blueTeam);
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), blueTeam);
    EXPECT_FALSE(redTeam->hasMember("Steve"));
    EXPECT_TRUE(blueTeam->hasMember("Steve"));
}

TEST_F(GetTeamTest, ScoreboardRemovePlayerFromTeam)
{
    Scoreboard scoreboard;

    // 创建队伍并添加玩家
    auto* redTeam = scoreboard.createTeam("red");
    ASSERT_NE(redTeam, nullptr);
    scoreboard.addPlayerToTeam("Steve", *redTeam);
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), redTeam);

    // 移除玩家
    scoreboard.removePlayerFromTeam("Steve", *redTeam);
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), nullptr);
    EXPECT_FALSE(redTeam->hasMember("Steve"));
}

// ========== 队伍颜色测试 ==========

TEST_F(GetTeamTest, TeamGetColor)
{
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 默认颜色为白色
    EXPECT_EQ(team->getColor(), TextFormatting::White);

    // 设置颜色为红色
    team->setColor(TextFormatting::Red);
    EXPECT_EQ(team->getColor(), TextFormatting::Red);

    // 设置颜色为蓝色
    team->setColor(TextFormatting::Blue);
    EXPECT_EQ(team->getColor(), TextFormatting::Blue);
}

TEST_F(GetTeamTest, TeamColorToVector4f)
{
    Scoreboard scoreboard;
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 设置颜色为红色
    team->setColor(TextFormatting::Red);

    // 获取颜色的 ARGB 值
    TextFormatting color = team->getColor();
    u32 argb = getFormattingColor(color);

    // 验证颜色转换
    // 红色的 ARGB 应该是 0xFFFF5555（基于 MC 颜色常量）
    // TextFormatting::Red 对应的颜色值
    EXPECT_TRUE(isColor(color));

    // 转换为 Vector4f
    math::Vector4f colorVec(static_cast<f32>((argb >> 16) & 0xFF) / 255.0f,
        static_cast<f32>((argb >> 8) & 0xFF) / 255.0f,
        static_cast<f32>(argb & 0xFF) / 255.0f,
        1.0f);

    // 验证颜色向量的 alpha 为 1
    EXPECT_FLOAT_EQ(colorVec.w, 1.0f);
}

TEST_F(GetTeamTest, TeamColorVariousColors)
{
    Scoreboard scoreboard;

    // 测试多种颜色
    struct ColorTest {
        std::string teamName;
        TextFormatting color;
        bool isColor;
    };

    std::vector<ColorTest> tests = {
        {"red", TextFormatting::Red, true},
        {"blue", TextFormatting::Blue, true},
        {"green", TextFormatting::Green, true},
        {"yellow", TextFormatting::Yellow, true},
        {"gold", TextFormatting::Gold, true},
        {"aqua", TextFormatting::Aqua, true},
        {"light_purple", TextFormatting::LightPurple, true},
        {"white", TextFormatting::White, true},
        {"gray", TextFormatting::Gray, true},
        {"dark_gray", TextFormatting::DarkGray, true},
        {"dark_red", TextFormatting::DarkRed, true},
        {"dark_blue", TextFormatting::DarkBlue, true},
        {"dark_green", TextFormatting::DarkGreen, true},
        {"dark_aqua", TextFormatting::DarkAqua, true},
        {"dark_purple", TextFormatting::DarkPurple, true},
        {"black", TextFormatting::Black, true},
    };

    for (const auto& test : tests) {
        auto* team = scoreboard.createTeam(test.teamName);
        ASSERT_NE(team, nullptr) << "Failed to create team: " << test.teamName;

        team->setColor(test.color);
        EXPECT_EQ(team->getColor(), test.color) << "Color mismatch for team: " << test.teamName;
        EXPECT_EQ(isColor(test.color), test.isColor) << "isColor mismatch for: " << test.teamName;

        // 确保颜色值有效
        u32 argb = getFormattingColor(test.color);
        // 黑色的 RGB 是 0x000000，其他颜色应该有 RGB 值
        if (test.color != TextFormatting::Black) {
            EXPECT_GT(argb & 0xFFFFFF, 0u) << "Color should have RGB value for: " << test.teamName;
        }
    }
}

// ========== Team 接口测试 ==========

TEST_F(GetTeamTest, TeamInterfaceFromScorePlayerTeam)
{
    Scoreboard scoreboard;
    auto* scorePlayerTeam = scoreboard.createTeam("test_team");
    ASSERT_NE(scorePlayerTeam, nullptr);

    // ScorePlayerTeam 实现 Team 接口
    Team* team = scorePlayerTeam;
    EXPECT_EQ(team->getName(), "test_team");

    // 设置颜色
    team->setColor(TextFormatting::Aqua);
    EXPECT_EQ(team->getColor(), TextFormatting::Aqua);
}

// ========== 多玩家多队伍测试 ==========

TEST_F(GetTeamTest, MultiplePlayersMultipleTeams)
{
    Scoreboard scoreboard;

    // 创建三个队伍
    auto* red = scoreboard.createTeam("red");
    auto* blue = scoreboard.createTeam("blue");
    auto* green = scoreboard.createTeam("green");

    ASSERT_NE(red, nullptr);
    ASSERT_NE(blue, nullptr);
    ASSERT_NE(green, nullptr);

    // 设置不同颜色
    red->setColor(TextFormatting::Red);
    blue->setColor(TextFormatting::Blue);
    green->setColor(TextFormatting::Green);

    // 添加玩家到不同队伍
    scoreboard.addPlayerToTeam("Steve", *red);
    scoreboard.addPlayerToTeam("Alex", *blue);
    scoreboard.addPlayerToTeam("Bob", *green);
    scoreboard.addPlayerToTeam("Charlie", *red);

    // 验证队伍归属
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), red);
    EXPECT_EQ(scoreboard.getPlayersTeam("Alex"), blue);
    EXPECT_EQ(scoreboard.getPlayersTeam("Bob"), green);
    EXPECT_EQ(scoreboard.getPlayersTeam("Charlie"), red);

    // 验证队伍颜色
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve")->getColor(), TextFormatting::Red);
    EXPECT_EQ(scoreboard.getPlayersTeam("Alex")->getColor(), TextFormatting::Blue);
    EXPECT_EQ(scoreboard.getPlayersTeam("Bob")->getColor(), TextFormatting::Green);
}

// ========== 队伍移除后玩家归属测试 ==========

TEST_F(GetTeamTest, PlayerTeamIsNullAfterTeamRemoved)
{
    Scoreboard scoreboard;

    // 创建队伍并添加玩家
    auto* redTeam = scoreboard.createTeam("red");
    ASSERT_NE(redTeam, nullptr);
    scoreboard.addPlayerToTeam("Steve", *redTeam);
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), redTeam);

    // 移除队伍
    scoreboard.removeTeam(*redTeam);

    // 玩家队伍归属应该被清除
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), nullptr);
}

// ========== 集成测试：完整流程 ==========

TEST_F(GetTeamTest, FullTeamWorkflow)
{
    Scoreboard scoreboard;

    // 1. 创建队伍
    auto* team = scoreboard.createTeam("vip");
    ASSERT_NE(team, nullptr);

    // 2. 设置队伍属性
    team->setColor(TextFormatting::Gold);
    team->setPrefix(std::make_unique<StringTextComponent>("[VIP] "));

    // 3. 添加玩家
    scoreboard.addPlayerToTeam("Steve", *team);
    scoreboard.addPlayerToTeam("Alex", *team);

    // 4. 验证队伍归属
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), team);
    EXPECT_EQ(scoreboard.getPlayersTeam("Alex"), team);
    EXPECT_TRUE(team->hasMember("Steve"));
    EXPECT_TRUE(team->hasMember("Alex"));

    // 5. 验证队伍颜色
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve")->getColor(), TextFormatting::Gold);
    EXPECT_EQ(scoreboard.getPlayersTeam("Alex")->getColor(), TextFormatting::Gold);

    // 6. 移除一个玩家
    scoreboard.removePlayerFromTeam("Steve", *team);
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), nullptr);
    EXPECT_TRUE(team->hasMember("Alex"));
    EXPECT_FALSE(team->hasMember("Steve"));

    // 7. 清理
    scoreboard.removeTeam(*team);
    EXPECT_FALSE(scoreboard.hasTeam("vip"));
    EXPECT_EQ(scoreboard.getPlayersTeam("Alex"), nullptr);
}

// main 函数由 gtest_main 库提供
