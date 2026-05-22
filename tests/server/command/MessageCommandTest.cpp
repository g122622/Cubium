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
 * @file MessageCommandTest.cpp
 * @brief MessageCommand 单元测试
 *
 * 测试 /msg 命令的注册、解析和权限检查。
 * 私聊消息发送的完整测试应在集成测试环境中进行。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/MessageCommand.hpp"

namespace mc::command {

class MessageTestServer final : public test::BaseTestServer {};

class MessageCommandTest : public ::testing::Test {
protected:
    void SetUp() override { MessageCommand::registerTo(m_server.commandRegistry().dispatcher()); }

    MessageTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

TEST_F(MessageCommandTest, MessageCommandIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "msg") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "msg command should be registered";
}

TEST_F(MessageCommandTest, TellAliasIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "tell") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "tell alias should be registered";
}

TEST_F(MessageCommandTest, WAliasIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "w") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "w alias should be registered";
}

TEST_F(MessageCommandTest, MessageCommandRequiresPermissionLevel0)
{
    ServerCommandSource noPermSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 0, "test");

    const auto result = m_server.commandRegistry().execute("msg TestPlayer hello", noPermSource);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(MessageCommandTest, MessageCommandParsesTargetAndMessage)
{
    const auto result = m_server.commandRegistry().execute("msg TestPlayer Hello World", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(MessageCommandTest, MessageCommandWithNoTargetsReturnsZero)
{
    const auto result = m_server.commandRegistry().execute("msg @p hello", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(MessageCommandTest, MessageCommandWithSelector)
{
    const auto result = m_server.commandRegistry().execute("msg @p hello", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(MessageCommandTest, MessageCommandWithQuotedMessage)
{
    const auto result = m_server.commandRegistry().execute("msg TestPlayer \"Hello World\"", m_console);

    EXPECT_TRUE(result.success());
}

} // namespace mc::command
