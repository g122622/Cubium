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
 * @brief 差分能力：`Mth.positiveModulo` 与 `Mth.wrapDegrees`（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行
 *         → { "result": "..." }
 *
 * 复刻原版 `Mth` 的四个函数：
 *
 * ```java
 * public static int positiveModulo(int x, int y)      { return Math.floorMod(x, y); }
 * public static float positiveModulo(float x, float y)  { return (x % y + y) % y; }
 * public static double positiveModulo(double x, double y) { return (x % y + y) % y; }
 *
 * public static int wrapDegrees(int deg) {
 *     int i = deg % 360;
 *     if (i >= 180)  { i -= 360; }
 *     if (i < -180)  { i += 360; }
 *     return i;
 * }
 * // float / double 版本结构相同
 * ```
 *
 * ## 三个关键点
 *
 * 1. **`positiveModulo` 的浮点版本不是 `Math.floorMod`**。整数版走 `floorMod`，浮点版是
 *    `(x % y + y) % y`——先取余（符号跟随被除数），加一个周期再取余。两者在**负数被除数**
 *    上都返回非负结果，但浮点版对 `y < 0` 的行为与 `floorMod` 不同（结果符号跟随 `y`）。
 *
 * 2. **`wrapDegrees` 用两个独立 `if`，不是 `while` 也不是 `if/else if`**。由于 `% 360`
 *    的结果严格落在 `(-360, 360)`，两次调整至多命中一次，所以三者在**有限输入**下等价。
 *    但它依赖「`%` 的结果范围」这一前提——若有人把 `%` 换成不减的写法（如直接对超大值
 *    判断），`if` 就会失效。本能力用大幅度的输入把这一前提钉住。
 *
 * 3. **`wrapDegrees` 的区间是 `[-180, 180)`**，不是 `(-180, 180]`。`180` 会被调整成
 *    `-180`，而 `-180` 保持不变——两个端点不对称。
 *
 * 单元格式：
 * - `modi|x|y`     —— 整数 positiveModulo
 * - `modf|x|y`     —— 单精度 positiveModulo
 * - `modd|x|y`     —— 双精度 positiveModulo
 * - `wrap|x`       —— 双精度 wrapDegrees（并同时输出整数与单精度版本便于对照）
 *
 * TODO: Cubium 的 `MathUtils::wrapDegrees` 用 `if / else if`（原版是两个独立 `if`），
 * 在有限输入下等价——本能力会把这个等价性验证出来。Cubium 目前没有 `positiveModulo`，
 * 那部分是待实现的行为基准。
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

std::string formatFloat(f64 value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.6g", value);
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
 * @brief 原版 `Math.floorMod`：结果的符号跟随除数。
 */
i32 floorMod(i32 x, i32 y)
{
    const i32 r = x % y;
    return (r != 0 && ((r < 0) != (y < 0))) ? r + y : r;
}

/**
 * @brief 原版 `Mth.positiveModulo(float, float)`：`(x % y + y) % y`。
 */
f32 positiveModuloF(f32 x, f32 y)
{
    return std::fmod(std::fmod(x, y) + y, y);
}

/**
 * @brief 原版 `Mth.positiveModulo(double, double)`。
 */
f64 positiveModuloD(f64 x, f64 y)
{
    return std::fmod(std::fmod(x, y) + y, y);
}

/**
 * @brief 原版 `Mth.wrapDegrees(int)`：两个独立 `if`。
 */
i32 wrapDegreesI(i32 degrees)
{
    i32 result = degrees % 360;
    if (result >= 180) {
        result -= 360;
    }
    if (result < -180) {
        result += 360;
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

/**
 * @brief 原版 `Mth.wrapDegrees(double)`。
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

/// 单元表。
constexpr const char* UNITS[] = {
    // positiveModulo 整数：正、负被除数
    "modi|7|3",
    "modi|-7|3",
    "modi|7|-3",
    "modi|-7|-3",
    "modi|0|5",
    "modi|-1|5",
    // positiveModulo 整数：整除时余数为 0（不能变成 y）
    "modi|-6|3",
    "modi|6|3",
    // positiveModulo 浮点
    "modf|7.5|3",
    "modf|-7.5|3",
    "modf|-1|5",
    "modf|0|5",
    "modd|7.5|3",
    "modd|-7.5|3",
    "modd|-1|5",
    "modd|0|5",
    // wrapDegrees：区间边界
    "wrap|0",
    "wrap|180",
    "wrap|-180",
    "wrap|179.9",
    "wrap|-179.9",
    "wrap|359",
    "wrap|-359",
    // 超过一个周期（检验 `%` 的范围前提）
    "wrap|720",
    "wrap|-720",
    "wrap|900",
    "wrap|-900",
    "wrap|1000000",
    "wrap|-1000000",
    // 小数
    "wrap|45.5",
    "wrap|-45.5",
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
    if (parts.empty()) {
        throw std::runtime_error("mth_modulo: 空单元");
    }

    if (parts[0] == "modi" && parts.size() == 3) {
        const i32 x = std::stoi(parts[1]);
        const i32 y = std::stoi(parts[2]);
        return json{{"unit", unit}, {"mode", "modi"}, {"result", std::to_string(floorMod(x, y))}}.dump();
    }
    if (parts[0] == "modf" && parts.size() == 3) {
        const f32 x = static_cast<f32>(std::stod(parts[1]));
        const f32 y = static_cast<f32>(std::stod(parts[2]));
        return json{{"unit", unit}, {"mode", "modf"}, {"result", formatFloat(static_cast<f64>(positiveModuloF(x, y)))}}
            .dump();
    }
    if (parts[0] == "modd" && parts.size() == 3) {
        const f64 x = std::stod(parts[1]);
        const f64 y = std::stod(parts[2]);
        return json{{"unit", unit}, {"mode", "modd"}, {"result", formatFloat(positiveModuloD(x, y))}}.dump();
    }
    if (parts[0] == "wrap" && parts.size() == 2) {
        const f64 x = std::stod(parts[1]);
        const f32 xf = static_cast<f32>(x);
        const i32 xi = static_cast<i32>(x);
        return json{{"unit", unit},
            {"mode", "wrap"},
            {"resultD", formatFloat(wrapDegreesD(x))},
            {"resultF", formatFloat(static_cast<f64>(wrapDegreesF(xf)))},
            {"resultI", wrapDegreesI(xi)}}
            .dump();
    }

    throw std::runtime_error("mth_modulo: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(mth_modulo, &writeList, &runLine)

} // namespace mc::diff
