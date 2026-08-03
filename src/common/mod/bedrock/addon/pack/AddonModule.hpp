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
#include "common/mod/bedrock/addon/pack/PackVersion.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace mc::mod::bedrock::addon {

/**
 * @brief 模块类型枚举
 */
enum class AddonModuleType : u8 {
    Script,        // 脚本模块
    Data,          // 数据模块
    Resources,     // 资源模块
    SkinPack,      // 皮肤包模块
    WorldTemplate, // 世界模板
    Unknown
};

/**
 * @brief 模块声明
 *
 * 表示清单中的一个模块声明
 */
struct AddonModule {
    AddonModuleType type = AddonModuleType::Unknown;
    std::string uuid;
    PackVersion version;
    std::string name;                    // 仅用于脚本模块
    std::string entry;                   // 脚本入口点，如 "scripts/main.js"
    std::optional<std::string> language; // 脚本语言，如 "javascript"

    /**
     * @brief 从字符串解析模块类型
     * @param typeStr 类型字符串
     * @return 对应的模块类型枚举值
     */
    static AddonModuleType parseType(std::string_view typeStr) noexcept;

    /**
     * @brief 将模块类型转换为字符串
     * @return 类型字符串
     */
    [[nodiscard]] const char* typeToString() const noexcept;
};

} // namespace mc::mod::bedrock::addon
