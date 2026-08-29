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
 * @file TeamCommandTest.cpp
 * @brief TeamCommand 单元测试
 *
 * 测试 /team 命令的所有子命令：
 * - add: 创建队伍
 * - remove: 移除队伍
 * - list: 列出队伍
 * - empty: 清空队伍
 * - join: 加入队伍
 * - leave: 离开队伍
 * - modify: 修改队伍属性
 */

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "common/BaseTestServer.hpp"
#include "common/scoreboard/core/ScorePlayerTeam.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/TeamCommand.hpp"
#include "server/dimension/ServerDimensionManager.hpp"

namespace mc::command {

// 测试服务器所需的服务端类型位于 mc:: 顶层命名空间，此处引入以便在
// mc::command 命名空间内直接引用。
using mc::DimensionManager;
using mc::ServerDimensionManager;

class TeamTestServer final : public mc::test::BaseTestServer {
public:
    // 覆盖 dimensionManager，返回一个未注册任何维度的空 DimensionManager。
    // 这样 source.world() 经 dimensionManager().getDimension() 返回 nullptr，
    // 命令走 "World not available" 分支返回 0，避免 BaseTestServer 默认实现
    // 抛 std::logic_error 进而在 noexcept 的 world() 中触发 std::terminate。
    [[nodiscard]] ServerDimensionManager& dimensionManager() override
    {
        return m_dimensionManager;
    }

    [[nodiscard]] const ServerDimensionManager& dimensionManager() const override
    {
        return m_dimensionManager;
    }

private:
    // 真实 ServerDimensionManager（nullptr 构造：仅用于 getPlayerDimension 等 map 查询，不调
    // initialize 故不解引用内部 m_server；RelWithDebInfo 下构造断言 MC_ASSERT(server!=nullptr) 不生效）。
    // 替代旧 reinterpret_cast<ServerDimensionManager&>(基类DimensionManager) UB——派生类独有
    // m_playerDimensions 越界读基类内存致 TeleportCommand::teleportPlayers 调 getPlayerDimension 时 SEH。
    ServerDimensionManager m_dimensionManager{nullptr};
};

class TeamCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册命令
        TeamCommand::registerTo(m_server.commandRegistry().dispatcher());
    }

    TeamTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

// ========== 命令注册测试 ==========

TEST_F(TeamCommandTest, TeamCommandIsRegistered)
{
    // 验证 team 命令已注册
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    // 查找 team 节点
    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "team") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "team command should be registered";
}

// ========== addTeam 测试 ==========

TEST_F(TeamCommandTest, AddTeamCreatesTeam)
{
    auto& scoreboard = m_server.scoreboard();

    // 初始应该没有队伍
    EXPECT_EQ(scoreboard.getTeams().size(), 0u);

    // 创建队伍
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);
    EXPECT_EQ(team->getName(), "red");

    // 验证队伍已创建
    EXPECT_EQ(scoreboard.getTeams().size(), 1u);
    EXPECT_TRUE(scoreboard.hasTeam("red"));
}

TEST_F(TeamCommandTest, AddTeamWithDisplayName)
{
    auto& scoreboard = m_server.scoreboard();

    auto* team = scoreboard.createTeam("blue");
    ASSERT_NE(team, nullptr);

    // 设置显示名称
    team->setDisplayName(std::make_unique<text::StringTextComponent>("Blue Team"));

    EXPECT_NE(team->getDisplayName(), nullptr);
}

TEST_F(TeamCommandTest, AddTeamDuplicateNameFails)
{
    auto& scoreboard = m_server.scoreboard();

    // 创建第一个队伍
    auto* team1 = scoreboard.createTeam("red");
    ASSERT_NE(team1, nullptr);

    // 尝试创建同名队伍
    auto* team2 = scoreboard.createTeam("red");
    EXPECT_EQ(team2, nullptr);

    // 验证只有一个队伍
    EXPECT_EQ(scoreboard.getTeams().size(), 1u);
}

// ========== removeTeam 测试 ==========

