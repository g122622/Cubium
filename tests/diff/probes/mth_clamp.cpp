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
 * @brief 差分能力：`Mth` 的钳制、映射与平方函数族（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.util.Mth`：
 *
 * ```java
 * public static int clamp(int value, int min, int max) {
 *     return Math.min(Math.max(value, min), max);
 * }
 *
 * public static float clamp(float value, float min, float max) {
 *     return value < min ? min : Math.min(value, max);
 * }
 *
 * public static double clampedLerp(double delta, double start, double end) {
 *     if (delta < 0.0) { return start; }
 *     return delta > 1.0 ? end : lerp(delta, start, end);
 * }
 *
 * public static int absMax(int a, int b) { return Math.max(Math.abs(a), Math.abs(b)); }
 * public static int chessboardDistance(int x1, int x2, int z1, int z2) {
 *     return absMax(z1 - x1, z2 - x2);
 * }
 *
 * public static float square(float value) { return value * value; }
 * public static int square(int value) { return value * value; }         // int 溢出环绕
 * public static long square(long value) { return value * value; }
 *
 * public static double inverseLerp(double value, double start, double end) {
 *     return (value - start) / (end - start);
 * }
 *
 * public static double clampedMap(double value, double inMin, double inMax,
 *                                 double outMin, double outMax) {
 *     return clampedLerp(inverseLerp(value, inMin, inMax), outMin, outMax);
 * }
 *
 * public static double lerp(double delta, double start, double end) {
 *     return start + delta * (end - start);
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **浮点 `clamp` 与整数 `clamp` 的写法不同，NaN 行为也不同。**
 *
 *    | 版本 | 实现 |
 *    |---|---|
 *    | `int` | `Math.min(Math.max(v, min), max)` |
 *    | `float` / `double` | `v < min ? min : Math.min(v, max)` |
 *
 *    浮点版本用 `<` 比较：`NaN < min` 为 `false`，于是走 `Math.min(NaN, max)`，而
 *    `Math.min` 对 `NaN` 返回 **`NaN`**。所以 **`clamp(NaN, ...)` 得 `NaN`**，不是 `min`。
 *
 *    若把浮点版本也写成 `Math.max` 开头，`Math.max(NaN, min)` 同样返回 `NaN`，结果一致——
 *    但两者的**分支路径**不同，对 `-0.0` 等边界可能有微妙差异。
 *
 * 2. **`clampedLerp` 有两个短路分支，且顺序是「先下界、后上界」。**
 *
 *    ```java
 *    if (delta < 0.0) { return start; }
 *    return delta > 1.0 ? end : lerp(delta, start, end);
 *    ```
 *
 *    `delta = 0.0` **不**短路，会走 `lerp(0, start, end) = start + 0 * (end - start)`。
 *    若 `end - start` 是 `Infinity`，`0 * Infinity = NaN`，所以 **`clampedLerp(0, a, Inf)`
 *    得 `NaN` 而不是 `a`**。上界 `delta = 1.0` 同理走 `lerp` 而非直接返回 `end`。
 *
 * 3. **`clampedMap` 是先算 `inverseLerp` 再钳制的组合。**
 *
 *    ```java
 *    clampedLerp(inverseLerp(value, inMin, inMax), outMin, outMax)
 *    ```
 *
 *    `inMin == inMax` 时 `inverseLerp` 出现 `0.0 / 0.0 = NaN`，于是 `clampedLerp(NaN, ...)`
 *    两个分支都不成立，走 `lerp(NaN, ...)` → **`NaN`**。
 *
 * 4. **`square(int)` 与 `square(long)` 会溢出环绕。**
 *
 *    `square(int)` 是 32 位乘法，`square(long)` 是 64 位乘法。`Mth.square(int)` 在
 *    `|value| > 46340` 时必然溢出。Java 的整数溢出是定义良好的二补数环绕。
 *
 * 单元格式：
 * - `clampi|<v>|<min>|<max>` / `clampf|<v>|<min>|<max>` / `clampd|<v>|<min>|<max>`
 * - `clampedLerpD|<delta>|<start>|<end>` / `clampedLerpF|<delta>|<start>|<end>`
 * - `absMaxI|<a>|<b>` / `absMaxD|<a>|<b>`
 * - `chessboard|<x1>|<x2>|<z1>|<z2>`
 * - `squareI|<v>` / `squareL|<v>` / `squareD|<v>`
 * - `inverseLerpD|<v>|<start>|<end>`
 * - `clampedMapD|<v>|<inMin>|<inMax>|<outMin>|<outMax>`
 *
 * 浮点输出统一按 `%.9g` 格式化并去掉尾随零，`NaN` / `Infinity` 原样输出。
 *
 * TODO: 未覆盖 `map`（非钳制版）、`cube`、`positiveModulo` 的浮点版本，以及 `Mth.lerpInt`。
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
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

// 全部直接调用生产代码，探针内不再保留任何副本。
// 探针自带副本会让生产实现的缺陷隐身——`direction_nearest` 曾因此整整一轮没测到真代码。

i32 clampInt(i32 value, i32 min, i32 max)
{
    return math::clamp(value, min, max);
}

f32 clampFloat(f32 value, f32 min, f32 max)
{
    return math::clamp(value, min, max);
}

f64 clampDouble(f64 value, f64 min, f64 max)
{
    return math::clamp(value, min, max);
}

f64 lerpDouble(f64 delta, f64 start, f64 end)
{
    return math::lerp(start, end, delta);
}

f32 lerpFloat(f32 delta, f32 start, f32 end)
{
    return math::lerp(start, end, delta);
}

f64 clampedLerpDouble(f64 delta, f64 start, f64 end)
{
    return math::clampedLerp(start, end, delta);
}

f32 clampedLerpFloat(f32 delta, f32 start, f32 end)
{
    return math::clampedLerp(start, end, delta);
}

f64 inverseLerpDouble(f64 value, f64 start, f64 end)
{
    return math::inverseLerp(value, start, end);
}

f64 clampedMapDouble(f64 value, f64 inMin, f64 inMax, f64 outMin, f64 outMax)
{
    return math::clampedMap(value, inMin, inMax, outMin, outMax);
}

i32 absMaxInt(i32 a, i32 b)
{
    return math::absMax(a, b);
}

i32 chessboardDistance(i32 x1, i32 x2, i32 z1, i32 z2)
{
    // 这里的形参名沿用原版单元表的 (x1, x2, z1, z2)，但**位置**直接按序传给生产函数。
    // 生产签名的形参名是 (x1, z1, x2, z2)，命名不同却逐位等价：
    // 两边都是 absMax(第3个 - 第1个, 第4个 - 第2个)。
    return math::chessboardDistance(x1, x2, z1, z2);
}

i32 squareInt(i32 value)
{
    return math::square(value);
}

i64 squareLong(i64 value)
{
    return math::square(value);
}

/**
 * @brief 浮点输出格式化：`%.9g` 后去掉尾随零，并处理 `NaN` / `Infinity`。
 *
 * 与其它能力保持同一契约，指数形式只在尾数部分去零。
 */
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

    // 指数形式：只对尾数去尾随零。
    const auto expPos = text.find_first_of("eE");
    if (expPos != std::string::npos) {
        std::string mantissa = text.substr(0, expPos);
        const std::string exponent = text.substr(expPos);
        if (mantissa.find('.') != std::string::npos) {
            while (!mantissa.empty() && mantissa.back() == '0') {
                mantissa.pop_back();
            }
            if (!mantissa.empty() && mantissa.back() == '.') {
                mantissa.pop_back();
            }
        }
        return mantissa + exponent;
    }

    if (text.find('.') != std::string::npos) {
        while (!text.empty() && text.back() == '0') {
            text.pop_back();
        }
        if (!text.empty() && text.back() == '.') {
            text.pop_back();
        }
    }
    return text;
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

i32 parseInt(const std::string& text)
{
    if (text == "MIN") {
        return INT32_MIN;
    }
    if (text == "MAX") {
        return INT32_MAX;
    }
    return static_cast<i32>(std::stoll(text));
}

f64 parseDouble(const std::string& text)
{
    if (text == "NaN") {
        return std::nan("");
    }
    if (text == "Inf") {
        return std::numeric_limits<f64>::infinity();
    }
    if (text == "-Inf") {
        return -std::numeric_limits<f64>::infinity();
    }
    return std::stod(text);
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // clamp 整数：正常、边界、反向区间
        "clampi|5|0|10",
        "clampi|0|0|10",
        "clampi|10|0|10",
        "clampi|15|0|10",
        "clampi|-5|0|10",
        "clampi|5|10|0",
        "clampi|5|5|5",
        // clamp 浮点：含 NaN
        "clampf|5|0|10",
        "clampf|-5|0|10",
        "clampf|NaN|0|10",
        "clampf|Inf|0|10",
        "clampd|0.5|0|1",
        "clampd|-0.1|0|1",
        "clampd|NaN|0|1",
        // clampedLerp：短路与边界
        "clampedLerpD|-0.5|10|20",
        "clampedLerpD|0|10|20",
        "clampedLerpD|0.5|10|20",
        "clampedLerpD|1|10|20",
        "clampedLerpD|1.5|10|20",
        "clampedLerpF|-0.5|10|20",
        "clampedLerpF|0.25|10|20",
        // absMax / chessboard
        "absMaxI|-5|3",
        "absMaxI|5|-3",
        "absMaxI|0|0",
        "absMaxD|-2.5|1.5",
        "chessboard|0|3|0|4",
        "chessboard|0|0|0|0",
        "chessboard|10|3|10|4",
        // square：溢出
        "squareI|5",
        "squareI|-5",
        "squareI|46340",
        "squareI|46341",
        "squareL|46341",
        "squareD|2.5",
        // inverseLerp
        "inverseLerpD|15|10|20",
        "inverseLerpD|10|10|20",
        "inverseLerpD|20|10|20",
        "inverseLerpD|5|10|10",
        // clampedMap
        "clampedMapD|15|10|20|0|100",
        "clampedMapD|5|10|20|0|100",
        "clampedMapD|25|10|20|0|100",
        "clampedMapD|10|10|10|0|100",
        // clampedMap 的边界样本：上面的用例都是 10/20、0/100 这类整齐数值，
        // 在小数端点上无法暴露 `outMin + t*(outMax-outMin)` 被 FMA 融合的问题。
        "clampedMapD|12.545929800000003|2.7720000000000002|17.118000000000002|4.8760000000000003|6.9310000000000009",
        "clampedMapD|11.2066044|8.1850000000000005|16.625|6.8540000000000001|23.667999999999999",
        "clampedMapD|15.084479330000002|1.7890000000000001|16.327999999999999|13.249000000000001|26.125",
        "clampedMapD|3.0866682900000004|1.1879999999999999|4.8849999999999998|6.7789999999999999|17.59",
        "clampedMapD|15.342021949999999|10.59|15.994999999999999|1.2130000000000001|4.4690000000000003",
        "clampedMapD|13.80464952|9.4369999999999994|15.183|12.515000000000001|19.741",
    };
    for (const auto* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("mth_clamp: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "clampi" && parts.size() == 4) {
        const i32 value = clampInt(parseInt(parts[1]), parseInt(parts[2]), parseInt(parts[3]));
        return json{{"unit", unit}, {"value", std::to_string(value)}}.dump();
    }
    if (mode == "clampf" && parts.size() == 4) {
        const f32 value = clampFloat(static_cast<f32>(parseDouble(parts[1])),
            static_cast<f32>(parseDouble(parts[2])),
            static_cast<f32>(parseDouble(parts[3])));
        return json{{"unit", unit}, {"value", formatNumber(value)}}.dump();
    }
    if (mode == "clampd" && parts.size() == 4) {
        const f64 value = clampDouble(parseDouble(parts[1]), parseDouble(parts[2]), parseDouble(parts[3]));
        return json{{"unit", unit}, {"value", formatNumber(value)}}.dump();
    }
    if (mode == "clampedLerpD" && parts.size() == 4) {
        const f64 value = clampedLerpDouble(parseDouble(parts[1]), parseDouble(parts[2]), parseDouble(parts[3]));
        return json{{"unit", unit}, {"value", formatNumber(value)}}.dump();
    }
    if (mode == "clampedLerpF" && parts.size() == 4) {
        const f32 value = clampedLerpFloat(static_cast<f32>(parseDouble(parts[1])),
            static_cast<f32>(parseDouble(parts[2])),
            static_cast<f32>(parseDouble(parts[3])));
        return json{{"unit", unit}, {"value", formatNumber(value)}}.dump();
    }
    if (mode == "absMaxI" && parts.size() == 3) {
        const i32 value = absMaxInt(parseInt(parts[1]), parseInt(parts[2]));
        return json{{"unit", unit}, {"value", std::to_string(value)}}.dump();
    }
    if (mode == "absMaxD" && parts.size() == 3) {
        const f64 value = std::max(std::abs(parseDouble(parts[1])), std::abs(parseDouble(parts[2])));
        return json{{"unit", unit}, {"value", formatNumber(value)}}.dump();
    }
    if (mode == "chessboard" && parts.size() == 5) {
        const i32 value =
            chessboardDistance(parseInt(parts[1]), parseInt(parts[2]), parseInt(parts[3]), parseInt(parts[4]));
        return json{{"unit", unit}, {"value", std::to_string(value)}}.dump();
    }
    if (mode == "squareI" && parts.size() == 2) {
        const i32 value = squareInt(parseInt(parts[1]));
        return json{{"unit", unit}, {"value", std::to_string(value)}}.dump();
    }
    if (mode == "squareL" && parts.size() == 2) {
        const i64 value = squareLong(static_cast<i64>(std::stoll(parts[1])));
        return json{{"unit", unit}, {"value", std::to_string(value)}}.dump();
    }
    if (mode == "squareD" && parts.size() == 2) {
        const f64 v = parseDouble(parts[1]);
        return json{{"unit", unit}, {"value", formatNumber(v * v)}}.dump();
    }
    if (mode == "inverseLerpD" && parts.size() == 4) {
        const f64 value = inverseLerpDouble(parseDouble(parts[1]), parseDouble(parts[2]), parseDouble(parts[3]));
        return json{{"unit", unit}, {"value", formatNumber(value)}}.dump();
    }
    if (mode == "clampedMapD" && parts.size() == 6) {
        const f64 value = clampedMapDouble(parseDouble(parts[1]),
            parseDouble(parts[2]),
            parseDouble(parts[3]),
            parseDouble(parts[4]),
            parseDouble(parts[5]));
        return json{{"unit", unit}, {"value", formatNumber(value)}}.dump();
    }

    throw std::runtime_error("mth_clamp: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(mth_clamp, &writeList, &runLine)

} // namespace mc::diff
