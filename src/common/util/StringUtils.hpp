#pragma once

#include "../core/Types.hpp"
#include <algorithm>
#include <cctype>
#include <string>

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
[[nodiscard]] inline String toLowerAscii(StringView text)
{
    String lowered;
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
[[nodiscard]] inline bool isNumeric(StringView text, bool allowSign = true)
{
    if (text.empty()) {
        return false;
    }

    size_t start = 0;
    if (allowSign && (text[0] == '+' || text[0] == '-')) {
        start = 1;
        if (text.size() == 1) {
            return false;  // 只有符号
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
