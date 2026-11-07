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
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 单元表。
///
/// `ceillong` 的重点在**恰好整数**与**负数**：
/// 整数不该被 +1；负数靠截断方向决定，与 libm 的 ceil 殊途同归。
constexpr const char* UNITS[] = {
    // ceillong：正数
    "ceillong|2.1",
    "ceillong|2.0",
    "ceillong|2.9",
    "ceillong|0.1",
    "ceillong|0.0",
    "ceillong|0.9",
    // ceillong：负数（截断方向关键）
    "ceillong|-2.1",
    "ceillong|-2.0",
    "ceillong|-2.9",
    "ceillong|-0.1",
    "ceillong|-0.0",
    "ceillong|-0.9",
    // ceillong：大值
    "ceillong|1000000.5",
    "ceillong|-1000000.5",
    "ceillong|1e15",
    // map：常规区间
    "map|0|0|10|0|100",
    "map|5|0|10|0|100",
    "map|10|0|10|0|100",
    // map：越界（不钳制）
    "map|-5|0|10|0|100",
    "map|15|0|10|0|100",
    // map：反向区间
    "map|5|0|10|100|0",
    "map|0|10|0|100|0",
    // map：负区间
    "map|-5|-10|10|0|100",
    "map|0|-10|10|-50|50",
    // map：退化区间（除零 -> inf 或 nan）
    "map|5|5|5|0|100",
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

/// 格式化浮点：9 位有效数字，去尾随零，处理 nan / inf。
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

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("mth_round_ext: unit needs at least two fields: " + unit);
    }

    const std::string mode = parts[0];

    if (mode == "ceillong" && parts.size() == 2) {
        const i64 result = math::ceilLong(std::stod(parts[1]));
        return json{{"unit", unit}, {"value", std::to_string(result)}}.dump();
    }
    if (mode == "map" && parts.size() == 6) {
        const f64 result = math::map(
            std::stod(parts[1]), std::stod(parts[2]), std::stod(parts[3]), std::stod(parts[4]), std::stod(parts[5]));
        return json{{"unit", unit}, {"value", formatDouble(result)}}.dump();
    }

    throw std::runtime_error("mth_round_ext: unrecognized unit: " + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(mth_round_ext, &writeList, &runLine)

} // namespace mc::diff
