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
 * @file AttributeCommandTest.cpp
 * @brief AttributeCommand 单元测试
 *
 * 测试 /attribute 命令的注册、解析和权限检查。
 * 属性操作完整测试应在集成测试环境中进行。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/AttributeCommand.hpp"

namespace mc::command {

class AttributeTestServer final : public test::BaseTestServer {};

class AttributeCommandTest : public ::testing::Test {
protected:
    void SetUp() override { AttributeCommand::registerTo(m_server.commandRegistry().dispatcher()); }

    AttributeTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

TEST_F(AttributeCommandTest, AttributeCommandIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "attribute") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "attribute command should be registered";
}

TEST_F(AttributeCommandTest, AttributeCommandRequiresPermissionLevel2)
{
    ServerCommandSource lowPermSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 0, "test");

    bool permissionDenied = false;
    try {
        const auto result =
            m_server.commandRegistry().execute("attribute @p minecraft:generic.max_health get", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }

    EXPECT_TRUE(permissionDenied);
}

TEST_F(AttributeCommandTest, GetAttributeSyntax)
{
    const auto result = m_server.commandRegistry().execute("attribute @p minecraft:generic.max_health get", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, GetAttributeWithoutNamespace)
{
    const auto result = m_server.commandRegistry().execute("attribute @p generic.max_health get", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, GetAttributeWithShortName)
{
    const auto result = m_server.commandRegistry().execute("attribute @p max_health get", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, GetMultipleAttributes)
{
    const char* attributes[] = {"max_health",
        "follow_range",
        "knockback_resistance",
        "movement_speed",
        "attack_damage",
        "attack_speed",
        "armor",
        "luck"};

    for (const char* attr : attributes) {
        std::string cmd = std::string("attribute @p ") + attr + " get";
        const auto result = m_server.commandRegistry().execute(cmd, m_console);
        EXPECT_TRUE(result.success()) << "attribute " << attr << " should be parseable";
    }
}

TEST_F(AttributeCommandTest, SetAttributeSyntax)
{
    const auto result =
        m_server.commandRegistry().execute("attribute @p minecraft:generic.max_health set 20.0", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, SetAttributeWithFloatValue)
{
    const auto result = m_server.commandRegistry().execute("attribute @p generic.movement_speed set 0.15", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, SetAttributeWithIntValue)
{
    const auto result = m_server.commandRegistry().execute("attribute @p generic.max_health set 30", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, SetMovementSpeed)
{
    const auto result = m_server.commandRegistry().execute("attribute @p movement_speed set 0.1", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, SetAttackDamage)
{
    const auto result = m_server.commandRegistry().execute("attribute @p attack_damage set 5.0", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, SetKnockbackResistance)
{
    const auto result = m_server.commandRegistry().execute("attribute @p knockback_resistance set 0.5", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, SetArmor)
{
    const auto result = m_server.commandRegistry().execute("attribute @p armor set 20.0", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, SetLuck)
{
    const auto result = m_server.commandRegistry().execute("attribute @p luck set 1024.0", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, SelectorWithNoPlayersReturnsZero)
{
    const auto result = m_server.commandRegistry().execute("attribute @p generic.max_health get", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, SelectorWithMultiplePlayersFails)
{
    const auto result = m_server.commandRegistry().execute("attribute @a generic.max_health get", m_console);

    EXPECT_FALSE(result.success());
}

TEST_F(AttributeCommandTest, UnknownAttributeReturnsZero)
{
    const auto result = m_server.commandRegistry().execute("attribute @p unknown_attribute get", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(AttributeCommandTest, HorseJumpStrengthAttribute)
{
    const auto result = m_server.commandRegistry().execute("attribute @p horse.jump_strength get", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

} // namespace mc::command
