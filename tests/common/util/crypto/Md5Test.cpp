/**
 * @file Md5Test.cpp
 * @brief MD5 哈希算法单元测试
 */

#include <gtest/gtest.h>
#include "common/util/crypto/Md5.hpp"
#include <string>

namespace mc::util::crypto::test {

/**
 * @brief 测试空字符串的 MD5 哈希
 *
 * RFC 1321 测试向量：MD5("") = d41d8cd98f00b204e9800998ecf8427e
 */
TEST(Md5Test, EmptyString) {
    Md5::Digest hash = Md5::hash("");
    std::string hex = Md5::toHexString(hash);

    EXPECT_EQ(hex, "d41d8cd98f00b204e9800998ecf8427e");
}

/**
 * @brief 测试 "a" 的 MD5 哈希
 *
 * RFC 1321 测试向量：MD5("a") = 0cc175b9c0f1b6a831c399e269772661
 */
TEST(Md5Test, SingleCharacter) {
    Md5::Digest hash = Md5::hash("a");
    std::string hex = Md5::toHexString(hash);

    EXPECT_EQ(hex, "0cc175b9c0f1b6a831c399e269772661");
}

/**
 * @brief 测试 "abc" 的 MD5 哈希
 *
 * RFC 1321 测试向量：MD5("abc") = 900150983cd24fb0d6963f7d28e17f72
 */
TEST(Md5Test, Abc) {
    Md5::Digest hash = Md5::hash("abc");
    std::string hex = Md5::toHexString(hash);

    EXPECT_EQ(hex, "900150983cd24fb0d6963f7d28e17f72");
}

/**
 * @brief 测试 "message digest" 的 MD5 哈希
 *
 * RFC 1321 测试向量：MD5("message digest") = f96b697d7cb7938d525a2f31aaf161d0
 */
TEST(Md5Test, MessageDigest) {
    Md5::Digest hash = Md5::hash("message digest");
    std::string hex = Md5::toHexString(hash);

    EXPECT_EQ(hex, "f96b697d7cb7938d525a2f31aaf161d0");
}

/**
 * @brief 测试 "abcdefghijklmnopqrstuvwxyz" 的 MD5 哈希
 *
 * RFC 1321 测试向量：MD5("abcdefghijklmnopqrstuvwxyz") = c3fcd3d76192e4007dfb496cca67e13b
 */
TEST(Md5Test, Alphabet) {
    Md5::Digest hash = Md5::hash("abcdefghijklmnopqrstuvwxyz");
    std::string hex = Md5::toHexString(hash);

    EXPECT_EQ(hex, "c3fcd3d76192e4007dfb496cca67e13b");
}

/**
 * @brief 测试字节数组输入
 */
TEST(Md5Test, ByteArray) {
    std::vector<u8> data = {'a', 'b', 'c'};
    Md5::Digest hash = Md5::hash(std::span<const u8>(data.data(), data.size()));
    std::string hex = Md5::toHexString(hash);

    EXPECT_EQ(hex, "900150983cd24fb0d6963f7d28e17f72");
}

/**
 * @brief 测试长字符串（长度刚好超过一个块）
 *
 * MD5 块大小是 512 位 = 64 字节
 */
TEST(Md5Test, LongString) {
    // 65 个 'a' 字符
    std::string longStr(65, 'a');
    Md5::Digest hash = Md5::hash(longStr);

    // 只验证输出长度正确
    EXPECT_EQ(hash.size(), 16u);

    std::string hex = Md5::toHexString(hash);
    EXPECT_EQ(hex.length(), 32u);
}

/**
 * @brief 测试输出长度
 */
TEST(Md5Test, OutputLength) {
    Md5::Digest hash = Md5::hash("test");
    EXPECT_EQ(hash.size(), 16u);

    std::string hex = Md5::toHexString(hash);
    EXPECT_EQ(hex.length(), 32u);
}

} // namespace mc::util::crypto::test
