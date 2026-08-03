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
 * @file UuidUtils.hpp
 * @brief UUID 工具函数
 *
 * 提供 UUID 格式转换、离线模式 UUID 生成和随机 UUID v4 生成功能。
 *
 * Minecraft 1.16.5 离线模式 UUID 生成算法：
 * UUID.nameUUIDFromBytes(("OfflinePlayer:" + username).getBytes(StandardCharsets.UTF_8))
 * 这会生成一个 UUID v3（基于 MD5 哈希的命名空间 UUID）。
 *
 * 随机 UUID v4 生成参考 MC 1.21.11: Mth.createInsecureUUID(RandomSource)，
 * 使用非加密安全的随机数生成器，设置版本号为 4、变体为 RFC 4122。
 */
#pragma once

#include "common/command/ICommandSource.hpp" // for Uuid and UuidHash
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include <array>
#include <cstddef>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>

namespace mc {
namespace util {

/**
 * @brief 将字符串 UUID 转换为数组格式
 *
 * Entity 使用字符串 UUID（十六进制），而 Raid 使用数组格式。
 * 此函数将十六进制字符串转换为 16 字节数组。
 *
 * @param uuidStr 十六进制格式的 UUID 字符串（32 个字符）
 * @return 16 字节的 UUID 数组，如果解析失败则返回全零数组
 */
inline Uuid uuidFromString(const std::string& uuidStr)
{
    Uuid result{};
    if (uuidStr.length() < 32) {
        return result;
    }

    for (size_t i = 0; i < 16; ++i) {
        std::string byteStr = uuidStr.substr(i * 2, 2);
        try {
            result[i] = static_cast<u8>(std::stoul(byteStr, nullptr, 16));
        }
        catch (...) {
            return Uuid{};
        }
    }
    return result;
}

/**
 * @brief 将数组格式 UUID 转换为字符串格式
 *
 * @param uuid 16 字节的 UUID 数组
 * @return 十六进制格式的 UUID 字符串（32 个小写字符）
 */
inline std::string uuidToString(const Uuid& uuid)
{
    std::ostringstream oss;
    for (const auto& byte : uuid) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return oss.str();
}

/**
 * @brief 将 UUID 转换为带连字符的标准格式
 *
 * @param uuid 16 字节的 UUID 数组
 * @return 带连字符的 UUID 字符串（格式：xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx）
 */
inline std::string uuidToStringWithDashes(const Uuid& uuid)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    // 时间戳低 32 位 (time_low)
    for (size_t i = 0; i < 4; ++i) {
        oss << std::setw(2) << static_cast<int>(uuid[i]);
    }
    oss << '-';

    // 时间戳中间 16 位 (time_mid)
    for (size_t i = 4; i < 6; ++i) {
        oss << std::setw(2) << static_cast<int>(uuid[i]);
    }
    oss << '-';

    // 时间戳高 16 位 + 版本 (time_hi_and_version)
    for (size_t i = 6; i < 8; ++i) {
        oss << std::setw(2) << static_cast<int>(uuid[i]);
    }
    oss << '-';

    // 时钟序列 + 变体 (clock_seq_hi_and_reserved, clock_seq_low)
    for (size_t i = 8; i < 10; ++i) {
        oss << std::setw(2) << static_cast<int>(uuid[i]);
    }
    oss << '-';

    // 节点 ID (node)
    for (size_t i = 10; i < 16; ++i) {
        oss << std::setw(2) << static_cast<int>(uuid[i]);
    }

    return oss.str();
}

/**
 * @brief 从 MD5 哈希生成 UUID v3
 *
 * 将 MD5 哈希结果转换为 UUID v3 格式。
 * UUID v3 的格式要求：
 * - 第 6 字节的高 4 位设置版本号为 3 (0x30)
 * - 第 8 字节的高 2 位设置变体为 RFC 4122 (0x80)
 *
 * @param md5Hash MD5 哈希结果（16 字节）
 * @return UUID v3 数组
 */
inline Uuid uuidFromMd5(const std::array<u8, 16>& md5Hash)
{
    Uuid uuid = md5Hash;

    // 设置版本号为 3 (name-based UUID using MD5)
    // 版本号位于 time_hi_and_version 字段的高 4 位
    // uuid[6] = (uuid[6] & 0x0F) | 0x30
    uuid[6] = (uuid[6] & 0x0F) | 0x30;

    // 设置变体为 RFC 4122
    // 变体位于 clock_seq_hi_and_reserved 字段的高 2 位
    // uuid[8] = (uuid[8] & 0x3F) | 0x80
    uuid[8] = (uuid[8] & 0x3F) | 0x80;

    return uuid;
}

/**
 * @brief 生成 Minecraft 离线模式 UUID
 *
 * 参考 MC 1.16.5 PlayerEntity.getOfflineUUID():
 * UUID.nameUUIDFromBytes(("OfflinePlayer:" + username).getBytes(StandardCharsets.UTF_8))
 *
 * 这是一个 UUID v3（基于 MD5 哈希的命名空间 UUID）。
 *
 * @param username 玩家用户名
 * @return 离线模式 UUID（16 字节数组）
 */
inline Uuid generateOfflineUuid(const std::string& username)
{
    // 前向声明 MD5 哈希函数（实际实现在 Md5.hpp 中）
    // 这里需要包含 Md5.hpp，但为了保持头文件独立性，我们使用外部函数
    extern std::array<u8, 16> computeMd5Hash(const std::string& data);

    std::string input = "OfflinePlayer:" + username;
    std::array<u8, 16> md5Hash = computeMd5Hash(input);
    return uuidFromMd5(md5Hash);
}

/**
 * @brief 生成随机 UUID v4
 *
 * 参考 MC 1.21.11: Mth.createInsecureUUID(RandomSource)。
 * 使用非加密安全的随机数生成器，设置版本号为 4、变体为 RFC 4122。
 *
 * UUID v4 格式：
 * - 第 6 字节的高 4 位设置版本号为 4 (0x40)
 * - 第 8 字节的高 2 位设置变体为 RFC 4122 (0x80)
 *
 * @param random 随机数生成器
 * @return UUID v4 数组（16 字节）
 */
inline Uuid generateRandomUuid(math::Random& random)
{
    // 生成两个 64 位随机数作为 UUID 的 MSB 和 LSB
    const u64 msb = random.nextU64();
    const u64 lsb = random.nextU64();

    Uuid uuid{};

    // 将 MSB 写入前 8 字节（大端序）
    for (size_t i = 0; i < 8; ++i) {
        uuid[i] = static_cast<u8>((msb >> (56 - i * 8)) & 0xFF);
    }

    // 将 LSB 写入后 8 字节（大端序）
    for (size_t i = 0; i < 8; ++i) {
        uuid[8 + i] = static_cast<u8>((lsb >> (56 - i * 8)) & 0xFF);
    }

    // 设置版本号为 4 (random-based UUID)
    // uuid[6] 的高 4 位设置为 0b0100 (版本 4)
    uuid[6] = (uuid[6] & 0x0F) | 0x40;

    // 设置变体为 RFC 4122
    // uuid[8] 的高 2 位设置为 0b10
    uuid[8] = (uuid[8] & 0x3F) | 0x80;

    return uuid;
}

/**
 * @brief 将 UUID 转换为字符串（用于 Entity::uuid() 兼容）
 *
 * @param uuid 16 字节的 UUID 数组
 * @return 32 个十六进制字符的小写字符串
 */
inline std::string uuidToHexString(const Uuid& uuid)
{
    return uuidToString(uuid);
}

} // namespace util
} // namespace mc
