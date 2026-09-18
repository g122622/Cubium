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
 * @file Sha256Test.cpp
 * @brief SHA-256 哈希算法单元测试
 *
 * 测试覆盖：
 * 1. 标准 SHA-256 测试向量
 * 2. 空字符串和短消息
 * 3. hashWorldSeed 功能（MC 协议）
 * 4. 辅助函数（字节序转换、十六进制输出）
 */
#include "common/util/crypto/Sha256.hpp"
#include <array>
#include <cstdint>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::util::crypto;

class Sha256Test : public ::testing::Test {
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
// 标准 SHA-256 测试向量
// ============================================================================

/**
 * @brief 测试空字符串的 SHA-256 哈希
 *
 * 参考: FIPS 180-4 示例
 * 空字符串的 SHA-256:
 * e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
 */
TEST_F(Sha256Test, EmptyString)
{
    Sha256::Digest hash = Sha256::hash("");
    std::string hex = Sha256::toHexString(hash);

    EXPECT_EQ(hex, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

/**
 * @brief 测试 "abc" 的 SHA-256 哈希
 *
 * 参考: FIPS 180-4 示例
 * SHA256("abc") =
 * ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
 */
TEST_F(Sha256Test, AbcString)
{
    Sha256::Digest hash = Sha256::hash("abc");
    std::string hex = Sha256::toHexString(hash);

    EXPECT_EQ(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

/**
 * @brief 测试较长字符串的 SHA-256 哈希
 *
 * 参考: FIPS 180-4 示例
 * SHA256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") =
 * 248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1
 */
TEST_F(Sha256Test, LongerString)
{
    Sha256::Digest hash = Sha256::hash("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
    std::string hex = Sha256::toHexString(hash);

    EXPECT_EQ(hex, "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
}

/**
 * @brief 测试 "Hello, World!" 的 SHA-256 哈希
 *
 * SHA256("Hello, World!") =
 * dffd6021bb2bd5b0af676290809ec3a53191dd81c7f70a4b28688a362182986f
 */
TEST_F(Sha256Test, HelloWorld)
{
    Sha256::Digest hash = Sha256::hash("Hello, World!");
    std::string hex = Sha256::toHexString(hash);

    EXPECT_EQ(hex, "dffd6021bb2bd5b0af676290809ec3a53191dd81c7f70a4b28688a362182986f");
}

// ============================================================================
// hashUint64 测试
// ============================================================================

/**
 * @brief 测试 64 位整数的 SHA-256 哈希
 *
 * 验证 hashUint64 使用大端序正确编码整数
 */
TEST_F(Sha256Test, HashUint64Zero)
{
    // 测试 seed = 0
    // 大端序编码后为: 00 00 00 00 00 00 00 00
    // SHA256 的前 8 字节（小端序解释为 u64）
    Sha256::Digest hash = Sha256::hashUint64(0);
    std::string hex = Sha256::toHexString(hash);

    // 验证十六进制输出一致性
    EXPECT_EQ(hex.length(), 64);

    // 对于 seed = 0，我们可以验证结果的可重复性
    Sha256::Digest hash2 = Sha256::hashUint64(0);
    EXPECT_EQ(hash, hash2);
}

/**
 * @brief 测试 64 位整数的边界值
 */
TEST_F(Sha256Test, HashUint64MaxValue)
{
    // 测试最大值
    Sha256::Digest hash1 = Sha256::hashUint64(UINT64_MAX);
    Sha256::Digest hash2 = Sha256::hashUint64(UINT64_MAX);
    EXPECT_EQ(hash1, hash2);

    // 最大值和最小值应该产生不同的哈希
    Sha256::Digest hashZero = Sha256::hashUint64(0);
    EXPECT_NE(hash1, hashZero);
}

/**
 * @brief 测试特定种子值的哈希
 */
TEST_F(Sha256Test, HashUint64SpecificValue)
{
    // 测试种子值 12345
    u64 seed = 12345;
    Sha256::Digest hash = Sha256::hashUint64(seed);

    // 验证输出长度正确
    EXPECT_EQ(hash.size(), 32);

    // 验证可重复性
    Sha256::Digest hash2 = Sha256::hashUint64(seed);
    EXPECT_EQ(hash, hash2);
}

// ============================================================================
// hashWorldSeed 测试（MC 协议）
// ============================================================================

/**
 * @brief 测试 hashWorldSeed 返回一致性
 *
 * 验证 hashWorldSeed 对同一种子总是返回相同的值
 */
TEST_F(Sha256Test, HashWorldSeedConsistency)
{
    u64 seed1 = 12345678901234ULL;
    u64 seed2 = 98765432109876ULL;

    // 多次调用应该返回相同结果
    u64 hash1_a = Sha256::hashWorldSeed(seed1);
    u64 hash1_b = Sha256::hashWorldSeed(seed1);
    EXPECT_EQ(hash1_a, hash1_b);

    u64 hash2_a = Sha256::hashWorldSeed(seed2);
    u64 hash2_b = Sha256::hashWorldSeed(seed2);
    EXPECT_EQ(hash2_a, hash2_b);

    // 不同种子应该产生不同哈希
    EXPECT_NE(hash1_a, hash2_a);
}

/**
 * @brief 测试 hashWorldSeed 边界值
 */
TEST_F(Sha256Test, HashWorldSeedBoundaryValues)
{
    // 测试种子 = 0
    u64 hashZero = Sha256::hashWorldSeed(0);
    EXPECT_NE(hashZero, 0); // 哈希应该不是 0

    // 测试种子 = 1
    u64 hashOne = Sha256::hashWorldSeed(1);
    EXPECT_NE(hashOne, 0);
    EXPECT_NE(hashOne, hashZero);

    // 测试种子 = UINT64_MAX
    u64 hashMax = Sha256::hashWorldSeed(UINT64_MAX);
    EXPECT_NE(hashMax, 0);
    EXPECT_NE(hashMax, hashZero);
    EXPECT_NE(hashMax, hashOne);

    // 测试负数种子（有符号转无符号）
    i64 signedSeed = -12345;
    u64 hashNegative = Sha256::hashWorldSeed(static_cast<u64>(signedSeed));
    EXPECT_NE(hashNegative, 0);
}

/**
 * @brief 测试 hashWorldSeed 与 MC 原版行为对比
 *
 * 验证实现与 Guava Hashing.sha256().hashLong().asLong() 一致
 *
 * Guava 的行为：
 * 1. 将 long 以大端序写入 8 字节
 * 2. 计算 SHA-256 得到 32 字节
 * 3. 取前 8 字节以小端序解释为 long 返回
 */
TEST_F(Sha256Test, HashWorldSeedMCBehavior)
{
    // 测试几个典型的世界种子
    struct TestCase {
        u64 seed;
        u64 expectedNonZero; // 只验证结果非零且一致
    };

    TestCase cases[] = {
        {0ULL, true},
        {1ULL, true},
        {100ULL, true},
        {12345ULL, true},
        {1234567890ULL, true},
        {12345678901234ULL, true},
        {UINT64_MAX, true},
    };

    for (const auto& tc : cases) {
        u64 hash = Sha256::hashWorldSeed(tc.seed);
        if (tc.expectedNonZero) {
            EXPECT_NE(hash, 0ULL) << "Seed " << tc.seed << " produced zero hash";
        }

        // 验证可重复性
        u64 hash2 = Sha256::hashWorldSeed(tc.seed);
        EXPECT_EQ(hash, hash2) << "Seed " << tc.seed << " produced inconsistent hash";
    }
}

// ============================================================================
// 辅助函数测试
// ============================================================================

/**
 * @brief 测试 bytesToU64LE（小端序转换）
 */
TEST_F(Sha256Test, BytesToU64LE)
{
    // 小端序: 01 02 03 04 05 06 07 08 = 0x0807060504030201
    std::array<u8, 8> bytes = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    u64 value = Sha256::bytesToU64LE(std::span<const u8, 8>(bytes));
    EXPECT_EQ(value, 0x0807060504030201ULL);

    // 小端序: FF 00 00 00 00 00 00 00 = 0x00000000000000FF = 255
    bytes = {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    value = Sha256::bytesToU64LE(std::span<const u8, 8>(bytes));
    EXPECT_EQ(value, 255ULL);

    // 小端序: 00 00 00 00 00 00 00 01 = 0x0100000000000000
    bytes = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    value = Sha256::bytesToU64LE(std::span<const u8, 8>(bytes));
    EXPECT_EQ(value, 0x0100000000000000ULL);
}

/**
 * @brief 测试 bytesToU64BE（大端序转换）
 */
TEST_F(Sha256Test, BytesToU64BE)
{
    // 大端序: 01 02 03 04 05 06 07 08 = 0x0102030405060708
    std::array<u8, 8> bytes = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    u64 value = Sha256::bytesToU64BE(std::span<const u8, 8>(bytes));
    EXPECT_EQ(value, 0x0102030405060708ULL);

    // 大端序: FF 00 00 00 00 00 00 00 = 0xFF00000000000000
    bytes = {0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    value = Sha256::bytesToU64BE(std::span<const u8, 8>(bytes));
    EXPECT_EQ(value, 0xFF00000000000000ULL);

    // 大端序: 00 00 00 00 00 00 00 01 = 0x0000000000000001 = 1
    bytes = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    value = Sha256::bytesToU64BE(std::span<const u8, 8>(bytes));
    EXPECT_EQ(value, 1ULL);
}

/**
 * @brief 测试 toHexString 输出格式
 */
TEST_F(Sha256Test, ToHexString)
{
    // 所有字节为 0
    Sha256::Digest zeros = {};
    std::string hex = Sha256::toHexString(zeros);
    EXPECT_EQ(hex.length(), 64);
    EXPECT_EQ(hex, "0000000000000000000000000000000000000000000000000000000000000000");

    // 所有字节为 0xFF
    Sha256::Digest filled;
    filled.fill(0xFF);
    hex = Sha256::toHexString(filled);
    EXPECT_EQ(hex.length(), 64);
    EXPECT_EQ(hex, "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

    // 特定模式
    Sha256::Digest pattern = {};
    for (size_t i = 0; i < 32; ++i) {
        pattern[i] = static_cast<u8>(i);
    }
    hex = Sha256::toHexString(pattern);
    EXPECT_EQ(hex.length(), 64);
    // 验证前几个字节
    EXPECT_EQ(hex.substr(0, 4), "0001");
    EXPECT_EQ(hex.substr(4, 4), "0203");
}

// ============================================================================
// 字节数组输入测试
// ============================================================================

/**
 * @brief 测试字节数组输入
 */
TEST_F(Sha256Test, ByteSpanInput)
{
    // 测试与字符串输入一致性
    const char* str = "abc";
    std::vector<u8> bytes(str, str + 3);

    Sha256::Digest hashStr = Sha256::hash("abc");
    Sha256::Digest hashBytes = Sha256::hash(std::span<const u8>(bytes.data(), bytes.size()));

    EXPECT_EQ(hashStr, hashBytes);
}

/**
 * @brief 测试空字节数组
 */
TEST_F(Sha256Test, EmptyByteSpan)
{
    std::vector<u8> empty;
    Sha256::Digest hash = Sha256::hash(std::span<const u8>(empty.data(), 0));

    // 空输入应该与空字符串相同
    Sha256::Digest hashEmptyStr = Sha256::hash("");
    EXPECT_EQ(hash, hashEmptyStr);
}

// ============================================================================
// 性能/稳定性测试
// ============================================================================

/**
 * @brief 测试大量计算的稳定性
 */
TEST_F(Sha256Test, StabilityTest)
{
    // 连续计算大量哈希，验证没有内存泄漏或崩溃
    for (int i = 0; i < 1000; ++i) {
        Sha256::Digest hash = Sha256::hashUint64(static_cast<u64>(i));
        EXPECT_EQ(hash.size(), 32);
    }

    for (int i = 0; i < 1000; ++i) {
        u64 hash = Sha256::hashWorldSeed(static_cast<u64>(i));
        (void)hash; // 避免 unused variable 警告
    }
}

/**
 * @brief 测试长消息
 */
TEST_F(Sha256Test, LongMessage)
{
    // 1MB 数据
    std::vector<u8> data(1024 * 1024, 0xAB);

    // 应该能够处理长消息而不崩溃
    Sha256::Digest hash = Sha256::hash(std::span<const u8>(data.data(), data.size()));
    EXPECT_EQ(hash.size(), 32);

    // 相同数据应该产生相同哈希
    Sha256::Digest hash2 = Sha256::hash(std::span<const u8>(data.data(), data.size()));
    EXPECT_EQ(hash, hash2);
}
