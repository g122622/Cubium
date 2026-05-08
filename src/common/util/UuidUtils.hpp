#pragma once

#include "common/command/ICommandSource.hpp"  // for Uuid and UuidHash
#include <string>
#include <sstream>
#include <iomanip>

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
inline Uuid uuidFromString(const std::string& uuidStr) {
    Uuid result{};
    if (uuidStr.length() < 32) {
        return result;
    }

    for (size_t i = 0; i < 16; ++i) {
        std::string byteStr = uuidStr.substr(i * 2, 2);
        try {
            result[i] = static_cast<u8>(std::stoul(byteStr, nullptr, 16));
        } catch (...) {
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
inline std::string uuidToString(const Uuid& uuid) {
    std::ostringstream oss;
    for (const auto& byte : uuid) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return oss.str();
}

} // namespace util
} // namespace mc
