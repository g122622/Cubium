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

#include "common/core/Types.hpp"
#include "common/network/connection/LocalConnection.hpp"
#include <atomic>
#include <thread>
#include <vector>
#include <gtest/gtest.h>

using namespace mc::network;
using namespace mc;

// ============================================================================
// 基础测试
// ============================================================================

TEST(LocalConnectionTest, CreateEndpoint)
{
    LocalEndpoint endpoint;
    EXPECT_FALSE(endpoint.isConnected());
    EXPECT_FALSE(endpoint.hasData());
    EXPECT_EQ(endpoint.pendingCount(), 0);
}

TEST(LocalConnectionTest, CreateConnectionPair)
{
    LocalConnectionPair pair;
    EXPECT_FALSE(pair.clientEndpoint().isConnected());
    EXPECT_FALSE(pair.serverEndpoint().isConnected());
}

TEST(LocalConnectionTest, ConnectPair)
{
    LocalConnectionPair pair;
    pair.connect();

    EXPECT_TRUE(pair.clientEndpoint().isConnected());
    EXPECT_TRUE(pair.serverEndpoint().isConnected());
}

TEST(LocalConnectionTest, DisconnectPair)
{
    LocalConnectionPair pair;
    pair.connect();
    EXPECT_TRUE(pair.clientEndpoint().isConnected());

    pair.disconnect();
    EXPECT_FALSE(pair.clientEndpoint().isConnected());
    EXPECT_FALSE(pair.serverEndpoint().isConnected());
}

// ============================================================================
// 发送/接收测试
// ============================================================================

TEST(LocalConnectionTest, SendReceiveSinglePacket)
{
    LocalConnectionPair pair;
    pair.connect();

    // 发送数据
    std::vector<u8> sendData = {1, 2, 3, 4, 5};
    pair.clientEndpoint().send(sendData.data(), sendData.size());

    // 接收数据
    std::vector<u8> receiveData;
    EXPECT_TRUE(pair.serverEndpoint().receive(receiveData));
    EXPECT_EQ(receiveData.size(), sendData.size());
    EXPECT_EQ(receiveData, sendData);
}

TEST(LocalConnectionTest, SendMultiplePackets)
{
    LocalConnectionPair pair;
    pair.connect();

    // 发送多个数据包
    std::vector<std::vector<u8>> packets = {{1, 2, 3}, {4, 5, 6, 7}, {8, 9, 10, 11, 12}};

    for (const auto& packet : packets) {
        pair.clientEndpoint().send(packet.data(), packet.size());
    }

    // 接收所有数据包
    EXPECT_EQ(pair.serverEndpoint().pendingCount(), 3);

    for (size_t i = 0; i < packets.size(); ++i) {
        std::vector<u8> receiveData;
        EXPECT_TRUE(pair.serverEndpoint().receive(receiveData));
        EXPECT_EQ(receiveData, packets[i]);
    }

    EXPECT_FALSE(pair.serverEndpoint().hasData());
}

TEST(LocalConnectionTest, BidirectionalCommunication)
{
    LocalConnectionPair pair;
    pair.connect();

    // 客户端发送到服务端
    std::vector<u8> clientData = {1, 2, 3};
    pair.clientEndpoint().send(clientData.data(), clientData.size());

    std::vector<u8> serverReceive;
    EXPECT_TRUE(pair.serverEndpoint().receive(serverReceive));
    EXPECT_EQ(serverReceive, clientData);

    // 服务端发送到客户端
    std::vector<u8> serverData = {4, 5, 6};
    pair.serverEndpoint().send(serverData.data(), serverData.size());

    std::vector<u8> clientReceive;
    EXPECT_TRUE(pair.clientEndpoint().receive(clientReceive));
    EXPECT_EQ(clientReceive, serverData);
}

TEST(LocalConnectionTest, EmptyQueue)
{
    LocalConnectionPair pair;
    pair.connect();

    std::vector<u8> data;
    EXPECT_FALSE(pair.clientEndpoint().receive(data));
    EXPECT_FALSE(pair.serverEndpoint().receive(data));
}

