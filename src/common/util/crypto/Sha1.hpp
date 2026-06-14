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
 * @file Sha1.hpp
 * @brief SHA-1 哈希算法实现
 *
 * 提供符合 FIPS 180-4 标准的 SHA-1 哈希计算功能。
 * 主要用于皮肤系统计算纹理哈希值。
 *
 * 注意：SHA-1 已被证明存在碰撞攻击漏洞，不应用于安全敏感的场景。
 * 此实现仅用于 Minecraft 皮肤缓存键生成等非安全需求。
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
 * @brief SHA-1 哈希计算器
 *
 * 提供符合 FIPS 180-4 标准的 SHA-1 哈希计算功能。
 * 支持多种输入格式：字节序列、字符串。
 *
 * 使用示例：
 * @code
 * // 计算字节数组的哈希
 * std::array<u8, 20> hash = Sha1::hash(dataSpan);
 * std::string hexString = Sha1::toHexString(hash);
 *
 * // 计算字符串的哈希
 * std::array<u8, 20> hash = Sha1::hash("Hello, World!");
 * @endcode
 */
class Sha1 {
public:
    /// SHA-1 输出长度（20 字节 = 160 位）
    static constexpr std::size_t DIGEST_SIZE = 20;

    /// SHA-1 哈希结果类型
    using Digest = std::array<u8, DIGEST_SIZE>;

    /**
     * @brief 计算数据的 SHA-1 哈希
     *
     * @param data 输入数据的字节视图
     * @return 20 字节的哈希结果
     */
    [[nodiscard]] static Digest hash(std::span<const u8> data);

    /**
     * @brief 计算字符串的 SHA-1 哈希
     *
     * @param str 输入字符串
     * @return 20 字节的哈希结果
     */
    [[nodiscard]] static Digest hash(std::string_view str);

    /**
     * @brief 将哈希结果转换为十六进制字符串
     *
     * @param digest 哈希结果
     * @return 40 个十六进制字符的小写字符串
     */
    [[nodiscard]] static std::string toHexString(const Digest& digest);

private:
    /// SHA-1 初始哈希值
    static constexpr std::array<u32, 5> INITIAL_HASH = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};

    /// SHA-1 轮常量
    static constexpr u32 K0 = 0x5A827999; // 0..19
    static constexpr u32 K1 = 0x6ED9EBA1; // 20..39
    static constexpr u32 K2 = 0x8F1BBCDC; // 40..59
    static constexpr u32 K3 = 0xCA62C1D6; // 60..79

    /// SHA-1 块大小（64 字节 = 512 位）
    static constexpr std::size_t BLOCK_SIZE = 64;

    /**
     * @brief 处理单个 512 位块
     *
     * @param block 64 字节的块数据
     * @param state 当前哈希状态（输入/输出）
     */
    static void processBlock(const u8* block, std::array<u32, 5>& state);

    /**
     * @brief 对消息进行填充
     *
     * @param message 原始消息
     * @return 填充后的消息（长度为 512 的倍数）
     */
    [[nodiscard]] static std::vector<u8> padMessage(std::span<const u8> message);

    /// 循环左移
    [[nodiscard]] static constexpr u32 rotl(u32 x, std::size_t n) { return (x << n) | (x >> (32 - n)); }
};

} // namespace crypto
} // namespace util
} // namespace mc
