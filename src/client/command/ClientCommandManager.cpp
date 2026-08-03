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

#include "ClientCommandManager.hpp"

#include "common/command/CommandNode.hpp"
#include "common/command/CommandTreeSnapshot.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/suggestions/Suggestions.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc::client::command {

ClientCommandManager::ClientCommandManager() = default;

void ClientCommandManager::clear()
{
    m_snapshot.nodes.clear();
}

Result<void> ClientCommandManager::applyCommandTreeJson(std::string_view jsonText)
{
    auto snapshotResult = mc::command::CommandTreeSnapshot::fromJsonString(jsonText);
    if (snapshotResult.failed()) {
        clear();
        spdlog::error("Failed to apply command tree from JSON: {}", snapshotResult.error().message());
        return snapshotResult.error();
    }
    spdlog::info("Successfully applied command tree with {} nodes", snapshotResult.value().nodes.size());

    m_snapshot = std::move(snapshotResult.value());
    return Result<void>::ok();
}

bool ClientCommandManager::hasCommandTree() const noexcept
{
    return !m_snapshot.empty();
}

std::vector<std::string> ClientCommandManager::getCommandNames() const
{
    std::vector<std::string> names;
    if (m_snapshot.nodes.empty()) {
        return names;
    }

    const auto& root = m_snapshot.nodes.front();
    names.reserve(root.children.size());
    for (u32 childId : root.children) {
        const auto* child = _getNode(childId);
        if (child != nullptr && child->type == mc::command::NodeType::Literal) {
            names.push_back(child->name);
        }
    }

    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    return names;
}

mc::command::Suggestions ClientCommandManager::getSuggestions(std::string_view input, i32 cursor) const
{
    if (!hasCommandTree() || cursor <= 0 || input.empty()) {
        return mc::command::Suggestions::empty();
    }

    const i32 clampedCursor = std::clamp(cursor, 0, static_cast<i32>(input.size()));
    std::string_view prefixInput = input.substr(0, static_cast<size_t>(clampedCursor));

    if (!_isCommandInput(prefixInput)) {
        return mc::command::Suggestions::empty();
    }

    mc::command::StringReader reader(prefixInput);
    reader.skip();

    const auto* currentNode = _getNode(0);
    if (currentNode == nullptr) {
        return mc::command::Suggestions::empty();
    }

    while (true) {
        reader.skipWhitespace();
        if (!reader.canRead()) {
            return _collectSuggestions(*currentNode, input, clampedCursor, clampedCursor, "");
        }

        const i32 tokenStart = reader.getCursor();
        mc::command::StringReader tokenReader = reader;
        const std::string token = tokenReader.readUnquotedString();
        const i32 tokenEnd = tokenReader.getCursor();
        const bool tokenCompleted = tokenEnd < static_cast<i32>(prefixInput.size());

        if (!tokenCompleted) {
            return _collectSuggestions(*currentNode, input, tokenStart, tokenEnd, token);
        }

        const auto* exactLiteral = static_cast<const mc::command::CommandTreeNodeSnapshot*>(nullptr);
        const auto* argumentMatch = static_cast<const mc::command::CommandTreeNodeSnapshot*>(nullptr);

        for (u32 childId : currentNode->children) {
            const auto* child = _getNode(childId);
            if (child == nullptr) {
                continue;
            }

            if (child->type == mc::command::NodeType::Literal) {
                if (_toLower(child->name) == _toLower(token)) {
                    exactLiteral = child;
                    break;
                }
                continue;
            }

            if (argumentMatch == nullptr && _matchesFixedCandidate(*child, token)) {
                argumentMatch = child;
            }
        }

        if (exactLiteral != nullptr) {
            currentNode = exactLiteral;
            reader = tokenReader;
            continue;
        }

        if (argumentMatch == nullptr) {
            return _collectSuggestions(*currentNode, input, tokenStart, tokenEnd, token);
        }

        currentNode = argumentMatch;
        reader = tokenReader;
    }
}

void ClientCommandManager::setPlayerNameProvider(CandidateProvider provider)
{
    m_playerNameProvider = std::move(provider);
}

void ClientCommandManager::setEntityNameProvider(CandidateProvider provider)
{
    m_entityNameProvider = std::move(provider);
}

void ClientCommandManager::setItemNameProvider(CandidateProvider provider)
{
    m_itemNameProvider = std::move(provider);
}

