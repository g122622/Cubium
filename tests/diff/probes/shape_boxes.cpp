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
/// 自身盒子固定为 (0,0,0)-(1,1,1)。对每个轴，other 盒子取三种位置：
///   - 另外两轴不重叠（应原样放行）；
///   - 移动方向上已分离（应原样放行）；
///   - 真正相撞（应削减到贴面）。
constexpr const char* UNITS[] = {
    // boxes：形状拆盒
    "boxes|0,0,0,1,1,1",
    "boxes|0.0625,0,0.0625,0.9375,0.09375,0.9375",
    "boxes|0.25,0,0.25,0.75,0.5,0.75",
    "boxes|0.375,0.25,0.375,0.625,1,0.625",
    "boxes|-1,-2,-3,4,5,6",
    "boxes|0,0,0,0,0,0",
    // edges：形状拆棱（12 条棱的端点对）
    "edges|0,0,0,1,1,1",
    "edges|0.0625,0,0.0625,0.9375,0.09375,0.9375",
    "edges|0.25,0,0.25,0.75,0.5,0.75",
    "edges|0.375,0.25,0.375,0.625,1,0.625",
    "edges|-1,-2,-3,4,5,6",
    "edges|0,0,0,0,0,0",
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
        throw std::runtime_error("shape_boxes: expected " + std::to_string(expected) + " numbers: " + unit);
    }
    return values;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() != 2) {
        throw std::runtime_error("shape_boxes: unit needs two fields: " + unit);
    }
    const std::string mode = parts[0];
    const auto box = parseNumbers(parts[1], 6, unit);

    const VoxelShape shape = Shapes::box(box[0], box[1], box[2], box[3], box[4], box[5]);

    // 每个盒/棱按顺序输出 "a1,a2,a3,b1,b2,b3"，以分号连接。
    std::string joined;
    const auto collect = [&joined](f64 x1, f64 y1, f64 z1, f64 x2, f64 y2, f64 z2) {
        if (!joined.empty()) {
            joined.push_back(';');
        }
        char buffer[256];
        std::snprintf(buffer, sizeof(buffer), "%.6f,%.6f,%.6f,%.6f,%.6f,%.6f", x1, y1, z1, x2, y2, z2);
        joined += buffer;
    };
    if (mode == "boxes") {
        shape.forAllBoxes(collect);
    } else if (mode == "edges") {
        shape.forAllEdges(collect);
    } else {
        throw std::runtime_error("shape_boxes: unknown mode: " + unit);
    }
    const auto unused = [](f64 x1, f64 y1, f64 z1, f64 x2, f64 y2, f64 z2) {};
    MC_UNUSED(unused);

    return json{{"unit", unit}, {"value", joined}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(shape_boxes, &writeList, &runLine)

} // namespace mc::diff
