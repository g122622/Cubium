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

#include "VillagePools.hpp"
#include "../ProcessorLists.hpp"
#include "../../../jigsaw/JigsawPiece.hpp"
#include "resource/ResourceLocation.hpp"
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace structure {
namespace pools {

using jigsaw::EmptyJigsawPiece;
using jigsaw::JigsawPattern;
using jigsaw::JigsawPatternRegistry;
using jigsaw::JigsawPlacementBehaviour;
using jigsaw::SingleJigsawPiece;

// 注册标志
static bool s_registered = false;

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 创建单个模板池元素的便捷函数
 *
 * @param templateName 模板名称（如 "minecraft:village/plains/town_centers/plains_fountain_01"）
 * @param behaviour 放置行为
 * @return JigsawPiece 指针
 */
static std::unique_ptr<SingleJigsawPiece> makeSinglePiece(
    const std::string& templateName, JigsawPlacementBehaviour behaviour = JigsawPlacementBehaviour::Rigid)
{
    auto piece = std::make_unique<SingleJigsawPiece>(templateName, behaviour);
    return piece;
}

/**
 * @brief 创建空元素
 */
static std::unique_ptr<jigsaw::JigsawPiece> makeEmptyPiece()
{
    return EmptyJigsawPiece::instance().clone();
}

// ============================================================================
// VillagePools 实现
// ============================================================================

void VillagePools::registerAll(JigsawPatternRegistry& registry)
{
    if (s_registered) {
        return;
    }

    spdlog::info("[VillagePools] Registering village template pools...");

    // 注册公共池
    registerCommonPools(registry);

    // 注册各生物群系村庄
    PlainsVillagePools::registerAll(registry);
    DesertVillagePools::registerAll(registry);
    SavannaVillagePools::registerAll(registry);
    SnowyVillagePools::registerAll(registry);
    TaigaVillagePools::registerAll(registry);

    s_registered = true;
    spdlog::info("[VillagePools] Village template pools registered successfully");
}

void VillagePools::registerCommonPools(JigsawPatternRegistry& registry)
{
    // ========================================================================
    // village/common/animals - 村庄动物池
    // MC 1.16.5: VillagePools.field_244091_a
    // ========================================================================
    auto animals = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/common/animals"),
        ResourceLocation("minecraft", "empty"));

    // MC 1.16.5: animals/cows_1 (weight: 7), animals/pigs_1 (weight: 7)
    // animals/horses_1~5 (weight: 1 each), animals/sheep_1~2 (weight: 1 each)
    // empty (weight: 5)
    // 注：动物池需要 FeatureJigsawPiece 支持，当前使用空元素占位
    animals->addPiece(makeEmptyPiece(), 7);  // cows placeholder
    animals->addPiece(makeEmptyPiece(), 7);  // pigs placeholder
    for (int i = 0; i < 5; ++i) {
        animals->addPiece(makeEmptyPiece(), 1);  // horses placeholder
    }
    for (int i = 0; i < 2; ++i) {
        animals->addPiece(makeEmptyPiece(), 1);  // sheep placeholder
    }
    animals->addPiece(makeEmptyPiece(), 5);
    registry.registerPattern(std::move(animals));

    // ========================================================================
    // village/common/sheep - 羊池
    // MC 1.16.5: VillagePools.field_244092_b
    // ========================================================================
    auto sheep = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/common/sheep"),
        ResourceLocation("minecraft", "empty"));

    sheep->addPiece(makeEmptyPiece(), 1);  // sheep_1 placeholder
    sheep->addPiece(makeEmptyPiece(), 1);  // sheep_2 placeholder
    registry.registerPattern(std::move(sheep));

    // ========================================================================
    // village/common/cats - 猫池
    // MC 1.16.5: VillagePools.field_244093_c
    // ========================================================================
    auto cats = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/common/cats"),
        ResourceLocation("minecraft", "empty"));

    // MC 1.16.5: 10 种猫各 1，空元素 3
    for (int i = 0; i < 10; ++i) {
        cats->addPiece(makeEmptyPiece(), 1);
    }
    cats->addPiece(makeEmptyPiece(), 3);
    registry.registerPattern(std::move(cats));

    // ========================================================================
    // village/common/butcher_animals - 屠夫动物池
    // MC 1.16.5: VillagePools.field_244094_d
    // ========================================================================
    auto butcherAnimals = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/common/butcher_animals"),
        ResourceLocation("minecraft", "empty"));

    butcherAnimals->addPiece(makeEmptyPiece(), 3);  // cows placeholder
    butcherAnimals->addPiece(makeEmptyPiece(), 3);  // pigs placeholder
    butcherAnimals->addPiece(makeEmptyPiece(), 1);  // sheep_1
    butcherAnimals->addPiece(makeEmptyPiece(), 1);  // sheep_2
    registry.registerPattern(std::move(butcherAnimals));

    // ========================================================================
    // village/common/iron_golem - 铁傀儡池
    // MC 1.16.5: VillagePools.field_244095_e
    // ========================================================================
    auto ironGolem = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/common/iron_golem"),
        ResourceLocation("minecraft", "empty"));

    ironGolem->addPiece(makeEmptyPiece(), 1);
    registry.registerPattern(std::move(ironGolem));

    // ========================================================================
    // village/common/well_bottoms - 井底池
    // MC 1.16.5: VillagePools.field_244096_f
    // ========================================================================
    auto wellBottoms = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/common/well_bottoms"),
        ResourceLocation("minecraft", "empty"));

    wellBottoms->addPiece(
        makeSinglePiece("minecraft:village/common/well_bottom", JigsawPlacementBehaviour::Rigid), 1);
    registry.registerPattern(std::move(wellBottoms));
}

