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
 * @brief 字节传输抽象接口（消息边界感知）
 *
 * 上层 Connection 经本接口收发字节消息，不感知 TCP/UDP/Local 差异：
 * - send：投递一条完整消息（帧已由实现负责），DeliveryHint 仅 RakNet 用。
 * - onMessage：实现收到完整消息后回调（帧已解包）。
 * - onDisconnect：连接断开回调。
 *
 * 注意：本接口是 Wire 模式（字节传输）的统一抽象。Local 模式（同进程零拷贝直传
 * ir::Packet）走独立的 ILocalTransport，不经本接口序列化——见 LocalTransport.hpp。
 */
class ITransport {
public:
    using MessageCallback = std::function<void(const u8* data, usize size)>;
    using DisconnectCallback = std::function<void()>;

    virtual ~ITransport() = default;

    /**
     * @brief 投递一条消息字节
     *
     * @return 失败返回错误（如未连接、底层写失败）
     */
    [[nodiscard]] virtual Result<void> send(const u8* data, usize size, DeliveryHint hint) = 0;

    /**
     * @brief 投递一条消息字节（vector 便利重载）
     */
    [[nodiscard]] Result<void> send(const std::vector<u8>& data, DeliveryHint hint)
    {
        return send(data.data(), data.size(), hint);
    }

    /**
     * @brief 注册消息到达回调
     */
    virtual void onMessage(MessageCallback callback) = 0;

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
