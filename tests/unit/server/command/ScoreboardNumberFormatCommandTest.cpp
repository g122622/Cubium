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
 * @file ScoreboardNumberFormatCommandTest.cpp
 * @brief /scoreboard ... numberformat 命令测试
 *
 * 覆盖两条命令路径（两者最终都写进记分板模型，但作用对象不同）：
 * - `objectives modify <objective> numberformat [blank|styled <style>|fixed <contents>]`
 *   → 目标级格式（无子节点 = 清除）
 * - `players display numberformat <targets> <objective> [...]`
 *   → 单条分数的覆盖格式（多目标逗号分隔、`*` 全部持有者）
 *
 * 断言落在「记分板模型里的实际值」与「渲染结果」上，而不只是返回值，避免命令只是返回成功
 * 却没有真正写入。
 */

#include "common/BaseTestServer.hpp"
#include "common/scoreboard/core/Score.hpp"
#include "common/scoreboard/core/ScoreCriteria.hpp"
#include "common/scoreboard/core/ScoreObjective.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/TextStyle.hpp"
#include "common/util/text/numbers/NumberFormat.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/ScoreboardCommand.hpp"
#include "server/scoreboard/ServerScoreboard.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>

using namespace mc;
using namespace mc::command;
using namespace mc::server;
namespace numbers = mc::text::numbers;

class ScoreboardNumberFormatCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        scoreboard::ScoreCriteriaRegistry::instance().registerBuiltinCriteria();
        ScoreboardCommand::registerTo(m_server.commandRegistry().dispatcher());
        m_source = ServerCommandSource::forConsole(&m_server);
        // 每个用例都从「存在一个 dummy 判据目标」开始
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

// ============================================================================
// 目标级格式
// ============================================================================

TEST_F(ScoreboardNumberFormatCommandTest, BlankSetsBlankFormat)
{
    ASSERT_EQ(_run("scoreboard objectives modify kills numberformat blank"), 1);

    auto* objective = _objective();
    ASSERT_NE(objective, nullptr);
    ASSERT_TRUE(objective->getNumberFormat().has_value());
    EXPECT_EQ(objective->getNumberFormat()->type(), numbers::NumberFormatType::Blank);
}

TEST_F(ScoreboardNumberFormatCommandTest, NoSubNodeClearsFormat)
{
    ASSERT_EQ(_run("scoreboard objectives modify kills numberformat blank"), 1);
    ASSERT_TRUE(_objective()->getNumberFormat().has_value());

    // 原版 numberformat 本身可执行，语义是「清除自定义格式」（传 null），不是 blank
    ASSERT_EQ(_run("scoreboard objectives modify kills numberformat"), 1);
    EXPECT_FALSE(_objective()->getNumberFormat().has_value());
}

TEST_F(ScoreboardNumberFormatCommandTest, StyledSetsStyle)
{
    ASSERT_EQ(_run("scoreboard objectives modify kills numberformat styled {color:\"red\",bold:true}"), 1);

    auto* objective = _objective();
    ASSERT_TRUE(objective->getNumberFormat().has_value());
    const auto& format = *objective->getNumberFormat();
    EXPECT_EQ(format.type(), numbers::NumberFormatType::Styled);
    ASSERT_NE(format.style(), nullptr);
    EXPECT_EQ(format.style()->getColor(), text::TextFormatting::Red);
    EXPECT_EQ(format.style()->bold(), true);

    // 渲染：数字套样式
    const auto rendered = format.format(12);
    ASSERT_NE(rendered, nullptr);
    EXPECT_EQ(rendered->getUnformattedText(), "12");
    EXPECT_EQ(rendered->getStyle().getColor(), text::TextFormatting::Red);
}

TEST_F(ScoreboardNumberFormatCommandTest, FixedSetsComponent)
{
    ASSERT_EQ(_run("scoreboard objectives modify kills numberformat fixed Max"), 1);

    auto* objective = _objective();
    ASSERT_TRUE(objective->getNumberFormat().has_value());
    const auto& format = *objective->getNumberFormat();
    EXPECT_EQ(format.type(), numbers::NumberFormatType::Fixed);
    ASSERT_NE(format.value(), nullptr);
    EXPECT_EQ(format.value()->getUnformattedText(), "Max");

    const auto rendered = format.format(999);
    ASSERT_NE(rendered, nullptr);
    EXPECT_EQ(rendered->getUnformattedText(), "Max");
}

