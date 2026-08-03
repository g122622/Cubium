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
 * @file Sha1.cpp
 * @brief SHA-1 哈希算法实现
 *
 * 提供符合 FIPS 180-4 标准的 SHA-1 哈希计算功能。
 */

#include "Sha1.hpp"
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

Sha1::Digest Sha1::hash(std::span<const u8> data)
{
    std::vector<u8> padded = padMessage(data);

    std::array<u32, 5> state = INITIAL_HASH;

    // 处理每个 512 位块
    for (std::size_t i = 0; i < padded.size(); i += BLOCK_SIZE) {
        processBlock(&padded[i], state);
    }

    // 将状态转换为字节（大端序）
    Digest result;
    for (std::size_t i = 0; i < 5; ++i) {
        result[i * 4 + 0] = static_cast<u8>((state[i] >> 24) & 0xFF);
        result[i * 4 + 1] = static_cast<u8>((state[i] >> 16) & 0xFF);
        result[i * 4 + 2] = static_cast<u8>((state[i] >> 8) & 0xFF);
        result[i * 4 + 3] = static_cast<u8>((state[i] >> 0) & 0xFF);
    }

    return result;
}

Sha1::Digest Sha1::hash(std::string_view str)
{
    return hash(std::span<const u8>(reinterpret_cast<const u8*>(str.data()), str.size()));
}

std::string Sha1::toHexString(const Digest& digest)
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

std::vector<u8> Sha1::padMessage(std::span<const u8> message)
{
    std::vector<u8> padded(message.begin(), message.end());

    // 追加 1 位（0x80）
    padded.push_back(0x80);

    // 填充 0 直到长度 ≡ 56 (mod 64)
    while (padded.size() % BLOCK_SIZE != 56) {
        padded.push_back(0x00);
    }

    // 追加原始长度（64 位大端序）
    const u64 bitLength = static_cast<u64>(message.size()) * 8;
    for (int i = 7; i >= 0; --i) {
        padded.push_back(static_cast<u8>((bitLength >> (i * 8)) & 0xFF));
    }

    return padded;
}

void Sha1::processBlock(const u8* block, std::array<u32, 5>& state)
{
    // 将块解析为 16 个 32 位大端序字
    std::array<u32, 80> W;
    for (std::size_t i = 0; i < 16; ++i) {
        W[i] = (static_cast<u32>(block[i * 4 + 0]) << 24) | (static_cast<u32>(block[i * 4 + 1]) << 16) |
            (static_cast<u32>(block[i * 4 + 2]) << 8) | static_cast<u32>(block[i * 4 + 3]);
    }

    // 扩展为 80 个字
    for (std::size_t i = 16; i < 80; ++i) {
        W[i] = rotl(W[i - 3] ^ W[i - 8] ^ W[i - 14] ^ W[i - 16], 1);
    }

    u32 a = state[0];
    u32 b = state[1];
    u32 c = state[2];
    u32 d = state[3];
    u32 e = state[4];

    // 第 0..19 轮: Ch + K0
    for (std::size_t i = 0; i < 20; ++i) {
        u32 temp = rotl(a, 5) + ((b & c) ^ (~b & d)) + e + W[i] + K0;
        e = d;
        d = c;
        c = rotl(b, 30);
        b = a;
        a = temp;
    }

    // 第 20..39 轮: Parity + K1
    for (std::size_t i = 20; i < 40; ++i) {
        u32 temp = rotl(a, 5) + (b ^ c ^ d) + e + W[i] + K1;
        e = d;
        d = c;
        c = rotl(b, 30);
        b = a;
        a = temp;
    }

    // 第 40..59 轮: Maj + K2
    for (std::size_t i = 40; i < 60; ++i) {
        u32 temp = rotl(a, 5) + ((b & c) ^ (b & d) ^ (c & d)) + e + W[i] + K2;
        e = d;
        d = c;
        c = rotl(b, 30);
        b = a;
        a = temp;
    }

    // 第 60..79 轮: Parity + K3
    for (std::size_t i = 60; i < 80; ++i) {
        u32 temp = rotl(a, 5) + (b ^ c ^ d) + e + W[i] + K3;
        e = d;
        d = c;
        c = rotl(b, 30);
        b = a;
        a = temp;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

} // namespace crypto
} // namespace util
} // namespace mc
