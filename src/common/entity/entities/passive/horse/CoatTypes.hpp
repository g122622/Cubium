#pragma once

#include "../../../../core/Types.hpp"

namespace mc {

/**
 * @brief 马的花纹类型
 *
 * 对齐 1.16.5 `CoatTypes`。
 */
enum class CoatTypes : u8 {
    None = 0,
    White = 1,
    WhiteField = 2,
    WhiteDots = 3,
    BlackDots = 4
};

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
