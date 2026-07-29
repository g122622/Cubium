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

#include "common/network/transport/TcpTransport.hpp"

#include <asio.hpp>

namespace mc::network::transport {

TcpTransport::TcpTransport() = default;

TcpTransport::~TcpTransport()
{
    close();
}

Result<void> TcpTransport::connect(const Endpoint& endpoint)
{
    try {
        m_socket = std::make_unique<asio::ip::tcp::socket>(m_ioContext);
        asio::ip::tcp::resolver resolver(m_ioContext);
        const auto results = resolver.resolve(endpoint.host, std::to_string(endpoint.port));
        asio::connect(*m_socket, results);

        m_running = true;
        m_connected = true;
        m_receiveThread = std::make_unique<std::thread>([this]() { _receiveLoop(); });
        return Result<void>::ok();
    }
    catch (const std::exception& e) {
        m_socket.reset();
        return Error(ErrorCode::ConnectionFailed, std::string("TCP connect failed: ") + e.what(), "TcpTransport::connect");
    }
}

void TcpTransport::attachConnectedSocket(asio::ip::tcp::socket socket)
{
    m_socket = std::make_unique<asio::ip::tcp::socket>(std::move(socket));
    m_running = true;
    m_connected = true;
    m_receiveThread = std::make_unique<std::thread>([this]() { _receiveLoop(); });
}

Result<void> TcpTransport::send(const u8* data, usize size, DeliveryHint /*hint*/)
{
    if (!isConnected() || m_socket == nullptr) {
        return Error(ErrorCode::InvalidState, "TCP not connected", "TcpTransport::send");
    }
    if (data == nullptr || size == 0) {
        return Result<void>::ok();
    }

    std::lock_guard<std::mutex> lock(m_sendMutex);
    try {
        asio::write(*m_socket, asio::buffer(data, size));
        return Result<void>::ok();
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::ConnectionFailed, std::string("TCP write failed: ") + e.what(), "TcpTransport::send");
    }
}

void TcpTransport::onBytes(ByteCallback callback)
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_byteCallback = std::move(callback);
}

void TcpTransport::onDisconnect(DisconnectCallback callback)
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_disconnectCallback = std::move(callback);
}

bool TcpTransport::isConnected() const noexcept
{
    return m_connected;
}

void TcpTransport::close()
{
    if (!m_running.exchange(false)) {
        return;
    }
    m_connected = false;

    if (m_socket != nullptr) {
        asio::error_code ignored;
        m_socket->shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
        m_socket->close(ignored);
    }
    if (m_receiveThread != nullptr && m_receiveThread->joinable()) {
        m_receiveThread->join();
    }
    m_socket.reset();
    m_receiveThread.reset();

    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        if (m_disconnectCallback) {
            m_disconnectCallback();
            m_disconnectCallback = nullptr;
        }
    }
}

void TcpTransport::_receiveLoop()
{
    constexpr usize kChunkSize = 4096;
    u8 chunk[kChunkSize];

    while (m_running) {
        asio::error_code ec;
        const usize bytesRead = m_socket->read_some(asio::buffer(chunk, kChunkSize), ec);
        if (ec) {
            // 连接关闭或出错
            break;
        }
        if (bytesRead == 0) {
            continue;
        }

        ByteCallback callback;
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            callback = m_byteCallback;
        }
        if (callback) {
            callback(chunk, bytesRead);
        }
    }

    m_connected = false;
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        if (m_disconnectCallback) {
            m_disconnectCallback();
            m_disconnectCallback = nullptr;
        }
    }
}

} // namespace mc::network::transport
