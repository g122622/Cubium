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

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace mc::util {

// ============================================================================
// UTF-8 与宽字符 / 路径之间的转换
//
// Windows 上进程命令行与文件系统 API 都是 UTF-16，而本项目的字符串、日志、
// gflags 参数一律是 UTF-8。两者之间必须显式转换，不能依赖窄字符接口：
// CRT 把宽命令行转成窄 argv 时按**当前 ANSI 代码页**编码，代码页外的字符（如
// 中文路径在非中文区域设置下）会直接变成 '?' 且不可恢复。
// ============================================================================

/**
 * @brief 把 UTF-8 文本转换成宽字符串
 *
 * @param text UTF-8 文本
 * @return 转换后的宽字符串；输入非法 UTF-8 时返回空串
 */
[[nodiscard]] std::wstring utf8ToWide(std::string_view text);

/**
 * @brief 把宽字符串转换成 UTF-8 文本
 *
 * @param text 宽字符串
 * @return 转换后的 UTF-8 文本；输入含未配对代理项时返回空串
 */
[[nodiscard]] std::string wideToUtf8(std::wstring_view text);

/**
 * @brief 把 UTF-8 文本解释为文件系统路径
 *
 * 直接 `std::filesystem::path(std::string)` 在 Windows 上按 ANSI 代码页解码，
 * 含非 ASCII 的路径会失真。凡是路径来源是 UTF-8 文本（命令行参数、配置文件、
 * 网络/数据包内容）的地方都应经此构造。
 *
 * @param text UTF-8 文本
 * @return 路径；输入非法 UTF-8 时返回空路径
 */
[[nodiscard]] std::filesystem::path pathFromUtf8(std::string_view text);

/**
 * @brief 把文件系统路径转换成 UTF-8 文本
 *
 * 与 pathFromUtf8 互逆。`std::filesystem::path::string()` 在 Windows 上同样按
 * ANSI 代码页编码，不能直接用于日志、JSON、网络等需要 UTF-8 的出口。
 *
 * @param path 文件系统路径
 * @return UTF-8 文本
 */
[[nodiscard]] std::string pathToUtf8(const std::filesystem::path& path);

/**
 * @brief 获取进程的完整命令行，按 UTF-8 拆分成参数数组
 *
 * 只在 Windows 上需要：CRT 构造的 `argv` 已经把宽命令行按 ANSI 代码页压扁，
 * 代码页外的字符无法还原。本方法改从 `GetCommandLineW()` 取原始宽字符命令行，
 * 用与 CRT 相同的分词规则拆分后逐个转成 UTF-8，因此非 ASCII 参数可完整保留。
 *
 * - 非 Windows 平台直接返回空 vector（调用方应继续使用 main 的 argv）。
 * - 返回值[0] 是程序自身路径。
 * - 解析失败时返回空 vector。
 *
 * @return UTF-8 参数数组
 */
[[nodiscard]] std::vector<std::string> commandLineArgumentsUtf8();

} // namespace mc::util
