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
 * IMPLIED, INCLUDING ANY OF FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * @file StopSoundCommandTest.cpp
 * @brief StopSoundCommand 单元测试
 *
 * 测试 /stopsound 命令的注册、解析、权限检查和各语法变体。
 * 声音包文的完整端到端测试应在集成测试环境中进行。
 *
 * 注意：使用 @p/@a 选择器时需要位置上下文，在纯控制台测试环境中
 * 可能导致 PlayerResolver 的位置排序逻辑访问无效坐标而崩溃，
 * 因此本测试使用玩家名而非选择器来指定目标玩家。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/StopSoundCommand.hpp"
#include "server/dimension/ServerDimensionManager.hpp"

namespace mc::command {

// 测试服务器所需的服务端类型位于 mc:: 顶层命名空间，此处引入以便在
// mc::command 命名空间内直接引用。
using mc::DimensionManager;
using mc::ServerDimensionManager;

class StopSoundTestServer final : public mc::test::BaseTestServer {
public:
    StopSoundTestServer() = default;

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

class StopSoundCommandTest : public ::testing::Test {
protected:
    void SetUp() override { StopSoundCommand::registerTo(m_server.commandRegistry().dispatcher()); }

    StopSoundTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

// ========== 命令注册测试 ==========

TEST_F(StopSoundCommandTest, StopSoundCommandIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "stopsound") {
            found = true;
            break;
        }
    }

    EXPECT_TRUE(found) << "stopsound command should be registered";
}

// ========== 权限测试 ==========

TEST_F(StopSoundCommandTest, StopSoundCommandRequiresPermissionLevel2)
{
    ServerCommandSource lowPermSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 0, "test");

    bool permissionDenied = false;
    try {
        const auto result = m_server.commandRegistry().execute("stopsound Steve", lowPermSource);
        permissionDenied = (result.value() == 0);
    }
    catch (...) {
        permissionDenied = true;
    }

    EXPECT_TRUE(permissionDenied);
}

// ========== /stopsound <player> - 停止所有声音 ==========

