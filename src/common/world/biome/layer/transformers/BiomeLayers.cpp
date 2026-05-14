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
    // 参考 MC 1.16.5 BiomeLayer.apply:
    // int i = (value & 3840) >> 8;  // 提取特殊位
    // value = value & -3841;        // 清除特殊位
    //
    // if (!LayerUtil.isOcean(value) && value != 14) {  // 非海洋且非蘑菇岛
    //     switch(value) {
    //         case 1:  // Warm (DESERT type)
    //             if (i > 0) {
    //                 return context.random(3) == 0 ? 39 : 38;  // wooded_badlands_plateau or badlands_plateau
    //             }
    //             return getBiomeId(DESERT, context);  // 从 DESERT 类型中选择
    //         case 2:  // Medium/Lukewarm (WARM type)
    //             if (i > 0) {
    //                 return 21;  // jungle
    //             }
    //             return getBiomeId(WARM, context);  // 从 WARM 类型中选择
    //         case 3:  // Cool (COOL type)
    //             if (i > 0) {
    //                 return 32;  // birch_forest (实际上应该是 27, 但 MC 中特殊位返回 32=giant_tree_taiga)
    //             }
    //             return getBiomeId(COOL, context);  // 从 COOL 类型中选择
    //         case 4:  // Icy (ICY type)
    //             return getBiomeId(ICY, context);  // 从 ICY 类型中选择
    //         default:
    //             return 14;  // mushroom_fields
    //     }
    // } else {
    //     return value;
    // }

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
            // MC 1.16.5 field_202744_r = {2, 2, 2, 35, 35, 1}
            // Desert(2) x3, Savanna(35) x2, Plains(1) x1
            return WARM_BIOMES[ctx.nextInt(6)];

        case BiomeValues::Climate::Medium: // 2 - WARM type (丛林区域)
            if (special > 0) {
                return BiomeValues::Jungle; // 21
            }
            // MC 1.16.5 field_202743_q = {2, 4, 3, 6, 1, 5}
            // Desert(2), Forest(4), Mountains(3), Swamp(6), Plains(1), Taiga(5)
            // 这是 DESERT_LEGACY 类型，用于旧版生成
            // 新版 WARM 类型: Forest, DarkForest, Mountains, Plains, BirchForest, Swamp (各权重10)
            {
                // 随机选择：森林、黑森林、山地、平原、桦木森林、沼泽
                // 参考 BiomeManager.WARM: Forest(10), DarkForest(10), Mountains(10), Plains(10), BirchForest(10),
                // Swamp(10)
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
                return BiomeValues::GiantTreeTaiga; // 32 (注意：MC 中特殊位返回 32)
            }
            // MC 1.16.5 field_202745_s = {4, 29, 3, 1, 27, 6}
            // Forest(4), DarkForestHills(29), Mountains(3), Plains(1), BirchForest(27), Swamp(6)
            return COOL_BIOMES[ctx.nextInt(6)];

        case BiomeValues::Climate::Icy: // 4 - ICY type
            // MC 1.16.5 field_202747_u = {12, 12, 12, 30}
            // SnowyPlains(12) x3, WoodedMountains(30) x1
            // 实际 BiomeManager.ICY: SnowyTundra(30), SnowyTaiga(10)
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
    // 参考 MC RareBiomeLayer.apply:
    // return context.random(57) == 0 && value == 1 ? 129 : value;

    // 平原 (1) 有 1/57 概率变成向日葵平原 (129)
    if (value == BiomeValues::Plains && ctx.nextInt(57) == 0) {
        return BiomeValues::SunflowerPlains;
    }
    return value;
}

// ============================================================================
// ShoreLayer 实现
// ============================================================================

// MC 1.16.5 雪地生物群系集合
// field_242942_b = {26, 11, 12, 13, 140, 30, 31, 158, 10}
// snowy_beach(26), frozen_river(11), snowy_plains(12), snowy_mountains(13),
// ice_spikes(140), snowy_taiga(30), snowy_taiga_hills(31), snowy_taiga_mountains(158), frozen_ocean(10)
static bool isSnowyBiomeMC(i32 biome)
{
    return biome == 26 || biome == 11 || biome == 12 || biome == 13 || biome == 140 || biome == 30 || biome == 31 ||
        biome == 158 || biome == 10;
}

// MC 1.16.5 丛林生物群系集合
// field_242943_c = {168, 169, 21, 22, 23, 149, 151}
// bamboo_jungle(168), bamboo_jungle_hills(169), jungle(21), jungle_hills(22),
// jungle_edge(23), modified_jungle(149), modified_jungle_edge(151)
static bool isJungleBiomeMC(i32 biome)
{
    return biome == 168 || biome == 169 || biome == 21 || biome == 22 || biome == 23 || biome == 149 || biome == 151;
}

// MC 1.16.5 丛林兼容生物群系列表
static bool isJungleCompatibleMC(i32 biome)
{
    return isJungleBiomeMC(biome) || biome == 4 || biome == 5 || BiomeValues::isOcean(biome);
    // 丛林兼容: 丛林类、森林(4)、针叶林(5)、海洋
}

// MC 1.16.5 恶地生物群系集合
static bool isMesaBiomeMC(i32 biome)
{
    return biome == 37 || biome == 38 || biome == 39 || // badlands, wooded_badlands_plateau, badlands_plateau
        biome == 165 || biome == 166 ||
        biome == 167; // eroded_badlands, modified_wooded_badlands_plateau, modified_badlands_plateau
}

i32 ShoreLayer::apply(IAreaContext& ctx, i32 north, i32 east, i32 south, i32 west, i32 center)
{
    (void)ctx; // 不使用

    // 参考 MC 1.16.5 ShoreLayer.apply

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
    // 参考 MC SmoothLayer.apply:
    // boolean flag = west == east;
    // boolean flag1 = north == south;
    // if (flag == flag1) {
    //     if (flag) {
    //         return context.random(2) == 0 ? east : north;
    //     } else {
    //         return center;
    //     }
    // } else {
    //     return flag ? east : north;
    // }

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
