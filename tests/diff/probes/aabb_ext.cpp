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
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 单元表。
///
/// 盒子刻意选 Y 方向有厚度的（y1 到 y2 不相等），
/// 这样才能区分 `getCenter`（Y 取中点）与 `getBottomCenter`（Y 取 minY）。
constexpr const char* UNITS[] = {
    // center / bottomcenter / min / max：标准盒子
    "center|0,0,0,1,1,1",
    "bottomcenter|0,0,0,1,1,1",
    "minpos|0,0,0,1,1,1",
    "maxpos|0,0,0,1,1,1",
    // 非对称盒子（全是奇数，中点带 .5）
    "center|1,2,3,5,8,11",
    "bottomcenter|1,2,3,5,8,11",
    "minpos|1,2,3,5,8,11",
    "maxpos|1,2,3,5,8,11",
    // 负数盒子
    "center|-5,-8,-11,-1,-2,-3",
    "bottomcenter|-5,-8,-11,-1,-2,-3",
    "minpos|-5,-8,-11,-1,-2,-3",
    "maxpos|-5,-8,-11,-1,-2,-3",
    // 退化盒子（零尺寸）
    "center|2,2,2,2,2,2",
    "bottomcenter|2,2,2,2,2,2",
    // 扁平盒子（Y 零厚度）
    "center|0,5,0,1,5,1",
    "bottomcenter|0,5,0,1,5,1",
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
        throw std::runtime_error("aabb_ext: expected " + std::to_string(expected) + " numbers: " + unit);
    }
    return values;
}

std::string formatDouble(f64 value)
{
    if (std::isnan(value)) {
        return "NaN";
    }
    if (std::isinf(value)) {
        return value > 0 ? "Infinity" : "-Infinity";
    }
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.9g", value);
    return buffer;
}

std::string formatVector(const Vector3d& vec)
{
    return formatDouble(vec.x) + "," + formatDouble(vec.y) + "," + formatDouble(vec.z);
}

std::string formatBox(const AxisAlignedBB& box)
{
    return formatDouble(box.minX) + "," + formatDouble(box.minY) + "," + formatDouble(box.minZ) + "," +
        formatDouble(box.maxX) + "," + formatDouble(box.maxY) + "," + formatDouble(box.maxZ);
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("aabb_ext: unit needs at least two fields: " + unit);
    }

    const std::string mode = parts[0];
    const auto boxValues = parseNumbers(parts[1], 6, unit);
    AxisAlignedBB box(boxValues[0], boxValues[1], boxValues[2], boxValues[3], boxValues[4], boxValues[5]);

    if (mode == "center" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", formatVector(box.getCenter())}}.dump();
    }
    if (mode == "bottomcenter" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", formatVector(box.getBottomCenter())}}.dump();
    }
    if (mode == "minpos" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", formatVector(box.getMinPosition())}}.dump();
    }
    if (mode == "maxpos" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", formatVector(box.getMaxPosition())}}.dump();
    }

    throw std::runtime_error("aabb_ext: unrecognized unit: " + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(aabb_ext, &writeList, &runLine)

} // namespace mc::diff
