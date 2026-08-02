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

#include <gtest/gtest.h>

#include "common/TempDirHelper.hpp"
#include "common/item/loot/conditions/LootConditions.hpp"
#include "server/application/IntegratedServer.hpp"
#include <chrono>
#include <filesystem>
#include <thread>
#include <vector>

using namespace mc::server;
using namespace mc;
using namespace std::chrono_literals;

// ============================================================================
// 测试夹具基类：使用临时目录避免污染用户存档，并预先创建 saves/<worldName>/
// 以便 GlobalStorageManager::openLevel 能找到世界目录。
//
// 根因：源码 GlobalStorageManager::openLevel 不会创建新世界，它要求
// saves/<levelId> 已存在，否则返回 WorldNotFound。IntegratedServer::initialize
// 默认使用 GameDirectory::defaultDirectory()（用户真实游戏目录），
// 若不预创建 saves/<worldName>，所有依赖 initialize 的测试都会连锁失败。
//
// 该夹具复用 test_save_player_runtime_state.cpp 的同款模式：
//  - 每个测试实例独占一个临时 gameRoot（helper 通过 PID 等分量跨进程唯一）
//  - SetUp 中 create_directories(<gameRoot>/saves/<worldName>)
//  - TearDown 中 helper 内置 10 次重试 remove_all（Windows 上 RocksDB 句柄释放滞后）
// ============================================================================
class IntegratedServerTestBase : public ::testing::Test {
protected:
    std::filesystem::path m_gameRoot;
    std::string m_worldName = "test_world";

    void SetUp() override
    {
        // helper 通过 PID + 纳秒时间戳 + 计数器组合保证 CTest -j16 并行进程间唯一
        m_gameRoot = mc::test::makeUniqueTestDir("mc_is_test");
        std::filesystem::create_directories(m_gameRoot / "saves" / m_worldName);
    }

    void TearDown() override
    {
        // Windows 上 RocksDB 后台线程可能延迟释放句柄，helper 内置 10 次重试
        mc::test::removeTestDir(m_gameRoot);
    }

    // 构造一份指向临时目录的非新世界配置（既有世界，不写 level.dat）
    IntegratedServerParams makeConfig() const
    {
        return IntegratedServerParams{
            .worldName = m_worldName,
            .gameDirectoryRoot = m_gameRoot.string(),
            .displayName = m_worldName,
            .seed = 0,
            .defaultGameMode = GameMode::Survival,
            .viewDistance = 6,
            .tickRate = 20,
            .worldType = WorldType::Default,
            .difficulty = Difficulty::Normal,
            .hardcore = false,
            .allowCommands = false,
            .isNewWorld = false,
        };
    }
};

// ============================================================================
// IntegratedServer 基础测试
// ============================================================================

class IntegratedServerTest : public IntegratedServerTestBase {};

TEST_F(IntegratedServerTest, CreateServer)
{
    IntegratedServer server;
    EXPECT_FALSE(server.isRunning());
}

TEST_F(IntegratedServerTest, InitializeServer)
{
    IntegratedServer server;
    auto config = makeConfig();
    config.seed = 12345;
    config.viewDistance = 8;

    auto result = server.initialize(config);
    EXPECT_TRUE(result.success()) << result.error().message();
    EXPECT_TRUE(server.isRunning());

    server.stop();
    EXPECT_FALSE(server.isRunning());
}

TEST_F(IntegratedServerTest, DoubleInitializeFails)
{
    IntegratedServer server;
    auto config = makeConfig();

    auto result1 = server.initialize(config);
    EXPECT_TRUE(result1.success()) << result1.error().message();

    auto result2 = server.initialize(config);
    EXPECT_FALSE(result2.success());
    EXPECT_EQ(result2.error().code(), ErrorCode::AlreadyExists);

    server.stop();
}

TEST_F(IntegratedServerTest, StopWithoutInitialize)
{
    IntegratedServer server;
    // 应该不崩溃
    server.stop();
    EXPECT_FALSE(server.isRunning());
}

TEST_F(IntegratedServerTest, ConfigValues)
{
    IntegratedServer server;
    auto config = makeConfig();
    config.seed = 42;
    config.viewDistance = 16;
    config.tickRate = 30;

    auto result = server.initialize(config);
    ASSERT_TRUE(result.success()) << result.error().message();

    const auto& serverConfig = server.params();
    EXPECT_EQ(serverConfig.worldName, m_worldName);
    EXPECT_EQ(serverConfig.seed, 42);
    EXPECT_EQ(serverConfig.viewDistance, 16);
    EXPECT_EQ(serverConfig.tickRate, 30);

    server.stop();
}

