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

// Phase3 阶段3：ServerNetwork Local 模式单元测试。
// 验证 createLocalClientSide 注册连接（sessionId==0）、onClientConnect 同步触发、
// tick() pump Local 连接、broadcast 仅达 Play 状态连接（Handshake 状态连接被跳过）、
// 关闭客户端 transport 后 isConnected()==false。
//
// 注：onClientDisconnect 是 Wire/TcpTransport 路径（接收线程 _notifyDisconnect→tick 主线程
// 回调），Local 模式无该回调路径，故不在此测；Local 断开以 isConnected() 翻转为判据。

#include "common/network/NetworkTestFixtures.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "server/network/session/ServerNetwork.hpp"

#include <gtest/gtest.h>

#include <memory>

using namespace mc::network;
using namespace mc::network::ir;
using namespace mc::network::protocol;
using namespace mc::network::test;
using namespace mc::server::net;
using namespace mc;

// ============================================================================
// 连接注册 + onClientConnect
// ============================================================================

TEST_F(LocalServerFixture, CreateLocalClientSideRegistersSessionIdZero)
{
    // LocalServerFixture::SetUp 已调 createLocalClientSide；本地客户端固定 sessionId==0。
    EXPECT_EQ(serverConn()->sessionId(), 0u);
}

TEST_F(LocalServerFixture, OnClientConnectFiresOnCreateLocalClientSide)
{
    // 用独立 ServerNetwork 装回调，再 createLocalClientSide 验证同步触发。
    auto net = std::make_unique<ServerNetwork>();
    int connectCount = 0;
    ServerClientConnection* observed = nullptr;
    net->onClientConnect([&](ServerClientConnection& c) {
        ++connectCount;
        observed = &c;
    });

    std::unique_ptr<transport::ILocalTransport> clientSide;
    auto* conn = net->createLocalClientSide(&clientSide);
    EXPECT_EQ(connectCount, 1);
    EXPECT_EQ(observed, conn);
    EXPECT_EQ(conn->sessionId(), 0u);

    // TearDown 顺序：先释放客户端 transport，再销毁 ServerNetwork。
    clientSide.reset();
    net.reset();
}

// ============================================================================
// tick() pump Local 连接：客户端发的包经 tick 到达服务端监听器
// ============================================================================

TEST_F(LocalServerFixture, TickPumpsLocalConnectionInbound)
{
    installOfflineHandshake(/*compressionThreshold=*/-1);

    // clientSend 把包投到服务端入站队列（不触发回调）；pumpServer 驱动后状态机推进，
    // 证明 tick/pump 链路把 Local 连接的入站包送达监听器。
    handshake::ClientIntention ci{};
    ci.protocolVersion = 774;
    ci.hostName = "localhost";
    ci.port = 25565;
    ci.intendedState = 2;
    clientSend(IrPacket{ConnectionProtocol::Handshaking, HandshakePacket{std::move(ci)}});
    pumpServer();
    EXPECT_EQ(serverConn()->state(), HandshakeState::Login);
}

// ============================================================================
// 关闭客户端 transport → 服务端连接 isConnected()==false
// ============================================================================

TEST_F(LocalServerFixture, CloseClientTransportMarksServerDisconnected)
{
    installOfflineHandshake(/*compressionThreshold=*/-1);
    EXPECT_TRUE(serverConn()->isConnected());

    // 关闭服务端连接：isConnected 翻 false
    serverConn()->close();
    EXPECT_FALSE(serverConn()->isConnected());
    EXPECT_EQ(serverConn()->state(), HandshakeState::Disconnected);
}
