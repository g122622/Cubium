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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/json/JsonRead.hpp"
#include <algorithm>
#include <cstddef>
#include <exception>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

using namespace mc::trace;

namespace mc::command {

namespace {

/// 读取字符串数组字段；缺失或类型不符时返回空数组
[[nodiscard]] std::vector<std::string> _readStringArray(const nlohmann::json& json, std::string_view field)
{
    std::vector<std::string> result;
    const auto* array = util::json::findField(json, field);
    if (array == nullptr || !array->is_array()) {
        return result;
    }
    result.reserve(array->size());
    for (const auto& entry : *array) {
        const auto text = util::json::getString(entry);
        if (text.failed()) {
            continue;
        }
        result.push_back(text.value());
    }
    return result;
}

/// 读取无符号 32 位整数数组字段；缺失或类型不符时返回空数组
[[nodiscard]] std::vector<u32> _readU32Array(const nlohmann::json& json, std::string_view field)
{
    std::vector<u32> result;
    const auto* array = util::json::findField(json, field);
    if (array == nullptr || !array->is_array()) {
        return result;
    }
    result.reserve(array->size());
    for (const auto& entry : *array) {
        const auto number = util::json::getInt64(entry);
        if (number.failed()) {
            continue;
        }
        result.push_back(static_cast<u32>(number.value()));
    }
    return result;
}

[[nodiscard]] Result<CommandTreeNodeSnapshot> parseNodeSnapshot(const nlohmann::json& json)
{
    CommandTreeNodeSnapshot snapshot;

    MC_TRY_ASSIGN(snapshot.id, util::json::getInt64(json, "id").map<u32>([](i64 id) {
        return static_cast<u32>(id);
    }));
    MC_TRY_DECLARE(typeName, util::json::getString(json, "type"));
    snapshot.type = detail::parseNodeType(typeName);
    snapshot.name = util::json::getStringOr(json, "name", std::string_view{});
    snapshot.typeName = util::json::getStringOr(json, "typeName", snapshot.name);
    snapshot.metadata = util::json::getObjectOrEmpty(json, "metadata");
    snapshot.examples = _readStringArray(json, "examples");
    snapshot.children = _readU32Array(json, "children");
    snapshot.redirectModifier =
        detail::parseRedirectModifier(util::json::getStringOr(json, "redirectModifier", "none"));
    snapshot.executable = util::json::getBoolOr(json, "executable", false);
    snapshot.suggestionKind = detail::parseSuggestionKind(util::json::getStringOr(json, "suggestionKind", "none"));
    snapshot.suggestionCandidates = _readStringArray(json, "suggestionCandidates");

    if (const auto* redirect = util::json::findField(json, "redirect"); redirect != nullptr && !redirect->is_null()) {
        MC_TRY_ASSIGN(snapshot.redirect, util::json::getInt64(*redirect).map<u32>([](i64 id) {
            return static_cast<u32>(id);
        }));
    }

    // 二进制 CommandNode 编码字段（向后兼容：缺失时取默认值）。
    if (const auto* networkId = util::json::findField(json, "argumentNetworkId");
        networkId != nullptr && !networkId->is_null()) {
        MC_TRY_ASSIGN(snapshot.argumentNetworkId, util::json::getInt32(*networkId));
    }
    snapshot.argumentProperties = util::json::getObjectOrEmpty(json, "argumentProperties");
    if (const auto* providerId = util::json::findField(json, "suggestionProviderId");
        providerId != nullptr && !providerId->is_null()) {
        MC_TRY_ASSIGN(snapshot.suggestionProviderId, util::json::getString(*providerId));
    }
    snapshot.restricted = util::json::getBoolOr(json, "restricted", false);

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

    if (!json.contains("nodes") || !json["nodes"].is_array()) {
        return Error(ErrorCode::InvalidData, "Command tree snapshot is missing nodes");
    }

    CommandTreeSnapshot snapshot;
    const auto& nodeArray = json["nodes"];
    snapshot.nodes.reserve(nodeArray.size());

    for (const auto& nodeJson : nodeArray) {
        if (!nodeJson.is_object()) {
            return Error(ErrorCode::InvalidData, "Command tree node must be a JSON object");
        }

        auto nodeSnapshot = parseNodeSnapshot(nodeJson);
        if (nodeSnapshot.failed()) {
            return nodeSnapshot.error();
        }
        snapshot.nodes.push_back(std::move(nodeSnapshot).value());
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