bool VillagePools::isRegistered()
{
    return s_registered;
}

// ============================================================================
// PlainsVillagePools 实现
// ============================================================================

namespace PlainsVillagePools {

void registerAll(JigsawPatternRegistry& registry)
{
    // ========================================================================
    // village/plains/town_centers - 起始池
    // MC 1.16.5: PlainsVillagePools.field_244090_a
    // ========================================================================
    auto townCenters = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/plains/town_centers"),
        ResourceLocation("minecraft", "empty"));

    // MC 1.16.5: 正常村庄中心
    // plains_fountain_01 (weight: 50, mossify 20%)
    // plains_meeting_point_1 (weight: 50, mossify 20%)
    // plains_meeting_point_2 (weight: 50, no processor)
    // plains_meeting_point_3 (weight: 50, mossify 20%)
    // 注：当前 SingleJigsawPiece 不支持处理器列表，后续需要扩展
    townCenters->addPiece(
        makeSinglePiece("minecraft:village/plains/town_centers/plains_fountain_01"), 50);
    townCenters->addPiece(
        makeSinglePiece("minecraft:village/plains/town_centers/plains_meeting_point_1"), 50);
    townCenters->addPiece(
        makeSinglePiece("minecraft:village/plains/town_centers/plains_meeting_point_2"), 50);
    townCenters->addPiece(
        makeSinglePiece("minecraft:village/plains/town_centers/plains_meeting_point_3"), 50);

    // MC 1.16.5: 僵尸村庄中心 (weight: 1 each)
    townCenters->addPiece(
        makeSinglePiece("minecraft:village/plains/zombie/town_centers/plains_fountain_01"), 1);
    townCenters->addPiece(
        makeSinglePiece("minecraft:village/plains/zombie/town_centers/plains_meeting_point_1"), 1);
    townCenters->addPiece(
        makeSinglePiece("minecraft:village/plains/zombie/town_centers/plains_meeting_point_2"), 1);
    townCenters->addPiece(
        makeSinglePiece("minecraft:village/plains/zombie/town_centers/plains_meeting_point_3"), 1);

    registry.registerPattern(std::move(townCenters));

