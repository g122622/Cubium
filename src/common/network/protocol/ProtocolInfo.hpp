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
#include "common/network/codec/IdDispatchCodec.hpp"
#include "common/network/protocol/ConnectionProtocol.hpp"
#include "common/network/protocol/PacketFlow.hpp"
#include "common/network/protocol/PacketType.hpp"

namespace mc::network::protocol {

/**
 * @brief 一个 (阶段, 流向) 的包表（对应 Java ProtocolInfo）
 *
 * 持有一个 IdDispatchCodec（包表），描述某阶段某流向的所有包。Connection 在某阶段
 * 持有当前流向的 ProtocolInfo，编解码经它的 dispatch 完成。
 *
 * @tparam B 缓冲类型
 * @tparam Variant 该阶段的包变体（如 ir::PlayPacket = std::variant<...>）
 */
template <typename B, typename Variant>
class ProtocolInfo {
public:
    ProtocolInfo(ConnectionProtocol phase, PacketFlow flow)
        : m_phase(phase)
        , m_flow(flow)
    {}

    [[nodiscard]] ConnectionProtocol phase() const noexcept { return m_phase; }
    [[nodiscard]] PacketFlow flow() const noexcept { return m_flow; }

    /**
     * @brief 取包表（供 Connection 编解码调用）
     */
    [[nodiscard]] codec::IdDispatchCodec<B, Variant>& dispatch() noexcept { return m_dispatch; }
    [[nodiscard]] const codec::IdDispatchCodec<B, Variant>& dispatch() const noexcept { return m_dispatch; }

    /**
     * @brief 编码一个包（写 id + payload）
     */
    [[nodiscard]] Result<void> encode(B& buf, const Variant& value) const { return m_dispatch.encode(buf, value); }

    /**
     * @brief 解码一个包（读 id + payload）
     */
    [[nodiscard]] Result<Variant> decode(B& buf) const { return m_dispatch.decode(buf); }

private:
    ConnectionProtocol m_phase;
    PacketFlow m_flow;
    codec::IdDispatchCodec<B, Variant> m_dispatch;
};

} // namespace mc::network::protocol
