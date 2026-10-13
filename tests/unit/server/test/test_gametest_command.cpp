/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permitted persons to whom the Software is
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

// /gametest 命令集成测试。
//
// 使用同步 GameTestServer 门面初始化世界与命令注册表，由测试线程串行推进世界和 ticker。
// IntegratedServer 会自行启动 tick 线程，不能同时由测试线程手动调用 tick()。
//
// 覆盖：
//   - /gametest clear 在空 ticker 上不崩
//   - /gametest runall 调度入口和同步 tick 不崩
//   - 命令解析失败（未知子命令）返回错误
//
// 注：完整 /gametest runall 跑通内置样例需 worldgen 数据 + 程序化模板，已在
// test_gametest_server.cpp 覆盖；此处聚焦命令层（注册/解析/调度入口不崩）。

#include <gtest/gtest.h>

#include "common/TempDirHelper.hpp"
#include "server/application/MinecraftServer.hpp"
#include "server/command/CommandRegistry.hpp" // commandRegistry() 返回完整类型
#include "server/command/ServerCommandSource.hpp"
#include "server/test/facade/GameTestCommand.hpp"
#include "server/test/facade/GameTestServer.hpp"
#include "server/test/framework/ticker/GameTestTicker.hpp"

#include <filesystem>

using namespace mc::server;
using mc::command::ServerCommandSource; // ServerCommandSource 在 mc::command 非 mc::server

namespace {
// 同步 GameTestServer 命令夹具。
class GameTestCommandFixture : public ::testing::Test {
protected:
    std::filesystem::path m_gameRoot;
    std::string m_worldName = "gt_cmd_world";
    std::unique_ptr<mc::test::GameTestServer> m_server;
    ServerCommandSource m_console{nullptr};

    void SetUp() override
    {
        m_gameRoot = mc::test::makeUniqueTestDir("mc_gt_cmd");
        std::filesystem::create_directories(m_gameRoot / "saves" / m_worldName);
        mc::test::GameTestTicker::instance().forceStop();

        m_server = std::make_unique<mc::test::GameTestServer>();
        mc::test::GameTestServerParams config;
        config.worldName = m_worldName;
        config.gameDirectoryRoot = m_gameRoot.string();
        config.seed = 0;
        config.defaultGameMode = mc::GameMode::Creative;
        config.viewDistance = 4;
        config.simulationDistance = 4;
        config.tickRate = 20;
        config.worldType = mc::WorldType::Flat;
        config.difficulty = mc::Difficulty::Normal;
        config.hardcore = false;
        config.allowCommands = true;
        config.isNewWorld = false;
        config.testsFilter = "__NoSuchGameTestCommandSetup__";
        config.maxTicks = 200;
        auto result = m_server->initialize(config);
        if (!result.success()) {
            GTEST_SKIP() << "GameTestServer initialize failed (likely missing worldgen data): "
                         << result.error().message();
        }
        m_console = ServerCommandSource::forConsole(m_server.get());
    }

    void TearDown() override
    {
        mc::test::GameTestTicker::instance().forceStop();
        mc::test::GameTestCommand::forceClearAllInstances();
        if (m_server) {
            m_server->stop();
            m_server.reset();
        }
        mc::test::GameTestTicker::instance().forceStop();
        mc::test::removeTestDir(m_gameRoot);
    }
};
} // namespace

// ============================================================================
// /gametest clear：空 ticker 不崩
// ============================================================================

TEST_F(GameTestCommandFixture, ClearOnEmptyTickerDoesNotCrash)
{
    auto result = m_server->commandRegistry().execute("gametest clear", m_console);
    // 命令应成功执行（返回 0 或正数）；空 ticker 清理是 no-op。
    EXPECT_TRUE(result.success()) << result.error().message();
}

// ============================================================================
// /gametest runall：注册的内置测试可被调度，缺少结构资源也不能导致进程中止
// ============================================================================

TEST_F(GameTestCommandFixture, RunAllDoesNotCrash)
{
    auto result = m_server->commandRegistry().execute("gametest runall", m_console);
    EXPECT_TRUE(result.success()) << result.error().message();
    // 世界与测试状态机由当前线程串行推进，服务端没有后台 tick 线程。
    auto& serverBase = static_cast<mc::server::MinecraftServer&>(*m_server);
    for (int i = 0; i < 5; ++i) {
        serverBase.tick();
        mc::test::GameTestTicker::instance().tick();
        mc::test::GameTestCommand::cleanupCompletedInstances();
    }
    // 此处验证命令调度和状态机推进不崩；完整测试成功与退出码由运行门面测试覆盖。
    SUCCEED();
}

// ============================================================================
// 未知子命令：应解析失败
// ============================================================================

TEST_F(GameTestCommandFixture, UnknownSubcommandFails)
{
    auto result = m_server->commandRegistry().execute("gametest bogus_subcommand_xyz", m_console);
    // 未知子命令应返回失败。
    EXPECT_FALSE(result.success());
}

// ============================================================================
// /gametest locate（TODO stub）：调用不崩
// ============================================================================

TEST_F(GameTestCommandFixture, LocateStubDoesNotCrash)
{
    // locate/pos 为 TODO stub（占位返回），验证调用不崩。
    // TODO: 待实现后改为行为断言。
    auto result = m_server->commandRegistry().execute("gametest locate alwaysSucceed", m_console);
    // stub 可能成功（返回占位）或失败，仅验证不崩。
    SUCCEED();
}
