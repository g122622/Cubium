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

#include "../../../core/Types.hpp"

namespace mc {
namespace layer {

/**
 * @brief Layer 系统内部使用的生物群系值
 *
 * 这些值用于 Layer 系统的中间处理，最终会被映射到实际的 BiomeId。
 * 参考 MC 1.16.5 LayerUtil 和各个 Layer 类中使用的值。
 *
 * 注意：这些值与 Biomes 命名空间中的 ID 是相同的，因为 MC 直接使用 ID 进行处理。
 *
 * MC 1.16.5 biome IDs 参考:
 * https://minecraft.gamepedia.com/Java_Edition_1.16.5
 */
namespace BiomeValues {

// ============================================================================
// 基础生物群系 ID（与 MC 1.16.5 完全一致）
// ============================================================================

// 基础生物群系 (0-9)
constexpr i32 Ocean = 0;
constexpr i32 Plains = 1;
constexpr i32 Desert = 2;
constexpr i32 Mountains = 3; // extreme_hills
constexpr i32 Forest = 4;
constexpr i32 Taiga = 5;
constexpr i32 Swamp = 6;
constexpr i32 River = 7;
constexpr i32 NetherWastes = 8;
constexpr i32 TheEnd = 9;

// 寒冷/雪地生物群系 (10-13)
constexpr i32 FrozenOcean = 10;
constexpr i32 FrozenRiver = 11;
constexpr i32 SnowyPlains = 12; // snowy_tundra
constexpr i32 SnowyMountains = 13;

// 蘑菇岛 (14-15)
constexpr i32 MushroomFields = 14;
constexpr i32 MushroomFieldShore = 15;

// 海滩 (16)
constexpr i32 Beach = 16;

// 山地变体和丘陵 (17-20)
constexpr i32 DesertHills = 17;
constexpr i32 WoodedHills = 18; // 也称作 wooded_hills
constexpr i32 TaigaHills = 19;
constexpr i32 MountainEdge = 20; // MC 中已弃用，但 ID 保留

// 丛林 (21-23)
constexpr i32 Jungle = 21;
constexpr i32 JungleHills = 22;
constexpr i32 JungleEdge = 23;

// 深海和石岸 (24-25)
constexpr i32 DeepOcean = 24;
constexpr i32 StoneShore = 25;

// 雪地海滩 (26)
constexpr i32 SnowyBeach = 26;

// 桦木森林 (27-28)
constexpr i32 BirchForest = 27;
constexpr i32 BirchForestHills = 28;

// 黑森林 (29)
constexpr i32 DarkForest = 29;

// 雪地针叶林 (30-31)
constexpr i32 SnowyTaiga = 30;
constexpr i32 SnowyTaigaHills = 31;

// 大型针叶林 (32-33)
constexpr i32 GiantTreeTaiga = 32;
constexpr i32 GiantTreeTaigaHills = 33;

// 热带草原 (34-36)
constexpr i32 WoodedMountains = 34; // extreme_hills_with_trees
constexpr i32 Savanna = 35;
constexpr i32 SavannaPlateau = 36;

// 恶地 (37-39)
constexpr i32 Badlands = 37;
constexpr i32 WoodedBadlandsPlateau = 38;
constexpr i32 BadlandsPlateau = 39;

// 小型生物群系 (40-43)
constexpr i32 SmallEndIslands = 40;
constexpr i32 EndMidlands = 41;
constexpr i32 EndHighlands = 42;
constexpr i32 EndBarrens = 43;

// 海洋温度变体 (44-50)
constexpr i32 WarmOcean = 44;
constexpr i32 LukewarmOcean = 45;
constexpr i32 ColdOcean = 46;
constexpr i32 DeepWarmOcean = 47;
constexpr i32 DeepLukewarmOcean = 48;
constexpr i32 DeepColdOcean = 49;
constexpr i32 DeepFrozenOcean = 50;

// 变体生物群系（129-167，稀有变体）
constexpr i32 SunflowerPlains = 129;
constexpr i32 DesertLakes = 130;
constexpr i32 GravellyMountains = 131;
constexpr i32 FlowerForest = 132;
constexpr i32 TaigaMountains = 133;
constexpr i32 SwampHills = 134;
constexpr i32 IceSpikes = 140;
constexpr i32 ModifiedJungle = 149;
constexpr i32 ModifiedJungleEdge = 151;
constexpr i32 TallBirchForest = 155;
constexpr i32 TallBirchHills = 156;
constexpr i32 DarkForestHills = 157;
constexpr i32 SnowyTaigaMountains = 158;
constexpr i32 GiantSpruceTaiga = 160;
constexpr i32 GiantSpruceTaigaHills = 161;
constexpr i32 ModifiedGravellyMountains = 162;
constexpr i32 ShatteredSavanna = 163;
constexpr i32 ShatteredSavannaPlateau = 164;
constexpr i32 ErodedBadlands = 165;
constexpr i32 ModifiedWoodedBadlandsPlateau = 166;
constexpr i32 ModifiedBadlandsPlateau = 167;
constexpr i32 BambooJungle = 168;
constexpr i32 BambooJungleHills = 169;

// 下界生物群系 (170-173，MC 1.16 新增)
constexpr i32 SoulSandValley = 170;
constexpr i32 CrimsonForest = 171;
constexpr i32 WarpedForest = 172;
constexpr i32 BasaltDeltas = 173;

// ============================================================================
// 温度区域值（Layer 内部使用）
// 这些值在 AddSnowLayer 中生成，在 BiomeLayer 中转换为实际生物群系
// ============================================================================

namespace Climate {
constexpr i32 Ocean = 0;  // 海洋（保持不变）
constexpr i32 Warm = 1;   // 温暖区域（沙漠、热带草原等）
constexpr i32 Medium = 2; // 中等温度（平原、森林等）
constexpr i32 Cool = 3;   // 凉爽区域（针叶林等）
constexpr i32 Icy = 4;    // 冰冻区域（雪地）
} // namespace Climate

// ============================================================================
// 特殊位标记
// ============================================================================

/**
 * @brief 特殊变体位掩码
 *
 * 在 EdgeLayer.Special 中使用，用于生成稀有变体。
 * 值存储在 bits 8-11 (mask 0xF00)
 */
namespace SpecialBits {
constexpr i32 Mask = 0xF00; // bits 8-11
constexpr i32 Shift = 8;    // 右移位数

/**
 * @brief 提取特殊变体索引
 * @param value 层值
 * @return 特殊变体索引 (0-15)
 */
inline i32 extract(i32 value)
{
    return (value & Mask) >> Shift;
}

/**
 * @brief 设置特殊变体位
 * @param value 层值
 * @param special 特殊变体索引 (0-15)
 * @return 带有特殊位的值
 */
inline i32 set(i32 value, i32 special)
{
    return (value & ~Mask) | ((special << Shift) & Mask);
}
} // namespace SpecialBits

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 检查是否为海洋生物群系（包括深海）
 */
inline bool isOcean(i32 biome)
{
    return biome == Ocean || biome == DeepOcean || biome == WarmOcean || biome == LukewarmOcean || biome == ColdOcean ||
        biome == FrozenOcean || biome == DeepWarmOcean || biome == DeepLukewarmOcean || biome == DeepColdOcean ||
        biome == DeepFrozenOcean;
}

/**
 * @brief 检查是否为浅海洋生物群系（不包括深海）
 */
inline bool isShallowOcean(i32 biome)
{
    return biome == Ocean || biome == WarmOcean || biome == LukewarmOcean || biome == ColdOcean || biome == FrozenOcean;
}

/**
 * @brief 检查两个生物群系是否相似（属于同一类别）
 *
 * 参考 MC LayerUtil.areBiomesSimilar
 */
bool areBiomesSimilar(i32 a, i32 b);

// ============================================================================
// 生物群系类别检查函数（用于 Layer 处理）
// ============================================================================

/**
 * @brief 检查是否为恶地（Badlands/Mesa）类别
 * @param biome 生物群系 ID
 * @return 是否为恶地类别
 */
[[nodiscard]] inline bool isBadlands(i32 biome)
{
    return biome == Badlands || biome == WoodedBadlandsPlateau || biome == BadlandsPlateau || biome == ErodedBadlands ||
        biome == ModifiedWoodedBadlandsPlateau || biome == ModifiedBadlandsPlateau;
}

/**
 * @brief 检查是否为丛林类别
 * @param biome 生物群系 ID
 * @return 是否为丛林类别
 */
[[nodiscard]] inline bool isJungle(i32 biome)
{
    return biome == Jungle || biome == JungleHills || biome == JungleEdge || biome == BambooJungle ||
        biome == BambooJungleHills || biome == ModifiedJungle || biome == ModifiedJungleEdge;
}

/**
 * @brief 检查是否与丛林兼容（丛林、森林、针叶林、海洋）
 * @param biome 生物群系 ID
 * @return 是否与丛林兼容
 */
[[nodiscard]] inline bool isJungleCompatible(i32 biome)
{
    return isJungle(biome) || biome == Forest || biome == Taiga || isOcean(biome);
}

/**
 * @brief 检查是否为雪地类别
 * @param biome 生物群系 ID
 * @return 是否为雪地类别
 */
[[nodiscard]] inline bool isSnowy(i32 biome)
{
    return biome == SnowyPlains || biome == SnowyMountains || biome == SnowyBeach || biome == SnowyTaiga ||
        biome == SnowyTaigaHills || biome == SnowyTaigaMountains || biome == IceSpikes || biome == FrozenOcean ||
        biome == DeepFrozenOcean || biome == FrozenRiver;
}

/**
 * @brief 检查是否为山地类别
 * @param biome 生物群系 ID
 * @return 是否为山地类别
 */
[[nodiscard]] inline bool isMountain(i32 biome)
{
    return biome == Mountains || biome == WoodedMountains || biome == GravellyMountains ||
        biome == ModifiedGravellyMountains || biome == MountainEdge; // MountainEdge 在 MC 中是 ID 20
}

/**
 * @brief 检查周围是否有海洋邻居
 * @param north 北边值
 * @param east 东边值
 * @param south 南边值
 * @param west 西边值
 * @param shallowOnly 是否只检查浅海
 * @return 是否有海洋邻居
 */
[[nodiscard]] inline bool hasOceanNeighbor(i32 north, i32 east, i32 south, i32 west, bool shallowOnly = false)
{
    if (shallowOnly) {
        return isShallowOcean(north) || isShallowOcean(east) || isShallowOcean(south) || isShallowOcean(west);
    }
    return isOcean(north) || isOcean(east) || isOcean(south) || isOcean(west);
}

} // namespace BiomeValues
} // namespace layer
} // namespace mc
