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

#include "common/network/buffer/RegistryByteBuf.hpp"
#include "common/network/crypto/Crypt.hpp"
#include "common/network/ir/IrPacket.hpp"
#include "common/network/pipeline/Connection.hpp"
#include "common/network/pipeline/ProtocolTableSet.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/protocol/PacketFlow.hpp"
#include "common/network/transport/LocalTransport.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <vector>

using namespace mc::network::pipeline;
using namespace mc::network::transport;
using namespace mc::network;
using namespace mc;

namespace {

using B = buffer::RegistryByteBuf;

ir::IrPacket makeKeepAlive(i64 id)
{
    ir::play::KeepAlive ka;
    ka.id = id;
    return ir::IrPacket{protocol::ConnectionProtocol::Play, ir::PlayPacket{ka}};
}

// 一对经 LocalTransportPair 互连的 Connection：A 发 → B pumpLocal 收。
// Connection 直接拥有各自侧的 LocalTransport（LocalTransportPair 互为 peer，
// 各自 move 进 Connection 后 peer 指针仍有效——LocalTransportPair::create 已 setPeer）。
struct LocalConnectionPair {
    std::shared_ptr<ProtocolTableSet<B>> tables;
    std::unique_ptr<Connection<B>> client;
    std::unique_ptr<Connection<B>> server;
};

LocalConnectionPair makePair()
{
    auto pair = LocalTransportPair::create();
    auto tables = std::make_shared<ProtocolTableSet<B>>(); // Local 模式仅用于阶段校验，空表即可
    LocalConnectionPair out;
    out.tables = tables;
    // client 流向 Serverbound（发出方向），server 流向 Clientbound。
    // LocalTransport 继承 ILocalTransport，unique_ptr<LocalTransport> 可 move 进 ILocalTransport 槽位。
    out.client = std::make_unique<Connection<B>>(std::move(pair.client), tables, protocol::PacketFlow::Serverbound);
    out.server = std::make_unique<Connection<B>>(std::move(pair.server), tables, protocol::PacketFlow::Clientbound);
    return out;
}

} // namespace

TEST(ConnectionLocal, SendThenPumpDeliversToListener)
{
    auto pair = makePair();

    std::vector<i64> received;
    pair.server->onPacket([&](const ir::IrPacket& pkt) {
        const auto* play = std::get_if<ir::PlayPacket>(&pkt.packet);
        if (play != nullptr) {
            if (const auto* ka = std::get_if<ir::play::KeepAlive>(play)) {
                received.push_back(ka->id);
            }
        }
    });

    ASSERT_TRUE(pair.client->send(makeKeepAlive(7)).success());
    pair.server->pumpLocal(); // 驱动 server 端收件箱
    EXPECT_EQ(received, (std::vector<i64>{7}));
}

TEST(ConnectionLocal, NewConnectionDefaultsToHandshaking)
{
    auto pair = makePair();
    EXPECT_EQ(pair.client->phase(), protocol::ConnectionProtocol::Handshaking);
    EXPECT_EQ(pair.server->phase(), protocol::ConnectionProtocol::Handshaking);
    EXPECT_TRUE(pair.client->isLocalMode());
    EXPECT_TRUE(pair.server->isLocalMode());
}

TEST(ConnectionLocal, PhaseSwitchOnTerminalReceive)
{
    auto pair = makePair();
    // 发 ConfigurationAcknowledged（terminal，Play→Configuration）到 server。
    // 注意：Local 模式接收侧在 _installLocalReceive 里切阶段，需 server.pumpLocal 后生效。
    pair.server->setPhase(protocol::ConnectionProtocol::Play);
    EXPECT_EQ(pair.server->phase(), protocol::ConnectionProtocol::Play);

    pair.client->setPhase(protocol::ConnectionProtocol::Play);
    ir::play::ConfigurationAcknowledged ca;
    ASSERT_TRUE(pair.client->send(ir::IrPacket{protocol::ConnectionProtocol::Play, ir::PlayPacket{ca}}).success());
    pair.server->pumpLocal();
    EXPECT_EQ(pair.server->phase(), protocol::ConnectionProtocol::Configuration);
}

TEST(ConnectionLocal, ThreePacketsFifo)
{
    auto pair = makePair();
    std::vector<i64> ids;
    pair.server->onPacket([&](const ir::IrPacket& pkt) {
        const auto* play = std::get_if<ir::PlayPacket>(&pkt.packet);
        if (play != nullptr) {
            if (const auto* ka = std::get_if<ir::play::KeepAlive>(play)) {
                ids.push_back(ka->id);
            }
        }
    });

    for (i64 i = 1; i <= 3; ++i) {
        ASSERT_TRUE(pair.client->send(makeKeepAlive(i)).success());
    }
    pair.server->pumpLocal();
    EXPECT_EQ(ids, (std::vector<i64>{1, 2, 3}));
}

TEST(ConnectionLocal, ListenerReceivesCorrectAltIndex)
{
    auto pair = makePair();
    std::atomic<int> altIndex{-1};
    pair.server->onPacket([&](const ir::IrPacket& pkt) {
        const auto* play = std::get_if<ir::PlayPacket>(&pkt.packet);
        if (play != nullptr) {
            altIndex = static_cast<int>(play->index());
        }
    });

    // KeepAlive 是 PlayPacket 的 altIndex 5
    ASSERT_TRUE(pair.client->send(makeKeepAlive(42)).success());
    pair.server->pumpLocal();
    EXPECT_EQ(altIndex.load(), 5);
}

TEST(ConnectionLocal, CloseThenSendFails)
{
    auto pair = makePair();
    pair.client->close();
    auto r = pair.client->send(makeKeepAlive(1));
    ASSERT_FALSE(r.success());
    EXPECT_EQ(r.error().code(), ErrorCode::InvalidState);
}

TEST(ConnectionLocal, IsConnectedBeforeAndAfterClose)
{
    auto pair = makePair();
    EXPECT_TRUE(pair.client->isConnected());
    EXPECT_TRUE(pair.server->isConnected());
    pair.client->close();
    EXPECT_FALSE(pair.client->isConnected());
}

TEST(ConnectionLocal, SetupCompressionAndEncryptionAreNoOpInLocalMode)
{
    auto pair = makePair();
    // Local 模式不经压缩/加密流水线，调用 setup* 不应影响 send/recv 正确性。
    pair.client->setupCompression(256);
    std::array<u8, crypto::kSharedSecretBytes> secret{};
    ASSERT_TRUE(pair.client->setupEncryption(secret).success());

    std::vector<i64> received;
    pair.server->onPacket([&](const ir::IrPacket& pkt) {
        const auto* play = std::get_if<ir::PlayPacket>(&pkt.packet);
        if (play != nullptr) {
            if (const auto* ka = std::get_if<ir::play::KeepAlive>(play)) {
                received.push_back(ka->id);
            }
        }
    });
    ASSERT_TRUE(pair.client->send(makeKeepAlive(99)).success());
    pair.server->pumpLocal();
    EXPECT_EQ(received, (std::vector<i64>{99}));
}

TEST(ConnectionLocal, Bidirectional)
{
    auto pair = makePair();
    std::atomic<int> clientCount{0};
    pair.client->onPacket([&](const ir::IrPacket& /*pkt*/) { clientCount++; });

    ASSERT_TRUE(pair.server->send(makeKeepAlive(5)).success());
    pair.client->pumpLocal(); // client 收件箱驱动
    EXPECT_EQ(clientCount.load(), 1);
}