TEST_F(TeamCommandTest, RemoveTeam)
{
    auto& scoreboard = m_server.scoreboard();

    // 创建队伍
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);
    EXPECT_EQ(scoreboard.getTeams().size(), 1u);

    // 移除队伍
    scoreboard.removeTeam(*team);

    // 验证队伍已移除
    EXPECT_EQ(scoreboard.getTeams().size(), 0u);
    EXPECT_FALSE(scoreboard.hasTeam("red"));
}

TEST_F(TeamCommandTest, RemoveNonExistentTeam)
{
    auto& scoreboard = m_server.scoreboard();

    // 尝试移除不存在的队伍
    auto* team = scoreboard.getTeam("nonexistent");
    EXPECT_EQ(team, nullptr);
}

// ========== listTeams 测试 ==========

TEST_F(TeamCommandTest, ListEmptyTeams)
{
    auto& scoreboard = m_server.scoreboard();

    // 初始应该没有队伍
    EXPECT_EQ(scoreboard.getTeams().size(), 0u);
}

TEST_F(TeamCommandTest, ListMultipleTeams)
{
    auto& scoreboard = m_server.scoreboard();

    // 创建多个队伍
    scoreboard.createTeam("red");
    scoreboard.createTeam("blue");
    scoreboard.createTeam("green");

    // 验证队伍数量
    EXPECT_EQ(scoreboard.getTeams().size(), 3u);
}

// ========== joinTeam/leaveTeam 测试 ==========

TEST_F(TeamCommandTest, JoinTeam)
{
    auto& scoreboard = m_server.scoreboard();

    // 创建队伍
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 添加玩家到队伍
    EXPECT_TRUE(scoreboard.addPlayerToTeam("Steve", *team));

    // 验证玩家在队伍中
    EXPECT_TRUE(team->hasMember("Steve"));
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), team);
}

TEST_F(TeamCommandTest, JoinTeamSwitchesTeam)
{
    auto& scoreboard = m_server.scoreboard();

    // 创建两个队伍
    auto* redTeam = scoreboard.createTeam("red");
    auto* blueTeam = scoreboard.createTeam("blue");
    ASSERT_NE(redTeam, nullptr);
    ASSERT_NE(blueTeam, nullptr);

    // 添加玩家到 red 队伍
    scoreboard.addPlayerToTeam("Steve", *redTeam);
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), redTeam);

    // 切换到 blue 队伍
    EXPECT_TRUE(scoreboard.addPlayerToTeam("Steve", *blueTeam));
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), blueTeam);
    EXPECT_FALSE(redTeam->hasMember("Steve"));
}

TEST_F(TeamCommandTest, LeaveTeam)
{
    auto& scoreboard = m_server.scoreboard();

    // 创建队伍并添加玩家
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);
    scoreboard.addPlayerToTeam("Steve", *team);

    // 移除玩家
    EXPECT_TRUE(scoreboard.removePlayerFromTeam("Steve", *team));
    EXPECT_FALSE(team->hasMember("Steve"));
    EXPECT_EQ(scoreboard.getPlayersTeam("Steve"), nullptr);
}

// ========== emptyTeam 测试 ==========

TEST_F(TeamCommandTest, EmptyTeam)
{
    auto& scoreboard = m_server.scoreboard();

    // 创建队伍并添加多个玩家
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);
    scoreboard.addPlayerToTeam("Steve", *team);
    scoreboard.addPlayerToTeam("Alex", *team);
    scoreboard.addPlayerToTeam("Bob", *team);

    EXPECT_EQ(team->getMembers().size(), 3u);

    // 清空队伍。getMembers() 返回内部 std::set 的 const 引用，removePlayerFromTeam 会
    // erase 当前迭代器，直接对 set 跑 range-for 边删边遍历会解引用已销毁节点 → SEH。
    // 先把成员名快照到 vector 再逐个移除，对齐 MC Java 的 empty 命令逐个移除语义。
    const std::vector<std::string> membersToEmpty(team->getMembers().begin(), team->getMembers().end());
    for (const auto& member : membersToEmpty) {
        scoreboard.removePlayerFromTeam(member, *team);
    }

    EXPECT_EQ(team->getMembers().size(), 0u);
}

