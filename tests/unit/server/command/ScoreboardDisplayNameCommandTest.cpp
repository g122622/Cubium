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
 * @file ScoreboardDisplayNameCommandTest.cpp
 * @brief /scoreboard players display name 命令测试
 *
 * 覆盖 `players display name <targets> <objective> [<name>]`：
 * - 带 `<name>` 时写入该分数条目的自定义显示名；
 * - 不带 `<name>` 时清除覆盖、回退到持有者名字；
 * - `targets` 的逗号分隔与 `*`（所有已记录分数的持有者）展开。
 *
 * 断言落在「记分板模型里的实际值」上，而不只是命令返回值，避免命令只是返回成功
 * 却没有真正写入。
 */

#include "common/BaseTestServer.hpp"
#include "common/scoreboard/core/Score.hpp"
#include "common/scoreboard/core/ScoreCriteria.hpp"
#include "common/scoreboard/core/ScoreObjective.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/ScoreboardCommand.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"

#include <gtest/gtest.h>

#include <string>

using namespace mc;
using namespace mc::command;
using namespace mc::server;

class ScoreboardDisplayNameCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        scoreboard::ScoreCriteriaRegistry::instance().registerBuiltinCriteria();
        ScoreboardCommand::registerTo(m_server.commandRegistry().dispatcher());
        m_source = ServerCommandSource::forConsole(&m_server);
        ASSERT_EQ(_run("scoreboard objectives add kills dummy"), 1);
    }

    /// 执行命令并返回结果码（失败时返回 0）
    i32 _run(const std::string& input)
    {
        const auto result = m_server.commandRegistry().execute(input, m_source);
        return result.success() ? result.value() : 0;
    }

    [[nodiscard]] scoreboard::ScoreObjective* _objective()
    {
        return m_server.scoreboard().getObjective("kills");
    }

    [[nodiscard]] scoreboard::Score* _score(const std::string& holder)
    {
        auto* objective = _objective();
        return objective == nullptr ? nullptr : m_server.scoreboard().getScore(holder, *objective);
    }

    mc::test::BaseTestServer m_server;
    ServerCommandSource m_source = ServerCommandSource::forConsole(nullptr);
};

TEST_F(ScoreboardDisplayNameCommandTest, SetsDisplayNameOnTheScoreEntry)
{
    ASSERT_EQ(_run("scoreboard players display name Steve kills {\"text\":\"Red Steve\"}"), 1);

    auto* score = _score("Steve");
    ASSERT_NE(score, nullptr);
    ASSERT_NE(score->getDisplay(), nullptr) << "the command must write the score entry display name";
    EXPECT_EQ(score->getDisplay()->getUnformattedText(), "Red Steve");
}

// 原版 `display name <targets> <objective>` 本身可执行，语义是「清除自定义显示名」。
TEST_F(ScoreboardDisplayNameCommandTest, NoNameArgumentClearsTheOverride)
{
    ASSERT_EQ(_run("scoreboard players display name Steve kills {\"text\":\"Red Steve\"}"), 1);
    ASSERT_NE(_score("Steve")->getDisplay(), nullptr);

    ASSERT_EQ(_run("scoreboard players display name Steve kills"), 1);
    auto* score = _score("Steve");
    ASSERT_NE(score, nullptr);
    EXPECT_EQ(score->getDisplay(), nullptr) << "omitting <name> must clear the display name, not set an empty one";
}

// 逗号分隔的多个持有者都要被写入，且各自持有一份独立组件。
TEST_F(ScoreboardDisplayNameCommandTest, CommaSeparatedTargetsAreAllUpdated)
{
    // 返回值是受影响条目数（两个持有者），与原版一致。
    ASSERT_EQ(_run("scoreboard players display name Steve,Alex kills {\"text\":\"Shared\"}"), 2);

    for (const char* holder : {"Steve", "Alex"}) {
        auto* score = _score(holder);
        ASSERT_NE(score, nullptr) << holder;
        ASSERT_NE(score->getDisplay(), nullptr) << holder;
        EXPECT_EQ(score->getDisplay()->getUnformattedText(), "Shared") << holder;
    }
}

// `*` 展开为所有已记录分数的持有者。
TEST_F(ScoreboardDisplayNameCommandTest, WildcardExpandsToTrackedHolders)
{
    ASSERT_EQ(_run("scoreboard players set Steve kills 1"), 1);
    ASSERT_EQ(_run("scoreboard players set Alex kills 2"), 1);

    ASSERT_EQ(_run("scoreboard players display name * kills {\"text\":\"Everybody\"}"), 2);

    for (const char* holder : {"Steve", "Alex"}) {
        auto* score = _score(holder);
        ASSERT_NE(score, nullptr) << holder;
        ASSERT_NE(score->getDisplay(), nullptr) << holder;
        EXPECT_EQ(score->getDisplay()->getUnformattedText(), "Everybody") << holder;
    }
}

TEST_F(ScoreboardDisplayNameCommandTest, UnknownObjectiveFails)
{
    EXPECT_EQ(_run("scoreboard players display name Steve nosuch {\"text\":\"x\"}"), 0);
}
