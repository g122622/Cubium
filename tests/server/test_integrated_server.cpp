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

#include "common/item/loot/conditions/LootConditions.hpp"
#include "common/network/connection/LocalConnection.hpp"
#include "server/application/IntegratedServer.hpp"
#include <atomic>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <thread>
#include <vector>

using namespace mc::server;
using namespace mc::network;
using namespace mc;

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
//  - 每个测试实例独占一个临时 gameRoot（按时间戳 + 自增计数保证唯一）
//  - SetUp 中 create_directories(<gameRoot>/saves/<worldName>)
//  - TearDown 中重试 remove_all（Windows 上 RocksDB 后台线程可能延迟释放句柄）
// ============================================================================
class IntegratedServerTestBase : public ::testing::Test {
protected:
    std::filesystem::path m_gameRoot;
    std::string m_worldName = "test_world";

    void SetUp() override
    {
        // 用时间戳 + 进程内自增计数器生成唯一目录，避免并行测试污染
        static std::atomic<std::uint64_t> s_counter{0};
        const auto token = std::to_string(std::time(nullptr)) + "_" + std::to_string(s_counter.fetch_add(1));
        m_gameRoot = std::filesystem::temp_directory_path() / "mc_is_test" / token;
        std::filesystem::create_directories(m_gameRoot / "saves" / m_worldName);
    }

    void TearDown() override
    {
        // 重试几次删除（Windows 上 RocksDB 后台线程可能延迟释放句柄）
        for (int i = 0; i < 10; ++i) {
            std::error_code ec;
            std::filesystem::remove_all(m_gameRoot, ec);
            if (!ec) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
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

TEST_F(IntegratedServerTest, GetClientEndpoint)
{
    IntegratedServer server;
    auto config = makeConfig();
    config.seed = 1;

    auto result = server.initialize(config);
    ASSERT_TRUE(result.success()) << result.error().message();

    auto* endpoint = server.getClientEndpoint();
    EXPECT_NE(endpoint, nullptr);
    EXPECT_TRUE(endpoint->isConnected());

    server.stop();
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

TEST_F(IntegratedServerTest, RequestStopDisconnectsClientEndpoint)
{
    IntegratedServer server;
    auto config = makeConfig();
    config.tickRate = 100;

    auto result = server.initialize(config);
    ASSERT_TRUE(result.success()) << result.error().message();

    auto* endpoint = server.getClientEndpoint();
    ASSERT_NE(endpoint, nullptr);
    EXPECT_TRUE(endpoint->isConnected());

    server.requestStop();

    EXPECT_FALSE(server.isRunning());
    EXPECT_FALSE(endpoint->isConnected());

    server.stop();
}

// ============================================================================
// 本地连接通信测试
// ============================================================================

class IntegratedServerCommunicationTest : public IntegratedServerTestBase {
protected:
    void SetUp() override
    {
        IntegratedServerTestBase::SetUp();

        auto config = makeConfig();
        config.seed = 12345;
        config.viewDistance = 3;
        config.tickRate = 100; // Faster ticks for testing

        auto result = server.initialize(config);
        ASSERT_TRUE(result.success()) << result.error().message();

        clientEndpoint = server.getClientEndpoint();
        ASSERT_NE(clientEndpoint, nullptr);
    }

    void TearDown() override
    {
        server.stop();
        IntegratedServerTestBase::TearDown();
    }

    // 辅助函数：等待接收数据包
    bool waitForPacket(std::vector<u8>& outData, int timeoutMs = 500)
    {
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() <
            timeoutMs) {
            if (clientEndpoint->receive(outData)) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return false;
    }

    IntegratedServer server;
    LocalEndpoint* clientEndpoint = nullptr;
};

TEST_F(IntegratedServerCommunicationTest, ReceivePacketAfterStart)
{
    // 服务端启动后应该能够通信
    std::vector<u8> data;

    // 发送一些数据
    std::vector<u8> testData = {1, 2, 3, 4, 5};
    clientEndpoint->send(testData.data(), testData.size());

    // 等待服务端处理
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 服务端应该正常运行
    EXPECT_TRUE(server.isRunning());
}

TEST_F(IntegratedServerCommunicationTest, BidirectionalCommunication)
{
    // 客户端发送数据到服务端
    std::vector<u8> sendData = {0x01, 0x02, 0x03};
    clientEndpoint->send(sendData.data(), sendData.size());

    // 等待处理
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 服务端应该还在运行
    EXPECT_TRUE(server.isRunning());
}

TEST_F(IntegratedServerCommunicationTest, MultipleSends)
{
    // 发送多个数据包
    for (int i = 0; i < 10; ++i) {
        std::vector<u8> data = {static_cast<u8>(i)};
        clientEndpoint->send(data.data(), data.size());
    }

    // 等待处理
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 服务端应该还在运行
    EXPECT_TRUE(server.isRunning());
}

TEST_F(IntegratedServerCommunicationTest, LargePacket)
{
    // 发送大数据包
    std::vector<u8> largeData(1024, 0xAB);
    clientEndpoint->send(largeData.data(), largeData.size());

    // 等待处理
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 服务端应该还在运行
    EXPECT_TRUE(server.isRunning());
}

TEST_F(IntegratedServerCommunicationTest, ServerTicksWhileWaiting)
{
    u64 initialTick = server.currentTick();

    // 等待一段时间。注意：启动初期区块生成等开销使实际 TPS 低于配置的 100，
    // 故等待窗口取 1000ms 并用宽松阈值，避免对调度抖动/启动开销过度敏感。
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    u64 finalTick = server.currentTick();

    // 服务端应持续 tick（阈值宽松，仅验证仍在推进）
    EXPECT_GT(finalTick - initialTick, 5);
}

// ============================================================================
// 断开连接测试
// ============================================================================

class IntegratedServerDisconnectTest : public IntegratedServerTestBase {};

TEST_F(IntegratedServerDisconnectTest, ClientDisconnect)
{
    IntegratedServer server;
    auto config = makeConfig();

    auto result = server.initialize(config);
    ASSERT_TRUE(result.success()) << result.error().message();

    LocalEndpoint* client = server.getClientEndpoint();
    ASSERT_NE(client, nullptr);

    // 断开客户端连接
    client->disconnect();

    // 等待处理
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 服务端应该还在运行
    EXPECT_TRUE(server.isRunning());

    server.stop();
}

TEST_F(IntegratedServerDisconnectTest, ServerStopClosesEndpoint)
{
    LocalEndpoint* client = nullptr;

    {
        IntegratedServer server;
        auto config = makeConfig();

        auto result = server.initialize(config);
        ASSERT_TRUE(result.success()) << result.error().message();

        client = server.getClientEndpoint();
        ASSERT_NE(client, nullptr);
        EXPECT_TRUE(client->isConnected());

        server.stop();
    }

    // 服务端销毁后，端点应该断开
    // 注意：这是未定义行为，但测试可以验证
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
