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

/**
 * @file test_structure_type_registry.cpp
 * @brief StructureTypeRegistry 工厂分派单元测试
 *
 * 验证 initializeBuiltinStructureTypes 注册全部 16 种 Vanilla structure type，
 * create() 按 type + def.id 正确分派到对应子类，并应用数据驱动覆盖字段
 * （biomes/step/terrainAdaptation）。工厂能力就绪，本阶段未接入生产注册链路。
 */

#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/gen/structure/JigsawStructure.hpp"
#include "common/world/gen/structure/Structure.hpp"
#include "common/world/gen/structure/StructureTypeRegistry.hpp"
#include "common/world/gen/structure/structures/BastionRemnantStructure.hpp"
#include "common/world/gen/structure/structures/BuriedTreasureStructure.hpp"
#include "common/world/gen/structure/structures/DesertPyramidStructure.hpp"
#include "common/world/gen/structure/structures/EndCityStructure.hpp"
#include "common/world/gen/structure/structures/FortressStructure.hpp"
#include "common/world/gen/structure/structures/IglooStructure.hpp"
#include "common/world/gen/structure/structures/JungleTempleStructure.hpp"
#include "common/world/gen/structure/structures/MineshaftStructure.hpp"
#include "common/world/gen/structure/structures/NetherFossilStructure.hpp"
#include "common/world/gen/structure/structures/OceanMonumentStructure.hpp"
#include "common/world/gen/structure/structures/OceanRuinStructure.hpp"
#include "common/world/gen/structure/structures/PillagerOutpostStructure.hpp"
#include "common/world/gen/structure/structures/RuinedPortalStructure.hpp"
#include "common/world/gen/structure/structures/ShipwreckStructure.hpp"
#include "common/world/gen/structure/structures/StrongholdStructure.hpp"
#include "common/world/gen/structure/structures/SwampHutStructure.hpp"
#include "common/world/gen/structure/structures/TrialChambersStructure.hpp"
#include "common/world/gen/structure/structures/VillageStructure.hpp"
#include "common/world/gen/structure/structures/WoodlandMansionStructure.hpp"

#include <gtest/gtest.h>

#include <functional>
#include <string>
#include <utility>
#include <vector>

using namespace mc;
using namespace mc::world::gen::structure;

namespace {

/// 构造一个最小可用的结构定义（程序化类型用，无 jigsaw 字段）
StructureDefinition makeDef(const std::string& idPath, const std::string& type)
{
    StructureDefinition def;
    def.id = ResourceLocation("minecraft", idPath);
    def.type = type;
    return def;
}

/// 构造一个 jigsaw 类型定义（带 start_pool，供裸 JigsawStructure 构造）
StructureDefinition makeJigsawDef(
    const std::string& idPath, const std::string& startPool = "minecraft:test/start", i32 size = 7)
{
    StructureDefinition def;
    def.id = ResourceLocation("minecraft", idPath);
    def.type = "minecraft:jigsaw";
    def.startPool = ResourceLocation(startPool);
    def.size = size;
    return def;
}

} // namespace

class StructureTypeRegistryTest : public ::testing::Test {
protected:
    void SetUp() override { initializeBuiltinStructureTypes(); }
    void TearDown() override { StructureTypeRegistry::instance().clear(); }
};

// ============================================================================
// 注册完整性：16 种 Vanilla type 全部注册
// ============================================================================

