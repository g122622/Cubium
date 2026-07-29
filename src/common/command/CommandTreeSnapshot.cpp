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

#include "CommandTreeSnapshot.hpp"

#include "common/profiler/TraceEvents.hpp"
#include <algorithm>

using namespace mc::trace;

namespace mc::command {

namespace {

[[nodiscard]] CommandTreeNodeSnapshot parseNodeSnapshot(const nlohmann::json& json)
{
    CommandTreeNodeSnapshot snapshot;

    snapshot.id = json.at("id").get<u32>();
    snapshot.type = detail::parseNodeType(json.at("type").get<std::string>());
    snapshot.name = json.value("name", std::string{});
    snapshot.typeName = json.value("typeName", snapshot.name);
    snapshot.metadata = json.value("metadata", nlohmann::json::object());
    snapshot.examples = json.value("examples", std::vector<std::string>{});
    snapshot.children = json.value("children", std::vector<u32>{});
    snapshot.redirectModifier = detail::parseRedirectModifier(json.value("redirectModifier", std::string{"none"}));
    snapshot.executable = json.value("executable", false);
    snapshot.suggestionKind = detail::parseSuggestionKind(json.value("suggestionKind", std::string{"none"}));
    snapshot.suggestionCandidates = json.value("suggestionCandidates", std::vector<std::string>{});

    if (json.contains("redirect") && !json.at("redirect").is_null()) {
        snapshot.redirect = json.at("redirect").get<u32>();
    }

    // 二进制 CommandNode 编码字段（向后兼容：缺失时取默认值）。
    if (json.contains("argumentNetworkId") && !json.at("argumentNetworkId").is_null()) {
        snapshot.argumentNetworkId = json.at("argumentNetworkId").get<i32>();
    }
    snapshot.argumentProperties = json.value("argumentProperties", nlohmann::json::object());
    if (json.contains("suggestionProviderId") && !json.at("suggestionProviderId").is_null()) {
        snapshot.suggestionProviderId = json.at("suggestionProviderId").get<std::string>();
    }
    snapshot.restricted = json.value("restricted", false);

    return snapshot;
}

} // namespace

nlohmann::json CommandTreeSnapshot::toJson() const
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Network, "CommandTreeSnapshot::toJson", "nodeCount", nodes.size());

    nlohmann::json json;
    json["nodes"] = nlohmann::json::array();

    for (const auto& node : nodes) {
        nlohmann::json nodeJson;
        nodeJson["id"] = node.id;
        nodeJson["type"] = detail::toString(node.type);
        nodeJson["name"] = node.name;
        nodeJson["typeName"] = node.typeName;
        nodeJson["metadata"] = node.metadata;
        nodeJson["examples"] = node.examples;
        nodeJson["children"] = node.children;
        nodeJson["redirect"] = node.redirect ? nlohmann::json(*node.redirect) : nlohmann::json(nullptr);
        nodeJson["redirectModifier"] = detail::toString(node.redirectModifier);
        nodeJson["executable"] = node.executable;
        nodeJson["suggestionKind"] = detail::toString(node.suggestionKind);
        nodeJson["suggestionCandidates"] = node.suggestionCandidates;
        // 二进制 CommandNode 编码字段。
        nodeJson["argumentNetworkId"] =
            node.argumentNetworkId ? nlohmann::json(*node.argumentNetworkId) : nlohmann::json(nullptr);
        nodeJson["argumentProperties"] = node.argumentProperties;
        nodeJson["suggestionProviderId"] =
            node.suggestionProviderId ? nlohmann::json(*node.suggestionProviderId) : nlohmann::json(nullptr);
        nodeJson["restricted"] = node.restricted;
        json["nodes"].push_back(std::move(nodeJson));
    }

    return json;
}

std::string CommandTreeSnapshot::toJsonString() const
{
    return toJson().dump();
}

Result<CommandTreeSnapshot> CommandTreeSnapshot::fromJson(const nlohmann::json& json)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.IO.Resource, "CommandTreeSnapshot::fromJson");

    if (!json.is_object()) {
        return Error(ErrorCode::InvalidData, "Command tree snapshot must be a JSON object");
    }

    if (!json.contains("nodes") || !json.at("nodes").is_array()) {
        return Error(ErrorCode::InvalidData, "Command tree snapshot is missing nodes");
    }

    CommandTreeSnapshot snapshot;
    const auto& nodeArray = json.at("nodes");
    snapshot.nodes.reserve(nodeArray.size());

    for (const auto& nodeJson : nodeArray) {
        if (!nodeJson.is_object()) {
            return Error(ErrorCode::InvalidData, "Command tree node must be a JSON object");
        }

        try {
            snapshot.nodes.push_back(parseNodeSnapshot(nodeJson));
        }
        catch (const std::exception& e) {
            return Error(ErrorCode::InvalidData, e.what());
        }
    }

    std::sort(snapshot.nodes.begin(), snapshot.nodes.end(), [](const auto& left, const auto& right) {
        return left.id < right.id;
    });

    for (size_t index = 0; index < snapshot.nodes.size(); ++index) {
        if (snapshot.nodes[index].id != static_cast<u32>(index)) {
            return Error(ErrorCode::InvalidData, "Command tree node ids must be contiguous starting at 0");
        }
    }

    return snapshot;
}

Result<CommandTreeSnapshot> CommandTreeSnapshot::fromJsonString(std::string_view jsonText)
{
    try {
        auto json = nlohmann::json::parse(jsonText.begin(), jsonText.end());
        return fromJson(json);
    }
    catch (const nlohmann::json::exception& e) {
        return Error(ErrorCode::InvalidData, e.what());
    }
}

} // namespace mc::command
