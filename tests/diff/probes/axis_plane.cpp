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
/// 覆盖 `Direction.Axis` 上的全部轴语义，以及 `Directions.getAxis()` 的分组。
constexpr const char* UNITS[] = {
    // choosei：轴决定取第几个分量（三个分量互不相同，能区分任何取错轴）
    "choosei|x",
    "choosei|y",
    "choosei|z",
    // positive / negative：Z 轴正方向是 SOUTH
    "positive|x",
    "positive|y",
    "positive|z",
    "negative|x",
    "negative|y",
    "negative|z",
    // planeof：X/Z 水平，Y 垂直
    "planeof|x",
    "planeof|y",
    "planeof|z",
    // axisof：六个方向
    "axisof|down",
    "axisof|up",
    "axisof|north",
    "axisof|south",
    "axisof|west",
    "axisof|east",
    // test：水平面
    "test|horizontal|north",
    "test|horizontal|east",
    "test|horizontal|south",
    "test|horizontal|west",
    "test|horizontal|up",
    "test|horizontal|down",
    // test：垂直面
    "test|vertical|up",
    "test|vertical|down",
    "test|vertical|north",
    "test|vertical|east",
    // test：null（原版对 null 返回 false）
    "test|horizontal|null",
    "test|vertical|null",
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

Axis parseAxis(const std::string& text)
{
    const auto parsed = Axes::fromName(text);
    if (!parsed.has_value()) {
        throw std::runtime_error("axis_plane: unknown axis: " + text);
    }
    return *parsed;
}

Direction parseDirection(const std::string& name)
{
    const auto parsed = Directions::fromName(name);
    if (!parsed.has_value()) {
        throw std::runtime_error("axis_plane: unknown direction: " + name);
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

const char* planeName(Plane plane)
{
    return plane == Plane::Vertical ? "vertical" : "horizontal";
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("axis_plane: unit needs at least two fields: " + unit);
    }

    const std::string mode = parts[0];

    if (mode == "choosei" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", Axes::choose(parseAxis(parts[1]), 11, 22, 33)}}.dump();
    }
    if ((mode == "positive" || mode == "negative") && parts.size() == 2) {
        const Axis axis = parseAxis(parts[1]);
        const Direction dir = mode == "positive" ? Axes::getPositive(axis) : Axes::getNegative(axis);
        return json{{"unit", unit}, {"value", directionName(dir)}}.dump();
    }
    if (mode == "planeof" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", planeName(Axes::getPlane(parseAxis(parts[1])))}}.dump();
    }
    if (mode == "axisof" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", Axes::toString(Directions::getAxis(parseDirection(parts[1])))}}.dump();
    }
    if (mode == "test" && parts.size() == 3) {
        // 原版 Plane.test 接受可空方向，null 一律返回 false。
        const Plane plane = parts[1] == "vertical" ? Plane::Vertical : Plane::Horizontal;
        if (parts[2] == "null") {
            return json{{"unit", unit}, {"value", false}}.dump();
        }
        const Direction dir = parseDirection(parts[2]);
        return json{{"unit", unit}, {"value", Axes::getPlane(Directions::getAxis(dir)) == plane}}.dump();
    }

    throw std::runtime_error("axis_plane: unrecognized unit: " + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(axis_plane, &writeList, &runLine)

} // namespace mc::diff
