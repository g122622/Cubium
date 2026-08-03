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

#include "../core/Types.hpp"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>

namespace mc::util {

/**
 * @brief 将字符串转换为小写（ASCII）
 *
 * 仅处理 ASCII 字符，非 ASCII 字符保持不变。
 * 用于不区分大小写的字符串比较和搜索。
 *
 * @param text 输入字符串
 * @return 转换后的小写字符串
 */
[[nodiscard]] inline std::string toLowerAscii(std::string_view text)
{
    std::string lowered;
    lowered.reserve(text.size());
    for (const char character : text) {
        lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
    }
    return lowered;
}

/**
 * @brief 检查字符串是否只包含数字
 *
 * @param text 输入字符串
 * @param allowSign 是否允许前导符号（+/-）
 * @return 是否只包含数字
 */
[[nodiscard]] inline bool isNumeric(std::string_view text, bool allowSign = true)
{
    if (text.empty()) {
        return false;
    }

    size_t start = 0;
    if (allowSign && (text[0] == '+' || text[0] == '-')) {
        start = 1;
        if (text.size() == 1) {
            return false; // 只有符号
        }
    }

    for (size_t i = start; i < text.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) {
            return false;
        }
    }
    return true;
}

} // namespace mc::util
