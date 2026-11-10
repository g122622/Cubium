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
/// `cw` / `ccw` 是绕 Y 轴旋转（水平面）；`cwaxis` / `ccwaxis` 是绕任意轴旋转。
/// 方向位于旋转轴上时原版返回自身——这一点在各轴上都单独覆盖。
constexpr const char* UNITS[] = {
    // 水平顺时针：四个方向各一条
    "cw|north",
    "cw|east",
    "cw|south",
    "cw|west",
    // 水平逆时针
    "ccw|north",
    "ccw|east",
    "ccw|south",
    "ccw|west",
    // 按轴向旋转：方向位于该轴上时返回自身
    "cwaxis|east|x",
    "cwaxis|west|x",
    "cwaxis|up|y",
    "cwaxis|down|y",
    "cwaxis|north|z",
    "cwaxis|south|z",
    // 按轴向旋转：非法组合
    "cwaxis|up|x",
    "cwaxis|north|y",
    // 按轴向旋转：合法方向
    "cwaxis|up|z",
    "cwaxis|north|x",
    "cwaxis|east|z",
    "cwaxis|down|z",
    "ccwaxis|east|x",
    "ccwaxis|west|x",
    "ccwaxis|up|y",
    "ccwaxis|up|z",
    "ccwaxis|north|x",
    "ccwaxis|down|z",
    "ccwaxis|east|z",
    // toYRot：四个水平方向
    "toyrot|south",
    "toyrot|west",
    "toyrot|north",
    "toyrot|east",
    // toYRot：垂直方向（data2d = -1，故 -1 & 3 = 3 → 270）
    "toyrot|up",
    "toyrot|down",
    // fromYRot：四个基准角度
    "fromyrot|0",
    "fromyrot|90",
    "fromyrot|180",
    "fromyrot|270",
    // fromYRot：边界（+0.5 后恰好落整）
    "fromyrot|45",
    "fromyrot|44.9",
    "fromyrot|45.1",
    "fromyrot|-45",
    "fromyrot|-44.9",
    // fromYRot：超过一周
    "fromyrot|360",
    "fromyrot|-90",
    "fromyrot|720",
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
        throw std::runtime_error("direction_rotate: unknown direction: " + name);
    }
    return *parsed;
}

Axis parseAxis(const std::string& text)
{
    const auto parsed = Axes::fromName(text);
    if (!parsed.has_value()) {
        throw std::runtime_error("direction_rotate: unknown axis: " + text);
    }
    return *parsed;
}

const char* directionName(Direction dir)
{
    switch (dir) {
        case Direction::Down:
            return "down";
        case Direction::Up:
            return "up";
        case Direction::North:
            return "north";
        case Direction::South:
            return "south";
        case Direction::West:
            return "west";
        case Direction::East:
            return "east";
        default:
            return "unknown";
    }
}

std::string formatAngle(f32 value)
{
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.1f", static_cast<f64>(value));
    return buffer;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("direction_rotate: unit needs at least two fields: " + unit);
    }

    const std::string mode = parts[0];

    if (mode == "cw" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", directionName(Directions::getClockWise(parseDirection(parts[1])))}}
            .dump();
    }
    if (mode == "ccw" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", directionName(Directions::getCounterClockWise(parseDirection(parts[1])))}}
            .dump();
    }
    if ((mode == "cwaxis" || mode == "ccwaxis") && parts.size() == 3) {
        const Direction dir = parseDirection(parts[1]);
        const Axis axis = parseAxis(parts[2]);
        const Direction rotated =
            mode == "cwaxis" ? Directions::getClockWise(axis, dir) : Directions::getCounterClockWise(axis, dir);
        return json{{"unit", unit}, {"value", directionName(rotated)}}.dump();
    }
    if (mode == "toyrot" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", formatAngle(Directions::toYRot(parseDirection(parts[1])))}}.dump();
    }
    if (mode == "fromyrot" && parts.size() == 2) {
        const f64 yaw = std::stod(parts[1]);
        return json{{"unit", unit}, {"value", directionName(Directions::fromYRot(yaw))}}.dump();
    }

    throw std::runtime_error("direction_rotate: unrecognized unit: " + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(direction_rotate, &writeList, &runLine)

} // namespace mc::diff
