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

#include "TrunkPlacerParser.hpp"

#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/gen/feature/tree/trunk/StraightTrunkPlacer.hpp"
#include "common/world/gen/feature/tree/trunk/TrunkPlacers.hpp"
#include "common/world/gen/feature/tree/trunk/UpwardsBranchingTrunkPlacer.hpp"
#include "common/world/gen/valueprovider/IntProviderParser.hpp"

#include <spdlog/spdlog.h>

#include <string>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace parser {
namespace TrunkPlacerParser {

namespace {

std::string stripNamespace(const std::string& s)
{
    constexpr std::string_view prefix = "minecraft:";
    if (s.size() > prefix.size() && s.substr(0, prefix.size()) == prefix) {
        return s.substr(prefix.size());
    }
    return s;
}

/**
 * @brief 读取 base_height / height_rand_a / height_rand_b 三个通用字段
 */
Result<bool> readBaseHeights(const nlohmann::json& obj, i32& baseHeight, i32& heightRandA, i32& heightRandB)
{
    if (!obj.contains("base_height") || !obj["base_height"].is_number_integer() || !obj.contains("height_rand_a") ||
        !obj["height_rand_a"].is_number_integer() || !obj.contains("height_rand_b") ||
        !obj["height_rand_b"].is_number_integer()) {
        return Error(
            ErrorCode::InvalidData, "trunk placer missing base_height/height_rand_a/height_rand_b integer fields");
    }
    baseHeight = obj["base_height"].get<i32>();
    heightRandA = obj["height_rand_a"].get<i32>();
    heightRandB = obj["height_rand_b"].get<i32>();
    return true;
}

i32 getInt(const nlohmann::json& obj, const char* key)
{
    return (obj.contains(key) && obj[key].is_number_integer()) ? obj[key].get<i32>() : 0;
}

f32 getFloat(const nlohmann::json& obj, const char* key, f32 fallback)
{
    return (obj.contains(key) && obj[key].is_number()) ? obj[key].get<f32>() : fallback;
}

/// 解析 can_grow_through（"#minecraft:xxx" 标签字符串 → const BlockTag*）。
const BlockTag* parseCanGrowThrough(const nlohmann::json& field)
{
    if (!field.is_string()) {
        return nullptr;
    }
    const std::string entry = field.get<std::string>();
    if (entry.empty() || entry[0] != '#') {
        return nullptr;
    }
    return BlockTags::getTag(ResourceLocation(entry.substr(1)));
}

} // namespace

Result<std::unique_ptr<TrunkPlacer>> parse(const nlohmann::json& placerObj)
{
    if (!placerObj.is_object() || !placerObj.contains("type") || !placerObj["type"].is_string()) {
        return Error(ErrorCode::InvalidData, "trunk placer JSON missing 'type' string");
    }

    const std::string type = stripNamespace(placerObj["type"].get<std::string>());

    i32 baseHeight = 0;
    i32 heightRandA = 0;
    i32 heightRandB = 0;
    if (auto baseResult = readBaseHeights(placerObj, baseHeight, heightRandA, heightRandB); !baseResult.success()) {
        return baseResult.error();
    }

    if (type == "straight_trunk_placer") {
        return std::unique_ptr<TrunkPlacer>(
            std::make_unique<StraightTrunkPlacer>(baseHeight, heightRandA, heightRandB));
    }
    if (type == "dark_oak_trunk_placer") {
        return std::unique_ptr<TrunkPlacer>(std::make_unique<DarkOakTrunkPlacer>(baseHeight, heightRandA, heightRandB));
    }
    if (type == "fancy_trunk_placer") {
        return std::unique_ptr<TrunkPlacer>(std::make_unique<FancyTrunkPlacer>(baseHeight, heightRandA, heightRandB));
    }
    if (type == "forking_trunk_placer") {
        return std::unique_ptr<TrunkPlacer>(std::make_unique<ForkyTrunkPlacer>(baseHeight, heightRandA, heightRandB));
    }
    if (type == "giant_trunk_placer") {
        return std::unique_ptr<TrunkPlacer>(std::make_unique<GiantTrunkPlacer>(baseHeight, heightRandA, heightRandB));
    }
    if (type == "mega_jungle_trunk_placer") {
        return std::unique_ptr<TrunkPlacer>(
            std::make_unique<MegaJungleTrunkPlacer>(baseHeight, heightRandA, heightRandB));
    }
    if (type == "bending_trunk_placer") {
        const i32 minHeightForLeaves = getInt(placerObj, "min_height_for_leaves");
        if (!placerObj.contains("bend_length")) {
            return Error(ErrorCode::InvalidData, "bending_trunk_placer missing 'bend_length' IntProvider");
        }
        auto bendResult = valueprovider::IntProviderParser::parse(placerObj["bend_length"]);
        if (!bendResult.success()) {
            return bendResult.error();
        }
        return std::unique_ptr<TrunkPlacer>(std::make_unique<BendingTrunkPlacer>(
            baseHeight, heightRandA, heightRandB, minHeightForLeaves, bendResult.value()));
    }
    if (type == "cherry_trunk_placer") {
        return std::unique_ptr<TrunkPlacer>(std::make_unique<CherryTrunkPlacer>(baseHeight,
            heightRandA,
            heightRandB,
            getInt(placerObj, "branch_count_min"),
            getInt(placerObj, "branch_count_max"),
            getInt(placerObj, "branch_horizontal_length_min"),
            getInt(placerObj, "branch_horizontal_length_max"),
            getInt(placerObj, "branch_start_offset_from_top_min"),
            getInt(placerObj, "branch_start_offset_from_top_max"),
            getInt(placerObj, "branch_end_offset_from_top_min"),
            getInt(placerObj, "branch_end_offset_from_top_max")));
    }
    if (type == "upwards_branching_trunk_placer") {
        if (!placerObj.contains("extra_branch_steps")) {
            return Error(
                ErrorCode::InvalidData, "upwards_branching_trunk_placer missing 'extra_branch_steps' IntProvider");
        }
        auto stepsResult = valueprovider::IntProviderParser::parse(placerObj["extra_branch_steps"], 1);
        if (!stepsResult.success()) {
            return stepsResult.error();
        }
        if (!placerObj.contains("extra_branch_length")) {
            return Error(
                ErrorCode::InvalidData, "upwards_branching_trunk_placer missing 'extra_branch_length' IntProvider");
        }
        auto lengthResult = valueprovider::IntProviderParser::parse(placerObj["extra_branch_length"], 0);
        if (!lengthResult.success()) {
            return lengthResult.error();
        }
        const f32 branchProb = getFloat(placerObj, "place_branch_per_log_probability", 0.0f);
        const BlockTag* canGrowThrough = nullptr;
        if (placerObj.contains("can_grow_through")) {
            canGrowThrough = parseCanGrowThrough(placerObj["can_grow_through"]);
            if (canGrowThrough == nullptr) {
                return Error(
                    ErrorCode::NotFound, "upwards_branching_trunk_placer can_grow_through must be a '#tag' string");
            }
        }
        return std::unique_ptr<TrunkPlacer>(std::make_unique<UpwardsBranchingTrunkPlacer>(baseHeight,
            heightRandA,
            heightRandB,
            std::move(stepsResult).value(),
            branchProb,
            std::move(lengthResult).value(),
            canGrowThrough));
    }

    return Error(ErrorCode::NotFound, "unsupported trunk placer type '" + type + "'");
}

} // namespace TrunkPlacerParser
} // namespace parser
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
