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

#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include <cstddef>
#include <initializer_list>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::command {

/**
 * @brief 命令树建议来源
 */
enum class CommandTreeSuggestionKind : u8 {
    None = 0,
    Fixed,
    CommandNames,
    PlayerNames,
    EntityNames,
    ItemNames,
};

/**
 * @brief 命令树节点快照
 */
struct CommandTreeNodeSnapshot {
    u32 id = 0;
    NodeType type = NodeType::Root;
    std::string name;
    std::string typeName;
    nlohmann::json metadata = nlohmann::json::object();
    std::vector<std::string> examples;
    std::vector<u32> children;
    std::optional<u32> redirect;
    RedirectModifier redirectModifier = RedirectModifier::None;
    bool executable = false;
    CommandTreeSuggestionKind suggestionKind = CommandTreeSuggestionKind::None;
    std::vector<std::string> suggestionCandidates;

    // ---- 以下字段供 clientbound/minecraft:commands 二进制 CommandNode 编码使用 ----
    // ArgumentType 在 Java 1.21.11 COMMAND_ARGUMENT_TYPE 注册表中的数值 id（ArgumentTypeInfos.bootstrap
    // 注册顺序）。仅 Argument 节点有值；未知 typeName（如 "enum"）为 nullopt，编码器据此报错。
    std::optional<i32> argumentNetworkId;
    // ArgumentType 的类型特定 properties 中间表示（JSON）。来自 ArgumentType::serializeMetadata()
    // （Integer/Float 的 min/max、String 的 mode、Time 的 min、Entity 的 single/playersOnly）。
    // 编码器按 argumentNetworkId 分发解析此 JSON 写二进制。
    nlohmann::json argumentProperties = nlohmann::json::object();
    // 自定义建议提供器的 Java Identifier（如 "minecraft:ask_server"）。仅 hasCustomSuggestions 节点有值。
    // vanilla SuggestionProviders 对未注册 provider 默认返回 minecraft:ask_server。
    std::optional<std::string> suggestionProviderId;
    // 1.21.x flags bit5 restricted。项目无此信息，保守 false。
    bool restricted = false;
};

/**
 * @brief 命令树快照
 */
class CommandTreeSnapshot {
public:
    std::vector<CommandTreeNodeSnapshot> nodes;

    /**
     * @brief 判断快照是否为空
     */
    [[nodiscard]] bool empty() const noexcept
    {
        return nodes.empty();
    }

    /**
     * @brief 序列化为 JSON 对象
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /**
     * @brief 序列化为 JSON 字符串
     */
    [[nodiscard]] std::string toJsonString() const;

    /**
     * @brief 从 JSON 对象反序列化
     */
    [[nodiscard]] static Result<CommandTreeSnapshot> fromJson(const nlohmann::json& json);

    /**
     * @brief 从 JSON 字符串反序列化
     */
    [[nodiscard]] static Result<CommandTreeSnapshot> fromJsonString(std::string_view jsonText);
};

