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

#include "FoliagePlacerParser.hpp"

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/gen/feature/FeatureSpread.hpp"
#include "common/world/gen/feature/tree/foliage/BlobFoliagePlacer.hpp"
#include "common/world/gen/feature/tree/foliage/CherryFoliagePlacer.hpp"
#include "common/world/gen/feature/tree/foliage/FoliagePlacer.hpp"
#include "common/world/gen/feature/tree/foliage/FoliagePlacers.hpp"
#include "common/world/gen/feature/tree/foliage/RandomSpreadFoliagePlacer.hpp"
#include "common/world/gen/valueprovider/IntProviderParser.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace parser {
namespace FoliagePlacerParser {

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
 * @brief 读取 radius/offset 字段为 FeatureSpread（MC 1.21 为 IntProvider）
 *
 * 接受裸整数（简写为 ConstantInt）或 IntProvider 对象（如 uniform）。
 */
Result<bool> readRadiusOffset(const nlohmann::json& obj, FeatureSpread& radius, FeatureSpread& offset)
{
    if (!obj.contains("radius") || !obj.contains("offset")) {
        return Error(ErrorCode::InvalidData, "foliage placer missing 'radius'/'offset' fields");
    }
    auto radiusResult = valueprovider::IntProviderParser::parse(obj["radius"]);
    if (!radiusResult.success()) {
        return radiusResult.error();
    }
    auto offsetResult = valueprovider::IntProviderParser::parse(obj["offset"]);
    if (!offsetResult.success()) {
        return offsetResult.error();
    }
    radius = FeatureSpread::of(std::move(radiusResult).value());
    offset = FeatureSpread::of(std::move(offsetResult).value());
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

} // namespace

Result<std::unique_ptr<FoliagePlacer>> parse(const nlohmann::json& placerObj)
{
    if (!placerObj.is_object() || !placerObj.contains("type") || !placerObj["type"].is_string()) {
        return Error(ErrorCode::InvalidData, "foliage placer JSON missing 'type' string");
    }

    const std::string type = stripNamespace(placerObj["type"].get<std::string>());

    FeatureSpread radius;
    FeatureSpread offset;
    if (auto roResult = readRadiusOffset(placerObj, radius, offset); !roResult.success()) {
        return roResult.error();
    }

    if (type == "blob_foliage_placer") {
        return std::unique_ptr<FoliagePlacer>(
            std::make_unique<BlobFoliagePlacer>(radius, offset, getInt(placerObj, "height")));
    }
    if (type == "pine_foliage_placer") {
        return std::unique_ptr<FoliagePlacer>(
            std::make_unique<PineFoliagePlacer>(radius, offset, getInt(placerObj, "height")));
    }
    if (type == "spruce_foliage_placer") {
        return std::unique_ptr<FoliagePlacer>(
            std::make_unique<SpruceFoliagePlacer>(radius, offset, getInt(placerObj, "height")));
    }
    if (type == "acacia_foliage_placer") {
        return std::unique_ptr<FoliagePlacer>(std::make_unique<AcaciaFoliagePlacer>(radius, offset));
    }
    if (type == "dark_oak_foliage_placer") {
        return std::unique_ptr<FoliagePlacer>(
            std::make_unique<DarkOakFoliagePlacer>(radius, offset, getInt(placerObj, "height")));
    }
    if (type == "jungle_foliage_placer") {
        return std::unique_ptr<FoliagePlacer>(
            std::make_unique<JungleFoliagePlacer>(radius, offset, getInt(placerObj, "height")));
    }
    if (type == "mega_pine_foliage_placer") {
        return std::unique_ptr<FoliagePlacer>(
            std::make_unique<MegaPineFoliagePlacer>(radius, offset, getInt(placerObj, "height")));
    }
    if (type == "bush_foliage_placer") {
        return std::unique_ptr<FoliagePlacer>(std::make_unique<BushFoliagePlacer>(radius, offset));
    }
    if (type == "fancy_foliage_placer") {
        return std::unique_ptr<FoliagePlacer>(
            std::make_unique<FancyFoliagePlacer>(radius, offset, getInt(placerObj, "height")));
    }
    if (type == "cherry_foliage_placer") {
        return std::unique_ptr<FoliagePlacer>(std::make_unique<CherryFoliagePlacer>(radius,
            offset,
            getInt(placerObj, "height"),
            getFloat(placerObj, "wide_bottom_layer_hole_chance", 0.0f),
            getFloat(placerObj, "corner_hole_chance", 0.0f),
            getFloat(placerObj, "hanging_leaves_chance", 0.0f),
            getFloat(placerObj, "hanging_leaves_extension_chance", 0.0f)));
    }
    if (type == "random_spread_foliage_placer") {
        if (!placerObj.contains("foliage_height")) {
            return Error(ErrorCode::InvalidData, "random_spread_foliage_placer missing 'foliage_height' IntProvider");
        }
        auto fhResult = valueprovider::IntProviderParser::parse(placerObj["foliage_height"]);
        if (!fhResult.success()) {
            return fhResult.error();
        }
        return std::unique_ptr<FoliagePlacer>(std::make_unique<RandomSpreadFoliagePlacer>(
            radius, offset, fhResult.value(), getInt(placerObj, "leaf_placement_attempts")));
    }

    return Error(ErrorCode::NotFound, "unsupported foliage placer type '" + type + "'");
}

} // namespace FoliagePlacerParser
} // namespace parser
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
