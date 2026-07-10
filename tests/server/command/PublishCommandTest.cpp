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
 * @file PublishCommandTest.cpp
 * @brief PublishCommand 单元测试
 *
 * 测试 /publish 命令的服务器类型检查和基本功能。
 * 局域网发布的完整测试应在集成测试环境中进行。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/PublishCommand.hpp"
#include "server/dimension/ServerDimensionManager.hpp"

namespace mc::command {

// 测试服务器所需的服务端类型位于 mc:: 顶层命名空间，此处引入以便在
// mc::command 命名空间内直接引用。
using mc::DimensionManager;
using mc::ServerDimensionManager;

class DedicatedTestServer final : public test::BaseTestServer {
public:
    [[nodiscard]] bool isIntegrated() const noexcept override { return false; }
    [[nodiscard]] bool isDedicated() const noexcept override { return true; }

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

class IntegratedTestServer final : public test::BaseTestServer {
public:
    [[nodiscard]] bool isIntegrated() const noexcept override { return true; }
    [[nodiscard]] bool isDedicated() const noexcept override { return false; }

    // 测试桩：模拟局域网发布成功
    [[nodiscard]] Result<void> publishToLan(i32 port, bool allowCheats) override
    {
        m_publishCalled = true;
        m_lastPort = port;
        m_lastAllowCheats = allowCheats;
        return Result<void>::ok();
    }

    [[nodiscard]] bool publishCalled() const noexcept { return m_publishCalled; }
    [[nodiscard]] i32 lastPublishedPort() const noexcept { return m_lastPort; }
    [[nodiscard]] bool lastAllowCheats() const noexcept { return m_lastAllowCheats; }

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
    bool m_publishCalled = false;
    i32 m_lastPort = 0;
    bool m_lastAllowCheats = false;
};

TEST(DedicatedTestServerTypeTest, IsIntegratedReturnsFalse)
{
    DedicatedTestServer server;
    EXPECT_FALSE(server.isIntegrated());
}

TEST(DedicatedTestServerTypeTest, IsDedicatedReturnsTrue)
{
    DedicatedTestServer server;
    EXPECT_TRUE(server.isDedicated());
}

TEST(IntegratedTestServerTypeTest, IsIntegratedReturnsTrue)
{
    IntegratedTestServer server;
    EXPECT_TRUE(server.isIntegrated());
}

TEST(IntegratedTestServerTypeTest, IsDedicatedReturnsFalse)
{
    IntegratedTestServer server;
    EXPECT_FALSE(server.isDedicated());
}

class PublishCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        PublishCommand::registerTo(m_integratedServer.commandRegistry().dispatcher());
        PublishCommand::registerTo(m_dedicatedServer.commandRegistry().dispatcher());
    }

    IntegratedTestServer m_integratedServer;
    DedicatedTestServer m_dedicatedServer;
};

TEST_F(PublishCommandTest, PublishCommandIsRegistered)
{
    const auto& registry = m_integratedServer.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "publish") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "publish command should be registered";
}

TEST_F(PublishCommandTest, PublishCommandRequiresPermissionLevel4)
{
    ServerCommandSource noPermSource(&m_integratedServer, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0);

    const auto result = m_integratedServer.commandRegistry().execute("publish", noPermSource);

    EXPECT_FALSE(result.success());
}

TEST_F(PublishCommandTest, PublishCommandFailsOnDedicatedServer)
{
    ServerCommandSource source(&m_dedicatedServer, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 4);

    const auto result = m_dedicatedServer.commandRegistry().execute("publish", source);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(PublishCommandTest, PublishCommandSucceedsOnIntegratedServer)
{
    ServerCommandSource source(&m_integratedServer, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 4);

    const auto result = m_integratedServer.commandRegistry().execute("publish", source);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
    EXPECT_TRUE(m_integratedServer.publishCalled());
    // 默认端口应为 25565
    EXPECT_EQ(m_integratedServer.lastPublishedPort(), 25565);
    EXPECT_FALSE(m_integratedServer.lastAllowCheats());
}

TEST_F(PublishCommandTest, PublishCommandWithCustomPort)
{
    ServerCommandSource source(&m_integratedServer, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 4);

    const auto result = m_integratedServer.commandRegistry().execute("publish 25566", source);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
    EXPECT_TRUE(m_integratedServer.publishCalled());
    EXPECT_EQ(m_integratedServer.lastPublishedPort(), 25566);
    EXPECT_FALSE(m_integratedServer.lastAllowCheats());
}

TEST_F(PublishCommandTest, PublishCommandWithPortAndCheats)
{
    ServerCommandSource source(&m_integratedServer, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 4);

    const auto result = m_integratedServer.commandRegistry().execute("publish 25566 true", source);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
    EXPECT_TRUE(m_integratedServer.publishCalled());
    EXPECT_EQ(m_integratedServer.lastPublishedPort(), 25566);
    EXPECT_TRUE(m_integratedServer.lastAllowCheats());
}

TEST_F(PublishCommandTest, PublishCommandWithCheatsOnly)
{
    ServerCommandSource source(&m_integratedServer, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 4);

    // 仅指定 cheats 参数，端口使用默认值
    const auto result = m_integratedServer.commandRegistry().execute("publish true", source);

    // publish 命令的语法是 /publish [port] [allowCheats]，不支持仅指定 cheats
    // "true" 会被尝试解析为 port，但 bool 类型不匹配 integer，所以应该失败
    EXPECT_FALSE(result.success());
}

TEST_F(PublishCommandTest, PublishCommandInvalidPortTooLow)
{
    ServerCommandSource source(&m_integratedServer, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 4);

    const auto result = m_integratedServer.commandRegistry().execute("publish 0", source);

    EXPECT_FALSE(result.success());
}

TEST_F(PublishCommandTest, PublishCommandInvalidPortTooHigh)
{
    ServerCommandSource source(&m_integratedServer, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 4);

    const auto result = m_integratedServer.commandRegistry().execute("publish 70000", source);

    EXPECT_FALSE(result.success());
}

} // namespace mc::command
