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
 * @brief 差分能力：`Mth.inverseLerp` 与 `Mth.lerp3`（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.util.Mth` 中 [`mth_lerp`](./mth_lerp.md) 未覆盖的两个函数：
 *
 * ```java
 * public static double inverseLerp(double value, double from, double to) {
 *     return (value - from) / (to - from);
 * }
 *
 * public static float inverseLerp(float value, float from, float to) {
 *     return (value - from) / (to - from);
 * }
 *
 * public static double lerp3(double d1, double d2,
 *                            double d3, double d4, double d5, double d6,
 *                            double d7, double d8, double d9, double d10, double d11) {
 *     return lerp(d2, lerp2(d1, d2, d3, d4, d5, d6), lerp2(d1, d2, d7, d8, d9, d10));
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`inverseLerp` 不做任何 clamp，也不保护 `from == to`。**
 *
 *    ```java
 *    return (value - from) / (to - from);
 *    ```
 *
 *    - `value < from` 时返回**负值**
 *    - `value > to` 时返回**大于 1** 的值
 *    - **`from == to` 时**：`0/0` 得 **`NaN`**（`value == from`）或 `±Infinity`（`value != from`）
 *
 *    与 [`mth_lerp`](./mth_lerp.md) 的 `clampedLerp`（会 clamp 到 `[from, to]`）形成对比——
 *    `inverseLerp` 是「求比例」，不负责夹取。
 *
 * 2. **`inverseLerp` 的分母是 `to - from`，不是 `from - to`。**
 *
 *    反向区间（`from > to`）时分母为负，结果符号也随之翻转：
 *    `inverseLerp(5, 10, 0)` = `(5-10)/(0-10)` = `0.5`。
 *
 * 3. **`lerp3` 的 11 个参数布局：外层权重是第 3 个参数，不是第 1 个或第 2 个。**
 *
 *    ```java
 *    lerp3(d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11)
 *      = lerp(d3,
 *             lerp2(d1, d2, d4, d5, d6, d7),
 *             lerp2(d1, d2, d8, d9, d10, d11))
 *    ```
 *
 *    即：
 *    - `d1`、`d2` 是**外层权重对**（同时作为 `lerp2` 的第一个权重）
 *    - `d3..d6` 是第一个 `lerp2` 的四个值
 *    - `d7..d10` 是第二个 `lerp2` 的四个值
 *    - **`d11` 参数在实现里完全没有被使用**（见下方「未覆盖范围」）
 *
 *    **`d11` 在实现里完全未被使用**——这是历史遗留（早期可能是真三维插值），重构后签名未清理。
 *
 *    源码签名有 11 个 `double`，但函数体只用到前 10 个。这是历史遗留——早期的 `lerp3` 可能是
 *    三维插值需要更多参数，重构后未清理签名。
 *
 *    **Cubium 若实现 `lerp3`，必须同样忽略第 11 个参数**，否则在某些调用点（本来传入的值就
 *    没有意义）会产生差异。
 *
 * 单元格式：
 * - `invD|<value>|<from>|<to>` —— `double` 版 `inverseLerp`
 * - `invF|<value>|<from>|<to>` —— `float` 版 `inverseLerp`
 * - `lerp3|<d1>|<d2>|...|<d11>` —— 11 个竖线分隔的值
 *
 * 浮点按 `%.9g` 格式化后去掉尾随零（`float` 版必须用 `%.9g`，`%.6g` 会掩盖精度差异）。
 * `NaN` 与 `Infinity` 分别输出为字符串 `NaN` / `Infinity` / `-Infinity`。
 *
 * TODO: 未覆盖 `lerp3` 在密度函数与噪声烘焙里的实际调用点（属 world 型）。
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

/// 格式化 `double`：`%.9g` 并去尾零，特殊值单独处理。
std::string formatD(f64 value)
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

/// 格式化 `float`。
std::string formatF(f32 value)
{
    return formatD(static_cast<f64>(value));
}

/// 原版 `Mth.inverseLerp(double, double, double)`。
f64 inverseLerpD(f64 value, f64 from, f64 to)
{
    return (value - from) / (to - from);
}

/// 原版 `Mth.inverseLerp(float, float, float)`。
f32 inverseLerpF(f32 value, f32 from, f32 to)
{
    return (value - from) / (to - from);
}

/// 原版 `Mth.lerp`（前置声明，`lerp2` 需要）。
f64 lerp(f64 delta, f64 start, f64 end);

/// 原版 `Mth.lerp2`（`mth_lerp` 已覆盖，此处为本文件内部依赖）。
f64 lerp2(f64 d0, f64 d1, f64 d2, f64 d3, f64 d4, f64 d5)
{
    return lerp(d1, lerp(d0, d2, d3), lerp(d0, d4, d5));
}

/// 原版 `Mth.lerp`（两参版本）。
f64 lerp(f64 delta, f64 start, f64 end)
{
    return start + delta * (end - start);
}

/// 原版 `Mth.lerp3`：外层权重是**第 3 个**参数 `d3`，第 11 个参数未被使用。
///
/// ```java
/// return lerp(p3, lerp2(p1, p2, p4, p5, p6, p7), lerp2(p1, p2, p8, p9, p10, p11));
/// ```
f64 lerp3(f64 d1, f64 d2, f64 d3, f64 d4, f64 d5, f64 d6, f64 d7, f64 d8, f64 d9, f64 d10, f64 d11)
{
    return lerp(d3, lerp2(d1, d2, d4, d5, d6, d7), lerp2(d1, d2, d8, d9, d10, d11));
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // invD：正常区间
        "invD|5|0|10",
        "invD|0|0|10",
        "invD|10|0|10",
        "invD|2.5|0|10",
        // invD：超出区间（不 clamp）
        "invD|-5|0|10",
        "invD|15|0|10",
        // invD：反向区间
        "invD|5|10|0",
        "invD|2.5|10|0",
        // invD：退化区间 from == to（产生 NaN / Infinity）
        "invD|0|0|0",
        "invD|5|0|0",
        "invD|-5|0|0",
        // invF：float 版
        "invF|5|0|10",
        "invF|1|0|3",
        "invF|2|0|3",
        "invF|0|0|0",
        "invF|5|0|0",
        // lerp3：权重 0 与 1
        "lerp3|0|0|0|0|0|0|10|10|10|10|0",
        "lerp3|0|1|0|0|0|0|10|10|10|10|0",
        "lerp3|1|0|0|0|0|0|10|10|10|10|0",
        "lerp3|1|1|0|0|0|0|10|10|10|10|0",
        // lerp3：d11 应被忽略
        "lerp3|0.5|0.5|0|0|0|0|10|10|10|10|0",
        "lerp3|0.5|0.5|0|0|0|0|10|10|10|10|999",
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

std::vector<f64> splitComma(const std::string& text)
{
    std::vector<f64> values;
    std::string current;
    for (char ch : text) {
        if (ch == ',') {
            values.push_back(std::stod(current));
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    values.push_back(std::stod(current));
    return values;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("mth_inverse_lerp: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "invD" && parts.size() == 4) {
        const f64 result = inverseLerpD(std::stod(parts[1]), std::stod(parts[2]), std::stod(parts[3]));
        return json{{"unit", unit}, {"value", formatD(result)}}.dump();
    }
    if (mode == "invF" && parts.size() == 4) {
        const f32 result = inverseLerpF(static_cast<f32>(std::stod(parts[1])),
            static_cast<f32>(std::stod(parts[2])),
            static_cast<f32>(std::stod(parts[3])));
        return json{{"unit", unit}, {"value", formatF(result)}}.dump();
    }
    if (mode == "lerp3" && parts.size() == 12) {
        f64 values[11];
        for (i32 i = 0; i < 11; ++i) {
            values[i] = std::stod(parts[i + 1]);
        }
        const f64 result = lerp3(values[0],
            values[1],
            values[2],
            values[3],
            values[4],
            values[5],
            values[6],
            values[7],
            values[8],
            values[9],
            values[10]);
        return json{{"unit", unit}, {"value", formatD(result)}}.dump();
    }

    throw std::runtime_error("mth_inverse_lerp: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("mth_inverse_lerp", &writeList, &runLine)

} // namespace mc::diff