namespace detail {

[[nodiscard]] inline const char* toString(NodeType type)
{
    switch (type) {
        case NodeType::Root:
            return "root";
        case NodeType::Literal:
            return "literal";
        case NodeType::Argument:
            return "argument";
    }
    return "root";
}

[[nodiscard]] inline NodeType parseNodeType(std::string_view text)
{
    if (text == "root") {
        return NodeType::Root;
    }
    if (text == "literal") {
        return NodeType::Literal;
    }
    return NodeType::Argument;
}

[[nodiscard]] inline const char* toString(RedirectModifier modifier)
{
    switch (modifier) {
        case RedirectModifier::None:
            return "none";
        case RedirectModifier::Single:
            return "single";
        case RedirectModifier::Fork:
            return "fork";
    }
    return "none";
}

[[nodiscard]] inline RedirectModifier parseRedirectModifier(std::string_view text)
{
    if (text == "single") {
        return RedirectModifier::Single;
    }
    if (text == "fork") {
        return RedirectModifier::Fork;
    }
    return RedirectModifier::None;
}

[[nodiscard]] inline const char* toString(CommandTreeSuggestionKind kind)
{
    switch (kind) {
        case CommandTreeSuggestionKind::None:
            return "none";
        case CommandTreeSuggestionKind::Fixed:
            return "fixed";
        case CommandTreeSuggestionKind::CommandNames:
            return "command_names";
        case CommandTreeSuggestionKind::PlayerNames:
            return "player_names";
        case CommandTreeSuggestionKind::EntityNames:
            return "entity_names";
        case CommandTreeSuggestionKind::ItemNames:
            return "item_names";
    }
    return "none";
}

[[nodiscard]] inline CommandTreeSuggestionKind parseSuggestionKind(std::string_view text)
{
    if (text == "fixed") {
        return CommandTreeSuggestionKind::Fixed;
    }
    if (text == "command_names") {
        return CommandTreeSuggestionKind::CommandNames;
    }
    if (text == "player_names") {
        return CommandTreeSuggestionKind::PlayerNames;
    }
    if (text == "entity_names") {
        return CommandTreeSuggestionKind::EntityNames;
    }
    if (text == "item_names") {
        return CommandTreeSuggestionKind::ItemNames;
    }
    return CommandTreeSuggestionKind::None;
}

[[nodiscard]] inline bool pathMatches(const std::vector<std::string>& path, std::initializer_list<const char*> segments)
{
    if (path.size() != segments.size()) {
        return false;
    }

    auto pathIt = path.begin();
    auto segmentIt = segments.begin();
    for (; pathIt != path.end(); ++pathIt, ++segmentIt) {
        if (*pathIt != *segmentIt) {
            return false;
        }
    }
    return true;
}

template <typename NodeT>
[[nodiscard]] inline CommandTreeSuggestionKind inferSuggestionKind(
    const std::vector<std::string>& path, const NodeT& node, std::vector<std::string>& candidates)
{
    candidates.clear();

    if (pathMatches(path, {"help", "command"})) {
        return CommandTreeSuggestionKind::CommandNames;
    }

    if (pathMatches(path, {"time", "query", "type"})) {
        candidates = {"day", "daytime", "gametime"};
        return CommandTreeSuggestionKind::Fixed;
    }

    const std::string typeName = node.getTypeName();
    if (typeName == "entity") {
        // 原版只有一种 entity 参数类型，是否限玩家由 metadata 的 playersOnly 决定：
        // 限玩家时补全玩家名，否则补全实体名。
        const nlohmann::json metadata = node.getMetadata();
        const bool playersOnly = metadata.contains("playersOnly") && metadata.at("playersOnly").is_boolean()
            && metadata.at("playersOnly").template get<bool>();
        return playersOnly ? CommandTreeSuggestionKind::PlayerNames : CommandTreeSuggestionKind::EntityNames;
    }

    if (typeName == "item" || typeName == "item_predicate") {
        return CommandTreeSuggestionKind::ItemNames;
    }

    if (!node.getExamples().empty()) {
        candidates = node.getExamples();
        return CommandTreeSuggestionKind::Fixed;
    }

    return CommandTreeSuggestionKind::None;
}

/// ArgumentType 内部 typeName → Java 1.21.11 COMMAND_ARGUMENT_TYPE 数值注册表 id
/// （ArgumentTypeInfos.bootstrap 注册顺序，0-based）。对齐 vanilla ClientboundCommandsPacket
/// serializeCap 写 VarInt(numericId)（非 Identifier 字符串）。未知 typeName（如 "enum"）返回
/// nullopt，编码器据此报错。本项目命令注册实际用到的 typeName 全在此表内（enum 不使用）。
[[nodiscard]] inline std::optional<i32> argumentNetworkIdForTypeName(std::string_view typeName)
{
    // 键按 ArgumentTypeInfos.bootstrap 的注册顺序排列，注释即注册名。
    static const std::unordered_map<std::string_view, i32> kMap = {
        {"bool", 0},           // brigadier:bool
        {"float", 1},          // brigadier:float
        {"double", 2},         // brigadier:double
        {"integer", 3},        // brigadier:integer
        {"long", 4},           // brigadier:long
        {"string", 5},         // brigadier:string（word/phrase/greedy_string 是旧类型名，一并兼容）
        {"word", 5},
        {"phrase", 5},
        {"greedy_string", 5},
        {"entity", 6},         // minecraft:entity（entities/player/players 是旧类型名）
        {"entities", 6},
        {"player", 6},
        {"players", 6},
        {"game_profile", 7},   // minecraft:game_profile
        {"block_pos", 8},      // minecraft:block_pos
        {"column_pos", 9},     // minecraft:column_pos
        {"vec3", 10},          // minecraft:vec3
        {"vec2", 11},          // minecraft:vec2
        {"block_state", 12},   // minecraft:block_state
        {"nbt_compound", 21},  // minecraft:nbt_compound_tag（旧名一并兼容）
        {"nbt_compound_tag", 21},
        {"nbt_tag", 22},       // minecraft:nbt_tag
        {"nbt_path", 23},      // minecraft:nbt_path
        {"item", 14},          // minecraft:item_stack（旧名一并兼容）
        {"item_stack", 14},
        // 原版枚举型参数在 1.21.11 仍是注册表里的参数类型（ArgumentTypeInfos 有 numeric id），
        // 只是 payload 为空；这里补齐编号，编码器按 SingletonArgumentInfo 写空 payload。
        {"objective_criteria", 25},  // minecraft:objective_criteria
        {"operation", 26},           // minecraft:operation
        {"item_predicate", 15},// minecraft:item_predicate
        {"particle", 16},      // minecraft:particle
        {"component", 17},     // minecraft:component
        {"message", 39},       // minecraft:message
        {"resource", 18},      // minecraft:resource
        {"resource_or_tag", 19},      // minecraft:resource_or_tag
        {"resource_or_tag_key", 20},  // minecraft:resource_or_tag_key
        {"resource_key", 24},  // minecraft:resource_key
        {"rotation", 29},      // minecraft:rotation
        {"item_slot", 34},     // minecraft:item_slot
        {"objective", 32},     // minecraft:objective
        {"team", 35},          // minecraft:team
        {"score_holder", 31},  // minecraft:score_holder
        {"swizzle", 30},       // minecraft:swizzle
        {"entity_anchor", 27}, // minecraft:entity_anchor
        {"resource_location", 36}, // minecraft:resource_location
        {"identifier", 36},    // 旧类型名，指向同一个 id
        {"function", 37},      // minecraft:function
        {"dimension", 41},     // minecraft:dimension
        {"gamemode", 42},      // minecraft:gamemode
        {"time", 43},          // minecraft:time
    };
    auto it = kMap.find(typeName);
    if (it == kMap.end()) {
        return std::nullopt;
    }
    return it->second;
}

} // namespace detail

