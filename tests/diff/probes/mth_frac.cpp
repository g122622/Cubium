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
 * @brief 差分能力：`Mth.frac` 的两个重载，以及它们依赖的 `floor` / `lfloor`。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.util.Mth`：
 *
 * ```java
 * public static int  floor(float f)    { int i = (int) f;   return f < i ? i - 1 : i; }
 * public static long lfloor(double d)  { long i = (long) d; return d < i ? i - 1L : i; }
 *
 * public static float  frac(float f)   { return f - floor(f); }     // int 宽度
 * public static double frac(double d)  { return d - lfloor(d); }    // long 宽度
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`frac(float)` 在大输入上完全失效——它不是「小数部分」。**
 *
 *    `floor(f)` 返回 **`int`**，当 `|f|` 超过 `Integer.MAX_VALUE`（约 `2.1e9`）时，
 *    `(int) f` 在 Java 里**饱和到 `Integer.MAX_VALUE`**。
 *
 *    - `frac(1e10F)` = `1e10 - 2147483647` ≈ **`9999999999`**（不是 `0`！）
 *    - `frac(3e9F)` = `3e9 - 2147483647` ≈ `852516353`
 *
 *    **`frac(float)` 的返回值可以远大于 1**，与「小数部分」的直觉完全不符。
 *
 *    而 `frac(double)` 用 `long`（9.2e18 才溢出），在合理量级上行为正常。
 *
 * 2. **`frac` 对负数返回正的小数部分（因为 `floor` 向负无穷）。**
 *
 *    - `frac(-1.5F)` = `-1.5 - (-2) = 0.5`
 *    - 若 `floor` 写成向零截断，会得 `-1.5 - (-1) = -0.5`——**符号相反**
 *
 *    因此 `frac` 的值域是 `[0, 1)`，**不是 `(-1, 1)`**。
 *
 * 3. **`lfloor` 与 `floor` 的实现结构相同，只有位宽不同（`long` / `int`）。**
 *
 *    两者都是「先向零截断，再对负小数减一」。
 *
 *    `lfloor` 的输入是 `double`，精度 53 位；`floor` 的输入是 `float`，精度 24 位。
 *
 * 4. **`frac` 的精度损失：对很大的 `float`，`f - floor(f)` 可能精确为 0。**
 *
 *    `float` 有 24 位有效数字，当 `|f| >= 2^24 = 16777216` 时，`f` 本身已是整数，
 *    `frac(f)` 得 `0`。
 *
 *    `double` 的阈值是 `2^52 ≈ 4.5e15`。
 *
 * 单元格式：
 * - `fracF|<x>` —— `frac(float)`
 * - `fracD|<x>` —— `frac(double)`
 * - `floorF|<x>` —— `floor(float) → int`
 * - `floorD|<x>` —— `lfloor(double) → long`
 *
 * `floorF` 输出 `int`（32 位），`floorD` 输出 `long`（按**字符串**给出）。
 * `fracF` / `fracD` 按 `%.9g` 格式化后去尾零。
 *
 * TODO: 未覆盖 `Mth.frac` 在坐标插值与粒子偏移中的实际调用点（属渲染/客户端，本项目客户端已停止
 * 维护）。
 */
#include <cmath>
#include <cstdio>
#include <cstdlib>
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

    // 去掉小数部分的尾零，同时处理指数形式（如 1.00000000e+300 -> 1e+300）。
    const std::size_t exponentAt = text.find('e') != std::string::npos ? text.find('e') : text.find('E');
    const std::string mantissa = exponentAt == std::string::npos ? text : text.substr(0, exponentAt);
    const std::string exponent = exponentAt == std::string::npos ? std::string() : text.substr(exponentAt);

    std::string trimmed = mantissa;
    if (trimmed.find('.') != std::string::npos) {
        while (!trimmed.empty() && trimmed.back() == '0') {
            trimmed.pop_back();
        }
        if (!trimmed.empty() && trimmed.back() == '.') {
            trimmed.pop_back();
        }
    }
    return trimmed + exponent;
}

/**
 * Java 的 `(int)` 转换：向零截断，超界饱和。
 *
 * C++ 的 `static_cast<i32>` 对超界是未定义行为，必须显式复刻 Java 的饱和语义。
 */
i32 javaToInt(f32 value)
{
    if (std::isnan(value)) {
        return 0;
    }
    if (value >= 2147483647.0F) {
        return 2147483647;
    }
    if (value <= -2147483648.0F) {
        return -2147483648;
    }
    return static_cast<i32>(value);
}

/// Java 的 `(long)` 转换：向零截断，超界饱和。
i64 javaToLong(f64 value)
{
    if (std::isnan(value)) {
        return 0;
    }
    if (value >= 9223372036854775807.0) {
        return 9223372036854775807LL;
    }
    if (value <= -9223372036854775808.0) {
        return -9223372036854775807LL - 1;
    }
    return static_cast<i64>(value);
}

/// 原版 `Mth.floor(float)`。
i32 mthFloor(f32 value)
{
    const i32 truncated = javaToInt(value);
    return value < static_cast<f32>(truncated) ? truncated - 1 : truncated;
}

/// 原版 `Mth.lfloor(double)`。
i64 mthLfloor(f64 value)
{
    const i64 truncated = javaToLong(value);
    return value < static_cast<f64>(truncated) ? truncated - 1 : truncated;
}

/// 原版 `Mth.frac(float)`：注意 `floor` 返回 `int`。
f32 fracF(f32 value)
{
    return value - static_cast<f32>(mthFloor(value));
}

/// 原版 `Mth.frac(double)`：注意 `lfloor` 返回 `long`。
f64 fracD(f64 value)
{
    return value - static_cast<f64>(mthLfloor(value));
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // fracF：常规与小数值
        "fracF|0",
        "fracF|1.5",
        "fracF|1.25",
        "fracF|0.75",
        "fracF|2",
        // fracF：负数（floor 向负无穷 -> 结果为正）
        "fracF|-1.5",
        "fracF|-1.25",
        "fracF|-2",
        "fracF|-0.25",
        // fracF：大输入（int 饱和 -> 完全失效）
        "fracF|1e10",
        "fracF|3e9",
        "fracF|-3e9",
        // fracF：float 精度阈值（2^24）
        "fracF|16777216",
        "fracF|16777217",
        "fracF|8388608.5",
        // fracD：与 float 版对照
        "fracD|1.5",
        "fracD|-1.5",
        "fracD|1e10",
        "fracD|1e300",
        // floorF / floorD
        "floorF|1.5",
        "floorF|-1.5",
        "floorF|1e10",
        "floorF|-1e10",
        "floorD|1.5",
        "floorD|-1.5",
        "floorD|1e10",
        "floorD|-1e300",
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
    if (parts.size() != 2) {
        throw std::runtime_error("mth_frac: 单元需要 2 段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "fracF") {
        return json{{"unit", unit}, {"value", formatNumber(fracF(std::stof(parts[1])))}}.dump();
    }
    if (mode == "fracD") {
        return json{{"unit", unit}, {"value", formatNumber(fracD(std::stod(parts[1])))}}.dump();
    }
    if (mode == "floorF") {
        return json{{"unit", unit}, {"value", mthFloor(std::stof(parts[1]))}}.dump();
    }
    if (mode == "floorD") {
        return json{{"unit", unit}, {"value", std::to_string(mthLfloor(std::stod(parts[1])))}}.dump();
    }

    throw std::runtime_error("mth_frac: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("mth_frac", &writeList, &runLine)

} // namespace mc::diff
