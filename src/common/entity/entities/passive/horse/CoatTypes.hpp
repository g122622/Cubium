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
 * @brief 马的花纹类型
 */
enum class CoatTypes : u8 { None = 0, White = 1, WhiteField = 2, WhiteDots = 3, BlackDots = 4 };

inline constexpr u8 COAT_TYPES_COUNT = 5;

/**
 * @brief 获取花纹 ID
 */
[[nodiscard]] constexpr u8 getCoatTypeId(CoatTypes type)
{
    return static_cast<u8>(type);
}

/**
 * @brief 通过 ID 获取花纹类型
 *
 * 使用 vanilla 风格的取模归一化。
 */
[[nodiscard]] constexpr CoatTypes getCoatTypeById(i32 id)
{
    const i32 normalized = ((id % COAT_TYPES_COUNT) + COAT_TYPES_COUNT) % COAT_TYPES_COUNT;
    return static_cast<CoatTypes>(normalized);
}

/**
 * @brief 获取花纹名称
 */
[[nodiscard]] constexpr const char* getCoatTypeName(CoatTypes type)
{
    switch (type) {
        case CoatTypes::None:
            return "none";
        case CoatTypes::White:
            return "white";
        case CoatTypes::WhiteField:
            return "whitefield";
        case CoatTypes::WhiteDots:
            return "whitedots";
        case CoatTypes::BlackDots:
            return "blackdots";
    }

    return "none";
}

} // namespace mc
