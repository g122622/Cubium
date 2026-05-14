/**
 * @file UuidUtilsTest.cpp
 * @brief UUID 工具函数单元测试
 */

#include "common/util/UuidUtils.hpp"
#include "common/util/crypto/Md5.hpp"
#include <array>
#include <string>
#include <gtest/gtest.h>

namespace mc::util::test {

/**
 * @brief 测试 uuidToString 和 uuidFromString 互逆
 */
TEST(UuidUtilsTest, StringRoundTrip)
{
    Uuid original = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};

    std::string str = uuidToString(original);
    EXPECT_EQ(str.length(), 32u);
    EXPECT_EQ(str, "0123456789abcdeffedcba9876543210");

    Uuid parsed = uuidFromString(str);
    EXPECT_EQ(parsed, original);
}

/**
 * @brief 测试 uuidFromString 解析错误（短字符串）
 */
TEST(UuidUtilsTest, FromStringTooShort)
{
    Uuid result = uuidFromString("short");
    // 应该返回全零
    Uuid expected = {};
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试 uuidToStringWithDashes 格式
 */
TEST(UuidUtilsTest, ToStringWithDashes)
{
    Uuid uuid = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};

    std::string str = uuidToStringWithDashes(uuid);
    EXPECT_EQ(str.length(), 36u);
    EXPECT_EQ(str, "01234567-89ab-cdef-fedc-ba9876543210");
}

/**
 * @brief 测试 uuidFromMd5 版本设置
 *
 * UUID v3 的版本号应该在第 6 字节的高 4 位（值为 3）
 * 变体应该在第 8 字节的高 2 位（值为 2，即 0x80 掩码后）
 */
TEST(UuidUtilsTest, UuidFromMd5_VersionAndVariant)
{
    // 创建一个示例 MD5 哈希
    std::array<u8, 16> md5 = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10};

    Uuid uuid = uuidFromMd5(md5);

    // 检查版本号（第 6 字节高 4 位应该是 3）
    EXPECT_EQ((uuid[6] >> 4) & 0x0F, 3);

    // 检查变体（第 8 字节高 2 位应该是 10，即 0x80 掩码后）
    EXPECT_EQ((uuid[8] >> 6) & 0x03, 2);
}

/**
 * @brief 测试 generateOfflineUuid 与 Minecraft 兼容性
 *
 * 验证生成的离线 UUID 格式正确
 */
TEST(UuidUtilsTest, GenerateOfflineUuid_Format)
{
    Uuid uuid = generateOfflineUuid("Steve");

    // 检查版本号
    EXPECT_EQ((uuid[6] >> 4) & 0x0F, 3);

    // 检查变体
    EXPECT_EQ((uuid[8] >> 6) & 0x03, 2);

    // 转换为字符串应该有正确的长度
    std::string str = uuidToString(uuid);
    EXPECT_EQ(str.length(), 32u);
}

/**
 * @brief 测试离线 UUID 确定性
 *
 * 相同的用户名应该生成相同的 UUID
 */
TEST(UuidUtilsTest, GenerateOfflineUuid_Deterministic)
{
    Uuid uuid1 = generateOfflineUuid("Steve");
    Uuid uuid2 = generateOfflineUuid("Steve");

    EXPECT_EQ(uuid1, uuid2);
}

/**
 * @brief 测试不同用户名生成不同 UUID
 */
TEST(UuidUtilsTest, GenerateOfflineUuid_Different)
{
    Uuid uuid1 = generateOfflineUuid("Steve");
    Uuid uuid2 = generateOfflineUuid("Alex");

    EXPECT_NE(uuid1, uuid2);
}

/**
 * @brief 测试 Minecraft 离线 UUID 算法正确性
 *
 * 验证我们的实现与 Java 的 UUID.nameUUIDFromBytes 结果一致
 *
 * Java 测试代码：
 * UUID uuid = UUID.nameUUIDFromBytes("OfflinePlayer:Steve".getBytes(StandardCharsets.UTF_8));
 * System.out.println(uuid.toString());
 *
 * MD5("OfflinePlayer:Steve") = 5627dd98e6bebc21f8a8e92344183641
 * 转换为 UUID v3 后：5627dd98-e6be-3c21-b8a8-e92344183641
 */
TEST(UuidUtilsTest, MinecraftOfflineUuid_SpecialName)
{
    // 手动计算 "OfflinePlayer:Steve" 的 MD5
    std::string input = "OfflinePlayer:Steve";
    std::array<u8, 16> md5 = crypto::Md5::hash(input);
    Uuid uuid = uuidFromMd5(md5);

    // 预期的 UUID（带连字符格式）
    // 通过 Java UUID.nameUUIDFromBytes(("OfflinePlayer:" + "Steve").getBytes()) 获取
    // 注意：实际 Java 输出为 5627dd98-e6be-3c21-b8a8-e92344183641
    std::string expectedWithDashes = "5627dd98-e6be-3c21-b8a8-e92344183641";

    std::string actualWithDashes = uuidToStringWithDashes(uuid);
    EXPECT_EQ(actualWithDashes, expectedWithDashes);
}

/**
 * @brief 测试 generateOfflineUuid 与手动计算一致
 */
TEST(UuidUtilsTest, GenerateOfflineUuid_Consistency)
{
    Uuid uuid1 = generateOfflineUuid("TestPlayer");

    // 手动计算
    std::string input = "OfflinePlayer:TestPlayer";
    std::array<u8, 16> md5 = crypto::Md5::hash(input);
    Uuid uuid2 = uuidFromMd5(md5);

    EXPECT_EQ(uuid1, uuid2);
}

/**
 * @brief 测试空用户名
 */
TEST(UuidUtilsTest, GenerateOfflineUuid_EmptyUsername)
{
    // 空用户名也应该生成有效的 UUID
    Uuid uuid = generateOfflineUuid("");

    // 检查版本号
    EXPECT_EQ((uuid[6] >> 4) & 0x0F, 3);

    // 检查变体
    EXPECT_EQ((uuid[8] >> 6) & 0x03, 2);
}

/**
 * @brief 测试用户名大小写敏感性
 *
 * Minecraft 用户名是区分大小写的
 */
TEST(UuidUtilsTest, GenerateOfflineUuid_CaseSensitive)
{
    Uuid uuid1 = generateOfflineUuid("Steve");
    Uuid uuid2 = generateOfflineUuid("steve");
    Uuid uuid3 = generateOfflineUuid("STEVE");

    EXPECT_NE(uuid1, uuid2);
    EXPECT_NE(uuid1, uuid3);
    EXPECT_NE(uuid2, uuid3);
}

/**
 * @brief 测试 UUID 全零检测
 */
TEST(UuidUtilsTest, ZeroUuid)
{
    Uuid zero = {};

    for (const auto& byte : zero) {
        EXPECT_EQ(byte, 0);
    }

    std::string str = uuidToString(zero);
    EXPECT_EQ(str, "00000000000000000000000000000000");
}

} // namespace mc::util::test
