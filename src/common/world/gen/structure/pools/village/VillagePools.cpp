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
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/gen/jigsaw/EmptyJigsawPiece.hpp"
#include "common/world/gen/jigsaw/JigsawPiece.hpp"
#include "common/world/gen/jigsaw/JigsawTypes.hpp"
#include "common/world/gen/jigsaw/SingleJigsawPiece.hpp"
#include "common/world/gen/jigsaw/TemplatePool.hpp"
#include "common/world/gen/jigsaw/TemplatePoolRegistry.hpp"
#include <memory>
#include <string>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace structure {
namespace pools {

using jigsaw::EmptyJigsawPiece;
using jigsaw::JigsawPlacementBehaviour;
using jigsaw::SingleJigsawPiece;
using jigsaw::TemplatePool;
using jigsaw::TemplatePoolRegistry;

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
 *
 * EmptyJigsawPiece 是单例（clone 返回 nullptr），用 make_unique 创建临时实例，
 * TemplatePool::addPiece 检测到 isEmpty() 后存入单例指针并丢弃临时对象。
 */
static std::unique_ptr<jigsaw::JigsawPiece> makeEmptyPiece()
{
    return std::make_unique<EmptyJigsawPiece>();
}

// ============================================================================
// VillagePools 实现
// ============================================================================

void VillagePools::registerAll(TemplatePoolRegistry& registry)
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

void VillagePools::registerCommonPools(TemplatePoolRegistry& registry)
{
    // ========================================================================
    // village/common/animals - 村庄动物池
    // ========================================================================
    auto animals = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/common/animals"), ResourceLocation("minecraft", "empty"));

    // 动物池：FeatureJigsawPiece::place() 已实现（可放置配置化地物），
    // 但动物生成依赖实体刷怪（非 ConfiguredFeature），此处仍用空元素占位。
    animals->addPiece(makeEmptyPiece(), 7); // cows placeholder
    animals->addPiece(makeEmptyPiece(), 7); // pigs placeholder
    for (i32 i = 0; i < 5; ++i) {
        animals->addPiece(makeEmptyPiece(), 1); // horses placeholder
    }
    for (i32 i = 0; i < 2; ++i) {
        animals->addPiece(makeEmptyPiece(), 1); // sheep placeholder
    }
    animals->addPiece(makeEmptyPiece(), 5);
    registry.registerPool(std::move(animals));

    // ========================================================================
    // village/common/sheep - 羊池
    // ========================================================================
    auto sheep = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/common/sheep"), ResourceLocation("minecraft", "empty"));

    sheep->addPiece(makeEmptyPiece(), 1); // sheep_1 placeholder
    sheep->addPiece(makeEmptyPiece(), 1); // sheep_2 placeholder
    registry.registerPool(std::move(sheep));

    // ========================================================================
    // village/common/cats - 猫池
    // ========================================================================
    auto cats = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/common/cats"), ResourceLocation("minecraft", "empty"));

    // 10 种猫各 1，空元素 3
    for (i32 i = 0; i < 10; ++i) {
        cats->addPiece(makeEmptyPiece(), 1);
    }
    cats->addPiece(makeEmptyPiece(), 3);
    registry.registerPool(std::move(cats));

    // ========================================================================
    // village/common/butcher_animals - 屠夫动物池
    // ========================================================================
    auto butcherAnimals = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/common/butcher_animals"), ResourceLocation("minecraft", "empty"));

    butcherAnimals->addPiece(makeEmptyPiece(), 3); // cows placeholder
    butcherAnimals->addPiece(makeEmptyPiece(), 3); // pigs placeholder
    butcherAnimals->addPiece(makeEmptyPiece(), 1); // sheep_1
    butcherAnimals->addPiece(makeEmptyPiece(), 1); // sheep_2
    registry.registerPool(std::move(butcherAnimals));

    // ========================================================================
    // village/common/iron_golem - 铁傀儡池
    // ========================================================================
    auto ironGolem = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/common/iron_golem"), ResourceLocation("minecraft", "empty"));

    ironGolem->addPiece(makeEmptyPiece(), 1);
    registry.registerPool(std::move(ironGolem));

    // ========================================================================
    // village/common/well_bottoms - 井底池
    // ========================================================================
    auto wellBottoms = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/common/well_bottoms"), ResourceLocation("minecraft", "empty"));

    wellBottoms->addPiece(makeSinglePiece("minecraft:village/common/well_bottom", JigsawPlacementBehaviour::Rigid), 1);
    registry.registerPool(std::move(wellBottoms));
}

bool VillagePools::isRegistered()
{
    return s_registered;
}

// ============================================================================
// PlainsVillagePools 实现
// ============================================================================

namespace PlainsVillagePools {

void registerAll(TemplatePoolRegistry& registry)
{
    // ========================================================================
    // village/plains/town_centers - 起始池
    // ========================================================================
    auto townCenters = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/plains/town_centers"), ResourceLocation("minecraft", "empty"));

    // 正常村庄中心
    // 注：当前 SingleJigsawPiece 不支持处理器列表，后续需要扩展
    townCenters->addPiece(makeSinglePiece("minecraft:village/plains/town_centers/plains_fountain_01"), 50);
    townCenters->addPiece(makeSinglePiece("minecraft:village/plains/town_centers/plains_meeting_point_1"), 50);
    townCenters->addPiece(makeSinglePiece("minecraft:village/plains/town_centers/plains_meeting_point_2"), 50);
    townCenters->addPiece(makeSinglePiece("minecraft:village/plains/town_centers/plains_meeting_point_3"), 50);

    // 僵尸村庄中心
    townCenters->addPiece(makeSinglePiece("minecraft:village/plains/zombie/town_centers/plains_fountain_01"), 1);
    townCenters->addPiece(makeSinglePiece("minecraft:village/plains/zombie/town_centers/plains_meeting_point_1"), 1);
    townCenters->addPiece(makeSinglePiece("minecraft:village/plains/zombie/town_centers/plains_meeting_point_2"), 1);
    townCenters->addPiece(makeSinglePiece("minecraft:village/plains/zombie/town_centers/plains_meeting_point_3"), 1);

    registry.registerPool(std::move(townCenters));

    // ========================================================================
    // village/plains/streets - 街道池
    // fallback: village/plains/terminators
    // ========================================================================
    auto streets = std::make_unique<TemplatePool>(ResourceLocation("minecraft", "village/plains/streets"),
        ResourceLocation("minecraft", "village/plains/terminators"));

    // 街道模板
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

    registry.registerPool(std::move(streets));

    // ========================================================================
    // village/plains/zombie/streets - 僵尸村庄街道
    // ========================================================================
    auto zombieStreets = std::make_unique<TemplatePool>(ResourceLocation("minecraft", "village/plains/zombie/streets"),
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

    registry.registerPool(std::move(zombieStreets));

    // ========================================================================
    // village/plains/houses - 房屋池
    // ========================================================================
    auto houses = std::make_unique<TemplatePool>(ResourceLocation("minecraft", "village/plains/houses"),
        ResourceLocation("minecraft", "village/plains/terminators"));

    // 小型房屋
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

    registry.registerPool(std::move(houses));

    // ========================================================================
    // village/plains/zombie/houses - 僵尸村庄房屋
    // ========================================================================
    auto zombieHouses = std::make_unique<TemplatePool>(ResourceLocation("minecraft", "village/plains/zombie/houses"),
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

    registry.registerPool(std::move(zombieHouses));

    // ========================================================================
    // village/plains/terminators - 终止池
    // ========================================================================
    auto terminators = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/plains/terminators"), ResourceLocation("minecraft", "empty"));

    terminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_01"), 1);
    terminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_02"), 1);
    terminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_03"), 1);
    terminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_04"), 1);

    registry.registerPool(std::move(terminators));

    // ========================================================================
    // village/plains/trees - 树木池
    // ========================================================================
    auto trees = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/plains/trees"), ResourceLocation("minecraft", "empty"));

    // 注：FeatureJigsawPiece::place() 已实现（ConfiguredFeatureRegistry 名称映射 + 配置化地物放置），
    // 此处仍用空元素占位；后续可替换为 makeFeaturePiece("minecraft:oak_tree") 等真实地物块。
    trees->addPiece(makeEmptyPiece(), 1); // oak_tree placeholder

    registry.registerPool(std::move(trees));

    // ========================================================================
    // village/plains/decor - 装饰池
    // ========================================================================
    auto decor = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/plains/decor"), ResourceLocation("minecraft", "empty"));

    decor->addPiece(makeSinglePiece("minecraft:village/plains/decor/plains_lamp_1"), 2);
    decor->addPiece(makeEmptyPiece(), 1); // oak_tree placeholder
    decor->addPiece(makeEmptyPiece(), 1); // feature placeholder
    decor->addPiece(makeEmptyPiece(), 1); // feature placeholder
    decor->addPiece(makeEmptyPiece(), 2);

    registry.registerPool(std::move(decor));

    // ========================================================================
    // village/plains/zombie/decor - 僵尸村庄装饰池
    // ========================================================================
    auto zombieDecor = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/plains/zombie/decor"), ResourceLocation("minecraft", "empty"));

    zombieDecor->addPiece(makeSinglePiece("minecraft:village/plains/decor/plains_lamp_1"), 1);
    zombieDecor->addPiece(makeEmptyPiece(), 1);
    zombieDecor->addPiece(makeEmptyPiece(), 1);
    zombieDecor->addPiece(makeEmptyPiece(), 1);
    zombieDecor->addPiece(makeEmptyPiece(), 2);

    registry.registerPool(std::move(zombieDecor));

    // ========================================================================
    // village/plains/villagers - 村民池
    // ========================================================================
    auto villagers = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/plains/villagers"), ResourceLocation("minecraft", "empty"));

    villagers->addPiece(makeEmptyPiece(), 1);  // nitwit placeholder
    villagers->addPiece(makeEmptyPiece(), 1);  // baby placeholder
    villagers->addPiece(makeEmptyPiece(), 10); // unemployed placeholder

    registry.registerPool(std::move(villagers));

    // ========================================================================
    // village/plains/zombie/villagers - 僵尸村民池
    // ========================================================================
    auto zombieVillagers = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/plains/zombie/villagers"), ResourceLocation("minecraft", "empty"));

    zombieVillagers->addPiece(makeEmptyPiece(), 1);  // zombie_nitwit placeholder
    zombieVillagers->addPiece(makeEmptyPiece(), 10); // zombie_unemployed placeholder

    registry.registerPool(std::move(zombieVillagers));
}

} // namespace PlainsVillagePools

// ============================================================================
// DesertVillagePools 实现
// ============================================================================

namespace DesertVillagePools {

void registerAll(TemplatePoolRegistry& registry)
{
    // ========================================================================
    // village/desert/town_centers - 起始池
    // 正常村庄中心
    // 僵尸村庄中心 (zombie meeting_point: 2, 2, 1)
    // ========================================================================
    auto townCenters = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/desert/town_centers"), ResourceLocation("minecraft", "empty"));

    // 正常村庄
    townCenters->addPiece(makeSinglePiece("minecraft:village/desert/town_centers/desert_meeting_point_1"), 98);
    townCenters->addPiece(makeSinglePiece("minecraft:village/desert/town_centers/desert_meeting_point_2"), 98);
    townCenters->addPiece(makeSinglePiece("minecraft:village/desert/town_centers/desert_meeting_point_3"), 49);

    // 僵尸村庄
    townCenters->addPiece(makeSinglePiece("minecraft:village/desert/zombie/town_centers/desert_meeting_point_1"), 2);
    townCenters->addPiece(makeSinglePiece("minecraft:village/desert/zombie/town_centers/desert_meeting_point_2"), 2);
    townCenters->addPiece(makeSinglePiece("minecraft:village/desert/zombie/town_centers/desert_meeting_point_3"), 1);

    registry.registerPool(std::move(townCenters));

    // ========================================================================
    // village/desert/streets - 街道池
    // ========================================================================
    auto streets = std::make_unique<TemplatePool>(ResourceLocation("minecraft", "village/desert/streets"),
        ResourceLocation("minecraft", "village/desert/terminators"));

    streets->addPiece(makeSinglePiece("minecraft:village/desert/streets/corner_01"), 3);
    streets->addPiece(makeSinglePiece("minecraft:village/desert/streets/corner_02"), 3);
    streets->addPiece(makeSinglePiece("minecraft:village/desert/streets/straight_01"), 4);
    streets->addPiece(makeSinglePiece("minecraft:village/desert/streets/straight_02"), 4);
    streets->addPiece(makeSinglePiece("minecraft:village/desert/streets/straight_03"), 3);
    streets->addPiece(makeSinglePiece("minecraft:village/desert/streets/crossroad_01"), 3);
    streets->addPiece(makeSinglePiece("minecraft:village/desert/streets/crossroad_02"), 3);
    streets->addPiece(makeSinglePiece("minecraft:village/desert/streets/crossroad_03"), 3);
    streets->addPiece(makeSinglePiece("minecraft:village/desert/streets/square_01"), 3);
    streets->addPiece(makeSinglePiece("minecraft:village/desert/streets/square_02"), 3);
    streets->addPiece(makeSinglePiece("minecraft:village/desert/streets/turn_01"), 3);

    registry.registerPool(std::move(streets));

    // ========================================================================
    // village/desert/zombie/streets - 僵尸村庄街道
    // ========================================================================
    auto zombieStreets = std::make_unique<TemplatePool>(ResourceLocation("minecraft", "village/desert/zombie/streets"),
        ResourceLocation("minecraft", "village/desert/zombie/terminators"));

    zombieStreets->addPiece(makeSinglePiece("minecraft:village/desert/zombie/streets/corner_01"), 3);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/desert/zombie/streets/corner_02"), 3);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/desert/zombie/streets/straight_01"), 4);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/desert/zombie/streets/straight_02"), 4);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/desert/zombie/streets/straight_03"), 3);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/desert/zombie/streets/crossroad_01"), 3);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/desert/zombie/streets/crossroad_02"), 3);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/desert/zombie/streets/crossroad_03"), 3);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/desert/zombie/streets/square_01"), 3);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/desert/zombie/streets/square_02"), 3);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/desert/zombie/streets/turn_01"), 3);

    registry.registerPool(std::move(zombieStreets));

    // ========================================================================
    // village/desert/houses - 房屋池
    // ========================================================================
    auto houses = std::make_unique<TemplatePool>(ResourceLocation("minecraft", "village/desert/houses"),
        ResourceLocation("minecraft", "village/desert/terminators"));

    // 小型房屋
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_small_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_small_house_2"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_small_house_3"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_small_house_4"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_small_house_5"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_small_house_6"), 1);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_small_house_7"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_small_house_8"), 2);

    // 中型房屋
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_medium_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_medium_house_2"), 2);

    // 职业建筑
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_butcher_shop_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_tool_smith_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_fletcher_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_shepherd_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_armorer_1"), 1);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_fisher_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_tannery_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_cartographer_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_library_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_mason_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_weaponsmith_1"), 2);

    // 神殿
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_temple_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_temple_2"), 2);

    // 农场
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_large_farm_1"), 11);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_farm_1"), 4);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_farm_2"), 4);

    // 动物圈
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_animal_pen_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_animal_pen_2"), 2);

    // 空元素
    houses->addPiece(makeEmptyPiece(), 5);

    registry.registerPool(std::move(houses));

    // ========================================================================
    // village/desert/zombie/houses - 僵尸村庄房屋
    // ========================================================================
    auto zombieHouses = std::make_unique<TemplatePool>(ResourceLocation("minecraft", "village/desert/zombie/houses"),
        ResourceLocation("minecraft", "village/desert/zombie/terminators"));

    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/zombie/houses/desert_small_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/zombie/houses/desert_small_house_2"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/zombie/houses/desert_small_house_3"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/zombie/houses/desert_small_house_4"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/zombie/houses/desert_small_house_5"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/zombie/houses/desert_small_house_6"), 1);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/zombie/houses/desert_small_house_7"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/zombie/houses/desert_small_house_8"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/zombie/houses/desert_medium_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/zombie/houses/desert_medium_house_2"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_butcher_shop_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_tool_smith_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_fletcher_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_shepherd_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_armorer_1"), 1);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_fisher_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_tannery_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_cartographer_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_library_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_mason_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_weaponsmith_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_temple_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_temple_2"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_large_farm_1"), 7);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_farm_1"), 4);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_farm_2"), 4);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_animal_pen_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/desert/houses/desert_animal_pen_2"), 2);
    zombieHouses->addPiece(makeEmptyPiece(), 5);

    registry.registerPool(std::move(zombieHouses));

    // ========================================================================
    // village/desert/terminators - 终止池
    // ========================================================================
    auto terminators = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/desert/terminators"), ResourceLocation("minecraft", "empty"));

    terminators->addPiece(makeSinglePiece("minecraft:village/desert/terminators/terminator_01"), 1);
    terminators->addPiece(makeSinglePiece("minecraft:village/desert/terminators/terminator_02"), 1);

    registry.registerPool(std::move(terminators));

    // ========================================================================
    // village/desert/zombie/terminators - 僵尸村庄终止池
    // ========================================================================
    auto zombieTerminators = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/desert/zombie/terminators"), ResourceLocation("minecraft", "empty"));

    zombieTerminators->addPiece(makeSinglePiece("minecraft:village/desert/terminators/terminator_01"), 1);
    zombieTerminators->addPiece(makeSinglePiece("minecraft:village/desert/zombie/terminators/terminator_02"), 1);

    registry.registerPool(std::move(zombieTerminators));

    // ========================================================================
    // village/desert/decor - 装饰池
    // ========================================================================
    auto decor = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/desert/decor"), ResourceLocation("minecraft", "empty"));

    decor->addPiece(makeSinglePiece("minecraft:village/desert/desert_lamp_1"), 10);
    decor->addPiece(makeEmptyPiece(), 4); // Features placeholder
    decor->addPiece(makeEmptyPiece(), 4); // Features placeholder
    decor->addPiece(makeEmptyPiece(), 10);

    registry.registerPool(std::move(decor));

    // ========================================================================
    // village/desert/zombie/decor - 僵尸村庄装饰池
    // ========================================================================
    auto zombieDecor = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/desert/zombie/decor"), ResourceLocation("minecraft", "empty"));

    zombieDecor->addPiece(makeSinglePiece("minecraft:village/desert/desert_lamp_1"), 10);
    zombieDecor->addPiece(makeEmptyPiece(), 4);
    zombieDecor->addPiece(makeEmptyPiece(), 4);
    zombieDecor->addPiece(makeEmptyPiece(), 10);

    registry.registerPool(std::move(zombieDecor));

    // ========================================================================
    // village/desert/villagers - 村民池
    // ========================================================================
    auto villagers = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/desert/villagers"), ResourceLocation("minecraft", "empty"));

    villagers->addPiece(makeSinglePiece("minecraft:village/desert/villagers/nitwit"), 1);
    villagers->addPiece(makeSinglePiece("minecraft:village/desert/villagers/baby"), 1);
    villagers->addPiece(makeSinglePiece("minecraft:village/desert/villagers/unemployed"), 10);

    registry.registerPool(std::move(villagers));

    // ========================================================================
    // village/desert/zombie/villagers - 僵尸村民池
    // ========================================================================
    auto zombieVillagers = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/desert/zombie/villagers"), ResourceLocation("minecraft", "empty"));

    zombieVillagers->addPiece(makeSinglePiece("minecraft:village/desert/zombie/villagers/nitwit"), 1);
    zombieVillagers->addPiece(makeSinglePiece("minecraft:village/desert/zombie/villagers/unemployed"), 10);

    registry.registerPool(std::move(zombieVillagers));
}

} // namespace DesertVillagePools