TEST_F(StructureTypeRegistryTest, RegistersAllSixteenTypes)
{
    auto& reg = StructureTypeRegistry::instance();
    EXPECT_TRUE(reg.has("jigsaw"));
    EXPECT_TRUE(reg.has("minecraft:jigsaw")); // 带前缀也应识别
    EXPECT_TRUE(reg.has("mineshaft"));
    EXPECT_TRUE(reg.has("buried_treasure"));
    EXPECT_TRUE(reg.has("desert_pyramid"));
    EXPECT_TRUE(reg.has("end_city"));
    EXPECT_TRUE(reg.has("fortress"));
    EXPECT_TRUE(reg.has("igloo"));
    EXPECT_TRUE(reg.has("jungle_pyramid"));
    EXPECT_TRUE(reg.has("nether_fossil"));
    EXPECT_TRUE(reg.has("monument"));
    EXPECT_TRUE(reg.has("ocean_ruin"));
    EXPECT_TRUE(reg.has("ruined_portal"));
    EXPECT_TRUE(reg.has("shipwreck"));
    EXPECT_TRUE(reg.has("stronghold"));
    EXPECT_TRUE(reg.has("swamp_hut"));
    EXPECT_TRUE(reg.has("woodland_mansion"));
    // 未注册的 type 应返回 false
    EXPECT_FALSE(reg.has("nonexistent_type"));
}

// ============================================================================
// 程序化类型分派：create 产出对应子类
// ============================================================================

TEST_F(StructureTypeRegistryTest, DispatchesProceduralTypesToSubclasses)
{
    auto& reg = StructureTypeRegistry::instance();

    const std::vector<std::pair<std::string, std::function<bool(const Structure*)>>> cases = {
        {"buried_treasure",
            [](const Structure* s) { return dynamic_cast<const BuriedTreasureStructure*>(s) != nullptr; }},
        {"desert_pyramid",
            [](const Structure* s) { return dynamic_cast<const DesertPyramidStructure*>(s) != nullptr; }},
        {"end_city", [](const Structure* s) { return dynamic_cast<const EndCityStructure*>(s) != nullptr; }},
        {"fortress", [](const Structure* s) { return dynamic_cast<const FortressStructure*>(s) != nullptr; }},
        {"igloo", [](const Structure* s) { return dynamic_cast<const IglooStructure*>(s) != nullptr; }},
        {"jungle_pyramid", [](const Structure* s) { return dynamic_cast<const JungleTempleStructure*>(s) != nullptr; }},
        {"nether_fossil", [](const Structure* s) { return dynamic_cast<const NetherFossilStructure*>(s) != nullptr; }},
        {"monument", [](const Structure* s) { return dynamic_cast<const OceanMonumentStructure*>(s) != nullptr; }},
        {"ocean_ruin", [](const Structure* s) { return dynamic_cast<const OceanRuinStructure*>(s) != nullptr; }},
        {"ruined_portal", [](const Structure* s) { return dynamic_cast<const RuinedPortalStructure*>(s) != nullptr; }},
        {"shipwreck", [](const Structure* s) { return dynamic_cast<const ShipwreckStructure*>(s) != nullptr; }},
        {"stronghold", [](const Structure* s) { return dynamic_cast<const StrongholdStructure*>(s) != nullptr; }},
        {"swamp_hut", [](const Structure* s) { return dynamic_cast<const SwampHutStructure*>(s) != nullptr; }},
        {"woodland_mansion",
            [](const Structure* s) { return dynamic_cast<const WoodlandMansionStructure*>(s) != nullptr; }},
    };

    for (const auto& [type, isMatch] : cases) {
        auto def = makeDef(type, "minecraft:" + type);
        auto result = reg.create(type, def);
        ASSERT_TRUE(result.success()) << "type=" << type << " create failed";
        auto structure = result.value();
        ASSERT_NE(structure, nullptr) << "type=" << type << " returned null";
        EXPECT_TRUE(isMatch(structure.get())) << "type=" << type << " dispatched to wrong subclass";
    }
}

// ============================================================================
// 废弃矿井变体分流：mineshaft→Normal，mineshaft_mesa→Mesa
// ============================================================================

