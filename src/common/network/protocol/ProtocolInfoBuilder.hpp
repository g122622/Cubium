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

#include "common/network/codec/StreamCodec.hpp"
#include "common/network/protocol/PacketType.hpp"
#include "common/network/protocol/ProtocolInfo.hpp"

#include <memory>
#include <variant>
#include <vector>

namespace mc::network::protocol {

/**
 * @brief 构建一个 (阶段, 流向) 的 ProtocolInfo 包表
 *
 * 链式 addPacket：每包传入 PacketType + 该包的 codec + 该包在 Variant 中的备选项下标。
 * addPacket 顺序即 packet id（0 起递增）。对齐 Java GameProtocols 的注册顺序。
 *
 * @tparam B 缓冲类型
 * @tparam Variant 该阶段包变体
 */
template <typename B, typename Variant>
class ProtocolInfoBuilder {
public:
    ProtocolInfoBuilder(ConnectionProtocol phase, PacketFlow flow)
        : m_info(std::make_unique<ProtocolInfo<B, Variant>>(phase, flow))
    {}

    /**
     * @brief 登记一个包
     *
     * @param type 逻辑类型标识
     * @param altIndex 该包 struct 在 Variant 中的备选项下标（std::variant_alternative 索引）
     * @param codec 该包的 StreamCodec<B, PacketStruct>（按值持有）
     *
     * 调用方负责保证 altIndex 与 codec 的值类型一致。
     */
    template <typename PacketStruct, typename Codec>
    void addPacket(PacketType type, usize altIndex, Codec codec)
    {
        static_assert(
            std::is_base_of_v<codec::StreamCodec<B, PacketStruct>, Codec> || codec::CodecFor<Codec, B, PacketStruct>,
            "Codec 须满足 StreamCodec<B,PacketStruct> 接口或 CodecFor 概念");

        // matches：variant 当前下标 == altIndex
        auto matches = [altIndex](const Variant& value) { return value.index() == altIndex; };

        // codec 被 encode/decode 两个闭包共享，故存 shared_ptr。
        auto shared = std::make_shared<Codec>(std::move(codec));

        // encodePayload：取 variant 备选项指针交 codec.encode（下标由 matches 保证）。
        auto encodePayload = [shared](B& buf, const Variant& value) {
            const auto* ptr = std::get_if<PacketStruct>(&value);
            shared->encode(buf, *ptr);
        };

        // decode：codec.decode 出 PacketStruct，包成 Variant。
        auto decode = [shared](B& buf) -> Result<Variant> {
            PacketStruct value;
            MC_TRY_ASSIGN(value, shared->decode(buf));
            return Variant{std::move(value)};
        };

        m_info->dispatch().addPacket(std::move(matches), std::move(encodePayload), std::move(decode));
        m_types.push_back(std::move(type));
    }

    [[nodiscard]] std::unique_ptr<ProtocolInfo<B, Variant>> build() { return std::move(m_info); }

private:
    std::unique_ptr<ProtocolInfo<B, Variant>> m_info;
    std::vector<PacketType> m_types;
};

} // namespace mc::network::protocol
