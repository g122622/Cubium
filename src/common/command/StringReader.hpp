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
#include <limits>
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

    [[nodiscard]] std::string_view getString() const noexcept
    {
        return m_input;
    }

    [[nodiscard]] i32 getCursor() const noexcept
    {
        return m_cursor;
    }

    [[nodiscard]] i32 getRemainingLength() const noexcept
    {
        return static_cast<i32>(m_input.length()) - m_cursor;
    }

    [[nodiscard]] i32 getTotalLength() const noexcept
    {
        return static_cast<i32>(m_input.length());
    }

    [[nodiscard]] bool canRead(i32 length = 1) const noexcept
    {
        if (length < 0) {
            return false;
        }
        return static_cast<size_t>(m_cursor) + static_cast<size_t>(length) <= m_input.length();
    }

    /// 游标是否已停在末尾（brigadier 的 `canRead()` / `getRemainingLength() == 0`）。
    /// 本类的 {@link canRead} 语义是「还能再读 length 个字符」，游标在末尾时仍为 true。
    [[nodiscard]] bool atEnd() const noexcept
    {
        return m_cursor >= getTotalLength();
    }

    [[nodiscard]] char peek() const noexcept
    {
        return canRead() ? m_input[static_cast<size_t>(m_cursor)] : '\0';
    }

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

    void skip(i32 count) noexcept
    {
        m_cursor += count;
    }

    void setCursor(i32 cursor) noexcept
    {
        m_cursor = cursor;
    }

    // ========== 空白处理 ==========

    void skipWhitespace()
    {
        while (canRead() && isWhitespace(peek())) {
            skip();
        }
    }

    [[nodiscard]] static bool isWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

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

    /**
     * @brief 读取一个「词」（brigadier StringReader.getString 的语义）
     *
     * 本文件的 {@link readUnquotedString} 读的是「到空白为止的整段」（与原版 getUnquotedString
     * 不同），命令参数普遍依赖这一行为；原版 getString 则是「带引号读引号串，否则读到 `"`
     * 或空白」。message / component 这类词参数需要原版语义：`say {"text":"hi"}` 只取到
     * `{"text"` 为止。
     */
    [[nodiscard]] std::string readWord()
    {
        if (!canRead()) {
            return "";
        }

        if (peek() == SYNTAX_QUOTE) {
            return readQuotedString();
        }

        const i32 start = m_cursor;
        while (canRead() && !isWhitespace(peek()) && peek() != SYNTAX_QUOTE) {
            skip();
        }
        const size_t startIndex = static_cast<size_t>(start);
        const size_t endIndex = static_cast<size_t>(m_cursor);
        return std::string(m_input.substr(startIndex, endIndex - startIndex));
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
     * @brief 读取整数（brigadier StringReader.readInt 的语义）
     *
     * 与原版逐条一致：不接受前导空白；'+' 不非法（会作为单词的一部分留给调用方）；
     * 数字部分按 i32 相乘自然回绕（`2147483648` 读成 -2147483648），不报溢出；
     * 读不到数字时游标回到起点再报错。
     */
    [[nodiscard]] i32 readInt()
    {
        i32 start = m_cursor;
        // i32 乘法本身会回绕，直接按位运算与 Java 的 `result * 10 + c` 一致。
        bool negative = false;
        if (canRead() && peek() == '-') {
            skip();
            if (!canRead()) {
                setCursor(start);
                throw CommandException(CommandErrorType::IntegerExpected, "Expected integer", start);
            }
            negative = true;
        }
        if (canRead() && isDigit(peek())) {
            const i32 digitsStart = m_cursor;
            while (canRead() && isDigit(peek())) {
                skip();
            }
            const std::string digits(
                m_input.substr(static_cast<size_t>(digitsStart), static_cast<size_t>(m_cursor - digitsStart)));
            i32 result = 0;
            for (char c : digits) {
                result = static_cast<i32>(static_cast<u32>(result) * 10u + static_cast<u32>(c - '0'));
            }
            return negative ? -result : result;
        }
        setCursor(start);
        throw CommandException(
            CommandErrorType::IntegerExpected, "Expected integer at position " + std::to_string(start), start);
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
     * @brief 读取浮点数（brigadier StringReader.readDouble 的语义）
     *
     * 与整数共用同一套数字语法：可选的 '-'、小数、可选的多位小数；不接受 '+'、
     * 前导空白或指数（`1e3` 只读成 1，`e3` 留给调用方当单位/后缀）。
     * 读不到数字时游标回到起点再报错。
     */
    [[nodiscard]] f64 readDouble()
    {
        i32 start = m_cursor;
        bool negative = false;
        if (canRead() && peek() == '-') {
            skip();
            if (!canRead()) {
                setCursor(start);
                throw CommandException(
                    CommandErrorType::FloatExpected, "Expected float at position " + std::to_string(start), start);
            }
            negative = true;
        }
        f64 result = 0.0;
        f64 decimalPlace = 0.1;
        bool hasDigits = false;
        bool hasDecimal = false;
        while (canRead()) {
            const char c = peek();
            if (c == '.') {
                if (hasDecimal) {
                    break;
                }
                hasDecimal = true;
                skip();
                continue;
            }
            if (!isDigit(c)) {
                break;
            }
            if (hasDecimal) {
                result += static_cast<f64>(c - '0') * decimalPlace;
                decimalPlace *= 0.1;
            } else {
                result = result * 10.0 + static_cast<f64>(c - '0');
            }
            hasDigits = true;
            skip();
        }
        if (!hasDigits) {
            setCursor(start);
            throw CommandException(
                CommandErrorType::FloatExpected, "Expected float at position " + std::to_string(start), start);
        }
        return negative ? -result : result;
    }

    /**
     * @brief 读取浮点数（单精度）
     */
    [[nodiscard]] f32 readFloat()
    {
        return static_cast<f32>(readDouble());
    }

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

    [[nodiscard]] static bool isDigit(char c)
    {
        return c >= '0' && c <= '9';
    }

    /// brigadier {@code StringReader.isAllowedNumber}：数字或小数点。
    [[nodiscard]] static bool isAllowedNumber(char c)
    {
        return (c >= '0' && c <= '9') || c == '.';
    }

    [[nodiscard]] static bool isLetter(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

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
