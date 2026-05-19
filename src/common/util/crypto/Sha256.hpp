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
 * @file Sha256.hpp
 * @brief SHA-256 哈希算法实现
 *
 * 提供符合 FIPS 180-4 标准的 SHA-256 哈希计算功能。
 * 主要用于计算世界种子的哈希值，用于 Minecraft 协议中的 hashedSeed。
 *
 * 参考 Minecraft 1.16.5 BiomeManager.func_235200_a_:
 * return Hashing.sha256().hashLong(p_235200_0_).asLong();
 */
#pragma once

#include "common/core/Types.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mc {
namespace util {
namespace crypto {

/**
 * @brief SHA-256 哈希计算器
 *
 * 提供符合 FIPS 180-4 标准的 SHA-256 哈希计算功能。
 * 支持多种输入格式：字节序列、字符串、64位整数。
 *
 * 使用示例：
 * @code
 * // 计算 64 位整数的哈希
 * u64 seed = 12345678901234ULL;
 * u64 hashedSeed = Sha256::hashUint64(seed);
 *
 * // 计算字节数组的哈希
 * std::array<u8, 32> hash = Sha256::hash(dataSpan);
 *
 * // 计算字符串的哈希
 * std::array<u8, 32> hash = Sha256::hash("Hello, World!");
 * @endcode
 */
class Sha256 {
public:
    /// SHA-256 输出长度（32 字节 = 256 位）
    static constexpr std::size_t DIGEST_SIZE = 32;

    /// SHA-256 哈希结果类型
    using Digest = std::array<u8, DIGEST_SIZE>;

    /**
     * @brief 计算数据的 SHA-256 哈希
     *
     * @param data 输入数据的字节视图
     * @return 32 字节的哈希结果
     */
    [[nodiscard]] static Digest hash(std::span<const u8> data);

    /**
     * @brief 计算字符串的 SHA-256 哈希
     *
     * @param str 输入字符串
     * @return 32 字节的哈希结果
     */
    [[nodiscard]] static Digest hash(std::string_view str);

    /**
     * @brief 计算 64 位整数的 SHA-256 哈希
     *
     * 将整数以大端序字节形式计算哈希，与 Guava Hashing.sha256().hashLong() 行为一致。
     * 这是 Minecraft 协议中计算 hashedSeed 的标准方法。
     *
     * 参考 MC 1.16.5 BiomeManager.func_235200_a_:
     * Hashing.sha256().hashLong(seed).asLong()
     *
     * @param value 64 位整数值
     * @return 32 字节的哈希结果
     */
    [[nodiscard]] static Digest hashUint64(u64 value);

    /**
     * @brief 计算世界种子的哈希值（hashedSeed）
     *
     * 对世界种子进行 SHA-256 哈希，返回前 8 字节作为 64 位整数。
     * 这是 Minecraft 1.16.5+ 协议中 hashedSeed 的标准计算方法。
     *
     * 实现细节：
     * 1. 将种子（u64）以大端序转换为 8 字节
     * 2. 计算 SHA-256 哈希得到 32 字节
     * 3. 取前 8 字节以小端序解释为 u64 返回
     *
     * @param worldSeed 世界种子
     * @return hashedSeed 值（SHA-256 哈希前 8 字节）
     */
    [[nodiscard]] static u64 hashWorldSeed(u64 worldSeed);

    /**
     * @brief 将哈希结果转换为十六进制字符串
     *
     * @param digest 哈希结果
     * @return 64 个十六进制字符的小写字符串
     */
    [[nodiscard]] static std::string toHexString(const Digest& digest);

    /**
     * @brief 从小端序字节构造 u64
     *
     * @param bytes 8 字节数组（小端序）
     * @return u64 值
     */
    [[nodiscard]] static u64 bytesToU64LE(std::span<const u8, 8> bytes);

