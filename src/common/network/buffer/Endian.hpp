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

namespace mc::network::buffer {

/**
 * @brief 大端序网络字节序工具
 *
 * Java 版线协议全程大端。本工具迁移自旧 PacketSerializer 的 NetworkEndian，
 * 供 buffer/ByteBuf 内部及 codec 层按需使用。基岩版（小端）不使用本类。
 */
struct Endian {
    [[nodiscard]] static u16 swap16(u16 value) noexcept;
    [[nodiscard]] static u32 swap32(u32 value) noexcept;
    [[nodiscard]] static u64 swap64(u64 value) noexcept;

    [[nodiscard]] static u16 hostToNetwork16(u16 value) noexcept { return swap16(value); }
    [[nodiscard]] static u32 hostToNetwork32(u32 value) noexcept { return swap32(value); }
    [[nodiscard]] static u64 hostToNetwork64(u64 value) noexcept { return swap64(value); }
    [[nodiscard]] static u16 networkToHost16(u16 value) noexcept { return swap16(value); }
    [[nodiscard]] static u32 networkToHost32(u32 value) noexcept { return swap32(value); }
    [[nodiscard]] static u64 networkToHost64(u64 value) noexcept { return swap64(value); }
};

} // namespace mc::network::buffer
