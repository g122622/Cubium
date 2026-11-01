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
 * @brief 差分能力：`Mth` 的角度差与朝向平滑（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.util.Mth`：
 *
 * ```java
 * public static float degreesDifference(float from, float to) {
 *     return wrapDegrees(to - from);              // 注意参数顺序
 * }
 *
 * public static float degreesDifferenceAbs(float from, float to) {
 *     return abs(degreesDifference(from, to));
 * }
 *
 * public static float approachDegrees(float current, float target, float maxStep) {
 *     float diff = degreesDifference(current, target);
 *     return approach(current, current + diff, maxStep);
 * }
 *
 * public static float rotateIfNecessary(float current, float target, float maxStep) {
 *     float diff = degreesDifference(current, target);
 *     float clamped = clamp(diff, -maxStep, maxStep);
 *     return target - clamped;                    // 从 target 反推
 * }
 *
 * public static int    absMax(int a, int b)       { return Math.max(Math.abs(a), Math.abs(b)); }
 * public static float  absMax(float a, float b)   { return Math.max(Math.abs(a), Math.abs(b)); }
 * public static double absMax(double a, double b) { return Math.max(Math.abs(a), Math.abs(b)); }
 *
 * public static int sign(double value) {
 *     if (value == 0.0) return 0;
 *     return value > 0.0 ? 1 : -1;
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`degreesDifference(from, to)` 的参数是「起点, 终点」，内部算 `wrapDegrees(to - from)`。**
 *
 *    返回值是「从 `from` 转到 `to` 需要的最短转角」，符号表示方向，区间 `(-180, 180]`。
 *
 *    **注意 `degreesDifference(a, b) = -degreesDifference(b, a)` 只在非边界处成立**——
 *    因为 `wrapDegrees(180)` 得 `-180`，边界处会破坏反对称性。
 *
 * 2. **`rotateIfNecessary` 从 `target` 反推，不是从 `current` 前进。**
 *
 *    ```java
 *    return target - clamp(diff, -maxStep, maxStep);
 *    ```
 *
 *    直觉写法是 `current + clampedStep`，但原版是 `target - clampedDiff`。
 *
 *    由于 `diff = wrapDegrees(target - current)`，两者在**没有 wrap 时数学等价**
 *    （`target - (target - current) = current`）。但**一旦发生 wrap，结果会落在 `target` 附近
 *    而不是 `current` 附近**：
 *
 *    - `rotateIfNecessary(170, -170, 5)`：`diff = wrapDegrees(-340) = 20`，
 *      `clamp(20, -5, 5) = 5`，`-170 - 5 = -175`
 *    - 直觉的 `current + 5 = 175`——**两者相差整整 350 度**
 *
 *    `-175` 与 `175` 在角度上等价（差 360），但**数值不同**，会写进实体数据。
 *
 * 3. **`sign` 把 `NaN` 判为 `-1`。**
 *
 *    ```java
 *    if (value == 0.0) return 0;
 *    return value > 0.0 ? 1 : -1;
 *    ```
 *
 *    `NaN == 0.0` 为 `false`，`NaN > 0.0` 也为 `false`，所以**走到 `-1`**。
 *    这与 `Math.signum(NaN)` 返回 `NaN` 不同。
 *
 * 4. **`absMax` 的 `int` 版在 `Integer.MIN_VALUE` 上溢出。**
 *
 *    `Math.abs(Integer.MIN_VALUE)` 仍是 `Integer.MIN_VALUE`（负），因此
 *    `absMax(Integer.MIN_VALUE, 0)` 返回 `Integer.MIN_VALUE`（负数），**不是 `0` 或正数**。
 *
 *    这是补码的固有边界，Cubium 若用 `std::abs` 也会得到同样的负值。
 *
 * 单元格式：
 * - `diff|<from>|<to>` —— `degreesDifference`
 * - `diffAbs|<from>|<to>` —— `degreesDifferenceAbs`
 * - `approachDeg|<current>|<target>|<maxStep>` —— `approachDegrees`
 * - `rotate|<current>|<target>|<maxStep>` —— `rotateIfNecessary`
 * - `absMaxI|<a>|<b>` / `absMaxF|<a>|<b>` / `absMaxD|<a>|<b>` —— `absMax` 三个重载
 * - `sign|<value>` —— `sign`
 *
 * 角度类结果是 `float`，按 `%.9g` 格式化后去尾零；`absMaxI` 与 `sign` 输出整数。
 */
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 格式化浮点：`%.9g` 并去尾零。
std::string formatNumber(f64 value)
{
    if (std::isnan(value)) {
        return "NaN";
    }
    if (std::isinf(value)) {
        return value > 0 ? "Infinity" : "-Infinity";
    }
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.9g", value);
    std::string text(buffer);
    if (text.find('.') != std::string::npos && text.find('e') == std::string::npos &&
        text.find('E') == std::string::npos) {
        while (!text.empty() && text.back() == '0') {
            text.pop_back();
        }
        if (!text.empty() && text.back() == '.') {
            text.pop_back();
        }
    }
    return text;
}

/// 原版 `Mth.wrapDegrees(float)`。
f32 wrapDegrees(f32 degrees)
{
    f32 f = std::fmod(degrees, 360.0F);
    if (f >= 180.0F) {
        f -= 360.0F;
    }
    if (f < -180.0F) {
        f += 360.0F;
    }
    return f;
}

/// 原版 `Mth.abs(float)`。
f32 mthAbs(f32 value)
{
    return value >= 0.0F ? value : -value;
}

/// 原版 `Mth.clamp(float, float, float)`。
f32 clampF(f32 value, f32 min, f32 max)
{
    return value < min ? min : (value > max ? max : value);
}

/// 原版 `Mth.degreesDifference`：注意是 `wrapDegrees(to - from)`。
f32 degreesDifference(f32 from, f32 to)
{
    return wrapDegrees(to - from);
}

/// 原版 `Mth.approach`。
f32 approach(f32 current, f32 target, f32 step)
{
    const f32 magnitude = mthAbs(step);
    return current < target ? clampF(current + magnitude, current, target)
                            : clampF(current - magnitude, target, current);
}

/// 原版 `Mth.approachDegrees`。
f32 approachDegrees(f32 current, f32 target, f32 maxStep)
{
    const f32 diff = degreesDifference(current, target);
    return approach(current, current + diff, maxStep);
}

/// 原版 `Mth.rotateIfNecessary`：从 `target` 反推。
f32 rotateIfNecessary(f32 current, f32 target, f32 maxStep)
{
    const f32 diff = degreesDifference(current, target);
    const f32 clamped = clampF(diff, -maxStep, maxStep);
    return target - clamped;
}

/// 原版 `Mth.sign(double)`：`NaN` 走 `-1`。
i32 sign(f64 value)
{
    if (value == 0.0) {
        return 0;
    }
    return value > 0.0 ? 1 : -1;
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // diff：常规与跨边界
        "diff|0|90",
        "diff|90|0",
        "diff|170|-170",
        "diff|-170|170",
        "diff|0|180",
        "diff|180|0",
        // diffAbs
        "diffAbs|170|-170",
        "diffAbs|-170|170",
        // approachDeg：常规逼近
        "approachDeg|0|90|10",
        "approachDeg|0|90|100",
        "approachDeg|90|0|10",
        // approachDeg：跨 ±180 边界
        "approachDeg|170|-170|10",
        "approachDeg|-170|170|10",
        // rotate：常规
        "rotate|0|90|10",
        "rotate|0|90|100",
        "rotate|90|0|10",
        // rotate：跨边界（关键：从 target 反推）
        "rotate|170|-170|5",
        "rotate|-170|170|5",
        "rotate|170|-170|100",
        // absMax
        "absMaxI|5|-10",
        "absMaxI|-2147483648|0",
        "absMaxI|-2147483648|5",
        "absMaxF|-3.5|2",
        "absMaxD|-3.5|2",
        // sign
        "sign|0",
        "sign|5",
        "sign|-5",
        "sign|0.0001",
        "sign|-0.0001",
    };
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

f32 parseF(const std::string& text)
{
    return static_cast<f32>(std::stod(text));
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("mth_degrees: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "diff" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", formatNumber(degreesDifference(parseF(parts[1]), parseF(parts[2])))}}
            .dump();
    }
    if (mode == "diffAbs" && parts.size() == 3) {
        const f32 difference = degreesDifference(parseF(parts[1]), parseF(parts[2]));
        return json{{"unit", unit}, {"value", formatNumber(mthAbs(difference))}}.dump();
    }
    if (mode == "approachDeg" && parts.size() == 4) {
        return json{{"unit", unit},
            {"value", formatNumber(approachDegrees(parseF(parts[1]), parseF(parts[2]), parseF(parts[3])))}}
            .dump();
    }
    if (mode == "rotate" && parts.size() == 4) {
        return json{{"unit", unit},
            {"value", formatNumber(rotateIfNecessary(parseF(parts[1]), parseF(parts[2]), parseF(parts[3])))}}
            .dump();
    }
    if (mode == "absMaxI" && parts.size() == 3) {
        const i32 first = std::stoi(parts[1]);
        const i32 second = std::stoi(parts[2]);
        const i32 absFirst = first < 0 ? static_cast<i32>(0U - static_cast<u32>(first)) : first;
        const i32 absSecond = second < 0 ? static_cast<i32>(0U - static_cast<u32>(second)) : second;
        return json{{"unit", unit}, {"value", std::max(absFirst, absSecond)}}.dump();
    }
    if (mode == "absMaxF" && parts.size() == 3) {
        return json{
            {"unit", unit}, {"value", formatNumber(std::max(mthAbs(parseF(parts[1])), mthAbs(parseF(parts[2]))))}}
            .dump();
    }
    if (mode == "absMaxD" && parts.size() == 3) {
        return json{{"unit", unit},
            {"value", formatNumber(std::max(std::fabs(std::stod(parts[1])), std::fabs(std::stod(parts[2]))))}}
            .dump();
    }
    if (mode == "sign" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", sign(std::stod(parts[1]))}}.dump();
    }

    throw std::runtime_error("mth_degrees: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("mth_degrees", &writeList, &runLine)

} // namespace mc::diff
