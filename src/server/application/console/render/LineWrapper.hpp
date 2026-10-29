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
#include <string_view>
#include <vector>

namespace mc::server::console {

/**
 * @brief 单行文本的显示宽度（终端单元格数）
 *
 * 按东亚宽字符全角处理（与 FTXUI 的 `string_width` 口径一致），但**不含**制表符展开：
 * 制表符按 1 个单元格计。调用方若需要稳定宽度，应先经 expandTabs()。
 *
 * 非法 UTF-8 序列按 1 个单元格计，保证函数不会抛异常、不会越界。
 *
 * @param text UTF-8 单行文本（不含换行）
 * @return 显示宽度
 */
[[nodiscard]] usize displayWidth(std::string_view text);

/**
 * @brief 按显示宽度切分为若干显示行
 *
 * 用于日志区/事件区的预折行：FTXUI 的 `text()` 只按换行符分段、不做自动折行，超宽
 * 内容会被直接截断。先在紧凑行上折好，日志区每帧只做"取窗口 + 拼元素"。
 *
 * 规则：
 * - 优先在空格处断行（空格归左行并保留，保证复制出来的文本与原文一致）；
 * - 单词本身超过整行宽度时按字符硬切（中文等无空格文本即走这条）；
 * - 空文本产出一个空串元素（保证行数与内容一一对应）。
 *
 * @param text UTF-8 单行文本（不含换行）
 * @param maxWidth 每行最大显示宽度（≤ 0 时视为不折行）
 * @return 由左到右排列的显示行
 */
[[nodiscard]] std::vector<std::string> wrapByDisplayWidth(std::string_view text, usize maxWidth);

/**
 * @brief 把制表符替换为空格
 *
 * 日志文本里可能出现制表符，终端对其宽度定义不统一（FTXUI 按 1 单元格渲染），会让
 * 折行宽度与粘贴出来的文本对不上。统一展开为空格后，显示宽度与字符语义一致。
 *
 * @param text 原始文本
 * @param tabWidth 制表符展开的宽度
 * @return 展开后的文本
 */
[[nodiscard]] std::string expandTabs(std::string_view text, usize tabWidth = 4);

} // namespace mc::server::console
