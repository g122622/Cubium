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
 * @brief 差分能力：`ARGB` 的通道提取、打包与混合（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.util.ARGB`：
 *
 * ```java
 * public static int alpha(int c) { return c >>> 24; }        // 无符号右移
 * public static int red(int c)   { return c >> 16 & 0xFF; }  // 有符号右移
 * public static int green(int c) { return c >> 8 & 0xFF; }
 * public static int blue(int c)  { return c & 0xFF; }
 *
 * public static int color(int a, int r, int g, int b) {
 *     return (a & 0xFF) << 24 | (r & 0xFF) << 16 | (g & 0xFF) << 8 | b & 0xFF;
 * }
 * public static int color(int r, int g, int b) { return color(255, r, g, b); }
 *
 * public static int multiply(int a, int b) {
 *     if (a == -1) return b;
 *     if (b == -1) return a;
 *     return color(alpha(a)*alpha(b)/255, red(a)*red(b)/255,
 *                  green(a)*green(b)/255, blue(a)*blue(b)/255);
 * }
 *
 * public static int greyscale(int c) {
 *     int i = (int)(red(c)*0.3F + green(c)*0.59F + blue(c)*0.11F);
 *     return color(alpha(c), i, i, i);
 * }
 *
 * public static int addRgb(int a, int b) {
 *     return color(alpha(a), Math.min(red(a)+red(b), 255),
 *                  Math.min(green(a)+green(b), 255), Math.min(blue(a)+blue(b), 255));
 * }
 *
 * public static int scaleRGB(int c, float s) { return scaleRGB(c, s, s, s); }
 * public static int scaleRGB(int c, float sr, float sg, float sb) {
 *     return color(alpha(c),
 *         Math.clamp((long)((int)(red(c)*sr)), 0, 255),
 *         Math.clamp((long)((int)(green(c)*sg)), 0, 255),
 *         Math.clamp((long)((int)(blue(c)*sb)), 0, 255));
 * }
 * public static int scaleRGB(int c, int s) {
 *     return color(alpha(c),
 *         Math.clamp((long)red(c)*s/255L, 0, 255), ...);
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`alpha` 用 `>>>`（无符号右移），其余通道用 `>>`（有符号右移）。**
 *
 *    ```java
 *    alpha(c)  = c >>> 24        // 0..255
 *    red(c)    = c >> 16 & 0xFF  // 0..255
 *    ```
 *
 *    `alpha` 用无符号右移，**负数颜色（alpha >= 128）也能得到正确的 0..255**；若写成
 *    `c >> 24` 会得到负数。其余通道因为后面有 `& 0xFF`，`>>` 与 `>>>` 结果相同。
 *
 * 2. **`color(int,int,int)` 三参版本的 alpha 是 255，不是 0。**
 *
 *    这与 [`mth_hsv`](./mth_hsv.md) 的 `hsvToRgb`（alpha 恒 0）**相反**。
 *    `ARGB.color(r,g,b)` 给不透明的颜色，`Mth.hsvToRgb` 给全透明的颜色。
 *
 * 3. **`multiply` 对 `-1`（即 `0xFFFFFFFF`，纯白不透明）有短路。**
 *
 *    ```java
 *    if (a == -1) return b;
 *    if (b == -1) return a;
 *    ```
 *
 *    短路**不影响数值结果**（`x * 255 / 255 == x`），但回避了逐通道除法。
 *
 *    **注意 `multiply` 会让颜色变暗**：两个半透明色相乘，alpha 与各通道都会缩小。
 *
 * 4. **`scaleRGB` 的 `float` 版与 `int` 版算法不同，结果可能相差 1。**
 *
 *    | 重载 | 算法 |
 *    |---|---|
 *    | `scaleRGB(c, float s)` | `(int)(channel * s)` 截断后再 clamp |
 *    | `scaleRGB(c, int s)` | `(long)channel * s / 255L` 整除后再 clamp |
 *
 *    `scaleRGB(c, 0.5F)` 与 `scaleRGB(c, 127)` 在 `channel = 255` 时：
 *    - float 版：`(int)(255 * 0.5) = 127`
 *    - int 版：`255 * 127 / 255 = 127`
 *
 *    两者相同，但 `channel = 200, s = 0.5F` 时：float 版 `100`，int 版 `200*127/255 = 99`。
 *    **相差 1。**
 *
 * 单元格式（颜色是有符号 32 位整数，可用 `0x` 前缀的十六进制）：
 * - `ch|<color>` —— 输出 `alpha,red,green,blue`
 * - `pack|<a>|<r>|<g>|<b>` / `pack3|<r>|<g>|<b>`
 * - `mul|<a>|<b>` —— `multiply`
 * - `grey|<c>` —— `greyscale`
 * - `add|<a>|<b>` —— `addRgb`
 * - `scaleF|<c>|<s>` —— `scaleRGB(int, float)`
 * - `scaleF3|<c>|<sr>|<sg>|<sb>` —— `scaleRGB(int, float, float, float)`
 * - `scaleI|<c>|<s>` —— `scaleRGB(int, int)`
 * - `mulAlpha|<c>|<s>` —— `multiplyAlpha`
 * - `blend|<a>|<b>` —— `alphaBlend`
 *
 * 颜色值支持 `0x` 前缀的十六进制输入，便于书写带 alpha 的颜色。
 * 输出统一为**有符号十进制**。
 *
 * TODO: 未覆盖 `srgbToLinearChannel` / `linearToSrgbChannel`（涉及 sRGB 伽马曲线）、
 * `meanLinear`、`srgbLerp`、`subtractRgb`、`color(Vec3)`、`toVector4`。
 */
#include <algorithm>
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

/// 解析颜色：支持十进制与 `0x` 十六进制。
i32 parseColor(const std::string& text)
{
    if (text.size() > 2 && (text[0] == '0') && (text[1] == 'x' || text[1] == 'X')) {
        return static_cast<i32>(std::stoull(text.substr(2), nullptr, 16));
    }
    return static_cast<i32>(std::stoll(text));
}

/// 原版 `ARGB.alpha`：`c >>> 24`。
i32 alpha(i32 color)
{
    return static_cast<i32>(static_cast<u32>(color) >> 24);
}

/// 原版 `ARGB.red`：`c >> 16 & 0xFF`。
i32 red(i32 color)
{
    return (color >> 16) & 0xFF;
}

/// 原版 `ARGB.green`：`c >> 8 & 0xFF`。
i32 green(i32 color)
{
    return (color >> 8) & 0xFF;
}

/// 原版 `ARGB.blue`：`c & 0xFF`。
i32 blue(i32 color)
{
    return color & 0xFF;
}

/// 原版 `ARGB.color(a, r, g, b)`。
i32 pack(i32 a, i32 r, i32 g, i32 b)
{
    return static_cast<i32>((static_cast<u32>(a & 0xFF) << 24) | (static_cast<u32>(r & 0xFF) << 16) |
        (static_cast<u32>(g & 0xFF) << 8) | static_cast<u32>(b & 0xFF));
}

/// 原版 `ARGB.multiply`。
i32 multiply(i32 first, i32 second)
{
    if (first == -1) {
        return second;
    }
    if (second == -1) {
        return first;
    }
    return pack(alpha(first) * alpha(second) / 255,
        red(first) * red(second) / 255,
        green(first) * green(second) / 255,
        blue(first) * blue(second) / 255);
}

/// 原版 `ARGB.greyscale`。
i32 greyscale(i32 color)
{
    const i32 grey = static_cast<i32>(static_cast<f32>(red(color)) * 0.3F + static_cast<f32>(green(color)) * 0.59F +
        static_cast<f32>(blue(color)) * 0.11F);
    return pack(alpha(color), grey, grey, grey);
}

/// 原版 `ARGB.addRgb`。
i32 addRgb(i32 first, i32 second)
{
    return pack(alpha(first),
        std::min(red(first) + red(second), 255),
        std::min(green(first) + green(second), 255),
        std::min(blue(first) + blue(second), 255));
}

/// 原版 `ARGB.scaleRGB(int, float, float, float)`。
i32 scaleRgbFloat(i32 color, f32 sr, f32 sg, f32 sb)
{
    const i32 r = static_cast<i32>(static_cast<f32>(red(color)) * sr);
    const i32 g = static_cast<i32>(static_cast<f32>(green(color)) * sg);
    const i32 b = static_cast<i32>(static_cast<f32>(blue(color)) * sb);
    return pack(alpha(color), std::clamp(r, 0, 255), std::clamp(g, 0, 255), std::clamp(b, 0, 255));
}

/// 原版 `ARGB.scaleRGB(int, int)`：注意是 `long` 乘法后整除 255。
i32 scaleRgbInt(i32 color, i32 scale)
{
    const auto r = static_cast<i64>(red(color)) * scale / 255LL;
    const auto g = static_cast<i64>(green(color)) * scale / 255LL;
    const auto b = static_cast<i64>(blue(color)) * scale / 255LL;
    return pack(alpha(color),
        static_cast<i32>(std::clamp<i64>(r, 0, 255)),
        static_cast<i32>(std::clamp<i64>(g, 0, 255)),
        static_cast<i32>(std::clamp<i64>(b, 0, 255)));
}

/// 原版 `ARGB.multiplyAlpha`。
i32 multiplyAlpha(i32 color, f32 scale)
{
    if (color == 0 || scale <= 0.0F) {
        return 0;
    }
    if (scale >= 1.0F) {
        return color;
    }
    const i32 newAlpha = static_cast<i32>(static_cast<f32>(alpha(color)) * scale);
    // 原版 ARGB.color(float alpha, int rgb)：用新 alpha 替换，RGB 保持不变。
    return pack(newAlpha, red(color), green(color), blue(color));
}

/// 原版 `ARGB.alphaBlendChannel`：`(overlay*oAlpha + base*(outAlpha-oAlpha)) / outAlpha`。
i32 alphaBlendChannel(i32 outAlpha, i32 overlayAlpha, i32 baseChannel, i32 overlayChannel)
{
    if (outAlpha == 0) {
        return 0;
    }
    return (overlayChannel * overlayAlpha + baseChannel * (outAlpha - overlayAlpha)) / outAlpha;
}

/// 原版 `ARGB.alphaBlend`。
i32 alphaBlend(i32 base, i32 overlay)
{
    const i32 baseAlpha = alpha(base);
    const i32 overlayAlpha = alpha(overlay);
    if (overlayAlpha == 255) {
        return overlay;
    }
    if (overlayAlpha == 0) {
        return base;
    }
    const i32 outAlpha = overlayAlpha + baseAlpha * (255 - overlayAlpha) / 255;
    return pack(outAlpha,
        alphaBlendChannel(outAlpha, overlayAlpha, red(base), red(overlay)),
        alphaBlendChannel(outAlpha, overlayAlpha, green(base), green(overlay)),
        alphaBlendChannel(outAlpha, overlayAlpha, blue(base), blue(overlay)));
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // ch：通道提取
        "ch|0x00000000",
        "ch|0xFFFFFFFF",
        "ch|0xFF0000",
        "ch|0x00FF00",
        "ch|0x0000FF",
        "ch|0x80FF8040",
        "ch|-1",
        "ch|-65536",
        // pack
        "pack|255|255|0|0",
        "pack|0|255|0|0",
        "pack|128|0|0|0",
        "pack3|255|0|0",
        "pack3|0|0|255",
        // mul
        "mul|0xFFFFFFFF|0x8000FF00",
        "mul|0x8000FF00|0xFFFFFFFF",
        "mul|0xFFFFFFFF|0xFFFFFFFF",
        "mul|0x80808080|0x80808080",
        // grey
        "grey|0x00FF0000",
        "grey|0x0000FF00",
        "grey|0x000000FF",
        "grey|0xFFFFFFFF",
        "grey|0x00123456",
        // add
        "add|0x00FF0000|0x0000FF00",
        "add|0x00FF0000|0x00FF0000",
        "add|0x80000000|0x00123456",
        // scaleF / scaleI
        "scaleF|0x00FF8040|0.5",
        "scaleF|0x00FF8040|2",
        "scaleF3|0x00FF8040|1|0.5|0",
        "scaleI|0x00FF8040|127",
        "scaleI|0x00FF8040|255",
        "scaleI|0x00FF8040|0",
        // mulAlpha
        "mulAlpha|0xFF0000FF|0.5",
        "mulAlpha|0xFF0000FF|0",
        "mulAlpha|0xFF0000FF|1",
        // blend
        "blend|0x00333333|0x80FF0000",
        "blend|0x00333333|0xFFFFFFFF",
        "blend|0x00333333|0x00FFFFFF",
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
        throw std::runtime_error("argb_ops: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "ch" && parts.size() == 2) {
        const i32 color = parseColor(parts[1]);
        const std::string value = std::to_string(alpha(color)) + "," + std::to_string(red(color)) + "," +
            std::to_string(green(color)) + "," + std::to_string(blue(color));
        return json{{"unit", unit}, {"value", value}}.dump();
    }
    if (mode == "pack" && parts.size() == 5) {
        return json{{"unit", unit},
            {"value", pack(std::stoi(parts[1]), std::stoi(parts[2]), std::stoi(parts[3]), std::stoi(parts[4]))}}
            .dump();
    }
    if (mode == "pack3" && parts.size() == 4) {
        return json{{"unit", unit}, {"value", pack(255, std::stoi(parts[1]), std::stoi(parts[2]), std::stoi(parts[3]))}}
            .dump();
    }
    if (mode == "mul" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", multiply(parseColor(parts[1]), parseColor(parts[2]))}}.dump();
    }
    if (mode == "grey" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", greyscale(parseColor(parts[1]))}}.dump();
    }
    if (mode == "add" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", addRgb(parseColor(parts[1]), parseColor(parts[2]))}}.dump();
    }
    if (mode == "scaleF" && parts.size() == 3) {
        const f32 scale = parseF(parts[2]);
        return json{{"unit", unit}, {"value", scaleRgbFloat(parseColor(parts[1]), scale, scale, scale)}}.dump();
    }
    if (mode == "scaleF3" && parts.size() == 5) {
        return json{{"unit", unit},
            {"value", scaleRgbFloat(parseColor(parts[1]), parseF(parts[2]), parseF(parts[3]), parseF(parts[4]))}}
            .dump();
    }
    if (mode == "scaleI" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", scaleRgbInt(parseColor(parts[1]), std::stoi(parts[2]))}}.dump();
    }
    if (mode == "mulAlpha" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", multiplyAlpha(parseColor(parts[1]), parseF(parts[2]))}}.dump();
    }
    if (mode == "blend" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", alphaBlend(parseColor(parts[1]), parseColor(parts[2]))}}.dump();
    }

    throw std::runtime_error("argb_ops: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("argb_ops", &writeList, &runLine)

} // namespace mc::diff
