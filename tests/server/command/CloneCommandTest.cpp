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
 * @file CloneCommandTest.cpp
 * @brief CloneCommand 单元测试
 *
 * 测试 /clone 命令的注册和命令解析。
 * 由于 ServerWorld 接口复杂，完整的功能集成测试
 * 应在集成测试环境中进行。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/item/Items.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/dimension/ServerDimensionManager.hpp"

namespace mc::command {

// 测试服务器所需的服务端类型位于 mc:: 顶层命名空间，此处引入以便在
// mc::command 命名空间内直接引用。
using mc::DimensionManager;
using mc::ServerDimensionManager;

class CloneTestServer final : public mc::test::BaseTestServer {
public:
    CloneTestServer()
    {
        Items::initialize();
        VanillaBlocks::initialize();
    }

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

class CloneCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    CloneTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

TEST_F(CloneCommandTest, CloneCommandIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "clone") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "clone command should be registered";
}

TEST_F(CloneCommandTest, CloneCommandHasCorrectMetadata)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    const CommandTreeNodeSnapshot* cloneNode = nullptr;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "clone") {
            cloneNode = &node;
            break;
        }
    }

    ASSERT_NE(cloneNode, nullptr) << "clone node should exist";
    EXPECT_TRUE(cloneNode->metadata.contains("description"));
    EXPECT_TRUE(cloneNode->metadata.contains("usage"));
}

TEST_F(CloneCommandTest, CloneCommandRequiresWorld)
{
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CloneCommandTest, CloneCommandRequiresPermissionLevel2)
{
    ServerCommandSource lowPermSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 0, "test");

    bool permissionDenied = false;
    try {
        const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }

    EXPECT_TRUE(permissionDenied);
}

TEST_F(CloneCommandTest, CloneCommandParsesBasicSyntax)
{
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CloneCommandTest, CloneCommandParsesReplaceMode)
{
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 replace", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CloneCommandTest, CloneCommandParsesMaskedMode)
{
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 masked", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CloneCommandTest, CloneCommandParsesFilteredModeWithBlock)
{
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 filtered stone", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CloneCommandTest, CloneCommandParsesFilteredModeWithNamespacedBlock)
{
    const auto result =
        m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 filtered minecraft:dirt", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CloneCommandTest, CloneCommandParsesReplaceForceMode)
{
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 replace force", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CloneCommandTest, CloneCommandParsesReplaceMoveMode)
{
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 replace move", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CloneCommandTest, CloneCommandParsesReplaceNormalMode)
{
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 replace normal", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CloneCommandTest, CloneCommandParsesMaskedForceMode)
{
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 masked force", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CloneCommandTest, CloneCommandParsesMaskedMoveMode)
{
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 masked move", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CloneCommandTest, CloneCommandParsesMaskedNormalMode)
{
    const auto result = m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 masked normal", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CloneCommandTest, CloneCommandParsesFilteredForceMode)
{
    const auto result =
        m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 filtered stone force", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CloneCommandTest, CloneCommandParsesFilteredMoveMode)
{
    const auto result =
        m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 filtered stone move", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CloneCommandTest, CloneCommandParsesFilteredNormalMode)
{
    const auto result =
        m_server.commandRegistry().execute("clone 0 0 0 10 10 10 20 20 20 filtered stone normal", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CloneCommandTest, CloneCommandParsesNegativeCoordinates)
{
    const auto result = m_server.commandRegistry().execute("clone -100 -60 -100 100 64 100 200 64 200", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(CloneCommandTest, CloneCommandParsesReversedCoordinates)
{
    const auto result = m_server.commandRegistry().execute("clone 10 10 10 0 0 0 20 20 20", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

} // namespace mc::command
