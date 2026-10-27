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

#include "ExecuteModifierNode.hpp"

#include "common/command/CommandContext.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/StringReader.hpp"
#include "common/core/Types.hpp"
#include "server/command/ServerCommandSource.hpp"

#include <cstddef>
#include <cstdio>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc {
namespace command {

/**
 * @brief /execute 的 run 节点 parse 时写进上下文的标记
 *
 * dispatcher 逐层回溯候选，中途每个 run 都会读同一个 command 槽（不移动游标）；只有走过
 * 整条修改器链、读到链尾 "run" 的那个 run 才会把 complete 置 true。执行器据此判断本次
 * 解析是否完整，以及按哪条链改写命令源。
 */
struct ExecuteParseMarker {
    /// 走到链尾的这段 run 记录的修改器链（管道分隔的修改器字面量），如 "as|at"。
    std::string chain;
    /// 是否是 /execute 顶层的 run。
    bool topLevel = false;
    /// 是否真的读到了链尾的 "run"。
    bool complete = false;
};

/// 上下文里保存 {@link ExecuteParseMarker} 的槽名。
inline constexpr const char* kParseMarkerParam = "commandParseMarker";

/**
 * @brief /execute 链末端的 run 节点（承担原版 ForwardingCommand 的角色）
 *
 * 原版在 run 节点下挂一棵「下一段命令」的子树，由 CommandDispatcher 继续派发；Cubium 的
 * 解析器只派发一次，所以这里在 parse 阶段手动继续：
 *
 *   execute as @a at @s run say hi
 *     as → @a → at → @s → run → command("say hi")
 *     选中链写成 "commandChain" = "0;1;"，执行器据此逐层改写命令源。
 *
 * run 之后的每一段都要在这里解析完：既不是修改器也不是 run 时按「未知参数」报错，
 * 否则解析器会把半截命令当成成功。修改器的参数树登记在 {@link s_modifiers}，与 execute
 * 下挂的是同一批节点，解析结果写进同一个上下文。
 *

 * command 子节点是一个「不读输入」的占位参数（StringArgumentType::empty()）：它只负责让
 * 节点链上存在 command 这个参数名，并让字段留下尚未消费的剩余文本——正是这份剩余文本让
 * dispatcher 继续尝试 execute 下的其它子节点。若换成 greedyString，它在 `execute run` 处
 * 就会吞掉全部剩余文本并成为「后面还有字段」的成功候选，`run` 之后的内容便被误读。
 */
class ExecuteRunNode : public LiteralCommandNode<ServerCommandSource> {
public:
    /// chain：前置修改器的索引；kTopLevel 表示 /execute 顶层 run。
    static constexpr i32 kTopLevel = -1;

    ExecuteRunNode(i32 chain, CommandCallback<ServerCommandSource> command)
        : LiteralCommandNode<ServerCommandSource>("run")
        , m_chain(chain)
        , m_command(std::move(command))
    {
        // 原版的 run 是字面量节点、其下没有 command 子节点（下一段命令由第二条 dispatcher
        // 派发）。Cubium 为了复用一次派发，在 run 下挂了 command 占位参数——它不属于协议里的
        // 命令树，由 ExecuteCommand 注册处标记隐藏；run 节点本身照常出现在快照里。
    }

    [[nodiscard]] i32 chain() const noexcept
    {
        return m_chain;
    }

    /// 注册阶段登记全部修改器节点，parse 时按「字面量 → 索引」继续解析。
    static void setModifiers(std::vector<std::shared_ptr<CommandNode<ServerCommandSource>>> modifiers)
    {
        s_modifierIndices.clear();
        for (size_t i = 0; i < modifiers.size(); ++i) {
            s_modifierIndices[modifiers[i]->getName()] = static_cast<i32>(i);
        }
        s_modifiers = std::move(modifiers);
    }

    /// /execute 下那个不带前置修改器的 run（`execute run <command>`）。
    static void setTopLevelRun(const std::shared_ptr<ExecuteRunNode>& run)
    {
        s_topLevelRun = run;
    }

    void parse(StringReader& reader, CommandContext<ServerCommandSource>& context) const override
    {
        LiteralCommandNode<ServerCommandSource>::parse(reader, context);
        context.setArgument(kParseMarkerParam, ExecuteParseMarker{});

        const i32 runEnd = reader.getCursor();
        std::string chainText = currentChain();

        // 逐段读「修改器字面量 + 参数」，直到链尾的 run 或嵌套命令原文。
        while (true) {
            const i32 tokenStart = reader.getCursor();
            reader.skipWhitespace();

            const std::string token = reader.readUnquotedString();
            if (token.empty() || token == "run") {
                reader.setCursor(tokenStart);
                reader.skipWhitespace();
                break;
            }

            const i32 index = modifierIndex(token);
            const std::shared_ptr<CommandNode<ServerCommandSource>> segmentNode =
                index < 0 || static_cast<size_t>(index) >= s_modifiers.size()
                ? nullptr
                : segmentFor(*s_modifiers[static_cast<size_t>(index)], std::to_string(index));
            if (!segmentNode) {
                reader.setCursor(tokenStart);
                reader.skipWhitespace();
                break;
            }

            try {
                segmentNode->parse(reader, context);
            }
            catch (const CommandException&) {
                reader.setCursor(tokenStart);
                reader.skipWhitespace();
                break;
            }

            chainText += "|";
            chainText += token;

            // 参数读完后如果后面还有一个词，先看它是不是下一个修改器：不是就说明
            // 到了嵌套命令原文（`as @p run help`：targets 读走 @p 后剩下 "run help"）。
            StringReader lookahead = reader;
            lookahead.skipWhitespace();
            if (!lookahead.canRead()) {
                break;
            }
            const std::string next = lookahead.readUnquotedString();
            if (next != "run" && modifierIndex(next) < 0) {
                break;
            }
        }

        std::string command;
        if (reader.canRead()) {
            command = reader.readString();
            reader.setCursor(reader.getTotalLength());
        }
        // dispatcher 会接着派发 run 下的 command 节点，把剩余文本交给它；那一步产生的
        // 「输入用完」只是待解释状态，执行器据此与本标记决定要不要真的执行。
        context.setPendingHandOff();
        context.setArgument("command", command);
        // 节点链上的 command 占位槽（不读输入的参数）会再把空值写回去，标记成「已由 run
        // 节点填好」，由占位槽在执行时跳过写入——见 ExecuteCommand 注册处的 runCommandArg。
        context.setArgument(kParseMarkerParam, ExecuteParseMarker{chainText, m_chain == kTopLevel, true});

        // 回到 run 之后的位置，让 dispatcher 继续按原样派发（转发节点会读到 command 槽）。
        reader.setCursor(runEnd);
    }

private:
    /// 该 run 节点自己的 command 子节点（读走嵌套命令原文的末端参数）。
    [[nodiscard]] std::shared_ptr<CommandNode<ServerCommandSource>> runChildNode() const
    {
        for (const auto& name : getArguments()) {
            if (auto child = getChild(name)) {
                return child;
            }
        }
        return nullptr;
    }

