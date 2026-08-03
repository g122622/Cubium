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

#include "StringTemplate.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace function {

/// 命令行最大字符数限制（与 MC 原版一致）
namespace {
constexpr size_t MAX_COMMAND_LINE_LENGTH = 2000000;
}

bool StringTemplate::isValidVariableName(const std::string& name) noexcept
{
    if (name.empty()) {
        return false;
    }
    for (char c : name) {
        const bool isAlpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        const bool isDigit = (c >= '0' && c <= '9');
        const bool isUnderscore = (c == '_');
        if (!isAlpha && !isDigit && !isUnderscore) {
            return false;
        }
    }
    return true;
}

StringTemplate StringTemplate::fromString(const std::string& input)
{
    std::vector<std::string> segments;
    std::vector<std::string> variables;

    const auto length = input.size();
    // 找第一个 '$' (ASCII 36)
    size_t k = input.find('$');
    size_t j = 0; // 上次切割位置

    while (k != std::string::npos) {
        // '$' 后必须紧跟 '(' (ASCII 40)
        if (k != length - 1 && input[k + 1] == '(') {
            // 加入前面的文本片段
            segments.emplace_back(input.substr(j, k - j));

            // 找匹配的 ')' (ASCII 41)
            const size_t closePos = input.find(')', k + 1);
            if (closePos == std::string::npos) {
                throw std::invalid_argument("Unterminated macro variable");
            }

            // 提取变量名（位于 $( 之后、) 之前）
            const std::string varName = input.substr(k + 2, closePos - (k + 2));
            if (!isValidVariableName(varName)) {
                throw std::invalid_argument("Invalid macro variable name '" + varName + "'");
            }
            variables.push_back(varName);

            j = closePos + 1;
            k = input.find('$', j);
        } else {
            // '$' 不跟 '('，跳过找下一个
            k = input.find('$', k + 1);
        }
    }

    if (variables.empty()) {
        throw std::invalid_argument("No variables in macro");
    }

    // 加入尾部文本片段（即使为空也加入，保持 segments.size() == variables.size() + 1）
    segments.emplace_back(input.substr(j));

    return StringTemplate(std::move(segments), std::move(variables));
}

std::string StringTemplate::substitute(const std::vector<std::string>& values) const
{
    if (values.size() != m_variables.size()) {
        throw std::invalid_argument("StringTemplate::substitute: values count does not match variables count");
    }

    std::string result;
    for (size_t i = 0; i < m_variables.size(); ++i) {
        result += m_segments[i];
        result += values[i];
        if (result.size() > MAX_COMMAND_LINE_LENGTH) {
            throw std::runtime_error("Command too long: " + std::to_string(result.size()) + " characters");
        }
    }
    // 追加尾部片段（segments 比 variables 多一个）
    if (m_segments.size() > m_variables.size()) {
        result += m_segments.back();
        if (result.size() > MAX_COMMAND_LINE_LENGTH) {
            throw std::runtime_error("Command too long: " + std::to_string(result.size()) + " characters");
        }
    }
    return result;
}

} // namespace function
} // namespace mc
