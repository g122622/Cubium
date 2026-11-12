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
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace mc::text {
class ITextComponent;
}

namespace mc::command {

/**
 * @brief 命令错误类型
 *
 * 定义命令解析过程中可能遇到的各种错误类型。它是**值类型**的一部分，
 * 不再通过 C++ 异常传递（项目正在推进 `-fno-exceptions`）。
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
    NbtPathDataTooDeep,      // 待写入的数据叠加路径深度后超过 NBT 最大嵌套深度
    InvalidNbtPath,          // 无效的 NBT 路径语法

    // 权限错误
    PermissionDenied, // 权限不足

    // 通用错误
    Unknown, // 未知错误
};

/**
 * @brief 命令错误（纯值类型）
 *
 * 携带 `CommandErrorType`（决定翻译键与客户端提示分类）、原始消息（通常就是翻译键）、
 * 出错游标（用于「最佳失败候选」比较与客户端高亮）以及原始输入。
 *
 * 【重要】它**不是** `std::exception` 派生类，也不再被 `throw`：所有解析/执行函数统一
 * 用 `Result<T, CommandError>` 把失败返回给调用方。这样命令子系统在
 * `-fno-exceptions` 下可以正常编译，且玩家输入错误不会终止进程。
 */
class CommandError {
public:
    CommandError() = default;

    CommandError(CommandErrorType type, std::string message)
        : m_type(type)
        , m_message(std::move(message))
        , m_cursor(-1)
    {}

    CommandError(CommandErrorType type, std::string message, i32 cursor)
        : m_type(type)
        , m_message(std::move(message))
        , m_cursor(cursor)
    {}

    CommandError(CommandErrorType type, const char* message, i32 cursor)
        : m_type(type)
        , m_message(message)
        , m_cursor(cursor)
    {}

    /**
     * @brief 携带文本组件的错误（面向玩家的显示文本）
     *
     * 命令实现应使用翻译组件（`text::translatable`），让客户端按自己的语言本地化；
     * 字符串 message 仍保留给日志与断言。
     *
     * @param type 错误类型
     * @param message 错误组件
     */
    CommandError(CommandErrorType type, std::shared_ptr<const text::ITextComponent> message)
        : m_type(type)
        , m_component(std::move(message))
        , m_cursor(-1)
    {}

    /**
     * @brief 携带文本组件的错误（带游标）
     *
     * @param type 错误类型
     * @param message 错误组件
     * @param cursor 出错游标
     */
    CommandError(CommandErrorType type, std::shared_ptr<const text::ITextComponent> message, i32 cursor)
        : m_type(type)
        , m_component(std::move(message))
        , m_cursor(cursor)
    {}

    /**
     * @brief 由通用 `Error` 构造
     *
     * 用于跨子系统传播：命令代码调用返回 `Result<T>`（错误类型为 `mc::Error`）的函数时，
     * `MC_TRY` 会借这个构造函数把 `Error` 隐式转成 `CommandError`。
     */
    CommandError(const Error& error) // NOLINT: 允许隐式转换
        : m_type(CommandErrorType::Unknown)
        , m_message(error.message())
        , m_cursor(-1)
    {}

    [[nodiscard]] CommandErrorType type() const noexcept
    {
        return m_type;
    }

    [[nodiscard]] const std::string& message() const noexcept
    {
        return m_message;
    }

    /**
     * @brief 取得面向用户的错误文本
     *
     * 命令实现经常直接使用翻译键（如 "commands.setblock.success"），message() 原样
     * 保留以便日志排查；展示给玩家/控制台时应使用本方法：按当前语言解析成可读文本，
     * 找不到翻译时回退原文。
     *
     * @return 翻译后的错误文本
     */
    [[nodiscard]] std::string translatedMessage() const;

    /**
     * @brief 取得面向玩家的错误组件
     *
     * 命令实现通过组件构造错误时返回该组件（服务端不本地化，直接透传给客户端）；
     * 通过字符串构造时返回 nullptr，调用方应回退到 `translatedMessage()`。
     *
     * @return 错误组件指针，可能为空
     */
    [[nodiscard]] const text::ITextComponent* component() const noexcept
    {
        return m_component.get();
    }

    [[nodiscard]] i32 cursor() const noexcept
    {
        return m_cursor;
    }

    /**
     * @brief 附加原始输入字符串
     * @param input 原始输入字符串
     */
    [[nodiscard]] CommandError withInput(std::string_view input) const
    {
        CommandError result(*this);
        result.m_input = std::string(input);
        return result;
    }

    [[nodiscard]] const std::string& input() const noexcept
    {
        return m_input;
    }

    void setInput(const std::string& input)
    {
        m_input = input;
    }

    /**
     * @brief 生成用于日志/断言的可读文本
     *
     * `Result::value()` 在失败时用 `error().toString()` 作为断言消息，因此错误载体必须
     * 提供该方法。
     */
    [[nodiscard]] std::string toString() const
    {
        std::string text = "[CommandError " + std::to_string(static_cast<i32>(m_type)) + "] " + m_message;
        if (m_cursor >= 0) {
            text += " (cursor " + std::to_string(m_cursor) + ")";
        }
        return text;
    }

private:
    CommandErrorType m_type = CommandErrorType::Unknown;
    std::string m_message;
    std::shared_ptr<const text::ITextComponent> m_component;
    i32 m_cursor = -1;
    std::string m_input;
};

/**
 * @brief 无参数命令错误工厂
 *
 * 用于创建不需要额外参数的错误消息。
 */
class SimpleCommandError {
public:
    SimpleCommandError(CommandErrorType type, std::string message)
        : m_type(type)
        , m_message(std::move(message))
    {}

    [[nodiscard]] CommandError create() const
    {
        return CommandError(m_type, m_message);
    }

    [[nodiscard]] CommandError createWithContext(i32 cursor, std::string_view input) const
    {
        CommandError result(m_type, m_message, cursor);
        result.setInput(std::string(input));
        return result;
    }

private:
    CommandErrorType m_type;
    std::string m_message;
};

/**
 * @brief 带参数命令错误工厂
 *
 * 用于创建带参数的错误消息，格式串只支持按顺序替换的 `{}` 占位符。
 */
template <typename... Args>
class DynamicCommandError {
public:
    DynamicCommandError(CommandErrorType type, std::string format)
        : m_type(type)
        , m_format(std::move(format))
    {}

    [[nodiscard]] CommandError create(Args... args) const
    {
        return CommandError(m_type, _formatMessage(args...));
    }

    [[nodiscard]] CommandError createWithContext(i32 cursor, std::string_view input, Args... args) const
    {
        CommandError result(m_type, _formatMessage(args...), cursor);
        result.setInput(std::string(input));
        return result;
    }

private:
    std::string _formatMessage(Args... args) const
    {
        std::string result = m_format;
        ((_replaceFirst(result, "{}", std::to_string(args))), ...);
        return result;
    }

    static void _replaceFirst(std::string& str, const std::string& from, const std::string& to)
    {
        const size_t pos = str.find(from);
        if (pos != std::string::npos) {
            str.replace(pos, from.length(), to);
        }
    }

    CommandErrorType m_type;
    std::string m_format;
};

} // namespace mc::command