    // ========================================================================
    // village/plains/streets - 街道池
    // MC 1.16.5: PlainsVillagePools 静态块
    // fallback: village/plains/terminators
    // ========================================================================
    auto streets = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/plains/streets"),
        ResourceLocation("minecraft", "village/plains/terminators"));

    // MC 1.16.5: 街道模板
    streets->addPiece(makeSinglePiece("minecraft:village/plains/streets/corner_01"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/plains/streets/corner_02"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/plains/streets/corner_03"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/plains/streets/straight_01"), 4);
    streets->addPiece(makeSinglePiece("minecraft:village/plains/streets/straight_02"), 4);
    streets->addPiece(makeSinglePiece("minecraft:village/plains/streets/straight_03"), 7);
    streets->addPiece(makeSinglePiece("minecraft:village/plains/streets/straight_04"), 7);
    streets->addPiece(makeSinglePiece("minecraft:village/plains/streets/straight_05"), 3);
    streets->addPiece(makeSinglePiece("minecraft:village/plains/streets/straight_06"), 4);
    streets->addPiece(makeSinglePiece("minecraft:village/plains/streets/crossroad_01"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/plains/streets/crossroad_02"), 1);
    streets->addPiece(makeSinglePiece("minecraft:village/plains/streets/crossroad_03"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/plains/streets/crossroad_04"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/plains/streets/crossroad_05"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/plains/streets/crossroad_06"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/plains/streets/turn_01"), 3);

    registry.registerPattern(std::move(streets));

    // ========================================================================
    // village/plains/zombie/streets - 僵尸村庄街道
    // ========================================================================
    auto zombieStreets = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/plains/zombie/streets"),
        ResourceLocation("minecraft", "village/plains/terminators"));

    // 与正常街道相同结构
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/plains/streets/corner_01"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/plains/streets/corner_02"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/plains/streets/corner_03"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/plains/streets/straight_01"), 4);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/plains/streets/straight_02"), 4);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/plains/streets/straight_03"), 7);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/plains/streets/straight_04"), 7);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/plains/streets/straight_05"), 3);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/plains/streets/straight_06"), 4);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/plains/streets/crossroad_01"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/plains/streets/crossroad_02"), 1);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/plains/streets/crossroad_03"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/plains/streets/crossroad_04"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/plains/streets/crossroad_05"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/plains/streets/crossroad_06"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/plains/streets/turn_01"), 3);

    registry.registerPattern(std::move(zombieStreets));

    // ========================================================================
    // village/plains/houses - 房屋池
    // ========================================================================
    auto houses = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/plains/houses"),
        ResourceLocation("minecraft", "village/plains/terminators"));

    // MC 1.16.5: 小型房屋 (weight: 2 each)
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/small_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/small_house_2"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/small_house_3"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/small_house_4"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/small_house_5"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/small_house_6"), 1);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/small_house_7"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/small_house_8"), 3);

    // 中型房屋
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/medium_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/medium_house_2"), 2);

    // 大型房屋
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/big_house_1"), 2);

    // 职业建筑
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/butcher_shop_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/butcher_shop_2"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/tool_smith_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/fletcher_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/shepherds_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/armorer_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/fisher_cottage_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/tannery_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/cartographer_1"), 1);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/library_1"), 5);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/library_2"), 1);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/masons_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/weaponsmith_1"), 2);

    // 神殿
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/temple_3"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/temple_4"), 2);

    // 马厩
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/stable_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/stable_2"), 2);

    // 农场
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/large_farm_1"), 4);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/small_farm_1"), 4);

    // 动物圈
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/animal_pen_1"), 1);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/animal_pen_2"), 1);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/animal_pen_3"), 5);

    // 装饰
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/accessory_1"), 1);

    // 会议点
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/meeting_point_4"), 3);
    houses->addPiece(makeSinglePiece("minecraft:village/plains/houses/meeting_point_5"), 1);

    // 空元素（控制密度）
    houses->addPiece(makeEmptyPiece(), 10);

    registry.registerPattern(std::move(houses));

    // ========================================================================
    // village/plains/zombie/houses - 僵尸村庄房屋
    // ========================================================================
    auto zombieHouses = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/plains/zombie/houses"),
        ResourceLocation("minecraft", "village/plains/terminators"));

    // 与正常房屋结构相同，使用僵尸版本模板
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/small_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/small_house_2"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/small_house_3"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/small_house_4"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/small_house_5"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/small_house_6"), 1);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/small_house_7"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/small_house_8"), 3);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/medium_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/medium_house_2"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/big_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/butcher_shop_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/butcher_shop_2"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/tool_smith_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/fletcher_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/shepherds_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/armorer_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/fisher_cottage_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/tannery_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/cartographer_1"), 1);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/library_1"), 5);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/library_2"), 1);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/masons_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/weaponsmith_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/temple_3"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/temple_4"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/stable_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/stable_2"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/large_farm_1"), 4);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/small_farm_1"), 4);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/animal_pen_1"), 1);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/animal_pen_2"), 1);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/animal_pen_3"), 5);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/plains/zombie/houses/accessory_1"), 1);
    zombieHouses->addPiece(makeEmptyPiece(), 10);

    registry.registerPattern(std::move(zombieHouses));

    // ========================================================================
    // village/plains/terminators - 终止池
    // ========================================================================
    auto terminators = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/plains/terminators"),
        ResourceLocation("minecraft", "empty"));

    terminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_01"), 1);
    terminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_02"), 1);
    terminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_03"), 1);
    terminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_04"), 1);

    registry.registerPattern(std::move(terminators));

    // ========================================================================
    // village/plains/trees - 树木池
    // ========================================================================
    auto trees = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/plains/trees"),
        ResourceLocation("minecraft", "empty"));

    // 注：需要 FeatureJigsawPiece 支持橡树生成
    trees->addPiece(makeEmptyPiece(), 1);  // oak_tree placeholder

    registry.registerPattern(std::move(trees));

    // ========================================================================
    // village/plains/decor - 装饰池
    // ========================================================================
    auto decor = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/plains/decor"),
        ResourceLocation("minecraft", "empty"));

    decor->addPiece(makeSinglePiece("minecraft:village/plains/decor/plains_lamp_1"), 2);
    decor->addPiece(makeEmptyPiece(), 1);  // oak_tree placeholder
    decor->addPiece(makeEmptyPiece(), 1);  // feature placeholder
    decor->addPiece(makeEmptyPiece(), 1);  // feature placeholder
    decor->addPiece(makeEmptyPiece(), 2);

    registry.registerPattern(std::move(decor));

    // ========================================================================
    // village/plains/zombie/decor - 僵尸村庄装饰池
    // ========================================================================
    auto zombieDecor = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/plains/zombie/decor"),
        ResourceLocation("minecraft", "empty"));

    zombieDecor->addPiece(makeSinglePiece("minecraft:village/plains/decor/plains_lamp_1"), 1);
    zombieDecor->addPiece(makeEmptyPiece(), 1);
    zombieDecor->addPiece(makeEmptyPiece(), 1);
    zombieDecor->addPiece(makeEmptyPiece(), 1);
    zombieDecor->addPiece(makeEmptyPiece(), 2);

    registry.registerPattern(std::move(zombieDecor));

    // ========================================================================
    // village/plains/villagers - 村民池
    // ========================================================================
    auto villagers = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/plains/villagers"),
        ResourceLocation("minecraft", "empty"));

    villagers->addPiece(makeEmptyPiece(), 1);  // nitwit placeholder
    villagers->addPiece(makeEmptyPiece(), 1);  // baby placeholder
    villagers->addPiece(makeEmptyPiece(), 10); // unemployed placeholder

    registry.registerPattern(std::move(villagers));

    // ========================================================================
    // village/plains/zombie/villagers - 僵尸村民池
    // ========================================================================
    auto zombieVillagers = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/plains/zombie/villagers"),
        ResourceLocation("minecraft", "empty"));

    zombieVillagers->addPiece(makeEmptyPiece(), 1);  // zombie_nitwit placeholder
    zombieVillagers->addPiece(makeEmptyPiece(), 10); // zombie_unemployed placeholder

    registry.registerPattern(std::move(zombieVillagers));
}

} // namespace PlainsVillagePools

