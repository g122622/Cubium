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

#include "BiomeLayers.hpp"

namespace mc {
namespace layer {

// ============================================================================
// BiomeLayer 实现
// ============================================================================

constexpr i32 BiomeLayer::WARM_BIOMES[];
constexpr i32 BiomeLayer::COOL_BIOMES[];
constexpr i32 BiomeLayer::ICY_BIOMES[];

BiomeLayer::BiomeLayer(const Config& config)
    : m_config(config)
{}

i32 BiomeLayer::apply(IAreaContext& ctx, i32 value)
{
    // 提取特殊位 (bits 8-11)
    i32 special = BiomeValues::SpecialBits::extract(value);
    value = value & ~BiomeValues::SpecialBits::Mask;

    // 海洋和蘑菇岛保持不变
    if (BiomeValues::isOcean(value) || value == BiomeValues::MushroomFields) {
        return value;
    }

    switch (value) {
        case BiomeValues::Climate::Warm: // 1 - DESERT type
            if (special > 0) {
                // 有特殊位，返回恶地变体
                return ctx.nextInt(3) == 0 ? BiomeValues::WoodedBadlandsPlateau : BiomeValues::BadlandsPlateau;
            }
            // Desert x3, Savanna x2, Plains x1
            return WARM_BIOMES[ctx.nextInt(6)];

        case BiomeValues::Climate::Medium: // 2 - WARM type (丛林区域)
            if (special > 0) {
                return BiomeValues::Jungle; // 21
            }
            {
                // 随机选择：森林、黑森林、山地、平原、桦木森林、沼泽
                // 每种各权重10
                i32 rnd = ctx.nextInt(60);
                if (rnd < 10) return BiomeValues::Forest;      // 10/60
                if (rnd < 20) return BiomeValues::DarkForest;  // 10/60
                if (rnd < 30) return BiomeValues::Mountains;   // 10/60
                if (rnd < 40) return BiomeValues::Plains;      // 10/60
                if (rnd < 50) return BiomeValues::BirchForest; // 10/60
                return BiomeValues::Swamp;                     // 10/60
            }

        case BiomeValues::Climate::Cool: // 3 - COOL type
            if (special > 0) {
                return BiomeValues::GiantTreeTaiga; // 32
            }
            return COOL_BIOMES[ctx.nextInt(6)];

        case BiomeValues::Climate::Icy: // 4 - ICY type
            // SnowyPlains x3, WoodedMountains x1
            return ICY_BIOMES[ctx.nextInt(4)];

        default:
            // 未知值，返回蘑菇岛
            return BiomeValues::MushroomFields;
    }
}

// ============================================================================
// RareBiomeLayer 实现
// ============================================================================

i32 RareBiomeLayer::apply(IAreaContext& ctx, i32 value)
{
    // 平原 (1) 有 1/57 概率变成向日葵平原 (129)
    if (value == BiomeValues::Plains && ctx.nextInt(57) == 0) {
        return BiomeValues::SunflowerPlains;
    }
    return value;
}

// ============================================================================
// ShoreLayer 实现
// ============================================================================

// 雪地生物群系集合
static bool isSnowyBiomeMC(i32 biome)
{
    return biome == 26 || biome == 11 || biome == 12 || biome == 13 || biome == 140 || biome == 30 || biome == 31 ||
        biome == 158 || biome == 10;
}

// 丛林生物群系集合
static bool isJungleBiomeMC(i32 biome)
{
    return biome == 168 || biome == 169 || biome == 21 || biome == 22 || biome == 23 || biome == 149 || biome == 151;
}

// 丛林兼容生物群系列表
static bool isJungleCompatibleMC(i32 biome)
{
    return isJungleBiomeMC(biome) || biome == 4 || biome == 5 || BiomeValues::isOcean(biome);
}

// 恶地生物群系集合
static bool isMesaBiomeMC(i32 biome)
{
    return biome == 37 || biome == 38 || biome == 39 || // badlands, wooded_badlands_plateau, badlands_plateau
        biome == 165 || biome == 166 ||
        biome == 167; // eroded_badlands, modified_wooded_badlands_plateau, modified_badlands_plateau
}

i32 ShoreLayer::apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center)
{
    (void)ctx; // 不使用

    // 蘑菇岛 -> 蘑菇岛海岸
    if (center == BiomeValues::MushroomFields) { // 14
        if (BiomeValues::isShallowOcean(north) || BiomeValues::isShallowOcean(east) ||
            BiomeValues::isShallowOcean(south) || BiomeValues::isShallowOcean(west)) {
            return BiomeValues::MushroomFieldShore; // 15
        }
    }
    // 丛林类 -> 丛林边缘或海滩
    else if (isJungleBiomeMC(center)) {
        if (!isJungleCompatibleMC(north) || !isJungleCompatibleMC(east) || !isJungleCompatibleMC(south) ||
            !isJungleCompatibleMC(west)) {
            return BiomeValues::JungleEdge; // 23
        }
        if (BiomeValues::isOcean(north) || BiomeValues::isOcean(east) || BiomeValues::isOcean(south) ||
            BiomeValues::isOcean(west)) {
            return BiomeValues::Jungle; // 21 - 海洋相邻时保持丛林
        }
    }
    // 山地、恶地高原、山地边缘 -> 石岸
    else if (center != BiomeValues::Mountains && center != BiomeValues::WoodedMountains &&
        center != BiomeValues::MountainEdge) {
        // 雪地 -> 雪地海滩
        if (isSnowyBiomeMC(center) && !BiomeValues::isOcean(center)) {
            if (BiomeValues::isOcean(north) || BiomeValues::isOcean(east) || BiomeValues::isOcean(south) ||
                BiomeValues::isOcean(west)) {
                return BiomeValues::SnowyBeach; // 26
            }
        }
        // 恶地高原边缘 -> 沙漠
        else if (center != BiomeValues::WoodedBadlandsPlateau && center != BiomeValues::BadlandsPlateau) {
            // 普通生物群系 -> 海滩
            if (!BiomeValues::isOcean(center) && center != BiomeValues::River && center != BiomeValues::Swamp) {
                if (BiomeValues::isOcean(north) || BiomeValues::isOcean(east) || BiomeValues::isOcean(south) ||
                    BiomeValues::isOcean(west)) {
                    return BiomeValues::Beach; // 16
                }
            }
        }
        // 恶地高原 -> 石岸（海洋相邻时）
        else if (BiomeValues::isOcean(north) || BiomeValues::isOcean(east) || BiomeValues::isOcean(south) ||
            BiomeValues::isOcean(west)) {
            return BiomeValues::StoneShore; // 25
        }
    }
    // 山地类 -> 石岸
    else if (!BiomeValues::isOcean(center)) {
        if (BiomeValues::isOcean(north) || BiomeValues::isOcean(east) || BiomeValues::isOcean(south) ||
            BiomeValues::isOcean(west)) {
            return BiomeValues::StoneShore; // 25
        }
    }
    // 恶地（37/165）-> 沙漠（非恶地邻居或非恶地海洋邻居时）
    else if (center == BiomeValues::Badlands || center == BiomeValues::ErodedBadlands) {
        if (!BiomeValues::isOcean(north) && !BiomeValues::isOcean(east) && !BiomeValues::isOcean(south) &&
            !BiomeValues::isOcean(west) &&
            (!isMesaBiomeMC(north) || !isMesaBiomeMC(east) || !isMesaBiomeMC(south) || !isMesaBiomeMC(west))) {
            return BiomeValues::Desert; // 2
        }
    }

    return center;
}

// ============================================================================
// SmoothLayer 实现
// ============================================================================

i32 SmoothLayer::apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center)
{
    bool ewEqual = (east == west);
    bool nsEqual = (north == south);

    if (ewEqual == nsEqual) {
        if (ewEqual) {
            // 东西相等且南北相等，随机选择
            return ctx.pickRandom(east, north);
        } else {
            // 都不相等，保持中心
            return center;
        }
    } else {
        // 其中一对相等
        return ewEqual ? east : north;
    }
}

} // namespace layer
} // namespace mc
