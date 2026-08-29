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
 * @file TriggerCommandTest.cpp
 * @brief TriggerCommand 单元测试
 *
 * 测试 /trigger 命令的所有子命令：
 * - trigger <objective>: 增加 1 分
 * - trigger <objective> add <value>: 增加指定分数
 * - trigger <objective> set <value>: 设置指定分数
 *
 * 测试触发器锁定机制：
 * - 触发器使用后自动锁定
 * - /scoreboard players enable 可以重新启用
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/scoreboard/core/Score.hpp"
#include "common/scoreboard/core/ScoreCriteria.hpp"
#include "common/scoreboard/core/ScoreObjective.hpp"
#include "common/scoreboard/criteria/TriggerCriteria.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/ScoreboardCommand.hpp"
#include "server/command/commands/TriggerCommand.hpp"
#include "server/dimension/ServerDimensionManager.hpp"

namespace mc::command {

// 测试服务器所需的服务端类型位于 mc:: 顶层命名空间，此处引入以便在
// mc::command 命名空间内直接引用。
using mc::DimensionManager;
using mc::ServerDimensionManager;

/**
 * @brief 测试服务器，用于 TriggerCommand 测试
 */
class TriggerTestServer final : public mc::test::BaseTestServer {
public:
    TriggerTestServer() { scoreboard::ScoreCriteriaRegistry::instance().registerBuiltinCriteria(); }

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

class TriggerCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册命令
        TriggerCommand::registerTo(m_server.commandRegistry().dispatcher());
        ScoreboardCommand::registerTo(m_server.commandRegistry().dispatcher());
    }

    TriggerTestServer m_server;
};

// ========== 命令注册测试 ==========

TEST_F(TriggerCommandTest, TriggerCommandIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "trigger") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "trigger command should be registered";
}

// ========== 触发器基本功能测试 ==========

TEST_F(TriggerCommandTest, TriggerObjectiveNotExists)
{
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    auto result = m_server.commandRegistry().execute("/trigger nonexistent", source);
    // 目标不存在时应该返回 0（失败）
    EXPECT_EQ(result.value(), 0);
}

TEST_F(TriggerCommandTest, TriggerNotTriggerCriteria)
{
    auto& scoreboard = m_server.scoreboard();
    auto* dummyCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("dummy");
    ASSERT_NE(dummyCriteria, nullptr);

    auto* objective = scoreboard.addObjective("test_dummy", *dummyCriteria);
    ASSERT_NE(objective, nullptr);

    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    auto result = m_server.commandRegistry().execute("/trigger test_dummy", source);
    // 非 trigger 判据时应该返回 0（失败）
    EXPECT_EQ(result.value(), 0);
}

TEST_F(TriggerCommandTest, TriggerNotPrimed)
{
    auto& scoreboard = m_server.scoreboard();
    auto* triggerCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("trigger");
    ASSERT_NE(triggerCriteria, nullptr);

    auto* objective = scoreboard.addObjective("test_trigger", *triggerCriteria);
    ASSERT_NE(objective, nullptr);

    // 玩家没有分数，触发器未准备
    ServerCommandSource source = ServerCommandSource::forConsole(&m_server);
    auto result = m_server.commandRegistry().execute("/trigger test_trigger", source);
    // 未准备时应该返回 0（失败）
    EXPECT_EQ(result.value(), 0);
}

TEST_F(TriggerCommandTest, TriggerSuccess)
{
    auto& scoreboard = m_server.scoreboard();
    auto* triggerCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("trigger");
    ASSERT_NE(triggerCriteria, nullptr);

    auto* objective = scoreboard.addObjective("test_trigger", *triggerCriteria);
    ASSERT_NE(objective, nullptr);

    // 为玩家准备触发器（创建分数并解锁）
    auto* score = scoreboard.getOrCreateScore("Steve", *objective);
    ASSERT_NE(score, nullptr);
    score->setLocked(false);

    ServerCommandSource source(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 1, "Steve");
    auto result = m_server.commandRegistry().execute("/trigger test_trigger", source);

    EXPECT_EQ(result.value(), 1);
    EXPECT_EQ(score->getScorePoints(), 1);
    EXPECT_TRUE(score->isLocked()); // 触发后自动锁定
}

