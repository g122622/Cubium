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

namespace {

/**
 * @brief 把 VarInt 编码进字节缓冲（与 ByteBuf::writeVarUInt 一致的实现，避免环依赖 buffer）
 *
 * 帧长度前缀用 VarInt 编码，传输层独立实现避免引入 buffer 依赖。
 */
void writeVarUInt(std::vector<u8>& out, u32 value)
{
    while (true) {
        if ((value & ~static_cast<u32>(0x7F)) == 0) {
            out.push_back(static_cast<u8>(value));
            return;
        }
        out.push_back(static_cast<u8>((value & 0x7Fu) | 0x80u));
        value >>= 7;
    }
}

/**
 * @brief 从字节缓冲读 VarInt，返回 (值, 消费字节数)；不完整返回 false
 */
[[nodiscard]] bool tryReadVarUInt(const u8* data, usize size, u32& outValue, usize& outConsumed)
{
    u32 value = 0;
    for (usize i = 0; i < 5 && i < size; ++i) {
        const u8 byte = data[i];
        value |= static_cast<u32>(byte & 0x7Fu) << (7 * i);
        if ((byte & 0x80u) == 0) {
            outValue = value;
            outConsumed = i + 1;
            return true;
        }
    }
    return false; // 数据不足或超过 5 字节
}

} // namespace

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
        return Error(ErrorCode::ConnectionFailed, std::string("TCP 连接失败: ") + e.what(), "TcpTransport::connect");
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
        return Error(ErrorCode::InvalidState, "TCP 未连接", "TcpTransport::send");
    }

    // 帧 = VarInt(长度) + payload
    std::vector<u8> frame;
    frame.reserve(size + 5);
    writeVarUInt(frame, static_cast<u32>(size));
    if (data != nullptr && size > 0) {
        frame.insert(frame.end(), data, data + size);
    }

    std::lock_guard<std::mutex> lock(m_sendMutex);
    try {
        asio::write(*m_socket, asio::buffer(frame.data(), frame.size()));
        return Result<void>::ok();
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::ConnectionFailed, std::string("TCP 写失败: ") + e.what(), "TcpTransport::send");
    }
}

void TcpTransport::onMessage(MessageCallback callback)
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_messageCallback = std::move(callback);
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

        {
            std::lock_guard<std::mutex> lock(m_receiveMutex);
            m_receiveBuffer.insert(m_receiveBuffer.end(), chunk, chunk + bytesRead);
        }
        _tryFrameMessages();
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

void TcpTransport::_tryFrameMessages()
{
    MessageCallback callback;
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        callback = m_messageCallback;
    }
    if (!callback) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_receiveMutex);
    usize offset = 0;
    while (offset < m_receiveBuffer.size()) {
        u32 frameLength = 0;
        usize varIntSize = 0;
        if (!tryReadVarUInt(
                m_receiveBuffer.data() + offset, m_receiveBuffer.size() - offset, frameLength, varIntSize)) {
            break; // 长度 VarInt 不完整，等更多数据
        }
        const usize totalFrameSize = varIntSize + frameLength;
        if (offset + totalFrameSize > m_receiveBuffer.size()) {
            break; // payload 不完整，等更多数据
        }

        const u8* payload = m_receiveBuffer.data() + offset + varIntSize;
        callback(payload, frameLength);
        offset += totalFrameSize;
    }

    if (offset > 0) {
        // 移除已消费的字节，保留残留
        m_receiveBuffer.erase(m_receiveBuffer.begin(), m_receiveBuffer.begin() + static_cast<std::ptrdiff_t>(offset));
    }
}

} // namespace mc::network::transport
