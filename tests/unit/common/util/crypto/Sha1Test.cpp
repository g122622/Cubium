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
 * @file Sha1Test.cpp
 * @brief SHA-1 哈希算法单元测试
 *
 * 测试覆盖：
 * 1. FIPS 180-4 标准 SHA-1 测试向量
 * 2. 空字符串和短消息
 * 3. 较长消息（多块处理）
 * 4. 十六进制输出格式
 */
#include "common/util/crypto/Sha1.hpp"
#include <array>
#include <cstdint>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::util::crypto;

class Sha1Test : public ::testing::Test {
protected:
    // 辅助函数：将十六进制字符串转换为字节数组
    static std::vector<u8> hexToBytes(const std::string& hex)
    {
        std::vector<u8> bytes;
        for (size_t i = 0; i < hex.length(); i += 2) {
            u8 byte = static_cast<u8>(std::stoi(hex.substr(i, 2), nullptr, 16));
            bytes.push_back(byte);
        }
        return bytes;
    }
};

// ============================================================================
// FIPS 180-4 标准 SHA-1 测试向量
// ============================================================================

/**
 * @brief 测试空字符串的 SHA-1 哈希
 *
 * 参考: FIPS 180-4
 * SHA1("") = da39a3ee5e6b4b0d3255bfef95601890afd80709
 */
TEST_F(Sha1Test, EmptyString)
{
    Sha1::Digest hash = Sha1::hash("");
    std::string hex = Sha1::toHexString(hash);

    EXPECT_EQ(hex, "da39a3ee5e6b4b0d3255bfef95601890afd80709");
}

/**
 * @brief 测试 "abc" 的 SHA-1 哈希
 *
 * 参考: FIPS 180-4 示例
 * SHA1("abc") = a9993e364706816aba3e25717850c26c9cd0d89d
 */
TEST_F(Sha1Test, Abc)
{
    Sha1::Digest hash = Sha1::hash("abc");
    std::string hex = Sha1::toHexString(hash);

    EXPECT_EQ(hex, "a9993e364706816aba3e25717850c26c9cd0d89d");
}

/**
 * @brief 测试 FIPS 180-4 第二个测试向量
 *
 * SHA1("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")
 * = 84983e441c3bd26ebaae4aa1f95129e5e54670f1
 */
