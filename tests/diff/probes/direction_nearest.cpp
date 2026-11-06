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
/// `approx` 重点覆盖零向量与极小分量——原版初值是 `Float.MIN_VALUE`（1.175e-38），
/// 点积为 0 时不满足 `> 初值`，因此保持 NORTH；初值若取 -1 会错误地变成 DOWN。
///
/// `nearest` 重点覆盖并列情形——原版用**严格大于**判定，任意两轴绝对值相等
/// 都落到 fallback。
constexpr const char* UNITS[] = {
    // approx：六个轴向
    "approxF|1|0|0",
    "approxF|-1|0|0",
    "approxF|0|1|0",
    "approxF|0|-1|0",
    "approxF|0|0|1",
    "approxF|0|0|-1",
    // approx：零向量（关键：返回 NORTH）
    "approxF|0|0|0",
    // approx：极小值（点积仍是 0）
    "approxF|1e-40|0|0",
    "approxF|-1e-40|0|0",
    "approxF|1e-46|1e-46|1e-46",
    // approx：斜向
    "approxF|1|1|0",
    "approxF|1|0|1",
    "approxF|0|1|1",
    "approxF|1|1|1",
    "approxF|-1|-1|-1",
    "approxF|2|1|0",
    "approxF|1|2|0",
    // approxD：double 版（先降精度）
    "approxD|1|0|0",
    "approxD|0|0|0",
    "approxD|1e-300|0|0",
    // nearest：单轴主导
    "nearest|5|1|1|north",
    "nearest|-5|1|1|north",
    "nearest|1|5|1|north",
    "nearest|1|-5|1|north",
    "nearest|1|1|5|north",
    "nearest|1|1|-5|north",
    // nearest：全零 → fallback
    "nearest|0|0|0|north",
    "nearest|0|0|0|up",
    // nearest：并列 → fallback（严格大于）
    "nearest|5|5|1|north",
    "nearest|5|1|5|north",
    "nearest|1|5|5|north",
    "nearest|5|5|5|north",
    "nearest|-5|-5|1|up",
    // nearest：绝对值比较，符号独立
    "nearest|-5|1|1|north",
    "nearest|5|-1|-1|north",
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
        throw std::runtime_error("direction_nearest: unknown direction: " + name);
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
            return "null";
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("direction_nearest: unit needs at least two fields: " + unit);
    }

    const std::string mode = parts[0];

    if (mode == "approxF" && parts.size() == 4) {
        const f32 x = static_cast<f32>(std::stod(parts[1]));
        const f32 y = static_cast<f32>(std::stod(parts[2]));
        const f32 z = static_cast<f32>(std::stod(parts[3]));
        return json{{"unit", unit}, {"value", directionName(getApproximateNearest(x, y, z))}}.dump();
    }
    if (mode == "approxD" && parts.size() == 4) {
        // 原版 double 重载先把参数降成 float 再转调。
        const f64 x = std::stod(parts[1]);
        const f64 y = std::stod(parts[2]);
        const f64 z = std::stod(parts[3]);
        return json{{"unit", unit},
            {"value",
                directionName(getApproximateNearest(static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(z)))}}
            .dump();
    }
    if (mode == "nearest" && parts.size() == 5) {
        const i32 x = std::stoi(parts[1]);
        const i32 y = std::stoi(parts[2]);
        const i32 z = std::stoi(parts[3]);
        const Direction fallback = parseDirection(parts[4]);
        return json{{"unit", unit}, {"value", directionName(getNearest(x, y, z, fallback))}}.dump();
    }

    throw std::runtime_error("direction_nearest: unrecognized unit: " + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(direction_nearest, &writeList, &runLine)

} // namespace mc::diff
