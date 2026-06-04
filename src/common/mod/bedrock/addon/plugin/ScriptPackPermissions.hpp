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
 * @brief 脚本包权限
 *
 * 定义行为包脚本允许执行的操作。
 * 权限在manifest.json的capabilities字段中声明。
 */
enum class ScriptPermission : u32 {
    /// 允许使用eval()
    AllowEval = 1 << 0,
};

/**
 * @brief 脚本包权限集合
 *
 * 基于位掩码的权限管理，解析自manifest的capabilities字段。
 */
class ScriptPackPermissions {
public:
    ScriptPackPermissions() noexcept = default;

    /**
     * @brief 从capabilities字符串列表构造权限
     * @param capabilities manifest中声明的capabilities
     */
    explicit ScriptPackPermissions(const std::vector<std::string>& capabilities);

    /**
     * @brief 设置权限
     */
    void setPermission(ScriptPermission perm, bool enabled = true) noexcept;

    /**
     * @brief 检查是否拥有指定权限
     */
    [[nodiscard]] bool hasPermission(ScriptPermission perm) const noexcept;

    /**
     * @brief 获取原始权限位掩码
     */
    [[nodiscard]] u32 rawFlags() const noexcept { return m_flags; }

private:
    u32 m_flags = 0;
};

} // namespace mc::mod::bedrock::addon
