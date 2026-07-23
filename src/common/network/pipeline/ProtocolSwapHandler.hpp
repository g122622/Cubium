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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OF OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "common/network/ir/IrPacket.hpp"
#include "common/network/pipeline/ProtocolTableSet.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"

#include <type_traits>

namespace mc::network::pipeline {

/**
 * @brief terminal 包特征：struct 含 static constexpr bool kTerminal
 *
 * 对应 MC Java Packet::isTerminal()。terminal 包处理后触发阶段切换。
 * 用 SFINAE 探测 kTerminal 成员，无则视为非 terminal（默认 false）。
 */
template <typename T, typename = void>
struct IsTerminal : std::false_type {};

template <typename T>
struct IsTerminal<T, std::void_t<decltype(T::kTerminal)>> : std::bool_constant<T::kTerminal> {};

template <typename T>
inline constexpr bool kIsTerminalV = IsTerminal<T>::value;

/**
 * @brief 协议交换处理器（terminal 包驱动阶段切换）
 *
 * 对应 MC Java Connection 在收到/发出 terminal 包时 setupOutboundProtocol/InboundProtocol。
 *
 * 状态机阶段切换规则：
 * - Handshaking 阶段 ClientIntention：按 intendedState（1=Status, 2=Login）切。
 * - Login 阶段 LoginAcknowledged/LoginFinished：切到 Configuration。
 * - Configuration 阶段 FinishConfiguration：切到 Play。
 *
 * 本类无状态，全是静态判定函数，供 Connection 在编解码后调用决定是否切阶段。
 */
class ProtocolSwapHandler {
public:
    /**
     * @brief 判定一个 IrPacket 是否为 terminal 包，并给出下一阶段
     *
     * 非 terminal 返回 {false, 当前阶段}；terminal 返回 {true, 下一阶段}。
     * 握手阶段按 ClientIntention.intendedState 决定去 Status 还是 Login。
     */
    [[nodiscard]] static TerminalCheck check(const ir::IrPacket& packet, protocol::PacketFlow flow);

private:
    [[nodiscard]] static protocol::ConnectionProtocol nextForTerminal(
        const ir::IrPacket& packet, protocol::PacketFlow flow);
};

} // namespace mc::network::pipeline
