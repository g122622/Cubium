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

#include "common/core/Types.hpp"

namespace mc {
namespace world {
namespace biome {

// ============================================================================
// 预定义生物群系ID常量
// 完整列表，ID 与原版完全一致
// ============================================================================

namespace Biomes {

// 基础生物群系 (0-13)
constexpr BiomeId Ocean = 0;
constexpr BiomeId Plains = 1;
constexpr BiomeId Desert = 2;
constexpr BiomeId Mountains = 3; // extreme_hills
constexpr BiomeId Forest = 4;
constexpr BiomeId Taiga = 5;
constexpr BiomeId Swamp = 6;
constexpr BiomeId River = 7;
constexpr BiomeId NetherWastes = 8;
constexpr BiomeId TheEnd = 9;
constexpr BiomeId FrozenOcean = 10;
constexpr BiomeId FrozenRiver = 11;
constexpr BiomeId SnowyPlains = 12; // snowy_tundra
constexpr BiomeId SnowyMountains = 13;

// 蘑菇岛 (14-15)
constexpr BiomeId MushroomFields = 14;
constexpr BiomeId MushroomFieldShore = 15;

// 海滩 (16)
constexpr BiomeId Beach = 16;

// 山地变体和丘陵 (17-20)
constexpr BiomeId DesertHills = 17;
constexpr BiomeId WoodedHills = 18;
constexpr BiomeId TaigaHills = 19;
constexpr BiomeId MountainEdge = 20; // 已弃用，但 ID 保留

// 丛林 (21-23)
constexpr BiomeId Jungle = 21;
constexpr BiomeId JungleHills = 22;
constexpr BiomeId JungleEdge = 23;

// 深海和石岸 (24-25)
constexpr BiomeId DeepOcean = 24;
constexpr BiomeId StoneShore = 25;

// 雪地海滩 (26)
constexpr BiomeId SnowyBeach = 26;

// 桦木森林 (27-28)
constexpr BiomeId BirchForest = 27;
constexpr BiomeId BirchForestHills = 28;

// 黑森林 (29)
constexpr BiomeId DarkForest = 29;

// 雪地针叶林 (30-31)
constexpr BiomeId SnowyTaiga = 30;
constexpr BiomeId SnowyTaigaHills = 31;

// 大型针叶林 (32-33)
constexpr BiomeId GiantTreeTaiga = 32;
constexpr BiomeId GiantTreeTaigaHills = 33;

// 热带草原 (34-36)
constexpr BiomeId WoodedMountains = 34; // extreme_hills_with_trees
constexpr BiomeId Savanna = 35;
constexpr BiomeId SavannaPlateau = 36;

// 恶地 (37-39)
constexpr BiomeId Badlands = 37;
constexpr BiomeId WoodedBadlandsPlateau = 38;
constexpr BiomeId BadlandsPlateau = 39;

// 末地生物群系 (40-43)
constexpr BiomeId SmallEndIslands = 40;
constexpr BiomeId EndMidlands = 41;
constexpr BiomeId EndHighlands = 42;
constexpr BiomeId EndBarrens = 43;

// 海洋温度变体 (44-50)
constexpr BiomeId WarmOcean = 44;
constexpr BiomeId LukewarmOcean = 45;
constexpr BiomeId ColdOcean = 46;
constexpr BiomeId DeepWarmOcean = 47;
constexpr BiomeId DeepLukewarmOcean = 48;
constexpr BiomeId DeepColdOcean = 49;
constexpr BiomeId DeepFrozenOcean = 50;

// TheVoid (55)
// 56-127 保留

// 变体生物群系（129-169，稀有变体）
constexpr BiomeId SunflowerPlains = 129;
constexpr BiomeId DesertLakes = 130;
constexpr BiomeId GravellyMountains = 131;
constexpr BiomeId FlowerForest = 132;
constexpr BiomeId TaigaMountains = 133;
constexpr BiomeId SwampHills = 134;
// 135-139 保留
constexpr BiomeId IceSpikes = 140;
// 141-148 保留
constexpr BiomeId ModifiedJungle = 149;
// 150 保留
constexpr BiomeId ModifiedJungleEdge = 151;
// 152-154 保留
constexpr BiomeId TallBirchForest = 155;
constexpr BiomeId TallBirchHills = 156;
constexpr BiomeId DarkForestHills = 157;
constexpr BiomeId SnowyTaigaMountains = 158;
// 159 保留
constexpr BiomeId GiantSpruceTaiga = 160;
constexpr BiomeId GiantSpruceTaigaHills = 161;
constexpr BiomeId ModifiedGravellyMountains = 162;
constexpr BiomeId ShatteredSavanna = 163;
constexpr BiomeId ShatteredSavannaPlateau = 164;
constexpr BiomeId ErodedBadlands = 165;
constexpr BiomeId ModifiedWoodedBadlandsPlateau = 166;
constexpr BiomeId ModifiedBadlandsPlateau = 167;
constexpr BiomeId BambooJungle = 168;
constexpr BiomeId BambooJungleHills = 169;

// 下界生物群系 (170-173)
constexpr BiomeId SoulSandValley = 170;
constexpr BiomeId CrimsonForest = 171;
constexpr BiomeId WarpedForest = 172;
constexpr BiomeId BasaltDeltas = 173;

// 新增生物群系 (174-185)
constexpr BiomeId Meadow = 174;
constexpr BiomeId Grove = 175;
constexpr BiomeId SnowySlopes = 176;
constexpr BiomeId JaggedPeaks = 177;
constexpr BiomeId FrozenPeaks = 178;
constexpr BiomeId StonyPeaks = 179;
constexpr BiomeId DripstoneCaves = 180;
constexpr BiomeId LushCaves = 181;
constexpr BiomeId DeepDark = 182;
constexpr BiomeId MangroveSwamp = 183;
constexpr BiomeId CherryGrove = 184;
constexpr BiomeId PaleGarden = 185;

// 重命名生物群系（使用新名称，旧ID不变）
constexpr BiomeId WindsweptHills = Mountains;                 // 原 Mountains (3)
constexpr BiomeId WindsweptForest = WoodedMountains;          // 原 WoodedMountains (34)
constexpr BiomeId WindsweptGravellyHills = GravellyMountains; // 原 GravellyMountains (131)
constexpr BiomeId StonyShore = StoneShore;                    // 原 StoneShore (25)
constexpr BiomeId OldGrowthPineTaiga = GiantTreeTaiga;        // 原 GiantTreeTaiga (32)
constexpr BiomeId OldGrowthSpruceTaiga = GiantSpruceTaiga;    // 原 GiantSpruceTaiga (160)
constexpr BiomeId OldGrowthBirchForest = TallBirchForest;     // 原 TallBirchForest (155)
constexpr BiomeId SparseJungle = JungleEdge;                  // 原 JungleEdge (23)
constexpr BiomeId WoodedBadlands = WoodedBadlandsPlateau;     // 原 WoodedBadlandsPlateau (38)
constexpr BiomeId WindsweptSavanna = ShatteredSavanna;        // 原 ShatteredSavanna (163)

// 生物群系总数（最大 ID + 1）
constexpr BiomeId Count = 186;

/**
 * @brief 判断生物群系是否为海洋或河流类型
 *
 * 水下音乐只在海洋或河流生物群系中播放。
 * TODO: 未来应改用 BiomeTags 标签系统替代此硬编码函数
 *
 * @param biomeId 生物群系ID
 * @return 是否为海洋或河流生物群系
 */
[[nodiscard]] inline bool isOceanOrRiverBiome(BiomeId biomeId)
{
    switch (biomeId) {
        case Ocean:
        case WarmOcean:
        case LukewarmOcean:
        case ColdOcean:
        case FrozenOcean:
        case DeepOcean:
        case DeepWarmOcean:
        case DeepLukewarmOcean:
        case DeepColdOcean:
        case DeepFrozenOcean:
        case River:
        case FrozenRiver:
            return true;
        default:
            return false;
    }
}

} // namespace Biomes

} // namespace biome
} // namespace world
} // namespace mc

// 旧命名空间兼容别名
namespace mc {
namespace Biomes = ::mc::world::biome::Biomes;
} // namespace mc
