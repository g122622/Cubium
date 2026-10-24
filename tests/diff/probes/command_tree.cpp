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
 * @file command_tree.cpp
 * @brief 差分能力：服务端命令树（注册了哪些命令、每条命令接受什么参数）。
 *
 * `list`：唯一的单元 `tree`
 * `run` ：注册全部内置命令后遍历分发器，每个根命令输出一棵扁平化的树：
 *
 *        命令名<TAB>根命令的一级子节点签名
 *
 *   其中参数写成 `名字:类型`（如 `mode:gamemode`），字面量原样输出；子节点按名字排序，
 *   中等深度的分支（如 `execute`）只展开一层——树的形状与参数类型是最容易分叉的部分，
 *   逐层展开会让报告淹没在细节里。
 *
 * 命令树是 `/help`、Tab 补全、以及客户端命令补全列表的直接来源：某条命令在原版存在而这里
 * 没有、或参数名/类型不同，玩家与数据包立刻能感觉到。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "server/command/CommandRegistry.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;
using Dispatcher = mc::command::CommandDispatcher<mc::command::ServerCommandSource>;
using NodePtr = Dispatcher::NodePtr;

/** 节点的签名：字面量就是名字，参数写成 `名字:类型`。 */
std::string signature(const mc::command::CommandNode<mc::command::ServerCommandSource>& node)
{
    if (node.getType() == mc::command::NodeType::Argument) {
        return node.getName() + ":" + node.getTypeName();
    }
    return node.getName();
}

/** 按名字排序后的子节点签名。 */
std::vector<std::string> childSignatures(const mc::command::CommandNode<mc::command::ServerCommandSource>* node)
{
    std::vector<std::string> result;
    if (!node) {
        return result;
    }
    std::vector<std::pair<std::string, const mc::command::CommandNode<mc::command::ServerCommandSource>*>> children;
    for (const auto& entry : node->getChildren()) {
        children.emplace_back(entry.first, entry.second.get());
    }
    std::sort(children.begin(), children.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    for (const auto& [name, child] : children) {
        result.push_back(signature(*child));
    }
    return result;
}

/**
 * 把一条根命令展开成一行：命令名 + 一级子节点；一级子节点若还有子节点，
 * 再追加 `名字(子1,子2)`，让 `execute`/`scoreboard` 这类分叉命令的形状也能比出来。
 */
std::string describeTree(const NodePtr& root)
{
    std::vector<std::pair<std::string, const mc::command::CommandNode<mc::command::ServerCommandSource>*>> children;
    for (const auto& entry : root->getChildren()) {
        children.emplace_back(entry.first, entry.second.get());
    }
    std::sort(children.begin(), children.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    std::vector<std::string> parts;
    for (const auto& [name, childPtr] : children) {
        const auto* child = childPtr;
        std::string text = signature(*child);
        const auto grandchildren = childSignatures(childPtr);
        if (!grandchildren.empty()) {
            text += "(";
            for (size_t i = 0; i < grandchildren.size(); ++i) {
                if (i != 0) {
                    text += ",";
                }
                text += grandchildren[i];
            }
            text += ")";
        }
        parts.push_back(std::move(text));
    }
    std::sort(parts.begin(), parts.end());

    std::string line = root->getName();
    for (const auto& part : parts) {
        line += " ";
        line += part;
    }
    return line;
}

void writeList(FILE* out)
{
    std::fputs("tree", out);
    std::fputc('\n', out);
}

std::string runLine(std::string_view)
{
    // CommandRegistry 的构造函数会注册全部内置命令；探针不执行任何命令，只遍历。
    const mc::command::CommandRegistry registry;
    const auto root = registry.dispatcher().getRoot();

    std::vector<std::string> lines;
    for (const auto& entry : root->getChildren()) {
        lines.push_back(describeTree(entry.second));
    }
    std::sort(lines.begin(), lines.end());

    return json{{"tree", lines}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("command_tree", &writeList, &runLine)

} // namespace mc::diff