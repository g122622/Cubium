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

#include "MaterialColor.hpp"
#include "common/core/Types.hpp"
#include "util/assert/AssertMacros.hpp"
#include <array>
#include <cstddef>

namespace mc::world::map {

std::array<MaterialColor, static_cast<size_t>(MaterialColorId::COUNT)> MaterialColor::s_colors = {};
bool MaterialColor::s_initialized = false;

MaterialColor::MaterialColor(MaterialColorId id, u32 rgb)
    : m_id(id)
    , m_rgb(rgb)
{}

void MaterialColor::initialize()
{
    if (s_initialized) {
        return;
    }

    // RGB值与Minecraft 1.16.5 MaterialColor完全对应
    s_colors[static_cast<size_t>(MaterialColorId::AIR)] = MaterialColor(MaterialColorId::AIR, 0x000000);
    s_colors[static_cast<size_t>(MaterialColorId::GRASS)] = MaterialColor(MaterialColorId::GRASS, 0x7FB238);
    s_colors[static_cast<size_t>(MaterialColorId::SAND)] = MaterialColor(MaterialColorId::SAND, 0xF7E9A3);
    s_colors[static_cast<size_t>(MaterialColorId::WOOL)] = MaterialColor(MaterialColorId::WOOL, 0xC7C7C7);
    s_colors[static_cast<size_t>(MaterialColorId::TNT)] = MaterialColor(MaterialColorId::TNT, 0xFF0000);
    s_colors[static_cast<size_t>(MaterialColorId::ICE)] = MaterialColor(MaterialColorId::ICE, 0xA0A0FF);
    s_colors[static_cast<size_t>(MaterialColorId::IRON)] = MaterialColor(MaterialColorId::IRON, 0xA7A7A7);
    s_colors[static_cast<size_t>(MaterialColorId::FOLIAGE)] = MaterialColor(MaterialColorId::FOLIAGE, 0x007C00);
    s_colors[static_cast<size_t>(MaterialColorId::SNOW)] = MaterialColor(MaterialColorId::SNOW, 0xFFFFFF);
    s_colors[static_cast<size_t>(MaterialColorId::CLAY)] = MaterialColor(MaterialColorId::CLAY, 0xA4A8B8);
    s_colors[static_cast<size_t>(MaterialColorId::DIRT)] = MaterialColor(MaterialColorId::DIRT, 0x976D4D);
    s_colors[static_cast<size_t>(MaterialColorId::STONE)] = MaterialColor(MaterialColorId::STONE, 0x707070);
    s_colors[static_cast<size_t>(MaterialColorId::WATER)] = MaterialColor(MaterialColorId::WATER, 0x4040CF);
    s_colors[static_cast<size_t>(MaterialColorId::WOOD)] = MaterialColor(MaterialColorId::WOOD, 0x8F7748);
    s_colors[static_cast<size_t>(MaterialColorId::QUARTZ)] = MaterialColor(MaterialColorId::QUARTZ, 0xFFFFD5);
    s_colors[static_cast<size_t>(MaterialColorId::ADOBE)] = MaterialColor(MaterialColorId::ADOBE, 0xD87F33);

    // 16种染料颜色
    s_colors[static_cast<size_t>(MaterialColorId::MAGENTA)] = MaterialColor(MaterialColorId::MAGENTA, 0xB24CD8);
    s_colors[static_cast<size_t>(MaterialColorId::LIGHT_BLUE)] = MaterialColor(MaterialColorId::LIGHT_BLUE, 0x6699D8);
    s_colors[static_cast<size_t>(MaterialColorId::YELLOW)] = MaterialColor(MaterialColorId::YELLOW, 0xE5E533);
    s_colors[static_cast<size_t>(MaterialColorId::LIME)] = MaterialColor(MaterialColorId::LIME, 0x7FCC19);
    s_colors[static_cast<size_t>(MaterialColorId::PINK)] = MaterialColor(MaterialColorId::PINK, 0xF27FA5);
    s_colors[static_cast<size_t>(MaterialColorId::GRAY)] = MaterialColor(MaterialColorId::GRAY, 0x4C4C4C);
    s_colors[static_cast<size_t>(MaterialColorId::LIGHT_GRAY)] = MaterialColor(MaterialColorId::LIGHT_GRAY, 0x999999);
    s_colors[static_cast<size_t>(MaterialColorId::CYAN)] = MaterialColor(MaterialColorId::CYAN, 0x4C7F99);
    s_colors[static_cast<size_t>(MaterialColorId::PURPLE)] = MaterialColor(MaterialColorId::PURPLE, 0x7F66B2);
    s_colors[static_cast<size_t>(MaterialColorId::BLUE)] = MaterialColor(MaterialColorId::BLUE, 0x334CB2);
    s_colors[static_cast<size_t>(MaterialColorId::BROWN)] = MaterialColor(MaterialColorId::BROWN, 0x664C33);
    s_colors[static_cast<size_t>(MaterialColorId::GREEN)] = MaterialColor(MaterialColorId::GREEN, 0x667F33);
    s_colors[static_cast<size_t>(MaterialColorId::RED)] = MaterialColor(MaterialColorId::RED, 0x993333);
    s_colors[static_cast<size_t>(MaterialColorId::BLACK)] = MaterialColor(MaterialColorId::BLACK, 0x191919);

    // 特殊颜色
    s_colors[static_cast<size_t>(MaterialColorId::GOLD)] = MaterialColor(MaterialColorId::GOLD, 0xFAEE4D);
    s_colors[static_cast<size_t>(MaterialColorId::DIAMOND)] = MaterialColor(MaterialColorId::DIAMOND, 0x5CDBD5);
    s_colors[static_cast<size_t>(MaterialColorId::LAPIS)] = MaterialColor(MaterialColorId::LAPIS, 0x4A80FF);
    s_colors[static_cast<size_t>(MaterialColorId::EMERALD)] = MaterialColor(MaterialColorId::EMERALD, 0x00D93A);
    s_colors[static_cast<size_t>(MaterialColorId::OBSIDIAN)] = MaterialColor(MaterialColorId::OBSIDIAN, 0x815E31);
    s_colors[static_cast<size_t>(MaterialColorId::NETHERRACK)] = MaterialColor(MaterialColorId::NETHERRACK, 0x700200);

    // 16种陶瓦颜色
    s_colors[static_cast<size_t>(MaterialColorId::WHITE_TERRACOTTA)] =
        MaterialColor(MaterialColorId::WHITE_TERRACOTTA, 0xD1B1A1);
    s_colors[static_cast<size_t>(MaterialColorId::ORANGE_TERRACOTTA)] =
        MaterialColor(MaterialColorId::ORANGE_TERRACOTTA, 0x9F5224);
    s_colors[static_cast<size_t>(MaterialColorId::MAGENTA_TERRACOTTA)] =
        MaterialColor(MaterialColorId::MAGENTA_TERRACOTTA, 0x95576C);
    s_colors[static_cast<size_t>(MaterialColorId::LIGHT_BLUE_TERRACOTTA)] =
        MaterialColor(MaterialColorId::LIGHT_BLUE_TERRACOTTA, 0x706C8A);
    s_colors[static_cast<size_t>(MaterialColorId::YELLOW_TERRACOTTA)] =
        MaterialColor(MaterialColorId::YELLOW_TERRACOTTA, 0xBA8524);
    s_colors[static_cast<size_t>(MaterialColorId::LIME_TERRACOTTA)] =
        MaterialColor(MaterialColorId::LIME_TERRACOTTA, 0x677521);
    s_colors[static_cast<size_t>(MaterialColorId::PINK_TERRACOTTA)] =
        MaterialColor(MaterialColorId::PINK_TERRACOTTA, 0xA04D4E);
    s_colors[static_cast<size_t>(MaterialColorId::GRAY_TERRACOTTA)] =
        MaterialColor(MaterialColorId::GRAY_TERRACOTTA, 0x392923);
    s_colors[static_cast<size_t>(MaterialColorId::LIGHT_GRAY_TERRACOTTA)] =
        MaterialColor(MaterialColorId::LIGHT_GRAY_TERRACOTTA, 0x876B62);
    s_colors[static_cast<size_t>(MaterialColorId::CYAN_TERRACOTTA)] =
        MaterialColor(MaterialColorId::CYAN_TERRACOTTA, 0x575C5C);
    s_colors[static_cast<size_t>(MaterialColorId::PURPLE_TERRACOTTA)] =
        MaterialColor(MaterialColorId::PURPLE_TERRACOTTA, 0x7A4958);
    s_colors[static_cast<size_t>(MaterialColorId::BLUE_TERRACOTTA)] =
        MaterialColor(MaterialColorId::BLUE_TERRACOTTA, 0x4C3E51);
    s_colors[static_cast<size_t>(MaterialColorId::BROWN_TERRACOTTA)] =
        MaterialColor(MaterialColorId::BROWN_TERRACOTTA, 0x4C3223);
    s_colors[static_cast<size_t>(MaterialColorId::GREEN_TERRACOTTA)] =
        MaterialColor(MaterialColorId::GREEN_TERRACOTTA, 0x4C522A);
    s_colors[static_cast<size_t>(MaterialColorId::RED_TERRACOTTA)] =
        MaterialColor(MaterialColorId::RED_TERRACOTTA, 0x8E3D2E);
    s_colors[static_cast<size_t>(MaterialColorId::BLACK_TERRACOTTA)] =
        MaterialColor(MaterialColorId::BLACK_TERRACOTTA, 0x251610);

    // 下界1.16新增颜色
    s_colors[static_cast<size_t>(MaterialColorId::CRIMSON_NYLIUM)] =
        MaterialColor(MaterialColorId::CRIMSON_NYLIUM, 0xBD5851);
    s_colors[static_cast<size_t>(MaterialColorId::CRIMSON_STEM)] =
        MaterialColor(MaterialColorId::CRIMSON_STEM, 0x94654D);
    s_colors[static_cast<size_t>(MaterialColorId::CRIMSON_HYPHAE)] =
        MaterialColor(MaterialColorId::CRIMSON_HYPHAE, 0x5C2B3D);
    s_colors[static_cast<size_t>(MaterialColorId::WARPED_NYLIUM)] =
        MaterialColor(MaterialColorId::WARPED_NYLIUM, 0x167E86);
    s_colors[static_cast<size_t>(MaterialColorId::WARPED_STEM)] = MaterialColor(MaterialColorId::WARPED_STEM, 0x3A8F8C);
    s_colors[static_cast<size_t>(MaterialColorId::WARPED_HYPHAE)] =
        MaterialColor(MaterialColorId::WARPED_HYPHAE, 0x563B4E);
    s_colors[static_cast<size_t>(MaterialColorId::WARPED_WART)] = MaterialColor(MaterialColorId::WARPED_WART, 0x14B485);

    s_initialized = true;
}

const MaterialColor& MaterialColor::getById(MaterialColorId id)
{
    MC_ASSERT(s_initialized);
    auto index = static_cast<size_t>(id);
    MC_ASSERT(index < static_cast<size_t>(MaterialColorId::COUNT));
    return s_colors[index];
}

const MaterialColor& MaterialColor::getByIndex(u8 index)
{
    MC_ASSERT(s_initialized);
    MC_ASSERT(index < static_cast<u8>(MaterialColorId::COUNT));
    return s_colors[index];
}

u32 MaterialColor::computeMapColor(u8 colorIndex, u8 shadeIndex)
{
    MC_ASSERT(shadeIndex < SHADE_COUNT);

    // 颜色索引0 = 空气（透明）
    if (colorIndex == 0) {
        return 0;
    }

    // 超出范围的颜色索引使用最近的有效值
    if (colorIndex >= static_cast<u8>(MaterialColorId::COUNT)) {
        colorIndex = static_cast<u8>(MaterialColorId::COUNT) - 1;
    }

    const MaterialColor& color = s_colors[colorIndex];
    u32 rgb = color.m_rgb;

    // 应用阴影亮度乘数
    u32 multiplier = SHADE_MULTIPLIERS[shadeIndex];

    u8 r = static_cast<u8>(((rgb >> 16) & 0xFF) * multiplier / 255);
    u8 g = static_cast<u8>(((rgb >> 8) & 0xFF) * multiplier / 255);
    u8 b = static_cast<u8>((rgb & 0xFF) * multiplier / 255);

    // ARGB格式，全不透明
    return 0xFF000000u | (static_cast<u32>(b) << 16) | (static_cast<u32>(g) << 8) | static_cast<u32>(r);
}

u32 MaterialColor::pixelToArgb(u8 pixelByte)
{
    u8 colorIndex = pixelByte >> 2;
    u8 shadeIndex = pixelByte & 0x03;
    return computeMapColor(colorIndex, shadeIndex);
}

} // namespace mc::world::map
