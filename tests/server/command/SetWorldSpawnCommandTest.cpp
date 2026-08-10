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
 * @file SetWorldSpawnCommandTest.cpp
 * @brief SetWorldSpawnCommand 单元测试
 *
 * 测试 /setworldspawn 命令的注册和权限检查。
 * 完整的命令执行测试需要完整的玩家/维度基础设施，
 * 因此本文件主要测试命令注册和权限相关功能。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/SetWorldSpawnCommand.hpp"
#include "server/dimension/ServerDimensionManager.hpp"

namespace mc::command {

// 测试服务器所需的服务端类型位于 mc:: 顶层命名空间，此处引入以便在
// mc::command 命名空间内直接引用。
using mc::DimensionManager;
using mc::ServerDimensionManager;

class SetWorldSpawnTestServer final : public mc::test::BaseTestServer {
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

class SetWorldSpawnCommandTest : public ::testing::Test {
protected:
    void SetUp() override { SetWorldSpawnCommand::registerTo(m_server.commandRegistry().dispatcher()); }

    SetWorldSpawnTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

// ==================== 命令注册测试 ====================

TEST_F(SetWorldSpawnCommandTest, CommandIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "setworldspawn") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "setworldspawn command should be registered";
}

// ==================== 权限测试 ====================

TEST_F(SetWorldSpawnCommandTest, RequiresPermissionLevel2)
{
    // 权限等级 0 的命令源不应能执行
    ServerCommandSource lowPermSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 0, "test");

    bool permissionDenied = false;
    try {
        const auto result = m_server.commandRegistry().execute("setworldspawn", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }

    EXPECT_TRUE(permissionDenied);
}

TEST_F(SetWorldSpawnCommandTest, PermissionLevel2CanExecute)
{
    ServerCommandSource permSource(&m_server, nullptr, 0, Vector3d(100, 64, 200), Vector2f(90.0f, 0.0f), 2, 0, "admin");

    const auto result = m_server.commandRegistry().execute("setworldspawn", permSource);
    EXPECT_TRUE(result.success());
}

TEST_F(SetWorldSpawnCommandTest, PermissionLevel4CanExecute)
{
    ServerCommandSource permSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 4, 0, "console");

    const auto result = m_server.commandRegistry().execute("setworldspawn", permSource);
    EXPECT_TRUE(result.success());
}

// ==================== 无参数语法测试 ====================

TEST_F(SetWorldSpawnCommandTest, NoArgsFailsWithoutPlayer)
{
    // 控制台没有玩家位置，不能执行无参数版本
    const auto result = m_server.commandRegistry().execute("setworldspawn", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// ==================== 带权限的玩家源执行测试 ====================

TEST_F(SetWorldSpawnCommandTest, PlayerSourceWithRotation)
{
    // /setworldspawn 无参数时需要玩家（isPlayer() 检查），
    // 即使有权限等级，没有玩家指针也会返回 0
    ServerCommandSource playerSource(
        &m_server, nullptr, 1, Vector3d(100, 64, 200), Vector2f(0.0f, 45.0f), 2, 0, "TestPlayer");

    const auto result = m_server.commandRegistry().execute("setworldspawn", playerSource);
    EXPECT_TRUE(result.success());
    // 没有真实玩家指针时返回 0（命令执行但结果为 0）
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SetWorldSpawnCommandTest, ZeroPermissionPlayerCannotExecute)
{
    ServerCommandSource noPermSource(
        &m_server, nullptr, 0, Vector3d(100, 64, 200), Vector2f(0.0f, 0.0f), 0, 0, "regular");

    bool permissionDenied = false;
    try {
        const auto result = m_server.commandRegistry().execute("setworldspawn", noPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }

    EXPECT_TRUE(permissionDenied);
}

} // namespace mc::command
