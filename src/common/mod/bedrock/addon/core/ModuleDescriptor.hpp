#pragma once

#include "common/core/Types.hpp"
#include <string>
#include <vector>

namespace mc::mod::bedrock::addon {

/**
 * @brief 模块版本号
 *
 * 兼容基岩版 [major, minor, patch] 格式
 */
struct ModuleVersion {
    i32 major = 0;
    i32 minor = 0;
    i32 patch = 0;

    [[nodiscard]] std::string toString() const
    {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }

    [[nodiscard]] bool operator==(const ModuleVersion& other) const
    {
        return major == other.major && minor == other.minor && patch == other.patch;
    }

    [[nodiscard]] bool operator!=(const ModuleVersion& other) const { return !(*this == other); }

    [[nodiscard]] bool operator<(const ModuleVersion& other) const
    {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        return patch < other.patch;
    }

    [[nodiscard]] bool operator<=(const ModuleVersion& other) const { return *this < other || *this == other; }

    [[nodiscard]] bool operator>(const ModuleVersion& other) const { return other < *this; }

    [[nodiscard]] bool operator>=(const ModuleVersion& other) const { return other <= *this; }

    [[nodiscard]] bool isCompatibleWith(const ModuleVersion& required) const
    {
        // 主版本号必须一致，次版本号和补丁版本号必须大于等于
        return major == required.major &&
            (minor > required.minor || (minor == required.minor && patch >= required.patch));
    }
};

/**
 * @brief 模块描述符
 *
 * 描述一个脚本模块的名称、版本和唯一标识
 */
struct ModuleDescriptor {
    std::string name;             // 模块名，如 "@minecraft/server"
    ModuleVersion version;        // 模块版本
    std::string uuid;             // 模块UUID
    bool isRuntimeModule = false; // 是否为运行时模块
};

} // namespace mc::mod::bedrock::addon