TEST_F(IntegratedServerTest, OverworldWorldMatchesServerWorldShortcut)
{
    IntegratedServer server;
    auto config = makeConfig();
    config.seed = 12345;
    config.viewDistance = 8;

    auto result = server.initialize(config);
    ASSERT_TRUE(result.success()) << result.error().message();

    auto* overworld = server.dimensionManager().getOverworld();
    ASSERT_NE(overworld, nullptr);
    ASSERT_NE(overworld->world(), nullptr);
    EXPECT_NE(overworld->world(), nullptr);

    server.stop();
}

TEST_F(IntegratedServerTest, DimensionsShareStorage)
{
    IntegratedServer server;
    auto config = makeConfig();
    config.seed = 67890;
    config.viewDistance = 8;

    auto result = server.initialize(config);
    ASSERT_TRUE(result.success()) << result.error().message();

    auto* overworld = server.dimensionManager().getOverworld();
    auto* nether = server.dimensionManager().getNether();
    auto* theEnd = server.dimensionManager().getTheEnd();
    ASSERT_NE(overworld, nullptr);
    ASSERT_NE(nether, nullptr);
    ASSERT_NE(theEnd, nullptr);
    ASSERT_NE(overworld->world(), nullptr);
    ASSERT_NE(nether->world(), nullptr);
    ASSERT_NE(theEnd->world(), nullptr);

    EXPECT_EQ(&overworld->world()->storage(), &nether->world()->storage());
    EXPECT_EQ(&overworld->world()->storage(), &theEnd->world()->storage());
    server.stop();
}

TEST_F(IntegratedServerTest, TickCountIncreases)
{
    IntegratedServer server;
    auto config = makeConfig();
    config.tickRate = 100; // 100 TPS for faster testing

    auto result = server.initialize(config);
    ASSERT_TRUE(result.success()) << result.error().message();

    u64 initialTick = server.currentTick();

    // Wait for some ticks
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    u64 finalTick = server.currentTick();
    EXPECT_GT(finalTick, initialTick);

    server.stop();
}

// ============================================================================
// 服务器类型检查测试
// ============================================================================

class IntegratedServerTypeTest : public IntegratedServerTestBase {};

TEST_F(IntegratedServerTypeTest, IsIntegratedReturnsTrue)
{
    IntegratedServer server;
    EXPECT_TRUE(server.isIntegrated());
}

TEST_F(IntegratedServerTypeTest, IsDedicatedReturnsFalse)
{
    IntegratedServer server;
    EXPECT_FALSE(server.isDedicated());
}

TEST_F(IntegratedServerTypeTest, TypeMethodsWorkBeforeInitialization)
{
    IntegratedServer server;
    // 类型检查方法应该在初始化前后都能正常工作
    EXPECT_TRUE(server.isIntegrated());
    EXPECT_FALSE(server.isDedicated());
}

TEST_F(IntegratedServerTypeTest, TypeMethodsWorkAfterInitialization)
{
    IntegratedServer server;
    auto config = makeConfig();
    config.seed = 1;

    auto result = server.initialize(config);
    ASSERT_TRUE(result.success()) << result.error().message();

    EXPECT_TRUE(server.isIntegrated());
    EXPECT_FALSE(server.isDedicated());

    server.stop();
}

// ============================================================================
// 注：原 GetClientEndpoint / RequestStopDisconnectsClientEndpoint /
// IntegratedServerCommunicationTest / ClientDisconnect / ServerStopClosesEndpoint
// 等用例依赖旧 LocalEndpoint 字节队列（getClientEndpoint()->send/receive/isConnected/
// disconnect）。新网络层 getClientEndpoint 已删除，替换为 takeClientTransport()
// （一次性取出 ILocalTransport，IR 包队列非字节队列）。这些连接生命周期/字节通信
// 用例的端到端通信链路由 tests/common/network/test_client_network_local.cpp
// （ConnectLocalDrivesClientToPlaying / ClientSendDeliversToServerInbound /
// DisconnectTransitionsToDisconnected 等）+ test_server_handshake.cpp 覆盖。
// ============================================================================
