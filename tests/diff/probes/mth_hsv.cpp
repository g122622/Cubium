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
 * @brief 差分能力：`Mth` 的 HSV → ARGB 颜色转换（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.util.Mth`：
 *
 * ```java
 * public static int hsvToRgb(float hue, float saturation, float value) {
 *     return hsvToArgb(hue, saturation, value, 0);
 * }
 *
 * public static int hsvToArgb(float hue, float sat, float val, int alpha) {
 *     int i = (int)(hue * 6.0F) % 6;
 *     float f  = hue * 6.0F - i;
 *     float f1 = val * (1.0F - sat);
 *     float f2 = val * (1.0F - f * sat);
 *     float f3 = val * (1.0F - (1.0F - f) * sat);
 *     float r, g, b;
 *     switch (i) {
 *         case 0 -> { r = val; g = f3;   b = f1; }
 *         case 1 -> { r = f2;  g = val;  b = f1; }
 *         case 2 -> { r = f1;  g = val;  b = f3; }
 *         case 3 -> { r = f1;  g = f2;   b = val; }
 *         case 4 -> { r = f3;  g = f1;   b = val; }
 *         case 5 -> { r = val; g = f1;   b = f2; }
 *         default -> throw new RuntimeException(...);
 *     }
 *     return ARGB.color(alpha,
 *         clamp((int)(r * 255.0F), 0, 255),
 *         clamp((int)(g * 255.0F), 0, 255),
 *         clamp((int)(b * 255.0F), 0, 255));
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`hue = 1.0` 回绕到红色段（`i = 0`），不是 `i = 6`。**
 *
 *    ```java
 *    int i = (int)(hue * 6.0F) % 6;
 *    ```
 *
 *    `1.0 * 6 = 6`，`(int)6 = 6`，`6 % 6 = 0` → 走 `case 0`，得到**纯红色**。
 *
 *    `hue = 0.0` 也是 `i = 0` 纯红。**所以 `hue` 是循环的，`0` 与 `1` 等价。**
 *
 * 2. **`(int)` 是向零截断，`hue` 落在分段边界时偏向低段。**
 *
 *    | `hue` | `hue * 6` | `(int)` | `% 6` | 段 |
 *    |---|---|---|---|---|
 *    | `0.0` | `0` | 0 | 0 | 红 |
 *    | `1/6 ≈ 0.1667` | `1` | 1 | 1 | 黄 |
 *    | `2/6 ≈ 0.3333` | `2` | 2 | 2 | 绿 |
 *    | `0.5` | `3` | 3 | 3 | 青 |
 *    | `4/6 ≈ 0.6667` | `4` | 4 | 4 | 蓝 |
 *    | `5/6 ≈ 0.8333` | `5` | 5 | 5 | 品红 |
 *    | **`1.0`** | **`6`** | **6** | **0** | **红（回绕）** |
 *
 *    `hue = 1/6` 时 `f = 1.0 - 1 = 0`，所以该点正好是纯黄色。
 *
 * 3. **每个分量是 `(int)(f * 255.0F)` 截断，不是四舍五入。**
 *
 *    `0.5 * 255 = 127.5`，`(int)` 得 **`127`**，而不是 `128`。
 *
 *    `hsvToRgb(0, 0, 0.5)`（灰 50%）→ 三个分量都是 `127` → `0x007F7F7F`（alpha = 0）。
 *
 * 4. **`hsvToRgb` 的 alpha 恒为 0，不是 255。**
 *
 *    ```java
 *    return hsvToArgb(hue, sat, val, 0);
 *    ```
 *
 *    结果形如 `0x00RRGGBB`——**高位是 0**。调用方若要用于渲染通常需要自己补 alpha。
 *
 *    `ARGB.color(a, r, g, b)` 的布局是 `a << 24 | r << 16 | g << 8 | b`。
 *
 * 单元格式：
 * - `rgb|<hue>|<sat>|<val>` —— `hsvToRgb`（alpha 0）
 * - `argb|<hue>|<sat>|<val>|<alpha>` —— `hsvToArgb`
 *
 * 输出是**有符号 32 位整数**（与 Java `int` 一致）。当 alpha 的最高位为 1 时结果为负数。
 *
 * TODO: 未覆盖 `Mth.atan2`（依赖 257 项 `ASIN_TAB` / `COS_TAB` 与 `fastInvSqrt`，
 * 且这些表由 `Math.sin` 填充，存在 JDK 与 MSVC 的 sin 差异风险）、`Mth.hsvToArgb` 的
 * 非有限输入（`NaN` / `Infinity`）。
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

/// 原版 `Mth.clamp(int, int, int)`。
i32 clampI32(i32 value, i32 min, i32 max)
{
    if (value < min) {
        return min;
    }
    return value > max ? max : value;
}

/// 原版 `ARGB.color(int, int, int, int)`：`a<<24 | r<<16 | g<<8 | b`。
i32 argbColor(i32 alpha, i32 red, i32 green, i32 blue)
{
    return static_cast<i32>((static_cast<u32>(alpha & 0xFF) << 24) | (static_cast<u32>(red & 0xFF) << 16) |
        (static_cast<u32>(green & 0xFF) << 8) | static_cast<u32>(blue & 0xFF));
}

/// 原版 `Mth.hsvToArgb`。
i32 hsvToArgb(f32 hue, f32 saturation, f32 value, i32 alpha)
{
    const i32 i = static_cast<i32>(hue * 6.0F) % 6;
    const f32 f = hue * 6.0F - static_cast<f32>(i);
    const f32 f1 = value * (1.0F - saturation);
    const f32 f2 = value * (1.0F - f * saturation);
    const f32 f3 = value * (1.0F - (1.0F - f) * saturation);

    f32 r = 0.0F;
    f32 g = 0.0F;
    f32 b = 0.0F;
    switch (i) {
        case 0:
            r = value;
            g = f3;
            b = f1;
            break;
        case 1:
            r = f2;
            g = value;
            b = f1;
            break;
        case 2:
            r = f1;
            g = value;
            b = f3;
            break;
        case 3:
            r = f1;
            g = f2;
            b = value;
            break;
        case 4:
            r = f3;
            g = f1;
            b = value;
            break;
        case 5:
            r = value;
            g = f1;
            b = f2;
            break;
        default:
            throw std::runtime_error("mth_hsv: HSV 段索引越界：" + std::to_string(i));
    }

    return argbColor(alpha,
        clampI32(static_cast<i32>(r * 255.0F), 0, 255),
        clampI32(static_cast<i32>(g * 255.0F), 0, 255),
        clampI32(static_cast<i32>(b * 255.0F), 0, 255));
}

/// 原版 `Mth.hsvToRgb`。
i32 hsvToRgb(f32 hue, f32 saturation, f32 value)
{
    return hsvToArgb(hue, saturation, value, 0);
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // rgb：六个纯色段
        "rgb|0|1|1",
        "rgb|0.1666667|1|1",
        "rgb|0.3333333|1|1",
        "rgb|0.5|1|1",
        "rgb|0.6666667|1|1",
        "rgb|0.8333333|1|1",
        // rgb：hue = 1.0 回绕
        "rgb|1|1|1",
        // rgb：饱和度与明度为 0
        "rgb|0|0|1",
        "rgb|0|0|0.5",
        "rgb|0|0|0",
        // rgb：中间色
        "rgb|0|0.5|1",
        "rgb|0.25|1|1",
        "rgb|0.75|1|1",
        // argb：非零 alpha
        "argb|0|1|1|255",
        "argb|0|1|1|128",
        "argb|0|0|0|0",
        "argb|0.5|1|1|255",
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
        throw std::runtime_error("mth_hsv: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "rgb" && parts.size() == 4) {
        const i32 result = hsvToRgb(parseF(parts[1]), parseF(parts[2]), parseF(parts[3]));
        return json{{"unit", unit}, {"value", result}}.dump();
    }
    if (mode == "argb" && parts.size() == 5) {
        const i32 result = hsvToArgb(parseF(parts[1]), parseF(parts[2]), parseF(parts[3]), std::stoi(parts[4]));
        return json{{"unit", unit}, {"value", result}}.dump();
    }

    throw std::runtime_error("mth_hsv: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(mth_hsv, &writeList, &runLine)

} // namespace mc::diff
