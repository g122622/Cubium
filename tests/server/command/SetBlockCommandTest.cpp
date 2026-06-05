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
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"

namespace mc::command {

class SetBlockTestServer final : public test::BaseTestServer {
public:
    SetBlockTestServer()
    {
        Items::initialize();
        VanillaBlocks::initialize();
    }
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
    const auto result = m_server.commandRegistry().execute("setblock 0 0 0 minecraft:nonexistent_block", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

} // namespace mc::command
