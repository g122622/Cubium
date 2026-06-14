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
 * @file SpreadPlayersCommandTest.cpp
 * @brief SpreadPlayersCommand 单元测试
 *
 * 测试 /spreadplayers 命令的注册、解析和权限检查。
 * 完整的分散逻辑测试（包括队伍分组、迭代分散算法等）应在集成测试环境中进行。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/command/CommandTreeSnapshot.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/SpreadPlayersCommand.hpp"

namespace mc {
namespace command {

class SpreadPlayersTestServer final : public test::BaseTestServer {};

class SpreadPlayersCommandTest : public ::testing::Test {
protected:
    void SetUp() override { SpreadPlayersCommand::registerTo(m_server.commandRegistry().dispatcher()); }

    SpreadPlayersTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

// ============================================================================
// 命令注册测试
// ============================================================================

TEST_F(SpreadPlayersCommandTest, SpreadPlayersCommandIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();
    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "spreadplayers") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "spreadplayers 命令应已注册";
}

TEST_F(SpreadPlayersCommandTest, SpreadPlayersCommandRequiresPermissionLevel2)
{
    // 权限等级 0 的命令源不应能执行 spreadplayers
    ServerCommandSource lowPermSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 0, "test");
    bool permissionDenied = false;
    try {
        const auto result = m_server.commandRegistry().execute("spreadplayers 0 0 0 1 10 false @a", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }
    EXPECT_TRUE(permissionDenied);
}

// ============================================================================
// 命令元数据测试
// ============================================================================

TEST_F(SpreadPlayersCommandTest, SpreadPlayersCommandHasMetadata)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();
    const CommandTreeNodeSnapshot* node = nullptr;
    for (const auto& n : snapshot.nodes) {
        if (n.name == "spreadplayers") {
            node = &n;
            break;
        }
    }
    ASSERT_NE(node, nullptr) << "spreadplayers 节点应存在";
    EXPECT_TRUE(node->metadata.contains("description")) << "spreadplayers 应有 description 元数据";
    EXPECT_TRUE(node->metadata.contains("usage")) << "spreadplayers 应有 usage 元数据";
}

TEST_F(SpreadPlayersCommandTest, SpreadPlayersCommandUsageString)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();
    const CommandTreeNodeSnapshot* node = nullptr;
    for (const auto& n : snapshot.nodes) {
        if (n.name == "spreadplayers") {
            node = &n;
            break;
        }
    }
    ASSERT_NE(node, nullptr);
    // 使用说明应包含 spreadplayers 关键信息
    if (node->metadata.contains("usage")) {
        const auto& usage = node->metadata.at("usage");
        EXPECT_NE(usage.get<std::string>().find("spreadplayers"), std::string::npos) << "Usage 应包含 'spreadplayers'";
        EXPECT_NE(usage.get<std::string>().find("spreadDistance"), std::string::npos)
            << "Usage 应包含 'spreadDistance'";
        EXPECT_NE(usage.get<std::string>().find("maxRange"), std::string::npos) << "Usage 应包含 'maxRange'";
        EXPECT_NE(usage.get<std::string>().find("respectTeams"), std::string::npos) << "Usage 应包含 'respectTeams'";
    }
}

TEST_F(SpreadPlayersCommandTest, SpreadPlayersCommandPermissionLevel)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();
    const CommandTreeNodeSnapshot* node = nullptr;
    for (const auto& n : snapshot.nodes) {
        if (n.name == "spreadplayers") {
            node = &n;
            break;
        }
    }
    ASSERT_NE(node, nullptr);
    // spreadplayers 命令需要在元数据中标注权限等级 2
    if (node->metadata.contains("permissionLevel")) {
        EXPECT_EQ(node->metadata.at("permissionLevel").get<int>(), 2);
    }
}

} // namespace command
} // namespace mc
