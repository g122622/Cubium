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
#include "common/util/Direction.hpp"
#include "common/world/block/base/types/BlockPos.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 单元表。
///
/// `rotate` 覆盖四个旋转值，含负坐标与 Y 轴上的点（Y 不受旋转影响）。
/// `multiply` 覆盖 0 / 1 / 正 / 负四个因子。
constexpr const char* UNITS[] = {
    "rotate|NONE|5,3,7",
    "rotate|CLOCKWISE_90|5,3,7",
    "rotate|CLOCKWISE_180|5,3,7",
    "rotate|COUNTERCLOCKWISE_90|5,3,7",
    "rotate|CLOCKWISE_90|-5,3,7",
    "rotate|CLOCKWISE_90|5,3,-7",
    "rotate|CLOCKWISE_90|0,0,0",
    "rotate|COUNTERCLOCKWISE_90|1,2,3",
    "rotate|CLOCKWISE_90|0,99,0",
    "rotate|CLOCKWISE_180|0,-99,0",
    "cross|1,0,0|0,1,0",
    "cross|1,0,0|0,0,1",
    "cross|0,1,0|0,0,1",
    "cross|1,2,3|1,2,3",
    "cross|2,3,4|5,6,7",
    "cross|0,0,0|1,2,3",
    "min|5,3,7|2,8,4",
    "max|5,3,7|2,8,4",
    "min|1,1,1|1,1,1",
    "max|-5,-3,-7|-2,-8,-4",
    "multiply|5,3,7|0",
    "multiply|5,3,7|1",
    "multiply|5,3,7|2",
    "multiply|5,3,7|-1",
    "multiply|-5,3,-7|-3",
    "subtract|5,3,7|2,8,4",
    "subtract|5,3,7|5,3,7",
    "subtract|0,0,0|1,2,3",
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

BlockPos parsePos(const std::string& text)
{
    std::string current;
    std::vector<i32> values;
    for (size_t index = 0; index <= text.size(); ++index) {
        if (index == text.size() || text[index] == ',') {
            values.push_back(std::stoi(current));
            current.clear();
        } else {
            current.push_back(text[index]);
        }
    }
    if (values.size() != 3) {
        throw std::runtime_error("blockpos_geom: expected 3 coordinates: " + text);
    }
    return BlockPos{values[0], values[1], values[2]};
}

Rotation parseRotation(const std::string& text)
{
    if (text == "NONE") {
        return Rotation::None;
    }
    if (text == "CLOCKWISE_90") {
        return Rotation::Clockwise90;
    }
    if (text == "CLOCKWISE_180") {
        return Rotation::Clockwise180;
    }
    if (text == "COUNTERCLOCKWISE_90") {
        return Rotation::CounterClockwise90;
    }
    throw std::runtime_error("blockpos_geom: unknown rotation: " + text);
}

std::string dumpPos(const BlockPos& pos)
{
    return std::to_string(pos.x) + "," + std::to_string(pos.y) + "," + std::to_string(pos.z);
}

std::string formatDouble(f64 value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.6f", value);
    return buffer;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("blockpos_geom: unit needs at least two fields: " + unit);
    }

    const std::string mode = parts[0];

    if (mode == "rotate" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", dumpPos(rotate(parsePos(parts[2]), parseRotation(parts[1])))}}.dump();
    }
    if (mode == "cross" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", dumpPos(cross(parsePos(parts[1]), parsePos(parts[2])))}}.dump();
    }
    if (mode == "min" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", dumpPos(min(parsePos(parts[1]), parsePos(parts[2])))}}.dump();
    }
    if (mode == "max" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", dumpPos(max(parsePos(parts[1]), parsePos(parts[2])))}}.dump();
    }
    if (mode == "multiply" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", dumpPos(multiply(parsePos(parts[1]), std::stoi(parts[2])))}}.dump();
    }
    if (mode == "subtract" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", dumpPos(subtract(parsePos(parts[1]), parsePos(parts[2])))}}.dump();
    }
    if (mode == "containing" && parts.size() == 2) {
        // 原版 BlockPos.containing(double, double, double) 对每个分量向下取整。
        const std::string& text = parts[1];
        std::string current;
        std::vector<f64> values;
        for (size_t index = 0; index <= text.size(); ++index) {
            if (index == text.size() || text[index] == ',') {
                values.push_back(std::stod(current));
                current.clear();
            } else {
                current.push_back(text[index]);
            }
        }
        if (values.size() != 3) {
            throw std::runtime_error("blockpos_geom: expected 3 values: " + text);
        }
        return json{{"unit", unit}, {"value", dumpPos(containing(values[0], values[1], values[2]))}}.dump();
    }

    throw std::runtime_error("blockpos_geom: unrecognized unit: " + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(blockpos_geom, &writeList, &runLine)

} // namespace mc::diff
