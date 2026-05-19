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
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeEffects.hpp"
#include <cmath>

namespace mc {
namespace client {

// === 静态成员初始化 ===

std::unique_ptr<GrassColorResolver> BiomeColors::s_grassColorResolver;
std::unique_ptr<FoliageColorResolver> BiomeColors::s_foliageColorResolver;
std::unique_ptr<WaterColorResolver> BiomeColors::s_waterColorResolver;

// === GrassColorResolver ===

u32 GrassColorResolver::getColor(const Biome& biome, f64 x, f64 z) const
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
            // 黑森林：草颜色变暗
            return world::biome::BiomeEffects::DARK_FOREST_GRASS_COLOR;
        }
        case world::biome::GrassColorModifier::Badlands: {
            // 恶地：特殊黄褐色
            return world::biome::BiomeEffects::BADLANDS_GRASS_COLOR;
        }
        case world::biome::GrassColorModifier::None:
        default:
            // 3. 使用 grass colormap（返回标记值，由调用方处理）
            // 返回 0xFFFFFFFF 表示需要从 colormap 计算
            return 0xFFFFFFFF;
    }
}

// === FoliageColorResolver ===

u32 FoliageColorResolver::getColor(const Biome& biome, f64 x, f64 z) const
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

u32 WaterColorResolver::getColor(const Biome& biome, f64 x, f64 z) const
{
    // 水颜色直接从 BiomeEffects 获取
    return biome.waterColor();
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

const ColorResolver& BiomeColors::waterColorResolver()
{
    if (!s_waterColorResolver) {
        s_waterColorResolver = std::make_unique<WaterColorResolver>();
    }
    return *s_waterColorResolver;
}

u32 BiomeColors::calculateSwampColor(f64 x, f64 z, u32 color1, u32 color2)
{
    // MC 1.16.5 沼泽颜色算法
    // 使用 2D Perlin 噪声变体进行双色混合
    // 简化实现：使用正弦函数模拟噪声

    // MC 使用改进的 Perlin 噪声
    // 这里使用简化版本：基于坐标的确定性伪随机

    // 参考：BiomeColors.getGrassColor(SwampBiome)
    // 实际 MC 实现：
    // double noise = PerlinNoiseGenerator.getValue(x * 0.0225, z * 0.0225);
    // return noise < -0.1 ? 0x4C6139 : 0x6A7039;

    // 简化实现：使用确定性哈希
    const i64 seed = static_cast<i64>(std::floor(x * 0.0225)) * 31337 + static_cast<i64>(std::floor(z * 0.0225)) * 7919;

    // 简单的哈希函数
    i64 hash = seed;
    hash = (hash ^ (hash >> 30)) * 0xBF58476D1CE4E5B9LL;
    hash = (hash ^ (hash >> 27)) * 0x94D049BB133111EBLL;
    hash = hash ^ (hash >> 31);

    // 将哈希值映射到 [-1, 1] 范围
    const f64 noise =
        static_cast<f64>(hash & 0x7FFFFFFFFFFFFFFFLL) / static_cast<f64>(0x7FFFFFFFFFFFFFFFLL) * 2.0 - 1.0;

    // 根据噪声值选择颜色
    return noise < -0.1 ? color2 : color1;
}

} // namespace client
} // namespace mc
