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

#include "common/command/CommandNode.hpp"
#include "common/command/CommandResult.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/core/Types.hpp"
#include <any>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace mc::command {

/**
 * @brief 命令上下文
 *
 * 存储命令解析和执行过程中的所有信息：
 * - 命令源
 * - 解析的参数
 * - 执行路径
 */
template <typename S>
class CommandContext {
public:
    using NodePtr = std::shared_ptr<CommandNode<S>>;

    /**
     * @brief 构造命令上下文
     */
    CommandContext(S& source, std::string_view input, NodePtr rootNode)
        : m_source(source)
        , m_input(input)
        , m_rootNode(std::move(rootNode))
    {}

    // ========== 命令源 ==========

    [[nodiscard]] S& getSource() noexcept { return m_source; }
    [[nodiscard]] const S& getSource() const noexcept { return m_source; }

    // ========== 输入 ==========

    [[nodiscard]] std::string_view getInput() const noexcept { return m_input; }

    // ========== 参数获取 ==========

    /**
     * @brief 检查是否有指定参数
     */
    [[nodiscard]] bool hasArgument(const std::string& name) const
    {
        return m_arguments.find(name) != m_arguments.end();
    }

    /**
     * @brief 获取参数值
     * @tparam T 参数类型
     * @param name 参数名
     * @return 参数值
     * @throws std::out_of_range 如果参数不存在
     * @throws std::bad_any_cast 如果类型不匹配
     */
    template <typename T>
    [[nodiscard]] T getArgument(const std::string& name) const
    {
        auto it = m_arguments.find(name);
        if (it == m_arguments.end()) {
            throw std::out_of_range("Argument not found: " + name);
        }
        return std::any_cast<T>(it->second.value);
    }

    /**
     * @brief 尝试获取参数值
     * @tparam T 参数类型
     * @param name 参数名
     * @param defaultValue 默认值
     * @return 参数值或默认值
     */
    template <typename T>
    [[nodiscard]] T getArgumentOr(const std::string& name, const T& defaultValue) const
    {
        auto it = m_arguments.find(name);
        if (it == m_arguments.end()) {
            return defaultValue;
        }
        try {
            return std::any_cast<T>(it->second.value);
        }
        catch (...) {
            return defaultValue;
        }
    }

    /**
     * @brief 设置参数值
     */
    template <typename T>
    void setArgument(const std::string& name, const T& value, i32 cursor = -1)
    {
        Argument arg;
        arg.value = value;
        arg.cursor = cursor;
        m_arguments[name] = arg;
    }

    // ========== 节点路径 ==========

    /**
     * @brief 获取根节点
     */
    [[nodiscard]] NodePtr getRootNode() const noexcept { return m_rootNode; }

    /**
     * @brief 获取当前节点
     */
    [[nodiscard]] NodePtr getCurrentNode() const noexcept { return m_currentNode; }

    /**
     * @brief 设置当前节点
     */
    void setCurrentNode(NodePtr node) { m_currentNode = std::move(node); }

    // ========== 参数范围 ==========

    /**
     * @brief 获取参数的起始位置
     */
    [[nodiscard]] i32 getArgumentCursor(const std::string& name) const
    {
        auto it = m_arguments.find(name);
        return it != m_arguments.end() ? it->second.cursor : -1;
    }

    // ========== 子上下文 ==========

    /**
     * @brief 创建子上下文（用于重定向）
     */
    [[nodiscard]] CommandContext<S> copyFor(NodePtr newRoot) const
    {
        CommandContext<S> copy(m_source, m_input, newRoot);
        copy.m_arguments = m_arguments;
        copy.m_currentNode = m_currentNode;
        return copy;
    }

private:
    struct Argument {
        std::any value;
        i32 cursor = -1; // 参数在输入中的位置
    };

    S& m_source;
    std::string_view m_input;
    NodePtr m_rootNode;
    NodePtr m_currentNode;
    std::unordered_map<std::string, Argument> m_arguments;
};

/**
 * @brief 解析结果
 *
 * 存储命令解析的结果，包含可能的错误信息
 */
template <typename S>
class ParseResults {
public:
    ParseResults() = default;

    ParseResults(std::unique_ptr<CommandContext<S>> context, std::string_view remaining)
        : m_context(std::move(context))
        , m_remaining(remaining)
    {}

    ParseResults(CommandException error, i32 cursor)
        : m_error(std::move(error))
        , m_errorCursor(cursor)
    {}

    // 允许移动
    ParseResults(ParseResults&&) = default;
    ParseResults& operator=(ParseResults&&) = default;

    // 禁止复制
    ParseResults(const ParseResults&) = delete;
    ParseResults& operator=(const ParseResults&) = delete;

    [[nodiscard]] bool isSuccess() const noexcept { return !m_error.has_value(); }
    [[nodiscard]] bool isFailure() const noexcept { return m_error.has_value(); }

    [[nodiscard]] CommandContext<S>* getContext() noexcept { return m_context.get(); }
    [[nodiscard]] const CommandContext<S>* getContext() const noexcept { return m_context.get(); }

    [[nodiscard]] std::string_view getRemaining() const noexcept { return m_remaining; }

    [[nodiscard]] const std::optional<CommandException>& getError() const noexcept { return m_error; }
    [[nodiscard]] i32 getErrorCursor() const noexcept { return m_errorCursor; }

    /**
     * @brief 获取异常（如果有）
     */
    [[nodiscard]] std::optional<CommandException> getException() const
    {
        // 检查是否有解析异常
        if (m_error) {
            return m_error;
        }

        // 检查是否还有未读取的内容（需要有效的上下文）
        if (m_context && !m_remaining.empty()) {
            // 计算未读取部分的起始位置
            i32 cursor = static_cast<i32>(m_context->getInput().size()) - static_cast<i32>(m_remaining.size());
            return CommandException(CommandErrorType::DispatcherUnknownArgument, "Unknown argument", cursor);
        }

        return std::nullopt;
    }

private:
    std::unique_ptr<CommandContext<S>> m_context;
    std::string_view m_remaining;
    std::optional<CommandException> m_error;
    i32 m_errorCursor = -1;
};

} // namespace mc::command
