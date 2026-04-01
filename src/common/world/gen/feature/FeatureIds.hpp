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
    constexpr u32 CoalOre = 0;       // 煤矿
    constexpr u32 IronOre = 1;       // 铁矿
    constexpr u32 GoldOre = 2;       // 金矿
    constexpr u32 RedstoneOre = 3;   // 红石矿
    constexpr u32 DiamondOre = 4;    // 钻石矿
    constexpr u32 LapisOre = 5;      // 青金石矿
    constexpr u32 EmeraldOre = 6;    // 绿宝石矿
    constexpr u32 CopperOre = 7;     // 铜矿
    // 下界矿石 (8-10)
    constexpr u32 NetherQuartzOre = 8;   // 下界石英矿
    constexpr u32 NetherGoldOre = 9;     // 下界金矿
    constexpr u32 AncientDebris = 10;    // 远古残骸
    constexpr u32 Count = 11;             // 矿石特征总数
}

// ============================================================================
// VegetalDecoration 阶段特征ID
// 注意：ID在阶段内部从0开始，按注册顺序递增
// ============================================================================

namespace TreeFeatureIds {
    // 树木特征 (0-14)
    // 注意：ID必须与 TreeFeatures::initialize() 中注册的顺序一致
    constexpr u32 OakTree = 0;        // 橡树
    constexpr u32 BirchTree = 1;      // 白桦
    constexpr u32 SpruceTree = 2;     // 云杉
    constexpr u32 JungleTree = 3;     // 丛林树
    constexpr u32 AcaciaTree = 4;     // 金合欢
    constexpr u32 DarkOakTree = 5;    // 深色橡树
    constexpr u32 SparseOakTree = 6;  // 稀疏橡树
    constexpr u32 GiantSpruceTree = 7;  // 巨型云杉
    constexpr u32 GiantJungleTree = 8;  // 巨型丛林木
    constexpr u32 FancyOakTree = 9;     // 精美橡树
    constexpr u32 PineTree = 10;        // 松树
    constexpr u32 JungleBush = 11;      // 丛林灌木
    constexpr u32 SwampTree = 12;       // 沼泽橡树
    constexpr u32 MegaPineTree = 13;    // 巨型松树
    constexpr u32 TallBirchTree = 14;   // 高白桦
    constexpr u32 Count = 15;           // 树木特征总数
}

namespace FlowerFeatureIds {
    // 花卉特征 (5-9)
    // 基础偏移量 = TreeFeatureIds::Count = 5
    constexpr u32 Offset = TreeFeatureIds::Count;
    constexpr u32 PlainsFlowers = 0 + Offset;      // 平原花卉
    constexpr u32 ForestFlowers = 1 + Offset;      // 森林花卉
    constexpr u32 FlowerForestFlowers = 2 + Offset; // 繁花森林花卉
    constexpr u32 SwampFlowers = 3 + Offset;       // 沼泽花卉
    constexpr u32 Sunflower = 4 + Offset;          // 向日葵
    constexpr u32 Count = 5;                        // 花卉特征总数
}

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
    constexpr u32 Count = 7;                      // 草丛特征总数
}

namespace MushroomFeatureIds {
    // 巨型蘑菇特征 (17-18)
    // 基础偏移量 = TreeFeatureIds::Count + FlowerFeatureIds::Count + GrassFeatureIds::Count = 17
    constexpr u32 Offset = TreeFeatureIds::Count + FlowerFeatureIds::Count + GrassFeatureIds::Count;
    constexpr u32 BrownMushroom = 0 + Offset; // 棕色巨型蘑菇
    constexpr u32 RedMushroom = 1 + Offset;   // 红色巨型蘑菇
    constexpr u32 Count = 2;                   // 蘑菇特征总数
}

namespace CactusFeatureIds {
    // 仙人掌特征 (19-20)
    constexpr u32 Offset = TreeFeatureIds::Count + FlowerFeatureIds::Count + GrassFeatureIds::Count + MushroomFeatureIds::Count;
    constexpr u32 DesertCactus = 0 + Offset;    // 沙漠仙人掌
    constexpr u32 BadlandsCactus = 1 + Offset;  // 恶地仙人掌
    constexpr u32 Count = 2;                     // 仙人掌特征总数
}

namespace SugarCaneFeatureIds {
    // 甘蔗特征 (21-22)
    constexpr u32 Offset = TreeFeatureIds::Count + FlowerFeatureIds::Count + GrassFeatureIds::Count + MushroomFeatureIds::Count + CactusFeatureIds::Count;
    constexpr u32 Normal = 0 + Offset;   // 普通甘蔗
    constexpr u32 Dense = 1 + Offset;    // 密集甘蔗
    constexpr u32 Count = 2;              // 甘蔗特征总数
}

// ============================================================================
// SurfaceStructures 阶段特征ID
// ============================================================================

namespace IceSpikeFeatureIds {
    // 冰刺特征 (0-1)
    constexpr u32 Spike = 0;   // 尖塔型冰刺
    constexpr u32 Iceberg = 1; // 冰丘
    constexpr u32 Count = 2;    // 冰刺特征总数
}

// ============================================================================
// 便捷组合常量
// ============================================================================

namespace VegetationIds {
    /// VegetalDecoration阶段特征总数
    constexpr u32 TotalVegetalFeatures =
        TreeFeatureIds::Count +
        FlowerFeatureIds::Count +
        GrassFeatureIds::Count +
        MushroomFeatureIds::Count +
        CactusFeatureIds::Count +
        SugarCaneFeatureIds::Count;
}

} // namespace mc