// ============================================================================
// SavannaVillagePools 实现
// ============================================================================

namespace SavannaVillagePools {

void registerAll(TemplatePoolRegistry& registry)
{
    // ========================================================================
    // village/savanna/town_centers - 起始池
    // 正常村庄
    // 僵尸: meeting_point_1: 2, meeting_point_2: 1, meeting_point_3: 3, meeting_point_4: 3
    // ========================================================================
    auto townCenters = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/savanna/town_centers"), ResourceLocation("minecraft", "empty"));

    // 正常村庄
    townCenters->addPiece(makeSinglePiece("minecraft:village/savanna/town_centers/savanna_meeting_point_1"), 100);
    townCenters->addPiece(makeSinglePiece("minecraft:village/savanna/town_centers/savanna_meeting_point_2"), 50);
    townCenters->addPiece(makeSinglePiece("minecraft:village/savanna/town_centers/savanna_meeting_point_3"), 150);
    townCenters->addPiece(makeSinglePiece("minecraft:village/savanna/town_centers/savanna_meeting_point_4"), 150);

    // 僵尸村庄
    townCenters->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/town_centers/savanna_meeting_point_1"), 2);
    townCenters->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/town_centers/savanna_meeting_point_2"), 1);
    townCenters->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/town_centers/savanna_meeting_point_3"), 3);
    townCenters->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/town_centers/savanna_meeting_point_4"), 3);

    registry.registerPool(std::move(townCenters));

    // ========================================================================
    // village/savanna/streets - 街道池 (TerrainMatching)
    // ========================================================================
    auto streets = std::make_unique<TemplatePool>(ResourceLocation("minecraft", "village/savanna/streets"),
        ResourceLocation("minecraft", "village/savanna/terminators"));

    streets->addPiece(makeSinglePiece("minecraft:village/savanna/streets/corner_01"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/savanna/streets/corner_03"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/savanna/streets/straight_02"), 4);
    streets->addPiece(makeSinglePiece("minecraft:village/savanna/streets/straight_04"), 7);
    streets->addPiece(makeSinglePiece("minecraft:village/savanna/streets/straight_05"), 3);
    streets->addPiece(makeSinglePiece("minecraft:village/savanna/streets/straight_06"), 4);
    streets->addPiece(makeSinglePiece("minecraft:village/savanna/streets/straight_08"), 4);
    streets->addPiece(makeSinglePiece("minecraft:village/savanna/streets/straight_09"), 4);
    streets->addPiece(makeSinglePiece("minecraft:village/savanna/streets/straight_10"), 4);
    streets->addPiece(makeSinglePiece("minecraft:village/savanna/streets/straight_11"), 4);
    streets->addPiece(makeSinglePiece("minecraft:village/savanna/streets/crossroad_02"), 1);
    streets->addPiece(makeSinglePiece("minecraft:village/savanna/streets/crossroad_03"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/savanna/streets/crossroad_04"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/savanna/streets/crossroad_05"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/savanna/streets/crossroad_06"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/savanna/streets/crossroad_07"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/savanna/streets/split_01"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/savanna/streets/split_02"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/savanna/streets/turn_01"), 3);

    registry.registerPool(std::move(streets));

    // ========================================================================
    // village/savanna/zombie/streets - 僵尸村庄街道
    // ========================================================================
    auto zombieStreets = std::make_unique<TemplatePool>(ResourceLocation("minecraft", "village/savanna/zombie/streets"),
        ResourceLocation("minecraft", "village/savanna/zombie/terminators"));

    zombieStreets->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/streets/corner_01"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/streets/corner_03"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/streets/straight_02"), 4);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/streets/straight_04"), 7);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/streets/straight_05"), 3);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/streets/straight_06"), 4);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/streets/straight_08"), 4);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/streets/straight_09"), 4);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/streets/straight_10"), 4);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/streets/straight_11"), 4);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/streets/crossroad_02"), 1);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/streets/crossroad_03"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/streets/crossroad_04"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/streets/crossroad_05"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/streets/crossroad_06"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/streets/crossroad_07"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/streets/split_01"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/streets/split_02"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/streets/turn_01"), 3);

    registry.registerPool(std::move(zombieStreets));

    // ========================================================================
    // village/savanna/houses - 房屋池
    // ========================================================================
    auto houses = std::make_unique<TemplatePool>(ResourceLocation("minecraft", "village/savanna/houses"),
        ResourceLocation("minecraft", "village/savanna/terminators"));

    // 小型房屋
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_small_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_small_house_2"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_small_house_3"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_small_house_4"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_small_house_5"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_small_house_6"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_small_house_7"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_small_house_8"), 2);

    // 中型房屋
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_medium_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_medium_house_2"), 2);

    // 职业建筑
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_butchers_shop_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_butchers_shop_2"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_tool_smith_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_fletcher_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_shepherd_1"), 7);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_armorer_1"), 1);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_fisher_cottage_1"), 3);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_tannery_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_cartographer_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_library_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_mason_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_weaponsmith_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_weaponsmith_2"), 2);

    // 神殿
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_temple_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_temple_2"), 3);

    // 农场
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_large_farm_1"), 4);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_large_farm_2"), 6);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_small_farm"), 4);

    // 动物圈
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_animal_pen_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_animal_pen_2"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_animal_pen_3"), 2);

    // 空元素
    houses->addPiece(makeEmptyPiece(), 5);

    registry.registerPool(std::move(houses));

    // ========================================================================
    // village/savanna/zombie/houses - 僵尸村庄房屋
    // ========================================================================
    auto zombieHouses = std::make_unique<TemplatePool>(ResourceLocation("minecraft", "village/savanna/zombie/houses"),
        ResourceLocation("minecraft", "village/savanna/zombie/terminators"));

    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/houses/savanna_small_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/houses/savanna_small_house_2"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/houses/savanna_small_house_3"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/houses/savanna_small_house_4"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/houses/savanna_small_house_5"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/houses/savanna_small_house_6"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/houses/savanna_small_house_7"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/houses/savanna_small_house_8"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/houses/savanna_medium_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/houses/savanna_medium_house_2"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_butchers_shop_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_butchers_shop_2"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_tool_smith_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_fletcher_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_shepherd_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_armorer_1"), 1);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_fisher_cottage_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_tannery_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_cartographer_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_library_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_mason_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_weaponsmith_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_weaponsmith_2"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_temple_1"), 1);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_temple_2"), 3);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_large_farm_1"), 4);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/houses/savanna_large_farm_2"), 4);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_small_farm"), 4);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/houses/savanna_animal_pen_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/houses/savanna_animal_pen_2"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/houses/savanna_animal_pen_3"), 2);
    zombieHouses->addPiece(makeEmptyPiece(), 5);

    registry.registerPool(std::move(zombieHouses));

    // ========================================================================
    // village/savanna/terminators - 终止池
    // ========================================================================
    auto terminators = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/savanna/terminators"), ResourceLocation("minecraft", "empty"));

    terminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_01"), 1);
    terminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_02"), 1);
    terminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_03"), 1);
    terminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_04"), 1);
    terminators->addPiece(makeSinglePiece("minecraft:village/savanna/terminators/terminator_05"), 1);

    registry.registerPool(std::move(terminators));

    // ========================================================================
    // village/savanna/zombie/terminators - 僵尸村庄终止池
    // ========================================================================
    auto zombieTerminators = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/savanna/zombie/terminators"), ResourceLocation("minecraft", "empty"));

    zombieTerminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_01"), 1);
    zombieTerminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_02"), 1);
    zombieTerminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_03"), 1);
    zombieTerminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_04"), 1);
    zombieTerminators->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/terminators/terminator_05"), 1);

    registry.registerPool(std::move(zombieTerminators));

    // ========================================================================
    // village/savanna/trees - 树木池
    // ========================================================================
    auto trees = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/savanna/trees"), ResourceLocation("minecraft", "empty"));

    trees->addPiece(makeEmptyPiece(), 1); // Acacia tree placeholder

    registry.registerPool(std::move(trees));

    // ========================================================================
    // village/savanna/decor - 装饰池
    // ========================================================================
    auto decor = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/savanna/decor"), ResourceLocation("minecraft", "empty"));

    decor->addPiece(makeSinglePiece("minecraft:village/savanna/savanna_lamp_post_01"), 4);
    decor->addPiece(makeEmptyPiece(), 4); // Acacia tree placeholder
    decor->addPiece(makeEmptyPiece(), 4); // Feature placeholder
    decor->addPiece(makeEmptyPiece(), 1); // Feature placeholder
    decor->addPiece(makeEmptyPiece(), 4);

    registry.registerPool(std::move(decor));

    // ========================================================================
    // village/savanna/zombie/decor - 僵尸村庄装饰池
    // ========================================================================
    auto zombieDecor = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/savanna/zombie/decor"), ResourceLocation("minecraft", "empty"));

    zombieDecor->addPiece(makeSinglePiece("minecraft:village/savanna/savanna_lamp_post_01"), 4);
    zombieDecor->addPiece(makeEmptyPiece(), 4);
    zombieDecor->addPiece(makeEmptyPiece(), 4);
    zombieDecor->addPiece(makeEmptyPiece(), 1);
    zombieDecor->addPiece(makeEmptyPiece(), 4);

    registry.registerPool(std::move(zombieDecor));

    // ========================================================================
    // village/savanna/villagers - 村民池
    // ========================================================================
    auto villagers = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/savanna/villagers"), ResourceLocation("minecraft", "empty"));

    villagers->addPiece(makeSinglePiece("minecraft:village/savanna/villagers/nitwit"), 1);
    villagers->addPiece(makeSinglePiece("minecraft:village/savanna/villagers/baby"), 1);
    villagers->addPiece(makeSinglePiece("minecraft:village/savanna/villagers/unemployed"), 10);

    registry.registerPool(std::move(villagers));

    // ========================================================================
    // village/savanna/zombie/villagers - 僵尸村民池
    // ========================================================================
    auto zombieVillagers = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/savanna/zombie/villagers"), ResourceLocation("minecraft", "empty"));

    zombieVillagers->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/villagers/nitwit"), 1);
    zombieVillagers->addPiece(makeSinglePiece("minecraft:village/savanna/zombie/villagers/unemployed"), 10);

    registry.registerPool(std::move(zombieVillagers));
}

} // namespace SavannaVillagePools