// ============================================================================
// DesertVillagePools 实现 (占位)
// ============================================================================

namespace DesertVillagePools {

void registerAll(JigsawPatternRegistry& registry)
{
    // TODO: 实现沙漠村庄模板池
    // MC 1.16.5: DesertVillagePools.java

    // 注册空池以避免加载错误
    auto townCenters = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/desert/town_centers"),
        ResourceLocation("minecraft", "empty"));
    townCenters->addPiece(makeEmptyPiece(), 1);
    registry.registerPattern(std::move(townCenters));

    auto streets = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/desert/streets"),
        ResourceLocation("minecraft", "empty"));
    streets->addPiece(makeEmptyPiece(), 1);
    registry.registerPattern(std::move(streets));

    auto houses = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/desert/houses"),
        ResourceLocation("minecraft", "empty"));
    houses->addPiece(makeEmptyPiece(), 1);
    registry.registerPattern(std::move(houses));

    auto terminators = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/desert/terminators"),
        ResourceLocation("minecraft", "empty"));
    terminators->addPiece(makeEmptyPiece(), 1);
    registry.registerPattern(std::move(terminators));
}

} // namespace DesertVillagePools

// ============================================================================
// SavannaVillagePools 实现 (占位)
// ============================================================================

namespace SavannaVillagePools {

void registerAll(JigsawPatternRegistry& registry)
{
    // TODO: 实现热带草原村庄模板池
    // MC 1.16.5: SavannaVillagePools.java

    auto townCenters = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/savanna/town_centers"),
        ResourceLocation("minecraft", "empty"));
    townCenters->addPiece(makeEmptyPiece(), 1);
    registry.registerPattern(std::move(townCenters));

