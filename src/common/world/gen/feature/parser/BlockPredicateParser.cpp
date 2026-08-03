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
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/gen/feature/predicate/AllOfPredicate.hpp"
#include "common/world/gen/feature/predicate/BlockPredicate.hpp"
#include "common/world/gen/feature/predicate/HasSturdyFacePredicate.hpp"
#include "common/world/gen/feature/predicate/InsideWorldBoundsPredicate.hpp"
#include "common/world/gen/feature/predicate/MatchingBlockPredicate.hpp"
#include "common/world/gen/feature/predicate/MatchingFluidsPredicate.hpp"
#include "common/world/gen/feature/predicate/ReplaceablePredicate.hpp"
#include "common/world/gen/feature/predicate/SolidBlockPredicate.hpp"
#include "common/world/gen/feature/predicate/TagMatchPredicate.hpp"
#include "common/world/gen/feature/predicate/TrueBlockPredicate.hpp"
#include "common/world/gen/feature/predicate/WouldSurvivePredicate.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>

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

/// 解析可选的 "offset" 字段（[x,y,z] 整数数组，默认 0,0,0）。
/// 对齐 MC StateTestingCodec 的 Vec3i.offsetCodec(16).optionalFieldOf("offset", Vec3i.ZERO)。
Result<BlockPos> parseOffset(const nlohmann::json& predicateObj)
{
    BlockPos offset(0, 0, 0);
    if (!predicateObj.contains("offset")) {
        return offset;
    }
    const auto& arr = predicateObj["offset"];
    if (!arr.is_array() || arr.size() != 3) {
        return Error(ErrorCode::InvalidData, "block predicate 'offset' must be a 3-element int array");
    }
    for (i32 i = 0; i < 3; ++i) {
        if (!arr[i].is_number_integer()) {
            return Error(ErrorCode::InvalidData, "block predicate 'offset' entries must be integers");
        }
    }
    offset = BlockPos(arr[0].get<i32>(), arr[1].get<i32>(), arr[2].get<i32>());
    return offset;
}

/// 解析 "blocks" 字段（单个字符串或字符串数组）为 MatchingBlockPredicate 列表
Result<std::vector<std::unique_ptr<predicate::BlockPredicate>>> parseMatchingBlocks(
    const nlohmann::json& blocksField, const BlockPos& offset)
{
    std::vector<std::unique_ptr<predicate::BlockPredicate>> result;
    if (blocksField.is_string()) {
        const ResourceLocation blockLoc(blocksField.get<std::string>());
        const Block* block = BlockRegistry::instance().getBlock(blockLoc);
        if (block == nullptr) {
            return Error(ErrorCode::NotFound, "matching_blocks: unknown block '" + blockLoc.toString() + "'");
        }
        result.push_back(std::make_unique<predicate::MatchingBlockPredicate>(block, offset));
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
            result.push_back(std::make_unique<predicate::MatchingBlockPredicate>(block, offset));
        }
        return result;
    }
    return Error(ErrorCode::InvalidData, "matching_blocks 'blocks' must be string or array");
}

/// 解析 "fluids" 字段（单个字符串或字符串数组），每项为流体 id 或 "#tag" 引用。
/// 对齐 MC RegistryCodecs.homogeneousList(Registries.FLUID)。
Result<std::pair<std::vector<const fluid::Fluid*>, std::vector<const fluid::FluidTag*>>> parseMatchingFluids(
    const nlohmann::json& fluidsField)
{
    std::vector<const fluid::Fluid*> fluids;
    std::vector<const fluid::FluidTag*> tags;
    auto parseOne = [&](const std::string& entry) -> Result<void> {
        if (!entry.empty() && entry[0] == '#') {
            const ResourceLocation tagLoc(entry.substr(1));
            fluid::FluidTag* tag = fluid::FluidTags::getTag(tagLoc);
            if (tag == nullptr) {
                return Error(ErrorCode::NotFound, "matching_fluids: unknown fluid tag '" + tagLoc.toString() + "'");
            }
            tags.push_back(tag);
            return {};
        }
        const ResourceLocation fluidLoc(entry);
        fluid::Fluid* fluid = fluid::FluidRegistry::instance().getFluid(fluidLoc);
        if (fluid == nullptr) {
            return Error(ErrorCode::NotFound, "matching_fluids: unknown fluid '" + fluidLoc.toString() + "'");
        }
        fluids.push_back(fluid);
        return {};
    };

    if (fluidsField.is_string()) {
        auto r = parseOne(fluidsField.get<std::string>());
        if (!r.success()) {
            return r.error();
        }
        return std::make_pair(std::move(fluids), std::move(tags));
    }
    if (fluidsField.is_array()) {
        for (const auto& entry : fluidsField) {
            if (!entry.is_string()) {
                return Error(ErrorCode::InvalidData, "matching_fluids array entry must be a string");
            }
            auto r = parseOne(entry.get<std::string>());
            if (!r.success()) {
                return r.error();
            }
        }
        return std::make_pair(std::move(fluids), std::move(tags));
    }
    return Error(ErrorCode::InvalidData, "matching_fluids 'fluids' must be string or array");
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
        auto offsetResult = parseOffset(predicateObj);
        if (!offsetResult.success()) {
            return offsetResult.error();
        }
        auto blocksResult = parseMatchingBlocks(predicateObj["blocks"], offsetResult.value());
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

    if (type == "matching_fluids") {
        if (!predicateObj.contains("fluids")) {
            return Error(ErrorCode::InvalidData, "matching_fluids predicate missing 'fluids'");
        }
        auto offsetResult = parseOffset(predicateObj);
        if (!offsetResult.success()) {
            return offsetResult.error();
        }
        auto fluidsResult = parseMatchingFluids(predicateObj["fluids"]);
        if (!fluidsResult.success()) {
            return fluidsResult.error();
        }
        auto& [fluids, tags] = fluidsResult.value();
        return std::unique_ptr<predicate::BlockPredicate>(std::make_unique<predicate::MatchingFluidsPredicate>(
            std::move(fluids), std::move(tags), offsetResult.value()));
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
        // MC: optional "offset" (Vec3i, 默认 ZERO)
        auto offsetResult = parseOffset(predicateObj);
        if (!offsetResult.success()) {
            return offsetResult.error();
        }
        return std::unique_ptr<predicate::BlockPredicate>(
            std::make_unique<predicate::HasSturdyFacePredicate>(dir, offsetResult.value()));
    }

    if (type == "inside_world_bounds") {
        // MC: optional "offset" (Vec3i, 默认 ZERO)；test = !isOutsideBuildHeight(pos+offset)
        auto offsetResult = parseOffset(predicateObj);
        if (!offsetResult.success()) {
            return offsetResult.error();
        }
        return std::unique_ptr<predicate::BlockPredicate>(
            std::make_unique<predicate::InsideWorldBoundsPredicate>(offsetResult.value()));
    }

    return Error(ErrorCode::NotFound, "unsupported block predicate type '" + type + "'");
}

} // namespace BlockPredicateParser
} // namespace parser
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
