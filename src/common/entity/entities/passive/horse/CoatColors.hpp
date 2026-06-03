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

#include "common/core/Types.hpp"

namespace mc {

/**
 * @brief 马的基础毛色
 */
enum class CoatColors : u8 { White = 0, Creamy = 1, Chestnut = 2, Brown = 3, Black = 4, Gray = 5, DarkBrown = 6 };

inline constexpr u8 COAT_COLORS_COUNT = 7;

/**
 * @brief 获取毛色 ID
 */
[[nodiscard]] constexpr u8 getCoatColorId(CoatColors color)
{
    return static_cast<u8>(color);
}

/**
 * @brief 通过 ID 获取毛色
 *
 * 使用 vanilla 风格的取模归一化。
 */
[[nodiscard]] constexpr CoatColors getCoatColorById(i32 id)
{
    const i32 normalized = ((id % COAT_COLORS_COUNT) + COAT_COLORS_COUNT) % COAT_COLORS_COUNT;
    return static_cast<CoatColors>(normalized);
}

/**
 * @brief 获取毛色名称
 */
[[nodiscard]] constexpr const char* getCoatColorName(CoatColors color)
{
    switch (color) {
        case CoatColors::White:
            return "white";
        case CoatColors::Creamy:
            return "creamy";
        case CoatColors::Chestnut:
            return "chestnut";
        case CoatColors::Brown:
            return "brown";
        case CoatColors::Black:
            return "black";
        case CoatColors::Gray:
            return "gray";
        case CoatColors::DarkBrown:
            return "darkbrown";
    }

    return "white";
}

} // namespace mc
