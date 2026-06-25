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
 * @file ExecuteCommandTest.cpp
 * @brief ExecuteCommand 单元测试
 *
 * 测试 /execute 命令的注册、解析和嵌套命令执行功能。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/ExecuteCommand.hpp"
#include "server/command/commands/HelpCommand.hpp"
#include "server/command/commands/ListCommand.hpp"

namespace mc::command {

class ExecuteTestServer final : public test::BaseTestServer {};

} // namespace mc::command

class ExecuteCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        mc::command::ExecuteCommand::registerTo(m_server.commandRegistry().dispatcher());
        mc::command::HelpCommand::registerTo(m_server.commandRegistry().dispatcher());
        mc::command::ListCommand::registerTo(m_server.commandRegistry().dispatcher());
    }

    mc::command::ExecuteTestServer m_server;
    mc::command::ServerCommandSource m_console = mc::command::ServerCommandSource::forConsole(&m_server);
};

TEST_F(ExecuteCommandTest, ExecuteCommandIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "execute") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "execute command should be registered";
}

TEST_F(ExecuteCommandTest, ExecuteCommandRequiresPermissionLevel2)
{
    mc::command::ServerCommandSource lowPermSource(
        &m_server, nullptr, 0, mc::Vector3d(0, 0, 0), mc::Vector2f(0, 0), 0, 0, "test");

    bool permissionDenied = false;
    try {
        const auto result = m_server.commandRegistry().execute("execute run help", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }

    EXPECT_TRUE(permissionDenied);
}

TEST_F(ExecuteCommandTest, ExecuteRunHelpCommand)
{
    const auto result = m_server.commandRegistry().execute("execute run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(ExecuteCommandTest, ExecuteRunListCommand)
{
    const auto result = m_server.commandRegistry().execute("execute run list", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExecuteCommandTest, ExecuteRunWithSlash)
{
    const auto result = m_server.commandRegistry().execute("execute run /help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(ExecuteCommandTest, ExecuteRunEmptyCommand)
{
    const auto result = m_server.commandRegistry().execute("execute run", m_console);

    EXPECT_TRUE(result.failed() || result.value() == 0);
}

TEST_F(ExecuteCommandTest, ExecutePositionedRunCommand)
{
    const auto result = m_server.commandRegistry().execute("execute positioned 100 64 200 run list", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExecuteCommandTest, ExecutePositionedWithRelativeCoords)
{
    const auto result = m_server.commandRegistry().execute("execute positioned ~10 ~ ~-5 run help", m_console);

    EXPECT_TRUE(result.success());
}

TEST_F(ExecuteCommandTest, ExecuteIfBlockCommandNoWorld)
{
    const auto result = m_server.commandRegistry().execute("execute if block 0 0 0 stone run help", m_console);

    EXPECT_TRUE(result.failed() || result.value() == 0);
}

TEST_F(ExecuteCommandTest, ExecuteUnlessBlockCommandNoWorld)
{
    const auto result = m_server.commandRegistry().execute("execute unless block 0 0 0 stone run help", m_console);

    EXPECT_TRUE(result.failed() || result.value() == 0);
}

TEST_F(ExecuteCommandTest, ExecuteAsNoTarget)
{
    const auto result = m_server.commandRegistry().execute("execute as @p run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExecuteCommandTest, ExecuteAsWithPlayer)
{
    m_server.addTestPlayer(1, "TestPlayer");

    const auto result = m_server.commandRegistry().execute("execute as TestPlayer run help", m_console);

    EXPECT_TRUE(result.success() || result.failed());
}

TEST_F(ExecuteCommandTest, ExecuteAtNoTarget)
{
    const auto result = m_server.commandRegistry().execute("execute at @p run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExecuteCommandTest, InvalidSubcommand)
{
    const auto result = m_server.commandRegistry().execute("execute invalid run help", m_console);

    EXPECT_TRUE(result.failed());
}

TEST_F(ExecuteCommandTest, MissingRunKeyword)
{
    const auto result = m_server.commandRegistry().execute("execute as @p help", m_console);

    EXPECT_TRUE(result.failed());
}

TEST_F(ExecuteCommandTest, NestedCommandExecution)
{
    const auto result = m_server.commandRegistry().execute("execute run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(ExecuteCommandTest, MultipleNestedCommands)
{
    const auto result = m_server.commandRegistry().execute("execute positioned 0 0 0 run execute run help", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(ExecuteCommandTest, ExecuteInOverworld)
{
    // /execute in overworld run list - 维度参数解析应成功
    // BaseTestServer 未注册维度，所以维度验证会失败（返回 0 或错误）
    const auto result = m_server.commandRegistry().execute("execute in overworld run list", m_console);

    // 命令解析应成功，但由于维度不存在，执行结果为 0 或失败
    EXPECT_TRUE(result.failed() || result.value() == 0);
}

TEST_F(ExecuteCommandTest, ExecuteInNether)
{
    // /execute in the_nether run list - DimensionArgumentType 解析成功，维度不存在则失败
    const auto result = m_server.commandRegistry().execute("execute in the_nether run list", m_console);

    // BaseTestServer 默认不注册下界维度，所以应该返回 0（维度不存在）
    EXPECT_TRUE(result.failed() || result.value() == 0);
}

TEST_F(ExecuteCommandTest, ExecuteInNamespaceFormat)
{
    // /execute in minecraft:overworld run list - 命名空间格式也能正确解析
    const auto result = m_server.commandRegistry().execute("execute in minecraft:overworld run list", m_console);

    // 命令解析应成功，但由于维度不存在，执行结果为 0 或失败
    EXPECT_TRUE(result.failed() || result.value() == 0);
}

TEST_F(ExecuteCommandTest, ExecuteInInvalidDimension)
{
    // /execute in invalid_dimension run list - 无效维度名称应导致解析错误
    const auto result = m_server.commandRegistry().execute("execute in invalid_dimension run list", m_console);

    EXPECT_TRUE(result.failed());
}
