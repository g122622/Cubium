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
 * @brief 差分能力：`Mth.absMax` / `chessboardDistance` / `smallestSquareSide`（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.world.util.Mth`：
 *
 * ```java
 * public static int absMax(int a, int b) {
 *     return Math.max(Math.abs(a), Math.abs(b));
 * }
 * public static float absMax(float a, float b) {
 *     return Math.max(Math.abs(a), Math.abs(b));
 * }
 * public static double absMax(double a, double b) {
 *     return Math.max(Math.abs(a), Math.abs(b));
 * }
 *
 * public static int chessboardDistance(int x1, int z1, int x2, int z2) {
 *     return absMax(x2 - x1, z2 - z1);
 * }
 *
 * public static int smallestSquareSide(int itemCount) {
 *     if (itemCount < 0) {
 *         throw new IllegalArgumentException("itemCount must be greater than or equal to zero");
 *     }
 *     return ceil(Math.sqrt(itemCount));
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`absMax` 不是「先比较再取绝对值」，而是「先各自取绝对值再取较大者」。**
 *
 *    两者对常规值等价，但**对 `Integer.MIN_VALUE` 不同**：
 *
 *    ```java
 *    Math.abs(Integer.MIN_VALUE) == Integer.MIN_VALUE   // 溢出，仍为负
 *    ```
 *
 *    所以 `absMax(Integer.MIN_VALUE, 5)`：
 *    - 原版：`Math.max(-2147483648, 5)` = **`5`**
 *    - 「先比较再取绝对值」的错误实现：`Math.abs(Integer.MIN_VALUE)` = **`-2147483648`**
 *
 *    **结果符号都不同。**
 *
 * 2. **`chessboardDistance` 是 2D 的（切比雪夫距离），参数是两组 `x` / `z`。**
 *
 *    ```java
 *    return absMax(x2 - x1, z2 - z1);
 *    ```
 *
 *    **没有 `y` 分量**——它不是 3D 距离。名字里的 `chessboard` 指国际象棋王的移动距离
 *    （斜走也算一步）。
 *
 *    注意参数顺序是 `(x1, z1, x2, z2)`，**`z1` 在前**（不是 `(x1, x2, z1, z2)`）。
 *
 * 3. **`chessboardDistance` 的减法可能溢出。**
 *
 *    坐标是 `int`，`x2 - x1` 可能超出 `int` 范围（如 `2147483647 - (-1)`）。
 *    Java 的 `int` 减法**静默回绕**（wraparound），C++ 的有符号溢出是 UB。
 *
 *    探针必须用 `static_cast<i32>(static_cast<u32>(x2) - static_cast<u32>(x1))` 精确复刻。
 *
 * 4. **`smallestSquareSide(0)` 返回 `0`，负数抛异常。**
 *
 *    | 输入 | 结果 |
 *    |---|---|
 *    | `0` | `0` |
 *    | `1` | `1` |
 *    | `2` | `2` |
 *    | `4` | `2` |
 *    | `5` | `3` |
 *
 *    用 `ceil(Math.sqrt(n))`，不是 `Mth.ceil`（`sqrt` 返回 `double`，`ceil` 是 `Math.ceil`）。
 *
 * 单元格式：
 * - `absMaxInt|<a>|<b>` —— `absMax(int, int)`
 * - `absMaxFloat|<a>|<b>` —— `absMax(float, float)`
 * - `absMaxDouble|<a>|<b>` —— `absMax(double, double)`
 * - `chess|<x1>|<z1>|<x2>|<z2>` —— `chessboardDistance`
 * - `square|<n>` —— `smallestSquareSide`
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <stdexcept>
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

/// 原版 `Math.abs(int)`：对 `Integer.MIN_VALUE` 返回自身（溢出）。
i32 javaAbsInt(i32 value)
{
    if (value == std::numeric_limits<i32>::min()) {
        return value;
    }
    return value < 0 ? -value : value;
}

/// 原版 `Math.max(int, int)`。
i32 javaMaxInt(i32 a, i32 b)
{
    return a >= b ? a : b;
}

/// 原版 `Mth.absMax(int, int)`。
///
/// 直接转调生产代码：探针内保留副本会让生产实现的缺陷隐身。
i32 absMaxInt(i32 a, i32 b)
{
    return math::absMax(a, b);
}

/// 原版 `Mth.chessboardDistance`。
///
/// 生产签名是 `(x1, z1, x2, z2)`，与这里的形参逐位对应，直接按序转调。
i32 chessboardDistance(i32 x1, i32 z1, i32 x2, i32 z2)
{
    return math::chessboardDistance(x1, z1, x2, z2);
}

/// 格式化浮点：9 位有效数字 + 去尾随零 + **十进制全展开**（与 Java 的
/// `BigDecimal.stripTrailingZeros().toPlainString()` 一致），不使用科学计数法。
std::string formatFloat(f64 value)
{
    if (std::isnan(value)) {
        return "nan";
    }
    if (std::isinf(value)) {
        return value > 0 ? "inf" : "-inf";
    }
    // 先取 9 位有效数字的科学计数法形式，再手工展开成定点表示。
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.8e", value);
    std::string text = buffer;

    const std::size_t exponentAt = text.find('e');
    const i32 exponent = exponentAt == std::string::npos ? 0 : static_cast<i32>(std::stoi(text.substr(exponentAt + 1)));
    std::string mantissa = exponentAt == std::string::npos ? text : text.substr(0, exponentAt);

    std::string sign;
    if (!mantissa.empty() && (mantissa[0] == '-' || mantissa[0] == '+')) {
        if (mantissa[0] == '-') {
            sign = "-";
        }
        mantissa.erase(0, 1);
    }

    std::string digits;
    for (char ch : mantissa) {
        if (ch != '.') {
            digits.push_back(ch);
        }
    }
    while (!digits.empty() && digits.back() == '0') {
        digits.pop_back();
    }
    if (digits.empty()) {
        return "0";
    }

    // 数值 = digits * 10^(exponent - (digits.length - 1))，
    // 即小数点在 digits 中的位置为 exponent + 1。
    const i32 pointAt = exponent + 1;
    std::string result;
    if (pointAt <= 0) {
        result = "0." + std::string(static_cast<std::size_t>(-pointAt), '0') + digits;
    } else if (static_cast<std::size_t>(pointAt) >= digits.size()) {
        result = digits + std::string(static_cast<std::size_t>(pointAt) - digits.size(), '0');
    } else {
        result = digits.substr(0, static_cast<std::size_t>(pointAt)) + "." +
            digits.substr(static_cast<std::size_t>(pointAt));
    }
    return sign + result;
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // absMaxInt：常规
        "absMaxInt|3|-5",
        "absMaxInt|-3|-5",
        "absMaxInt|0|0",
        "absMaxInt|7|0",
        // absMaxInt：MIN_VALUE 溢出
        "absMaxInt|-2147483648|5",
        "absMaxInt|-2147483648|-2147483648",
        "absMaxInt|-2147483648|0",
        // absMaxFloat
        "absMaxFloat|3.5|-5.25",
        "absMaxFloat|-1.5|-2",
        "absMaxFloat|0|-0",
        // absMaxDouble
        "absMaxDouble|3.5|-5.25",
        "absMaxDouble|-1e10|1e9",
        // chess：常规
        "chess|0|0|3|4",
        "chess|0|0|4|3",
        "chess|5|5|5|5",
        "chess|-2|-3|1|1",
        // chess：溢出
        "chess|0|0|2147483647|-1",
        // square
        "square|0",
        "square|1",
        "square|2",
        "square|3",
        "square|4",
        "square|5",
        "square|9",
        "square|10",
        "square|16",
        "square|17",
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

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("mth_absmax: 单元段数不足：" + unit);
    }

    const std::string mode = parts[0];
    std::string value;

    if (mode == "absMaxInt" && parts.size() == 3) {
        const i32 a = static_cast<i32>(std::stoll(parts[1]));
        const i32 b = static_cast<i32>(std::stoll(parts[2]));
        value = std::to_string(absMaxInt(a, b));
    } else if (mode == "absMaxFloat" && parts.size() == 3) {
        const f32 a = std::stof(parts[1]);
        const f32 b = std::stof(parts[2]);
        const f32 result = std::fabs(a) >= std::fabs(b) ? std::fabs(a) : std::fabs(b);
        value = formatFloat(static_cast<f64>(result));
    } else if (mode == "absMaxDouble" && parts.size() == 3) {
        const f64 a = std::stod(parts[1]);
        const f64 b = std::stod(parts[2]);
        const f64 result = std::fabs(a) >= std::fabs(b) ? std::fabs(a) : std::fabs(b);
        value = formatFloat(result);
    } else if (mode == "chess" && parts.size() == 5) {
        const i32 x1 = static_cast<i32>(std::stoll(parts[1]));
        const i32 z1 = static_cast<i32>(std::stoll(parts[2]));
        const i32 x2 = static_cast<i32>(std::stoll(parts[3]));
        const i32 z2 = static_cast<i32>(std::stoll(parts[4]));
        value = std::to_string(chessboardDistance(x1, z1, x2, z2));
    } else if (mode == "square" && parts.size() == 2) {
        const i32 count = static_cast<i32>(std::stoll(parts[1]));
        if (count < 0) {
            value = "!IllegalArgumentException";
        } else {
            // 原版 ceil(Math.sqrt(count))，Math.sqrt 返回 double。
            value = std::to_string(static_cast<i32>(std::ceil(std::sqrt(static_cast<f64>(count)))));
        }
    } else {
        throw std::runtime_error("mth_absmax: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(mth_absmax, &writeList, &runLine)

} // namespace mc::diff
