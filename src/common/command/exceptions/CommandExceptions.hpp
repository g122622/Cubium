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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace mc::command {

/**
 * @brief 命令语法异常类型
 *
 * 定义命令解析过程中可能遇到的各种语法错误类型
 */
enum class CommandErrorType {
    // 分发器错误
    DispatcherUnknownCommand,            // 未知命令
    DispatcherUnknownArgument,           // 未知参数
    DispatcherExpectedArgumentSeparator, // 期望参数分隔符
    DispatcherExpectedLiteral,           // 期望字面量

    // 参数错误
    IntegerExpected,      // 期望整数
    IntegerTooLow,        // 整数太小
    IntegerTooHigh,       // 整数太大
    FloatExpected,        // 期望浮点数
    FloatTooLow,          // 浮点数太小
    FloatTooHigh,         // 浮点数太大
    BoolExpected,         // 期望布尔值
    StringExpected,       // 期望字符串
    StringQuotedExpected, // 期望引号字符串

    // 实体选择器错误
    EntityNotFound,           // 实体未找到
    PlayerNotFound,           // 玩家未找到
    EntityTooMany,            // 实体太多
    PlayerTooMany,            // 玩家太多
    EntitySelectorNotAllowed, // 选择器不允许
    EntitySelectorInvalid,    // 无效的选择器

    // 位置参数错误
    BlockPosUnloaded,   // 方块位置未加载
    BlockPosOutOfWorld, // 方块位置超出世界

    // NBT 路径错误
    NbtPathNotFound,         // NBT 路径未找到
    NbtPathMultipleResults,  // NBT 路径匹配多个结果
    NbtPathInvalidType,      // NBT 路径类型不匹配
    NbtPathIndexOutOfBounds, // NBT 列表索引越界
    InvalidNbtPath,          // 无效的 NBT 路径语法

    // 权限错误
    PermissionDenied, // 权限不足

    // 通用错误
    Unknown, // 未知错误
};

/**
 * @brief 命令语法异常
 *
 * 用于命令解析和执行过程中的错误报告
 */
class CommandException : public std::runtime_error {
public:
    explicit CommandException(CommandErrorType type, const std::string& message)
        : std::runtime_error(message)
        , m_type(type)
        , m_message(message)
        , m_cursor(-1)
    {}

    CommandException(CommandErrorType type, const std::string& message, i32 cursor)
        : std::runtime_error(message)
        , m_type(type)
        , m_message(message)
        , m_cursor(cursor)
    {}

    // 拷贝构造函数
    CommandException(const CommandException& other)
        : std::runtime_error(other)
        , m_type(other.m_type)
        , m_message(other.m_message)
        , m_cursor(other.m_cursor)
        , m_input(other.m_input)
    {}

    // 移动构造函数
    CommandException(CommandException&& other) noexcept
        : std::runtime_error(std::move(other))
        , m_type(other.m_type)
        , m_message(std::move(other.m_message))
        , m_cursor(other.m_cursor)
        , m_input(std::move(other.m_input))
    {}

    // 拷贝赋值运算符
    CommandException& operator=(const CommandException& other)
    {
        if (this != &other) {
            std::runtime_error::operator=(other);
            m_type = other.m_type;
            m_message = other.m_message;
            m_cursor = other.m_cursor;
            m_input = other.m_input;
        }
        return *this;
    }

    // 移动赋值运算符
    CommandException& operator=(CommandException&& other) noexcept
    {
        if (this != &other) {
            std::runtime_error::operator=(std::move(other));
            m_type = other.m_type;
            m_message = std::move(other.m_message);
            m_cursor = other.m_cursor;
            m_input = std::move(other.m_input);
        }
        return *this;
    }

    [[nodiscard]] CommandErrorType type() const noexcept { return m_type; }
    [[nodiscard]] const std::string& message() const noexcept { return m_message; }
    [[nodiscard]] i32 cursor() const noexcept { return m_cursor; }

    /**
     * @brief 创建带有上下文的异常
     * @param input 原始输入字符串
     */
    [[nodiscard]] CommandException withInput(std::string_view input) const
    {
        CommandException result(m_type, m_message, m_cursor);
        result.m_input = std::string(input);
        return result;
    }

    [[nodiscard]] const std::string& input() const noexcept { return m_input; }

    /**
     * @brief 设置输入字符串（内部使用）
     */
    void setInput(const std::string& input) { m_input = input; }

private:
    CommandErrorType m_type;
    std::string m_message;
    i32 m_cursor;
    std::string m_input;
};

/**
 * @brief 简单命令异常类型
 *
 * 用于创建无参数的异常消息
 */
class SimpleCommandException {
public:
    explicit SimpleCommandException(CommandErrorType type, const std::string& message)
        : m_type(type)
        , m_message(message)
    {}

    // 移动构造函数
    SimpleCommandException(SimpleCommandException&& other) noexcept
        : m_type(other.m_type)
        , m_message(std::move(other.m_message))
    {}

    // 移动赋值运算符
    SimpleCommandException& operator=(SimpleCommandException&& other) noexcept
    {
        if (this != &other) {
            m_type = other.m_type;
            m_message = std::move(other.m_message);
        }
        return *this;
    }

    [[nodiscard]] CommandException create() const { return CommandException(m_type, m_message); }

    [[nodiscard]] CommandException createWithContext(i32 cursor, std::string_view input) const
    {
        CommandException result(m_type, m_message, cursor);
        result.setInput(std::string(input));
        return result;
    }

private:
    CommandErrorType m_type;
    std::string m_message;
};

/**
 * @brief 动态命令异常类型
 *
 * 用于创建带参数的异常消息
 */
template <typename... Args>
class DynamicCommandException {
public:
    explicit DynamicCommandException(CommandErrorType type, const std::string& format)
        : m_type(type)
        , m_format(format)
    {}

    // 移动构造函数
    DynamicCommandException(DynamicCommandException&& other) noexcept
        : m_type(other.m_type)
        , m_format(std::move(other.m_format))
    {}

    // 移动赋值运算符
    DynamicCommandException& operator=(DynamicCommandException&& other) noexcept
    {
        if (this != &other) {
            m_type = other.m_type;
            m_format = std::move(other.m_format);
        }
        return *this;
    }

    [[nodiscard]] CommandException create(Args... args) const
    {
        return CommandException(m_type, _formatMessage(args...));
    }

private:
    std::string _formatMessage(Args... args) const
    {
        std::string result = m_format;
        // 简单实现：支持 {} 占位符
        ((_replaceFirst(result, "{}", std::to_string(args))), ...);
        return result;
    }

    static void _replaceFirst(std::string& str, const std::string& from, const std::string& to)
    {
        size_t pos = str.find(from);
        if (pos != std::string::npos) {
            str.replace(pos, from.length(), to);
        }
    }

    CommandErrorType m_type;
    std::string m_format;
};

} // namespace mc::command
