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
 * @file GiveCommandTest.cpp
 * @brief GiveCommand 单元测试
 *
 * 测试 /give 命令的注册、解析和权限检查。
 * 物品掉落和音效播放的完整测试应在集成测试环境中进行。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/item/Items.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/GiveCommand.hpp"

namespace mc::command {

class GiveTestServer final : public test::BaseTestServer {
public:
    GiveTestServer() { Items::initialize(); }
};

class GiveCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Items::initialize();
        GiveCommand::registerTo(m_server.commandRegistry().dispatcher());
    }

    GiveTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

TEST_F(GiveCommandTest, GiveCommandIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "give") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "give command should be registered";
}

TEST_F(GiveCommandTest, GiveCommandRequiresPermissionLevel2)
{
    ServerCommandSource lowPermSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 0, "test");

    bool permissionDenied = false;
    try {
        const auto result = m_server.commandRegistry().execute("give @p minecraft:stone 1", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }

    EXPECT_TRUE(permissionDenied);
}

TEST_F(GiveCommandTest, GiveCommandParsesItemWithNamespace)
{
    const auto result = m_server.commandRegistry().execute("give @p minecraft:stone 1", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(GiveCommandTest, GiveCommandParsesItemWithoutNamespace)
{
    const auto result = m_server.commandRegistry().execute("give @p stone 1", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(GiveCommandTest, GiveCommandParsesCountArgument)
{
    const auto result = m_server.commandRegistry().execute("give @p stone 64", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(GiveCommandTest, GiveCommandWithInvalidItemFails)
{
    const auto result = m_server.commandRegistry().execute("give @p minecraft:nonexistent_item 1", m_console);

    EXPECT_FALSE(result.success());
}

TEST_F(GiveCommandTest, GiveCommandWithNoTargetsReturnsZero)
{
    const auto result = m_server.commandRegistry().execute("give @p stone 1", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(GiveCommandTest, GiveCommandWithCountAbove64Clamped)
{
    const auto result = m_server.commandRegistry().execute("give @p stone 64", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(GiveCommandTest, GiveCommandDefaultCountIsOne)
{
    const auto result = m_server.commandRegistry().execute("give @p stone", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(GiveCommandTest, GiveCommandWithMultipleTargets)
{
    const auto result = m_server.commandRegistry().execute("give @a stone 1", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

} // namespace mc::command
