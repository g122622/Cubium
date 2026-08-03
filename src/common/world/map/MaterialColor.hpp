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
#include <cstddef>

namespace mc::world::map {

/**
 * @brief 地图颜色ID枚举
 *
 * 定义地图渲染使用的颜色索引，与Minecraft 1.16.5的MaterialColor对应。
 * 每种颜色有4个阴影级别(0-3)，最终颜色编码为 colorIndex * 4 + shadeIndex。
 */
enum class MaterialColorId : u8 {
    AIR = 0,
    GRASS = 1,
    SAND = 2,
    WOOL = 3,
    TNT = 4,
    ICE = 5,
    IRON = 6,
    FOLIAGE = 7,
    SNOW = 8,
    CLAY = 9,
    DIRT = 10,
    STONE = 11,
    WATER = 12,
    WOOD = 13,
    QUARTZ = 14,
    ADOBE = 15, // 橙色陶瓦/红沙
    MAGENTA = 16,
    LIGHT_BLUE = 17,
    YELLOW = 18,
    LIME = 19,
    PINK = 20,
    GRAY = 21,
    LIGHT_GRAY = 22,
    CYAN = 23,
    PURPLE = 24,
    BLUE = 25,
    BROWN = 26,
    GREEN = 27,
    RED = 28,
    BLACK = 29,
    GOLD = 30,
    DIAMOND = 31,
    LAPIS = 32,
    EMERALD = 33,
    OBSIDIAN = 34,
    NETHERRACK = 35,
    WHITE_TERRACOTTA = 36,
    ORANGE_TERRACOTTA = 37,
    MAGENTA_TERRACOTTA = 38,
    LIGHT_BLUE_TERRACOTTA = 39,
    YELLOW_TERRACOTTA = 40,
    LIME_TERRACOTTA = 41,
    PINK_TERRACOTTA = 42,
    GRAY_TERRACOTTA = 43,
    LIGHT_GRAY_TERRACOTTA = 44,
    CYAN_TERRACOTTA = 45,
    PURPLE_TERRACOTTA = 46,
    BLUE_TERRACOTTA = 47,
    BROWN_TERRACOTTA = 48,
    GREEN_TERRACOTTA = 49,
    RED_TERRACOTTA = 50,
    BLACK_TERRACOTTA = 51,
    CRIMSON_NYLIUM = 52,
    CRIMSON_STEM = 53,
    CRIMSON_HYPHAE = 54,
    WARPED_NYLIUM = 55,
    WARPED_STEM = 56,
    WARPED_HYPHAE = 57,
    WARPED_WART = 58,

    COUNT = 59
};

/**
 * @brief 地图颜色系统
 *
 * 提供地图颜色ID到RGB颜色的映射，以及阴影级别计算。
 * 地图像素颜色编码: byte = colorIndex * 4 + shadeIndex
 * 其中 colorIndex = MaterialColorId 的数值，shadeIndex = 0-3。
 */
class MaterialColor {
public:
    /**
     * @brief 阴影级别数量
     */
    static constexpr u8 SHADE_COUNT = 4;

    /**
     * @brief 阴影亮度乘数（0-3对应的亮度百分比）
     *
     * shade 0 = 180/255 (70.6%)
     * shade 1 = 220/255 (86.3%)
     * shade 2 = 255/255 (100%)
     * shade 3 = 135/255 (52.9%)
     */
    static constexpr std::array<u32, SHADE_COUNT> SHADE_MULTIPLIERS = {180, 220, 255, 135};

    MaterialColor() = default;
    MaterialColor(MaterialColorId id, u32 rgb);

    /**
     * @brief 初始化所有颜色定义，必须在使用前调用
     */
    static void initialize();

    /**
     * @brief 根据ID获取颜色定义
     */
    [[nodiscard]] static const MaterialColor& getById(MaterialColorId id);

    /**
     * @brief 根据颜色索引(u8)获取颜色定义
     */
    [[nodiscard]] static const MaterialColor& getByIndex(u8 index);

    /**
     * @brief 计算地图渲染颜色
     *
     * @param colorIndex 颜色索引 (MaterialColorId的数值)
     * @param shadeIndex 阴影级别 (0-3)
     * @return ARGB颜色值 (0xAARRGGBB格式)
     */
    [[nodiscard]] static u32 computeMapColor(u8 colorIndex, u8 shadeIndex);

    /**
     * @brief 从地图像素字节计算ARGB颜色
     *
     * @param pixelByte 地图像素字节 (colorIndex * 4 + shadeIndex)
     * @return ARGB颜色值，颜色索引为0时返回0（透明）
     */
    [[nodiscard]] static u32 pixelToArgb(u8 pixelByte);

    /**
     * @brief 获取颜色ID
     */
    [[nodiscard]] MaterialColorId id() const { return m_id; }

    /**
     * @brief 获取原始RGB颜色值
     */
    [[nodiscard]] u32 rgb() const { return m_rgb; }

private:
    MaterialColorId m_id;
    u32 m_rgb;

    /** 颜色查找表，由initialize()填充 */
    static std::array<MaterialColor, static_cast<size_t>(MaterialColorId::COUNT)> s_colors;
    static bool s_initialized;
};

} // namespace mc::world::map