TEST_F(ScoreboardNumberFormatCommandTest, FixedAcceptsJsonComponent)
{
    ASSERT_EQ(_run("scoreboard objectives modify kills numberformat fixed {\"text\":\"N/A\",\"bold\":true}"), 1);

    const auto& format = *_objective()->getNumberFormat();
    EXPECT_EQ(format.type(), numbers::NumberFormatType::Fixed);
    ASSERT_NE(format.value(), nullptr);
    EXPECT_EQ(format.value()->getUnformattedText(), "N/A");
    EXPECT_EQ(format.value()->getStyle().bold(), true);
}

TEST_F(ScoreboardNumberFormatCommandTest, UnknownObjectiveFails)
{
    EXPECT_EQ(_run("scoreboard objectives modify missing numberformat blank"), 0);
}

TEST_F(ScoreboardNumberFormatCommandTest, InvalidStyleIsRejected)
{
    // 非法样式必须整条命令失败，且不能把目标的格式改坏
    EXPECT_EQ(_run("scoreboard objectives modify kills numberformat styled {color:\"nope\"}"), 0);
    EXPECT_FALSE(_objective()->getNumberFormat().has_value());
}

// ============================================================================
// 分数级覆盖
// ============================================================================

TEST_F(ScoreboardNumberFormatCommandTest, PlayerOverrideAppliesToSingleHolder)
{
    ASSERT_EQ(_run("scoreboard players set Steve kills 7"), 1);
    ASSERT_EQ(_run("scoreboard players display numberformat Steve kills blank"), 1);

    auto* score = _score("Steve");
    ASSERT_NE(score, nullptr);
    ASSERT_TRUE(score->getNumberFormatOverride().has_value());
    EXPECT_EQ(score->getNumberFormatOverride()->type(), numbers::NumberFormatType::Blank);
}

TEST_F(ScoreboardNumberFormatCommandTest, PlayerOverrideClearedByNoSubNode)
{
    ASSERT_EQ(_run("scoreboard players set Steve kills 7"), 1);
    ASSERT_EQ(_run("scoreboard players display numberformat Steve kills blank"), 1);
    ASSERT_EQ(_run("scoreboard players display numberformat Steve kills"), 1);

    auto* score = _score("Steve");
    ASSERT_NE(score, nullptr);
    EXPECT_FALSE(score->getNumberFormatOverride().has_value());
}

TEST_F(ScoreboardNumberFormatCommandTest, PlayerOverrideAppliesToCommaSeparatedTargets)
{
    ASSERT_EQ(_run("scoreboard players display numberformat Steve,Alex kills fixed N/A"), 2);

    for (const auto& holder : {"Steve", "Alex"}) {
        auto* score = _score(holder);
        ASSERT_NE(score, nullptr) << holder;
        ASSERT_TRUE(score->getNumberFormatOverride().has_value()) << holder;
        EXPECT_EQ(score->getNumberFormatOverride()->type(), numbers::NumberFormatType::Fixed);
    }
}

TEST_F(ScoreboardNumberFormatCommandTest, WildcardAppliesToAllTrackedHolders)
{
    ASSERT_EQ(_run("scoreboard players set Steve kills 1"), 1);
    ASSERT_EQ(_run("scoreboard players set Alex kills 2"), 1);

    // `*` = 所有已记录分数的持有者（原版 getNamesWithDefaultWildcard）
    ASSERT_EQ(_run("scoreboard players display numberformat * kills styled {italic:true}"), 2);

    for (const auto& holder : {"Steve", "Alex"}) {
        auto* score = _score(holder);
        ASSERT_NE(score, nullptr) << holder;
        ASSERT_TRUE(score->getNumberFormatOverride().has_value()) << holder;
        EXPECT_EQ(score->getNumberFormatOverride()->type(), numbers::NumberFormatType::Styled);
    }
}

TEST_F(ScoreboardNumberFormatCommandTest, OverrideWinsOverObjectiveFormat)
{
    ASSERT_EQ(_run("scoreboard objectives modify kills numberformat styled {color:\"red\"}"), 1);
    ASSERT_EQ(_run("scoreboard players set Steve kills 42"), 1);
    ASSERT_EQ(_run("scoreboard players display numberformat Steve kills blank"), 1);

    auto* objective = _objective();
    auto* score = _score("Steve");
    ASSERT_NE(score, nullptr);

    // 覆盖格式优先：目标格式是 styled，但这条分数被覆盖成 blank → 渲染空文本
    const auto rendered = score->formatValue(*objective->getNumberFormat());
    ASSERT_NE(rendered, nullptr);
    EXPECT_EQ(rendered->getUnformattedText(), "");
}

TEST_F(ScoreboardNumberFormatCommandTest, UnknownObjectiveForPlayerFails)
{
    EXPECT_EQ(_run("scoreboard players display numberformat Steve missing blank"), 0);
}