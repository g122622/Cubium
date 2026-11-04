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

/**
 * @brief 差分能力：振动距离到红石强度的换算（`VibrationSystem#getRedstoneStrengthForDistance`）。
 *
 * `list`：监听半径 × 距离网格
 * `run` ：一行 -> { "strength": 1..15 }
 *
 * ## 公式
 *
 * <pre>
 *   d0       = 15.0 / radius
 *   strength = max(1, 15 - floor(d0 * distance))
 * </pre>
 *
 * 距离为 0 时强度 15（满信号），到边界时衰减到 1。半径越大衰减越慢。
 *
 * ## 为什么值得单独固定
 *
 * 这是幽匿感测体给红石比较器的**唯一**输出值。玩家整个振动装置（比较器读数分辨
 * 事件类型、距离衰减做距离判定）都建立在这个数上。
 *
 * 三个容易写错的地方：
 *
 * 1. **除号内外**：写成 `15 - floor(15 * distance / radius)` 在整数运算下会截断两次，
 *    和原版的双精度算法在某些点上差 1
 * 2. **下限**：`max(1, ...)` 不能漏，否则远处事件给出 0，比较器读成「无信号」而不是「弱信号」
 * 3. **距离为 0**：必须是 15，不能是 15 - 0 之外的任何值
 *
 * 这三个错都不会崩、不会报错，只是读数差 1 —— 而依赖精确读数的装置会整体失效。
 */

#include <bit>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/world/gameevent/VibrationSystem.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 监听半径：幽匿感测体 8、校准幽匿感测体 16、监守者 16，外加原版共振测试用的几个值
constexpr i32 RADII[] = {1, 2, 5, 8, 9, 15, 16, 32};

/// 距离采样：覆盖 0、边界、半径内外、半格偏移
constexpr f32 DISTANCES[] = {
    0.0f,
    0.5f,
    1.0f,
    2.0f,
    3.5f,
    4.0f,
    5.0f,
    7.5f,
    8.0f,
    9.0f,
    12.0f,
    15.0f,
    16.0f,
    20.0f,
    32.0f,
};

void writeList(FILE* out)
{
    for (const i32 radius : RADII) {
        for (const f32 distance : DISTANCES) {
            // list 用 "半径:距离" 的形式，距离保留一位小数避免精度歧义
            std::fprintf(out, "%d:%.1f\n", radius, static_cast<f64>(distance));
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const std::size_t sep = unit.find(':');
    if (sep == std::string::npos) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }
    const i32 radius = std::stoi(unit.substr(0, sep));
    const f64 distance = std::stod(unit.substr(sep + 1));
    if (radius <= 0 || distance < 0.0) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    json result{{"unit", unit}};
    result["strength"] = gameevent::VibrationSystem::getRedstoneStrengthForDistance(static_cast<f32>(distance), radius);
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(vibration_strength, &writeList, &runLine)

} // namespace mc::diff
