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
#include <optional>

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
            return 0xF0F0F0;
        case DyeColor::Orange:
            return 0xEB8844;
        case DyeColor::Magenta:
            return 0xC354CD;
        case DyeColor::LightBlue:
            return 0x6689D3;
        case DyeColor::Yellow:
            return 0xDECF2A;
        case DyeColor::Lime:
            return 0x41CD34;
        case DyeColor::Pink:
            return 0xD88198;
        case DyeColor::Gray:
            return 0x434343;
        case DyeColor::LightGray:
            return 0xABABAB;
        case DyeColor::Cyan:
            return 0x287697;
        case DyeColor::Purple:
            return 0x7B2FBE;
        case DyeColor::Blue:
            return 0x253192;
        case DyeColor::Brown:
            return 0x51301A;
        case DyeColor::Green:
            return 0x3B511A;
        case DyeColor::Red:
            return 0xB3312C;
        case DyeColor::Black:
            return 0x1E1B1B;
        default:
            return 0xF0F0F0;
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

} // namespace mc
