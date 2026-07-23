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

namespace mc::network::protocol {

/**
 * @brief 包流向（对应 Java PacketFlow）
 *
 * Serverbound = 客户端发往服务端；Clientbound = 服务端发往客户端。
 * 每个 (阶段, 流向) 对应一张包表（ProtocolInfo）。
 */
enum class PacketFlow : u8 {
    Serverbound = 0,
    Clientbound = 1,
};

[[nodiscard]] constexpr bool isServerbound(PacketFlow flow) noexcept
{
    return flow == PacketFlow::Serverbound;
}
[[nodiscard]] constexpr bool isClientbound(PacketFlow flow) noexcept
{
    return flow == PacketFlow::Clientbound;
}

} // namespace mc::network::protocol
