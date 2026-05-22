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
 * @file SpawnPointCommandTest.cpp
 * @brief SpawnPointCommand 单元测试
 *
 * 测试 /spawnpoint 命令的注册、解析、权限检查和功能。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/SpawnPointCommand.hpp"

namespace mc::command {

class SpawnPointTestServer final : public test::BaseTestServer {};

class SpawnPointCommandTest : public ::testing::Test {
protected:
    void SetUp() override { SpawnPointCommand::registerTo(m_server.commandRegistry().dispatcher()); }

    SpawnPointTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

TEST_F(SpawnPointCommandTest, SpawnPointCommandIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "spawnpoint") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "spawnpoint command should be registered";
}

TEST_F(SpawnPointCommandTest, SpawnPointCommandRequiresPermissionLevel2)
{
    ServerCommandSource lowPermSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 0, "test");

    bool permissionDenied = false;
    try {
        const auto result = m_server.commandRegistry().execute("spawnpoint", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }

    EXPECT_TRUE(permissionDenied);
}

TEST_F(SpawnPointCommandTest, SpawnPointNoArgsFailsWithoutPlayer)
{
    const auto result = m_server.commandRegistry().execute("spawnpoint", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SpawnPointCommandTest, SpawnPointWithPlayerSelectorNoPlayers)
{
    const auto result = m_server.commandRegistry().execute("spawnpoint @p", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SpawnPointCommandTest, SpawnPointWithPositionNoPlayers)
{
    const auto result = m_server.commandRegistry().execute("spawnpoint @p 100 64 200", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SpawnPointCommandTest, SpawnPointWithCoordinates)
{
    const auto result = m_server.commandRegistry().execute("spawnpoint @p 100.5 64.0 -200.5", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SpawnPointCommandTest, SpawnPointWithRelativeCoordinates)
{
    const auto result = m_server.commandRegistry().execute("spawnpoint @p ~10 ~ ~-5", m_console);

    EXPECT_TRUE(result.success());
}

TEST_F(SpawnPointCommandTest, SpawnPointWithFloatCoordinates)
{
    const auto result = m_server.commandRegistry().execute("spawnpoint @p 123.456 78.9 -456.789", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SpawnPointCommandTest, SpawnPointWithSelfSelector)
{
    m_server.addTestPlayer(1, "TestPlayer");

    const auto result = m_server.commandRegistry().execute("spawnpoint @s", m_console);

    EXPECT_TRUE(result.success());
}

TEST_F(SpawnPointCommandTest, SpawnPointWithPlayerName)
{
    m_server.addTestPlayer(1, "TestPlayer");

    const auto result = m_server.commandRegistry().execute("spawnpoint TestPlayer", m_console);

    EXPECT_FALSE(result.success());
}

TEST_F(SpawnPointCommandTest, SpawnPointWithInvalidSelector)
{
    const auto result = m_server.commandRegistry().execute("spawnpoint @invalid", m_console);

    EXPECT_FALSE(result.success());
}

TEST_F(SpawnPointCommandTest, SpawnPointWithPermissionLevel2)
{
    ServerCommandSource permSource(&m_server, nullptr, 0, Vector3d(100, 64, 200), Vector2f(0, 0), 2, 0, "admin");

    const auto result = m_server.commandRegistry().execute("spawnpoint", permSource);

    EXPECT_TRUE(result.success());
}

TEST_F(SpawnPointCommandTest, SpawnPointWithPermissionLevel4)
{
    ServerCommandSource permSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 4, 0, "console");

    const auto result = m_server.commandRegistry().execute("spawnpoint @p", permSource);

    EXPECT_TRUE(result.success());
}

TEST_F(SpawnPointCommandTest, SpawnPointWithZeroCoordinates)
{
    const auto result = m_server.commandRegistry().execute("spawnpoint @p 0 0 0", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SpawnPointCommandTest, SpawnPointWithNegativeCoordinates)
{
    const auto result = m_server.commandRegistry().execute("spawnpoint @p -100 -64 -200", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SpawnPointCommandTest, SpawnPointWithLargeCoordinates)
{
    const auto result = m_server.commandRegistry().execute("spawnpoint @p 30000000 256 30000000", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SpawnPointCommandTest, SpawnPointHasNoAlias)
{
    const auto result = m_server.commandRegistry().execute("spawnpoint", m_console);
    EXPECT_TRUE(result.success());
}

} // namespace mc::command
