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

#include "common/core/Types.hpp"
#include "common/network/ir/IrPacketBase.hpp"

#include <optional>
#include <string>

namespace mc::network::ir::handshake {

/**
 * @brief 客户端意图（C→S，握手阶段唯一包）
 *
 * 客户端告知服务端：协议版本、服务端主机名:端口、下一阶段意图（Status=1 查询 / Login=2 登录）。
 * 这是 terminal 包——处理后握手阶段结束，按 intendedState 切到 Status 或 Login。
 */
struct ClientIntention {
    static constexpr bool kTerminal = true;

    i32 protocolVersion; // Java 1.21.11 = 774
    std::string hostName;
    u16 port;
    i32 intendedState; // 1=Status, 2=Login

    /** 基岩预留：握手在基岩版走 RakNet offline/online 消息，不走此包，预留字段恒 nullopt */
    BedrockMeta bedrock{};
    [[nodiscard]] friend bool operator==(const ClientIntention&, const ClientIntention&) noexcept = default;
};

} // namespace mc::network::ir::handshake
