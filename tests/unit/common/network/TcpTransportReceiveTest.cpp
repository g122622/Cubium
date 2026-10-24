#include "common/network/transport/TcpTransport.hpp"
#include <array>
#include <chrono>
#include <future>
#include <mutex>
#include <thread>
#include <vector>
#include <asio.hpp>
#include <gtest/gtest.h>

namespace mc::network::transport {
namespace {

TEST(TcpTransportReceiveTest, EarlyBytesRemainAvailableUntilListenersAreReady)
{
    using namespace std::chrono_literals;
    asio::io_context context;
    asio::ip::tcp::acceptor acceptor(context, {asio::ip::tcp::v4(), 0});
    const auto port = acceptor.local_endpoint().port();
    const std::array<u8, 4> payload{1, 3, 5, 7};
    std::promise<void> sent;
    std::promise<void> finish;
    auto finishFuture = finish.get_future();
    std::jthread client([&] {
        asio::io_context clientContext;
        asio::ip::tcp::socket socket(clientContext);
        socket.connect({asio::ip::address_v4::loopback(), port});
        asio::write(socket, asio::buffer(payload));
        sent.set_value();
        finishFuture.wait();
    });
    TcpTransport transport;
    ASSERT_TRUE(transport.attachConnectedSocket(acceptor.accept()).success());
    sent.get_future().wait();
    // 给旧实现的接收线程消费首包的机会；新实现此时明确不读取 socket。
    std::this_thread::sleep_for(50ms);
    std::vector<u8> received;
    std::promise<void> complete;
    auto completeFuture = complete.get_future();
    transport.onBytes([&](const u8* bytes, usize size) {
        received.insert(received.end(), bytes, bytes + size);
        if (received.size() == payload.size()) {
            complete.set_value();
        }
    });
    const auto started = transport.startReceiving();
    EXPECT_TRUE(started.success());
    EXPECT_EQ(completeFuture.wait_for(2s), std::future_status::ready);
    transport.close();
    finish.set_value();
    EXPECT_EQ(received, (std::vector<u8>{1, 3, 5, 7}));
}

TEST(TcpTransportReceiveTest, ClosingBeforeReceiveStartupDoesNotWaitForAThread)
{
    using namespace std::chrono_literals;
    asio::io_context context;
    asio::ip::tcp::acceptor acceptor(context, {asio::ip::tcp::v4(), 0});
    asio::ip::tcp::socket peer(context);
    peer.connect({asio::ip::address_v4::loopback(), acceptor.local_endpoint().port()});
    TcpTransport transport;
    ASSERT_TRUE(transport.attachConnectedSocket(acceptor.accept()).success());
    EXPECT_FALSE(transport.startReceiving().success());
    const auto start = std::chrono::steady_clock::now();
    transport.close();
    EXPECT_LT(std::chrono::steady_clock::now() - start, 1s);
}

} // namespace
} // namespace mc::network::transport
