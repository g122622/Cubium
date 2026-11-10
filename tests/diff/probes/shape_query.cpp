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

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 单元表。
///
/// 每种形状都同时查询三轴，能覆盖「某轴 cube-like、另一轴不是」这类混合情形。
constexpr const char* UNITS[] = {
    // coords：坐标切分表
    "coords|0,0,0,1,1,1|x",
    "coords|0,0,0,1,1,1|y",
    "coords|0,0,0,1,1,1|z",
    "coords|0.25,0,0.25,0.75,0.5,0.75|x",
    "coords|0.25,0,0.25,0.75,0.5,0.75|y",
    "coords|0.25,0,0.25,0.75,0.5,0.75|z",
    "coords|-1,-2,-3,4,5,6|x",
    // iscube：整形状是否 cube-like
    "iscube|0,0,0,1,1,1|x",
    "iscube|0.25,0,0.25,0.75,0.5,0.75|x",
    "iscube|0,0,0,1,0.5,1|x",
    "iscube|0,0,0,0.5,1,1|z",
    "iscube|0,0,0,1,1,1|all",
    "iscube|0.25,0,0.25,0.75,0.5,0.75|all",
    "iscube|0,0,0,1,0.5,1|all",
    // findindex：坐标落在第几个区间
    "findindex|0,0,0,1,1,1|x|0.5",
    "findindex|0,0,0,1,1,1|x|0",
    "findindex|0,0,0,1,1,1|x|1",
    "findindex|0,0,0,1,1,1|x|-0.5",
    "findindex|0,0,0,1,1,1|x|1.5",
    "findindex|0.25,0,0.25,0.75,0.5,0.75|x|0",
    "findindex|0.25,0,0.25,0.75,0.5,0.75|x|0.25",
    "findindex|0.25,0,0.25,0.75,0.5,0.75|x|0.5",
    "findindex|0.25,0,0.25,0.75,0.5,0.75|x|0.75",
    "findindex|0.25,0,0.25,0.75,0.5,0.75|x|1",
    "findindex|0.25,0,0.25,0.75,0.5,0.75|y|0.5",
    // toaabbs：拆盒
    "toaabbs|0,0,0,1,1,1",
    "toaabbs|0.0625,0,0.0625,0.9375,0.09375,0.9375",
    "toaabbs|0.25,0,0.25,0.75,0.5,0.75",
    "toaabbs|-1,-2,-3,4,5,6",
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
        throw std::runtime_error("shape_query: expected " + std::to_string(expected) + " numbers: " + unit);
    }
    return values;
}

Axis parseAxis(const std::string& text)
{
    const auto parsed = Axes::fromName(text);
    if (!parsed.has_value()) {
        throw std::runtime_error("shape_query: unknown axis: " + text);
    }
    return *parsed;
}

std::string formatCoordinate(f64 value)
{
    char buffer[40];
    std::snprintf(buffer, sizeof(buffer), "%.6f", value);
    return buffer;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("shape_query: unit needs at least two fields: " + unit);
    }

    const std::string mode = parts[0];
    const auto box = parseNumbers(parts[1], 6, unit);
    const VoxelShape shape = Shapes::box(box[0], box[1], box[2], box[3], box[4], box[5]);

    if (mode == "coords" && parts.size() == 3) {
        // 坐标表按顺序拼接，既比数值也比顺序。
        const auto& coords = shape.getCoords(parseAxis(parts[2]));
        std::string joined;
        for (size_t index = 0; index < coords.size(); ++index) {
            if (index > 0) {
                joined.push_back(',');
            }
            joined += formatCoordinate(coords[index]);
        }
        return json{{"unit", unit}, {"value", joined}}.dump();
    }
    if (mode == "iscube" && parts.size() == 3) {
        if (parts[2] == "all") {
            return json{{"unit", unit}, {"value", shape.isCubeLike()}}.dump();
        }
        return json{{"unit", unit}, {"value", shape.isCubeLikeAlong(parseAxis(parts[2]))}}.dump();
    }
    if (mode == "findindex" && parts.size() == 4) {
        const Axis axis = parseAxis(parts[2]);
        const f64 coord = std::stod(parts[3]);
        return json{{"unit", unit}, {"value", shape.findIndex(axis, coord)}}.dump();
    }
    if (mode == "toaabbs" && parts.size() == 2) {
        const auto boxes = shape.toAabbs();
        std::string joined;
        for (size_t index = 0; index < boxes.size(); ++index) {
            if (index > 0) {
                joined.push_back(';');
            }
            joined += formatCoordinate(boxes[index].minX) + "," + formatCoordinate(boxes[index].minY) + "," +
                formatCoordinate(boxes[index].minZ) + "," + formatCoordinate(boxes[index].maxX) + "," +
                formatCoordinate(boxes[index].maxY) + "," + formatCoordinate(boxes[index].maxZ);
        }
        return json{{"unit", unit}, {"value", joined}}.dump();
    }

    throw std::runtime_error("shape_query: unrecognized unit: " + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(shape_query, &writeList, &runLine)

} // namespace mc::diff
