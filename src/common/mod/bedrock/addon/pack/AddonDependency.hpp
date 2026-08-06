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

#include "common/mod/bedrock/addon/pack/PackVersion.hpp"

#include <string>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::mod::bedrock::addon {

/**
 * @brief 依赖声明
 *
 * 基岩版 manifest 的 `dependencies` 条目有两种形态：
 * 1. 包间依赖：`{"uuid": "...", "version": [1,0,0]}`——用 `uuid` 引用另一个行为包，`version` 为版本数组。
 * 2. 原生模块依赖：`{"module_name": "@minecraft/server", "version": "1.13.0-beta"}`——用 `module_name`
 *    引用内置原生模块，`version` 为字符串（含预发布标识）。
 *
 * 两者共存在同一个 `dependencies` 数组里，本结构同时承载这两种形态：`moduleName` 非空即为模块依赖，
 * `uuid` 非空即为包依赖。
 */
struct AddonDependency {
    std::string uuid;          // 包依赖的目标包 UUID（包间依赖填此项）
    std::string moduleName;    // 模块依赖的目标模块名，如 "@minecraft/server"（模块依赖填此项）
    PackVersion version;       // 包依赖的版本要求（数组形式）
    std::string versionString; // 模块依赖的版本字符串，如 "1.13.0-beta"（原样保留，含预发布标识）

    /**
     * @brief 是否为原生模块依赖
     * @return moduleName 非空且以 "@minecraft/" 开头时为真。
     */
    [[nodiscard]] bool isModuleDependency() const noexcept
    {
        return !moduleName.empty() && moduleName.starts_with("@minecraft/");
    }

    /**
     * @brief 从JSON对象解析依赖
     * @param j JSON对象
     * @return 解析后的依赖对象
     */
    [[nodiscard]] static AddonDependency fromJson(const nlohmann::json& j);
};

} // namespace mc::mod::bedrock::addon
