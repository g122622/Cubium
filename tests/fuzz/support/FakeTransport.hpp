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
#include "common/network/transport/DeliveryHint.hpp"
#include "common/network/transport/ITransport.hpp"

#include <utility>
#include <vector>

namespace mc::fuzz {

/**
 * @brief 进程内伪传输：把 Connection 的流水线当成接在真实 socket 上那样驱动
 *
 * 用途：给入站流水线（解密 → 切帧 → 解压 → 解码 → 阶段切换）提供可控的字节到达，
 * 而不引入任何 socket、线程或真实对端。
 *
 * 与真实传输的对应关系：
 *   - `feed()` 相当于 socket 读到一块数据（块边界由调用方决定，因此可精确模拟 TCP 的
 *     任意切分：粘包、半包、跨帧残留）；
 *   - `send()` 记录上层写出的字节（供断言或差分对比使用）；
 *   - `close()` 触发断开回调，与 TcpTransport 的语义一致。
 */
class FakeTransport final : public network::transport::ITransport {
public:
    [[nodiscard]] Result<void> send(const u8* data, usize size, network::transport::DeliveryHint hint) override
    {
        (void)hint;
        if (!m_connected) {
            return Error(ErrorCode::InvalidState, "FakeTransport is closed", "FakeTransport::send");
        }
        if (data != nullptr && size > 0) {
            m_sent.insert(m_sent.end(), data, data + size);
        }
        return Result<void>::ok();
    }

    void onBytes(ByteCallback callback) override { m_onBytes = std::move(callback); }

    void onDisconnect(DisconnectCallback callback) override { m_onDisconnect = std::move(callback); }

    [[nodiscard]] bool isConnected() const noexcept override { return m_connected; }

    void close() override
    {
        if (!m_connected) {
            return;
        }
        m_connected = false;
        if (m_onDisconnect) {
            m_onDisconnect();
        }
    }

    /**
     * @brief 模拟对端投递一段字节（等价于 socket 读到一块数据）
     *
     * 已关闭或无接收回调时不投递——与真实传输在断开后不再回调的行为一致。
     */
    void feed(const u8* data, usize size)
    {
        if (!m_connected || m_onBytes == nullptr || data == nullptr || size == 0) {
            return;
        }
        m_onBytes(data, size);
    }

    /// 上层经本传输写出的全部字节（按写入顺序累积）
    [[nodiscard]] const std::vector<u8>& sentBytes() const noexcept { return m_sent; }

private:
    ByteCallback m_onBytes;
    DisconnectCallback m_onDisconnect;
    std::vector<u8> m_sent;
    bool m_connected = true;
};

} // namespace mc::fuzz
