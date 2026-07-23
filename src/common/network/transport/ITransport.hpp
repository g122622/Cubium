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
#include "common/core/Types.hpp"
#include "common/network/transport/DeliveryHint.hpp"
#include "common/network/transport/Endpoint.hpp"

#include <functional>
#include <vector>

namespace mc::network::transport {

/**
 * @brief 字节传输抽象接口（原始字节流，无消息边界）
 *
 * 上层 Connection 经本接口收发原始字节，不感知 TCP/UDP/Local 差异：
 * - send：投递原始字节（帧编解码由 Connection 流水线负责，不在本层）。
 * - onBytes：实现收到任意字节块时回调（流式，无消息边界保证）。
 *   Connection 内部的 VarintFraming 负责按 VarInt 长度前缀切帧。
 * - onDisconnect：连接断开回调。
 *
 * 注意：本接口是 Wire 模式（字节传输）的统一抽象。Local 模式（同进程零拷贝直传
 * ir::Packet）走独立的 ILocalTransport，不经本接口序列化——见 LocalTransport.hpp。
 * 设计为原始字节流而非"完整消息"：TCP 是流无边界，分帧属 Java wire 格式细节，
 * 由 pipeline/VarintFraming 处理；RakNet 数据报天然有边界，每个数据报作为一次 onBytes。
 */
class ITransport {
public:
    using ByteCallback = std::function<void(const u8* data, usize size)>;
    using DisconnectCallback = std::function<void()>;

    virtual ~ITransport() = default;

    /**
     * @brief 投递原始字节（已由上层流水线完成帧编解码/压缩/加密）
     *
     * @return 失败返回错误（如未连接、底层写失败）
     */
    [[nodiscard]] virtual Result<void> send(const u8* data, usize size, DeliveryHint hint) = 0;

    /**
     * @brief 投递原始字节（vector 便利重载）
     */
    [[nodiscard]] Result<void> send(const std::vector<u8>& data, DeliveryHint hint)
    {
        return send(data.data(), data.size(), hint);
    }

    /**
     * @brief 注册字节到达回调（流式，多次调用，单次回调可能含部分帧或多个帧）
     */
    virtual void onBytes(ByteCallback callback) = 0;

    /**
     * @brief 注册断开回调
     */
    virtual void onDisconnect(DisconnectCallback callback) = 0;

    /**
     * @brief 当前是否已连接
     */
    [[nodiscard]] virtual bool isConnected() const noexcept = 0;

    /**
     * @brief 主动关闭
     */
    virtual void close() = 0;
};

} // namespace mc::network::transport
