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
 * @file Md5.hpp
 * @brief MD5 哈希算法实现
 *
 * 提供 RFC 1321 标准的 MD5 哈希计算功能。
 * 主要用于生成 Minecraft 离线模式 UUID（UUID v3）。
 *
 * 注意：MD5 不应用于安全敏感的场景，仅用于 UUID 生成等兼容性需求。
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
 * @brief MD5 哈希计算器
 *
 * 提供符合 RFC 1321 标准的 MD5 哈希计算功能。
 * 主要用于生成 Minecraft 离线模式 UUID（UUID v3）。
 *
 * 注意：MD5 已被认为不安全，请勿用于密码存储或安全验证。
 * 此实现仅用于与 Minecraft 兼容的 UUID 生成。
 *
 * 使用示例：
 * @code
 * // 计算字符串的 MD5 哈希
 * Md5::Digest hash = Md5::hash("Hello, World!");
 * std::string hexString = Md5::toHexString(hash);
 *
 * // 生成离线模式 UUID
 * std::string input = "OfflinePlayer:Steve";
 * Md5::Digest md5 = Md5::hash(input);
 * Uuid uuid = UuidUtils::uuidFromMd5(md5);  // 设置版本和变体
 * @endcode
 */
class Md5 {
public:
    /// MD5 输出长度（16 字节 = 128 位）
    static constexpr std::size_t DIGEST_SIZE = 16;

    /// MD5 哈希结果类型
    using Digest = std::array<u8, DIGEST_SIZE>;

    /**
     * @brief 计算数据的 MD5 哈希
     *
     * @param data 输入数据的字节视图
     * @return 16 字节的哈希结果
     */
    [[nodiscard]] static Digest hash(std::span<const u8> data);

    /**
     * @brief 计算字符串的 MD5 哈希
     *
     * @param str 输入字符串
     * @return 16 字节的哈希结果
     */
    [[nodiscard]] static Digest hash(std::string_view str);

    /**
     * @brief 将哈希结果转换为十六进制字符串
     *
     * @param digest 哈希结果
     * @return 32 个十六进制字符的小写字符串
     */
    [[nodiscard]] static std::string toHexString(const Digest& digest);

private:
    /// MD5 初始哈希值（A, B, C, D）
    static constexpr std::array<u32, 4> INITIAL_HASH = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};

    /// MD5 块大小（64 字节 = 512 位）
    static constexpr std::size_t BLOCK_SIZE = 64;

    /**
     * @brief 处理单个 512 位块
     *
     * @param block 64 字节的块数据
     * @param state 当前哈希状态（输入/输出）
     */
    static void processBlock(const u8* block, std::array<u32, 4>& state);

    /**
     * @brief 对消息进行填充
     *
     * @param message 原始消息
     * @return 填充后的消息（长度为 512 的倍数）
     */
    [[nodiscard]] static std::vector<u8> padMessage(std::span<const u8> message);

    // MD5 辅助函数

    /// F 函数: (X & Y) | (~X & Z)
    [[nodiscard]] static constexpr u32 f(u32 x, u32 y, u32 z) { return (x & y) | (~x & z); }

    /// G 函数: (X & Z) | (Y & ~Z)
    [[nodiscard]] static constexpr u32 g(u32 x, u32 y, u32 z) { return (x & z) | (y & ~z); }

    /// H 函数: X ^ Y ^ Z
    [[nodiscard]] static constexpr u32 h(u32 x, u32 y, u32 z) { return x ^ y ^ z; }

    /// I 函数: Y ^ (X | ~Z)
    [[nodiscard]] static constexpr u32 i(u32 x, u32 y, u32 z) { return y ^ (x | ~z); }

    /// 循环左移
    [[nodiscard]] static constexpr u32 rotl(u32 x, std::size_t n) { return (x << n) | (x >> (32 - n)); }
};

} // namespace crypto
} // namespace util
} // namespace mc