TEST_F(TriggerCommandTest, TriggerAddSuccess)
{
    auto& scoreboard = m_server.scoreboard();
    auto* triggerCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("trigger");
    ASSERT_NE(triggerCriteria, nullptr);

    auto* objective = scoreboard.addObjective("test_trigger", *triggerCriteria);
    ASSERT_NE(objective, nullptr);

    auto* score = scoreboard.getOrCreateScore("Steve", *objective);
    ASSERT_NE(score, nullptr);
    score->setScorePoints(10);
    score->setLocked(false);

    ServerCommandSource source(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 1, "Steve");
    auto result = m_server.commandRegistry().execute("/trigger test_trigger add 5", source);

    EXPECT_EQ(result.value(), 15);
    EXPECT_EQ(score->getScorePoints(), 15);
    EXPECT_TRUE(score->isLocked());
}

TEST_F(TriggerCommandTest, TriggerSetSuccess)
{
    auto& scoreboard = m_server.scoreboard();
    auto* triggerCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("trigger");
    ASSERT_NE(triggerCriteria, nullptr);

    auto* objective = scoreboard.addObjective("test_trigger", *triggerCriteria);
    ASSERT_NE(objective, nullptr);

    auto* score = scoreboard.getOrCreateScore("Steve", *objective);
    ASSERT_NE(score, nullptr);
    score->setScorePoints(10);
    score->setLocked(false);

    ServerCommandSource source(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 1, "Steve");
    auto result = m_server.commandRegistry().execute("/trigger test_trigger set 100", source);

    EXPECT_EQ(result.value(), 100);
    EXPECT_EQ(score->getScorePoints(), 100);
    EXPECT_TRUE(score->isLocked());
}

TEST_F(TriggerCommandTest, TriggerAlreadyUsed)
{
    auto& scoreboard = m_server.scoreboard();
    auto* triggerCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("trigger");
    ASSERT_NE(triggerCriteria, nullptr);

    auto* objective = scoreboard.addObjective("test_trigger", *triggerCriteria);
    ASSERT_NE(objective, nullptr);

    auto* score = scoreboard.getOrCreateScore("Steve", *objective);
    ASSERT_NE(score, nullptr);
    score->setLocked(true); // 已锁定

    ServerCommandSource source(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 1, "Steve");
    auto result = m_server.commandRegistry().execute("/trigger test_trigger", source);
    // 已锁定的触发器应该返回 0（失败）
    EXPECT_EQ(result.value(), 0);
}

// ========== Scoreboard players enable 测试 ==========

TEST_F(TriggerCommandTest, EnableTriggerViaScoreboard)
{
    auto& scoreboard = m_server.scoreboard();
    auto* triggerCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("trigger");
    ASSERT_NE(triggerCriteria, nullptr);

    auto* objective = scoreboard.addObjective("test_trigger", *triggerCriteria);
    ASSERT_NE(objective, nullptr);

    // 使用 scoreboard players enable 启用触发器
    ServerCommandSource adminSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 0, "Admin");
    auto enableResult =
        m_server.commandRegistry().execute("/scoreboard players enable Steve test_trigger", adminSource);
    EXPECT_EQ(enableResult.value(), 1);

    // 验证触发器已准备
    auto* score = scoreboard.getScore("Steve", *objective);
    ASSERT_NE(score, nullptr);
    EXPECT_FALSE(score->isLocked());

    // 现在可以使用触发器
    ServerCommandSource playerSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 1, "Steve");
    auto triggerResult = m_server.commandRegistry().execute("/trigger test_trigger", playerSource);
    EXPECT_EQ(triggerResult.value(), 1);
    EXPECT_EQ(score->getScorePoints(), 1);
    EXPECT_TRUE(score->isLocked());
}

TEST_F(TriggerCommandTest, EnableNonTriggerObjective)
{
    auto& scoreboard = m_server.scoreboard();
    auto* dummyCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("dummy");
    ASSERT_NE(dummyCriteria, nullptr);

    auto* objective = scoreboard.addObjective("test_dummy", *dummyCriteria);
    ASSERT_NE(objective, nullptr);

    ServerCommandSource source(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 0, "Admin");
    auto result = m_server.commandRegistry().execute("/scoreboard players enable Steve test_dummy", source);
    // 非 trigger 判据时应该返回 0（失败）
    EXPECT_EQ(result.value(), 0);
}

