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

/**
 * @file FeatureIds.hpp
 * @brief 特征ID常量定义
 *
 * 定义所有注册到 FeatureRegistry 的特征ID。
 * 特征按装饰阶段分组，每个阶段内部从0开始编号。
 *
 * 注意：这些ID必须与 FeatureRegistry::initialize() 中注册的顺序一致。
 */

#include "../../../core/Types.hpp"

namespace mc {

// ============================================================================
// UndergroundOres 阶段特征ID
// ============================================================================

namespace OreFeatureIds {
constexpr u32 CoalOre = 0;     // 煤矿
constexpr u32 IronOre = 1;     // 铁矿
constexpr u32 GoldOre = 2;     // 金矿
constexpr u32 RedstoneOre = 3; // 红石矿
constexpr u32 DiamondOre = 4;  // 钻石矿
constexpr u32 LapisOre = 5;    // 青金石矿
constexpr u32 EmeraldOre = 6;  // 绿宝石矿
constexpr u32 CopperOre = 7;   // 铜矿
// 下界矿石 (8-10)
constexpr u32 NetherQuartzOre = 8; // 下界石英矿
constexpr u32 NetherGoldOre = 9;   // 下界金矿
constexpr u32 AncientDebris = 10;  // 远古残骸
constexpr u32 Count = 11;          // 矿石特征总数
} // namespace OreFeatureIds

// ============================================================================
// Lakes 阶段特征ID
// ============================================================================

namespace LakeFeatureIds {
constexpr u32 WaterLake = 0; // 水湖
constexpr u32 LavaLake = 1;  // 熔岩湖
constexpr u32 Count = 2;     // 湖泊特征总数
} // namespace LakeFeatureIds

// ============================================================================
// VegetalDecoration 阶段特征ID
// 注意：ID在阶段内部从0开始，按注册顺序递增
// ============================================================================

namespace TreeFeatureIds {
// 树木特征 (0-14)
// 注意：ID必须与 TreeFeatures::initialize() 中注册的顺序一致
constexpr u32 OakTree = 0;         // 橡树
constexpr u32 BirchTree = 1;       // 白桦
constexpr u32 SpruceTree = 2;      // 云杉
constexpr u32 JungleTree = 3;      // 丛林树
constexpr u32 AcaciaTree = 4;      // 金合欢
constexpr u32 DarkOakTree = 5;     // 深色橡树
constexpr u32 SparseOakTree = 6;   // 稀疏橡树
constexpr u32 GiantSpruceTree = 7; // 巨型云杉
constexpr u32 GiantJungleTree = 8; // 巨型丛林木
constexpr u32 FancyOakTree = 9;    // 精美橡树
constexpr u32 PineTree = 10;       // 松树
constexpr u32 JungleBush = 11;     // 丛林灌木
constexpr u32 SwampTree = 12;      // 沼泽橡树
constexpr u32 MegaPineTree = 13;   // 巨型松树
constexpr u32 TallBirchTree = 14;  // 高白桦
constexpr u32 Count = 15;          // 树木特征总数
} // namespace TreeFeatureIds

namespace FlowerFeatureIds {
// 花卉特征 (5-9)
// 基础偏移量 = TreeFeatureIds::Count = 5
constexpr u32 Offset = TreeFeatureIds::Count;
constexpr u32 PlainsFlowers = 0 + Offset;       // 平原花卉
constexpr u32 ForestFlowers = 1 + Offset;       // 森林花卉
constexpr u32 FlowerForestFlowers = 2 + Offset; // 繁花森林花卉
constexpr u32 SwampFlowers = 3 + Offset;        // 沼泽花卉
constexpr u32 Sunflower = 4 + Offset;           // 向日葵
constexpr u32 Count = 5;                        // 花卉特征总数
} // namespace FlowerFeatureIds

namespace GrassFeatureIds {
// 草丛特征 (10-16)
// 基础偏移量 = TreeFeatureIds::Count + FlowerFeatureIds::Count = 10
constexpr u32 Offset = TreeFeatureIds::Count + FlowerFeatureIds::Count;
constexpr u32 PlainsGrass = 0 + Offset;      // 平原草丛
constexpr u32 ForestGrass = 1 + Offset;      // 森林草丛
constexpr u32 JungleGrass = 2 + Offset;      // 丛林草丛
constexpr u32 SwampGrass = 3 + Offset;       // 沼泽草丛
constexpr u32 SavannaGrass = 4 + Offset;     // 稀树草原草丛
constexpr u32 TaigaGrass = 5 + Offset;       // 针叶林草丛
constexpr u32 BadlandsDeadBush = 6 + Offset; // 恶地枯萎灌木
constexpr u32 Count = 7;                     // 草丛特征总数
} // namespace GrassFeatureIds

namespace MushroomFeatureIds {
// 巨型蘑菇特征 (17-18)
// 基础偏移量 = TreeFeatureIds::Count + FlowerFeatureIds::Count + GrassFeatureIds::Count = 17
constexpr u32 Offset = TreeFeatureIds::Count + FlowerFeatureIds::Count + GrassFeatureIds::Count;
constexpr u32 BrownMushroom = 0 + Offset; // 棕色巨型蘑菇
constexpr u32 RedMushroom = 1 + Offset;   // 红色巨型蘑菇
constexpr u32 Count = 2;                  // 蘑菇特征总数
} // namespace MushroomFeatureIds

namespace CactusFeatureIds {
// 仙人掌特征 (19-20)
constexpr u32 Offset =
    TreeFeatureIds::Count + FlowerFeatureIds::Count + GrassFeatureIds::Count + MushroomFeatureIds::Count;
constexpr u32 DesertCactus = 0 + Offset;   // 沙漠仙人掌
constexpr u32 BadlandsCactus = 1 + Offset; // 恶地仙人掌
constexpr u32 Count = 2;                   // 仙人掌特征总数
} // namespace CactusFeatureIds

namespace SugarCaneFeatureIds {
// 甘蔗特征 (21-22)
constexpr u32 Offset = TreeFeatureIds::Count + FlowerFeatureIds::Count + GrassFeatureIds::Count +
    MushroomFeatureIds::Count + CactusFeatureIds::Count;
constexpr u32 Normal = 0 + Offset; // 普通甘蔗
constexpr u32 Dense = 1 + Offset;  // 密集甘蔗
constexpr u32 Count = 2;           // 甘蔗特征总数
} // namespace SugarCaneFeatureIds

// ============================================================================
// SurfaceStructures 阶段特征ID
// ============================================================================

namespace IceSpikeFeatureIds {
// 冰刺特征 (0-1)
constexpr u32 Spike = 0;   // 尖塔型冰刺
constexpr u32 Iceberg = 1; // 冰丘
constexpr u32 Count = 2;   // 冰刺特征总数
} // namespace IceSpikeFeatureIds

namespace EndSurfaceFeatureIds {
// 末地地表结构特征（紧跟冰刺特征之后）
constexpr u32 Offset = IceSpikeFeatureIds::Count;
constexpr u32 ObsidianSpike = 0 + Offset;  // 黑曜石柱
constexpr u32 EndGateway = 1 + Offset;     // 末地折跃门
constexpr u32 EndGatewayExit = 2 + Offset; // 退出折跃门
constexpr u32 Count = 3;                   // 末地地表结构特征总数
} // namespace EndSurfaceFeatureIds

// ============================================================================
// UndergroundDecoration 阶段特征ID（下界特征）
// ============================================================================

namespace GlowstoneFeatureIds {
// 萤石簇特征 (0-1)
constexpr u32 Normal = 0; // 普通萤石簇
constexpr u32 Large = 1;  // 大型萤石簇
constexpr u32 Count = 2;  // 萤石特征总数
} // namespace GlowstoneFeatureIds

namespace BasaltFeatureIds {
// 玄武岩特征 (2-4)
constexpr u32 Offset = GlowstoneFeatureIds::Count;
constexpr u32 ColumnNormal = 0 + Offset; // 普通玄武岩柱
constexpr u32 ColumnLarge = 1 + Offset;  // 大型玄武岩柱
constexpr u32 Delta = 2 + Offset;        // 玄武岩三角洲
constexpr u32 Count = 3;                 // 玄武岩特征总数
} // namespace BasaltFeatureIds

namespace MagmaFeatureIds {
// 岩浆池特征 (5-6)
constexpr u32 Offset = GlowstoneFeatureIds::Count + BasaltFeatureIds::Count;
constexpr u32 PatchNormal = 0 + Offset; // 普通岩浆池
constexpr u32 PatchDense = 1 + Offset;  // 密集岩浆池
constexpr u32 Count = 2;                // 岩浆池特征总数
} // namespace MagmaFeatureIds

// ============================================================================
// OceanFeatures 阶段特征ID (VegetalDecoration阶段)
// ============================================================================

namespace VegetationIds {
/// VegetalDecoration阶段陆地植被特征总数
constexpr u32 TotalVegetalFeatures = TreeFeatureIds::Count + FlowerFeatureIds::Count + GrassFeatureIds::Count +
    MushroomFeatureIds::Count + CactusFeatureIds::Count + SugarCaneFeatureIds::Count;
} // namespace VegetationIds

namespace KelpFeatureIds {
// 海带特征（紧跟陆地植被特征之后）
constexpr u32 Offset = VegetationIds::TotalVegetalFeatures;
constexpr u32 Cold = 0 + Offset; // 冷海带
constexpr u32 Warm = 1 + Offset; // 暖海带
constexpr u32 Count = 2;         // 海带特征总数
} // namespace KelpFeatureIds

namespace SeagrassFeatureIds {
// 海草特征（海带之后）
constexpr u32 Offset = KelpFeatureIds::Offset + KelpFeatureIds::Count;
constexpr u32 Simple = 0 + Offset;   // 简单海草
constexpr u32 Mixed = 1 + Offset;    // 混合海草（含高海草）
constexpr u32 Cold = 2 + Offset;     // 冷水海草
constexpr u32 DeepCold = 3 + Offset; // 深冷水海草
constexpr u32 Normal = 4 + Offset;   // 常规海草
constexpr u32 River = 5 + Offset;    // 河流海草
constexpr u32 Deep = 6 + Offset;     // 深海草
constexpr u32 Swamp = 7 + Offset;    // 沼泽海草
constexpr u32 Warm = 8 + Offset;     // 暖水海草
constexpr u32 DeepWarm = 9 + Offset; // 深暖水海草
constexpr u32 Count = 10;            // 海草特征总数
} // namespace SeagrassFeatureIds

namespace SeaPickleFeatureIds {
// 海泡菜特征（海草之后）
constexpr u32 Offset = SeagrassFeatureIds::Offset + SeagrassFeatureIds::Count;
constexpr u32 Normal = 0 + Offset; // 普通海泡菜
constexpr u32 Count = 1;           // 海泡菜特征总数
} // namespace SeaPickleFeatureIds

namespace CoralFeatureIds {
// 珊瑚特征（海泡菜之后）
constexpr u32 Offset = SeaPickleFeatureIds::Offset + SeaPickleFeatureIds::Count;
constexpr u32 Tube = 0 + Offset;       // 管状珊瑚
constexpr u32 Brain = 1 + Offset;      // 脑珊瑚
constexpr u32 Bubble = 2 + Offset;     // 气泡珊瑚
constexpr u32 Fire = 3 + Offset;       // 火焰珊瑚
constexpr u32 Horn = 4 + Offset;       // 角珊瑚
constexpr u32 DeadTube = 5 + Offset;   // 失活管状珊瑚
constexpr u32 DeadBrain = 6 + Offset;  // 失活脑珊瑚
constexpr u32 DeadBubble = 7 + Offset; // 失活气泡珊瑚
constexpr u32 DeadFire = 8 + Offset;   // 失活火焰珊瑚
constexpr u32 DeadHorn = 9 + Offset;   // 失活角珊瑚
constexpr u32 Count = 10;              // 珊瑚特征总数
} // namespace CoralFeatureIds

namespace OceanDecorationFeatureIds {
// 海洋装饰特征（珊瑚之后）
constexpr u32 Offset = CoralFeatureIds::Offset + CoralFeatureIds::Count;
constexpr u32 OceanProps = 0 + Offset; // 海洋装饰物（潮涌核心/气泡柱/海晶石部件等）
constexpr u32 Count = 1;               // 海洋装饰特征总数
} // namespace OceanDecorationFeatureIds

namespace BlueIceFeatureIds {
// 蓝冰特征（海洋装饰之后）
constexpr u32 Offset = OceanDecorationFeatureIds::Offset + OceanDecorationFeatureIds::Count;
constexpr u32 Normal = 0 + Offset; // 蓝冰簇
constexpr u32 Count = 1;           // 蓝冰特征总数
} // namespace BlueIceFeatureIds

// ============================================================================
// 下界植被特征ID (VegetalDecoration阶段)
// ============================================================================

namespace NetherFungusIds {
// 下界真菌特征
constexpr u32 Offset = BlueIceFeatureIds::Offset + BlueIceFeatureIds::Count;
constexpr u32 CrimsonFungus = 0 + Offset; // 绯红巨型真菌
constexpr u32 WarpedFungus = 1 + Offset;  // 诡异巨型真菌
constexpr u32 NetherFire = 2 + Offset;    // 下界火焰
constexpr u32 Count = 3;                  // 下界真菌特征总数
} // namespace NetherFungusIds

// ============================================================================
// 海洋特征总数
// ============================================================================

namespace OceanFeatureIds {
/// 所有海洋特征总数（用于VegetalDecoration阶段）
constexpr u32 TotalOceanFeatures = KelpFeatureIds::Count + SeagrassFeatureIds::Count + SeaPickleFeatureIds::Count +
    CoralFeatureIds::Count + OceanDecorationFeatureIds::Count + BlueIceFeatureIds::Count;
} // namespace OceanFeatureIds

} // namespace mc
