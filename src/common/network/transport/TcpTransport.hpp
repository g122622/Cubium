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

#include "common/network/transport/ITransport.hpp"

#include <memory>
#include <string>
#include <asio/ip/tcp.hpp>

namespace mc::network::transport {

/**
 * @brief 共享网络事件循环上的异步 TCP 传输。
 *
 * 每连接使用独立的串行执行器，编码任务、接收回调和写入队列不会并发访问协议状态。
 * execute/send 返回成功表示成功入队；异步错误通过断开回调报告。
 */
class TcpTransport final : public ITransport {
public:
    TcpTransport();
    ~TcpTransport() override;
    TcpTransport(const TcpTransport&) = delete;
    TcpTransport& operator=(const TcpTransport&) = delete;

    /** @brief 客户端同步建立连接，随后由共享事件循环收发。 */
    [[nodiscard]] Result<void> connect(const Endpoint& endpoint);
    /** @brief 接管已连接的 socket，并迁移到共享网络事件循环。 */
    [[nodiscard]] Result<void> attachConnectedSocket(asio::ip::tcp::socket socket);
    /** @brief 回调装配完成后开始异步接收。 */
    [[nodiscard]] Result<void> startReceiving();
    /** @brief 返回连接建立时保存的对端地址。 */
    [[nodiscard]] std::string remoteAddress() const;

    [[nodiscard]] Result<void> execute(Task task) override;
    void synchronize() override;
    void closeAfterFlush() override;
    [[nodiscard]] Result<void> send(const u8* data, usize size, DeliveryHint hint) override;
    void onBytes(ByteCallback callback) override;
    void onDisconnect(DisconnectCallback callback) override;
    [[nodiscard]] bool isConnected() const noexcept override;
    void close() override;

    /** @brief 获取尚未写完的字节数，用于监控和压力测试。 */
    [[nodiscard]] usize pendingWriteBytes() const noexcept;

    static constexpr usize MAX_PENDING_TASKS = 4096;
    static constexpr usize MAX_PENDING_WRITE_BYTES = 32 * 1024 * 1024;

private:
    struct State;
    std::shared_ptr<State> m_state;
};

} // namespace mc::network::transport
