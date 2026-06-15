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

#include "Biome.hpp"
#include "BiomeIds.hpp"

namespace mc {
namespace world {
namespace biome {

// ============================================================================
// 生物群系工厂函数
// ============================================================================

namespace BiomeFactory {

/**
 * @brief 创建平原生物群系
 */
Biome createPlains();

/**
 * @brief 创建沙漠生物群系
 */
Biome createDesert();

/**
 * @brief 创建山地生物群系
 */
Biome createMountains();

/**
 * @brief 创建森林生物群系
 */
Biome createForest();

/**
 * @brief 创建海洋生物群系
 */
Biome createOcean();

/**
 * @brief 创建深海生物群系
 */
Biome createDeepOcean();

/**
 * @brief 创建泰加林生物群系
 */
Biome createTaiga();

/**
 * @brief 创建丛林生物群系
 */
Biome createJungle();

/**
 * @brief 创建热带草原生物群系
 */
Biome createSavanna();

/**
 * @brief 创建恶地生物群系
 */
Biome createBadlands();

/**
 * @brief 创建海滩生物群系
 */
Biome createBeach();

/**
 * @brief 创建沼泽生物群系
 */
Biome createSwamp();

/**
 * @brief 创建河流生物群系
 */
Biome createRiver();

/**
 * @brief 创建繁茂丘陵生物群系
 */
Biome createWoodedHills();

/**
 * @brief 创建山地边缘生物群系
 */
Biome createMountainEdge();

/**
 * @brief 创建石岸生物群系
 */
Biome createStoneShore();

/**
 * @brief 创建积雪沙滩生物群系
 */
Biome createSnowyBeach();

/**
 * @brief 创建积雪平原生物群系
 */
Biome createSnowyPlains();

/**
 * @brief 创建黑森林生物群系
 */
Biome createDarkForest();

/**
 * @brief 创建桦木森林生物群系
 */
Biome createBirchForest();

/**
 * @brief 创建巨型针叶林生物群系
 */
Biome createGiantTreeTaiga();

/**
 * @brief 创建繁茂山地生物群系
 */
Biome createWoodedMountains();

/**
 * @brief 创建热带高原生物群系
 */
Biome createSavannaPlateau();

/**
 * @brief 创建恶地高原生物群系
 */
Biome createBadlandsPlateau();

/**
 * @brief 创建繁茂恶地高原生物群系
 */
Biome createWoodedBadlandsPlateau();

/**
 * @brief 创建风蚀恶地生物群系
 */
Biome createErodedBadlands();

/**
 * @brief 创建破碎热带草原生物群系
 */
Biome createShatteredSavanna();

/**
 * @brief 创建积雪泰加林生物群系
 */
Biome createSnowyTaiga();

/**
 * @brief 创建冻结海洋生物群系
 */
Biome createFrozenOcean();

/**
 * @brief 创建冻结河流生物群系
 */
Biome createFrozenRiver();

/**
 * @brief 创建积雪山地生物群系
 */
Biome createSnowyMountains();

/**
 * @brief 创建冰刺之地生物群系
 */
Biome createIceSpikes();

/**
 * @brief 创建深海冻结海洋生物群系
 */
Biome createDeepFrozenOcean();

// ============================================================================
// 高优先级生物群系（阶段1）
// ============================================================================

/**
 * @brief 创建温暖海洋生物群系
 * @note 温暖海洋，温度高，沙子底部
 */
Biome createWarmOcean();

/**
 * @brief 创建微温海洋生物群系
 */
Biome createLukewarmOcean();

/**
 * @brief 创建寒冷海洋生物群系
 */
Biome createColdOcean();

/**
 * @brief 创建深海温暖海洋生物群系
 */
Biome createDeepWarmOcean();

/**
 * @brief 创建深海微温海洋生物群系
 */
Biome createDeepLukewarmOcean();

/**
 * @brief 创建深海寒冷海洋生物群系
 */
Biome createDeepColdOcean();

/**
 * @brief 创建丛林丘陵生物群系
 * @note depth=0.45, scale=0.3
 */
Biome createJungleHills();

/**
 * @brief 创建丛林边缘生物群系
 * @note depth=0.1, scale=0.2
 */
Biome createJungleEdge();

/**
 * @brief 创建竹林生物群系
 * @note depth=0.1, scale=0.2 (暂不生成竹子)
 */
Biome createBambooJungle();

/**
 * @brief 创建竹林丘陵生物群系
 * @note depth=0.45, scale=0.3
 */
Biome createBambooJungleHills();

/**
 * @brief 创建桦木森林丘陵生物群系
 * @note depth=0.45, scale=0.3
 */
Biome createBirchForestHills();

/**
 * @brief 创建繁花森林生物群系
 * @note depth=0.1, scale=0.2
 */
Biome createFlowerForest();

/**
 * @brief 创建高大桦木森林生物群系
 * @note depth=0.1, scale=0.2 (高桦木)
 */
Biome createTallBirchForest();

/**
 * @brief 创建高大桦木丘陵生物群系
 * @note depth=0.45, scale=0.3
 */
Biome createTallBirchHills();

/**
 * @brief 创建黑森林丘陵生物群系
 * @note depth=0.45, scale=0.3
 */
Biome createDarkForestHills();

/**
 * @brief 创建蘑菇岛生物群系
 * @note depth=0.2, scale=0.3
 */
Biome createMushroomFields();

/**
 * @brief 创建蘑菇岛海岸生物群系
 * @note depth=0.0, scale=0.025
 */
Biome createMushroomFieldShore();

/**
 * @brief 创建沙漠丘陵生物群系
 * @note depth=0.225, scale=0.25
 */
Biome createDesertHills();

/**
 * @brief 创建针叶林丘陵生物群系
 * @note depth=0.3, scale=0.25
 */
Biome createTaigaHills();

/**
 * @brief 创建巨型云杉针叶林生物群系
 * @note depth=0.2, scale=0.2
 */
Biome createGiantSpruceTaiga();

/**
 * @brief 创建巨型云杉针叶林丘陵生物群系
 * @note depth=0.2, scale=0.2
 */
Biome createGiantSpruceTaigaHills();

// ============================================================================
// 中优先级生物群系（阶段2）
// ============================================================================

/**
 * @brief 创建向日葵平原生物群系
 */
Biome createSunflowerPlains();

/**
 * @brief 创建沙漠湖泊生物群系
 */
Biome createDesertLakes();

/**
 * @brief 创建砾石山地生物群系
 */
Biome createGravellyMountains();

/**
 * @brief 创建针叶林山地生物群系
 */
Biome createTaigaMountains();

/**
 * @brief 创建沼泽丘陵生物群系
 */
Biome createSwampHills();

/**
 * @brief 创建变异丛林生物群系
 */
Biome createModifiedJungle();

/**
 * @brief 创建变异丛林边缘生物群系
 */
Biome createModifiedJungleEdge();

/**
 * @brief 创建积雪针叶林山地生物群系
 */
Biome createSnowyTaigaMountains();

/**
 * @brief 创建变异砾石山地生物群系
 */
Biome createModifiedGravellyMountains();

/**
 * @brief 创建破碎热带草原高原生物群系
 */
Biome createShatteredSavannaPlateau();

/**
 * @brief 创建变异繁茂恶地高原生物群系
 */
Biome createModifiedWoodedBadlandsPlateau();

/**
 * @brief 创建变异恶地高原生物群系
 */
Biome createModifiedBadlandsPlateau();

/**
 * @brief 创建巨型针叶林丘陵生物群系
 * @note 已有 GiantTreeTaiga，这里是其丘陵变体
 */
Biome createGiantTreeTaigaHillsBiome();

/**
 * @brief 创建积雪针叶林丘陵生物群系
 */
Biome createSnowyTaigaHills();

// ============================================================================
// 下界生物群系
// ============================================================================

/**
 * @brief 创建下界荒地生物群系 (ID: 8)
 * @note 下界默认生物群系，以下界岩为主，有猪灵、恶魂、岩浆怪等
 */
Biome createNetherWastes();

/**
 * @brief 创建灵魂沙谷生物群系 (ID: 170)
 * @note 灵魂沙和灵魂土为主，蓝色迷雾，骷髅和恶魂
 */
Biome createSoulSandValley();

/**
 * @brief 创建绯红森林生物群系 (ID: 171)
 * @note 绯红菌和疣猪兽，红色主题
 */
Biome createCrimsonForest();

/**
 * @brief 创建诡异森林生物群系 (ID: 172)
 * @note 诡异菌和末影人，青色主题
 */
Biome createWarpedForest();

/**
 * @brief 创建玄武岩三角洲生物群系 (ID: 173)
 * @note 玄武岩和岩浆块，黑色颗粒效果
 */
Biome createBasaltDeltas();

// ============================================================================
// 末地生物群系
// ============================================================================

/**
 * @brief 创建末地生物群系 (ID: 9)
 * @note 末地主岛，末影龙战斗区域
 */
Biome createTheEnd();

/**
 * @brief 创建小型末地岛屿生物群系 (ID: 40)
 * @note 外岛的小型岛屿群
 */
Biome createSmallEndIslands();

/**
 * @brief 创建末地中部生物群系 (ID: 41)
 * @note 外岛过渡区域
 */
Biome createEndMidlands();

/**
 * @brief 创建末地高地生物群系 (ID: 42)
 * @note 末地城和紫颂树生成区域
 */
Biome createEndHighlands();

/**
 * @brief 创建末地荒地生物群系 (ID: 43)
 * @note 空旷区域，无特征
 */
Biome createEndBarrens();

// ============================================================================
// 新生物群系（洞穴与山崖）
// ============================================================================

/**
 * @brief 创建草甸生物群系 (ID: 174)
 * @note 山地草甸，温度适中
 */
Biome createMeadow();

/**
 * @brief 创建树林生物群系 (ID: 175)
 * @note 山地树林，降雪区域
 */
Biome createGrove();

/**
 * @brief 创建积雪坡地生物群系 (ID: 176)
 * @note 积雪山坡
 */
Biome createSnowySlopes();

/**
 * @brief 创建尖峭山峰生物群系 (ID: 177)
 * @note 雪覆盖的尖峰
 */
Biome createJaggedPeaks();

/**
 * @brief 创建冰封山峰生物群系 (ID: 178)
 * @note 冰覆盖的山峰
 */
Biome createFrozenPeaks();

/**
 * @brief 创建裸岩山峰生物群系 (ID: 179)
 * @note 石头和方解石山峰
 */
Biome createStonyPeaks();

/**
 * @brief 创建滴水石洞生物群系 (ID: 180)
 * @note 钟乳石和石笋洞穴
 */
Biome createDripstoneCaves();

/**
 * @brief 创建繁茂洞穴生物群系 (ID: 181)
 * @note 苔藓和发光浆果洞穴
 */
Biome createLushCaves();

/**
 * @brief 创建深暗之域生物群系 (ID: 182)
 * @note 幽匿和监守者
 */
Biome createDeepDark();

/**
 * @brief 创建红树林沼泽生物群系 (ID: 183)
 * @note 红树林和泥巴
 */
Biome createMangroveSwamp();

/**
 * @brief 创建樱花树林生物群系 (ID: 184)
 * @note 樱花树和粉色花瓣
 */
Biome createCherryGrove();

/**
 * @brief 创建苍白花园生物群系 (ID: 185)
 * @note 苍白橡树和嘎吱声
 */
Biome createPaleGarden();

} // namespace BiomeFactory

} // namespace biome
} // namespace world
} // namespace mc

// 旧命名空间兼容别名
namespace mc {
namespace BiomeFactory = ::mc::world::biome::BiomeFactory;
} // namespace mc
