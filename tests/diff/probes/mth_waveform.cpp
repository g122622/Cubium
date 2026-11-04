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
 * @brief 差分能力：`Mth` 的波形与缓动函数（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.util.Mth`：
 *
 * ```java
 * public static float triangleWave(float x, float period) {
 *     return (Math.abs(x % period - period * 0.5F) - period * 0.25F) / (period * 0.25F);
 * }
 *
 * public static double smoothstep(double x) {
 *     return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
 * }
 *
 * public static double smoothstepDerivative(double x) {
 *     return 30.0 * x * x * (x - 1.0) * (x - 1.0);
 * }
 *
 * public static float square(float x) { return x * x; }
 * public static float cube(float x)   { return x * x * x; }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`triangleWave` 的输出范围是 `[-1, 1]`，但两端都不取到。**
 *
 *    公式 `(|x % p - p/2| - p/4) / (p/4)`：
 *
 *    | `x % p` | 绝对值项 | 结果 |
 *    |---|---|---|
 *    | `0` | `p/2` | `(p/2 - p/4)/(p/4) = 1` |
 *    | `p/4` | `p/4` | `0` |
 *    | `p/2` | `0` | `-1` |
 *    | `3p/4` | `p/4` | `0` |
 *
 *    **在 `x % p = 0` 处取到 `1`（峰值），在 `x % p = p/2` 处取到 `-1`（谷值）。**
 *
 * 2. **`triangleWave` 的 `%` 是 Java 取模，负数结果为负——波形不正对称。**
 *
 *    Java 的 `%` 向零截断：`(-3) % 10 = -3`，而数学取模是 `7`。
 *
 *    `triangleWave(-3, 10)`：
 *    - `-3 % 10 = -3`
 *    - `|-3 - 5| = 8`
 *    - `(8 - 2.5) / 2.5 = 2.2`
 *
 *    **结果 `2.2` 超出了 `[-1,1]`！** 负输入会产生越界值，这是原版行为。
 *
 * 3. **`smoothstep` 是五次多项式（不是三次），没有 clamp。**
 *
 *    ```java
 *    return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
 *    ```
 *
 *    展开是 `6x⁵ - 15x⁴ + 10x³`——这是 **Perlin 的 quintic fade 曲线**，一阶与二阶导数在
 *    `x=0` 和 `x=1` 处都为 0。
 *
 *    - `smoothstep(0)` = `0`
 *    - `smoothstep(1)` = `6-15+10` = `1`
 *    - `smoothstep(0.5)` = `0.5`
 *    - **`smoothstep(-0.5)` = `6(-0.03125) - 15(0.0625) + 10(-0.125)` = `-0.1875 - 0.9375 - 1.25` = `-2.375`**
 *
 *    **超出 `[0,1]` 时不做 clamp**，值会继续外推。
 *
 * 4. **`smoothstepDerivative` 在 `x=0` 与 `x=1` 处为 0，在 `x=0.5` 处最大。**
 *
 *    ```java
 *    return 30.0 * x * x * (x - 1.0) * (x - 1.0);
 *    ```
 *
 *    注意 `(x-1.0)` 出现了**两次**（平方）。`smoothstepDerivative(0.5)` =
 *    `30 × 0.25 × 0.25 × 0.25` = `0.46875`。
 *
 *    这与 `smoothstep` 的解析导数一致：`d/dx(6x⁵-15x⁴+10x³) = 30x⁴-60x³+30x² = 30x²(x-1)²`。
 *
 * 单元格式：
 * - `tri|<x>|<period>` —— `triangleWave`
 * - `smooth|<x>` —— `smoothstep`（`double`）
 * - `smoothD|<x>` —— `smoothstepDerivative`（`double`）
 * - `square|<x>` / `cube|<x>` —— `float` 版
 *
 * `triangleWave` / `square` / `cube` 是 `float` 运算，`smoothstep` 系列是 `double`。
 * 统一按 `%.9g` 格式化后去尾零。
 *
 * TODO: 未覆盖 `Mth.lerp2` / `lerp3`（已在 `mth_lerp` / `mth_inverse_lerp` 覆盖）、
 * `Mth.parabolicRand`、`Mth.nextFloat` 等随机相关函数。
 */
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

/// 格式化浮点：`%.9g` 并去尾零，特殊值单独处理。
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

/// Java 的 `%`：向零截断。
f32 javaMod(f32 value, f32 modulus)
{
    return std::fmod(value, modulus);
}

/// 原版 `Mth.triangleWave`。
f32 triangleWave(f32 x, f32 period)
{
    return (std::fabs(javaMod(x, period) - period * 0.5F) - period * 0.25F) / (period * 0.25F);
}

f64 smoothstep(f64 x)
{
    return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
}

/// 原版 `Mth.smoothstepDerivative`。
f64 smoothstepDerivative(f64 x)
{
    return 30.0 * x * x * (x - 1.0) * (x - 1.0);
}

/// 原版 `Mth.square`。
f32 square(f32 x)
{
    return x * x;
}

/// 原版 `Mth.cube`。
f32 cube(f32 x)
{
    return x * x * x;
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // tri：周期内的四个关键点
        "tri|0|10",
        "tri|2.5|10",
        "tri|5|10",
        "tri|7.5|10",
        "tri|10|10",
        // tri：负输入（越界！）
        "tri|-3|10",
        "tri|-5|10",
        "tri|-10|10",
        // tri：小周期
        "tri|1|4",
        "tri|3|4",
        // smooth：区间内外
        "smooth|0",
        "smooth|1",
        "smooth|0.5",
        "smooth|0.25",
        "smooth|-0.5",
        "smooth|2",
        // smoothD：导数边界
        "smoothD|0",
        "smoothD|1",
        "smoothD|0.5",
        "smoothD|0.25",
        // square / cube
        "square|3",
        "square|-3",
        "square|0.5",
        "cube|3",
        "cube|-3",
        "cube|0.5",
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
        throw std::runtime_error("mth_waveform: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "tri" && parts.size() == 3) {
        const f32 result = triangleWave(parseF(parts[1]), parseF(parts[2]));
        return json{{"unit", unit}, {"value", formatNumber(result)}}.dump();
    }
    if (mode == "smooth" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", formatNumber(smoothstep(std::stod(parts[1])))}}.dump();
    }
    if (mode == "smoothD" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", formatNumber(smoothstepDerivative(std::stod(parts[1])))}}.dump();
    }
    if (mode == "square" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", formatNumber(square(parseF(parts[1])))}}.dump();
    }
    if (mode == "cube" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", formatNumber(cube(parseF(parts[1])))}}.dump();
    }

    throw std::runtime_error("mth_waveform: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(mth_waveform, &writeList, &runLine)

} // namespace mc::diff
