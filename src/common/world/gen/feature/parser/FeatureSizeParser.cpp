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

#include "FeatureSizeParser.hpp"

#include <spdlog/spdlog.h>

#include <optional>
#include <string>
#include <nlohmann/json.hpp>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace parser {
namespace FeatureSizeParser {

namespace {

/// 去掉 "minecraft:" 命名空间前缀
std::string stripNamespace(const std::string& s)
{
    constexpr std::string_view prefix = "minecraft:";
    if (s.size() > prefix.size() && s.substr(0, prefix.size()) == prefix) {
        return s.substr(prefix.size());
    }
    return s;
}

/**
 * @brief 读取裸整数字段，缺失时返回 fallback
 *
 * MC 1.21.11 中 FeatureSize 的 limit/size 字段都是裸整数（非 IntProvider），
 * Codec 使用 intRange(0, 81) 限制范围。此处复刻该范围校验。
 */
Result<i32> getInt(const nlohmann::json& obj, const char* key, bool required)
{
    if (!obj.contains(key) || !obj[key].is_number_integer()) {
        if (required) {
            return Error(
                ErrorCode::InvalidData, std::string("feature_size missing required integer field '") + key + "'");
        }
        return 0;
    }
    constexpr i32 MIN_VAL = 0;
    constexpr i32 MAX_VAL = 81;
    i32 value = obj[key].get<i32>();
    if (value < MIN_VAL || value > MAX_VAL) {
        return Error(ErrorCode::InvalidData,
            std::string("feature_size field '") + key + "' out of range [0, 81]: " + std::to_string(value));
    }
    return value;
}

/**
 * @brief 读取可选的 min_clipped_height 字段
 *
 * 对应 MC 1.21.11 FeatureSize.minClippedHeight（OptionalInt，Codec 范围 [0, 80]）。
 * 缺失时返回 std::nullopt，表示不允许裁剪生成。
 */
Result<std::optional<i32>> getOptionalInt(const nlohmann::json& obj, const char* key)
{
    if (!obj.contains(key)) {
        return std::optional<i32>{};
    }
    if (!obj[key].is_number_integer()) {
        return Error(ErrorCode::InvalidData, std::string("feature_size optional field '") + key + "' must be integer");
    }
    constexpr i32 MIN_VAL = 0;
    constexpr i32 MAX_VAL = 80;
    i32 value = obj[key].get<i32>();
    if (value < MIN_VAL || value > MAX_VAL) {
        return Error(ErrorCode::InvalidData,
            std::string("feature_size field '") + key + "' out of range [0, 80]: " + std::to_string(value));
    }
    return std::optional<i32>{value};
}

} // namespace

Result<std::unique_ptr<FeatureSize>> parse(const nlohmann::json& sizeObj)
{
    if (!sizeObj.is_object() || !sizeObj.contains("type") || !sizeObj["type"].is_string()) {
        return Error(ErrorCode::InvalidData, "feature_size JSON missing 'type' string");
    }

    const std::string type = stripNamespace(sizeObj["type"].get<std::string>());

    auto minClippedResult = getOptionalInt(sizeObj, "min_clipped_height");
    if (!minClippedResult.success()) {
        return minClippedResult.error();
    }
    std::optional<i32> minClipped = minClippedResult.value();

    if (type == "two_layers_feature_size") {
        auto limitR = getInt(sizeObj, "limit", true);
        if (!limitR.success()) {
            return limitR.error();
        }
        auto lowerR = getInt(sizeObj, "lower_size", true);
        if (!lowerR.success()) {
            return lowerR.error();
        }
        auto upperR = getInt(sizeObj, "upper_size", true);
        if (!upperR.success()) {
            return upperR.error();
        }
        return std::unique_ptr<FeatureSize>(
            std::make_unique<TwoLayersFeatureSize>(limitR.value(), lowerR.value(), upperR.value(), minClipped));
    }
    if (type == "three_layers_feature_size") {
        auto limitR = getInt(sizeObj, "limit", true);
        if (!limitR.success()) {
            return limitR.error();
        }
        auto upperLimitR = getInt(sizeObj, "upper_limit", true);
        if (!upperLimitR.success()) {
            return upperLimitR.error();
        }
        auto lowerR = getInt(sizeObj, "lower_size", true);
        if (!lowerR.success()) {
            return lowerR.error();
        }
        auto middleR = getInt(sizeObj, "middle_size", true);
        if (!middleR.success()) {
            return middleR.error();
        }
        auto upperR = getInt(sizeObj, "upper_size", true);
        if (!upperR.success()) {
            return upperR.error();
        }
        return std::unique_ptr<FeatureSize>(std::make_unique<ThreeLayersFeatureSize>(
            limitR.value(), upperLimitR.value(), lowerR.value(), middleR.value(), upperR.value(), minClipped));
    }

    spdlog::error("[FeatureSizeParser] unimplemented feature_size type: {}", type);
    return Error(ErrorCode::NotFound, std::string("unimplemented feature_size type: '") + type + "'");
}

} // namespace FeatureSizeParser
} // namespace parser
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
