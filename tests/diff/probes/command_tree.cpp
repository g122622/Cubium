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
 * `arguments`：每个参数节点的网络编码 {typeId, payload, suggestions}，键是从根到该节点的路径
 *   （`/execute as <targets:entity>` 这种叶子参数带 `:类型名` 后缀）；
 * `components`：全部节点的 {type, executable, restricted, redirect, redirectModifier, suggestions}，key 同上
 *   （参数节点的键不带类型后缀）——同一个通用参数类型在不同命令里承担不同角色、字面量的建议
 *   提供器被误挂，都会在这里体现；
 * `wire`：整棵树按 clientbound/minecraft:commands 编码后的完整字节和长度。
 *
 * 命令树是 `/help`、Tab 补全、以及客户端命令补全列表的直接来源：某条命令在原版存在而这里
 * 没有、或参数名/类型不同，玩家与数据包立刻能感觉到。
 */
#include <algorithm>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/CommandTreeSnapshot.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/network/outbound/CommandTreeEncoder.hpp"

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
        // 纯实现脚手架（/execute 链上的 run 等）不属于协议命令树，与命令树快照同样跳过。
        if (entry.second && entry.second->isTreeHidden()) {
            continue;
        }
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
        if (entry.second && entry.second->isTreeHidden()) {
            continue;
        }
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

/** 字节序列转小写十六进制串，便于放进 JSON 做逐字节比对。 */
std::string toHex(const std::vector<u8>& bytes)
{
    static constexpr char kDigits[] = "0123456789abcdef";
    std::string result;
    result.reserve(bytes.size() * 2);
    for (const u8 byte : bytes) {
        result.push_back(kDigits[byte >> 4]);
        result.push_back(kDigits[byte & 0x0F]);
    }
    return result;
}

/**
 * 一条参数节点的网络编码记录。
 *
 * 直接复用真实编码器（encodeArgumentPropertiesOnly），因此这里产出的字节与客户端实际收到的
 * 完全一致——数值 id、properties 载荷、建议提供器任一写错都会体现为字节差异。
 */
void recordArgument(
    const mc::command::CommandTreeNodeSnapshot& snapshotNode, const std::string& path, nlohmann::json& out)
{
    // typeId 单独报，payload 只报 properties 部分（不含类型 id），两侧口径一致。
    const auto encoded = mc::server::net::command_tree_detail::encodeArgumentPropertiesOnly(snapshotNode);
    if (!encoded.success()) {
        out[path] = nlohmann::json{
            {"typeId", snapshotNode.argumentNetworkId.value_or(-1)}, {"error", encoded.error().toString()}};
        return;
    }
    out[path] = nlohmann::json{{"typeId", snapshotNode.argumentNetworkId.value_or(-1)},
        {"payload", toHex(encoded.value())},
        {"suggestions", snapshotNode.suggestionProviderId.value_or("")}};
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

    // 除命令树的形状之外，还要把「客户端真正解码的东西」比出来：
    // clientbound/minecraft:commands 是二进制命令树，每个参数节点的网络编码是
    //   VarInt(参数类型数值 id) + 类型特定 properties 载荷 [+ VarInt 长度 + 建议提供器 Identifier]
    // 客户端按同一张 ArgumentTypeInfos 注册表逐字节读取。形状（名字/类型名）完全一致、
    // 但数值 id 写错时，客户端会按错误类型去读 payload，游标立刻错位，整棵树解不出来，
    // 表现为 "Failed to decode packet 'clientbound/minecraft:commands'" 并直接断开连接。
    // 这类错误只看名字与类型名是发现不了的，必须比字节。
    //
    // 键用「从根到该参数节点的路径」（如 `/team add <team>`），两侧都按同一条路径对齐，
    // 不依赖各自内部的节点编号——编号本身是遍历顺序的副产物，不适合当比对键。
    nlohmann::json arguments = nlohmann::json::object();
    nlohmann::json components = nlohmann::json::object();

    const auto snapshot = mc::command::buildCommandTreeSnapshot(registry.dispatcher());
    if (!snapshot.nodes.empty()) {
        std::vector<std::optional<std::string>> paths(snapshot.nodes.size());
        const auto collectPaths = [&](const auto& self, u32 index, const std::string& path) -> void {
            if (paths[index].has_value()) {
                return;
            }
            paths[index] = path;
            for (const u32 child : snapshot.nodes[index].children) {
                self(self, child, path + " " + snapshot.nodes[child].name);
            }
        };
        collectPaths(collectPaths, 0, "");
        // 快照节点按 BFS 编号，children 存的是编号；这里从根回溯出每个参数节点的路径，
        // 用「路径」而不是「编号」做比对键（编号是遍历产物，两侧不保证一致）。
        std::function<void(u32, std::string, std::string)> walk =
            [&](u32 index, std::string path, std::string componentKey) {
                const auto& node = snapshot.nodes[index];
                if (!path.empty()) {
                    // components 用规范键：参数节点不带类型后缀，与 arguments 的叶子键区分开。
                    // 节点层面的差异：同一个通用参数类型在不同命令里承担不同角色（如 `<source>` 的 string
                    // 单独作为字面量求值、还是作为 NBT 值），客户端拿到的 executable / redirectModifier /
                    // 建议提供器都会不同。
                    components[componentKey] = nlohmann::json{{"type", mc::command::detail::toString(node.type)},
                        {"executable", node.executable},
                        {"restricted", node.restricted},
                        {"redirectModifier", mc::command::detail::toString(node.redirectModifier)},
                        {"redirect",
                            node.redirect.has_value() ? nlohmann::json(paths[*node.redirect].value())
                                                      : nlohmann::json(nullptr)},
                        {"suggestions", node.suggestionProviderId.value_or("")}};
                }
                if (node.type == mc::command::NodeType::Argument) {
                    recordArgument(node, path, arguments);
                }
                for (const u32 childIndex : node.children) {
                    if (childIndex >= snapshot.nodes.size()) {
                        continue;
                    }
                    const auto& child = snapshot.nodes[childIndex];
                    std::string childPath = path + " " + child.name;
                    const std::string childComponentKey = componentKey + " " + child.name;
                    // 带子节点的参数（filtered 的 filter、loot 的 loot_table）在路径里不带类型后缀：
                    // 它们的子参数直接续在父路径上（oracle 的 `… filter:block_predicate` 落在父路径之后）。
                    if (child.type == mc::command::NodeType::Argument && child.children.empty()) {
                        childPath += ":" + child.typeName;
                    }
                    walk(childIndex, childPath, childComponentKey);
                }
            };
        walk(0, "", "");
    }

    // 整棵树的编码结果也要能成功产出（未知类型名会在这里暴露），并给出字节总数，
    // 让「某处 payload 多写/少写一个字节」这类错位在两侧同样可见。
    const auto whole = mc::server::net::encodeCommandTree(snapshot);
    nlohmann::json wire = nlohmann::json::object();
    if (whole.success()) {
        wire["byteLength"] = whole.value().size();
        wire["payload"] = toHex(whole.value());
    } else {
        wire["error"] = whole.error().toString();
    }

    return json{{"unit", "tree"}, {"tree", lines}, {"arguments", arguments}, {"components", components}, {"wire", wire}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("command_tree", &writeList, &runLine)

} // namespace mc::diff
