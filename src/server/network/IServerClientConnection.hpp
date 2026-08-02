/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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
#include "common/network/ir/IrPacket.hpp"

#include <string>

namespace mc::server::net {

/**
 * @brief 服务端单客户端连接接口（仅暴露业务侧所需的最小出站/断连语义）
 *
 * ServerPlayerData / PlayerManager / ConnectionManager 等业务层只关心"发包 +
 * 断连 + 是否在线"三件事，不需要也不应触及握手编排、入站队列、压缩/加密装配等
 * pipeline 细节。本接口把这部分依赖收窄到最小，使：
 *   - 生产实现 ServerClientConnection 持有完整 pipeline::Connection 并实现本接口；
 *   - 测试桩 FakeServerConnection 仅记录发送字节/断连原因即可实现本接口，
 *     不必构造真实 transport/协议表。
 *
 * disconnect(reason) 的语义对齐 MC Java 的
 * ServerPlayer.connection.disconnect(Component)：先发当前阶段的 Clientbound
 * Disconnect 包（reason 包成 JSON 文本组件），再关闭底层连接。close() 则是
 * 无理由直接断连（用于服务端关闭、对端已失联等无需告知原因的场景）。
 */
class IServerClientConnection {
public:
    virtual ~IServerClientConnection() = default;

    /// 出站发送一个 IR 包（实现负责按当前阶段编码并投递 transport）。
    [[nodiscard]] virtual Result<void> send(mc::network::ir::IrPacket packet) = 0;

    /// 仅断开底层连接，不发送 Disconnect 包。幂等。
    virtual void close() = 0;

    /// 底层连接是否仍在线（已 close 或对端失联返回 false）。
    [[nodiscard]] virtual bool isConnected() const noexcept = 0;

    /**
     * @brief 取对端网络地址（"host:port" 形式）。
     *
     * Wire 模式从底层 TCP socket remote_endpoint 取；Local 模式（集成服本地客户端）
     * 无网络对端，返回空串。供 PlayerManager 记录玩家 IP（BanIp 命令按 IP 踢人/封禁）用。
     * 已断开时返回空串。
     */
    [[nodiscard]] virtual std::string peerAddress() const = 0;

    /**
     * @brief 发送带原因的 Disconnect 包后断开连接（对齐 MC Java
     *        ServerCommonConnection.disconnect(Component reason)）。
     *
     * reason 为纯文本，实现负责包成 JSON 文本组件（Phase6 后可改用完整组件）。
     * 已断开时调用为幂等 no-op。
     * @param reason 踢出/断连原因（纯文本）
     */
    virtual void disconnect(const std::string& reason) = 0;
};

} // namespace mc::server::net
