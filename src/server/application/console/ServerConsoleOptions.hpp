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

namespace mc::server::console {

/// 日志区保留行数。约数 MB 内存，足以回看一次完整启动与近期运行。
inline constexpr usize LOG_LINE_CAPACITY = 2000;

/// 事件区（聊天/上下线/命令回执/业务事件）保留条数。
inline constexpr usize EVENT_CAPACITY = 500;

/**
 * @brief 交互式终端的构造参数
 *
 * 遵循项目规范：所有字段由调用方显式给出，不提供默认值。
 */
struct ServerConsoleOptions {
    /// 标题栏显示的服务端名称。
    std::string serverName;

    /// 版本号（显示在服务器信息分段）。
    std::string version;

    /// 世界名（server options 中的 worldName）。
    std::string worldName;

    /// 监听端口（显示在服务器信息分段）。
    u16 port = 0;

    /// 日志区保留行数。
    usize logCapacity = 0;

    /// 事件区保留条数。
    usize eventCapacity = 0;
};

} // namespace mc::server::console
