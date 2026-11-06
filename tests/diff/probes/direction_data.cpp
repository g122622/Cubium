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
#include "common/util/Direction.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 单元表。
///
/// 六个方向都覆盖，垂直方向专测「二维编码未定义」这一分支。
constexpr const char* UNITS[] = {
    // 三维数据值
    "v3|DOWN",
    "v3|UP",
    "v3|NORTH",
    "v3|SOUTH",
    "v3|WEST",
    "v3|EAST",
    "v3|NONE",
    // 二维数据值：只有水平方向有定义
    "v2|DOWN",
    "v2|UP",
    "v2|NORTH",
    "v2|SOUTH",
    "v2|WEST",
    "v2|EAST",
    "v2|NONE",
    // 按轴取分量：x=11, y=22, z=33
    "choose|DOWN|11,22,33",
    "choose|UP|11,22,33",
    "choose|NORTH|11,22,33",
    "choose|SOUTH|11,22,33",
    "choose|WEST|11,22,33",
    "choose|EAST|11,22,33",
    // 按轴取分量：负值与零
    "choose|EAST|-7,0,0",
    "choose|UP|-7,0,0",
    "choose|NORTH|-7,0,0",
    // 单位向量（整数 / f32 / f64）
    "u3i|DOWN",
    "u3i|UP",
    "u3i|NORTH",
    "u3i|SOUTH",
    "u3i|WEST",
    "u3i|EAST",
    "u3f|DOWN",
    "u3f|NORTH",
    "u3f|WEST",
    "u3f|EAST",
    "u3|DOWN",
    "u3|UP",
    "u3|SOUTH",
    "u3|EAST",
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

/// 把方向名转成枚举。名称与单元表里的写法一致（大写）。
Direction parseDirection(const std::string& name)
{
    if (name == "DOWN") {
        return Direction::Down;
    }
    if (name == "UP") {
        return Direction::Up;
    }
    if (name == "NORTH") {
        return Direction::North;
    }
    if (name == "SOUTH") {
        return Direction::South;
    }
    if (name == "WEST") {
        return Direction::West;
    }
    if (name == "EAST") {
        return Direction::East;
    }
    if (name == "NONE") {
        return Direction::None;
    }
    throw std::runtime_error("direction_data: unknown direction: " + name);
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("direction_data: unit needs at least two fields: " + unit);
    }

    const std::string mode = parts[0];
    const Direction dir = parseDirection(parts[1]);

    if (mode == "v3") {
        return json{{"unit", unit}, {"value", Directions::get3DDataValue(dir)}}.dump();
    }
    if (mode == "v2") {
        return json{{"unit", unit}, {"value", Directions::get2DDataValue(dir)}}.dump();
    }
    if (mode == "u3i") {
        const Vector3i v = Directions::getUnitVec3i(dir);
        return json{
            {"unit", unit}, {"value", std::to_string(v.x) + "," + std::to_string(v.y) + "," + std::to_string(v.z)}}
            .dump();
    }
    if (mode == "u3f") {
        const Vector3f v = Directions::getUnitVec3f(dir);
        char buffer[128];
        std::snprintf(buffer,
            sizeof(buffer),
            "%.1f,%.1f,%.1f",
            static_cast<double>(v.x),
            static_cast<double>(v.y),
            static_cast<double>(v.z));
        return json{{"unit", unit}, {"value", std::string(buffer)}}.dump();
    }
    if (mode == "u3") {
        const Vector3d v = Directions::getUnitVec3(dir);
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "%.1f,%.1f,%.1f", v.x, v.y, v.z);
        return json{{"unit", unit}, {"value", std::string(buffer)}}.dump();
    }
    if (mode == "choose" && parts.size() == 3) {
        const std::string& coords = parts[2];
        const size_t c1 = coords.find(',');
        const size_t c2 = coords.find(',', c1 + 1);
        if (c1 == std::string::npos || c2 == std::string::npos) {
            throw std::runtime_error("direction_data: choose needs x,y,z: " + unit);
        }
        const i32 x = std::stoi(coords.substr(0, c1));
        const i32 y = std::stoi(coords.substr(c1 + 1, c2 - c1 - 1));
        const i32 z = std::stoi(coords.substr(c2 + 1));
        return json{{"unit", unit}, {"value", Directions::choose(dir, x, y, z)}}.dump();
    }

    throw std::runtime_error("direction_data: unrecognized unit: " + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(direction_data, &writeList, &runLine)

} // namespace mc::diff
