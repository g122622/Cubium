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

#include "NetworkTestFixtures.hpp"
#include "common/network/pipeline/Connection.hpp"
#include "common/network/pipeline/OutboundPacket.hpp"
#include "common/network/transport/TcpTransport.hpp"

#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <asio.hpp>

namespace mc::network::test {
namespace {

using namespace std::chrono_literals;
using Conn = pipeline::Connection<buffer::RegistryByteBuf>;
using Phase = protocol::ConnectionProtocol;

class ConnectionWireAsyncTest : public NetworkTestBase {
protected:
    void SetUp() override
    {
        NetworkTestBase::SetUp();
        asio::ip::tcp::acceptor acceptor(m_context, {asio::ip::tcp::v4(), 0});
        m_peer.connect({asio::ip::address_v4::loopback(), acceptor.local_endpoint().port()});
        auto transport = std::make_unique<transport::TcpTransport>();
        ASSERT_TRUE(transport->attachConnectedSocket(acceptor.accept()).success());
        m_transport = transport.get();
        m_connection = std::make_unique<Conn>(std::move(transport), tables(), protocol::PacketFlow::Clientbound);
        m_peer.non_blocking(true);
    }

    void TearDown() override { m_connection.reset(); }

    std::vector<std::vector<u8>> _readFrames(usize count)
    {
        std::vector<std::vector<u8>> frames;
        std::vector<u8> pending;
        std::array<u8, 65536> bytes{};
        const auto deadline = std::chrono::steady_clock::now() + 5s;
        while (frames.size() < count && std::chrono::steady_clock::now() < deadline) {
            asio::error_code error;
            const auto received = m_peer.read_some(asio::buffer(bytes), error);
            if (!error)
                pending.insert(pending.end(), bytes.begin(), bytes.begin() + received);
            else if (error != asio::error::would_block && error != asio::error::try_again)
                break;
            while (!pending.empty()) {
                std::vector<u8> frame;
                auto decoded = pipeline::VarintFraming::tryDecodeFrame(pending, frame);
                if (!decoded.success() || !decoded.value()) break;
                frames.push_back(std::move(frame));
            }
            if (frames.size() < count) std::this_thread::sleep_for(1ms);
        }
        EXPECT_EQ(frames.size(), count);
        return frames;
    }

    static ir::IrPacket _keepAlive(i64 id) { return {Phase::Play, ir::PlayPacket{ir::play::KeepAlive{id}}}; }