    auto streets = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/savanna/streets"),
        ResourceLocation("minecraft", "empty"));
    streets->addPiece(makeEmptyPiece(), 1);
    registry.registerPattern(std::move(streets));

    auto houses = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/savanna/houses"),
        ResourceLocation("minecraft", "empty"));
    houses->addPiece(makeEmptyPiece(), 1);
    registry.registerPattern(std::move(houses));

    auto terminators = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/savanna/terminators"),
        ResourceLocation("minecraft", "empty"));
    terminators->addPiece(makeEmptyPiece(), 1);
    registry.registerPattern(std::move(terminators));
}

} // namespace SavannaVillagePools

// ============================================================================
// SnowyVillagePools 实现 (占位)
// ============================================================================

namespace SnowyVillagePools {

void registerAll(JigsawPatternRegistry& registry)
{
    // TODO: 实现雪地村庄模板池
    // MC 1.16.5: SnowyVillagePools.java

    auto townCenters = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/snowy/town_centers"),
        ResourceLocation("minecraft", "empty"));
    townCenters->addPiece(makeEmptyPiece(), 1);
    registry.registerPattern(std::move(townCenters));

    auto streets = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/snowy/streets"),
        ResourceLocation("minecraft", "empty"));
    streets->addPiece(makeEmptyPiece(), 1);
    registry.registerPattern(std::move(streets));

    auto houses = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/snowy/houses"),
        ResourceLocation("minecraft", "empty"));
    houses->addPiece(makeEmptyPiece(), 1);
    registry.registerPattern(std::move(houses));

    auto terminators = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/snowy/terminators"),
        ResourceLocation("minecraft", "empty"));
    terminators->addPiece(makeEmptyPiece(), 1);
    registry.registerPattern(std::move(terminators));
}

} // namespace SnowyVillagePools

// ============================================================================
// TaigaVillagePools 实现 (占位)
// ============================================================================

namespace TaigaVillagePools {

void registerAll(JigsawPatternRegistry& registry)
{
    // TODO: 实现针叶林村庄模板池
    // MC 1.16.5: TaigaVillagePools.java

    auto townCenters = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/taiga/town_centers"),
        ResourceLocation("minecraft", "empty"));
    townCenters->addPiece(makeEmptyPiece(), 1);
    registry.registerPattern(std::move(townCenters));

    auto streets = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/taiga/streets"),
        ResourceLocation("minecraft", "empty"));
    streets->addPiece(makeEmptyPiece(), 1);
    registry.registerPattern(std::move(streets));

    auto houses = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/taiga/houses"),
        ResourceLocation("minecraft", "empty"));
    houses->addPiece(makeEmptyPiece(), 1);
    registry.registerPattern(std::move(houses));

    auto terminators = std::make_unique<JigsawPattern>(
        ResourceLocation("minecraft", "village/taiga/terminators"),
        ResourceLocation("minecraft", "empty"));
    terminators->addPiece(makeEmptyPiece(), 1);
    registry.registerPattern(std::move(terminators));
}

} // namespace TaigaVillagePools

} // namespace pools
} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
