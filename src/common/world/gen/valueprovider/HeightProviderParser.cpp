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

#include "HeightProviderParser.hpp"

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/gen/surface/VerticalAnchor.hpp"
#include "common/world/gen/valueprovider/HeightProvider.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace world {
namespace gen {
namespace valueprovider {
namespace HeightProviderParser {

namespace {

/**
 * @brief 剥离 "minecraft:" 命名空间前缀
 */
std::string stripNamespace(const std::string& s)
{
    constexpr std::string_view prefix = "minecraft:";
    if (s.size() > prefix.size() && s.substr(0, prefix.size()) == prefix) {
        return s.substr(prefix.size());
    }
    return s;
}

/**
 * @brief 读取一个整数字段（不存在或非整数时返回 defaultValue）
 */
i32 readI32(const nlohmann::json& obj, const std::string& key, i32 defaultValue)
{
    if (obj.contains(key) && obj[key].is_number_integer()) {
        return obj[key].get<i32>();
    }
    return defaultValue;
}

} // namespace

Result<surface::VerticalAnchor> parseAnchor(const nlohmann::json& json)
{
    if (!json.is_object()) {
        return Error(ErrorCode::InvalidData, "VerticalAnchor must be a JSON object");
    }

    if (json.contains("absolute") && json["absolute"].is_number_integer()) {
        return surface::VerticalAnchor::absolute(json["absolute"].get<i32>());
    }
    if (json.contains("above_bottom") && json["above_bottom"].is_number_integer()) {
        return surface::VerticalAnchor::aboveBottom(json["above_bottom"].get<i32>());
    }
    if (json.contains("below_top") && json["below_top"].is_number_integer()) {
        return surface::VerticalAnchor::belowTop(json["below_top"].get<i32>());
    }

    return Error(ErrorCode::InvalidData, "VerticalAnchor object missing absolute/above_bottom/below_top");
}

Result<std::unique_ptr<HeightProvider>> parse(const nlohmann::json& json)
{
    if (!json.is_object() || !json.contains("type") || !json["type"].is_string()) {
        return Error(ErrorCode::InvalidData, "HeightProvider missing 'type' string");
    }

    const std::string type = stripNamespace(json["type"].get<std::string>());

    // constant: 单个 VerticalAnchor（在 "value" 字段）
    if (type == "constant") {
        if (!json.contains("value") || !json["value"].is_object()) {
            return Error(ErrorCode::InvalidData, "constant HeightProvider missing 'value' anchor");
        }
        auto anchorResult = parseAnchor(json["value"]);
        if (!anchorResult.success()) {
            return Error(anchorResult.error().code(), anchorResult.error().message());
        }
        std::unique_ptr<HeightProvider> provider = ConstantHeight::create(anchorResult.value());
        return provider;
    }

    // uniform / biased_to_bottom / very_biased_to_bottom / trapezoid: 两个 anchor + 可选 inner/plateau
    if (type == "uniform" || type == "biased_to_bottom" || type == "very_biased_to_bottom" || type == "trapezoid") {
        if (!json.contains("min_inclusive") || !json["min_inclusive"].is_object()) {
            return Error(ErrorCode::InvalidData, type + " HeightProvider missing 'min_inclusive' anchor");
        }
        if (!json.contains("max_inclusive") || !json["max_inclusive"].is_object()) {
            return Error(ErrorCode::InvalidData, type + " HeightProvider missing 'max_inclusive' anchor");
        }

        auto minResult = parseAnchor(json["min_inclusive"]);
        if (!minResult.success()) {
            return Error(minResult.error().code(), minResult.error().message());
        }
        auto maxResult = parseAnchor(json["max_inclusive"]);
        if (!maxResult.success()) {
            return Error(maxResult.error().code(), maxResult.error().message());
        }

        const surface::VerticalAnchor min = minResult.value();
        const surface::VerticalAnchor max = maxResult.value();

        if (type == "uniform") {
            std::unique_ptr<HeightProvider> provider = UniformHeight::create(min, max);
            return provider;
        }
        if (type == "biased_to_bottom") {
            const i32 inner = readI32(json, "inner", 1);
            std::unique_ptr<HeightProvider> provider = BiasedToBottomHeight::create(min, max, inner);
            return provider;
        }
        if (type == "very_biased_to_bottom") {
            const i32 inner = readI32(json, "inner", 1);
            std::unique_ptr<HeightProvider> provider = VeryBiasedToBottomHeight::create(min, max, inner);
            return provider;
        }
        // trapezoid
        const i32 plateau = readI32(json, "plateau", 0);
        std::unique_ptr<HeightProvider> provider = TrapezoidHeight::create(min, max, plateau);
        return provider;
    }

    return Error(ErrorCode::InvalidData, "Unknown HeightProvider type: " + type);
}

} // namespace HeightProviderParser
} // namespace valueprovider
} // namespace gen
} // namespace world
} // namespace mc
