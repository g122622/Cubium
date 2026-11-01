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
 * @brief 差分能力：`Mth.hsvToArgb` 与 `Mth.murmurHash3Mixer`（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版：
 *
 * ```java
 * public static int hsvToRgb(float h, float s, float v) {
 *     return hsvToArgb(h, s, v, 0);
 * }
 *
 * public static int hsvToArgb(float h, float s, float v, int alpha) {
 *     int i = (int)(h * 6.0F) % 6;
 *     float f  = h * 6.0F - i;
 *     float f1 = v * (1.0F - s);
 *     float f2 = v * (1.0F - f * s);
 *     float f3 = v * (1.0F - (1.0F - f) * s);
 *     float r, g, b;
 *     switch (i) {
 *         case 0: r = v;  g = f3; b = f1; break;
 *         case 1: r = f2; g = v;  b = f1; break;
 *         case 2: r = f1; g = v;  b = f3; break;
 *         case 3: r = f1; g = f2; b = v;  break;
 *         case 4: r = f3; g = f1; b = v;  break;
 *         case 5: r = v;  g = f1; b = f2; break;
 *         default: throw new RuntimeException(...);
 *     }
 *     return ARGB.color(alpha, clamp((int)(r * 255.0F), 0, 255),
 *                              clamp((int)(g * 255.0F), 0, 255),
 *                              clamp((int)(b * 255.0F), 0, 255));
 * }
 *
 * // ARGB.color(a, r, g, b) = (a & 0xFF) << 24 | (r & 0xFF) << 16 | (g & 0xFF) << 8 | (b & 0xFF)
 *
 * public static int murmurHash3Mixer(int value) {
 *     value ^= value >>> 16;
 *     value *= -2048144789;
 *     value ^= value >>> 13;
 *     value *= -1028477387;
 *     return value ^ value >>> 16;
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **色相区间用 `(int)(h * 6.0F) % 6`，而不是 `floor`。** 对正数 `h`，`(int)` 是截断，与
 *    `floor` 一致；但对**负数** `h`，`(int)` 向零截断，`%` 在 Java 中也返回负数，结果是
 *    **负区间**，会落到 `default` 分支**抛异常**。`hsvToArgb(-0.1f, 1, 1)` 不是把色相环绕，
 *    而是直接抛 `RuntimeException`。
 *
 * 2. **`h == 1.0F` 不是纯红，而是黄色。** `(int)(1.0 * 6) % 6 = 6 % 6 = 0`，落到区间 0，
 *    但 `f = 1.0 * 6.0 - 0 = 6.0`（不是 0）。于是
 *    `f3 = v * (1 - (1 - f) * s) = 1 * (1 - (1 - 6) * 1) = 6.0`，
 *    `g = 6.0 * 255 = 1530`，被 `clamp(..., 0, 255)` 钳成 **255**。
 *    实测 `hsvToArgb(1, 1, 1)` 得 `r=255, g=255, b=0`。
 *
 *    **色相 `1.0` 与 `0.0` 并不等价。** 我先按「`f` 是小数部分」的直觉断言结果是纯红，
 *    实测证明错了——`h*6 = 6` 被折回 `i = 0`，但 `f` 仍是 `6` 而不是 `0`。
 *
 * 3. **每个通道都先 `(int)(x * 255.0F)` 截断，再 `clamp(0, 255)`。** 截断在前意味着
 *    `0.999 × 255 = 254.745` 得 **254** 而不是 255。饱和度或明度略小于 1 时颜色会偏暗一档。
 *    `clamp` 兜住浮点误差导致的越界（如 `v = 1.0F` 时 `v * 255.0F` 恰好是 255.0，安全）。
 *
 * 4. **`murmurHash3Mixer` 用 `>>>`（逻辑右移），不是 `>>`。** 它对负数 `value` 至关重要：
 *    算术右移会把符号位带下来，散布特性完全破坏。两个乘数都是负数（`-2048144789`、
 *    `-1028477387`），在 32 位下溢出环绕。
 *
 * 单元格式：`<模式>|<参数…>`
 * - `hsv|<h>|<s>|<v>` —— 输出 RGBA 打包值与四个通道
 * - `hsva|<h>|<s>|<v>|<alpha>` —— 同上，alpha 指定
 * - `murmur|<value>` —— 输出混合后的 `int`
 *
 * TODO: 未覆盖 `ARGB.alpha/red/green/blue` 的反解（结构简单，本能力的输出已隐含验证）。
 */
#include <algorithm>
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

/// 原版 `Mth.clamp(int, int, int)`。
i32 clampInt(i32 value, i32 min, i32 max)
{
    return value < min ? min : std::min(value, max);
}

/// 原版 `ARGB.color(a, r, g, b)`：各通道先 `& 0xFF`。
i32 argbColor(i32 alpha, i32 red, i32 green, i32 blue)
{
    return ((alpha & 0xFF) << 24) | ((red & 0xFF) << 16) | ((green & 0xFF) << 8) | (blue & 0xFF);
}

/**
 * @brief 原版 `Mth.hsvToArgb`。
 *
 * @throws 当色相区间落在 `0..5` 之外（负 `h` 会出现）。
 */
i32 hsvToArgb(f32 h, f32 s, f32 v, i32 alpha)
{
    const i32 i = static_cast<i32>(h * 6.0f) % 6;
    const f32 f = h * 6.0f - static_cast<f32>(i);
    const f32 f1 = v * (1.0f - s);
    const f32 f2 = v * (1.0f - f * s);
    const f32 f3 = v * (1.0f - (1.0f - f) * s);

    f32 r = 0.0f;
    f32 g = 0.0f;
    f32 b = 0.0f;
    switch (i) {
        case 0:
            r = v;
            g = f3;
            b = f1;
            break;
        case 1:
            r = f2;
            g = v;
            b = f1;
            break;
        case 2:
            r = f1;
            g = v;
            b = f3;
            break;
        case 3:
            r = f1;
            g = f2;
            b = v;
            break;
        case 4:
            r = f3;
            g = f1;
            b = v;
            break;
        case 5:
            r = v;
            g = f1;
            b = f2;
            break;
        default:
            throw std::runtime_error("mth_color: HSV 转 RGB 失败，输入为 " + std::to_string(h) + ", " +
                std::to_string(s) + ", " + std::to_string(v));
    }

    return argbColor(alpha,
        clampInt(static_cast<i32>(r * 255.0f), 0, 255),
        clampInt(static_cast<i32>(g * 255.0f), 0, 255),
        clampInt(static_cast<i32>(b * 255.0f), 0, 255));
}

/// 原版 `Mth.hsvToRgb`。
i32 hsvToRgb(f32 h, f32 s, f32 v)
{
    return hsvToArgb(h, s, v, 0);
}

/// 原版 `Mth.murmurHash3Mixer`：`>>>` 是逻辑右移，乘数溢出环绕。
i32 murmurHash3Mixer(i32 value)
{
    u32 v = static_cast<u32>(value);
    v ^= v >> 16;
    v *= static_cast<u32>(-2048144789);
    v ^= v >> 13;
    v *= static_cast<u32>(-1028477387);
    v ^= v >> 16;
    return static_cast<i32>(v);
}

/// 从打包值取通道。
i32 channelAlpha(i32 argb)
{
    return static_cast<i32>(static_cast<u32>(argb) >> 24);
}

i32 channelRed(i32 argb)
{
    return (argb >> 16) & 0xFF;
}

i32 channelGreen(i32 argb)
{
    return (argb >> 8) & 0xFF;
}

i32 channelBlue(i32 argb)
{
    return argb & 0xFF;
}

/// 单元表。
constexpr const char* UNITS[] = {
    // HSV：六个色相区间
    "hsv|0|1|1",
    "hsv|0.166667|1|1",
    "hsv|0.333333|1|1",
    "hsv|0.5|1|1",
    "hsv|0.666667|1|1",
    "hsv|0.833333|1|1",
    // HSV：边界 h = 1（折回区间 0）
    "hsv|1|1|1",
    // HSV：饱和度 / 明度退化
    "hsv|0|0|1",
    "hsv|0|1|0",
    "hsv|0|0|0",
    "hsv|0.5|0|0.5",
    // HSV：中间调
    "hsv|0.25|0.5|0.8",
    "hsv|0.75|0.3|0.6",
    // HSV：截断而非四舍五入
    "hsv|0|1|0.999",
    "hsv|0|0.999|1",
    // 指定 alpha
    "hsva|0|1|1|255",
    "hsva|0.333333|1|1|128",
    "hsva|0.5|1|1|0",
    // murmur3：常规值
    "murmur|0",
    "murmur|1",
    "murmur|-1",
    "murmur|42",
    // murmur3：负数与边界
    "murmur|-2147483648",
    "murmur|2147483647",
    // murmur3：递增序列（验证扩散性）
    "murmur|100",
    "murmur|101",
    "murmur|102",
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
        throw std::runtime_error("mth_color: 单元至少两段：" + unit);
    }

    if (parts[0] == "murmur" && parts.size() == 2) {
        const i32 value = static_cast<i32>(std::stoll(parts[1]));
        return json{{"unit", unit}, {"value", murmurHash3Mixer(value)}}.dump();
    }

    if (parts[0] == "hsv" && parts.size() == 4) {
        const f32 h = static_cast<f32>(std::stod(parts[1]));
        const f32 s = static_cast<f32>(std::stod(parts[2]));
        const f32 v = static_cast<f32>(std::stod(parts[3]));
        const i32 argb = hsvToRgb(h, s, v);
        return json{{"unit", unit},
            {"value", argb},
            {"a", channelAlpha(argb)},
            {"r", channelRed(argb)},
            {"g", channelGreen(argb)},
            {"b", channelBlue(argb)}}
            .dump();
    }

    if (parts[0] == "hsva" && parts.size() == 5) {
        const f32 h = static_cast<f32>(std::stod(parts[1]));
        const f32 s = static_cast<f32>(std::stod(parts[2]));
        const f32 v = static_cast<f32>(std::stod(parts[3]));
        const i32 alpha = std::stoi(parts[4]);
        const i32 argb = hsvToArgb(h, s, v, alpha);
        return json{{"unit", unit},
            {"value", argb},
            {"a", channelAlpha(argb)},
            {"r", channelRed(argb)},
            {"g", channelGreen(argb)},
            {"b", channelBlue(argb)}}
            .dump();
    }

    throw std::runtime_error("mth_color: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("mth_color", &writeList, &runLine)

} // namespace mc::diff
