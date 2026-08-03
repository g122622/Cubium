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

#pragma once

#include "client/ui/kagero/Types.hpp"
#include "common/core/Types.hpp"

namespace mc::client::ui::kagero::paint {

/**
 * @brief 线性颜色
 */
struct Color {
    f32 r = 1.0f;
    f32 g = 1.0f;
    f32 b = 1.0f;
    f32 a = 1.0f;

    Color() = default;

    constexpr Color(f32 red, f32 green, f32 blue, f32 alpha = 1.0f)
        : r(red)
        , g(green)
        , b(blue)
        , a(alpha)
    {}

    [[nodiscard]] static constexpr Color fromARGB(u32 argb)
    {
        constexpr f32 kScale = 1.0f / 255.0f;
        return Color(static_cast<f32>((argb >> 16) & 0xFF) * kScale,
            static_cast<f32>((argb >> 8) & 0xFF) * kScale,
            static_cast<f32>(argb & 0xFF) * kScale,
            static_cast<f32>((argb >> 24) & 0xFF) * kScale);
    }

    [[nodiscard]] constexpr u32 toARGB() const
    {
        const u32 aa = static_cast<u32>(a * 255.0f) & 0xFF;
        const u32 rr = static_cast<u32>(r * 255.0f) & 0xFF;
        const u32 gg = static_cast<u32>(g * 255.0f) & 0xFF;
        const u32 bb = static_cast<u32>(b * 255.0f) & 0xFF;
        return (aa << 24) | (rr << 16) | (gg << 8) | bb;
    }
};

inline constexpr Color WHITE_COLOR{1.0f, 1.0f, 1.0f, 1.0f};
inline constexpr Color BLACK_COLOR{0.0f, 0.0f, 0.0f, 1.0f};
inline constexpr Color TRANSPARENT_COLOR{0.0f, 0.0f, 0.0f, 0.0f};

} // namespace mc::client::ui::kagero::paint
