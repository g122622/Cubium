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

#include "IntProviderParser.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/gen/valueprovider/IntProvider.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <fmt/format.h>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace world {
namespace gen {
namespace valueprovider {

namespace {

/**
 * @brief 去除 minecraft: 命名空间前缀
 */
std::string _stripNamespace(const std::string& type)
{
    constexpr std::string_view prefix = "minecraft:";
    if (type.size() > prefix.size() && type.substr(0, prefix.size()) == prefix) {
        return type.substr(prefix.size());
    }
    return type;
}

/**
 * @brief 解析 constant 类型 IntProvider
 *
 * 格式: { "value": N }
 */
Result<std::unique_ptr<IntProvider>> _parseConstant(const nlohmann::json& valueObj)
{
    if (!valueObj.contains("value") || !valueObj["value"].is_number_integer()) {
        return Error(ErrorCode::InvalidData, "IntProvider constant: missing or invalid 'value' field");
    }
    i32 value = valueObj["value"].get<i32>();
    return Result<std::unique_ptr<IntProvider>>(std::make_unique<ConstantInt>(value));
}

/**
 * @brief 解析 uniform 类型 IntProvider
 *
 * 格式: { "min_inclusive": N, "max_inclusive": M }
 */
Result<std::unique_ptr<IntProvider>> _parseUniform(const nlohmann::json& valueObj)
{
    if (!valueObj.contains("min_inclusive") || !valueObj["min_inclusive"].is_number_integer()) {
        return Error(ErrorCode::InvalidData, "IntProvider uniform: missing or invalid 'min_inclusive' field");
    }
    if (!valueObj.contains("max_inclusive") || !valueObj["max_inclusive"].is_number_integer()) {
        return Error(ErrorCode::InvalidData, "IntProvider uniform: missing or invalid 'max_inclusive' field");
    }
    i32 minInclusive = valueObj["min_inclusive"].get<i32>();
    i32 maxInclusive = valueObj["max_inclusive"].get<i32>();
    if (minInclusive > maxInclusive) {
        return Error(ErrorCode::InvalidData,
            fmt::format("IntProvider uniform: min_inclusive({}) > max_inclusive({})", minInclusive, maxInclusive));
    }
    return Result<std::unique_ptr<IntProvider>>(std::make_unique<UniformInt>(minInclusive, maxInclusive));
}

/**
 * @brief 解析 biased_to_bottom 类型 IntProvider
 *
 * 格式: { "min_inclusive": N, "max_inclusive": M }
 */
Result<std::unique_ptr<IntProvider>> _parseBiasedToBottom(const nlohmann::json& valueObj)
{
    if (!valueObj.contains("min_inclusive") || !valueObj["min_inclusive"].is_number_integer()) {
        return Error(ErrorCode::InvalidData, "IntProvider biased_to_bottom: missing or invalid 'min_inclusive' field");
    }
    if (!valueObj.contains("max_inclusive") || !valueObj["max_inclusive"].is_number_integer()) {
        return Error(ErrorCode::InvalidData, "IntProvider biased_to_bottom: missing or invalid 'max_inclusive' field");
    }
    i32 minInclusive = valueObj["min_inclusive"].get<i32>();
    i32 maxInclusive = valueObj["max_inclusive"].get<i32>();
    if (minInclusive > maxInclusive) {
        return Error(ErrorCode::InvalidData,
            fmt::format(
                "IntProvider biased_to_bottom: min_inclusive({}) > max_inclusive({})", minInclusive, maxInclusive));
    }
    return Result<std::unique_ptr<IntProvider>>(std::make_unique<BiasedToBottomInt>(minInclusive, maxInclusive));
}

/**
 * @brief 解析 clamped 类型 IntProvider
 *
 * 格式: { "source": <IntProvider>, "min_inclusive": N, "max_inclusive": M }
 */
Result<std::unique_ptr<IntProvider>> _parseClamped(const nlohmann::json& valueObj)
{
    if (!valueObj.contains("source")) {
        return Error(ErrorCode::InvalidData, "IntProvider clamped: missing 'source' field");
    }
    if (!valueObj.contains("min_inclusive") || !valueObj["min_inclusive"].is_number_integer()) {
        return Error(ErrorCode::InvalidData, "IntProvider clamped: missing or invalid 'min_inclusive' field");
    }
    if (!valueObj.contains("max_inclusive") || !valueObj["max_inclusive"].is_number_integer()) {
        return Error(ErrorCode::InvalidData, "IntProvider clamped: missing or invalid 'max_inclusive' field");
    }

    auto sourceResult = IntProviderParser::parse(valueObj["source"]);
    if (!sourceResult.success()) {
        return sourceResult.error();
    }

    i32 minInclusive = valueObj["min_inclusive"].get<i32>();
    i32 maxInclusive = valueObj["max_inclusive"].get<i32>();
    if (minInclusive > maxInclusive) {
        return Error(ErrorCode::InvalidData,
            fmt::format("IntProvider clamped: min_inclusive({}) > max_inclusive({})", minInclusive, maxInclusive));
    }

    return Result<std::unique_ptr<IntProvider>>(
        std::make_unique<ClampedInt>(sourceResult.value(), minInclusive, maxInclusive));
}

/**
 * @brief 解析 clamped_normal 类型 IntProvider
 *
 * 格式: { "mean": D, "deviation": D, "min_inclusive": N, "max_inclusive": M }
 */
Result<std::unique_ptr<IntProvider>> _parseClampedNormal(const nlohmann::json& valueObj)
{
    if (!valueObj.contains("mean") || !valueObj["mean"].is_number()) {
        return Error(ErrorCode::InvalidData, "IntProvider clamped_normal: missing or invalid 'mean' field");
    }
    if (!valueObj.contains("deviation") || !valueObj["deviation"].is_number()) {
        return Error(ErrorCode::InvalidData, "IntProvider clamped_normal: missing or invalid 'deviation' field");
    }
    if (!valueObj.contains("min_inclusive") || !valueObj["min_inclusive"].is_number_integer()) {
        return Error(ErrorCode::InvalidData, "IntProvider clamped_normal: missing or invalid 'min_inclusive' field");
    }
    if (!valueObj.contains("max_inclusive") || !valueObj["max_inclusive"].is_number_integer()) {
        return Error(ErrorCode::InvalidData, "IntProvider clamped_normal: missing or invalid 'max_inclusive' field");
    }

    f64 mean = valueObj["mean"].get<f64>();
    f64 deviation = valueObj["deviation"].get<f64>();
    i32 minInclusive = valueObj["min_inclusive"].get<i32>();
    i32 maxInclusive = valueObj["max_inclusive"].get<i32>();
    if (minInclusive > maxInclusive) {
        return Error(ErrorCode::InvalidData,
            fmt::format(
                "IntProvider clamped_normal: min_inclusive({}) > max_inclusive({})", minInclusive, maxInclusive));
    }

    return Result<std::unique_ptr<IntProvider>>(
        std::make_unique<ClampedNormalInt>(mean, deviation, minInclusive, maxInclusive));
}

/**
 * @brief 解析 weighted_list 类型 IntProvider
 *
 * 格式: { "distribution": [{"data": <IntProvider>, "weight": N}, ...] }
 */
Result<std::unique_ptr<IntProvider>> _parseWeightedList(const nlohmann::json& valueObj)
{
    if (!valueObj.contains("distribution") || !valueObj["distribution"].is_array()) {
        return Error(ErrorCode::InvalidData, "IntProvider weighted_list: missing or invalid 'distribution' field");
    }

    const auto& dist = valueObj["distribution"];
    if (dist.empty()) {
        return Error(ErrorCode::InvalidData, "IntProvider weighted_list: distribution must not be empty");
    }

    std::vector<WeightedListInt::WeightedEntry> entries;
    entries.reserve(dist.size());

    for (size_t i = 0; i < dist.size(); ++i) {
        const auto& entry = dist[i];

        if (!entry.contains("data")) {
            return Error(
                ErrorCode::InvalidData, fmt::format("IntProvider weighted_list: entry[{}] missing 'data' field", i));
        }
        if (!entry.contains("weight") || !entry["weight"].is_number_integer()) {
            return Error(ErrorCode::InvalidData,
                fmt::format("IntProvider weighted_list: entry[{}] missing or invalid 'weight' field", i));
        }

        auto dataResult = IntProviderParser::parse(entry["data"]);
        if (!dataResult.success()) {
            return dataResult.error();
        }

        i32 weight = entry["weight"].get<i32>();
        if (weight < 0) {
            return Error(ErrorCode::InvalidData,
                fmt::format("IntProvider weighted_list: entry[{}] has negative weight {}", i, weight));
        }

        WeightedListInt::WeightedEntry weightedEntry;
        weightedEntry.provider = dataResult.value();
        weightedEntry.weight = weight;
        entries.push_back(std::move(weightedEntry));
    }

    return Result<std::unique_ptr<IntProvider>>(std::make_unique<WeightedListInt>(std::move(entries)));
}

} // namespace

namespace IntProviderParser {

Result<std::unique_ptr<IntProvider>> parse(
    const nlohmann::json& json, std::optional<i32> minInclusive, std::optional<i32> maxInclusive)
{
    // 裸整数简写: 5 -> ConstantInt(5)
    if (json.is_number_integer()) {
        i32 value = json.get<i32>();
        auto provider = std::make_unique<ConstantInt>(value);
        // 校验范围
        if (minInclusive && provider->getMinValue() < *minInclusive) {
            return Error(ErrorCode::InvalidData,
                fmt::format("IntProvider constant value {} is less than minimum {}", value, *minInclusive));
        }
        if (maxInclusive && provider->getMaxValue() > *maxInclusive) {
            return Error(ErrorCode::InvalidData,
                fmt::format("IntProvider constant value {} exceeds maximum {}", value, *maxInclusive));
        }
        return Result<std::unique_ptr<IntProvider>>(std::move(provider));
    }

    if (!json.is_object()) {
        return Error(ErrorCode::InvalidData, "IntProvider must be an integer or a JSON object");
    }

    // 提取类型字段
    std::string type;
    if (json.contains("type") && json["type"].is_string()) {
        type = _stripNamespace(json["type"].get<std::string>());
    } else {
        return Error(ErrorCode::InvalidData, "IntProvider object missing 'type' field");
    }

    // 获取 value 子对象（MC 1.21 的 dispatch 格式将参数放在 "value" 下）
    // 对于 constant，value 字段也可以直接在顶层（兼容旧格式）
    nlohmann::json valueObj;
    if (json.contains("value") && json["value"].is_object()) {
        valueObj = json["value"];
    } else {
        // 没有嵌套的 value 对象时，参数直接在顶层
        valueObj = json;
    }

    Result<std::unique_ptr<IntProvider>> result(Error(ErrorCode::InvalidData, "unreachable"));

    if (type == "constant") {
        result = _parseConstant(valueObj);
    } else if (type == "uniform") {
        result = _parseUniform(valueObj);
    } else if (type == "biased_to_bottom") {
        result = _parseBiasedToBottom(valueObj);
    } else if (type == "clamped") {
        result = _parseClamped(valueObj);
    } else if (type == "clamped_normal") {
        result = _parseClampedNormal(valueObj);
    } else if (type == "weighted_list") {
        result = _parseWeightedList(valueObj);
    } else {
        return Error(ErrorCode::InvalidData, fmt::format("Unknown IntProvider type: {}", type));
    }

    if (!result.success()) {
        return result;
    }

    // 校验范围约束
    auto provider = result.value();
    if (minInclusive && provider->getMinValue() < *minInclusive) {
        return Error(ErrorCode::InvalidData,
            fmt::format("IntProvider {} minValue {} is less than required minimum {}",
                type,
                provider->getMinValue(),
                *minInclusive));
    }
    if (maxInclusive && provider->getMaxValue() > *maxInclusive) {
        return Error(ErrorCode::InvalidData,
            fmt::format("IntProvider {} maxValue {} exceeds required maximum {}",
                type,
                provider->getMaxValue(),
                *maxInclusive));
    }

    return Result<std::unique_ptr<IntProvider>>(std::move(provider));
}

} // namespace IntProviderParser

} // namespace valueprovider
} // namespace gen
} // namespace world
} // namespace mc
