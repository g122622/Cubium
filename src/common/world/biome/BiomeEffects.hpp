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

#include "../../core/Types.hpp"
#include <optional>

namespace mc {
namespace world {
namespace biome {

/**
 * @brief 草颜色修改器类型
 *
 * 某些生物群系会对草颜色应用特殊修改算法。
 */
enum class GrassColorModifier : u8 {
    None,       ///< 无修改，使用默认 colormap
    Swamp,      ///< 沼泽：基于 BIOME_INFO_NOISE 的双色混合（深绿/浅绿）
    DarkForest, ///< 黑森林：对 colormap 颜色应用位运算变暗 (color & 16711422 + 2634762) >> 1
    Badlands    ///< 恶地：使用固定草颜色覆盖（MC 1.21.11 中不作为修改器，而是直接设置 grassColor）
};

/**
 * @brief 生物群系视觉效果配置
 *
 * 存储生物群系的视觉相关属性，包括水体颜色、雾颜色、天空颜色等。
 * 这些属性主要用于客户端渲染，影响水体颜色、雾效果、天空渲染等。
 *
 * @note 所有颜色使用 RGB 格式 (0xRRGGBB)，alpha 通道默认为 0xFF。
 */
class BiomeEffects {
public:
    using OptionalColor = std::optional<u32>;

    BiomeEffects() = default;

    // === Getters ===

    /**
     * @brief 获取水体颜色
     * @return 水体颜色 (RGB格式)
     */
    [[nodiscard]] u32 waterColor() const noexcept { return m_waterColor; }

    /**
     * @brief 获取水下雾颜色
     * @return 水下雾颜色 (RGB格式)
     */
    [[nodiscard]] u32 waterFogColor() const noexcept { return m_waterFogColor; }

    /**
     * @brief 获取雾颜色
     * @return 雾颜色 (RGB格式)
     */
    [[nodiscard]] u32 fogColor() const noexcept { return m_fogColor; }

    /**
     * @brief 获取天空颜色
     * @return 天空颜色 (RGB格式)
     */
    [[nodiscard]] u32 skyColor() const noexcept { return m_skyColor; }

    /**
     * @brief 获取树叶颜色覆盖
     * @return 树叶颜色，如果未设置返回 nullopt
     *
     * @note 如果返回 nullopt，应使用 foliage colormap 计算颜色
     */
    [[nodiscard]] OptionalColor foliageColor() const noexcept { return m_foliageColor; }

    /**
     * @brief 获取草颜色覆盖
     * @return 草颜色，如果未设置返回 nullopt
     *
     * @note 如果返回 nullopt，应使用 grass colormap 计算颜色
     *       （可能还需要应用 grassColorModifier）
     */
    [[nodiscard]] OptionalColor grassColor() const noexcept { return m_grassColor; }

    /**
     * @brief 获取草颜色修改器
     * @return 草颜色修改器类型
     */
    [[nodiscard]] GrassColorModifier grassColorModifier() const noexcept { return m_grassColorModifier; }

    /**
     * @brief 获取干燥树叶颜色覆盖
     * @return 干燥树叶颜色，如果未设置返回 nullopt
     *
     * 新增，用于苍白花园等生物群系的干枯植被颜色。
     */
    [[nodiscard]] OptionalColor dryFoliageColor() const noexcept { return m_dryFoliageColor; }

    // === 默认颜色常量 ===

    static constexpr u32 DEFAULT_WATER_COLOR = 0x3F76E4;     ///< 默认水体颜色 (蓝色)
    static constexpr u32 DEFAULT_WATER_FOG_COLOR = 0x050533; ///< 默认水下雾颜色 (深蓝)
    static constexpr u32 DEFAULT_FOG_COLOR = 0xC0D8FF;       ///< 默认雾颜色 (浅蓝)
    static constexpr u32 DEFAULT_SKY_COLOR = 0x78A7FF;       ///< 默认天空颜色 (蓝色)

    // 特殊生物群系颜色常量
    static constexpr u32 SWAMP_WATER_COLOR = 0x617B64;     ///< 沼泽水体颜色
    static constexpr u32 SWAMP_WATER_FOG_COLOR = 0x232817; ///< 沼泽水下雾颜色
    static constexpr u32 SWAMP_FOG_COLOR = 0x7E8E8E;       ///< 沼泽雾颜色

    static constexpr u32 FROZEN_OCEAN_WATER_COLOR = 0x3938C9; ///< 冻洋水体颜色

    static constexpr u32 WARM_OCEAN_WATER_COLOR = 0x43D5EE;     ///< 暖水海洋水体颜色
    static constexpr u32 WARM_OCEAN_WATER_FOG_COLOR = 0x041F33; ///< 暖水海洋水下雾颜色

