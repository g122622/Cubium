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

/**
 * @file Md5.cpp
 * @brief MD5 哈希算法实现
 *
 * 提供 RFC 1321 标准的 MD5 哈希计算功能。
 */

#include "Md5.hpp"
#include "common/core/Types.hpp"
#include <array>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mc {
namespace util {
namespace crypto {

// MD5 轮常量表（每轮使用的正弦函数值）
// T[i] = floor(2^32 * abs(sin(i + 1)))，i = 0..63
static constexpr std::array<u32, 64> T = {0xd76aa478,
    0xe8c7b756,
    0x242070db,
    0xc1bdceee,
    0xf57c0faf,
    0x4787c62a,
    0xa8304613,
    0xfd469501,
    0x698098d8,
    0x8b44f7af,
    0xffff5bb1,
    0x895cd7be,
    0x6b901122,
    0xfd987193,
    0xa679438e,
    0x49b40821,
    0xf61e2562,
    0xc040b340,
    0x265e5a51,
    0xe9b6c7aa,
    0xd62f105d,
    0x02441453,
    0xd8a1e681,
    0xe7d3fbc8,
    0x21e1cde6,
    0xc33707d6,
    0xf4d50d87,
    0x455a14ed,
    0xa9e3e905,
    0xfcefa3f8,
    0x676f02d9,
    0x8d2a4c8a,
    0xfffa3942,
    0x8771f681,
    0x6d9d6122,
    0xfde5380c,
    0xa4beea44,
    0x4bdecfa9,
    0xf6bb4b60,
    0xbebfbc70,
    0x289b7ec6,
    0xeaa127fa,
    0xd4ef3085,
    0x04881d05,
    0xd9d4d039,
    0xe6db99e5,
    0x1fa27cf8,
    0xc4ac5665,
    0xf4292244,
    0x432aff97,
    0xab9423a7,
    0xfc93a039,
    0x655b59c3,
    0x8f0ccc92,
    0xffeff47d,
    0x85845dd1,
    0x6fa87e4f,
    0xfe2ce6e0,
    0xa3014314,
    0x4e0811a1,
    0xf7537e82,
    0xbd3af235,
    0x2ad7d2bb,
    0xeb86d391};

// 每轮的位移量
static constexpr std::array<std::size_t, 64> SHIFT = {7,
    12,
    17,
    22,
    7,
    12,
    17,
    22,
    7,
    12,
    17,
    22,
    7,
    12,
    17,
    22,
    5,
    9,
    14,
    20,
    5,
    9,
    14,
    20,
    5,
    9,
    14,
    20,
    5,
    9,
    14,
    20,
    4,
    11,
    16,
    23,
    4,
    11,
    16,
    23,
    4,
    11,
    16,
    23,
    4,
    11,
    16,
    23,
    6,
    10,
    15,
    21,
    6,
    10,
    15,
    21,
    6,
    10,
    15,
    21,
    6,
    10,
    15,
    21};

Md5::Digest Md5::hash(std::span<const u8> data)
{
    std::vector<u8> padded = padMessage(data);

    std::array<u32, 4> state = INITIAL_HASH;

    // 处理每个 512 位块
    for (std::size_t i = 0; i < padded.size(); i += BLOCK_SIZE) {
        processBlock(&padded[i], state);
    }

    // 将状态转换为字节（小端序）
    Digest result;
    for (std::size_t i = 0; i < 4; ++i) {
        result[i * 4 + 0] = static_cast<u8>((state[i] >> 0) & 0xFF);
        result[i * 4 + 1] = static_cast<u8>((state[i] >> 8) & 0xFF);
        result[i * 4 + 2] = static_cast<u8>((state[i] >> 16) & 0xFF);
        result[i * 4 + 3] = static_cast<u8>((state[i] >> 24) & 0xFF);
    }

    return result;
}

Md5::Digest Md5::hash(std::string_view str)
{
    return hash(std::span<const u8>(reinterpret_cast<const u8*>(str.data()), str.size()));
}

std::string Md5::toHexString(const Digest& digest)
{
    static constexpr char hexChars[] = "0123456789abcdef";
    std::string result;
    result.reserve(DIGEST_SIZE * 2);
    for (const u8 byte : digest) {
        result.push_back(hexChars[(byte >> 4) & 0x0F]);
        result.push_back(hexChars[byte & 0x0F]);
    }
    return result;
}

std::vector<u8> Md5::padMessage(std::span<const u8> message)
{
    std::vector<u8> padded(message.begin(), message.end());

    // 追加 1 位（0x80）
    padded.push_back(0x80);

    // 填充 0 直到长度 ≡ 56 (mod 64)
    while (padded.size() % BLOCK_SIZE != 56) {
        padded.push_back(0x00);
    }

    // 追加原始长度（64 位小端序）
    const u64 bitLength = static_cast<u64>(message.size()) * 8;
    for (std::size_t i = 0; i < 8; ++i) {
        padded.push_back(static_cast<u8>((bitLength >> (i * 8)) & 0xFF));
    }

    return padded;
}

void Md5::processBlock(const u8* block, std::array<u32, 4>& state)
{
    // 将块解析为 16 个 32 位小端序字
    std::array<u32, 16> M;
    for (std::size_t i = 0; i < 16; ++i) {
        M[i] = static_cast<u32>(block[i * 4 + 0]) | (static_cast<u32>(block[i * 4 + 1]) << 8) |
            (static_cast<u32>(block[i * 4 + 2]) << 16) | (static_cast<u32>(block[i * 4 + 3]) << 24);
    }

    u32 a = state[0];
    u32 b = state[1];
    u32 c = state[2];
    u32 d = state[3];

    // 第 1 轮 (0-15): F, K[i], M[k], s[i]
    // k = i
    for (std::size_t i = 0; i < 16; ++i) {
        u32 temp = b + rotl(a + f(b, c, d) + M[i] + T[i], SHIFT[i]);
        a = d;
        d = c;
        c = b;
        b = temp;
    }

    // 第 2 轮 (16-31): G, K[i], M[k], s[i]
    // k = (5*i + 1) mod 16
    for (std::size_t i = 16; i < 32; ++i) {
        std::size_t k = (5 * i + 1) % 16;
        u32 temp = b + rotl(a + g(b, c, d) + M[k] + T[i], SHIFT[i]);
        a = d;
        d = c;
        c = b;
        b = temp;
    }

    // 第 3 轮 (32-47): H, K[i], M[k], s[i]
    // k = (3*i + 5) mod 16
    for (std::size_t i = 32; i < 48; ++i) {
        std::size_t k = (3 * i + 5) % 16;
        u32 temp = b + rotl(a + h(b, c, d) + M[k] + T[i], SHIFT[i]);
        a = d;
        d = c;
        c = b;
        b = temp;
    }

    // 第 4 轮 (48-63): I, K[i], M[k], s[i]
    // k = (7*i) mod 16
    for (std::size_t round = 48; round < 64; ++round) {
        std::size_t k = (7 * round) % 16;
        u32 temp = b + rotl(a + i(b, c, d) + M[k] + T[round], SHIFT[round]);
        a = d;
        d = c;
        c = b;
        b = temp;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

} // namespace crypto
} // namespace util
} // namespace mc