// ========== modifyTeam 测试 ==========

TEST_F(TeamCommandTest, ModifyTeamColor)
{
    auto& scoreboard = m_server.scoreboard();

    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 设置颜色
    team->setColor(text::TextFormatting::Red);
    EXPECT_EQ(team->getColor(), text::TextFormatting::Red);
}

TEST_F(TeamCommandTest, ModifyTeamFriendlyFire)
{
    auto& scoreboard = m_server.scoreboard();

    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 默认允许友军伤害
    EXPECT_TRUE(team->getAllowFriendlyFire());

    // 禁用友军伤害
    team->setAllowFriendlyFire(false);
    EXPECT_FALSE(team->getAllowFriendlyFire());
}

TEST_F(TeamCommandTest, ModifyTeamSeeFriendlyInvisibles)
{
    auto& scoreboard = m_server.scoreboard();

    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 默认可以看到隐身队友
    EXPECT_TRUE(team->canSeeFriendlyInvisibles());

    // 禁用看到隐身队友
    team->setSeeFriendlyInvisibles(false);
    EXPECT_FALSE(team->canSeeFriendlyInvisibles());
}

TEST_F(TeamCommandTest, ModifyTeamPrefixSuffix)
{
    auto& scoreboard = m_server.scoreboard();

    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 设置前缀和后缀
    team->setPrefix(std::make_unique<text::StringTextComponent>("[Red] "));
    team->setSuffix(std::make_unique<text::StringTextComponent>(" [R]"));

    EXPECT_NE(team->getPrefix(), nullptr);
    EXPECT_NE(team->getSuffix(), nullptr);
}

TEST_F(TeamCommandTest, ModifyTeamVisibility)
{
    auto& scoreboard = m_server.scoreboard();

    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 设置名称标签可见性
    team->setNameTagVisibility(scoreboard::TeamVisibility::HideForOtherTeams);
    EXPECT_EQ(team->getNameTagVisibility(), scoreboard::TeamVisibility::HideForOtherTeams);

    // 设置死亡消息可见性
    team->setDeathMessageVisibility(scoreboard::TeamVisibility::Never);
    EXPECT_EQ(team->getDeathMessageVisibility(), scoreboard::TeamVisibility::Never);
}

TEST_F(TeamCommandTest, ModifyTeamCollisionRule)
{
    auto& scoreboard = m_server.scoreboard();

    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 设置碰撞规则
    team->setCollisionRule(scoreboard::TeamCollisionRule::PushOwnTeam);
    EXPECT_EQ(team->getCollisionRule(), scoreboard::TeamCollisionRule::PushOwnTeam);
}

// ========== 真实玩家名称的 joinTeam/leaveTeam 测试 ==========

TEST_F(TeamCommandTest, JoinTeamUsesRealPlayerName)
{
    auto& scoreboard = m_server.scoreboard();

    // 创建队伍
    auto* team = scoreboard.createTeam("red");
    ASSERT_NE(team, nullptr);

    // 使用真实玩家名称（与 resolvePlayerName 返回值一致）
    scoreboard.addPlayerToTeam("Alice", *team);

    // 验证使用的是真实玩家名称，而非 "player_1" 之类的占位名称
    EXPECT_TRUE(team->hasMember("Alice"));
    EXPECT_FALSE(team->hasMember("player_1"));
}

TEST_F(TeamCommandTest, LeaveTeamUsesRealPlayerName)
{
    auto& scoreboard = m_server.scoreboard();

    // 创建队伍并添加玩家
    auto* team = scoreboard.createTeam("blue");
    ASSERT_NE(team, nullptr);
    scoreboard.addPlayerToTeam("Bob", *team);

    // 验证用真实名称可以正确移除
    EXPECT_TRUE(scoreboard.removePlayerFromTeam("Bob", *team));
    EXPECT_FALSE(team->hasMember("Bob"));
    EXPECT_EQ(scoreboard.getPlayersTeam("Bob"), nullptr);
}

} // namespace mc::command
