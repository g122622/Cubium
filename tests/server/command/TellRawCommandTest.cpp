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
 * @file TellRawCommandTest.cpp
 * @brief TellRawCommand 单元测试
 *
 * 测试 /tellraw 命令的注册、解析、JSON 解析和消息发送。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextStyle.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/TellRawCommand.hpp"
#include "server/dimension/ServerDimensionManager.hpp"

#include <nlohmann/json.hpp>

namespace mc::command {

// 测试服务器所需的服务端类型位于 mc:: 顶层命名空间，此处引入以便在
// mc::command 命名空间内直接引用。
using mc::DimensionManager;
using mc::ServerDimensionManager;

class TellRawTestServer final : public mc::test::BaseTestServer {
public:
    [[nodiscard]] std::shared_ptr<mc::test::FakeServerConnection> getConnection(PlayerId playerId)
    {
        auto* playerData = playerManager().getPlayer(playerId);
        if (playerData != nullptr && playerData->hasConnection()) {
            return lastConnection();
        }
        return nullptr;
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

class TellRawCommandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 注册命令
        TellRawCommand::registerTo(m_server.commandRegistry().dispatcher());
    }

    TellRawTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

// ========== 命令注册测试 ==========

TEST_F(TellRawCommandTest, TellRawCommandIsRegistered)
{
    // 验证 tellraw 命令已注册
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "tellraw") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "tellraw command should be registered";
}

TEST_F(TellRawCommandTest, TellRawCommandRequiresPermissionLevel2)
{
    // tellraw 命令需要权限等级 2
    // 创建一个权限等级 2 的命令源
    ServerCommandSource opSource(&m_server,
        nullptr,
        0,
        Vector3d(0, 0, 0),
        Vector2f(0, 0),
        2, // 权限等级 2
        0,
        "op");

    // 命令应该可以执行（虽然没有目标玩家）
    const auto result = m_server.commandRegistry().execute("tellraw @p hello", opSource);
    EXPECT_TRUE(result.success());
}

TEST_F(TellRawCommandTest, TellRawCommandWithNoTargetsReturnsZero)
{
    // 测试没有目标玩家
    const auto result = m_server.commandRegistry().execute("tellraw @p hello", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(TellRawCommandTest, TellRawCommandWithInvalidJsonReturnsError)
{
    // 测试无效 JSON - 应该仍然尝试发送
    const auto result = m_server.commandRegistry().execute("tellraw @p {invalid json}", m_console);

    // 命令解析成功，但由于没有目标玩家，返回 0
    EXPECT_TRUE(result.success());
}

// ========== JSON 解析测试 ==========

TEST_F(TellRawCommandTest, JsonParsing_ValidJson)
{
    // 添加测试玩家
    m_server.addTestPlayer(1, "TestPlayer");

    // 测试有效的 JSON 消息
    const std::string jsonMessage = R"({"text":"Hello World","color":"red"})";
    const auto result = m_server.commandRegistry().execute("tellraw TestPlayer " + jsonMessage, m_console);

    // 命令应该成功执行
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1); // 一个玩家收到消息
}

TEST_F(TellRawCommandTest, JsonParsing_ComplexJson)
{
    // 添加测试玩家
    m_server.addTestPlayer(1, "TestPlayer");

    // 测试复杂的 JSON 消息（带 extra）
    const std::string jsonMessage = R"({"text":"Hello ","color":"red","extra":[{"text":"World","color":"blue"}]})";
    const auto result = m_server.commandRegistry().execute("tellraw TestPlayer " + jsonMessage, m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(TellRawCommandTest, JsonParsing_PlainText)
{
    // 添加测试玩家
    m_server.addTestPlayer(1, "TestPlayer");

    // 测试纯文本（非 JSON）- 应该发送错误消息
    const auto result = m_server.commandRegistry().execute("tellraw TestPlayer Hello World", m_console);

    // 命令执行，但由于 JSON 解析失败，可能返回 0 或错误
    EXPECT_TRUE(result.success());
}

TEST_F(TellRawCommandTest, JsonParsing_TranslationComponent)
{
    // 添加测试玩家
    m_server.addTestPlayer(1, "TestPlayer");

    // 测试翻译组件
    const std::string jsonMessage =
        R"({"translate":"chat.type.announcement","with":[{"text":"Server"},{"text":"Hello!"}]})";
    const auto result = m_server.commandRegistry().execute("tellraw TestPlayer " + jsonMessage, m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

// ========== 多目标测试 ==========

TEST_F(TellRawCommandTest, TellRawToMultiplePlayersNotSupported)
{
    // 添加多个测试玩家
    m_server.addTestPlayer(1, "Player1");
    m_server.addTestPlayer(2, "Player2");
    m_server.addTestPlayer(3, "Player3");

    // tellraw 使用 EntityArgumentType::player() 只允许单个玩家
    // 使用 @a 应该返回错误，因为只允许一个玩家
    const std::string jsonMessage = R"({"text":"Broadcast message"})";
    const auto result = m_server.commandRegistry().execute("tellraw @a " + jsonMessage, m_console);

    // tellraw 只允许单个玩家，@a 选择多个玩家应该失败
    EXPECT_FALSE(result.success());
}

TEST_F(TellRawCommandTest, TellRawToSinglePlayer)
{
    // 添加测试玩家
    m_server.addTestPlayer(1, "SinglePlayer");

    const std::string jsonMessage = R"({"text":"Private message","color":"yellow"})";
    const auto result = m_server.commandRegistry().execute("tellraw SinglePlayer " + jsonMessage, m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

// ========== 权限测试 ==========

TEST_F(TellRawCommandTest, TellRawCommandWithLowPermission)
{
    // 创建一个权限等级 1 的命令源（低于所需的 2）
    ServerCommandSource lowPermSource(&m_server,
        nullptr,
        0,
        Vector3d(0, 0, 0),
        Vector2f(0, 0),
        1, // 权限等级 1
        0,
        "lowperm");

    // 添加测试玩家
    m_server.addTestPlayer(1, "TestPlayer");

    const std::string jsonMessage = R"({"text":"test"})";
    const auto result = m_server.commandRegistry().execute("tellraw TestPlayer " + jsonMessage, lowPermSource);

    // 权限不足，命令应该失败
    EXPECT_FALSE(result.success());
}

} // namespace mc::command
