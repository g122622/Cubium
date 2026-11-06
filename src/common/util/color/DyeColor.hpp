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

#include "core/Types.hpp"
#include <array>
#include <optional>
#include <string_view>

namespace mc {

/**
 * @brief 染料颜色枚举
 *
 * 对应 MC DyeColor，定义16种标准染料颜色。
 * 广泛用于旗帜、皮革盔甲染色、混凝土、陶瓦、烟花之星等场景。
 */
enum class DyeColor : u8 {
    White = 0,
    Orange = 1,
    Magenta = 2,
    LightBlue = 3,
    Yellow = 4,
    Lime = 5,
    Pink = 6,
    Gray = 7,
    LightGray = 8,
    Cyan = 9,
    Purple = 10,
    Blue = 11,
    Brown = 12,
    Green = 13,
    Red = 14,
    Black = 15,
    Count = 16
};

/**
 * @brief 染料的资源名（vanilla `DyeColor.getSerializedName()`）
 *
 * 16 种颜色的序号与枚举值一致（`0 = white … 15 = black`）；数据组件、旗帜图案层
 * 与实体组件（如 `minecraft:sheep/color`）都用这同一套字面量。
 */
[[nodiscard]] constexpr std::string_view dyeColorName(DyeColor color) noexcept
{
    constexpr std::array<std::string_view, 16> NAMES{"white",
        "orange",
        "magenta",
        "light_blue",
        "yellow",
        "lime",
        "pink",
        "gray",
        "light_gray",
        "cyan",
        "purple",
        "blue",
        "brown",
        "green",
        "red",
        "black"};
    const auto index = static_cast<usize>(color);
    return index < NAMES.size() ? NAMES[index] : std::string_view{};
}

/**
 * @brief 染料对应的烟花色（vanilla DyeColor.getFireworkColor()）
 *
 * 注意与"纹理漫反射色"（getTextureDiffuseColor，盔甲/狼铠染色用）不同：
 * 烟花色整体更饱和，例如白色烟花为 0xF0F0F0 而纹理色为 0xF9FFFE。
 *
 * @param color 染料颜色
 * @return 0xRRGGBB 形式的烟花色
 */
[[nodiscard]] constexpr u32 dyeFireworkColor(DyeColor color) noexcept
{
    switch (color) {
        case DyeColor::White:
            return 15790320U;
        case DyeColor::Orange:
            return 15435844U;
        case DyeColor::Magenta:
            return 12801229U;
        case DyeColor::LightBlue:
            return 6719955U;
        case DyeColor::Yellow:
            return 14602026U;
        case DyeColor::Lime:
            return 4312372U;
        case DyeColor::Pink:
            return 14188952U;
        case DyeColor::Gray:
            return 4408131U;
        case DyeColor::LightGray:
            return 11250603U;
        case DyeColor::Cyan:
            return 2651799U;
        case DyeColor::Purple:
            return 8073150U;
        case DyeColor::Blue:
            return 2437522U;
        case DyeColor::Brown:
            return 5320730U;
        case DyeColor::Green:
            return 3887386U;
        case DyeColor::Red:
            return 11743532U;
        case DyeColor::Black:
            return 1973019U;
        default:
            return 0xF0F0F0;
    }
}

/**
 * @brief 染料对应的纹理漫反射色（vanilla DyeColor.getTextureDiffuseColor()）
 *
 * 这是盔甲/狼铠染色混色（DyedItemColor.applyDyes）实际使用的颜色表，
 * 与烟花色 [[dyeFireworkColor]] 不同：漫反射色整体偏灰（更接近材质底色），
 * 例如白色漫反射色为 0xF9FFFE 而烟花色为 0xF0F0F0。
 *
 * @param color 染料颜色
 * @return 0xRRGGBB 形式的纹理漫反射色
 */
[[nodiscard]] constexpr u32 dyeTextureDiffuseColor(DyeColor color) noexcept
{
    switch (color) {
        case DyeColor::White:
            return 0xF9FFFE;
        case DyeColor::Orange:
            return 0xF9801D;
        case DyeColor::Magenta:
            return 0xC74EBD;
        case DyeColor::LightBlue:
            return 0x3AB3DA;
        case DyeColor::Yellow:
            return 0xFED83D;
        case DyeColor::Lime:
            return 0x80C71F;
        case DyeColor::Pink:
            return 0xF38BAA;
        case DyeColor::Gray:
            return 0x474F52;
        case DyeColor::LightGray:
            return 0x9D9D97;
        case DyeColor::Cyan:
            return 0x169C9C;
        case DyeColor::Purple:
            return 0x8932B8;
        case DyeColor::Blue:
            return 0x3C44AA;
        case DyeColor::Brown:
            return 0x835432;
        case DyeColor::Green:
            return 0x5E7C16;
        case DyeColor::Red:
            return 0xB02E26;
        case DyeColor::Black:
            return 0x1D1D21;
        default:
            return 0xF9FFFE;
    }
}

/**
 * @brief 由烟花色反查染料颜色（vanilla DyeColor.byFireworkColor）
 *
 * @param fireworkColor 0xRRGGBB 形式的烟花色
 * @return 对应染料；无匹配返回 nullopt
 */
[[nodiscard]] constexpr std::optional<DyeColor> dyeColorByFireworkColor(u32 fireworkColor) noexcept
{
    for (u8 i = 0; i < static_cast<u8>(DyeColor::Count); ++i) {
        const auto color = static_cast<DyeColor>(i);
        if (dyeFireworkColor(color) == fireworkColor) {
            return color;
        }
    }
    return std::nullopt;
}

/**
 * @brief 按 id 取染料，对应原版 `DyeColor.byId`
 *
 * 原版用 `ByIdMap.continuous(..., OutOfBoundsStrategy.ZERO)`——越界一律返回
 * **下标 0（WHITE）**，不是按 16 取模回绕。负数同样落到 WHITE。
 *
 * 注意这与 `Rotation.BY_ID` 的 WRAP 策略不同：不同枚举选的越界策略并不统一。
 */
[[nodiscard]] constexpr DyeColor dyeColorById(i32 id) noexcept
{
    constexpr i32 COUNT = static_cast<i32>(DyeColor::Count);
    return (id >= 0 && id < COUNT) ? static_cast<DyeColor>(id) : static_cast<DyeColor>(0);
}

/**
 * @brief 按序列化名取染料，对应原版 `DyeColor.byName`
 *
 * 严格匹配小写序列化名；找不到时返回调用方给的默认值。
 */
[[nodiscard]] constexpr std::optional<DyeColor> dyeColorByName(std::string_view name) noexcept
{
    for (u8 i = 0; i < static_cast<u8>(DyeColor::Count); ++i) {
        const auto color = static_cast<DyeColor>(i);
        if (dyeColorName(color) == name) {
            return color;
        }
    }
    return std::nullopt;
}

/**
 * @brief 染料对应的文字色（vanilla DyeColor.getTextColor()）
 *
 * 注意这个名字有歧义：它不是"用该染料染出的皮革/羊毛色"，而是
 * **聊天与告示牌上显示该颜色名字时用的十六进制颜色码**（如 WHITE → 0xFFFFF）。
 *
 * 与 [[dyeTextureDiffuseColor]]（材质底色）、[[dyeFireworkColor]]（烟花色）都不同，
 * 三者两两互不相等。
 */
[[nodiscard]] constexpr u32 dyeTextColor(DyeColor color) noexcept
{
    switch (color) {
        case DyeColor::White:
            return 16777215U;
        case DyeColor::Orange:
            return 16738335U;
        case DyeColor::Magenta:
            return 16711935U;
        case DyeColor::LightBlue:
            return 10141901U;
        case DyeColor::Yellow:
            return 16776960U;
        case DyeColor::Lime:
            return 12582656U;
        case DyeColor::Pink:
            return 16738740U;
        case DyeColor::Gray:
            return 8421504U;
        case DyeColor::LightGray:
            return 13882323U;
        case DyeColor::Cyan:
            return 65535U;
        case DyeColor::Purple:
            return 10494192U;
        case DyeColor::Blue:
            return 255U;
        case DyeColor::Brown:
            return 9127187U;
        case DyeColor::Green:
            return 65280U;
        case DyeColor::Red:
            return 16711680U;
        case DyeColor::Black:
            return 0U;
        case DyeColor::Count:
            break;
    }
    return 0x000000;
}

} // namespace mc
