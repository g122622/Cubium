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

#include "BlockStateProviderParser.hpp"

#include "BlockStateParser.hpp"

#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>

#include <string>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace parser {

const BlockState* BlockStateProviderHandle::asSingle() const noexcept
{
    // 仅 Simple 情形可无随机源取单一状态；Weighted 情形调用方须自行持有 weighted 提供者。
    return kind == Kind::Simple ? simple : nullptr;
}

namespace BlockStateProviderParser {

namespace {

std::string stripNamespace(const std::string& s)
{
    constexpr std::string_view prefix = "minecraft:";
    if (s.size() > prefix.size() && s.substr(0, prefix.size()) == prefix) {
        return s.substr(prefix.size());
    }
    return s;
}

} // namespace

Result<BlockStateProviderHandle> parse(const nlohmann::json& providerObj)
{
    if (!providerObj.is_object() || !providerObj.contains("type") || !providerObj["type"].is_string()) {
        return Error(ErrorCode::InvalidData, "block state provider JSON missing 'type' string");
    }

    const std::string type = stripNamespace(providerObj["type"].get<std::string>());

    if (type == "simple_state_provider") {
        if (!providerObj.contains("state")) {
            return Error(ErrorCode::InvalidData, "simple_state_provider missing 'state' field");
        }
        auto stateResult = BlockStateParser::parse(providerObj["state"]);
        if (!stateResult.success()) {
            return stateResult.error();
        }
        BlockStateProviderHandle handle;
        handle.kind = BlockStateProviderHandle::Kind::Simple;
        handle.simple = stateResult.value();
        return handle;
    }

    if (type == "weighted_state_provider") {
        if (!providerObj.contains("entries") || !providerObj["entries"].is_array()) {
            return Error(ErrorCode::InvalidData, "weighted_state_provider missing 'entries' array");
        }
        auto weighted = std::make_unique<state::WeightedBlockStateProvider>();
        for (size_t i = 0; i < providerObj["entries"].size(); ++i) {
            const auto& entry = providerObj["entries"][i];
            if (!entry.contains("data")) {
                return Error(
                    ErrorCode::InvalidData, "weighted_state_provider entry[" + std::to_string(i) + "] missing 'data'");
            }
            auto entryState = BlockStateParser::parse(entry["data"]);
            if (!entryState.success()) {
                return entryState.error();
            }
            if (!entry.contains("weight") || !entry["weight"].is_number_integer()) {
                return Error(ErrorCode::InvalidData,
                    "weighted_state_provider entry[" + std::to_string(i) + "] missing 'weight'");
            }
            weighted->add(entryState.value(), entry["weight"].get<i32>());
        }
        BlockStateProviderHandle handle;
        handle.kind = BlockStateProviderHandle::Kind::Weighted;
        handle.weighted = std::move(weighted);
        return handle;
    }

    return Error(ErrorCode::NotFound,
        "unsupported block state provider type '" + type +
            "' (only simple_state_provider/weighted_state_provider implemented)");
}

} // namespace BlockStateProviderParser
} // namespace parser
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
