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

#include "common/network/ir/IrPacket.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/transport/LocalTransport.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <vector>

using namespace mc::network::transport;
using namespace mc::network;
using namespace mc;

namespace {

ir::IrPacket makeKeepAlive(i64 id)
{
    ir::play::KeepAlive ka;
    ka.id = id;
    return ir::IrPacket{protocol::ConnectionProtocol::Play, ir::PlayPacket{ka}};
}

} // namespace

TEST(LocalTransport, ClientToServerFifo)
{
    auto pair = LocalTransportPair::create();

    std::vector<i64> received;
    pair.server->onPacket([&](ir::IrPacket pkt) {
        const auto* play = std::get_if<ir::PlayPacket>(&pkt.packet);
        if (play != nullptr) {
            if (const auto* ka = std::get_if<ir::play::KeepAlive>(play)) {
                received.push_back(ka->id);
            }
        }
    });

    ASSERT_TRUE(pair.client->send(makeKeepAlive(1)).success());
    ASSERT_TRUE(pair.client->send(makeKeepAlive(2)).success());
    ASSERT_TRUE(pair.client->send(makeKeepAlive(3)).success());

    pair.server->pump();
    EXPECT_EQ(received, (std::vector<i64>{1, 2, 3}));
}

TEST(LocalTransport, ServerToClientBidirectional)
{
    auto pair = LocalTransportPair::create();

    std::atomic<int> count{0};
    pair.client->onPacket([&](ir::IrPacket /*pkt*/) { count++; });

    ASSERT_TRUE(pair.server->send(makeKeepAlive(99)).success());
    pair.client->pump();
    EXPECT_EQ(count.load(), 1);
}

TEST(LocalTransport, IsConnectedBeforeAndAfterClose)
{
    auto pair = LocalTransportPair::create();
    EXPECT_TRUE(pair.client->isConnected());
    EXPECT_TRUE(pair.server->isConnected());

    pair.client->close();
    EXPECT_FALSE(pair.client->isConnected());
    // server 仍连接（close 是本端状态，对端靠 onDisconnect 感知）
    EXPECT_TRUE(pair.server->isConnected());
}

TEST(LocalTransport, SendAfterCloseFails)
{
    auto pair = LocalTransportPair::create();
    pair.client->close();
    auto r = pair.client->send(makeKeepAlive(1));
    ASSERT_FALSE(r.success());
    EXPECT_EQ(r.error().code(), ErrorCode::InvalidState);
}

TEST(LocalTransport, CloseTriggersDisconnectCallback)
{
    auto pair = LocalTransportPair::create();
    std::atomic<bool> disconnected{false};
    pair.client->onDisconnect([&]() { disconnected = true; });

    EXPECT_FALSE(disconnected.load());
    pair.client->close();
    EXPECT_TRUE(disconnected.load());
}

TEST(LocalTransport, PumpWithNoCallbackIsSafe)
{
    // 未注册 onPacket 时 pump 不崩
    auto pair = LocalTransportPair::create();
    ASSERT_TRUE(pair.client->send(makeKeepAlive(1)).success());
    pair.server->pump(); // 无 callback，应安全无操作
    SUCCEED();
}

TEST(LocalTransport, PumpDeliversBatchesInOrder)
{
    auto pair = LocalTransportPair::create();
    std::vector<int> order;
    pair.server->onPacket([&](ir::IrPacket /*pkt*/) { order.push_back(static_cast<int>(order.size())); });

    for (int i = 0; i < 50; ++i) {
        ASSERT_TRUE(pair.client->send(makeKeepAlive(i)).success());
    }
    pair.server->pump();
    ASSERT_EQ(order.size(), 50u);
    for (int i = 0; i < 50; ++i) {
        EXPECT_EQ(order[i], i);
    }
}