    /// 本 run 已经选中过的修改器链（管道分隔）；顶层 run 为空。
    [[nodiscard]] std::string currentChain() const
    {
        if (m_chain == kTopLevel || static_cast<size_t>(m_chain) >= s_modifiers.size()) {
            return {};
        }
        return s_modifiers[static_cast<size_t>(m_chain)]->getName();
    }

    /// 该修改器的末端参数子树（首次使用时按需拼并缓存）。
    static std::shared_ptr<CommandNode<ServerCommandSource>> segmentFor(
        CommandNode<ServerCommandSource>& modifier, const std::string& segmentKey)
    {
        const std::string name = "commandSegment" + segmentKey;
        if (!modifier.getChild(name)) {
            if (auto segment = buildSegment(modifier, segmentKey)) {
                modifier.addChild(segment);
            }
        }
        return modifier.getChild(name);
    }

    [[nodiscard]] static bool hasSegment(const CommandNode<ServerCommandSource>& modifier)
    {
        return true;
    }

    /**
     * @brief 在修改器下拼出「一条走到底的末端参数子树」，缓存为 commandSegment<n>
     *
     * 原版把「下一个节点」挂在每个 run 下，execute 树里每条路径只派发一次；Cubium 的解析器
     * 不二次派发，run 节点只好自己把剩下的修改器链走完。这里沿 children 取第一个参数子节点，
     * 拼成「参数 → … → 末端参数」的路径，挂在发起这段 chain 的修改器下。
     *
     * 链上重复出现的同类修改器共用参数名，所以整条路径按「发起段」的序号写进
     * rootSegment<n> / pathSegment<n> 槽；执行器按同样的槽名读取。
     */
    static std::shared_ptr<CommandNode<ServerCommandSource>> buildSegment(
        const CommandNode<ServerCommandSource>& modifier, const std::string& segmentKey)
    {
        // 只沿「根参数 → 其下第一个字面量 → 该字面量的参数」走三跳，覆盖
        // positioned as <targets> / facing entity <targets> / on <关系> <entity> / rotated as。
        // 不再继续往下：参数节点下挂了 run，继续沿「第一个参数子节点」会走回 run。
        std::vector<const CommandNode<ServerCommandSource>*> path;
        const CommandNode<ServerCommandSource>* current = &modifier;
        for (size_t depth = 0; depth < 3; ++depth) {
            auto child = firstArgumentChild(*current);
            if (!child) {
                break;
            }
            path.push_back(child.get());
            const auto& literals = child->getLiterals();
            if (literals.empty()) {
                break;
            }
            auto next = child->getChild(*literals.begin());
            if (!next) {
                break;
            }
            current = next.get();
        }
        if (path.empty()) {
            return nullptr;
        }

        std::shared_ptr<CommandNode<ServerCommandSource>> head;
        for (size_t i = path.size(); i-- > 0;) {
            const std::string slot = i == 0 ? kRootSegmentPrefix + segmentKey : kPathSegmentPrefix + segmentKey;
            auto node = std::make_shared<ParentSlotNode<ServerCommandSource>>(*path[i], slot);
            if (head) {
                node->addChild(head);
            }
            head = node;
        }
        return head;
    }

    static std::shared_ptr<CommandNode<ServerCommandSource>> firstArgumentChild(
        const CommandNode<ServerCommandSource>& node)
    {
        for (const auto& name : node.getArguments()) {
            if (auto child = node.getChild(name)) {
                return child;
            }
        }
        return nullptr;
    }

    [[nodiscard]] static i32 modifierIndex(const std::string& literal)
    {
        const auto it = s_modifierIndices.find(literal);
        return it == s_modifierIndices.end() ? -1 : it->second;
    }

    i32 m_chain;
    CommandCallback<ServerCommandSource> m_command;

    static inline std::vector<std::shared_ptr<CommandNode<ServerCommandSource>>> s_modifiers;
    static inline std::unordered_map<std::string, i32> s_modifierIndices;
    static inline std::shared_ptr<ExecuteRunNode> s_topLevelRun;
};

} // namespace command
} // namespace mc