    asio::io_context m_context;
    asio::ip::tcp::socket m_peer{m_context};
    transport::TcpTransport* m_transport = nullptr;
    std::unique_ptr<Conn> m_connection;
};

TEST_F(ConnectionWireAsyncTest, EncodingIsQueuedAndPreservesPacketOrder)
{
    auto entered = std::make_shared<std::promise<void>>();
    auto release = std::make_shared<std::promise<void>>();
    auto released = release->get_future().share();
    ASSERT_TRUE(m_transport
            ->execute([entered, released] {
                entered->set_value();
                released.wait_for(3s);
                return Result<void>::ok();
            })
            .success());
    ASSERT_EQ(entered->get_future().wait_for(2s), std::future_status::ready);
    m_connection->setOutboundPhase(Phase::Play);
    const auto started = std::chrono::steady_clock::now();
    for (i64 id = 0; id < 64; ++id)
        EXPECT_TRUE(m_connection->send(_keepAlive(id)).success());
    EXPECT_LT(std::chrono::steady_clock::now() - started, 500ms);
    release->set_value();
    const auto frames = _readFrames(64);
    for (usize index = 0; index < frames.size(); ++index) {
        buffer::RegistryByteBuf bytes(frames[index].data(), frames[index].size());
        auto decoded = tables()->playCb->decode(bytes);
        ASSERT_TRUE(decoded.success());
        EXPECT_EQ(std::get<ir::play::KeepAlive>(decoded.value()).id, static_cast<i64>(index));
    }
}

TEST_F(ConnectionWireAsyncTest, CompressionStartsAfterTheCompressionNotification)
{
    m_connection->setOutboundPhase(Phase::Login);
    EXPECT_TRUE(m_connection->send({Phase::Login, ir::LoginPacket{ir::login::LoginCompression{1}}}).success());
    m_connection->setupCompression(1);
    ir::login::LoginFinished finished{};
    finished.username = "AsyncPlayer";
    EXPECT_TRUE(m_connection->send({Phase::Login, ir::LoginPacket{finished}}).success());
    const auto frames = _readFrames(2);
    ASSERT_EQ(frames.size(), 2u);
    buffer::RegistryByteBuf notification(frames[0].data(), frames[0].size());
    auto first = tables()->loginCb->decode(notification);
    ASSERT_TRUE(first.success());
    EXPECT_EQ(std::get<ir::login::LoginCompression>(first.value()).threshold, 1);
    pipeline::CompressionDecoder decoder(1);
    std::vector<u8> plain;
    ASSERT_TRUE(decoder.decode(frames[1], plain).success());
    buffer::RegistryByteBuf completion(plain.data(), plain.size());
    auto second = tables()->loginCb->decode(completion);
    ASSERT_TRUE(second.success());
    EXPECT_EQ(std::get<ir::login::LoginFinished>(second.value()).username, "AsyncPlayer");
}

TEST_F(ConnectionWireAsyncTest, ReconfigurationTerminalIsEncodedBeforeThePhaseChanges)
{
    m_connection->setOutboundPhase(Phase::Play);
    ASSERT_TRUE(m_connection->send({Phase::Play, ir::PlayPacket{ir::play::StartConfiguration{}}}).success());
    ASSERT_TRUE(m_connection->send({Phase::Configuration, ir::ConfigurationPacket{ir::configuration::KeepAlive{91}}})
            .success());
    const auto frames = _readFrames(2);
    ASSERT_EQ(frames.size(), 2u);
    buffer::RegistryByteBuf first(frames[0].data(), frames[0].size());
    auto terminal = tables()->playCb->decode(first);
    ASSERT_TRUE(terminal.success());
    EXPECT_TRUE(std::holds_alternative<ir::play::StartConfiguration>(terminal.value()));
    buffer::RegistryByteBuf second(frames[1].data(), frames[1].size());
    auto next = tables()->configurationCb->decode(second);
    ASSERT_TRUE(next.success());
    EXPECT_EQ(std::get<ir::configuration::KeepAlive>(next.value()).id, 91);
}

TEST_F(ConnectionWireAsyncTest, WrongPhaseFailsAsynchronouslyAndClosesConnection)
{
    ASSERT_TRUE(m_connection->send(_keepAlive(1)).success());
    m_transport->synchronize();
    EXPECT_FALSE(m_connection->isConnected());
}

TEST_F(ConnectionWireAsyncTest, EncryptionPreservesStreamStateAcrossQueuedPackets)
{
    m_connection->setOutboundPhase(Phase::Play);
    const std::array<u8, crypto::kSharedSecretBytes> key{};
    ASSERT_TRUE(m_connection->setupEncryption(key).success());
    ASSERT_TRUE(m_connection->send(_keepAlive(11)).success());
    ASSERT_TRUE(m_connection->send(_keepAlive(12)).success());
    pipeline::CipherDecoder cipher;
    ASSERT_TRUE(cipher.init(key).success());
    std::vector<u8> pending;
    std::vector<i64> ids;
    const auto deadline = std::chrono::steady_clock::now() + 3s;
    while (ids.size() < 2 && std::chrono::steady_clock::now() < deadline) {
        std::array<u8, 128> bytes{};
        asio::error_code error;
        const auto count = m_peer.read_some(asio::buffer(bytes), error);
        if (!error) {
            std::vector<u8> encrypted(bytes.begin(), bytes.begin() + count);
            std::vector<u8> plain;
            ASSERT_TRUE(cipher.decode(encrypted, plain).success());
            pending.insert(pending.end(), plain.begin(), plain.end());
            while (!pending.empty()) {
                std::vector<u8> frame;
                auto decoded = pipeline::VarintFraming::tryDecodeFrame(pending, frame);
                ASSERT_TRUE(decoded.success());
                if (!decoded.value()) break;
                buffer::RegistryByteBuf packet(frame.data(), frame.size());
                auto value = tables()->playCb->decode(packet);
                ASSERT_TRUE(value.success());
                ids.push_back(std::get<ir::play::KeepAlive>(value.value()).id);
            }
        } else if (error != asio::error::would_block && error != asio::error::try_again)
            break;
        if (ids.size() < 2) std::this_thread::sleep_for(1ms);
    }
    EXPECT_EQ(ids, (std::vector<i64>{11, 12}));
}

TEST_F(ConnectionWireAsyncTest, GracefulCloseDeliversEveryQueuedFrame)
{
    m_connection->setOutboundPhase(Phase::Play);
    for (i64 id = 0; id < 32; ++id)
        ASSERT_TRUE(m_connection->send(_keepAlive(id)).success());
    m_connection->closeAfterFlush();
    EXPECT_FALSE(m_connection->send(_keepAlive(100)).success());
    EXPECT_EQ(_readFrames(32).size(), 32u);
    m_transport->synchronize();
}

TEST_F(ConnectionWireAsyncTest, InvalidFrameClosesSafelyInsideReceiveCallback)
{
    std::promise<void> disconnected;
    m_transport->onDisconnect([&] { disconnected.set_value(); });
    ASSERT_TRUE(m_transport->startReceiving().success());
    // 超过帧长度前缀上限，错误处理在网络执行器内主动关闭。
    const std::array<u8, 6> malformed{0x80, 0x80, 0x80, 0x80, 0x80, 0x80};
    asio::error_code error;
    asio::write(m_peer, asio::buffer(malformed), error);
    ASSERT_FALSE(error);
    EXPECT_EQ(disconnected.get_future().wait_for(3s), std::future_status::ready);
    m_connection->close();
    m_transport->onDisconnect(nullptr);
}

TEST_F(ConnectionWireAsyncTest, SlowPeerDoesNotBlockConnectionExecutor)
{
    // 对端不读取大包；异步写入仍允许同一连接继续处理控制任务。
    std::vector<u8> bytes(4 * 1024 * 1024, 42);
    const auto started = std::chrono::steady_clock::now();
    EXPECT_TRUE(m_transport->send(bytes.data(), bytes.size(), transport::DeliveryHint::ReliableOrdered).success());
    std::promise<void> progressed;
    EXPECT_TRUE(m_transport
            ->execute([&] {
                progressed.set_value();
                return Result<void>::ok();
            })
            .success());
    EXPECT_EQ(progressed.get_future().wait_for(2s), std::future_status::ready);
    EXPECT_LT(std::chrono::steady_clock::now() - started, 2s);
    m_connection->close();
}

TEST_F(ConnectionWireAsyncTest, TaskQueueLimitDisconnectsAndCancelsPendingWork)
{
    auto entered = std::make_shared<std::promise<void>>();
    auto release = std::make_shared<std::promise<void>>();
    auto released = release->get_future().share();
    ASSERT_TRUE(m_transport
            ->execute([entered, released] {
                entered->set_value();
                released.wait_for(3s);
                return Result<void>::ok();
            })
            .success());
    ASSERT_EQ(entered->get_future().wait_for(2s), std::future_status::ready);
    std::atomic<int> executed{0};
    bool rejected = false;
    for (usize index = 0; index <= transport::TcpTransport::MAX_PENDING_TASKS; ++index) {
        auto result = m_transport->execute([&] {
            ++executed;
            return Result<void>::ok();
        });
        if (!result.success()) {
            rejected = true;
            break;
        }
    }
    release->set_value();
    EXPECT_TRUE(rejected);
    m_transport->synchronize();
    EXPECT_FALSE(m_transport->isConnected());
    EXPECT_EQ(executed.load(), 0);
}

TEST_F(ConnectionWireAsyncTest, EncodedByteLimitDisconnectsSlowPeer)
{
    std::vector<u8> bytes(1024 * 1024, 17);
    // 整段在同一个串行任务内入队，写入完成回调尚无机会释放额度。
    EXPECT_TRUE(m_transport
            ->execute([this, &bytes] {
                for (usize index = 0; index <= transport::TcpTransport::MAX_PENDING_WRITE_BYTES / bytes.size();
                    ++index) {
                    auto result =
                        m_transport->send(bytes.data(), bytes.size(), transport::DeliveryHint::ReliableOrdered);
                    if (!result.success()) return Result<void>::ok();
                }
                return Result<void>::ok();
            })
            .success());
    m_transport->synchronize();
    EXPECT_FALSE(m_transport->isConnected());
    EXPECT_EQ(m_transport->pendingWriteBytes(), 0u);
}

TEST(OutboundPacketTest, BroadcastCachesFramesPerEncodingConfiguration)
{
    pipeline::OutboundPacket packet({Phase::Play, ir::PlayPacket{ir::play::KeepAlive{42}}});
    int encodes = 0;
    int tableIdentity = 0;
    auto encode = [&]() -> Result<std::vector<u8>> {
        ++encodes;
        return std::vector<u8>{1, 2, 3};
    };
    auto first = packet.prepare(&tableIdentity, protocol::PacketFlow::Clientbound, 256, encode);
    auto repeated = packet.prepare(&tableIdentity, protocol::PacketFlow::Clientbound, 256, encode);
    ASSERT_TRUE(first.success());
    ASSERT_TRUE(repeated.success());
    EXPECT_EQ(first.value(), repeated.value());
    EXPECT_EQ(encodes, 1);
    EXPECT_TRUE(packet.prepare(&tableIdentity, protocol::PacketFlow::Clientbound, -1, encode).success());
    EXPECT_TRUE(packet.prepare(&tableIdentity, protocol::PacketFlow::Serverbound, 256, encode).success());
    EXPECT_EQ(encodes, 3);
}

TEST(OutboundPacketTest, SharedBlockEntityNbtIsDetachedBeforeEnqueue)
{
    auto tag = std::make_shared<nbt::CompoundTag>();
    tag->put("value", 7);
    ir::play::BlockEntityData data{};
    data.tag = tag;
    pipeline::OutboundPacket packet({Phase::Play, ir::PlayPacket{data}});
    tag->put("value", 9);
    const auto& frozen = std::get<ir::play::BlockEntityData>(std::get<ir::PlayPacket>(packet.packet().packet));
    EXPECT_NE(frozen.tag, tag);
    EXPECT_EQ(frozen.tag->get<nbt::tags::int_tag>("value"), 7);
}

} // namespace
} // namespace mc::network::test
