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

#pragma once

#include "common/core/Result.hpp"
#include "common/network/transport/Endpoint.hpp"
#include "common/network/transport/ITransport.hpp"

#include <asio.hpp>

#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace mc::network::transport {

/**
 * @brief TCP 原始字节传输
 *
 * 仅做同步 TCP socket 读写，不做任何帧编解码——帧编解码（VarInt 长度前缀）、
 * 压缩、加密都由 Connection 流水线负责。本类只把上层 send 的字节写进 socket，
 * 把 socket 读到的字节块回调给 onBytes。
 *
 * 线程模型沿用旧 NetworkClient 的成熟模式（已验证可工作）：
 * - 同步 connect（asio::connect）
 * - 独立接收线程跑同步 read_some 循环，每读到一块就回调 onBytes
 * - 发送在调用方线程同步 asio::write，mutex 保护并发 send
 * io_context 仅作 resolver/socket 工厂，不调 run()。
 *
 * TODO(Phase7): 若需高并发可改 io_context.run() + async_read/write；当前同步模型
 *              足以支撑我方互通必达目标。
 */
class TcpTransport : public ITransport {
public:
    TcpTransport();
    ~TcpTransport() override;

    TcpTransport(const TcpTransport&) = delete;
    TcpTransport& operator=(const TcpTransport&) = delete;

    /**
     * @brief 客户端：连接到 endpoint
     */
    [[nodiscard]] Result<void> connect(const Endpoint& endpoint);

    /**
     * @brief 服务端：从已 accept 的 socket 接管（ServerNetwork 创建后注入）
     *
     * TODO(Phase7): ServerNetwork accept 出 socket 后用本接口注入，省去 TcpTransport 自行 accept。
     */
    void attachConnectedSocket(asio::ip::tcp::socket socket);

    // === ITransport ===
    [[nodiscard]] Result<void> send(const u8* data, usize size, DeliveryHint hint) override;
    void onBytes(ByteCallback callback) override;
    void onDisconnect(DisconnectCallback callback) override;
    [[nodiscard]] bool isConnected() const noexcept override;
    void close() override;

private:
    /**
     * @brief 接收线程主循环：同步读字节块，回调 onBytes
     */
    void _receiveLoop();

    asio::io_context m_ioContext;
    std::unique_ptr<asio::ip::tcp::socket> m_socket;
    std::unique_ptr<std::thread> m_receiveThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};

    ByteCallback m_byteCallback;
    DisconnectCallback m_disconnectCallback;
    std::mutex m_callbackMutex;

    std::mutex m_sendMutex; // 保护并发 send 的同步 write
};

} // namespace mc::network::transport