// ============================================================================
// SnowyVillagePools 实现
// ============================================================================

namespace SnowyVillagePools {

void registerAll(TemplatePoolRegistry& registry)
{
    // ========================================================================
    // village/snowy/town_centers - 起始池
    // 正常村庄中心
    // 僵尸: meeting_point_1: 2, meeting_point_2: 1, meeting_point_3: 3
    // ========================================================================
    auto townCenters = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/snowy/town_centers"), ResourceLocation("minecraft", "empty"));

    // 正常村庄
    townCenters->addPiece(makeSinglePiece("minecraft:village/snowy/town_centers/snowy_meeting_point_1"), 100);
    townCenters->addPiece(makeSinglePiece("minecraft:village/snowy/town_centers/snowy_meeting_point_2"), 50);
    townCenters->addPiece(makeSinglePiece("minecraft:village/snowy/town_centers/snowy_meeting_point_3"), 150);

    // 僵尸村庄
    townCenters->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/town_centers/snowy_meeting_point_1"), 2);
    townCenters->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/town_centers/snowy_meeting_point_2"), 1);
    townCenters->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/town_centers/snowy_meeting_point_3"), 3);

    registry.registerPool(std::move(townCenters));

    // ========================================================================
    // village/snowy/streets - 街道池 (TerrainMatching)
    // ========================================================================
    auto streets = std::make_unique<TemplatePool>(ResourceLocation("minecraft", "village/snowy/streets"),
        ResourceLocation("minecraft", "village/snowy/terminators"));

    streets->addPiece(makeSinglePiece("minecraft:village/snowy/streets/corner_01"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/snowy/streets/corner_02"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/snowy/streets/corner_03"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/snowy/streets/square_01"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/snowy/streets/straight_01"), 4);
    streets->addPiece(makeSinglePiece("minecraft:village/snowy/streets/straight_02"), 4);
    streets->addPiece(makeSinglePiece("minecraft:village/snowy/streets/straight_03"), 4);
    streets->addPiece(makeSinglePiece("minecraft:village/snowy/streets/straight_04"), 7);
    streets->addPiece(makeSinglePiece("minecraft:village/snowy/streets/straight_06"), 4);
    streets->addPiece(makeSinglePiece("minecraft:village/snowy/streets/straight_08"), 4);
    streets->addPiece(makeSinglePiece("minecraft:village/snowy/streets/crossroad_02"), 1);
    streets->addPiece(makeSinglePiece("minecraft:village/snowy/streets/crossroad_03"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/snowy/streets/crossroad_04"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/snowy/streets/crossroad_05"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/snowy/streets/crossroad_06"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/snowy/streets/turn_01"), 3);

    registry.registerPool(std::move(streets));

    // ========================================================================
    // village/snowy/zombie/streets - 僵尸村庄街道
    // ========================================================================
    auto zombieStreets = std::make_unique<TemplatePool>(ResourceLocation("minecraft", "village/snowy/zombie/streets"),
        ResourceLocation("minecraft", "village/snowy/terminators"));

    zombieStreets->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/streets/corner_01"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/streets/corner_02"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/streets/corner_03"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/streets/square_01"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/streets/straight_01"), 4);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/streets/straight_02"), 4);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/streets/straight_03"), 4);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/streets/straight_04"), 7);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/streets/straight_06"), 4);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/streets/straight_08"), 4);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/streets/crossroad_02"), 1);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/streets/crossroad_03"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/streets/crossroad_04"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/streets/crossroad_05"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/streets/crossroad_06"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/streets/turn_01"), 3);

    registry.registerPool(std::move(zombieStreets));

    // ========================================================================
    // village/snowy/houses - 房屋池
    // ========================================================================
    auto houses = std::make_unique<TemplatePool>(ResourceLocation("minecraft", "village/snowy/houses"),
        ResourceLocation("minecraft", "village/snowy/terminators"));

    // 小型房屋
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_small_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_small_house_2"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_small_house_3"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_small_house_4"), 3);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_small_house_5"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_small_house_6"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_small_house_7"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_small_house_8"), 2);

    // 中型房屋
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_medium_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_medium_house_2"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_medium_house_3"), 2);

    // 职业建筑
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_butchers_shop_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_butchers_shop_2"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_tool_smith_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_fletcher_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_shepherds_house_1"), 3);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_armorer_house_1"), 1);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_armorer_house_2"), 1);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_fisher_cottage"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_tannery_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_cartographer_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_library_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_masons_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_masons_house_2"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_weapon_smith_1"), 2);

    // 神殿
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_temple_1"), 2);

    // 农场
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_farm_1"), 3);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_farm_2"), 3);

    // 动物圈
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_animal_pen_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_animal_pen_2"), 2);

    // 空元素
    houses->addPiece(makeEmptyPiece(), 6);

    registry.registerPool(std::move(houses));

    // ========================================================================
    // village/snowy/zombie/houses - 僵尸村庄房屋
    // ========================================================================
    auto zombieHouses = std::make_unique<TemplatePool>(ResourceLocation("minecraft", "village/snowy/zombie/houses"),
        ResourceLocation("minecraft", "village/snowy/terminators"));

    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/houses/snowy_small_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/houses/snowy_small_house_2"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/houses/snowy_small_house_3"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/houses/snowy_small_house_4"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/houses/snowy_small_house_5"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/houses/snowy_small_house_6"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/houses/snowy_small_house_7"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/houses/snowy_small_house_8"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/houses/snowy_medium_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/houses/snowy_medium_house_2"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/houses/snowy_medium_house_3"), 1);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_butchers_shop_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_butchers_shop_2"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_tool_smith_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_fletcher_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_shepherds_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_armorer_house_1"), 1);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_armorer_house_2"), 1);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_fisher_cottage"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_tannery_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_cartographer_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_library_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_masons_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_masons_house_2"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_weapon_smith_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_temple_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_farm_1"), 3);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_farm_2"), 3);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_animal_pen_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/snowy/houses/snowy_animal_pen_2"), 2);
    zombieHouses->addPiece(makeEmptyPiece(), 6);

    registry.registerPool(std::move(zombieHouses));

    // ========================================================================
    // village/snowy/terminators - 终止池
    // ========================================================================
    auto terminators = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/snowy/terminators"), ResourceLocation("minecraft", "empty"));

    terminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_01"), 1);
    terminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_02"), 1);
    terminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_03"), 1);
    terminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_04"), 1);

    registry.registerPool(std::move(terminators));

    // ========================================================================
    // village/snowy/trees - 树木池
    // ========================================================================
    auto trees = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/snowy/trees"), ResourceLocation("minecraft", "empty"));

    trees->addPiece(makeEmptyPiece(), 1); // Spruce tree placeholder

    registry.registerPool(std::move(trees));

    // ========================================================================
    // village/snowy/decor - 装饰池
    // ========================================================================
    auto decor = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/snowy/decor"), ResourceLocation("minecraft", "empty"));

    decor->addPiece(makeSinglePiece("minecraft:village/snowy/snowy_lamp_post_01"), 4);
    decor->addPiece(makeSinglePiece("minecraft:village/snowy/snowy_lamp_post_02"), 4);
    decor->addPiece(makeSinglePiece("minecraft:village/snowy/snowy_lamp_post_03"), 1);
    decor->addPiece(makeEmptyPiece(), 4); // Spruce tree placeholder
    decor->addPiece(makeEmptyPiece(), 4); // Feature placeholder
    decor->addPiece(makeEmptyPiece(), 1); // Feature placeholder
    decor->addPiece(makeEmptyPiece(), 9);

    registry.registerPool(std::move(decor));

    // ========================================================================
    // village/snowy/zombie/decor - 僵尸村庄装饰池
    // ========================================================================
    auto zombieDecor = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/snowy/zombie/decor"), ResourceLocation("minecraft", "empty"));

    zombieDecor->addPiece(makeSinglePiece("minecraft:village/snowy/snowy_lamp_post_01"), 1);
    zombieDecor->addPiece(makeSinglePiece("minecraft:village/snowy/snowy_lamp_post_02"), 1);
    zombieDecor->addPiece(makeSinglePiece("minecraft:village/snowy/snowy_lamp_post_03"), 1);
    zombieDecor->addPiece(makeEmptyPiece(), 4);
    zombieDecor->addPiece(makeEmptyPiece(), 4);
    zombieDecor->addPiece(makeEmptyPiece(), 4);
    zombieDecor->addPiece(makeEmptyPiece(), 7);

    registry.registerPool(std::move(zombieDecor));

    // ========================================================================
    // village/snowy/villagers - 村民池
    // ========================================================================
    auto villagers = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/snowy/villagers"), ResourceLocation("minecraft", "empty"));

    villagers->addPiece(makeSinglePiece("minecraft:village/snowy/villagers/nitwit"), 1);
    villagers->addPiece(makeSinglePiece("minecraft:village/snowy/villagers/baby"), 1);
    villagers->addPiece(makeSinglePiece("minecraft:village/snowy/villagers/unemployed"), 10);

    registry.registerPool(std::move(villagers));

    // ========================================================================
    // village/snowy/zombie/villagers - 僵尸村民池
    // ========================================================================
    auto zombieVillagers = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/snowy/zombie/villagers"), ResourceLocation("minecraft", "empty"));

    zombieVillagers->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/villagers/nitwit"), 1);
    zombieVillagers->addPiece(makeSinglePiece("minecraft:village/snowy/zombie/villagers/unemployed"), 10);

    registry.registerPool(std::move(zombieVillagers));
}

} // namespace SnowyVillagePools

