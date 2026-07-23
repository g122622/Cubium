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
 * @brief TCP 字节传输 + Java VarInt21 长度前缀帧编解码
 *
 * 对应 Java 线协议的 Varint21FrameDecoder/Prepender：每条消息 = VarInt(长度) + payload。
 * 本类把帧编解码收敛到传输层，上层 ITransport::onMessage 收到的就是完整 payload。
 *
 * 线程模型沿用旧 NetworkClient 的成熟模式（已验证可工作）：
 * - 同步 connect（asio::connect）
 * - 独立接收线程跑同步 read_some 循环，按帧长度切分消息回调 onMessage
 * - 发送在调用方线程同步 asio::write，mutex 保护并发 send
 * io_context 仅作 resolver/socket 工厂，不调 run()。
 *
 * TODO(Phase2/Phase7): 若需高并发可改 io_context.run() + async_read/write；当前同步模型
 *                      足以支撑我方互通必达目标。
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
    void onMessage(MessageCallback callback) override;
    void onDisconnect(DisconnectCallback callback) override;
    [[nodiscard]] bool isConnected() const noexcept override;
    void close() override;

private:
    /**
     * @brief 接收线程主循环：同步读字节，按 VarInt 帧长度切分消息
     */
    void _receiveLoop();

    /**
     * @brief 尝试从接收缓冲切出完整帧，回调 onMessage
     */
    void _tryFrameMessages();

    asio::io_context m_ioContext;
    std::unique_ptr<asio::ip::tcp::socket> m_socket;
    std::unique_ptr<std::thread> m_receiveThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};

    std::vector<u8> m_receiveBuffer; // 已读入但尚未切帧的残留字节
    std::mutex m_receiveMutex;       // 保护 m_receiveBuffer（接收线程写，_tryFrameMessages 读）

    MessageCallback m_messageCallback;
    DisconnectCallback m_disconnectCallback;
    std::mutex m_callbackMutex;

    std::mutex m_sendMutex; // 保护并发 send 的同步 write
};

} // namespace mc::network::transport
