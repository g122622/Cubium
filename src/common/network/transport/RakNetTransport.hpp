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

namespace mc::network::transport {

/**
 * @brief 基岩版 RakNet（UDP）传输 stub
 *
 * 基岩版基于 UDP 的 RakNet 协议：可靠有序靠 ACK/重传/排序 channel 实现，握手走
 * Offline(OpenConnectionRequest) → Online(ConnectionRequest with JWT) 双阶段。
 *
 * 本 stub 仅声明接口骨架，所有操作返回 NotImplemented 错误，不实现。基岩后端落地时
 * 接入 RakNet 实现（自研或引入第三方库）。
 *
 * TODO(bedrock): 实现 RakNet 帧编解码（GamePacket 0xfe + subclient）+ Offline/Online 握手。
 */
class RakNetTransport final : public ITransport {
public:
    RakNetTransport() = default;
    ~RakNetTransport() override = default;

    /**
     * @brief 基岩版连接（RakNet Offline→Online 握手）
     *
     * TODO(bedrock): 实现。当前返回 NotImplemented。
     */
    [[nodiscard]] Result<void> connect(const Endpoint& /*endpoint*/)
    {
        return Error(ErrorCode::NotInitialized, "RakNet transport not implemented", "RakNetTransport::connect");
    }

    // === ITransport（全部 stub，返回 NotImplemented）===
    [[nodiscard]] Result<void> send(const u8* /*data*/, usize /*size*/, DeliveryHint /*hint*/) override
    {
        return Error(ErrorCode::NotInitialized, "RakNet transport not implemented", "RakNetTransport::send");
    }

    void onBytes(ByteCallback /*callback*/) override
    {
        // TODO(bedrock): 注册字节回调
    }

    void onDisconnect(DisconnectCallback /*callback*/) override
    {
        // TODO(bedrock): 注册断开回调
    }

    [[nodiscard]] bool isConnected() const noexcept override { return false; }

    void close() override
    {
        // TODO(bedrock): 关闭 RakNet peer
    }
};

} // namespace mc::network::transport