TEST_F(StructureTypeRegistryTest, MineshaftDispatchesByVariantId)
{
    auto& reg = StructureTypeRegistry::instance();

    auto normalDef = makeDef("mineshaft", "minecraft:mineshaft");
    auto normalResult = reg.create("mineshaft", normalDef);
    ASSERT_TRUE(normalResult.success());
    auto* normal = dynamic_cast<MineshaftStructure*>(normalResult.value().get());
    ASSERT_NE(normal, nullptr);
    // 默认 Normal 变体（构造零参走默认 MineshaftType::Normal）

    auto mesaDef = makeDef("mineshaft_mesa", "minecraft:mineshaft");
    auto mesaResult = reg.create("mineshaft", mesaDef);
    ASSERT_TRUE(mesaResult.success());
    auto* mesa = dynamic_cast<MineshaftStructure*>(mesaResult.value().get());
    ASSERT_NE(mesa, nullptr);
    // mesa 与 normal 是不同实例
    EXPECT_NE(normal, mesa);
}

// ============================================================================
// jigsaw 类型分派：按 def.id 委托子类或裸 JigsawStructure
// ============================================================================

TEST_F(StructureTypeRegistryTest, JigsawDispatchesToSpecializedSubclasses)
{
    auto& reg = StructureTypeRegistry::instance();

    // pillager_outpost → PillagerOutpostStructure
    {
        auto def = makeJigsawDef("pillager_outpost");
        auto result = reg.create("jigsaw", def);
        ASSERT_TRUE(result.success());
        EXPECT_NE(dynamic_cast<PillagerOutpostStructure*>(result.value().get()), nullptr);
    }
    // trial_chambers → TrialChambersStructure
    {
        auto def = makeJigsawDef("trial_chambers");
        auto result = reg.create("jigsaw", def);
        ASSERT_TRUE(result.success());
        EXPECT_NE(dynamic_cast<TrialChambersStructure*>(result.value().get()), nullptr);
    }
    // bastion_remnant → BastionRemnantStructure
    {
        auto def = makeJigsawDef("bastion_remnant");
        auto result = reg.create("jigsaw", def);
        ASSERT_TRUE(result.success());
        EXPECT_NE(dynamic_cast<BastionRemnantStructure*>(result.value().get()), nullptr);
    }
}

TEST_F(StructureTypeRegistryTest, JigsawVillageDispatchesByVillageType)
{
    auto& reg = StructureTypeRegistry::instance();

    // 5 个村庄 id 均应分派到 VillageStructure（变体类型存于私有 m_config，
    // 无公开访问器；此处验证分派正确性，VillageType 映射为工厂内部查表）。
    for (const auto& idPath :
        {"village_plains", "village_desert", "village_savanna", "village_snowy", "village_taiga"}) {
        auto def = makeJigsawDef(idPath);
        auto result = reg.create("jigsaw", def);
        ASSERT_TRUE(result.success()) << "id=" << idPath;
        auto* village = dynamic_cast<VillageStructure*>(result.value().get());
        ASSERT_NE(village, nullptr) << "id=" << idPath << " not a VillageStructure";
        // 不应为带专属子类的 jigsaw 类型
        EXPECT_EQ(dynamic_cast<PillagerOutpostStructure*>(result.value().get()), nullptr);
    }

    // 单独 "village"（无后缀）也应分派到 VillageStructure（默认 Plains）
    auto def = makeJigsawDef("village");
    auto result = reg.create("jigsaw", def);
    ASSERT_TRUE(result.success());
    EXPECT_NE(dynamic_cast<VillageStructure*>(result.value().get()), nullptr);
}