TEST(LocalConnectionTest, DisconnectClearsQueue)
{
    LocalConnectionPair pair;
    pair.connect();

    // 发送数据
    std::vector<u8> data = {1, 2, 3};
    pair.clientEndpoint().send(data.data(), data.size());
    EXPECT_TRUE(pair.serverEndpoint().hasData());

    // 断开连接，队列应该被清空
    pair.disconnect();
    EXPECT_FALSE(pair.serverEndpoint().hasData());
}

// ============================================================================
// 线程安全测试
// ============================================================================

TEST(LocalConnectionTest, MultithreadSendReceive)
{
    LocalConnectionPair pair;
    pair.connect();

    const int numPackets = 100;
    std::atomic<int> receivedCount{0};

    // 接收线程
    std::thread receiver([&]() {
        std::vector<u8> data;
        while (receivedCount < numPackets) {
            if (pair.serverEndpoint().receive(data)) {
                receivedCount++;
            } else {
                std::this_thread::yield();
            }
        }
    });

    // 发送线程
    std::thread sender([&]() {
        for (int i = 0; i < numPackets; ++i) {
            std::vector<u8> data = {static_cast<u8>(i & 0xFF)};
            pair.clientEndpoint().send(data.data(), data.size());
        }
    });

    sender.join();
    receiver.join();

    EXPECT_EQ(receivedCount.load(), numPackets);
}

TEST(LocalConnectionTest, BlockingReceive)
{
    LocalConnectionPair pair;
    pair.connect();

    std::vector<u8> expectedData = {1, 2, 3, 4, 5};
    bool received = false;

    // 延迟发送的线程
    std::thread delayedSender([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        pair.clientEndpoint().send(expectedData.data(), expectedData.size());
    });

    // 主线程阻塞等待
    std::vector<u8> receiveData;
    auto start = std::chrono::steady_clock::now();
    received = pair.serverEndpoint().receiveWait(receiveData, 1000);
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    delayedSender.join();

    EXPECT_TRUE(received);
    EXPECT_EQ(receiveData, expectedData);
    EXPECT_GE(duration.count(), 50); // 至少等待了发送延迟
}

TEST(LocalConnectionTest, BlockingReceiveTimeout)
{
    LocalConnectionPair pair;
    pair.connect();

    std::vector<u8> receiveData;
    auto start = std::chrono::steady_clock::now();
    bool received = pair.serverEndpoint().receiveWait(receiveData, 100);
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    EXPECT_FALSE(received);
    EXPECT_GE(duration.count(), 100);
}

// ============================================================================
// 大数据测试
// ============================================================================

TEST(LocalConnectionTest, LargePacket)
{
    LocalConnectionPair pair;
    pair.connect();

    // 发送大数据包 (64KB)
    std::vector<u8> largeData(64 * 1024);
    for (size_t i = 0; i < largeData.size(); ++i) {
        largeData[i] = static_cast<u8>(i & 0xFF);
    }

    pair.clientEndpoint().send(largeData.data(), largeData.size());

    std::vector<u8> receiveData;
    EXPECT_TRUE(pair.serverEndpoint().receive(receiveData));
    EXPECT_EQ(receiveData.size(), largeData.size());
    EXPECT_EQ(receiveData, largeData);
}

// ============================================================================
// 边界情况测试
// ============================================================================

TEST(LocalConnectionTest, SendToDisconnectedEndpoint)
{
    LocalConnectionPair pair;
    // 不连接

    std::vector<u8> data = {1, 2, 3};
    pair.clientEndpoint().send(data.data(), data.size());

    // 服务端不应该收到数据
    EXPECT_FALSE(pair.serverEndpoint().hasData());
}

TEST(LocalConnectionTest, Reconnect)
{
    LocalConnectionPair pair;

    // 第一次连接
    pair.connect();
    std::vector<u8> data1 = {1, 2, 3};
    pair.clientEndpoint().send(data1.data(), data1.size());
    std::vector<u8> receiveData;
    EXPECT_TRUE(pair.serverEndpoint().receive(receiveData));

    // 断开连接
    pair.disconnect();

    // 重新连接
    pair.connect();
    std::vector<u8> data2 = {4, 5, 6};
    pair.clientEndpoint().send(data2.data(), data2.size());
    EXPECT_TRUE(pair.serverEndpoint().receive(receiveData));
    EXPECT_EQ(receiveData, data2);
}
