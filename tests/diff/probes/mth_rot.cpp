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
 * @brief 差分能力：`Mth` 的旋转插值（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": "..." }
 *
 * 复刻原版 `Mth`：
 *
 * ```java
 * public static float rotLerp(float delta, float start, float end) {
 *     return start + delta * wrapDegrees(end - start);
 * }
 *
 * public static double rotLerp(double delta, double start, double end) {
 *     return start + delta * wrapDegrees(end - start);
 * }
 *
 * public static float rotLerpRad(float delta, float start, float end) {
 *     float f = end - start;
 *     while (f < (float) -Math.PI) { f += (float) (Math.PI * 2); }
 *     while (f >= (float) Math.PI)  { f -= (float) (Math.PI * 2); }
 *     return start + delta * f;
 * }
 *
 * public static float triangleWave(float input, float period) {
 *     return (Math.abs(input % period - period * 0.5F) - period * 0.25F) / (period * 0.25F);
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`rotLerp` 走的是最短弧。** 它先把 `end - start` 用 `wrapDegrees` 折到
 *    `[-180, 180)` 再插值。所以 `rotLerp(0.5, 350, 10)` 的实际增量是 `wrapDegrees(-340) = 20`
 *    （跨过 0 度），结果 360；而 `rotLerp(0.5, 10, 350)` 的增量是 `wrapDegrees(340) = -20`，
 *    结果 0。**这不表示 0 度与 360 度等价**——插值后的值可能超出 `[0, 360)`。
 *
 * 2. **`rotLerpRad` 用 `while` 循环而不是 `wrapDegrees`。** 两者在常规输入下结果相同，但
 *    `rotLerpRad` 的循环可以处理任意大的差值（多次回绕），而 `wrapDegrees` 的两次 `if`
 *    依赖 `% 360` 的结果范围。弧度版用 **`(float) Math.PI`** 作为边界——不是双精度的 `PI`，
 *    这对恰好落在边界的输入有影响。
 *
 * 3. **`rotLerpRad` 的区间是 `[-PI, PI)`**（左闭右开），与 `wrapDegrees` 的 `[-180, 180)`
 *    一致。`f >= PI` 时才减，所以 `f` 恰好等于 `PI` 会被减成 `-PI`。
 *
 * 4. **`triangleWave` 只有非负输入才落在 `[-1, 1]`。** `(|x % p - p/2| - p/4) / (p/4)`：
 *    当 `x % p = p/2` 时结果 -1、当 `x % p = 0` 时结果 +1。但 Java 的 `%` 对**负数**取余
 *    结果也是负数（符号跟随被除数），负输入会让分子超出预期：
 *    `triangleWave(-0.25, 1)` 得 **2**、`triangleWave(-0.5, 1)` 得 **3**——完全在
 *    `[-1, 1]` 之外。原版调用方都保证传入非负值，但工具函数本身不做这个保证。
 *
 * 单元格式：`<模式>|<参数…>`
 *
 * TODO: 未覆盖 `Mth.sin` / `Mth.cos`（大角度查表近似，需单独能力）。
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 原版 `Math.PI` 转 `float` 后的值（注意是单精度）。
constexpr f32 PI_F = static_cast<f32>(3.141592653589793);
constexpr f32 TWO_PI_F = static_cast<f32>(3.141592653589793 * 2.0);

std::string formatNumber(f64 value)
{
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

/**
 * @brief 原版 `Mth.wrapDegrees(double)`：两个独立 `if`。
 */
f64 wrapDegreesD(f64 degrees)
{
    f64 result = std::fmod(degrees, 360.0);
    if (result >= 180.0) {
        result -= 360.0;
    }
    if (result < -180.0) {
        result += 360.0;
    }
    return result;
}

/**
 * @brief 原版 `Mth.wrapDegrees(float)`。
 */
f32 wrapDegreesF(f32 degrees)
{
    f32 result = std::fmod(degrees, 360.0f);
    if (result >= 180.0f) {
        result -= 360.0f;
    }
    if (result < -180.0f) {
        result += 360.0f;
    }
    return result;
}

/// 原版 `Mth.rotLerp(double, ...)`。
f64 rotLerpD(f64 delta, f64 start, f64 end)
{
    return start + delta * wrapDegreesD(end - start);
}

/// 原版 `Mth.rotLerp(float, ...)`。
f32 rotLerpF(f32 delta, f32 start, f32 end)
{
    return start + delta * wrapDegreesF(end - start);
}

/**
 * @brief 原版 `Mth.rotLerpRad`：用 `while` 循环折到 `[-PI, PI)`。
 */
f32 rotLerpRad(f32 delta, f32 start, f32 end)
{
    f32 diff = end - start;
    while (diff < -PI_F) {
        diff += TWO_PI_F;
    }
    while (diff >= PI_F) {
        diff -= TWO_PI_F;
    }
    return start + delta * diff;
}

/// 原版 `Mth.triangleWave`。
f32 triangleWave(f32 input, f32 period)
{
    return (std::fabs(std::fmod(input, period) - period * 0.5f) - period * 0.25f) / (period * 0.25f);
}

/// 单元表。
constexpr const char* UNITS[] = {
    // rotLerp(double)：常规插值
    "rotlerpd|0.5|0|90",
    "rotlerpd|0|0|90",
    "rotlerpd|1|0|90",
    "rotlerpd|0.5|0|180",
    // rotLerp：跨 0 度（最短弧）
    "rotlerpd|0.5|350|10",
    "rotlerpd|0.5|10|350",
    "rotlerpd|1|350|10",
    "rotlerpd|0|350|10",
    // rotLerp：差值恰好 180
    "rotlerpd|0.5|0|180",
    "rotlerpd|0.5|180|0",
    // rotLerp：多圈差值
    "rotlerpd|0.5|0|730",
    "rotlerpd|0.5|0|-730",
    // rotLerp(float)
    "rotlerpf|0.5|0|90",
    "rotlerpf|0.5|350|10",
    "rotlerpf|0.25|0|45",
    // rotLerpRad：常规
    "rotlerprad|0.5|0|1.5707963",
    "rotlerprad|0|0|1.5707963",
    "rotlerprad|1|0|1.5707963",
    // rotLerpRad：跨 ±PI
    "rotlerprad|0.5|3.0|-3.0",
    "rotlerprad|0.5|-3.0|3.0",
    // rotLerpRad：多圈
    "rotlerprad|0.5|0|12.566370",
    "rotlerprad|0.5|0|-12.566370",
    // triangleWave：内部点
    "triwave|0|1",
    "triwave|0.25|1",
    "triwave|0.5|1",
    "triwave|0.75|1",
    "triwave|1|1",
    // triangleWave：周期非 1
    "triwave|0|10",
    "triwave|2.5|10",
    "triwave|5|10",
    "triwave|7.5|10",
    // triangleWave：负输入（取余符号跟随被除数）
    "triwave|-0.25|1",
    "triwave|-0.5|1",
    "triwave|-2.5|10",
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

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("mth_rot: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "rotlerpd" && parts.size() == 4) {
        const f64 v = rotLerpD(std::stod(parts[1]), std::stod(parts[2]), std::stod(parts[3]));
        return json{{"unit", unit}, {"value", formatNumber(v)}}.dump();
    }
    if (mode == "rotlerpf" && parts.size() == 4) {
        const f32 v = rotLerpF(static_cast<f32>(std::stod(parts[1])),
            static_cast<f32>(std::stod(parts[2])),
            static_cast<f32>(std::stod(parts[3])));
        return json{{"unit", unit}, {"value", formatNumber(static_cast<f64>(v))}}.dump();
    }
    if (mode == "rotlerprad" && parts.size() == 4) {
        const f32 v = rotLerpRad(static_cast<f32>(std::stod(parts[1])),
            static_cast<f32>(std::stod(parts[2])),
            static_cast<f32>(std::stod(parts[3])));
        return json{{"unit", unit}, {"value", formatNumber(static_cast<f64>(v))}}.dump();
    }
    if (mode == "triwave" && parts.size() == 3) {
        const f32 v = triangleWave(static_cast<f32>(std::stod(parts[1])), static_cast<f32>(std::stod(parts[2])));
        return json{{"unit", unit}, {"value", formatNumber(static_cast<f64>(v))}}.dump();
    }

    throw std::runtime_error("mth_rot: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(mth_rot, &writeList, &runLine)

} // namespace mc::diff
