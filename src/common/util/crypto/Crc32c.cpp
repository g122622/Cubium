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

#include "common/util/crypto/Crc32c.hpp"
#include "common/core/Types.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <string_view>

namespace mc {
namespace util {
namespace crypto {

namespace {

/// 反射一个 32 位值（bit 反转），用于生成反射 CRC 表。
[[nodiscard]] constexpr u32 reflect32(u32 x) noexcept
{
    u32 r = 0;
    for (std::size_t i = 0; i < 32; ++i) {
        r = (r << 1) | (x & 1u);
        x >>= 1;
    }
    return r;
}

/// 生成 CRC-32C 反射查表（Castagnoli 多项式 0x1EDC6F41，反射后 0x82F63B78）。
[[nodiscard]] constexpr std::array<u32, 256> buildTable() noexcept
{
    std::array<u32, 256> t{};
    constexpr u32 kPolynomial = 0x82F63B78u; // reflected 0x1EDC6F41
    for (std::size_t i = 0; i < 256; ++i) {
        u32 c = static_cast<u32>(i);
        for (std::size_t k = 0; k < 8; ++k) {
            c = (c & 1u) ? (kPolynomial ^ (c >> 1)) : (c >> 1);
        }
        t[i] = c;
    }
    return t;
}

constexpr std::array<u32, 256> kTable = buildTable();

// 抑制未使用函数告警（reflect32 仅作文档保留，便于核对反射多项式）。
[[maybe_unused]] constexpr u32 kReflectedPolynomial = reflect32(0x1EDC6F41u);

} // namespace

const std::array<u32, 256> Crc32c::TABLE = kTable;

u32 Crc32c::hash(std::span<const u8> data)
{
    u32 crc = 0xFFFFFFFFu;
    for (const u8 b : data) {
        crc = kTable[(crc ^ b) & 0xFFu] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

u32 Crc32c::hash(std::string_view str)
{
    return hash(std::span<const u8>(reinterpret_cast<const u8*>(str.data()), str.size()));
}

} // namespace crypto
} // namespace util
} // namespace mc
