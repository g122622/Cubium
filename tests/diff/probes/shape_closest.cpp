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
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/physics/shape/Shapes.hpp"
#include "common/physics/shape/VoxelShape.hpp"
#include "common/util/math/Vector3.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 单元表。
///
/// 形状与点的组合刻意覆盖三类情形：
///   - 点在盒外（三轴都需投影 / 只在部分轴投影）；
///   - 点在盒内（应返回点自身）；
///   - 点在某个面上或角上（投影结果与原点重合）。
constexpr const char* UNITS[] = {
    // closestPointTo：满方块，点在外部三方向
    "closest|0,0,0,1,1,1|2,0.5,0.5",
    "closest|0,0,0,1,1,1|-1,0.5,0.5",
    "closest|0,0,0,1,1,1|0.5,2,0.5",
    "closest|0,0,0,1,1,1|0.5,0.5,-1",
    "closest|0,0,0,1,1,1|2,2,2",
    "closest|0,0,0,1,1,1|-1,-1,-1",
    // closestPointTo：点在内部（应返回自身）
    "closest|0,0,0,1,1,1|0.5,0.5,0.5",
    "closest|0,0,0,1,1,1|0.1,0.9,0.5",
    // closestPointTo：点在面上
    "closest|0,0,0,1,1,1|0,0.5,0.5",
    "closest|0,0,0,1,1,1|0.5,1,0.5",
    // closestPointTo：扁平形状（贴地）
    "closest|0.0625,0,0.0625,0.9375,0.09375,0.9375|0.5,0.5,0.5",
    "closest|0.0625,0,0.0625,0.9375,0.09375,0.9375|0.5,-1,0.5",
    // closestPointTo：细柱
    "closest|0.25,0,0.25,0.75,0.5,0.75|0,0.25,0.5",
    "closest|0.25,0,0.25,0.75,0.5,0.75|0.5,1,0.5",
    // singleEncompassing：各种形状的整平包裹
    "encap|0,0,0,1,1,1",
    "encap|0.0625,0,0.0625,0.9375,0.09375,0.9375",
    "encap|0.25,0,0.25,0.75,0.5,0.75",
    "encap|0.375,0.25,0.375,0.625,1,0.625",
    "encap|-1,-2,-3,4,5,6",
};

void writeList(FILE* out)
{
    for (const auto* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

std::vector<std::string> splitPipe(const std::string& text)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == '|') {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(current);
    return tokens;
}

/// 从 "a,b,c,..." 解析出 n 个 f64。
std::vector<f64> parseNumbers(const std::string& text, size_t expected, const std::string& unit)
{
    std::vector<f64> values;
    std::string current;
    for (size_t index = 0; index <= text.size(); ++index) {
        if (index == text.size() || text[index] == ',') {
            values.push_back(std::stod(current));
            current.clear();
        } else {
            current.push_back(text[index]);
        }
    }
    if (values.size() != expected) {
        throw std::runtime_error("shape_closest: expected " + std::to_string(expected) + " numbers: " + unit);
    }
    return values;
}

/// 把 6 个分量格式化成保留 6 位小数。
std::string joinNumbers(const std::vector<f64>& values)
{
    std::string joined;
    for (size_t index = 0; index < values.size(); ++index) {
        if (index > 0) {
            joined.push_back(',');
        }
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%.6f", values[index]);
        joined += buffer;
    }
    return joined;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("shape_closest: unit needs at least two fields: " + unit);
    }

    const std::string mode = parts[0];

    if (mode == "encap" && parts.size() == 2) {
        const auto box = parseNumbers(parts[1], 6, unit);
        const VoxelShape shape = Shapes::box(box[0], box[1], box[2], box[3], box[4], box[5]);
        const VoxelShape encap = shape.singleEncompassing();
        const AxisAlignedBB bounds = encap.bounds();
        return json{{"unit", unit},
            {"value", joinNumbers({bounds.minX, bounds.minY, bounds.minZ, bounds.maxX, bounds.maxY, bounds.maxZ})}}
            .dump();
    }
    if (mode == "closest" && parts.size() == 3) {
        const auto box = parseNumbers(parts[1], 6, unit);
        const auto point = parseNumbers(parts[2], 3, unit);
        const VoxelShape shape = Shapes::box(box[0], box[1], box[2], box[3], box[4], box[5]);
        const std::optional<Vector3> result = shape.closestPointTo(
            Vector3(static_cast<f32>(point[0]), static_cast<f32>(point[1]), static_cast<f32>(point[2])));
        if (!result.has_value()) {
            return json{{"unit", unit}, {"value", "empty"}}.dump();
        }
        return json{{"unit", unit},
            {"value",
                joinNumbers({static_cast<f64>(result->x), static_cast<f64>(result->y), static_cast<f64>(result->z)})}}
            .dump();
    }

    throw std::runtime_error("shape_closest: unrecognized unit: " + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(shape_closest, &writeList, &runLine)

} // namespace mc::diff
