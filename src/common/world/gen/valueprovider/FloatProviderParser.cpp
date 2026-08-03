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

#include "FloatProviderParser.hpp"

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/gen/valueprovider/FloatProvider.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace world {
namespace gen {
namespace valueprovider {
namespace FloatProviderParser {

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
 * @brief 读取浮点字段（不存在或非数字时返回 defaultValue）
 */
f32 readF32(const nlohmann::json& obj, const std::string& key, f32 defaultValue)
{
    if (obj.contains(key) && obj[key].is_number()) {
        return obj[key].get<f32>();
    }
    return defaultValue;
}

} // namespace

Result<std::unique_ptr<FloatProvider>> parse(const nlohmann::json& json)
{
    // 裸数字简写：0.5 -> ConstantFloat(0.5)
    if (json.is_number()) {
        std::unique_ptr<FloatProvider> provider = ConstantFloat::create(json.get<f32>());
        return provider;
    }

    if (!json.is_object() || !json.contains("type") || !json["type"].is_string()) {
        return Error(ErrorCode::InvalidData, "FloatProvider must be a number or an object with 'type'");
    }

    const std::string type = stripNamespace(json["type"].get<std::string>());

    // 同时支持嵌套 "value" 对象与扁平字段（MC 数据包两种写法都存在）
    const nlohmann::json& valueObj = (json.contains("value") && json["value"].is_object()) ? json["value"] : json;

    if (type == "constant") {
        if (valueObj.contains("value") && valueObj["value"].is_number()) {
            std::unique_ptr<FloatProvider> provider = ConstantFloat::create(valueObj["value"].get<f32>());
            return provider;
        }
        if (json.contains("value") && json["value"].is_number()) {
            std::unique_ptr<FloatProvider> provider = ConstantFloat::create(json["value"].get<f32>());
            return provider;
        }
        return Error(ErrorCode::InvalidData, "constant FloatProvider missing 'value'");
    }

    if (type == "uniform") {
        const f32 minInclusive = readF32(valueObj, "min_inclusive", 0.0f);
        const f32 maxExclusive = readF32(valueObj, "max_exclusive", 0.0f);
        std::unique_ptr<FloatProvider> provider = UniformFloat::create(minInclusive, maxExclusive);
        return provider;
    }

    if (type == "trapezoid") {
        const f32 minInclusive = readF32(valueObj, "min", 0.0f);
        const f32 maxInclusive = readF32(valueObj, "max", 0.0f);
        const f32 plateau = readF32(valueObj, "plateau", 0.0f);
        std::unique_ptr<FloatProvider> provider = TrapezoidFloat::create(minInclusive, maxInclusive, plateau);
        return provider;
    }

    if (type == "clamped_normal") {
        const f32 mean = readF32(valueObj, "mean", 0.0f);
        const f32 deviation = readF32(valueObj, "deviation", 0.0f);
        const f32 min = readF32(valueObj, "min", 0.0f);
        const f32 max = readF32(valueObj, "max", 0.0f);
        std::unique_ptr<FloatProvider> provider = ClampedNormalFloat::create(mean, deviation, min, max);
        return provider;
    }

    return Error(ErrorCode::InvalidData, "Unknown FloatProvider type: " + type);
}

} // namespace FloatProviderParser
} // namespace valueprovider
} // namespace gen
} // namespace world
} // namespace mc
