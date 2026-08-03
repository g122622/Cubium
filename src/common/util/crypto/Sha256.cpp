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
 * @file Sha256.cpp
 * @brief SHA-256 哈希算法实现
 *
 * 实现符合 FIPS 180-4 标准的 SHA-256 哈希算法。
 */
#include "Sha256.hpp"
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

Sha256::Digest Sha256::hash(std::span<const u8> data)
{
    // 初始化哈希状态
    std::array<u32, 8> state = INITIAL_HASH;

    // 填充消息
    std::vector<u8> padded = padMessage(data);

    // 处理每个 512 位块
    for (std::size_t i = 0; i < padded.size(); i += BLOCK_SIZE) {
        processBlock(&padded[i], state);
    }

    // 生成最终哈希（大端序输出）
    Digest result;
    for (std::size_t i = 0; i < 8; ++i) {
        result[i * 4] = static_cast<u8>((state[i] >> 24) & 0xFF);
        result[i * 4 + 1] = static_cast<u8>((state[i] >> 16) & 0xFF);
        result[i * 4 + 2] = static_cast<u8>((state[i] >> 8) & 0xFF);
        result[i * 4 + 3] = static_cast<u8>(state[i] & 0xFF);
    }

    return result;
}

Sha256::Digest Sha256::hash(std::string_view str)
{
    return hash(std::span<const u8>(reinterpret_cast<const u8*>(str.data()), str.size()));
}

Sha256::Digest Sha256::hashUint64(u64 value)
{
    // Guava Hashing.sha256().hashLong() 使用大端序
    std::array<u8, 8> bytes;
    bytes[0] = static_cast<u8>((value >> 56) & 0xFF);
    bytes[1] = static_cast<u8>((value >> 48) & 0xFF);
    bytes[2] = static_cast<u8>((value >> 40) & 0xFF);
    bytes[3] = static_cast<u8>((value >> 32) & 0xFF);
    bytes[4] = static_cast<u8>((value >> 24) & 0xFF);
    bytes[5] = static_cast<u8>((value >> 16) & 0xFF);
    bytes[6] = static_cast<u8>((value >> 8) & 0xFF);
    bytes[7] = static_cast<u8>(value & 0xFF);

    return hash(std::span<const u8>(bytes.data(), bytes.size()));
}

u64 Sha256::hashWorldSeed(u64 worldSeed)
{
    // 计算 SHA-256 哈希
    Digest digest = hashUint64(worldSeed);

    // 取前 8 字节，以小端序解释为 u64
    // 这与 Guava HashCode.asLong() 的行为一致
    // asLong() 返回前 8 字节的小端序解释
    return bytesToU64LE(std::span<const u8, 8>(digest.data(), 8));
}

std::string Sha256::toHexString(const Digest& digest)
{
    static constexpr char HEX_CHARS[] = "0123456789abcdef";

    std::string result;
    result.reserve(DIGEST_SIZE * 2);

    for (u8 byte : digest) {
        result.push_back(HEX_CHARS[(byte >> 4) & 0x0F]);
        result.push_back(HEX_CHARS[byte & 0x0F]);
    }

    return result;
}

u64 Sha256::bytesToU64LE(std::span<const u8, 8> bytes)
{
    return static_cast<u64>(bytes[0]) | (static_cast<u64>(bytes[1]) << 8) | (static_cast<u64>(bytes[2]) << 16) |
        (static_cast<u64>(bytes[3]) << 24) | (static_cast<u64>(bytes[4]) << 32) | (static_cast<u64>(bytes[5]) << 40) |
        (static_cast<u64>(bytes[6]) << 48) | (static_cast<u64>(bytes[7]) << 56);
}

u64 Sha256::bytesToU64BE(std::span<const u8, 8> bytes)
{
    return (static_cast<u64>(bytes[0]) << 56) | (static_cast<u64>(bytes[1]) << 48) |
        (static_cast<u64>(bytes[2]) << 40) | (static_cast<u64>(bytes[3]) << 32) | (static_cast<u64>(bytes[4]) << 24) |
        (static_cast<u64>(bytes[5]) << 16) | (static_cast<u64>(bytes[6]) << 8) | static_cast<u64>(bytes[7]);
}

void Sha256::processBlock(const u8* block, std::array<u32, 8>& state)
{
    // 准备消息调度数组 W
    std::array<u32, 64> W;

    // 前 16 个字来自输入块（大端序）
    for (std::size_t i = 0; i < 16; ++i) {
        W[i] = (static_cast<u32>(block[i * 4]) << 24) | (static_cast<u32>(block[i * 4 + 1]) << 16) |
            (static_cast<u32>(block[i * 4 + 2]) << 8) | static_cast<u32>(block[i * 4 + 3]);
    }

    // 扩展剩余 48 个字
    for (std::size_t i = 16; i < 64; ++i) {
        W[i] = gamma1(W[i - 2]) + W[i - 7] + gamma0(W[i - 15]) + W[i - 16];
    }

    // 初始化工作变量
    u32 a = state[0];
    u32 b = state[1];
    u32 c = state[2];
    u32 d = state[3];
    u32 e = state[4];
    u32 f = state[5];
    u32 g = state[6];
    u32 h = state[7];

    // 主循环（64 轮）
    for (std::size_t i = 0; i < 64; ++i) {
        u32 T1 = h + sigma1(e) + ch(e, f, g) + ROUND_CONSTANTS[i] + W[i];
        u32 T2 = sigma0(a) + maj(a, b, c);

        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }

    // 更新哈希状态
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

std::vector<u8> Sha256::padMessage(std::span<const u8> message)
{
    // 计算填充后的长度
    // 填充格式：消息 + 1 bit + 0 bits + 64 bit 长度
    // 最终长度必须是 512 位（64 字节）的倍数
    std::size_t messageLen = message.size();
    std::size_t paddedLen = messageLen + 1 + 8; // 消息 + 0x80 + 8 字节长度

    // 计算需要添加多少 0 字节
    std::size_t paddingZeros = 0;
    if (paddedLen % BLOCK_SIZE != 0) {
        paddingZeros = BLOCK_SIZE - (paddedLen % BLOCK_SIZE);
    }
    paddedLen += paddingZeros;

    // 创建填充后的消息
    std::vector<u8> padded(paddedLen, 0);

    // 复制原始消息
    std::memcpy(padded.data(), message.data(), messageLen);

    // 添加 1 bit (0x80)
    padded[messageLen] = 0x80;

    // 添加长度（大端序，64 位）
    // 注意：SHA-256 使用位数，不是字节数
    u64 messageBits = static_cast<u64>(messageLen) * 8;
    std::size_t lenPos = paddedLen - 8;
    padded[lenPos] = static_cast<u8>((messageBits >> 56) & 0xFF);
    padded[lenPos + 1] = static_cast<u8>((messageBits >> 48) & 0xFF);
    padded[lenPos + 2] = static_cast<u8>((messageBits >> 40) & 0xFF);
    padded[lenPos + 3] = static_cast<u8>((messageBits >> 32) & 0xFF);
    padded[lenPos + 4] = static_cast<u8>((messageBits >> 24) & 0xFF);
    padded[lenPos + 5] = static_cast<u8>((messageBits >> 16) & 0xFF);
    padded[lenPos + 6] = static_cast<u8>((messageBits >> 8) & 0xFF);
    padded[lenPos + 7] = static_cast<u8>(messageBits & 0xFF);

    return padded;
}

} // namespace crypto
} // namespace util
} // namespace mc
