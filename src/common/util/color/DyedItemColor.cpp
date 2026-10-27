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

#include "common/util/color/DyedItemColor.hpp"

#include <algorithm>

namespace mc {

namespace {

/// 提取 0xRRGGBB / 0xAARRGGBB 中的红色分量（对齐 vanilla ARGB.red）
[[nodiscard]] constexpr i32 _red(u32 color) noexcept
{
    return static_cast<i32>((color >> 16) & 0xFFU);
}

/// 提取绿色分量（对齐 vanilla ARGB.green）
[[nodiscard]] constexpr i32 _green(u32 color) noexcept
{
    return static_cast<i32>((color >> 8) & 0xFFU);
}

/// 提取蓝色分量（对齐 vanilla ARGB.blue）
[[nodiscard]] constexpr i32 _blue(u32 color) noexcept
{
    return static_cast<i32>(color & 0xFFU);
}

/// 三通道最大值（对齐 vanilla Math.max(r, Math.max(g, b))）
[[nodiscard]] constexpr i32 _maxChannel(i32 r, i32 g, i32 b) noexcept
{
    return std::max(r, std::max(g, b));
}

} // namespace

u32 DyedItemColor::applyDyes(std::optional<u32> currentColor, const std::vector<u32>& dyeColors) noexcept
{
    if (dyeColors.empty()) {
        return currentColor.value_or(0U);
    }

    // vanilla 用 int 累加，此处保持 i32 以对齐整数除法截断语义。
    i32 sumR = 0;
    i32 sumG = 0;
    i32 sumB = 0;
    i32 sumMax = 0;
    i32 count = 0;

    // 已有颜色也参与平均（对齐 vanilla：dyeditemcolor != null 时先累加当前颜色）
    if (currentColor.has_value()) {
        const u32 current = currentColor.value();
        const i32 r = _red(current);
        const i32 g = _green(current);
        const i32 b = _blue(current);
        sumMax += _maxChannel(r, g, b);
        sumR += r;
        sumG += g;
        sumB += b;
        ++count;
    }

    for (const u32 dyeColor : dyeColors) {
        const i32 r = _red(dyeColor);
        const i32 g = _green(dyeColor);
        const i32 b = _blue(dyeColor);
        sumMax += _maxChannel(r, g, b);
        sumR += r;
        sumG += g;
        sumB += b;
        ++count;
    }

    // count >= 1 由上方非空分支保证。
    i32 avgR = sumR / count;
    i32 avgG = sumG / count;
    i32 avgB = sumB / count;
    const f32 avgMaxChannel = static_cast<f32>(sumMax) / static_cast<f32>(count);
    const f32 maxAvg = static_cast<f32>(_maxChannel(avgR, avgG, avgB));

    // 亮度归一化：把平均值按 avgMax/maxAvg 缩放。maxAvg 为 0（纯黑输入）时保持原值，
    // 避免除零；vanilla 在此处为 0/0 = NaN 再截断为 0，效果等价。
    if (maxAvg > 0.0f) {
        avgR = static_cast<i32>(static_cast<f32>(avgR) * avgMaxChannel / maxAvg);
        avgG = static_cast<i32>(static_cast<f32>(avgG) * avgMaxChannel / maxAvg);
        avgB = static_cast<i32>(static_cast<f32>(avgB) * avgMaxChannel / maxAvg);
    }

    const u32 r = static_cast<u32>(std::max(0, std::min(255, avgR)));
    const u32 g = static_cast<u32>(std::max(0, std::min(255, avgG)));
    const u32 b = static_cast<u32>(std::max(0, std::min(255, avgB)));
    return (r << 16) | (g << 8) | b;
}

} // namespace mc
