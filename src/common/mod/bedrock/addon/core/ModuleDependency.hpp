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
 */

#pragma once

#include "common/mod/bedrock/addon/core/ModuleDescriptor.hpp"
#include <string>

namespace mc::mod::bedrock::addon {

/**
 * @brief 模块依赖声明
 *
 * 描述一个模块对另一个模块的依赖关系
 */
struct ModuleDependency {
    std::string name;       // 依赖模块名，如 "@minecraft/server"
    ModuleVersion version;  // 最低版本要求
    std::string preRelease; // 预发布标识，如 "beta"

    [[nodiscard]] bool isNativeDependency() const
    {
        // 以 @minecraft/ 开头的是原生模块依赖
        return name.starts_with("@minecraft/");
    }

    [[nodiscard]] bool isPackDependency() const
    {
        // UUID 格式的是包间依赖
        return !name.empty() && name.size() == 36 && name[8] == '-' && name[13] == '-' && name[18] == '-' &&
            name[23] == '-';
    }
};

} // namespace mc::mod::bedrock::addon
