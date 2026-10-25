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

#include "common/command/CommandContext.hpp"
#include "common/command/CommandNode.hpp"
#include "common/core/Types.hpp"

#include <string>
#include <utility>

namespace mc {
namespace command {

/// run 节点为「首个修改器段」的参数槽前缀（后接修改器索引）。
inline constexpr const char* kRootSegmentPrefix = "rootSegment";

/// run 节点为「后续修改器段」的参数槽前缀（后接修改器索引）。
inline constexpr const char* kPathSegmentPrefix = "pathSegment";

/**
 * @brief /execute 的上下文修改器节点（as / at / in / positioned / rotated ...）
 *
 * 原版这些字面量节点在解析阶段把「选中了哪个修改器」记进上下文（`getModifiers()`），
 * 末端 run 的执行器再取出整条链、依次改写命令源。Cubium 的 `LiteralCommandNode` 不写
 * 参数，于是这个子类在匹配成功后把索引写进 `commandModifier`，由 ExecuteRunNode 读取。
 */
class ExecuteModifierNode : public LiteralCommandNode<ServerCommandSource> {
public:
    ExecuteModifierNode(const std::string& literal, i32 index)
        : LiteralCommandNode<ServerCommandSource>(literal)
        , m_index(index)
    {}

    [[nodiscard]] i32 index() const noexcept { return m_index; }

    void parse(StringReader& reader, CommandContext<ServerCommandSource>& context) const override
    {
        LiteralCommandNode<ServerCommandSource>::parse(reader, context);
        context.setArgument("commandModifier", m_index);
    }

private:
    i32 m_index;
};


/**
 * @brief /execute run 下的 command 占位节点（名字对齐原版，但不读输入）
 *
 * 嵌套命令原文由 ExecuteRunNode 在链尾一次性写好（run 之后整段），dispatcher 再派发到
 * 本节点时输入已在末尾。它负责让节点链上出现 command 这个参数名，并且在 run 节点已经
 * 填好值时不再用空值覆盖。
 */
class CommandPlaceholderNode : public ArgumentCommandNode<ServerCommandSource, std::string> {
public:
    CommandPlaceholderNode()
        : ArgumentCommandNode<ServerCommandSource, std::string>("command", StringArgumentType::greedyString())
    {}

    [[nodiscard]] std::string getTypeName() const override { return "string"; }

    void parse(StringReader& reader, CommandContext<ServerCommandSource>& context) const override
    {
        // run 节点解析链尾时已经把整段原文写进 command；这里多半是同一段文本的第二遍，
        // 若此时还有剩余（`run` 之后的首个词是普通单词、但整段读取的边界不同），以原文为准。
        if (!reader.canRead()) {
            return;
        }
        std::string remaining = reader.getRemaining();
        reader.setCursor(reader.getTotalLength());
        context.setArgument("command", remaining);
    }
};

/**
 * @brief 复制 execute 下某个参数节点的解析逻辑，把结果写进指定槽的参数节点
 *
 * run 节点走完剩余修改器链时复用 execute 下那份参数节点（解析逻辑完全一致），但同一条
 * 链上可以重复出现同类修改器（`positioned 0 0 0 positioned as @p`），参数名会撞；
 * 所以每一段的参数值写进 rootSegment<n> / pathSegment<n>，执行器按槽名读取。
 */
template <typename S>
class ParentSlotNode : public CommandNode<S> {
public:
    ParentSlotNode(const CommandNode<S>& prototype, std::string slot)
        : m_prototype(prototype)
        , m_slot(std::move(slot))
    {}

    [[nodiscard]] NodeType getType() const noexcept override { return NodeType::Argument; }
    [[nodiscard]] std::string getName() const noexcept override { return m_slot; }
    [[nodiscard]] std::string getTypeName() const override { return m_prototype.getTypeName(); }
    [[nodiscard]] nlohmann::json getMetadata() const override { return m_prototype.getMetadata(); }

    void parse(StringReader& reader, CommandContext<S>& context) const override
    {
        const i32 start = reader.getCursor();
        m_prototype.parse(reader, context);
        // 原型把值写进自己的参数名；这里搬到链内专属槽名，避免多次出现互相覆盖。
        const std::string prototypeName = m_prototype.getName();
        if (prototypeName != m_slot) {
            if (context.hasArgument(prototypeName)) {
                context.setArgumentAny(m_slot, context.getArgumentAny(prototypeName), start);
            }
        }
    }

private:
    const CommandNode<S>& m_prototype;
    std::string m_slot;
};

} // namespace command
} // namespace mc