    /**
     * @brief 从大端序字节构造 u64
     *
     * @param bytes 8 字节数组（大端序）
     * @return u64 值
     */
    [[nodiscard]] static u64 bytesToU64BE(std::span<const u8, 8> bytes);

private:
    /// SHA-256 初始哈希值（前 32 位小数部分的平方根）
    static constexpr std::array<u32, 8> INITIAL_HASH = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

    /// SHA-256 轮常量（前 64 个质数立方根的小数部分）
    static constexpr std::array<u32, 64> ROUND_CONSTANTS = {0x428a2f98,
        0x71374491,
        0xb5c0fbcf,
        0xe9b5dba5,
        0x3956c25b,
        0x59f111f1,
        0x923f82a4,
        0xab1c5ed5,
        0xd807aa98,
        0x12835b01,
        0x243185be,
        0x550c7dc3,
        0x72be5d74,
        0x80deb1fe,
        0x9bdc06a7,
        0xc19bf174,
        0xe49b69c1,
        0xefbe4786,
        0x0fc19dc6,
        0x240ca1cc,
        0x2de92c6f,
        0x4a7484aa,
        0x5cb0a9dc,
        0x76f988da,
        0x983e5152,
        0xa831c66d,
        0xb00327c8,
        0xbf597fc7,
        0xc6e00bf3,
        0xd5a79147,
        0x06ca6351,
        0x14292967,
        0x27b70a85,
        0x2e1b2138,
        0x4d2c6dfc,
        0x53380d13,
        0x650a7354,
        0x766a0abb,
        0x81c2c92e,
        0x92722c85,
        0xa2bfe8a1,
        0xa81a664b,
        0xc24b8b70,
        0xc76c51a3,
        0xd192e819,
        0xd6990624,
        0xf40e3585,
        0x106aa070,
        0x19a4c116,
        0x1e376c08,
        0x2748774c,
        0x34b0bcb5,
        0x391c0cb3,
        0x4ed8aa4a,
        0x5b9cca4f,
        0x682e6ff3,
        0x748f82ee,
        0x78a5636f,
        0x84c87814,
        0x8cc70208,
        0x90befffa,
        0xa4506ceb,
        0xbef9a3f7,
        0xc67178f2};

    /// SHA-256 块大小（64 字节 = 512 位）
    static constexpr std::size_t BLOCK_SIZE = 64;

    /**
     * @brief 处理单个 512 位块
     *
     * @param block 64 字节的块数据
     * @param state 当前哈希状态（输入/输出）
     */
    static void processBlock(const u8* block, std::array<u32, 8>& state);

    /**
     * @brief 对消息进行填充
     *
     * @param message 原始消息
     * @return 填充后的消息（长度为 512 的倍数）
     */
    [[nodiscard]] static std::vector<u8> padMessage(std::span<const u8> message);

    // SHA-256 辅助函数

    /// 右旋转
    [[nodiscard]] static constexpr u32 rotr(u32 x, std::size_t n) { return (x >> n) | (x << (32 - n)); }

    /// 右移位
    [[nodiscard]] static constexpr u32 shr(u32 x, std::size_t n) { return x >> n; }

    /// 选择函数 (Ch)
    [[nodiscard]] static constexpr u32 ch(u32 x, u32 y, u32 z) { return (x & y) ^ (~x & z); }

    /// 多数函数 (Maj)
    [[nodiscard]] static constexpr u32 maj(u32 x, u32 y, u32 z) { return (x & y) ^ (x & z) ^ (y & z); }

    /// 大 Sigma 0
    [[nodiscard]] static constexpr u32 sigma0(u32 x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }

    /// 大 Sigma 1
    [[nodiscard]] static constexpr u32 sigma1(u32 x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }

    /// 小 sigma 0
    [[nodiscard]] static constexpr u32 gamma0(u32 x) { return rotr(x, 7) ^ rotr(x, 18) ^ shr(x, 3); }

    /// 小 sigma 1
    [[nodiscard]] static constexpr u32 gamma1(u32 x) { return rotr(x, 17) ^ rotr(x, 19) ^ shr(x, 10); }
};

} // namespace crypto
} // namespace util
} // namespace mc
