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
 */

#pragma once

#include "common/core/Types.hpp"

#include <algorithm>
#include <cmath>

namespace mc::util::argb {

/**
 * @brief 32 位 ARGB 打包颜色的运算，对应原版 `net.minecraft.util.ARGB`
 *
 * 打包布局是 `alpha << 24 | red << 16 | green << 8 | blue`。
 *
 * 一个贯穿全文件的关键点：**取 alpha 用逻辑右移，取其余分量用算术右移**。
 * 因为 alpha 在最高字节，若用算术右移会被符号位污染；而 red/green/blue 取出后
 * 还要 `& 0xFF`，算术右移与逻辑右移在掩码后结果相同。原版就是这么写的，
 * 这里逐条对应。
 */

/**
 * @brief 取 alpha 分量，对应原版 `ARGB.alpha`
 */
[[nodiscard]] inline constexpr u32 alpha(u32 color) noexcept
{
    return color >> 24U;
}

/**
 * @brief 取红分量，对应原版 `ARGB.red`
 */
[[nodiscard]] inline constexpr u32 red(u32 color) noexcept
{
    return (color >> 16U) & 0xFFU;
}

/**
 * @brief 取绿分量，对应原版 `ARGB.green`
 */
[[nodiscard]] inline constexpr u32 green(u32 color) noexcept
{
    return (color >> 8U) & 0xFFU;
}

/**
 * @brief 取蓝分量，对应原版 `ARGB.blue`
 */
[[nodiscard]] inline constexpr u32 blue(u32 color) noexcept
{
    return color & 0xFFU;
}

/**
 * @brief 打包四通道，对应原版 `ARGB.color(int, int, int, int)`
 *
 * 每个分量都先 `& 0xFF`，因此越界值会被截到低 8 位而不是被夹取——传 256 得 0。
 */
[[nodiscard]] inline constexpr u32 color(u32 a, u32 r, u32 g, u32 b) noexcept
{
    return ((a & 0xFFU) << 24U) | ((r & 0xFFU) << 16U) | ((g & 0xFFU) << 8U) | (b & 0xFFU);
}

/**
 * @brief 打包三个 RGB 分量（alpha 补满），对应原版 `ARGB.color(int, int, int)`
 */
[[nodiscard]] inline constexpr u32 color(u32 r, u32 g, u32 b) noexcept
{
    return color(255, r, g, b);
}

/**
 * @brief 浮点通道转 8 位，对应原版 `ARGB.as8BitChannel`
 *
 * `Mth.floor(v * 255)`——**向下取整**，不是四舍五入。
 */
[[nodiscard]] inline u32 as8BitChannel(f32 value) noexcept
{
    return static_cast<u32>(std::floor(value * 255.0f));
}

/**
 * @brief 8 位通道转浮点，对应原版 `ARGB.from8BitChannel`
 */
[[nodiscard]] inline constexpr f32 from8BitChannel(u32 value) noexcept
{
    return static_cast<f32>(value) / 255.0f;
}

/**
 * @brief 取 alpha 的浮点表示，对应原版 `ARGB.alphaFloat`
 */
[[nodiscard]] inline constexpr f32 alphaFloat(u32 colorValue) noexcept
{
    return from8BitChannel(alpha(colorValue));
}

/**
 * @brief 两个颜色逐通道相乘，对应原版 `ARGB.multiply`
 *
 * 任一参数为 `0xFFFFFFFF`（纯白不透明）时直接返回另一个——这是原版的短路优化，
 * 语义上与逐通道相乘等价（乘 255 再除 255），保留它是为了对齐边界行为。
 */
[[nodiscard]] inline constexpr u32 multiply(u32 first, u32 second) noexcept
{
    if (first == 0xFFFFFFFFU) {
        return second;
    }
    if (second == 0xFFFFFFFFU) {
        return first;
    }
    return color(alpha(first) * alpha(second) / 255U,
        red(first) * red(second) / 255U,
        green(first) * green(second) / 255U,
        blue(first) * blue(second) / 255U);
}

/**
 * @brief 两个颜色逐通道相加并夹到 255，对应原版 `ARGB.addRgb`
 *
 * **alpha 取第一个参数的**，不与第二个参数合成。
 */
[[nodiscard]] inline constexpr u32 addRgb(u32 first, u32 second) noexcept
{
    return color(alpha(first),
        std::min(red(first) + red(second), 255U),
        std::min(green(first) + green(second), 255U),
        std::min(blue(first) + blue(second), 255U));
}

/**
 * @brief 两个颜色逐通道相减并夹到 0，对应原版 `ARGB.subtractRgb`
 *
 * 同样只取第一个参数的 alpha。
 */
[[nodiscard]] inline constexpr u32 subtractRgb(u32 first, u32 second) noexcept
{
    return color(alpha(first),
        red(first) > red(second) ? red(first) - red(second) : 0U,
        green(first) > green(second) ? green(first) - green(second) : 0U,
        blue(first) > blue(second) ? blue(first) - blue(second) : 0U);
}

/**
 * @brief 缩放 alpha 通道，对应原版 `ARGB.multiplyAlpha`
 *
 * 三条短路：颜色为 0 或缩放 <= 0 时返回 0；缩放 >= 1 时原样返回。
 * 否则把 `alphaFloat(color) * scale` 经 `as8BitChannel` 写回 alpha。
 *
 * 注意中间是**浮点乘法后再向下取整**，不是整数乘除——`alpha=100, scale=0.5`
 * 得 `floor(0.39215...*255)=100`，与整数路径一致纯属巧合。
 */
[[nodiscard]] inline u32 multiplyAlpha(u32 colorValue, f32 scale) noexcept
{
    if (colorValue == 0 || scale <= 0.0f) {
        return 0;
    }
    if (scale >= 1.0f) {
        return colorValue;
    }
    // 只替换 alpha，RGB 保持不变
    return (as8BitChannel(alphaFloat(colorValue) * scale) << 24U) | (colorValue & 0x00FFFFFFU);
}

/**
 * @brief 缩放 RGB 三通道（各自系数），对应原版 `ARGB.scaleRGB(int, float, float, float)`
 *
 * 每通道先转 `i32`（**向零截断**）再夹到 `[0, 255]`。因为截断在夹取之前，
 * 负系数的结果会先变成负数再被夹成 0。
 */
[[nodiscard]] inline u32 scaleRgb(u32 colorValue, f32 redScale, f32 greenScale, f32 blueScale) noexcept
{
    const auto channel = [](u32 value, f32 scale) {
        const i32 scaled = static_cast<i32>(static_cast<f32>(value) * scale);
        return static_cast<u32>(std::clamp(scaled, 0, 255));
    };
    return color(alpha(colorValue),
        channel(red(colorValue), redScale),
        channel(green(colorValue), greenScale),
        channel(blue(colorValue), blueScale));
}

/**
 * @brief 缩放 RGB 三个通道（同一系数），对应原版 `ARGB.scaleRGB(int, float)`
 */
[[nodiscard]] inline u32 scaleRgb(u32 colorValue, f32 scale) noexcept
{
    return scaleRgb(colorValue, scale, scale, scale);
}

/**
 * @brief 按整数系数缩放 RGB，对应原版 `ARGB.scaleRGB(int, int)`
 *
 * 用 64 位中间值再除 255，避免 `value * scale` 在 32 位下溢出。
 */
[[nodiscard]] inline constexpr u32 scaleRgbInt(u32 colorValue, i32 scale) noexcept
{
    const auto channel = [scale](u32 value) {
        const i64 scaled = static_cast<i64>(value) * scale / 255LL;
        return static_cast<u32>(std::clamp<i64>(scaled, 0, 255));
    };
    return color(alpha(colorValue), channel(red(colorValue)), channel(green(colorValue)), channel(blue(colorValue)));
}

/**
 * @brief 转灰度，对应原版 `ARGB.greyscale`
 *
 * 亮度系数是 `0.3/0.59/0.11`（不是常见的 `0.299/0.587/0.114`），且结果**向下截断**。
 * alpha 保持原样。
 */
[[nodiscard]] inline constexpr u32 greyscale(u32 colorValue) noexcept
{
    const auto level = static_cast<u32>(static_cast<f32>(red(colorValue)) * 0.3f +
        static_cast<f32>(green(colorValue)) * 0.59f + static_cast<f32>(blue(colorValue)) * 0.11f);
    return color(alpha(colorValue), level, level, level);
}

/**
 * @brief 单通道的 alpha 混合，对应原版 `ARGB.alphaBlendChannel`
 *
 * 公式为 `(overlay*oAlpha + base*(outAlpha-oAlpha)) / outAlpha`，整数除法。
 *
 * @note 调用方保证 `outAlpha` 非零——原版在 `overlayAlpha` 为 0 或 255 时已短路。
 */
[[nodiscard]] inline constexpr u32 alphaBlendChannel(
    u32 outAlpha, u32 overlayAlpha, u32 baseChannel, u32 overlayChannel) noexcept
{
    return (overlayChannel * overlayAlpha + baseChannel * (outAlpha - overlayAlpha)) / outAlpha;
}

/**
 * @brief alpha 混合两个颜色，对应原版 `ARGB.alphaBlend`
 *
 * `overlay` 完全透明（alpha=0）时返回 `base`，完全不透明（alpha=255）时返回 `overlay`，
 * 否则按标准 alpha 合成计算。合成后的 alpha 是
 * `overlayAlpha + baseAlpha * (255 - overlayAlpha) / 255`。
 */
[[nodiscard]] inline constexpr u32 alphaBlend(u32 base, u32 overlay) noexcept
{
    const u32 baseAlpha = alpha(base);
    const u32 overlayAlpha = alpha(overlay);
    if (overlayAlpha == 255) {
        return overlay;
    }
    if (overlayAlpha == 0) {
        return base;
    }
    const u32 outAlpha = overlayAlpha + baseAlpha * (255 - overlayAlpha) / 255;
    return color(outAlpha,
        alphaBlendChannel(outAlpha, overlayAlpha, red(base), red(overlay)),
        alphaBlendChannel(outAlpha, overlayAlpha, green(base), green(overlay)),
        alphaBlendChannel(outAlpha, overlayAlpha, blue(base), blue(overlay)));
}

} // namespace mc::util::argb
