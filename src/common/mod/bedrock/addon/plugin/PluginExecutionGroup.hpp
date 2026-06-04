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

namespace mc::mod::bedrock::addon {

/**
 * @brief 插件执行分组
 *
 * 控制插件脚本在服务器生命周期的哪个阶段执行。
 * 分组决定了脚本的初始化时机和可用API范围。
 */
enum class PluginExecutionGroup : u8 {
    /// 在包加载之前执行（早期初始化，有限API）
    PrePackLoad = 0,
    /// 服务器启动时执行（标准时机，完整API）
    ServerStart = 1,
    /// 客户端等级加载时执行（世界级脚本）
    ClientLevel = 2,
};

/**
 * @brief 获取执行分组的显示名称
 */
[[nodiscard]] const char* pluginExecutionGroupName(PluginExecutionGroup group) noexcept;

} // namespace mc::mod::bedrock::addon
