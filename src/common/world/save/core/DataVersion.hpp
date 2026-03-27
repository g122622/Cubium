#pragma once

#include "../../../core/Types.hpp"

namespace mc::world::save {

/**
 * @brief 数据版本常量
 *
 * 用于存档版本兼容性检查。
 * 每个 Minecraft 版本都有唯一的数据版本号。
 *
 * 参考 MC 1.16.5 SharedConstants.getVersion().getWorldVersion()
 */
namespace DataVersion {

/// 当前支持的数据版本（MC 1.16.5 = 2586）
constexpr i32 CURRENT = 2586;

/// 未知版本（用于旧存档）
constexpr i32 UNKNOWN = -1;

/// MC 1.12.2
constexpr i32 V1_12_2 = 1451;

/// MC 1.13
constexpr i32 V1_13 = 1519;

/// MC 1.13.2
constexpr i32 V1_13_2 = 1628;

/// MC 1.14
constexpr i32 V1_14 = 1952;

/// MC 1.15
constexpr i32 V1_15 = 2225;

/// MC 1.16
constexpr i32 V1_16 = 2566;

/// MC 1.16.5
constexpr i32 V1_16_5 = 2586;

/// MC 1.17
constexpr i32 V1_17 = 2730;

/// MC 1.18
constexpr i32 V1_18 = 2860;

/**
 * @brief 检查数据版本是否兼容
 *
 * @param version 存档的数据版本
 * @return 如果可以加载返回 true
 */
[[nodiscard]] inline bool isCompatible(i32 version) {
    // 目前只支持 1.16.5 版本
    // 未来可以添加版本升级逻辑
    return version == CURRENT || version == UNKNOWN;
}

/**
 * @brief 获取版本名称
 *
 * @param version 数据版本号
 * @return 版本名称字符串
 */
[[nodiscard]] inline const char* getVersionName(i32 version) {
    switch (version) {
        case V1_12_2: return "1.12.2";
        case V1_13: return "1.13";
        case V1_13_2: return "1.13.2";
        case V1_14: return "1.14";
        case V1_15: return "1.15";
        case V1_16: return "1.16";
        case V1_16_5: return "1.16.5";
        case V1_17: return "1.17";
        case V1_18: return "1.18";
        case UNKNOWN: return "Unknown";
        default:
            if (version > CURRENT) {
                return "Newer";
            }
            return "Older";
    }
}

} // namespace DataVersion

} // namespace mc::world::save
