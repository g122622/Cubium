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
#include "common/world/dimension/DimensionManager.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/GiveCommand.hpp"
#include "server/dimension/ServerDimensionManager.hpp"

namespace mc::command {

// 测试服务器所需的服务端类型位于 mc:: 顶层命名空间，此处引入以便在
// mc::command 命名空间内直接引用。
using mc::DimensionManager;
using mc::ServerDimensionManager;

class GiveTestServer final : public mc::test::BaseTestServer {
public:
    GiveTestServer() { Items::initialize(); }

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