// ============================================================================
// TaigaVillagePools 实现
// ============================================================================

namespace TaigaVillagePools {

void registerAll(TemplatePoolRegistry& registry)
{
    // ========================================================================
    // village/taiga/town_centers - 起始池
    // 正常村庄
    // 僵尸: meeting_point_1: 1, meeting_point_2: 1
    // ========================================================================
    auto townCenters = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/taiga/town_centers"), ResourceLocation("minecraft", "empty"));

    // 正常村庄 (10% mossy)
    townCenters->addPiece(makeSinglePiece("minecraft:village/taiga/town_centers/taiga_meeting_point_1"), 49);
    townCenters->addPiece(makeSinglePiece("minecraft:village/taiga/town_centers/taiga_meeting_point_2"), 49);

    // 僵尸村庄
    townCenters->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/town_centers/taiga_meeting_point_1"), 1);
    townCenters->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/town_centers/taiga_meeting_point_2"), 1);

    registry.registerPool(std::move(townCenters));

    // ========================================================================
    // village/taiga/streets - 街道池 (TerrainMatching)
    // ========================================================================
    auto streets = std::make_unique<TemplatePool>(ResourceLocation("minecraft", "village/taiga/streets"),
        ResourceLocation("minecraft", "village/taiga/terminators"));

    streets->addPiece(makeSinglePiece("minecraft:village/taiga/streets/corner_01"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/taiga/streets/corner_02"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/taiga/streets/corner_03"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/taiga/streets/straight_01"), 4);
    streets->addPiece(makeSinglePiece("minecraft:village/taiga/streets/straight_02"), 4);
    streets->addPiece(makeSinglePiece("minecraft:village/taiga/streets/straight_03"), 4);
    streets->addPiece(makeSinglePiece("minecraft:village/taiga/streets/straight_04"), 7);
    streets->addPiece(makeSinglePiece("minecraft:village/taiga/streets/straight_05"), 7);
    streets->addPiece(makeSinglePiece("minecraft:village/taiga/streets/straight_06"), 4);
    streets->addPiece(makeSinglePiece("minecraft:village/taiga/streets/crossroad_01"), 1);
    streets->addPiece(makeSinglePiece("minecraft:village/taiga/streets/crossroad_02"), 1);
    streets->addPiece(makeSinglePiece("minecraft:village/taiga/streets/crossroad_03"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/taiga/streets/crossroad_04"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/taiga/streets/crossroad_05"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/taiga/streets/crossroad_06"), 2);
    streets->addPiece(makeSinglePiece("minecraft:village/taiga/streets/turn_01"), 3);

    registry.registerPool(std::move(streets));

    // ========================================================================
    // village/taiga/zombie/streets - 僵尸村庄街道
    // ========================================================================
    auto zombieStreets = std::make_unique<TemplatePool>(ResourceLocation("minecraft", "village/taiga/zombie/streets"),
        ResourceLocation("minecraft", "village/taiga/terminators"));

    zombieStreets->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/streets/corner_01"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/streets/corner_02"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/streets/corner_03"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/streets/straight_01"), 4);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/streets/straight_02"), 4);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/streets/straight_03"), 4);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/streets/straight_04"), 7);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/streets/straight_05"), 7);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/streets/straight_06"), 4);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/streets/crossroad_01"), 1);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/streets/crossroad_02"), 1);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/streets/crossroad_03"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/streets/crossroad_04"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/streets/crossroad_05"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/streets/crossroad_06"), 2);
    zombieStreets->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/streets/turn_01"), 3);

    registry.registerPool(std::move(zombieStreets));

    // ========================================================================
    // village/taiga/houses - 房屋池
    // ========================================================================
    auto houses = std::make_unique<TemplatePool>(ResourceLocation("minecraft", "village/taiga/houses"),
        ResourceLocation("minecraft", "village/taiga/terminators"));

    // 小型房屋 (10% mossy)
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_small_house_1"), 4);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_small_house_2"), 4);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_small_house_3"), 4);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_small_house_4"), 4);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_small_house_5"), 4);

    // 中型房屋
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_medium_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_medium_house_2"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_medium_house_3"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_medium_house_4"), 2);

    // 职业建筑
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_butcher_shop_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_tool_smith_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_fletcher_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_shepherds_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_armorer_house_1"), 1);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_armorer_2"), 1);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_fisher_cottage_1"), 3);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_tannery_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_cartographer_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_library_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_masons_house_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_weaponsmith_1"), 2);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_weaponsmith_2"), 2);

    // 神殿
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_temple_1"), 2);

    // 农场
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_large_farm_1"), 6);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_large_farm_2"), 6);
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_small_farm_1"), 1);

    // 动物圈
    houses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_animal_pen_1"), 2);

    // 空元素
    houses->addPiece(makeEmptyPiece(), 6);

    registry.registerPool(std::move(houses));

    // ========================================================================
    // village/taiga/zombie/houses - 僵尸村庄房屋
    // ========================================================================
    auto zombieHouses = std::make_unique<TemplatePool>(ResourceLocation("minecraft", "village/taiga/zombie/houses"),
        ResourceLocation("minecraft", "village/taiga/terminators"));

    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/houses/taiga_small_house_1"), 4);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/houses/taiga_small_house_2"), 4);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/houses/taiga_small_house_3"), 4);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/houses/taiga_small_house_4"), 4);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/houses/taiga_small_house_5"), 4);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/houses/taiga_medium_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/houses/taiga_medium_house_2"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/houses/taiga_medium_house_3"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/houses/taiga_medium_house_4"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_butcher_shop_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/houses/taiga_tool_smith_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_fletcher_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/houses/taiga_shepherds_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_armorer_house_1"), 1);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/houses/taiga_fisher_cottage_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_tannery_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/houses/taiga_cartographer_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/houses/taiga_library_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_masons_house_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_weaponsmith_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/houses/taiga_weaponsmith_2"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/houses/taiga_temple_1"), 2);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_large_farm_1"), 6);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/houses/taiga_large_farm_2"), 6);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_small_farm_1"), 1);
    zombieHouses->addPiece(makeSinglePiece("minecraft:village/taiga/houses/taiga_animal_pen_1"), 2);
    zombieHouses->addPiece(makeEmptyPiece(), 6);

    registry.registerPool(std::move(zombieHouses));

    // ========================================================================
    // village/taiga/terminators - 终止池
    // ========================================================================
    auto terminators = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/taiga/terminators"), ResourceLocation("minecraft", "empty"));

    terminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_01"), 1);
    terminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_02"), 1);
    terminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_03"), 1);
    terminators->addPiece(makeSinglePiece("minecraft:village/plains/terminators/terminator_04"), 1);

    registry.registerPool(std::move(terminators));

    // ========================================================================
    // village/taiga/decor - 装饰池
    // ========================================================================
    auto decor = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/taiga/decor"), ResourceLocation("minecraft", "empty"));

    decor->addPiece(makeSinglePiece("minecraft:village/taiga/taiga_lamp_post_1"), 10);
    decor->addPiece(makeSinglePiece("minecraft:village/taiga/taiga_decoration_1"), 4);
    decor->addPiece(makeSinglePiece("minecraft:village/taiga/taiga_decoration_2"), 1);
    decor->addPiece(makeSinglePiece("minecraft:village/taiga/taiga_decoration_3"), 1);
    decor->addPiece(makeSinglePiece("minecraft:village/taiga/taiga_decoration_4"), 1);
    decor->addPiece(makeSinglePiece("minecraft:village/taiga/taiga_decoration_5"), 2);
    decor->addPiece(makeSinglePiece("minecraft:village/taiga/taiga_decoration_6"), 1);
    decor->addPiece(makeEmptyPiece(), 4); // Spruce tree placeholder
    decor->addPiece(makeEmptyPiece(), 4); // Feature placeholder
    decor->addPiece(makeEmptyPiece(), 2); // Feature placeholder
    decor->addPiece(makeEmptyPiece(), 4); // Feature placeholder
    decor->addPiece(makeEmptyPiece(), 1); // Feature placeholder
    decor->addPiece(makeEmptyPiece(), 4);

    registry.registerPool(std::move(decor));

    // ========================================================================
    // village/taiga/zombie/decor - 僵尸村庄装饰池
    // ========================================================================
    auto zombieDecor = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/taiga/zombie/decor"), ResourceLocation("minecraft", "empty"));

    zombieDecor->addPiece(makeSinglePiece("minecraft:village/taiga/taiga_decoration_1"), 4);
    zombieDecor->addPiece(makeSinglePiece("minecraft:village/taiga/taiga_decoration_2"), 1);
    zombieDecor->addPiece(makeSinglePiece("minecraft:village/taiga/taiga_decoration_3"), 1);
    zombieDecor->addPiece(makeSinglePiece("minecraft:village/taiga/taiga_decoration_4"), 1);
    zombieDecor->addPiece(makeEmptyPiece(), 4);
    zombieDecor->addPiece(makeEmptyPiece(), 4);
    zombieDecor->addPiece(makeEmptyPiece(), 2);
    zombieDecor->addPiece(makeEmptyPiece(), 4);
    zombieDecor->addPiece(makeEmptyPiece(), 1);
    zombieDecor->addPiece(makeEmptyPiece(), 4);

    registry.registerPool(std::move(zombieDecor));

    // ========================================================================
    // village/taiga/villagers - 村民池
    // ========================================================================
    auto villagers = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/taiga/villagers"), ResourceLocation("minecraft", "empty"));

    villagers->addPiece(makeSinglePiece("minecraft:village/taiga/villagers/nitwit"), 1);
    villagers->addPiece(makeSinglePiece("minecraft:village/taiga/villagers/baby"), 1);
    villagers->addPiece(makeSinglePiece("minecraft:village/taiga/villagers/unemployed"), 10);

    registry.registerPool(std::move(villagers));

    // ========================================================================
    // village/taiga/zombie/villagers - 僵尸村民池
    // ========================================================================
    auto zombieVillagers = std::make_unique<TemplatePool>(
        ResourceLocation("minecraft", "village/taiga/zombie/villagers"), ResourceLocation("minecraft", "empty"));

    zombieVillagers->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/villagers/nitwit"), 1);
    zombieVillagers->addPiece(makeSinglePiece("minecraft:village/taiga/zombie/villagers/unemployed"), 10);

    registry.registerPool(std::move(zombieVillagers));
}

} // namespace TaigaVillagePools

} // namespace pools
} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
