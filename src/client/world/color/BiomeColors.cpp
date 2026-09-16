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

#include "BiomeColors.hpp"
#include "client/world/color/ColorResolver.hpp"
#include "common/core/Types.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeClimate.hpp"
#include "common/world/biome/BiomeEffects.hpp"
#include "common/world/gen/noise/PerlinSimplexNoise.hpp"
#include <memory>

namespace mc {
namespace client {

// === 静态成员初始化 ===

std::unique_ptr<GrassColorResolver> BiomeColors::s_grassColorResolver;
std::unique_ptr<FoliageColorResolver> BiomeColors::s_foliageColorResolver;
std::unique_ptr<DryFoliageColorResolver> BiomeColors::s_dryFoliageColorResolver;
std::unique_ptr<WaterColorResolver> BiomeColors::s_waterColorResolver;

// === GrassColorResolver ===

u32 GrassColorResolver::getColor(const Biome& biome, f64 x, f64 z) const noexcept
{
    // 1. 检查是否有覆盖颜色
    auto overrideColor = biome.effects().grassColor();
    if (overrideColor.has_value()) {
        return overrideColor.value();
    }

    // 2. 应用草颜色修改器
    switch (biome.effects().grassColorModifier()) {
        case world::biome::GrassColorModifier::Swamp: {
            // 沼泽使用双色噪声混合
            return BiomeColors::calculateSwampColor(x,
                z,
                world::biome::BiomeEffects::SWAMP_GRASS_COLOR,
                world::biome::BiomeEffects::SWAMP_GRASS_COLOR_DARK);
        }
        case world::biome::GrassColorModifier::DarkForest: {
            // MC 1.21.11: DarkForest 修改器对 colormap 结果应用位运算变暗
            // 公式: (color & 16711422) + 2634762 >> 1
            // 但由于此处 grassColor 已有覆盖值，直接返回覆盖颜色
            // 注意：正确的实现应该是先从 colormap 获取颜色，再应用位运算
            // 当前由于 DarkForest 生物群系同时设置了 grassColor 覆盖，
            // 此处返回覆盖颜色。当 colormap 系统实现后，应改为：
            //   u32 colormapColor = getGrassColormapColor(biome);
            //   return ((colormapColor & 16711422) + 2634762) >> 1;
            return world::biome::BiomeEffects::DARK_FOREST_GRASS_COLOR;
        }
        case world::biome::GrassColorModifier::Badlands:
            // 恶地使用 grassColor 覆盖值，不走修改器逻辑
            // 此分支不应到达（Badlands 生物群系同时设置了 grassColor 覆盖），
            // 但为了安全起见，返回恶地草颜色
            return world::biome::BiomeEffects::BADLANDS_GRASS_COLOR;
        case world::biome::GrassColorModifier::None:
        default:
            // 3. 使用 grass colormap（返回标记值，由调用方处理）
            // 返回 0xFFFFFFFF 表示需要从 colormap 计算
            return 0xFFFFFFFF;
    }
}

// === FoliageColorResolver ===

u32 FoliageColorResolver::getColor(const Biome& biome, f64 x, f64 z) const noexcept
{
    // 1. 检查是否有覆盖颜色
    auto overrideColor = biome.effects().foliageColor();
    if (overrideColor.has_value()) {
        return overrideColor.value();
    }

    // 2. 检查草颜色修改器（沼泽的树叶也使用双色混合）
    switch (biome.effects().grassColorModifier()) {
        case world::biome::GrassColorModifier::Swamp: {
            // 沼泽树叶使用相同的双色混合
            return BiomeColors::calculateSwampColor(x,
                z,
                world::biome::BiomeEffects::SWAMP_FOLIAGE_COLOR,
                world::biome::BiomeEffects::SWAMP_FOLIAGE_COLOR_DARK);
        }
        case world::biome::GrassColorModifier::Badlands: {
            // 恶地树叶颜色
            return world::biome::BiomeEffects::BADLANDS_FOLIAGE_COLOR;
        }
        case world::biome::GrassColorModifier::DarkForest:
        case world::biome::GrassColorModifier::None:
        default:
            // 3. 使用 foliage colormap
            return 0xFFFFFFFF;
    }
}

// === WaterColorResolver ===

u32 WaterColorResolver::getColor(const Biome& biome, f64 x, f64 z) const noexcept
{
    // 水颜色直接从 BiomeEffects 获取
    return biome.effects().waterColor();
}

// === DryFoliageColorResolver ===

u32 DryFoliageColorResolver::getColor(const Biome& biome, f64 x, f64 z) const noexcept
{
    // 1. 检查是否有覆盖颜色
    auto overrideColor = biome.effects().dryFoliageColor();
    if (overrideColor.has_value()) {
        return overrideColor.value();
    }
    // 2. 使用 dry_foliage colormap（返回标记值，由调用方处理）
    // 干枯植被没有沼泽/恶地等修改器分支，无覆盖即走 colormap
    return 0xFFFFFFFF;
}

// === BiomeColors ===

const ColorResolver& BiomeColors::grassColorResolver()
{
    if (!s_grassColorResolver) {
        s_grassColorResolver = std::make_unique<GrassColorResolver>();
    }
    return *s_grassColorResolver;
}

const ColorResolver& BiomeColors::foliageColorResolver()
{
    if (!s_foliageColorResolver) {
        s_foliageColorResolver = std::make_unique<FoliageColorResolver>();
    }
    return *s_foliageColorResolver;
}

const ColorResolver& BiomeColors::dryFoliageColorResolver()
{
    if (!s_dryFoliageColorResolver) {
        s_dryFoliageColorResolver = std::make_unique<DryFoliageColorResolver>();
    }
    return *s_dryFoliageColorResolver;
}

const ColorResolver& BiomeColors::waterColorResolver()
{
    if (!s_waterColorResolver) {
        s_waterColorResolver = std::make_unique<WaterColorResolver>();
    }
    return *s_waterColorResolver;
}

u32 BiomeColors::calculateSwampColor(f64 x, f64 z, u32 color1, u32 color2) noexcept
{
    // MC 1.21.11: 使用 BIOME_INFO_NOISE (seed=2345, octaves=[0]) 在 scale 0.0225 处采样
    // 如果噪声 < -0.1，返回深色 (color2)，否则返回浅色 (color1)
    const f64 noise = world::biome::biomeInfoNoise().getValue(x * 0.0225, z * 0.0225, false);
    return noise < -0.1 ? color2 : color1;
}

} // namespace client
} // namespace mc
