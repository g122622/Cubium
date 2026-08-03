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

#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <string>
#include <string_view>

namespace mc::command {

/**
 * @brief 字符串读取器
 *
 * 用于命令解析的游标式字符串读取器。
 * 支持回退操作，提供各种类型的解析方法。
 */
class StringReader {
public:
    static constexpr char SYNTAX_ESCAPE = '\\';
    static constexpr char SYNTAX_QUOTE = '"';

    explicit StringReader(std::string_view input)
        : m_input(input)
        , m_cursor(0)
    {}

    StringReader(const StringReader& other) = default;
    StringReader& operator=(const StringReader& other) = default;
    StringReader(StringReader&& other) noexcept = default;
    StringReader& operator=(StringReader&& other) noexcept = default;

    // ========== 基本访问 ==========

    [[nodiscard]] std::string_view getString() const noexcept { return m_input; }
    [[nodiscard]] i32 getCursor() const noexcept { return m_cursor; }
    [[nodiscard]] i32 getRemainingLength() const noexcept { return static_cast<i32>(m_input.length()) - m_cursor; }
    [[nodiscard]] i32 getTotalLength() const noexcept { return static_cast<i32>(m_input.length()); }

    [[nodiscard]] bool canRead(i32 length = 1) const noexcept
    {
        if (length < 0) {
            return false;
        }
        return static_cast<size_t>(m_cursor) + static_cast<size_t>(length) <= m_input.length();
    }

    [[nodiscard]] char peek() const noexcept { return canRead() ? m_input[static_cast<size_t>(m_cursor)] : '\0'; }

    [[nodiscard]] char peek(i32 offset) const noexcept
    {
        return canRead(offset + 1) ? m_input[static_cast<size_t>(m_cursor + offset)] : '\0';
    }

    [[nodiscard]] char read()
    {
        if (!canRead()) {
            throw CommandException(CommandErrorType::StringExpected, "Expected more input", m_cursor);
        }
        return m_input[static_cast<size_t>(m_cursor++)];
    }

    void skip()
    {
        if (canRead()) {
            m_cursor++;
        }
    }

    void skip(i32 count) noexcept { m_cursor += count; }

    void setCursor(i32 cursor) noexcept { m_cursor = cursor; }

    // ========== 空白处理 ==========

    void skipWhitespace()
    {
        while (canRead() && isWhitespace(peek())) {
            skip();
        }
    }

    [[nodiscard]] static bool isWhitespace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

    // ========== 字符串读取 ==========

    /**
     * @brief 读取直到遇到空白或字符串结束
     */
    [[nodiscard]] std::string readUnquotedString()
    {
        i32 start = m_cursor;
        while (canRead() && !isWhitespace(peek()) && peek() != SYNTAX_QUOTE) {
            skip();
        }
        const size_t startIndex = static_cast<size_t>(start);
        const size_t endIndex = static_cast<size_t>(m_cursor);
        return std::string(m_input.substr(startIndex, endIndex - startIndex));
    }

    /**
     * @brief 读取引号字符串
     * @throws CommandException 如果字符串未正确闭合
     */
    [[nodiscard]] std::string readQuotedString()
    {
        if (!canRead()) {
            return "";
        }

        char next = peek();
        if (next != SYNTAX_QUOTE) {
            throw CommandException(
                CommandErrorType::StringQuotedExpected, "Expected quote at start of string", m_cursor);
        }

        skip(); // 跳过开始引号

        std::string result;
        bool escaped = false;

        while (canRead()) {
            char c = read();

            if (escaped) {
                if (c == SYNTAX_QUOTE || c == SYNTAX_ESCAPE) {
                    result += c;
                } else {
                    // 无效的转义，回退并抛出异常
                    setCursor(getCursor() - 1);
                    throw CommandException(CommandErrorType::StringQuotedExpected, "Invalid escape sequence", m_cursor);
                }
                escaped = false;
            } else if (c == SYNTAX_ESCAPE) {
                escaped = true;
            } else if (c == SYNTAX_QUOTE) {
                return result;
            } else {
                result += c;
            }
        }

        throw CommandException(CommandErrorType::StringQuotedExpected, "Unclosed quoted string", m_cursor);
    }

    /**
     * @brief 读取字符串（自动检测是否带引号）
     */
    [[nodiscard]] std::string readString()
    {
        if (!canRead()) {
            return "";
        }

        char next = peek();
        if (next == SYNTAX_QUOTE) {
            return readQuotedString();
        }
        return readUnquotedString();
    }

    // ========== 数字读取 ==========

    /**
     * @brief 读取布尔值
     */
    [[nodiscard]] bool readBool()
    {
        i32 start = m_cursor;
        std::string value = readUnquotedString();

        if (value == "true") {
            return true;
        } else if (value == "false") {
            return false;
        }

        setCursor(start);
        throw CommandException(CommandErrorType::BoolExpected, "Expected boolean (true/false)", start);
    }