TEST_F(TriggerCommandTest, EnableNonExistentObjective)
{
    ServerCommandSource source(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 0, "Admin");
    auto result = m_server.commandRegistry().execute("/scoreboard players enable Steve nonexistent", source);
    // 目标不存在时应该返回 0（失败）
    EXPECT_EQ(result.value(), 0);
}

// ========== 分数锁定机制测试 ==========

TEST_F(TriggerCommandTest, ScoreLockMechanism)
{
    auto& scoreboard = m_server.scoreboard();
    auto* triggerCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("trigger");
    ASSERT_NE(triggerCriteria, nullptr);

    auto* objective = scoreboard.addObjective("trigger_test", *triggerCriteria);
    ASSERT_NE(objective, nullptr);

    // 新分数默认不锁定
    auto* score = scoreboard.getOrCreateScore("Player1", *objective);
    EXPECT_FALSE(score->isLocked());

    // 手动设置锁定
    score->setLocked(true);
    EXPECT_TRUE(score->isLocked());

    // reset() 会重置锁定状态
    score->reset();
    EXPECT_FALSE(score->isLocked());
    EXPECT_EQ(score->getScorePoints(), 0);
}

// ========== 触发器重新启用测试 ==========

TEST_F(TriggerCommandTest, ReenableTrigger)
{
    auto& scoreboard = m_server.scoreboard();
    auto* triggerCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("trigger");
    ASSERT_NE(triggerCriteria, nullptr);

    auto* objective = scoreboard.addObjective("retrigger", *triggerCriteria);
    ASSERT_NE(objective, nullptr);

    // 准备触发器
    ServerCommandSource adminSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 0, "Admin");
    m_server.commandRegistry().execute("/scoreboard players enable Player1 retrigger", adminSource);

    // 第一次触发
    ServerCommandSource playerSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 1, "Player1");
    auto result1 = m_server.commandRegistry().execute("/trigger retrigger add 10", playerSource);
    EXPECT_EQ(result1.value(), 10);

    auto* score = scoreboard.getScore("Player1", *objective);
    ASSERT_NE(score, nullptr);
    EXPECT_TRUE(score->isLocked());

    // 尝试再次触发应该失败（已锁定）
    auto result2 = m_server.commandRegistry().execute("/trigger retrigger add 5", playerSource);
    EXPECT_EQ(result2.value(), 0);
    // 分数应该仍然是 10
    EXPECT_EQ(score->getScorePoints(), 10);

    // 重新启用触发器
    m_server.commandRegistry().execute("/scoreboard players enable Player1 retrigger", adminSource);
    EXPECT_FALSE(score->isLocked());

    // 现在可以再次触发
    auto result3 = m_server.commandRegistry().execute("/trigger retrigger add 5", playerSource);
    EXPECT_EQ(result3.value(), 15); // 10 + 5
}

// ========== 玩家权限测试 ==========

TEST_F(TriggerCommandTest, ConsoleCannotUseTrigger)
{
    // 控制台命令源（不是玩家）
    ServerCommandSource consoleSource = ServerCommandSource::forConsole(&m_server);
    auto result = m_server.commandRegistry().execute("/trigger test", consoleSource);
    // 控制台不能使用 trigger 命令
    EXPECT_EQ(result.value(), 0);
}

// ========== /scoreboard players list 测试 ==========

TEST_F(TriggerCommandTest, ListPlayers)
{
    auto& scoreboard = m_server.scoreboard();
    auto* triggerCriteria = scoreboard::ScoreCriteriaRegistry::instance().getCriteria("trigger");
    ASSERT_NE(triggerCriteria, nullptr);

    auto* objective = scoreboard.addObjective("test_trigger", *triggerCriteria);
    ASSERT_NE(objective, nullptr);

    // 为玩家设置分数
    auto* score = scoreboard.getOrCreateScore("Steve", *objective);
    ASSERT_NE(score, nullptr);
    score->setScorePoints(42);

    ServerCommandSource source(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 0, "Admin");
    auto result = m_server.commandRegistry().execute("/scoreboard players list Steve", source);
    EXPECT_EQ(result.value(), 1);
}

} // namespace mc::command
