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

/**
 * @file GameRule.cpp
 * @brief 游戏规则类型实现
 */

#include "GameRule.hpp"
#include "common/core/Types.hpp"
#include <exception>
#include <string>

namespace mc::world::gamerule {

// ============================================================================
// BooleanGameRuleValue 特化
// ============================================================================

template <>
[[nodiscard]] std::string GameRuleValue<bool>::toString() const
{
    return m_value ? "true" : "false";
}

template <>
bool GameRuleValue<bool>::fromString(const std::string& value)
{
    if (value == "true" || value == "TRUE" || value == "1") {
        m_value = true;
        return true;
    } else if (value == "false" || value == "FALSE" || value == "0") {
        m_value = false;
        return true;
    }
    // 其他情况默认为 false
    m_value = false;
    return false;
}

// ============================================================================
// IntegerGameRuleValue 特化
// ============================================================================

template <>
[[nodiscard]] std::string GameRuleValue<i32>::toString() const
{
    return std::to_string(m_value);
}

template <>
bool GameRuleValue<i32>::fromString(const std::string& value)
{
    if (value.empty()) {
        m_value = 0;
        return false;
    }

    try {
        m_value = std::stoi(value);
        return true;
    }
    catch (const std::exception&) {
        // 解析失败，保持原值不变
        return false;
    }
}

} // namespace mc::world::gamerule
