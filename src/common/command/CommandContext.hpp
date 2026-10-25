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
#include <vector>

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

    [[nodiscard]] S& getSource() noexcept
    {
        return m_source;
    }

    [[nodiscard]] const S& getSource() const noexcept
    {
        return m_source;
    }

    // ========== 输入 ==========

    [[nodiscard]] std::string_view getInput() const noexcept
    {
        return m_input;
    }

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
     * @brief 按 std::any 取出参数值（不校验类型），用于把参数值搬到另一个槽名。
     * @throws std::out_of_range 如果参数不存在
     */
    [[nodiscard]] std::any getArgumentAny(const std::string& name) const
    {
        auto it = m_arguments.find(name);
        if (it == m_arguments.end()) {
            throw std::out_of_range("Argument not found: " + name);
        }
        return it->second.value;
    }

    /**
     * @brief 按 std::any 写入参数值（不校验类型）。
     */
    void setArgumentAny(const std::string& name, std::any value, i32 cursor = -1)
    {
        Argument arg;
        arg.value = std::move(value);
        arg.cursor = cursor;
        m_arguments[name] = std::move(arg);
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
    [[nodiscard]] NodePtr getRootNode() const noexcept
    {
        return m_rootNode;
    }

    /**
     * @brief 获取当前节点
     */
    [[nodiscard]] NodePtr getCurrentNode() const noexcept
    {
        return m_currentNode;
    }

    /**
     * @brief 设置当前节点
     */
    void setCurrentNode(NodePtr node)
    {
        m_currentNode = std::move(node);
    }

    // ========== 参数范围 ==========

    /**
     * @brief 获取参数的起始位置
     */
    [[nodiscard]] i32 getArgumentCursor(const std::string& name) const
    {
        auto it = m_arguments.find(name);
        return it != m_arguments.end() ? it->second.cursor : -1;
    }

    // ========== 执行链（/execute、/return run）==========

    /**
     * @brief 执行时由「链上第 index 个修改器」节点自己记录的参数。
     *
     * 原版在解析阶段就把 SelectedNode 链存进上下文；Cubium 的修改器/run 节点是普通字面量，
     * 节点自己的 parse 只做匹配，于是它们把选中的索引交给这里，执行器再按索引回查节点。
     */
    void storeNode(i32 index, NodePtr node)
    {
        if (index < 0) {
            return;
        }
        if (m_chainNodes.size() <= static_cast<size_t>(index)) {
            m_chainNodes.resize(static_cast<size_t>(index) + 1);
        }
        m_chainNodes[static_cast<size_t>(index)] = std::move(node);
    }

    [[nodiscard]] size_t getChainSize() const noexcept { return m_chainNodes.size(); }

    /// 链路节点表（与参数表分开存，避免同名的 "modifier"/"command" 参数改写链路本身）。
    void setChainNodes(std::vector<NodePtr> nodes) { m_chainNodes = std::move(nodes); }

    [[nodiscard]] const std::vector<NodePtr>& getChainNodes() const noexcept { return m_chainNodes; }

    [[nodiscard]] NodePtr getNode(i32 index) const
    {
        if (index < 0 || static_cast<size_t>(index) >= m_chainNodes.size()) {
            return nullptr;
        }
        return m_chainNodes[static_cast<size_t>(index)];
    }

    /**
     * @brief 把另一个上下文的参数与链节点并入本上下文（后写入的覆盖先写入的）。
     */
    void mergeForked(const CommandContext<S>& other)
    {
        for (const auto& [name, arg] : other.m_arguments) {
            m_arguments[name] = arg;
        }
        if (m_chainNodes.size() < other.m_chainNodes.size()) {
            m_chainNodes.resize(other.m_chainNodes.size());
        }
        for (size_t i = 0; i < other.m_chainNodes.size(); ++i) {
            if (other.m_chainNodes[i]) {
                m_chainNodes[i] = other.m_chainNodes[i];
            }
        }
    }

    /**
     * @brief 当前已写入的全部参数名（调试与执行链挑选用）。
     */
    [[nodiscard]] std::vector<std::string> getArgumentNames() const
    {
        std::vector<std::string> names;
        names.reserve(m_arguments.size());
        for (const auto& entry : m_arguments) {
            names.push_back(entry.first);
        }
        return names;
    }

    // ========== 回调来源 ==========

    /// 解析器每走一步记下「这条候选路径最后读到的节点回调」，执行时只认匹配本路径的那个。
    void setCallbackProvenance(const CommandCallback<S>& callback) { m_callbackProvenance = callback; }

    [[nodiscard]] const CommandCallback<S>& getCallbackProvenance() const noexcept { return m_callbackProvenance; }

    // ========== 解析/执行协作 ==========

    /**
     * @brief 标记「剩余文本由末端节点自己解释」（/execute 的 run 会继续读修改器链）。
     *
     * dispatcher 先派发到 run，run 自己把链与嵌套命令读完后会再把游标放回 run 之后，
     * 让 dispatcher 继续派发它下面的 command 节点；这一步产生的「输入用完」只是一个
     * 待解释的中间状态，不能算作完整的解析结果。
     */
    void setPendingHandOff() noexcept { m_pendingHandOff = true; }
    [[nodiscard]] bool isPendingHandOff() const noexcept { return m_pendingHandOff; }

    // ========== 子上下文 ==========

    /**
     * @brief 创建子上下文（用于重定向）
     */
    [[nodiscard]] CommandContext<S> copyFor(NodePtr newRoot) const
    {
        CommandContext<S> copy(m_source, m_input, newRoot);
        copy.m_arguments = m_arguments;
        copy.m_currentNode = m_currentNode;
        copy.m_chainNodes = m_chainNodes;
        copy.m_callbackProvenance = m_callbackProvenance;
        copy.m_pendingHandOff = m_pendingHandOff;
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
    std::vector<NodePtr> m_chainNodes;
    CommandCallback<S> m_callbackProvenance;
    bool m_pendingHandOff = false;
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

    [[nodiscard]] bool isSuccess() const noexcept
    {
        return !m_error.has_value();
    }

    [[nodiscard]] bool isFailure() const noexcept
    {
        return m_error.has_value();
    }

    [[nodiscard]] CommandContext<S>* getContext() noexcept
    {
        return m_context.get();
    }

    [[nodiscard]] const std::optional<CommandException>& getError() const noexcept { return m_error; }

    [[nodiscard]] i32 getErrorCursor() const noexcept { return m_errorCursor; }

    /**
     * @brief 解析后尚未消费的剩余文本
     *
     * dispatcher 用它比较成功候选（剩余越短越贴近完整命令）；非空表示这条命令没有被
     * 整个解释掉（`list extra` 的 "extra"）。
     */
    [[nodiscard]] std::string_view getRemaining() const noexcept { return m_remaining; }

    /**
     * @brief 获取异常（如果有）
     *
     * 与 brigadier 一致：只看解析错误。剩余文本是否算错误由 parseNodes 决定——它只会
     * 把「输入用完」的结果当成成功候选，所以这里不再额外检查 remaining。
     */
    [[nodiscard]] std::optional<CommandException> getException() const { return m_error; }

private:
    std::unique_ptr<CommandContext<S>> m_context;
    // 注意：remaining 的源头是 parse 里的临时 StringReader，切成视图会悬垂：
    // 对象被移动/返回时视图仍指向旧对象的成员。这里只保存一份拥有的副本。
    std::string m_remaining;
    std::optional<CommandException> m_error;
    i32 m_errorCursor = -1;

};

} // namespace mc::command
