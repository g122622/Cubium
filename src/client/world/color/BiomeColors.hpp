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

#include "ColorResolver.hpp"
#include "common/core/Types.hpp"
#include "common/world/biome/Biome.hpp"
#include <memory>

namespace mc {
namespace world::biome {
class Biome; // 前向声明
} // namespace world::biome
} // namespace mc

namespace mc {
namespace client {

/**
 * @brief 草颜色解析器
 *
 * 从生物群系获取草颜色：
 * 1. 如果生物群系有覆盖颜色，使用覆盖颜色
 * 2. 如果有草颜色修改器（沼泽/黑森林/恶地），应用修改器
 * 3. 否则使用 grass colormap（需要外部实现）
 */
class GrassColorResolver : public ColorResolver {
public:
    [[nodiscard]] u32 getColor(const Biome& biome, f64 x, f64 z) const noexcept override;
};

/**
 * @brief 树叶颜色解析器
 *
 * 从生物群系获取树叶颜色：
 * 1. 如果生物群系有覆盖颜色，使用覆盖颜色
 * 2. 否则使用 foliage colormap（需要外部实现）
 */
class FoliageColorResolver : public ColorResolver {
public:
    [[nodiscard]] u32 getColor(const Biome& biome, f64 x, f64 z) const noexcept override;
};

/**
 * @brief 干枯植被颜色解析器
 *
 * 对应原版 BiomeColors.DRY_FOLIAGE_COLOR_RESOLVER，仅用于 leaf_litter 着色。
 * 从生物群系获取干枯植被颜色：
 * 1. 如果生物群系有 dryFoliageColor 覆盖，使用覆盖颜色
 * 2. 否则使用 dry_foliage colormap（返回 0xFFFFFFFF 标记，由调用方处理）
 *
 * 与 grass/foliage 不同，dry foliage 没有沼泽/恶地等修改器分支。
 */
class DryFoliageColorResolver : public ColorResolver {
public:
    [[nodiscard]] u32 getColor(const Biome& biome, f64 x, f64 z) const noexcept override;
};

/**
 * @brief 水颜色解析器
 *
 * 从生物群系获取水体颜色。
 * 水颜色直接存储在 BiomeEffects 中，不需要 colormap。
 */
class WaterColorResolver : public ColorResolver {
public:
    [[nodiscard]] u32 getColor(const Biome& biome, f64 x, f64 z) const noexcept override;
};

/**
 * @brief 生物群系颜色常量和工具函数
 *
 * 提供全局颜色解析器实例和特殊生物群系颜色常量。
 *
 * 注意：草/树叶颜色常量定义在 BiomeEffects.hpp 中，避免重复。
 * 这里只定义云杉和桦树叶颜色（这些是固定值，不属于 BiomeEffects）。
 */
class BiomeColors {
public:
    /**
     * @brief 获取草颜色解析器
     * @return 草颜色解析器单例引用
     */
    [[nodiscard]] static const ColorResolver& grassColorResolver();

    /**
     * @brief 获取树叶颜色解析器
     * @return 树叶颜色解析器单例引用
     */
    [[nodiscard]] static const ColorResolver& foliageColorResolver();

    /**
     * @brief 获取干枯植被颜色解析器
     * @return 干枯植被颜色解析器单例引用
     */
    [[nodiscard]] static const ColorResolver& dryFoliageColorResolver();

    /**
     * @brief 获取水颜色解析器
     * @return 水颜色解析器单例引用
     */
    [[nodiscard]] static const ColorResolver& waterColorResolver();

    // === 云杉和桦树颜色常量 ===
    // 这些是固定值，不从 colormap 获取，也不属于 BiomeEffects

    static constexpr u32 SPRUCE_LEAVES_COLOR = 0x619961; ///< 云杉树叶颜色
    static constexpr u32 BIRCH_LEAVES_COLOR = 0x80A755;  ///< 桦树树叶颜色

    // 注意：沼泽、黑森林、恶地的颜色常量定义在 BiomeEffects.hpp 中
    // 使用 world::biome::BiomeEffects::SWAMP_GRASS_COLOR 等

    /**
     * @brief 计算沼泽颜色（双色噪声混合）
     *
     * 沼泽的草和树叶颜色使用双色调混合，基于位置噪声在两种颜色间插值。
     *
     * @param x X坐标
     * @param z Z坐标
     * @param color1 第一种颜色
     * @param color2 第二种颜色
     * @return 混合后的颜色
     */
    [[nodiscard]] static u32 calculateSwampColor(f64 x, f64 z, u32 color1, u32 color2) noexcept;

private:
    // 单例实例
    static std::unique_ptr<GrassColorResolver> s_grassColorResolver;
    static std::unique_ptr<FoliageColorResolver> s_foliageColorResolver;
    static std::unique_ptr<DryFoliageColorResolver> s_dryFoliageColorResolver;
    static std::unique_ptr<WaterColorResolver> s_waterColorResolver;
};

} // namespace client
} // namespace mc
