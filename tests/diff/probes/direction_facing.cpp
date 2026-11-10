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
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 单元表。
///
/// `isFacingAngle` 的角度刻意覆盖 45° 的整数倍及其邻域：
/// 45° 是相邻两个方向点积相等的分界，查表误差最容易在这里翻转结果。
constexpr const char* UNITS[] = {
    // axisdir：六个方向在各自轴上的正负
    "axisdir|down",
    "axisdir|up",
    "axisdir|north",
    "axisdir|south",
    "axisdir|west",
    "axisdir|east",
    // facing：四个水平方向在基准角度下的判定
    "facing|north|0",
    "facing|south|0",
    "facing|west|0",
    "facing|east|0",
    "facing|north|90",
    "facing|south|90",
    "facing|west|90",
    "facing|east|90",
    "facing|north|180",
    "facing|south|180",
    "facing|west|180",
    "facing|east|180",
    "facing|north|270",
    "facing|south|270",
    "facing|west|270",
    "facing|east|270",
    // facing：45° 分界邻域（点积为 0 附近，最容易翻转）
    "facing|north|45",
    "facing|north|44.9",
    "facing|north|45.1",
    "facing|east|45",
    "facing|east|44.9",
    "facing|east|45.1",
    "facing|south|45",
    "facing|west|45",
    "facing|north|135",
    "facing|east|135",
    "facing|north|225",
    "facing|east|225",
    "facing|north|315",
    "facing|east|315",
    // facing：负角度与超一周
    "facing|north|-45",
    "facing|east|-45",
    "facing|north|360",
    "facing|east|-90",
    "facing|north|720",
    // facing：垂直方向（法向量在 XZ 上为 0，恒为 false）
    "facing|up|0",
    "facing|down|0",
    "facing|up|90",
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

Direction parseDirection(const std::string& name)
{
    const auto parsed = Directions::fromName(name);
    if (!parsed.has_value()) {
        throw std::runtime_error("direction_facing: unknown direction: " + name);
    }
    return *parsed;
}

/// 与原版 `AxisDirection.getName()` 一致的字符串。
///
/// 原版返回的是带前缀的完整名称（"Towards positive" / "Towards negative"），
/// 不是枚举名的小写形式。
const char* axisDirectionName(AxisDirection dir)
{
    return dir == AxisDirection::Positive ? "Towards positive" : "Towards negative";
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("direction_facing: unit needs at least two fields: " + unit);
    }

    const std::string mode = parts[0];

    if (mode == "axisdir" && parts.size() == 2) {
        return json{
            {"unit", unit}, {"value", axisDirectionName(Directions::getAxisDirection(parseDirection(parts[1])))}}
            .dump();
    }
    if (mode == "facing" && parts.size() == 3) {
        const Direction dir = parseDirection(parts[1]);
        const f32 yaw = static_cast<f32>(std::stod(parts[2]));
        return json{{"unit", unit}, {"value", Directions::isFacingAngle(dir, yaw)}}.dump();
    }

    throw std::runtime_error("direction_facing: unrecognized unit: " + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(direction_facing, &writeList, &runLine)

} // namespace mc::diff
