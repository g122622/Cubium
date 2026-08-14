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
 * @file SetBlockCommandTest.cpp
 * @brief SetBlockCommand 单元测试
 *
 * 测试 /setblock 命令的注册和命令解析。
 * 由于 ServerWorld 接口复杂，完整的 destroy 模式集成测试
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

class SetBlockTestServer final : public mc::test::BaseTestServer {
public:
    SetBlockTestServer()
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

class SetBlockCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
    }

    SetBlockTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

TEST_F(SetBlockCommandTest, SetBlockCommandIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "setblock") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "setblock command should be registered";
}

TEST_F(SetBlockCommandTest, SetBlockCommandHasCorrectMetadata)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    const CommandTreeNodeSnapshot* setblockNode = nullptr;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "setblock") {
            setblockNode = &node;
            break;
        }
    }

    ASSERT_NE(setblockNode, nullptr) << "setblock node should exist";
    EXPECT_TRUE(setblockNode->metadata.contains("description"));
    EXPECT_TRUE(setblockNode->metadata.contains("usage"));
}

TEST_F(SetBlockCommandTest, SetBlockCommandRequiresWorld)
{
    const auto result = m_server.commandRegistry().execute("setblock 10 64 20 minecraft:stone", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SetBlockCommandTest, SetBlockCommandRequiresPermissionLevel2)
{
    ServerCommandSource lowPermSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 0, "test");

    bool permissionDenied = false;
    try {
        const auto result = m_server.commandRegistry().execute("setblock 10 64 20 minecraft:stone", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }

    EXPECT_TRUE(permissionDenied);
}

TEST_F(SetBlockCommandTest, SetBlockCommandParsesPosition)
{
    const auto result = m_server.commandRegistry().execute("setblock 100 -64 200 minecraft:stone", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SetBlockCommandTest, SetBlockCommandParsesBlockWithNamespace)
{
    const auto result = m_server.commandRegistry().execute("setblock 0 0 0 minecraft:dirt", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SetBlockCommandTest, SetBlockCommandParsesBlockWithoutNamespace)
{
    const auto result = m_server.commandRegistry().execute("setblock 0 0 0 stone", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SetBlockCommandTest, SetBlockCommandParsesDestroyMode)
{
    const auto result = m_server.commandRegistry().execute("setblock 0 0 0 stone destroy", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SetBlockCommandTest, SetBlockCommandParsesKeepMode)
{
    const auto result = m_server.commandRegistry().execute("setblock 0 0 0 stone keep", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SetBlockCommandTest, SetBlockCommandParsesReplaceMode)
{
    const auto result = m_server.commandRegistry().execute("setblock 0 0 0 stone replace", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(SetBlockCommandTest, SetBlockCommandWithInvalidBlockReturnsZero)
{
    // 无效方块名在命令参数解析阶段即失败（对齐 MC Java：未知方块抛解析错误，
    // 命令不执行），故 result.success() 为 false。此前该用例因 source.world()
    // 走 throwUnused 崩溃而从未真正执行，修复 dimensionManager 后才暴露真实行为。
    const auto result = m_server.commandRegistry().execute("setblock 0 0 0 minecraft:nonexistent_block", m_console);

    EXPECT_FALSE(result.success());
}

} // namespace mc::command
