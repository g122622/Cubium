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

#include "BlockPredicateParser.hpp"

#include "BlockStateParser.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/gen/feature/predicate/AllOfPredicate.hpp"
#include "common/world/gen/feature/predicate/HasSturdyFacePredicate.hpp"
#include "common/world/gen/feature/predicate/MatchingBlockPredicate.hpp"
#include "common/world/gen/feature/predicate/ReplaceablePredicate.hpp"
#include "common/world/gen/feature/predicate/SolidBlockPredicate.hpp"
#include "common/world/gen/feature/predicate/TagMatchPredicate.hpp"
#include "common/world/gen/feature/predicate/TrueBlockPredicate.hpp"
#include "common/world/gen/feature/predicate/WouldSurvivePredicate.hpp"

#include <spdlog/spdlog.h>

#include <string>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace parser {
namespace BlockPredicateParser {

namespace {

std::string stripNamespace(const std::string& s)
{
    constexpr std::string_view prefix = "minecraft:";
    if (s.size() > prefix.size() && s.substr(0, prefix.size()) == prefix) {
        return s.substr(prefix.size());
    }
    return s;
}

/// 解析 "blocks" 字段（单个字符串或字符串数组）为 MatchingBlockPredicate 列表
Result<std::vector<std::unique_ptr<predicate::BlockPredicate>>> parseMatchingBlocks(const nlohmann::json& blocksField)
{
    std::vector<std::unique_ptr<predicate::BlockPredicate>> result;
    if (blocksField.is_string()) {
        const ResourceLocation blockLoc(blocksField.get<std::string>());
        const Block* block = BlockRegistry::instance().getBlock(blockLoc);
        if (block == nullptr) {
            return Error(ErrorCode::NotFound, "matching_blocks: unknown block '" + blockLoc.toString() + "'");
        }
        result.push_back(std::make_unique<predicate::MatchingBlockPredicate>(block));
        return result;
    }
    if (blocksField.is_array()) {
        for (const auto& entry : blocksField) {
            if (!entry.is_string()) {
                return Error(ErrorCode::InvalidData, "matching_blocks array entry must be a string");
            }
            const ResourceLocation blockLoc(entry.get<std::string>());
            const Block* block = BlockRegistry::instance().getBlock(blockLoc);
            if (block == nullptr) {
                return Error(ErrorCode::NotFound, "matching_blocks: unknown block '" + blockLoc.toString() + "'");
            }
            result.push_back(std::make_unique<predicate::MatchingBlockPredicate>(block));
        }
        return result;
    }
    return Error(ErrorCode::InvalidData, "matching_blocks 'blocks' must be string or array");
}

/// 解析 predicates 数组
Result<std::vector<std::unique_ptr<predicate::BlockPredicate>>> parsePredicateArray(const nlohmann::json& arr)
{
    if (!arr.is_array()) {
        return Error(ErrorCode::InvalidData, "predicate array must be a JSON array");
    }
    std::vector<std::unique_ptr<predicate::BlockPredicate>> result;
    for (const auto& entry : arr) {
        auto parsed = parse(entry);
        if (!parsed.success()) {
            return parsed.error();
        }
        result.push_back(parsed.value());
    }
    return result;
}

} // namespace

Result<std::unique_ptr<predicate::BlockPredicate>> parse(const nlohmann::json& predicateObj)
{
    if (!predicateObj.is_object() || !predicateObj.contains("type") || !predicateObj["type"].is_string()) {
        return Error(ErrorCode::InvalidData, "block predicate JSON missing 'type' string");
    }

    const std::string type = stripNamespace(predicateObj["type"].get<std::string>());

    if (type == "matching_blocks") {
        if (!predicateObj.contains("blocks")) {
            return Error(ErrorCode::InvalidData, "matching_blocks predicate missing 'blocks'");
        }
        auto blocksResult = parseMatchingBlocks(predicateObj["blocks"]);
        if (!blocksResult.success()) {
            return blocksResult.error();
        }
        auto& blocks = blocksResult.value();
        if (blocks.size() == 1) {
            return std::unique_ptr<predicate::BlockPredicate>(std::move(blocks.front()));
        }
        // 多方块 → any_of（任一匹配即满足）
        return std::unique_ptr<predicate::BlockPredicate>(
            std::make_unique<predicate::AnyOfPredicate>(std::move(blocks)));
    }

    if (type == "matching_block_tag") {
        if (!predicateObj.contains("tag") || !predicateObj["tag"].is_string()) {
            return Error(ErrorCode::InvalidData, "matching_block_tag predicate missing 'tag' string");
        }
        return std::unique_ptr<predicate::BlockPredicate>(
            std::make_unique<predicate::TagMatchPredicate>(predicateObj["tag"].get<std::string>()));
    }

    if (type == "all_of") {
        if (!predicateObj.contains("predicates")) {
            return Error(ErrorCode::InvalidData, "all_of predicate missing 'predicates'");
        }
        auto arrResult = parsePredicateArray(predicateObj["predicates"]);
        if (!arrResult.success()) {
            return arrResult.error();
        }
        return std::unique_ptr<predicate::BlockPredicate>(
            std::make_unique<predicate::AllOfPredicate>(std::move(arrResult.value())));
    }

    if (type == "any_of") {
        if (!predicateObj.contains("predicates")) {
            return Error(ErrorCode::InvalidData, "any_of predicate missing 'predicates'");
        }
        auto arrResult = parsePredicateArray(predicateObj["predicates"]);
        if (!arrResult.success()) {
            return arrResult.error();
        }
        return std::unique_ptr<predicate::BlockPredicate>(
            std::make_unique<predicate::AnyOfPredicate>(std::move(arrResult.value())));
    }

    if (type == "not") {
        if (!predicateObj.contains("predicate")) {
            return Error(ErrorCode::InvalidData, "not predicate missing 'predicate'");
        }
        auto childResult = parse(predicateObj["predicate"]);
        if (!childResult.success()) {
            return childResult.error();
        }
        return std::unique_ptr<predicate::BlockPredicate>(
            std::make_unique<predicate::NotPredicate>(childResult.value()));
    }

    if (type == "would_survive") {
        if (!predicateObj.contains("state")) {
            return Error(ErrorCode::InvalidData, "would_survive predicate missing 'state'");
        }
        auto stateResult = BlockStateParser::parse(predicateObj["state"]);
        if (!stateResult.success()) {
            return stateResult.error();
        }
        return std::unique_ptr<predicate::BlockPredicate>(
            std::make_unique<predicate::WouldSurvivePredicate>(stateResult.value()));
    }

    if (type == "replaceable") {
        return std::unique_ptr<predicate::BlockPredicate>(std::make_unique<predicate::ReplaceablePredicate>());
    }
    if (type == "solid") {
        return std::unique_ptr<predicate::BlockPredicate>(std::make_unique<predicate::SolidBlockPredicate>());
    }
    if (type == "true") {
        return std::unique_ptr<predicate::BlockPredicate>(std::make_unique<predicate::TrueBlockPredicate>());
    }
    if (type == "has_sturdy_face") {
        Direction dir = Direction::Up;
        if (predicateObj.contains("direction") && predicateObj["direction"].is_string()) {
            auto parsed = mc::Directions::fromName(predicateObj["direction"].get<std::string>());
            if (parsed.has_value()) {
                dir = *parsed;
            }
        }
        return std::unique_ptr<predicate::BlockPredicate>(std::make_unique<predicate::HasSturdyFacePredicate>(dir));
    }

    return Error(ErrorCode::NotFound, "unsupported block predicate type '" + type + "'");
}

} // namespace BlockPredicateParser
} // namespace parser
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