    /**
     * @brief 读取整数
     */
    [[nodiscard]] i32 readInt()
    {
        i32 start = m_cursor;
        skipWhitespace();

        bool negative = false;
        if (canRead() && peek() == '-') {
            negative = true;
            skip();
        }

        i32 result = 0;
        bool hasDigits = false;

        while (canRead() && isDigit(peek())) {
            result = result * 10 + (peek() - '0');
            skip();
            hasDigits = true;
        }

        if (!hasDigits) {
            throw CommandException(CommandErrorType::IntegerExpected, "Expected integer", start);
        }

        return negative ? -result : result;
    }

    /**
     * @brief 读取带范围检查的整数
     */
    [[nodiscard]] i32 readInt(i32 min, i32 max)
    {
        i32 start = m_cursor;
        i32 result = readInt();

        if (result < min) {
            throw CommandException(
                CommandErrorType::IntegerTooLow, "Integer must be at least " + std::to_string(min), start);
        }
        if (result > max) {
            throw CommandException(
                CommandErrorType::IntegerTooHigh, "Integer must be at most " + std::to_string(max), start);
        }

        return result;
    }

    /**
     * @brief 读取浮点数
     */
    [[nodiscard]] f64 readDouble()
    {
        i32 start = m_cursor;
        skipWhitespace();

        bool negative = false;
        if (canRead() && peek() == '-') {
            negative = true;
            skip();
        }

        f64 result = 0.0;
        bool hasDigits = false;
        bool hasDecimal = false;
        f64 decimalPlace = 0.1;

        while (canRead() && (isDigit(peek()) || peek() == '.')) {
            if (peek() == '.') {
                if (hasDecimal) {
                    break; // 第二个小数点，停止
                }
                hasDecimal = true;
                skip();
                continue;
            }

            if (hasDecimal) {
                result += (peek() - '0') * decimalPlace;
                decimalPlace *= 0.1;
            } else {
                result = result * 10.0 + (peek() - '0');
            }
            skip();
            hasDigits = true;
        }

        if (!hasDigits) {
            throw CommandException(CommandErrorType::FloatExpected, "Expected float", start);
        }

        return negative ? -result : result;
    }

    /**
     * @brief 读取浮点数（单精度）
     */
    [[nodiscard]] f32 readFloat() { return static_cast<f32>(readDouble()); }

    /**
     * @brief 读取带范围检查的浮点数
     */
    [[nodiscard]] f64 readDouble(f64 min, f64 max)
    {
        i32 start = m_cursor;
        f64 result = readDouble();

        if (result < min) {
            throw CommandException(
                CommandErrorType::FloatTooLow, "Float must be at least " + std::to_string(min), start);
        }
        if (result > max) {
            throw CommandException(
                CommandErrorType::FloatTooHigh, "Float must be at most " + std::to_string(max), start);
        }

        return result;
    }

    // ========== 辅助方法 ==========

    [[nodiscard]] static bool isDigit(char c) { return c >= '0' && c <= '9'; }

    [[nodiscard]] static bool isLetter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

    [[nodiscard]] static bool isAllowedInUnquotedString(char c)
    {
        return isLetter(c) || isDigit(c) || c == '_' || c == '-' || c == '.' || c == '+';
    }

    /**
     * @brief 读取剩余内容
     */
    [[nodiscard]] std::string getRemaining() const
    {
        return std::string(m_input.substr(static_cast<size_t>(m_cursor)));
    }

    /**
     * @brief 从当前位置读取直到指定字符
     */
    [[nodiscard]] std::string readUntil(char terminator)
    {
        i32 start = m_cursor;
        while (canRead() && peek() != terminator) {
            skip();
        }
        const size_t startIndex = static_cast<size_t>(start);
        const size_t endIndex = static_cast<size_t>(m_cursor);
        return std::string(m_input.substr(startIndex, endIndex - startIndex));
    }

    /**
     * @brief 尝试匹配字符串
     * @return 是否匹配成功
     */
    bool tryRead(std::string_view expected) noexcept
    {
        if (!canRead(static_cast<i32>(expected.length()))) {
            return false;
        }

        i32 originalCursor = m_cursor;
        for (char c : expected) {
            if (peek() != c) {
                m_cursor = originalCursor;
                return false;
            }
            skip();
        }
        return true;
    }

    /**
     * @brief 期望读取指定字符
     */
    void expect(char c)
    {
        if (!canRead() || peek() != c) {
            throw CommandException(
                CommandErrorType::DispatcherExpectedLiteral, "Expected '" + std::string(1, c) + "'", m_cursor);
        }
        skip();
    }

private:
    std::string_view m_input;
    i32 m_cursor;
};

} // namespace mc::command
