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

    [[nodiscard]] std::string toString() const noexcept
    {
        return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    }

    [[nodiscard]] bool operator==(const ModuleVersion& other) const noexcept
    {
        return major == other.major && minor == other.minor && patch == other.patch;
    }

    [[nodiscard]] bool operator!=(const ModuleVersion& other) const noexcept { return !(*this == other); }

    [[nodiscard]] bool operator<(const ModuleVersion& other) const noexcept
    {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        return patch < other.patch;
    }

    [[nodiscard]] bool operator<=(const ModuleVersion& other) const noexcept { return *this < other || *this == other; }

    [[nodiscard]] bool operator>(const ModuleVersion& other) const noexcept { return other < *this; }

    [[nodiscard]] bool operator>=(const ModuleVersion& other) const noexcept { return other <= *this; }

    [[nodiscard]] bool isCompatibleWith(const ModuleVersion& required) const noexcept
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