TEST_F(StopSoundCommandTest, StopAllSoundsNoMatchingPlayer)
{
    const auto result = m_server.commandRegistry().execute("stopsound Steve", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(StopSoundCommandTest, StopAllSoundsWithPlayer)
{
    m_server.addTestPlayer(1, "Steve");

    const auto result = m_server.commandRegistry().execute("stopsound Steve", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(StopSoundCommandTest, StopAllSoundsWithMultiplePlayers)
{
    m_server.addTestPlayer(1, "Steve");
    m_server.addTestPlayer(2, "Alex");
    m_server.addTestPlayer(3, "Notch");

    // 使用多个单独命令而非 @a，避免选择器位置排序问题
    auto result1 = m_server.commandRegistry().execute("stopsound Steve", m_console);
    EXPECT_TRUE(result1.success());
    EXPECT_EQ(result1.value(), 1);

    auto result2 = m_server.commandRegistry().execute("stopsound Alex", m_console);
    EXPECT_TRUE(result2.success());
    EXPECT_EQ(result2.value(), 1);
}

// ========== /stopsound <player> * - 通配符测试 ==========

TEST_F(StopSoundCommandTest, StopAllSoundsWithWildcardNoPlayer)
{
    const auto result = m_server.commandRegistry().execute("stopsound Steve *", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(StopSoundCommandTest, StopAllSoundsWithWildcardWithPlayer)
{
    m_server.addTestPlayer(1, "Steve");

    const auto result = m_server.commandRegistry().execute("stopsound Steve *", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

// ========== /stopsound <player> * <sound> - 通配符+声音ID ==========

TEST_F(StopSoundCommandTest, StopSpecificSoundWithWildcardWithPlayer)
{
    m_server.addTestPlayer(1, "Steve");

    const auto result = m_server.commandRegistry().execute("stopsound Steve * minecraft:music.game", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(StopSoundCommandTest, StopSpecificSoundWithWildcardNoPlayer)
{
    const auto result = m_server.commandRegistry().execute("stopsound Steve * minecraft:music.game", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// ========== /stopsound <player> <source> - 各声源类别 ==========

TEST_F(StopSoundCommandTest, StopSoundByCategoryMaster)
{
    m_server.addTestPlayer(1, "Steve");

    const auto result = m_server.commandRegistry().execute("stopsound Steve master", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(StopSoundCommandTest, StopSoundByCategoryMusic)
{
    m_server.addTestPlayer(1, "Steve");

    const auto result = m_server.commandRegistry().execute("stopsound Steve music", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(StopSoundCommandTest, StopSoundByCategoryRecord)
{
    m_server.addTestPlayer(1, "Steve");

    const auto result = m_server.commandRegistry().execute("stopsound Steve record", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(StopSoundCommandTest, StopSoundByCategoryWeather)
{
    m_server.addTestPlayer(1, "Steve");

    const auto result = m_server.commandRegistry().execute("stopsound Steve weather", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(StopSoundCommandTest, StopSoundByCategoryBlock)
{
    m_server.addTestPlayer(1, "Steve");

    const auto result = m_server.commandRegistry().execute("stopsound Steve block", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(StopSoundCommandTest, StopSoundByCategoryHostile)
{
    m_server.addTestPlayer(1, "Steve");

    const auto result = m_server.commandRegistry().execute("stopsound Steve hostile", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(StopSoundCommandTest, StopSoundByCategoryNeutral)
{
    m_server.addTestPlayer(1, "Steve");

    const auto result = m_server.commandRegistry().execute("stopsound Steve neutral", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(StopSoundCommandTest, StopSoundByCategoryPlayer)
{
    m_server.addTestPlayer(1, "Steve");

    const auto result = m_server.commandRegistry().execute("stopsound Steve player", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(StopSoundCommandTest, StopSoundByCategoryAmbient)
{
    m_server.addTestPlayer(1, "Steve");

    const auto result = m_server.commandRegistry().execute("stopsound Steve ambient", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(StopSoundCommandTest, StopSoundByCategoryVoice)
{
    m_server.addTestPlayer(1, "Steve");

    const auto result = m_server.commandRegistry().execute("stopsound Steve voice", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(StopSoundCommandTest, StopSoundByCategoryUI)
{
    m_server.addTestPlayer(1, "Steve");

    const auto result = m_server.commandRegistry().execute("stopsound Steve ui", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(StopSoundCommandTest, StopSoundByCategoryNoPlayer)
{
    const auto result = m_server.commandRegistry().execute("stopsound Steve master", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// ========== /stopsound <player> <source> <sound> - 指定类别+声音ID ==========

TEST_F(StopSoundCommandTest, StopSpecificSoundByCategoryWithPlayer)
{
    m_server.addTestPlayer(1, "Steve");

    const auto result = m_server.commandRegistry().execute("stopsound Steve master minecraft:music.game", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(StopSoundCommandTest, StopSpecificSoundByCategoryNoPlayer)
{
    const auto result = m_server.commandRegistry().execute("stopsound Steve music minecraft:ambient.cave", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(StopSoundCommandTest, StopSpecificSoundWithoutNamespace)
{
    m_server.addTestPlayer(1, "Steve");

    const auto result = m_server.commandRegistry().execute("stopsound Steve master music.game", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

TEST_F(StopSoundCommandTest, StopSpecificSoundByCategoryWithAnotherPlayer)
{
    m_server.addTestPlayer(2, "Alex");

    const auto result = m_server.commandRegistry().execute("stopsound Alex weather minecraft:weather.rain", m_console);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

// ========== 权限等级2的命令源可以执行 ==========

TEST_F(StopSoundCommandTest, StopSoundCommandWithPermissionLevel2)
{
    ServerCommandSource opSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 2, 0, "op");
    m_server.addTestPlayer(1, "Steve");

    const auto result = m_server.commandRegistry().execute("stopsound Steve", opSource);

    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 1);
}

} // namespace mc::command
