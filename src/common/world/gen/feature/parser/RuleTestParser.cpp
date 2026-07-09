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

#include "RuleTestParser.hpp"

#include "BlockStateParser.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"

#include <spdlog/spdlog.h>

#include <string>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace parser {
namespace RuleTestParser {

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

Result<std::unique_ptr<RuleTest>> parse(const nlohmann::json& predicateObj)
{
    if (!predicateObj.is_object() || !predicateObj.contains("predicate_type") ||
        !predicateObj["predicate_type"].is_string()) {
        return Error(ErrorCode::InvalidData, "RuleTest JSON missing 'predicate_type' string");
    }

    const std::string type = stripNamespace(predicateObj["predicate_type"].get<std::string>());

    if (type == "always_true") {
        // AlwaysTrueRuleTest 构造为 private（单例 INSTANCE）；clone() 返回指向 INSTANCE 的
        // 非拥有 unique_ptr。数据包未使用 always_true，此处仅作完整性支持。
        return AlwaysTrueRuleTest::INSTANCE.clone();
    }

    if (type == "block_match") {
        if (!predicateObj.contains("block") || !predicateObj["block"].is_string()) {
            return Error(ErrorCode::InvalidData, "block_match RuleTest missing 'block' string");
        }
        const ResourceLocation blockLoc(predicateObj["block"].get<std::string>());
        const Block* block = BlockRegistry::instance().getBlock(blockLoc);
        if (block == nullptr) {
            return Error(ErrorCode::NotFound, "block_match RuleTest: unknown block '" + blockLoc.toString() + "'");
        }
        return std::unique_ptr<RuleTest>(std::make_unique<BlockMatchRuleTest>(block));
    }

    if (type == "random_block_match") {
        if (!predicateObj.contains("block") || !predicateObj["block"].is_string() ||
            !predicateObj.contains("probability") || !predicateObj["probability"].is_number()) {
            return Error(ErrorCode::InvalidData, "random_block_match RuleTest missing 'block'/'probability'");
        }
        const ResourceLocation blockLoc(predicateObj["block"].get<std::string>());
        const Block* block = BlockRegistry::instance().getBlock(blockLoc);
        if (block == nullptr) {
            return Error(ErrorCode::NotFound, "random_block_match RuleTest: unknown block '" + blockLoc.toString() + "'");
        }
        const f32 probability = predicateObj["probability"].get<f32>();
        return std::unique_ptr<RuleTest>(std::make_unique<RandomBlockMatchRuleTest>(block, probability));
    }

    if (type == "tag_match") {
        if (!predicateObj.contains("tag") || !predicateObj["tag"].is_string()) {
            return Error(ErrorCode::InvalidData, "tag_match RuleTest missing 'tag' string");
        }
        const std::string tag = predicateObj["tag"].get<std::string>();
        return std::unique_ptr<RuleTest>(std::make_unique<TagMatchRuleTest>(tag));
    }

    if (type == "block_state_match" || type == "blockstate_match") {
        if (!predicateObj.contains("block_state") || !predicateObj["block_state"].is_object()) {
            return Error(ErrorCode::InvalidData, "block_state_match RuleTest missing 'block_state' object");
        }
        auto stateResult = BlockStateParser::parse(predicateObj["block_state"]);
        if (!stateResult.success()) {
            return stateResult.error();
        }
        return std::unique_ptr<RuleTest>(std::make_unique<BlockStateMatchRuleTest>(stateResult.value()));
    }

    if (type == "random_block_state_match") {
        if (!predicateObj.contains("block_state") || !predicateObj["block_state"].is_object() ||
            !predicateObj.contains("probability") || !predicateObj["probability"].is_number()) {
            return Error(ErrorCode::InvalidData,
                "random_block_state_match RuleTest missing 'block_state'/'probability'");
        }
        auto stateResult = BlockStateParser::parse(predicateObj["block_state"]);
        if (!stateResult.success()) {
            return stateResult.error();
        }
        const f32 probability = predicateObj["probability"].get<f32>();
        return std::unique_ptr<RuleTest>(
            std::make_unique<RandomBlockStateMatchRuleTest>(stateResult.value(), probability));
    }

    return Error(ErrorCode::NotFound, "unsupported RuleTest predicate_type '" + type + "'");
}

} // namespace RuleTestParser
} // namespace parser
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
