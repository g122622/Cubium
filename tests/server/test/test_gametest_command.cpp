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
// 架构说明：GameTestCommand::registerTo 未在 CommandRegistry::registerDefaults 中登记
// （registerDefaults 经 IntegratedServer 被 client 共享编译，不能拉 server/test/ 依赖，
// 遵循 ServerScriptManager 同款分层约束）。生产路径由 GameTestServer::initialize 手动调
// GameTestCommand::registerTo(commandRegistry().dispatcher()) 注入。本测试镜像该模式：
// 用 IntegratedServer + 手动注册 + 手动 pump GameTestTicker（IntegratedServer::tick 不含
// GameTestTicker 推进，对齐 GameTestServer::tickOnce 的三段顺序）。
//
// 覆盖：
//   - /gametest clear 在空 ticker 上不崩
//   - /gametest runall（无匹配测试）不崩
//   - 命令解析失败（未知子命令）返回错误
//
// 注：完整 /gametest runall 跑通内置样例需 worldgen 数据 + 程序化模板，已在
// test_gametest_server.cpp 覆盖；此处聚焦命令层（注册/解析/调度入口不崩）。

#include <gtest/gtest.h>

#include "common/TempDirHelper.hpp"
#include "common/test/framework/ticker/GameTestTicker.hpp"
#include "server/application/IntegratedServer.hpp"
#include "server/application/MinecraftServer.hpp" // tick() 经基类公有调用（IntegratedServer::tick protected）
#include "server/command/CommandRegistry.hpp"     // commandRegistry() 返回完整类型
#include "server/command/ServerCommandSource.hpp"
#include "server/test/facade/GameTestCommand.hpp"

#include <chrono>
#include <filesystem>
#include <thread>

using namespace mc::server;
using mc::command::ServerCommandSource; // ServerCommandSource 在 mc::command 非 mc::server

namespace {
// IntegratedServer + GameTestCommand 注入夹具。
class GameTestCommandFixture : public ::testing::Test {
protected:
    std::filesystem::path m_gameRoot;
    std::string m_worldName = "gt_cmd_world";
    std::unique_ptr<IntegratedServer> m_server;
    ServerCommandSource m_console{nullptr};

    void SetUp() override
    {
        m_gameRoot = mc::test::makeUniqueTestDir("mc_gt_cmd");
        std::filesystem::create_directories(m_gameRoot / "saves" / m_worldName);
        mc::test::GameTestTicker::instance().forceStop();

        m_server = std::make_unique<IntegratedServer>();
        IntegratedServerParams config{
            .worldName = m_worldName,
            .gameDirectoryRoot = m_gameRoot.string(),
            .displayName = m_worldName,
            .seed = 0,
            .defaultGameMode = mc::GameMode::Creative,
            .viewDistance = 4,
            .simulationDistance = 4,
            .tickRate = 20,
            .worldType = mc::WorldType::Default,
            .difficulty = mc::Difficulty::Normal,
            .hardcore = false,
            .allowCommands = true,
            .isNewWorld = false,
        };
        auto result = m_server->initialize(config);
        if (!result.success()) {
            GTEST_SKIP() << "IntegratedServer initialize failed (likely missing worldgen data): "
                         << result.error().message();
        }
        // 镜像 GameTestServer::initialize:181 的手动注册（registerDefaults 不含 GameTestCommand）。
        mc::test::GameTestCommand::registerTo(m_server->commandRegistry().dispatcher());
        m_console = ServerCommandSource::forConsole(m_server.get());
    }

    void TearDown() override
    {
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
// /gametest runall：无匹配测试不崩（ticker 保持空）
// ============================================================================

TEST_F(GameTestCommandFixture, RunAllWithNoMatchingTestsDoesNotCrash)
{
    auto result = m_server->commandRegistry().execute("gametest runall", m_console);
    EXPECT_TRUE(result.success()) << result.error().message();
    // 手动 pump 几 tick（IntegratedServer::tick 不含 GameTestTicker 推进）。
    // tick() 在 IntegratedServer 中 protected，经基类 MinecraftServer 公有调用。
    auto& serverBase = static_cast<mc::server::MinecraftServer&>(*m_server);
    for (int i = 0; i < 5; ++i) {
        serverBase.tick();
        mc::test::GameTestTicker::instance().tick();
    }
    // 无匹配测试 → ticker 应仍为空（或快速完成清理）。
    // 不强制 isEmpty（runall 可能加入 0 实例后立即空），仅验证不崩。
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
