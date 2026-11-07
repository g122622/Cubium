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
#include "common/util/math/Vector3.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 单元表。
///
/// 角度刻意落在 90° 的整数倍（π/2 的倍数）上：查表在这些点返回精确的 0 与 ±1，
/// 而 libm 会给出 ±1e-8 的残差，这是最能区分两种实现的输入。
constexpr const char* UNITS[] = {
    // xrot：单位向量
    "xrot|0,1,0|0",
    "xrot|0,1,0|1.5707963267948966",
    "xrot|0,1,0|3.141592653589793",
    "xrot|0,1,0|-1.5707963267948966",
    "xrot|0,0,1|1.5707963267948966",
    "xrot|1,2,3|0.7853981633974483",
    "xrot|1,2,3|0",
    // yrot：单位向量
    "yrot|1,0,0|0",
    "yrot|1,0,0|1.5707963267948966",
    "yrot|1,0,0|3.141592653589793",
    "yrot|1,0,0|-1.5707963267948966",
    "yrot|0,0,1|1.5707963267948966",
    "yrot|1,2,3|0.7853981633974483",
    // zrot：单位向量
    "zrot|1,0,0|0",
    "zrot|1,0,0|1.5707963267948966",
    "zrot|1,0,0|3.141592653589793",
    "zrot|1,0,0|-1.5707963267948966",
    "zrot|0,1,0|1.5707963267948966",
    "zrot|1,2,3|0.7853981633974483",
    // projectedOn：投影
    "proj|1,2,3|1,0,0",
    "proj|1,2,3|0,1,0",
    "proj|1,2,3|1,1,1",
    "proj|1,2,3|0,0,0",
    "proj|0,0,0|1,2,3",
    "proj|-1,-2,-3|1,0,0",
    // horizontal：只保留 X/Z
    "horiz|1,2,3",
    "horiz|0,5,0",
    "horiz|-1,0,-3",
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

Vector3d parseVec(const std::string& text)
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
    if (values.size() != 3) {
        throw std::runtime_error("vec3_rotate: expected 3 components: " + text);
    }
    return Vector3d{values[0], values[1], values[2]};
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

json vecToJson(const Vector3d& vec)
{
    return json::array({formatDouble(vec.x), formatDouble(vec.y), formatDouble(vec.z)});
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("vec3_rotate: unit needs at least two fields: " + unit);
    }

    const std::string mode = parts[0];
    const Vector3d vec = parseVec(parts[1]);

    if ((mode == "xrot" || mode == "yrot" || mode == "zrot") && parts.size() == 3) {
        const f32 radians = static_cast<f32>(std::stod(parts[2]));
        Vector3d result{};
        if (mode == "xrot") {
            result = vec.xRot(radians);
        } else if (mode == "yrot") {
            result = vec.yRot(radians);
        } else {
            result = vec.zRot(radians);
        }
        return json{{"unit", unit}, {"result", vecToJson(result)}}.dump();
    }
    if (mode == "proj" && parts.size() == 3) {
        return json{{"unit", unit}, {"result", vecToJson(vec.projectedOn(parseVec(parts[2])))}}.dump();
    }
    if (mode == "horiz" && parts.size() == 2) {
        return json{{"unit", unit}, {"result", vecToJson(vec.horizontal())}}.dump();
    }

    throw std::runtime_error("vec3_rotate: unrecognized unit: " + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(vec3_rotate, &writeList, &runLine)

} // namespace mc::diff
