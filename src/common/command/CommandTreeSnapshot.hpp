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
#include "common/profiler/TraceEvents.hpp"
#include <initializer_list>
#include <queue>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

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
    [[nodiscard]] bool empty() const noexcept { return nodes.empty(); }

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
    if (typeName == "player" || typeName == "players") {
        return CommandTreeSuggestionKind::PlayerNames;
    }

    if (typeName == "entity" || typeName == "entities") {
        return CommandTreeSuggestionKind::EntityNames;
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
        if (node->hasRedirect()) {
            auto redirectIt = nodeIds.find(node->getRedirect().get());
            if (redirectIt != nodeIds.end()) {
                snapshotNode.redirect = redirectIt->second;
            }
        }

        for (const auto& childName : node->getLiterals()) {
            auto child = node->getChild(childName);
            if (child) {
                snapshotNode.children.push_back(nodeIds.at(child.get()));
            }
        }

        for (const auto& childName : node->getArguments()) {
            auto child = node->getChild(childName);
            if (child) {
                snapshotNode.children.push_back(nodeIds.at(child.get()));
            }
        }

        std::vector<std::string> suggestionCandidates;
        snapshotNode.suggestionKind =
            detail::inferSuggestionKind(dispatcher.getPath(node), *node, suggestionCandidates);
        snapshotNode.suggestionCandidates = std::move(suggestionCandidates);
    }

    return snapshot;
}

} // namespace mc::command
