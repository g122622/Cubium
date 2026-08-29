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
 * @file WardenSpawnTrackerCommandTest.cpp
 * @brief WardenSpawnTrackerCommand 单元测试
 *
 * 测试 /warden_spawn_tracker 命令的注册、权限检查和子命令解析。
 * 玩家执行的集成测试通过直接调用 WardenWarningEffect API 验证。
 */

#include <gtest/gtest.h>

#include "common/BaseTestServer.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/world/dimension/DimensionManager.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/command/commands/WardenSpawnTrackerCommand.hpp"
#include "server/dimension/ServerDimensionManager.hpp"

namespace mc {
namespace command {

class WardenSpawnTrackerTestServer final : public mc::test::BaseTestServer {
public:
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

class WardenSpawnTrackerCommandTest : public ::testing::Test {
protected:
    void SetUp() override { WardenSpawnTrackerCommand::registerTo(m_server.commandRegistry().dispatcher()); }

    WardenSpawnTrackerTestServer m_server;
    ServerCommandSource m_console = ServerCommandSource::forConsole(&m_server);
};

// ==================== 命令注册测试 ====================

TEST_F(WardenSpawnTrackerCommandTest, CommandIsRegistered)
{
    const auto& registry = m_server.commandRegistry();
    const auto snapshot = registry.getCommandTreeSnapshot();

    bool found = false;
    for (const auto& node : snapshot.nodes) {
        if (node.name == "warden_spawn_tracker") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "warden_spawn_tracker 命令应该被注册";
}

// ==================== 权限测试 ====================

TEST_F(WardenSpawnTrackerCommandTest, ClearRequiresPermissionLevel2)
{
    // 权限等级 0（普通玩家）不应有权限执行
    ServerCommandSource noPermSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 0, "test");
    auto result = m_server.commandRegistry().execute("warden_spawn_tracker clear", noPermSource);
    EXPECT_FALSE(result.success());
}

TEST_F(WardenSpawnTrackerCommandTest, SetRequiresPermissionLevel2)
{
    ServerCommandSource noPermSource(&m_server, nullptr, 0, Vector3d(0, 0, 0), Vector2f(0, 0), 0, 0, "test");
    auto result = m_server.commandRegistry().execute("warden_spawn_tracker set 3", noPermSource);
    EXPECT_FALSE(result.success());
}

// ==================== 语法解析测试 ====================

TEST_F(WardenSpawnTrackerCommandTest, ClearSubcommandParsesSuccessfully)
{
    // 控制台（权限等级4）可以解析命令，但因为不是玩家会执行失败（返回0）
    auto result = m_server.commandRegistry().execute("warden_spawn_tracker clear", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(WardenSpawnTrackerCommandTest, SetSubcommandParsesSuccessfully)
{
    auto result = m_server.commandRegistry().execute("warden_spawn_tracker set 3", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(WardenSpawnTrackerCommandTest, SetWithLevel0_ParsesSuccessfully)
{
    auto result = m_server.commandRegistry().execute("warden_spawn_tracker set 0", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(WardenSpawnTrackerCommandTest, SetWithLevel4_ParsesSuccessfully)
{
    auto result = m_server.commandRegistry().execute("warden_spawn_tracker set 4", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(WardenSpawnTrackerCommandTest, SetWithLevel5_ParseFails)
{
    // IntegerArgumentType::integer(0, 4) 不接受 5
    auto result = m_server.commandRegistry().execute("warden_spawn_tracker set 5", m_console);
    EXPECT_FALSE(result.success());
}

TEST_F(WardenSpawnTrackerCommandTest, SetWithNegativeLevel_ParseFails)
{
    // IntegerArgumentType::integer(0, 4) 不接受负数
    auto result = m_server.commandRegistry().execute("warden_spawn_tracker set -1", m_console);
    EXPECT_FALSE(result.success());
}

TEST_F(WardenSpawnTrackerCommandTest, SetWithoutLevel_ParseFails)
{
    // set 子命令需要参数
    auto result = m_server.commandRegistry().execute("warden_spawn_tracker set", m_console);
    EXPECT_FALSE(result.success());
}

TEST_F(WardenSpawnTrackerCommandTest, UnknownSubcommand_ParseFails)
{
    auto result = m_server.commandRegistry().execute("warden_spawn_tracker unknown", m_console);
    EXPECT_FALSE(result.success());
}

// ==================== 非玩家执行测试 ====================

TEST_F(WardenSpawnTrackerCommandTest, ClearFromConsole_ReturnsZero)
{
    // 控制台不是玩家，clear 子命令应返回0
    auto result = m_server.commandRegistry().execute("warden_spawn_tracker clear", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

TEST_F(WardenSpawnTrackerCommandTest, SetFromConsole_ReturnsZero)
{
    // 控制台不是玩家，set 子命令应返回0
    auto result = m_server.commandRegistry().execute("warden_spawn_tracker set 2", m_console);
    EXPECT_TRUE(result.success());
    EXPECT_EQ(result.value(), 0);
}

// ==================== WardenWarningEffect 与命令集成验证 ====================

// 以下测试直接调用 WardenWarningEffect API 来验证命令的行为逻辑，
// 而不依赖完整的 ServerPlayer 创建流程。

TEST_F(WardenSpawnTrackerCommandTest, ResetClearsWarningLevel)
{
    // 模拟 /warden_spawn_tracker clear 的核心逻辑
    entity::WardenWarningEffect effect;
    effect.setWarningLevel(3);
    EXPECT_EQ(effect.getWarningLevel(), 3);

    effect.reset();
    EXPECT_EQ(effect.getWarningLevel(), 0);
}

TEST_F(WardenSpawnTrackerCommandTest, SetWarningLevelWorks)
{
    // 模拟 /warden_spawn_tracker set <level> 的核心逻辑
    entity::WardenWarningEffect effect;
    EXPECT_EQ(effect.getWarningLevel(), 0);

    effect.setWarningLevel(2);
    EXPECT_EQ(effect.getWarningLevel(), 2);

    effect.setWarningLevel(4);
    EXPECT_EQ(effect.getWarningLevel(), 4);

    effect.setWarningLevel(0);
    EXPECT_EQ(effect.getWarningLevel(), 0);
}

TEST_F(WardenSpawnTrackerCommandTest, SetAndClearRoundTrip)
{
    entity::WardenWarningEffect effect;

    effect.setWarningLevel(3);
    EXPECT_EQ(effect.getWarningLevel(), 3);

    effect.reset();
    EXPECT_EQ(effect.getWarningLevel(), 0);
}

TEST_F(WardenSpawnTrackerCommandTest, ClearAlsoResetsCooldown)
{
    entity::WardenWarningEffect effect;

    // increaseWarning 会设置冷却
    effect.increaseWarning();
    EXPECT_TRUE(effect.onCooldown());
    EXPECT_EQ(effect.getWarningLevel(), 1);

    // reset 应该清除冷却
    effect.reset();
    EXPECT_FALSE(effect.onCooldown());
    EXPECT_EQ(effect.getWarningLevel(), 0);
}

} // namespace command
} // namespace mc
