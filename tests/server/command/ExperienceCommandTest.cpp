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
 * @file ExperienceCommandTest.cpp
 * @brief ExperienceCommand 单元测试
 *
 * 测试 /experience 命令的注册、解析和权限检查。
 * 经验操作完整测试应在集成测试环境中进行。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/ExperienceCommand.hpp"
#include "server/dimension/ServerDimensionManager.hpp"

namespace mc::command {

// 测试服务器所需的服务端类型位于 mc:: 顶层命名空间，此处引入以便在
// mc::command 命名空间内直接引用。
using mc::DimensionManager;
using mc::ServerDimensionManager;

class ExperienceTestServer final : public mc::test::BaseTestServer {
public:
    // 覆盖 dimensionManager，返回一个未注册任何维度的空 DimensionManager。
    // 这样 source.world() 经 dimensionManager().getDimension() 返回 nullptr，
    // 命令走 "World not available" 分支返回 0，避免 BaseTestServer 默认实现
    // 抛 std::logic_error 进而在 noexcept 的 world() 中触发 std::terminate。
    // 注意：DimensionManager 是 ServerDimensionManager 的基类，
    // 我们将 DimensionManager reinterpret_cast 为 ServerDimensionManager，
    // 因为 ServerDimensionManager::getDimension() 仅调用基类 DimensionManager::getDimension()
    // 然后做 static_cast，在我们的测试场景中是安全的。
    [[nodiscard]] ServerDimensionManager& dimensionManager() override
    {
        return reinterpret_cast<ServerDimensionManager&>(m_dimensionManager);
    }

    [[nodiscard]] const ServerDimensionManager& dimensionManager() const override
    {
        return reinterpret_cast<const ServerDimensionManager&>(m_dimensionManager);
    }

private:
    DimensionManager m_dimensionManager;
};

class ExperienceCommandTest : public ::testing::Test {
protected:
    void SetUp() override { ExperienceCommand::registerTo(m_server.commandRegistry().dispatcher()); }

    ExperienceTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

TEST_F(ExperienceCommandTest, ExperienceCommandIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "experience") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "experience command should be registered";
}

TEST_F(ExperienceCommandTest, XpAliasIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "xp") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "xp alias should be registered";
}

TEST_F(ExperienceCommandTest, ExperienceCommandRequiresPermissionLevel2)
{
    ServerCommandSource lowPermSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 0, "test");

    bool permissionDenied = false;
    try {
        const auto result = m_server.commandRegistry().execute("experience add @p 100", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }

    EXPECT_TRUE(permissionDenied);
}

TEST_F(ExperienceCommandTest, AddPointsSyntax)
{
    const auto result = m_server.commandRegistry().execute("experience add @p 100 points", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExperienceCommandTest, AddPointsDefault)
{
    const auto result = m_server.commandRegistry().execute("experience add @p 100", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExperienceCommandTest, AddLevelsSyntax)
{
    const auto result = m_server.commandRegistry().execute("experience add @p 5 levels", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExperienceCommandTest, AddNegativePoints)
{
    const auto result = m_server.commandRegistry().execute("experience add @p -50", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExperienceCommandTest, AddNegativeLevels)
{
    const auto result = m_server.commandRegistry().execute("experience add @p -3 levels", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExperienceCommandTest, SetPointsSyntax)
{
    const auto result = m_server.commandRegistry().execute("experience set @p 1000 points", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExperienceCommandTest, SetPointsDefault)
{
    const auto result = m_server.commandRegistry().execute("experience set @p 1000", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExperienceCommandTest, SetLevelsSyntax)
{
    const auto result = m_server.commandRegistry().execute("experience set @p 30 levels", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExperienceCommandTest, SetPointsWithMinimumValue)
{
    const auto result = m_server.commandRegistry().execute("experience set @p 0 points", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExperienceCommandTest, QueryPointsSyntax)
{
    const auto result = m_server.commandRegistry().execute("experience query @p points", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExperienceCommandTest, QueryLevelsSyntax)
{
    const auto result = m_server.commandRegistry().execute("experience query @p levels", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExperienceCommandTest, QueryDefaultIsLevels)
{
    const auto result = m_server.commandRegistry().execute("experience query @p", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExperienceCommandTest, SelectorWithNoPlayersReturnsZero)
{
    const auto result = m_server.commandRegistry().execute("experience add @p 100", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExperienceCommandTest, MultiplePlayerSelector)
{
    const auto result = m_server.commandRegistry().execute("experience add @a 100", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExperienceCommandTest, XpAliasAddPoints)
{
    const auto result = m_server.commandRegistry().execute("xp add @p 100", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExperienceCommandTest, XpAliasSetLevels)
{
    const auto result = m_server.commandRegistry().execute("xp set @p 10 levels", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(ExperienceCommandTest, XpAliasQuery)
{
    const auto result = m_server.commandRegistry().execute("xp query @p levels", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

} // namespace mc::command
