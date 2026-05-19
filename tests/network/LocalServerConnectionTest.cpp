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

#include "common/network/connection/LocalServerConnection.hpp"
#include "common/network/connection/LocalConnection.hpp"
#include <gtest/gtest.h>

using namespace mc::network;

// ============================================================================
// LocalServerConnection 测试
// ============================================================================

class LocalServerConnectionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_connectionPair = std::make_unique<LocalConnectionPair>();
        m_connectionPair->connect();
    }

    void TearDown() override { m_connectionPair.reset(); }

    std::unique_ptr<LocalConnectionPair> m_connectionPair;
};

TEST_F(LocalServerConnectionTest, BasicSendReceive)
{
    // 创建服务端连接包装器
    LocalServerConnection serverConn(&m_connectionPair->serverEndpoint());

    EXPECT_TRUE(serverConn.isConnected());
    EXPECT_EQ(serverConn.type(), ConnectionType::Local);

    // 发送数据
    mc::u8 sendData[] = {1, 2, 3, 4, 5};
    serverConn.send(sendData, 5);

    // 客户端接收
    std::vector<mc::u8> recvData;
    bool received = m_connectionPair->clientEndpoint().receive(recvData);
    EXPECT_TRUE(received);
    EXPECT_EQ(recvData.size(), static_cast<size_t>(5));
}

TEST_F(LocalServerConnectionTest, Disconnect)
{
    LocalServerConnection serverConn(&m_connectionPair->serverEndpoint());
    EXPECT_TRUE(serverConn.isConnected());

    serverConn.disconnect("Test disconnect");
    EXPECT_FALSE(serverConn.isConnected());
}

TEST_F(LocalServerConnectionTest, Identifier)
{
    LocalServerConnection serverConn(&m_connectionPair->serverEndpoint());
    std::string id = serverConn.identifier();
    EXPECT_TRUE(id.find("Local:") != std::string::npos);
}

TEST_F(LocalServerConnectionTest, SendWhenDisconnected)
{
    LocalServerConnection serverConn(&m_connectionPair->serverEndpoint());
    serverConn.disconnect();

    // 发送到断开的连接不应崩溃
    mc::u8 data[] = {1, 2, 3};
    serverConn.send(data, 3); // 不应崩溃
}

TEST_F(LocalServerConnectionTest, NullEndpoint)
{
    LocalServerConnection serverConn(nullptr);
    EXPECT_FALSE(serverConn.isConnected());
    // identifier 仍然会有一个 ID 号
    std::string id = serverConn.identifier();
    EXPECT_TRUE(id.find("Local:") != std::string::npos);

    // 发送到 null endpoint 不应崩溃
    mc::u8 data[] = {1, 2, 3};
    serverConn.send(data, 3); // 不应崩溃
}

TEST_F(LocalServerConnectionTest, UseThroughInterface)
{
    // 测试通过接口使用
    ConnectionPtr conn = std::make_shared<LocalServerConnection>(&m_connectionPair->serverEndpoint());

    EXPECT_TRUE(conn->isConnected());
    EXPECT_EQ(conn->type(), ConnectionType::Local);

    mc::u8 sendData[] = {10, 20, 30};
    conn->send(sendData, 3);

    std::vector<mc::u8> recvData;
    bool received = m_connectionPair->clientEndpoint().receive(recvData);
    EXPECT_TRUE(received);
    EXPECT_EQ(recvData.size(), static_cast<size_t>(3));
}
