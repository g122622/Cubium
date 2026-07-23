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
#include "common/network/protocol/PacketFlow.hpp"

#include <cstdint>
#include <functional>
#include <string>

namespace mc::network::protocol {

/**
 * @brief 包的逻辑类型标识（对应 Java PacketType）
 *
 * 逻辑标识：由 (flow, id 字符串) 唯一确定。整数 packet id 隐式——由 IdDispatchCodec
 * 注册顺序分配，不在此处硬编码。Java 后端在 addPacket 时把 PacketType 关联到表内位置。
 */
struct PacketType {
    PacketFlow flow;
    std::string id; // 如 "client_intention"、"keep_alive"（逻辑名，非 wire id）

    [[nodiscard]] bool operator==(const PacketType& other) const noexcept
    {
        return flow == other.flow && id == other.id;
    }
    [[nodiscard]] bool operator!=(const PacketType& other) const noexcept { return !(*this == other); }
};

/**
 * @brief PacketType 的哈希（用于 unordered_map 索引）
 */
struct PacketTypeHash {
    [[nodiscard]] usize operator()(const PacketType& type) const noexcept
    {
        return std::hash<u8>{}(static_cast<u8>(type.flow)) ^ (std::hash<std::string>{}(type.id) << 1);
    }
};

} // namespace mc::network::protocol
