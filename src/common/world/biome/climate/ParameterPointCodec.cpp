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

#include "common/world/biome/climate/ParameterPointCodec.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/world/biome/climate/ParameterTypes.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc::world::biome::climate {

namespace {

using json = nlohmann::json;

/// 原版 Climate.Parameter.CODEC = ExtraCodecs.intervalCodec(floatRange(-2,2))，
/// 形态三选一：裸数字→point；[min,max]→span；{min,max}→span。
/// 本函数严格镜像该语义，错误信息带字段名方便定位。
Result<Parameter> parseParameter(const json& node, std::string_view field)
{
    // 裸数字 → point(value)
    if (node.is_number()) {
        const f32 v = node.get<f32>();
        return Parameter::point(v);
    }
    // [min, max] 数组 → span
    if (node.is_array()) {
        if (node.size() != 2 || !node[0].is_number() || !node[1].is_number()) {
            return Error(ErrorCode::InvalidData,
                "spawn_target[" + std::string(field) + "]: array form must be [min,max] numbers");
        }
        const f32 lo = node[0].get<f32>();
        const f32 hi = node[1].get<f32>();
        return Parameter::span(lo, hi);
    }
    // {min, max} 对象 → span
    if (node.is_object()) {
        if (!node.contains("min") || !node.contains("max") || !node["min"].is_number() || !node["max"].is_number()) {
            return Error(ErrorCode::InvalidData,
                "spawn_target[" + std::string(field) + "]: object form must be {min,max} numbers");
        }
        const f32 lo = node["min"].get<f32>();
        const f32 hi = node["max"].get<f32>();
        return Parameter::span(lo, hi);
    }
    return Error(
        ErrorCode::InvalidData, "spawn_target[" + std::string(field) + "]: must be number, [min,max], or {min,max}");
}

/// 读取必填气候参数字段（不存在/类型错都报错）
Result<Parameter> readParam(const json& obj, std::string_view field)
{
    if (!obj.contains(field)) {
        return Error(ErrorCode::InvalidData, "spawn_target: missing field '" + std::string(field) + "'");
    }
    return parseParameter(obj[field], field);
}

/// 读取必填 offset（裸 float，量化为 i64）
Result<i64> readOffset(const json& obj)
{
    if (!obj.contains("offset") || !obj["offset"].is_number()) {
        return Error(ErrorCode::InvalidData, "spawn_target: missing number field 'offset'");
    }
    return quantizeCoord(obj["offset"].get<f32>());
}

} // namespace

Result<ParameterPoint> ParameterPointCodec::fromJson(const json& element)
{
    if (!element.is_object()) {
        return Error(ErrorCode::InvalidData, "spawn_target element must be an object");
    }
    auto temperature = readParam(element, "temperature");
    if (temperature.failed()) {
        return temperature.error();
    }
    auto humidity = readParam(element, "humidity");
    if (humidity.failed()) {
        return humidity.error();
    }
    auto continentalness = readParam(element, "continentalness");
    if (continentalness.failed()) {
        return continentalness.error();
    }
    auto erosion = readParam(element, "erosion");
    if (erosion.failed()) {
        return erosion.error();
    }
    auto depth = readParam(element, "depth");
    if (depth.failed()) {
        return depth.error();
    }
    auto weirdness = readParam(element, "weirdness");
    if (weirdness.failed()) {
        return weirdness.error();
    }
    auto offset = readOffset(element);
    if (offset.failed()) {
        return offset.error();
    }
    return ParameterPoint{temperature.value(),
        humidity.value(),
        continentalness.value(),
        erosion.value(),
        depth.value(),
        weirdness.value(),
        offset.value()};
}

Result<std::vector<ParameterPoint>> ParameterPointCodec::fromJsonArray(const json& array)
{
    if (!array.is_array()) {
        return Error(ErrorCode::InvalidData, "spawn_target must be an array");
    }
    std::vector<ParameterPoint> result;
    result.reserve(array.size());
    for (const auto& element : array) {
        auto pp = fromJson(element);
        if (pp.failed()) {
            return pp.error();
        }
        result.push_back(pp.value());
    }
    return result;
}

} // namespace mc::world::biome::climate