    static constexpr u32 LUKEWARM_OCEAN_WATER_COLOR = 0x45ADF2;     ///< 温水海洋水体颜色
    static constexpr u32 LUKEWARM_OCEAN_WATER_FOG_COLOR = 0x0E4673; ///< 温水海洋水下雾颜色

    static constexpr u32 COLD_OCEAN_WATER_COLOR = 0x3D57E6;     ///< 冷水海洋水体颜色
    static constexpr u32 COLD_OCEAN_WATER_FOG_COLOR = 0x1A3AA3; ///< 冷水海洋水下雾颜色

    static constexpr u32 SWAMP_GRASS_COLOR = 0x6A7039;        ///< 沼泽草颜色（浅色）
    static constexpr u32 SWAMP_GRASS_COLOR_DARK = 0x4C613C;   ///< 沼泽草颜色（深色）
    static constexpr u32 SWAMP_FOLIAGE_COLOR = 0x6A7039;      ///< 沼泽树叶颜色（浅色）
    static constexpr u32 SWAMP_FOLIAGE_COLOR_DARK = 0x4C613C; ///< 沼泽树叶颜色（深色）

    static constexpr u32 DARK_FOREST_GRASS_COLOR = 0x507A50; ///< 黑森林草颜色

    static constexpr u32 BADLANDS_GRASS_COLOR = 0x90814D;   ///< 恶地草颜色
    static constexpr u32 BADLANDS_FOLIAGE_COLOR = 0x9E814D; ///< 恶地树叶颜色

    // 对齐 MC 1.21.11 DryFoliageColor.FOLIAGE_DRY_DEFAULT (-10732494 → RGB 0x5C3C32)
    static constexpr u32 DEFAULT_DRY_FOLIAGE_COLOR = 0x5C3C32; ///< 默认干燥树叶颜色 (暗棕)

    /**
     * @brief Builder 模式用于构建 BiomeEffects
     *
     * 使用示例：
     * @code
     * auto effects = BiomeEffects::Builder()
     *     .waterColor(0x3F76E4)
     *     .waterFogColor(0x050533)
     *     .fogColor(0xC0D8FF)
     *     .build();
     * @endcode
     */
    class Builder {
    public:
        Builder& waterColor(u32 color)
        {
            m_waterColor = color;
            return *this;
        }

        Builder& waterFogColor(u32 color)
        {
            m_waterFogColor = color;
            return *this;
        }

        Builder& fogColor(u32 color)
        {
            m_fogColor = color;
            return *this;
        }

        Builder& skyColor(u32 color)
        {
            m_skyColor = color;
            return *this;
        }

        Builder& foliageColor(u32 color)
        {
            m_foliageColor = color;
            return *this;
        }

        Builder& grassColor(u32 color)
        {
            m_grassColor = color;
            return *this;
        }

        Builder& grassColorModifier(GrassColorModifier modifier)
        {
            m_grassColorModifier = modifier;
            return *this;
        }

        Builder& dryFoliageColor(u32 color)
        {
            m_dryFoliageColor = color;
            return *this;
        }

        BiomeEffects build() const
        {
            BiomeEffects effects;
            effects.m_waterColor = m_waterColor;
            effects.m_waterFogColor = m_waterFogColor;
            effects.m_fogColor = m_fogColor;
            effects.m_skyColor = m_skyColor;
            effects.m_foliageColor = m_foliageColor;
            effects.m_grassColor = m_grassColor;
            effects.m_grassColorModifier = m_grassColorModifier;
            effects.m_dryFoliageColor = m_dryFoliageColor;
            return effects;
        }

    private:
        u32 m_waterColor = DEFAULT_WATER_COLOR;
        u32 m_waterFogColor = DEFAULT_WATER_FOG_COLOR;
        u32 m_fogColor = DEFAULT_FOG_COLOR;
        u32 m_skyColor = DEFAULT_SKY_COLOR;
        OptionalColor m_foliageColor;
        OptionalColor m_grassColor;
        GrassColorModifier m_grassColorModifier = GrassColorModifier::None;
        OptionalColor m_dryFoliageColor;
    };

private:
    u32 m_waterColor = DEFAULT_WATER_COLOR;
    u32 m_waterFogColor = DEFAULT_WATER_FOG_COLOR;
    u32 m_fogColor = DEFAULT_FOG_COLOR;
    u32 m_skyColor = DEFAULT_SKY_COLOR;
    OptionalColor m_foliageColor;
    OptionalColor m_grassColor;
    GrassColorModifier m_grassColorModifier = GrassColorModifier::None;
    OptionalColor m_dryFoliageColor;
};

} // namespace biome
} // namespace world
} // namespace mc

// 旧命名空间兼容别名
namespace mc {
using BiomeEffects = ::mc::world::biome::BiomeEffects;
using GrassColorModifier = ::mc::world::biome::GrassColorModifier;
} // namespace mc