const mc::command::CommandTreeNodeSnapshot* ClientCommandManager::_getNode(u32 nodeId) const
{
    if (nodeId >= m_snapshot.nodes.size()) {
        return nullptr;
    }
    return &m_snapshot.nodes[nodeId];
}

mc::command::Suggestions ClientCommandManager::_collectSuggestions(const mc::command::CommandTreeNodeSnapshot& node,
    std::string_view fullInput,
    i32 start,
    i32 end,
    std::string_view tokenPrefix) const
{
    mc::command::SuggestionsBuilder builder(fullInput, start, end);

    for (u32 childId : node.children) {
        const auto* child = _getNode(childId);
        if (child == nullptr || child->type != mc::command::NodeType::Literal) {
            continue;
        }

        if (tokenPrefix.empty() || _startsWithIgnoreCase(child->name, tokenPrefix)) {
            builder.suggest(child->name);
        }
    }

    for (u32 childId : node.children) {
        const auto* child = _getNode(childId);
        if (child == nullptr || child->type != mc::command::NodeType::Argument) {
            continue;
        }

        const auto candidates = _getCandidates(*child);
        for (const auto& candidate : candidates) {
            if (tokenPrefix.empty() || _startsWithIgnoreCase(candidate, tokenPrefix)) {
                builder.suggest(candidate);
            }
        }
    }

    return builder.build();
}

std::vector<std::string> ClientCommandManager::_getCandidates(const mc::command::CommandTreeNodeSnapshot& node) const
{
    std::vector<std::string> candidates;

    switch (node.suggestionKind) {
        case mc::command::CommandTreeSuggestionKind::None:
            candidates = node.examples;
            break;
        case mc::command::CommandTreeSuggestionKind::Fixed:
            candidates = node.suggestionCandidates;
            if (candidates.empty()) {
                candidates = node.examples;
            }
            break;
        case mc::command::CommandTreeSuggestionKind::CommandNames:
            candidates = getCommandNames();
            break;
        case mc::command::CommandTreeSuggestionKind::PlayerNames:
            if (m_playerNameProvider) {
                candidates = m_playerNameProvider();
            }
            if (candidates.empty()) {
                candidates = node.examples;
            }
            break;
        case mc::command::CommandTreeSuggestionKind::EntityNames:
            if (m_entityNameProvider) {
                candidates = m_entityNameProvider();
            }
            if (candidates.empty()) {
                candidates = node.examples;
            }
            break;
        case mc::command::CommandTreeSuggestionKind::ItemNames:
            if (m_itemNameProvider) {
                candidates = m_itemNameProvider();
            } else {
                Item::forEachItem([&candidates](Item& item) { candidates.push_back(item.itemLocation().toString()); });
            }
            if (candidates.empty()) {
                candidates = node.examples;
            }
            break;
    }

    std::sort(candidates.begin(), candidates.end());
    candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());
    return candidates;
}

bool ClientCommandManager::_matchesFixedCandidate(
    const mc::command::CommandTreeNodeSnapshot& node, std::string_view token) const
{
    switch (node.suggestionKind) {
        case mc::command::CommandTreeSuggestionKind::Fixed:
        case mc::command::CommandTreeSuggestionKind::CommandNames:
            for (const auto& candidate : _getCandidates(node)) {
                if (_toLower(candidate) == _toLower(token)) {
                    return true;
                }
            }
            return false;
        case mc::command::CommandTreeSuggestionKind::None:
        case mc::command::CommandTreeSuggestionKind::PlayerNames:
        case mc::command::CommandTreeSuggestionKind::EntityNames:
        case mc::command::CommandTreeSuggestionKind::ItemNames:
            return !token.empty();
    }
    return !token.empty();
}

bool ClientCommandManager::_isCommandInput(std::string_view input)
{
    return !input.empty() && input.front() == '/';
}

std::string ClientCommandManager::_toLower(std::string_view input)
{
    std::string result(input.begin(), input.end());
    for (char& character : result) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return result;
}

bool ClientCommandManager::_startsWithIgnoreCase(std::string_view value, std::string_view prefix)
{
    if (prefix.size() > value.size()) {
        return false;
    }

    for (size_t index = 0; index < prefix.size(); ++index) {
        const unsigned char left = static_cast<unsigned char>(value[index]);
        const unsigned char right = static_cast<unsigned char>(prefix[index]);
        if (std::tolower(left) != std::tolower(right)) {
            return false;
        }
    }

    return true;
}

} // namespace mc::client::command