TEST_F(Sha1Test, FipsTwoBlockMessage)
{
    Sha1::Digest hash = Sha1::hash("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
    std::string hex = Sha1::toHexString(hash);

    EXPECT_EQ(hex, "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
}

/**
 * @brief 测试字节序列输入
 *
 * 使用与 "abc" 相同的字节序列，验证 span 接口
 */
TEST_F(Sha1Test, ByteSpanInput)
{
    const u8 data[] = {'a', 'b', 'c'};
    Sha1::Digest hash = Sha1::hash(std::span<const u8>(data, 3));
    std::string hex = Sha1::toHexString(hash);

    EXPECT_EQ(hex, "a9993e364706816aba3e25717850c26c9cd0d89d");
}

/**
 * @brief 测试较长消息（超过一个 512 位块）
 *
 * SHA1 重复 "a" 1000000 次:
 * = 34aa973cd4c4daa4f61eeb2bdbad27316534016f
 */
TEST_F(Sha1Test, LongMessage)
{
    std::string longMsg(1000000, 'a');
    Sha1::Digest hash = Sha1::hash(longMsg);
    std::string hex = Sha1::toHexString(hash);

    EXPECT_EQ(hex, "34aa973cd4c4daa4f61eeb2bdbad27316534016f");
}

/**
 * @brief 测试摘要长度
 *
 * SHA-1 摘要应为 20 字节（160 位）
 */
TEST_F(Sha1Test, DigestSize)
{
    EXPECT_EQ(Sha1::DIGEST_SIZE, 20u);
    EXPECT_EQ(Sha1::Digest{}.size(), 20u);
}

/**
 * @brief 测试十六进制输出长度
 *
 * SHA-1 十六进制输出应为 40 个字符
 */
TEST_F(Sha1Test, HexStringLength)
{
    Sha1::Digest hash = Sha1::hash("test");
    std::string hex = Sha1::toHexString(hash);

    EXPECT_EQ(hex.length(), 40u);
}

/**
 * @brief 测试十六进制输出为小写
 */
TEST_F(Sha1Test, HexStringLowercase)
{
    Sha1::Digest hash = Sha1::hash("abc");
    std::string hex = Sha1::toHexString(hash);

    // 确保所有字符都是小写十六进制
    for (char c : hex) {
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
    }
}

/**
 * @brief 测试相同输入产生相同输出（确定性）
 */
TEST_F(Sha1Test, Deterministic)
{
    Sha1::Digest hash1 = Sha1::hash("deterministic test");
    Sha1::Digest hash2 = Sha1::hash("deterministic test");

    EXPECT_EQ(hash1, hash2);
}

/**
 * @brief 测试不同输入产生不同输出
 */
TEST_F(Sha1Test, DifferentInputsDifferentOutputs)
{
    Sha1::Digest hash1 = Sha1::hash("input1");
    Sha1::Digest hash2 = Sha1::hash("input2");

    EXPECT_NE(hash1, hash2);
}

/**
 * @brief 测试 FIPS 180-4 第三个测试向量
 *
 * SHA1("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu")
 * = a49b2446a02c645bf419f995b67091253a04a259
 */
TEST_F(Sha1Test, FipsThreeBlockMessage)
{
    Sha1::Digest hash = Sha1::hash("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklm"
                                   "nopqrlmnopqrsmnopqrstnopqrstu");
    std::string hex = Sha1::toHexString(hash);

    EXPECT_EQ(hex, "a49b2446a02c645bf419f995b67091253a04a259");
}

/**
 * @brief 测试单字节消息
 */
TEST_F(Sha1Test, SingleByte)
{
    const u8 data[] = {0x61}; // 'a'
    Sha1::Digest hash = Sha1::hash(std::span<const u8>(data, 1));
    std::string hex = Sha1::toHexString(hash);

    // SHA1("a") = 86f7e437faa5a7fce15d1ddcb9eaeaea377667b8
    EXPECT_EQ(hex, "86f7e437faa5a7fce15d1ddcb9eaeaea377667b8");
}

/**
 * @brief 测试恰好 55 字节的消息（填充边界：恰好不触发额外块）
 *
 * 55 字节是 SHA-1 填充的关键边界：55 + 1(0x80) + 8(长度) = 64 = 一个块
 */
TEST_F(Sha1Test, Exactly55Bytes)
{
    std::string msg(55, 'A');
    Sha1::Digest hash = Sha1::hash(msg);
    std::string hex = Sha1::toHexString(hash);

    // 验证输出是有效的 40 字符十六进制
    EXPECT_EQ(hex.length(), 40u);

    // 验证确定性
    Sha1::Digest hash2 = Sha1::hash(msg);
    EXPECT_EQ(hash, hash2);
}

/**
 * @brief 测试 56 字节的消息（填充边界：需要额外块）
 *
 * 56 + 1(0x80) > 56，因此需要额外的块来存放长度
 */
TEST_F(Sha1Test, Exactly56Bytes)
{
    std::string msg(56, 'B');
    Sha1::Digest hash = Sha1::hash(msg);
    std::string hex = Sha1::toHexString(hash);

    EXPECT_EQ(hex.length(), 40u);

    // 验证确定性
    Sha1::Digest hash2 = Sha1::hash(msg);
    EXPECT_EQ(hash, hash2);
}

/**
 * @brief 测试 64 字节的消息（恰好一个完整块）
 */
TEST_F(Sha1Test, Exactly64Bytes)
{
    std::string msg(64, 'C');
    Sha1::Digest hash = Sha1::hash(msg);
    std::string hex = Sha1::toHexString(hash);

    EXPECT_EQ(hex.length(), 40u);

    Sha1::Digest hash2 = Sha1::hash(msg);
    EXPECT_EQ(hash, hash2);
}
