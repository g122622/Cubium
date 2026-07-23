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

#include "common/network/buffer/Endian.hpp"

namespace mc::network::buffer {

u16 Endian::swap16(u16 value) noexcept
{
    return static_cast<u16>((value >> 8) | (value << 8));
}

u32 Endian::swap32(u32 value) noexcept
{
    return ((value >> 24) & 0x000000FFu) | ((value >> 8) & 0x0000FF00u) | ((value << 8) & 0x00FF0000u) |
        ((value << 24) & 0xFF000000u);
}

u64 Endian::swap64(u64 value) noexcept
{
    return ((value >> 56) & 0x00000000000000FFu) | ((value >> 40) & 0x000000000000FF00u) |
        ((value >> 24) & 0x0000000000FF0000u) | ((value >> 8) & 0x00000000FF000000u) |
        ((value << 8) & 0x000000FF00000000u) | ((value << 24) & 0x0000FF0000000000u) |
        ((value << 40) & 0x00FF000000000000u) | ((value << 56) & 0xFF00000000000000u);
}

} // namespace mc::network::buffer