/**
 * @brief 从命令分发器生成命令树快照
 */
template <typename S>
[[nodiscard]] CommandTreeSnapshot buildCommandTreeSnapshot(const CommandDispatcher<S>& dispatcher)
{
    MC_TRACE_SCOPED_EVENT(::mc::trace::TraceEvents.Server.Network, "buildCommandTreeSnapshot");

    CommandTreeSnapshot snapshot;
    auto root = dispatcher.getRoot();
    if (!root) {
        return snapshot;
    }

    using NodePtr = typename CommandDispatcher<S>::NodePtr;
    std::queue<NodePtr> pending;
    std::vector<NodePtr> orderedNodes;
    std::unordered_map<const CommandNode<S>*, u32> nodeIds;

    pending.push(root);
    while (!pending.empty()) {
        NodePtr node = pending.front();
        pending.pop();

        if (!node) {
            continue;
        }

        // 纯实现脚手架（/execute 链上的 run 等）参与解析，但不属于协议里的命令树：
        // 整棵子树跳过，子节点也就不会被枚举。
        if (node->isTreeHidden()) {
            continue;
        }

        const auto* rawNode = node.get();
        if (nodeIds.find(rawNode) != nodeIds.end()) {
            continue;
        }

        const u32 nodeId = static_cast<u32>(orderedNodes.size());
        nodeIds.emplace(rawNode, nodeId);
        orderedNodes.push_back(node);

        for (const auto& childName : node->getLiterals()) {
            auto child = node->getChild(childName);
            if (child) {
                pending.push(child);
            }
        }

        for (const auto& childName : node->getArguments()) {
            auto child = node->getChild(childName);
            if (child) {
                pending.push(child);
            }
        }

        if (node->hasRedirect()) {
            pending.push(node->getRedirect());
        }
    }

    snapshot.nodes.resize(orderedNodes.size());

    // 路径只沿子节点遍历，重定向节点不改变目标的规范路径。
    // 保留 getPath 的深度优先首路径语义，避免每个节点都从根重新搜索整棵树。
    std::unordered_map<const CommandNode<S>*, std::vector<std::string>> nodePaths;
    std::vector<std::string> currentPath;
    const auto collectPaths = [&](const auto& self, const NodePtr& node) -> void {
        if (!nodePaths.emplace(node.get(), currentPath).second) {
            return;
        }
        const auto visitChild = [&](const std::string& name) {
            if (auto child = node->getChild(name); child && !child->isTreeHidden()) {
                currentPath.push_back(child->getName());
                self(self, child);
                currentPath.pop_back();
            }
        };
        for (const auto& name : node->getLiterals()) {
            visitChild(name);
        }
        for (const auto& name : node->getArguments()) {
            visitChild(name);
        }
    };
    collectPaths(collectPaths, root);

    for (size_t index = 0; index < orderedNodes.size(); ++index) {
        const auto& node = orderedNodes[index];
        auto& snapshotNode = snapshot.nodes[index];

        snapshotNode.id = static_cast<u32>(index);
        snapshotNode.type = node->getType();
        snapshotNode.name = node->getName();
        snapshotNode.typeName = node->getTypeName();
        snapshotNode.metadata = node->getMetadata();
        const auto& metadataInfo = node->getMetadataInfo();
        if (!metadataInfo.description.empty()) {
            snapshotNode.metadata["description"] = metadataInfo.description;
        }
        if (!metadataInfo.usage.empty()) {
            snapshotNode.metadata["usage"] = metadataInfo.usage;
        }
        snapshotNode.metadata["permissionLevel"] = metadataInfo.permissionLevel;
        snapshotNode.metadata["implemented"] = metadataInfo.implemented;
        snapshotNode.metadata["aliases"] = metadataInfo.aliases;
        snapshotNode.examples = node->getExamples();
        snapshotNode.executable = node->hasCommand();
        snapshotNode.redirectModifier = node->getRedirectModifier();
        if (node->hasRedirect() && node->getRedirect() && !node->getRedirect()->isTreeHidden()) {
            auto redirectIt = nodeIds.find(node->getRedirect().get());
            if (redirectIt != nodeIds.end()) {
                snapshotNode.redirect = redirectIt->second;
            }
        }

        // 隐藏节点没有进入 nodeIds，枚举子节点时一并跳过（否则 nodeIds.at 抛异常）。
        for (const auto& childName : node->getLiterals()) {
            auto child = node->getChild(childName);
            if (child && !child->isTreeHidden()) {
                snapshotNode.children.push_back(nodeIds.at(child.get()));
            }
        }

        for (const auto& childName : node->getArguments()) {
            auto child = node->getChild(childName);
            if (child && !child->isTreeHidden()) {
                snapshotNode.children.push_back(nodeIds.at(child.get()));
            }
        }

        std::vector<std::string> suggestionCandidates;
        snapshotNode.suggestionKind = detail::inferSuggestionKind(nodePaths[node.get()], *node, suggestionCandidates);
        snapshotNode.suggestionCandidates = std::move(suggestionCandidates);

        // 二进制 CommandNode 编码所需字段（见 CommandTreeEncoder）。
        // restricted：项目无 NodeInspector 等价信息，保守 false（默认值，不赋）。
        if (node->hasCustomSuggestions()) {
            // vanilla SuggestionProviders 对未注册 provider 默认返回 minecraft:ask_server；
            // 项目自定义 provider（Function/Dimension）未在客户端注册表登记，统一回落到此 Identifier。
            snapshotNode.suggestionProviderId = "minecraft:ask_server";
        }
        if (node->getType() == NodeType::Argument) {
            snapshotNode.argumentNetworkId = detail::argumentNetworkIdForTypeName(node->getTypeName());
            // argumentProperties 来自 ArgumentType::serializeMetadata()（含 Entity 的 single/playersOnly、
            // Integer/Float 的 min/max、String 的 mode、Time 的 min）。
            snapshotNode.argumentProperties = node->getMetadata();
        }
    }

    return snapshot;
}

} // namespace mc::command
