#pragma once

#include "../../../../core/Types.hpp"

namespace mc {

/**
 * @brief 马的基础毛色
 *
 * 对齐 1.16.5 `CoatColors`。
 */
enum class CoatColors : u8 {
    White = 0,
    Creamy = 1,
    Chestnut = 2,
    Brown = 3,
    Black = 4,
    Gray = 5,
    DarkBrown = 6
};

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
