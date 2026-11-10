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
#include "common/world/block/base/types/BlockPos.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 单元表。
///
/// 本能力验证 BlockPos 的三个基础变换的**语义正确性**（而非浮点精度）：
///   - `bottomCenter`：Y 不偏移，X/Z 各加 0.5；
///   - `multiply`：各分量按标量缩放（含 0、负数）；
///   - `subtract`：逐分量相减（含负坐标）。
constexpr const char* UNITS[] = {
    "bottom|0|0|0",
    "bottom|1|64|-1",
    "bottom|-100|-64|100",
    "bottom|1000|100|1000",
    "multiply|1|2|3|4",
    "multiply|1|2|3|-1",
    "multiply|1|2|3|0",
    "multiply|-5|0|5|10000",
    "multiply|100000|100000|100000|2",
    "subtract|10|20|30|1|2|3",
    "subtract|0|0|0|0|0|0",
    "subtract|-5|-10|-15|5|10|15",
    "subtract|1000|64|-1000|1000|64|-1000",
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

/// 把 f32 三元组格式化成 "x,y,z"，保留 6 位小数。
///
/// 不比对位模式：浮点末位差异不在本项目的对齐范围内。
std::string joinVec(f32 x, f32 y, f32 z)
{
    char buffer[128];
    std::snprintf(buffer,
        sizeof(buffer),
        "%.6f,%.6f,%.6f",
        static_cast<double>(x),
        static_cast<double>(y),
        static_cast<double>(z));
    return std::string(buffer);
}

std::string joinPos(i32 x, i32 y, i32 z)
{
    return std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z);
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("blockpos_center: unit needs at least two fields: " + unit);
    }

    const std::string mode = parts[0];

    if (mode == "bottom" && parts.size() == 4) {
        const BlockPos pos(std::stoi(parts[1]), std::stoi(parts[2]), std::stoi(parts[3]));
        const Vector3 c = pos.bottomCenter();
        return json{{"unit", unit}, {"value", joinVec(c.x, c.y, c.z)}}.dump();
    }
    if (mode == "multiply" && parts.size() == 5) {
        const BlockPos pos(std::stoi(parts[1]), std::stoi(parts[2]), std::stoi(parts[3]));
        const BlockPos result = pos.multiply(std::stoi(parts[4]));
        return json{{"unit", unit}, {"value", joinPos(result.x, result.y, result.z)}}.dump();
    }
    if (mode == "subtract" && parts.size() == 7) {
        const BlockPos a(std::stoi(parts[1]), std::stoi(parts[2]), std::stoi(parts[3]));
        const BlockPos b(std::stoi(parts[4]), std::stoi(parts[5]), std::stoi(parts[6]));
        const BlockPos result = a.subtract(b);
        return json{{"unit", unit}, {"value", joinPos(result.x, result.y, result.z)}}.dump();
    }

    throw std::runtime_error("blockpos_center: unrecognized unit: " + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(blockpos_center, &writeList, &runLine)

} // namespace mc::diff
