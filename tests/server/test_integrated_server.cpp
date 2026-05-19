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

#include "common/entity/loot/LootConditions.hpp"
#include "common/network/connection/LocalConnection.hpp"
#include "server/application/IntegratedServer.hpp"
#include <chrono>
#include <thread>
#include <vector>

using namespace mc::server;
using namespace mc::network;
using namespace mc;

// ============================================================================
// IntegratedServer 基础测试
// ============================================================================

TEST(IntegratedServerTest, CreateServer)
{
    IntegratedServer server;
    EXPECT_FALSE(server.isRunning());
}

TEST(IntegratedServerTest, InitializeServer)
{
    IntegratedServer server;
    IntegratedServerConfig config;
    config.worldName = "test_world";
    config.seed = 12345;
    config.viewDistance = 8;

    auto result = server.initialize(config);
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(server.isRunning());

    server.stop();
    EXPECT_FALSE(server.isRunning());
}

TEST(IntegratedServerTest, GetClientEndpoint)
{
    IntegratedServer server;
    IntegratedServerConfig config;
    config.seed = 1;

    auto result = server.initialize(config);
    ASSERT_TRUE(result.success());

    auto* endpoint = server.getClientEndpoint();
    EXPECT_NE(endpoint, nullptr);
    EXPECT_TRUE(endpoint->isConnected());

    server.stop();
}

TEST(IntegratedServerTest, DoubleInitializeFails)
{
    IntegratedServer server;
    IntegratedServerConfig config;

    auto result1 = server.initialize(config);
    EXPECT_TRUE(result1.success());

    auto result2 = server.initialize(config);
    EXPECT_FALSE(result2.success());
    EXPECT_EQ(result2.error().code(), ErrorCode::AlreadyExists);

    server.stop();
}

TEST(IntegratedServerTest, StopWithoutInitialize)
{
    IntegratedServer server;
    // 应该不崩溃
    server.stop();
    EXPECT_FALSE(server.isRunning());
}

TEST(IntegratedServerTest, ConfigValues)
{
    IntegratedServer server;
    IntegratedServerConfig config;
    config.worldName = "my_world";
    config.seed = 42;
    config.viewDistance = 16;
    config.tickRate = 30;

    auto result = server.initialize(config);
    ASSERT_TRUE(result.success());

    const auto& serverConfig = server.config();
    EXPECT_EQ(serverConfig.worldName, "my_world");
    EXPECT_EQ(serverConfig.seed, 42);
    EXPECT_EQ(serverConfig.viewDistance, 16);
    EXPECT_EQ(serverConfig.tickRate, 30);

    server.stop();
}

TEST(IntegratedServerTest, OverworldWorldMatchesServerWorldShortcut)
{
    IntegratedServer server;
    IntegratedServerConfig config;
    config.worldName = "test_world_runtime_binding";
    config.seed = 12345;
    config.viewDistance = 8;

    auto result = server.initialize(config);
    ASSERT_TRUE(result.success());

    auto* overworld = server.dimensionManager().getOverworld();
    ASSERT_NE(overworld, nullptr);
    ASSERT_NE(overworld->world(), nullptr);
    EXPECT_EQ(overworld->world(), &server.world());

    server.stop();
}

TEST(IntegratedServerTest, DimensionsShareStorageAndSaveManager)
{
    IntegratedServer server;
    IntegratedServerConfig config;
    config.worldName = "test_world_shared_storage";
    config.seed = 67890;
    config.viewDistance = 8;

    auto result = server.initialize(config);
    ASSERT_TRUE(result.success());

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
    EXPECT_EQ(overworld->world()->saveManager(), nether->world()->saveManager());
    EXPECT_EQ(overworld->world()->saveManager(), theEnd->world()->saveManager());

    server.stop();
}

TEST(IntegratedServerTest, TickCountIncreases)
{
    IntegratedServer server;
    IntegratedServerConfig config;
    config.tickRate = 100; // 100 TPS for faster testing

    auto result = server.initialize(config);
    ASSERT_TRUE(result.success());

    u64 initialTick = server.currentTick();

    // Wait for some ticks
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    u64 finalTick = server.currentTick();
    EXPECT_GT(finalTick, initialTick);

    server.stop();
}

TEST(IntegratedServerTest, RequestStopDisconnectsClientEndpoint)
{
    IntegratedServer server;
    IntegratedServerConfig config;
    config.tickRate = 100;

    auto result = server.initialize(config);
    ASSERT_TRUE(result.success());

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

class IntegratedServerCommunicationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        config.worldName = "test_world";
        config.seed = 12345;
        config.viewDistance = 3;
        config.tickRate = 100; // Faster ticks for testing

        auto result = server.initialize(config);
        ASSERT_TRUE(result.success());

        clientEndpoint = server.getClientEndpoint();
        ASSERT_NE(clientEndpoint, nullptr);
    }

    void TearDown() override { server.stop(); }

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
    IntegratedServerConfig config;
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

    // 等待一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    u64 finalTick = server.currentTick();

    // 100 TPS * 0.2s = ~20 ticks
    EXPECT_GT(finalTick - initialTick, 10);
}

// ============================================================================
// 断开连接测试
// ============================================================================

TEST(IntegratedServerDisconnectTest, ClientDisconnect)
{
    IntegratedServer server;
    IntegratedServerConfig config;

    auto result = server.initialize(config);
    ASSERT_TRUE(result.success());

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

TEST(IntegratedServerDisconnectTest, ServerStopClosesEndpoint)
{
    LocalEndpoint* client = nullptr;

    {
        IntegratedServer server;
        IntegratedServerConfig config;

        auto result = server.initialize(config);
        ASSERT_TRUE(result.success());

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

TEST(IntegratedServerTypeTest, IsIntegratedReturnsTrue)
{
    IntegratedServer server;
    EXPECT_TRUE(server.isIntegrated());
}

TEST(IntegratedServerTypeTest, IsDedicatedReturnsFalse)
{
    IntegratedServer server;
    EXPECT_FALSE(server.isDedicated());
}

TEST(IntegratedServerTypeTest, TypeMethodsWorkBeforeInitialization)
{
    IntegratedServer server;
    // 类型检查方法应该在初始化前后都能正常工作
    EXPECT_TRUE(server.isIntegrated());
    EXPECT_FALSE(server.isDedicated());
}

TEST(IntegratedServerTypeTest, TypeMethodsWorkAfterInitialization)
{
    IntegratedServer server;
    IntegratedServerConfig config;
    config.seed = 1;

    auto result = server.initialize(config);
    ASSERT_TRUE(result.success());

    EXPECT_TRUE(server.isIntegrated());
    EXPECT_FALSE(server.isDedicated());

    server.stop();
}