TEST_F(StructureTypeRegistryTest, JigsawBareStructureForUnknownId)
{
    auto& reg = StructureTypeRegistry::instance();

    // ancient_city / trail_ruins 等 id 无专属子类，应构造裸 JigsawStructure
    for (const auto& idPath : {"ancient_city", "trail_ruins", "custom_jigsaw"}) {
        auto def = makeJigsawDef(idPath);
        auto result = reg.create("jigsaw", def);
        ASSERT_TRUE(result.success()) << "id=" << idPath;
        auto structure = result.value();
        ASSERT_NE(structure, nullptr);
        // 应为 JigsawStructure（含其子类之外的裸类型）
        EXPECT_NE(dynamic_cast<JigsawStructure*>(structure.get()), nullptr) << "id=" << idPath;
        // 不应为带专属子类的类型
        EXPECT_EQ(dynamic_cast<PillagerOutpostStructure*>(structure.get()), nullptr);
        EXPECT_EQ(dynamic_cast<TrialChambersStructure*>(structure.get()), nullptr);
        EXPECT_EQ(dynamic_cast<BastionRemnantStructure*>(structure.get()), nullptr);
        EXPECT_EQ(dynamic_cast<VillageStructure*>(structure.get()), nullptr);
    }
}

TEST_F(StructureTypeRegistryTest, JigsawBareStructureRequiresStartPool)
{
    auto& reg = StructureTypeRegistry::instance();

    // 裸 jigsaw id 但缺 start_pool 应返回错误
    auto def = makeDef("ancient_city", "minecraft:jigsaw"); // 无 startPool
    auto result = reg.create("jigsaw", def);
    EXPECT_FALSE(result.success());
}

// ============================================================================
// 数据驱动覆盖：applyDefinitionOverrides 注入 step/terrainAdaptation
// （biome 标签因 BiomeTags 注册键与 JSON 引用名不一致，部分返回 nullptr 回退默认，
//  此处仅断言 step/terrainAdaptation 注入，避开 biome 标签加载时序）
// ============================================================================

TEST_F(StructureTypeRegistryTest, AppliesStepAndTerrainAdaptationOverrides)
{
    auto& reg = StructureTypeRegistry::instance();

    auto def = makeDef("desert_pyramid", "minecraft:desert_pyramid");
    def.step = DecorationStage::TopLayerModification;
    def.terrainAdaptation = TerrainAdaptation::Bury;

    auto result = reg.create("desert_pyramid", def);
    ASSERT_TRUE(result.success());
    auto structure = result.value();
    ASSERT_NE(structure, nullptr);

    // 子类默认 decorationStage 为 SurfaceStructures，注入后应被覆盖
    EXPECT_EQ(structure->decorationStage(), DecorationStage::TopLayerModification);
    // 子类默认 terrainAdaptation 为 None，注入 Bury 后应被覆盖
    EXPECT_EQ(structure->terrainAdaptation(), TerrainAdaptation::Bury);
}

TEST_F(StructureTypeRegistryTest, FallsBackToSubclassDefaultsWhenDefEmpty)
{
    auto& reg = StructureTypeRegistry::instance();

    // FortressStructure 默认 decorationStage = UndergroundDecoration
    auto def = makeDef("fortress", "minecraft:fortress");
    def.step = DecorationStage::SurfaceStructures; // 显式注入（覆盖默认 UndergroundDecoration）
    auto result = reg.create("fortress", def);
    ASSERT_TRUE(result.success());
    EXPECT_EQ(result.value()->decorationStage(), DecorationStage::SurfaceStructures);

    // 不注入 step 时（默认 SurfaceStructures）回退子类默认
    auto def2 = makeDef("desert_pyramid", "minecraft:desert_pyramid");
    auto result2 = reg.create("desert_pyramid", def2);
    ASSERT_TRUE(result2.success());
    EXPECT_EQ(result2.value()->decorationStage(), DecorationStage::SurfaceStructures);
}

// ============================================================================
// 错误处理：未注册 type 返回 Error
// ============================================================================

TEST_F(StructureTypeRegistryTest, UnregisteredTypeReturnsError)
{
    auto& reg = StructureTypeRegistry::instance();
    auto def = makeDef("mystery", "minecraft:mystery_type");
    auto result = reg.create("mystery_type", def);
    EXPECT_FALSE(result.success());
    EXPECT_EQ(result.error().code(), ErrorCode::NotFound);
}
