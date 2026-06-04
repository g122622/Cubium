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

#include "common/mod/bedrock/addon/plugin/ScriptPackPermissions.hpp"

#include <string>
#include <vector>

namespace mc::mod::bedrock::addon {

/**
 * @brief 脚本包配置
 *
 * 解析自行为包manifest.json，控制脚本模块的加载行为。
 * 包含模块名称、允许/排除列表和权限。
 */
class ScriptPackConfiguration {
public:
    ScriptPackConfiguration() = default;

    /**
     * @brief 移动构造函数
     */
    ScriptPackConfiguration(ScriptPackConfiguration&& other) noexcept;

    /**
     * @brief 移动赋值运算符
     */
    ScriptPackConfiguration& operator=(ScriptPackConfiguration&& other) noexcept;

    /**
     * @brief 设置允许的模块列表
     * @param modules 模块名称列表（如"@minecraft/server"）
     */
    void setAllowedModules(std::vector<std::string> modules);

    /**
     * @brief 设置排除的模块列表
     * @param modules 模块名称列表
     */
    void setExcludedModules(std::vector<std::string> modules);

    /**
     * @brief 设置脚本权限
     */
    void setPermissions(ScriptPackPermissions permissions);

    /**
     * @brief 检查指定模块是否被允许加载
     * @param moduleName 模块名称
     * @return 如果允许列表为空则全部允许，否则检查是否在列表中
     */
    [[nodiscard]] bool isModuleAllowed(const std::string& moduleName) const;

    /**
     * @brief 检查指定模块是否被排除
     * @param moduleName 模块名称
     */
    [[nodiscard]] bool isModuleExcluded(const std::string& moduleName) const;

    /**
     * @brief 获取权限
     */
    [[nodiscard]] const ScriptPackPermissions& permissions() const;

private:
    std::vector<std::string> m_allowedModules;
    std::vector<std::string> m_excludedModules;
    ScriptPackPermissions m_permissions;
};

} // namespace mc::mod::bedrock::addon
