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
 * @file test_structure_definition_loader.cpp
 * @brief StructureDefinitionLoader 字段解析单元测试
 *
 * 验证 spawn_overrides / pool_aliases / start_height(简写与分派两种形式) 的解析。
 * Loader 当前零生产消费者（数据驱动加载未接入 MinecraftServer），本测试直接调
 * loadFromJson + getDefinition 断言解析后的 StructureDefinition 字段。
 */

#include "common/resource/ResourceLocation.hpp"
#include "common/world/gen/structure/StructureDefinitionLoader.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::gen::structure;
using namespace mc::world::gen::jigsaw;

class StructureDefinitionLoaderTest : public ::testing::Test {
protected:
    void SetUp() override { StructureDefinitionLoader::clear(); }
    void TearDown() override { StructureDefinitionLoader::clear(); }

    /// 加载 JSON 并断言成功，返回解析后的定义指针
    static const StructureDefinition* loadOk(const std::string& json, const ResourceLocation& id)
    {
        auto result = StructureDefinitionLoader::loadFromJson(json, id);
        EXPECT_TRUE(result.success()) << "loadFromJson failed: " << result.error().message();
        return StructureDefinitionLoader::getDefinition(id);
    }
};

/// 村庄平原：jigsaw，简写 start_height（单键锚点 {absolute:0}），空 spawn_overrides，beard_thin
TEST_F(StructureDefinitionLoaderTest, ParseVillagePlainsShorthandHeight)
{
    const std::string json = R"({
        "type": "minecraft:jigsaw",
        "biomes": "#minecraft:has_structure/village_plains",
        "max_distance_from_center": 80,
        "project_start_to_heightmap": "WORLD_SURFACE_WG",
        "size": 6,
        "spawn_overrides": {},
        "start_height": { "absolute": 0 },
        "start_pool": "minecraft:village/plains/town_centers",
        "step": "surface_structures",
        "terrain_adaptation": "beard_thin",
        "use_expansion_hack": true
    })";

    const auto* def = loadOk(json, ResourceLocation("minecraft", "village_plains"));
    ASSERT_NE(def, nullptr);
    EXPECT_EQ(def->type, "minecraft:jigsaw");
    EXPECT_EQ(def->biomes, ResourceLocation("#minecraft:has_structure/village_plains"));
    EXPECT_EQ(def->startPool, ResourceLocation("minecraft:village/plains/town_centers"));
    EXPECT_EQ(def->size, 6);
    EXPECT_EQ(def->maxDistanceFromCenter.value().horizontal, 80);
    EXPECT_EQ(def->maxDistanceFromCenter.value().vertical, 80);
    EXPECT_TRUE(def->projectStartToHeightmap);
    EXPECT_EQ(def->heightmapName, "WORLD_SURFACE_WG");
    EXPECT_TRUE(def->useExpansionHack);
    // 简写 start_height 应解析为非空 ConstantHeight
    ASSERT_NE(def->startHeight, nullptr);
    // 空 spawn_overrides 解析为空 map
    EXPECT_TRUE(def->spawnOverrides.empty());
    // beard_thin
    EXPECT_EQ(def->terrainAdaptation, TerrainAdaptation::BeardThin);
    EXPECT_EQ(def->step, DecorationStage::SurfaceStructures);
}

/// 试炼密室：jigsaw，分派 start_height（uniform），完整 spawn_overrides（8 类别
/// piece），pool_aliases（direct/random/random_group）
TEST_F(StructureDefinitionLoaderTest, ParseTrialChambersDispatchedHeightAndAliases)
{
    const std::string json = R"({
        "type": "minecraft:jigsaw",
        "biomes": "#minecraft:has_structure/trial_chambers",
        "dimension_padding": 10,
        "liquid_settings": "ignore_waterlogging",
        "max_distance_from_center": 116,
        "pool_aliases": [
            {
                "type": "minecraft:random_group",
                "groups": [
                    {
                        "data": [
                            {"type": "minecraft:direct", "alias": "minecraft:trial_chambers/spawner/contents/ranged", "target": "minecraft:trial_chambers/spawner/ranged/skeleton"},
                            {"type": "minecraft:direct", "alias": "minecraft:trial_chambers/spawner/contents/slow_ranged", "target": "minecraft:trial_chambers/spawner/slow_ranged/skeleton"}
                        ],
                        "weight": 1
                    },
                    {
                        "data": [
                            {"type": "minecraft:direct", "alias": "minecraft:trial_chambers/spawner/contents/ranged", "target": "minecraft:trial_chambers/spawner/ranged/stray"}
                        ],
                        "weight": 1
                    }
                ]
            },
            {
                "type": "minecraft:random",
                "alias": "minecraft:trial_chambers/spawner/contents/melee",
                "targets": [
                    {"data": "minecraft:trial_chambers/spawner/melee/zombie", "weight": 1},
                    {"data": "minecraft:trial_chambers/spawner/melee/husk", "weight": 1},
                    {"data": "minecraft:trial_chambers/spawner/melee/spider", "weight": 1}
                ]
            },
            {
                "type": "minecraft:direct",
                "alias": "minecraft:trial_chambers/spawner/contents/baby_melee",
                "target": "minecraft:trial_chambers/spawner/baby_melee/zombie"
            }
        ],
        "size": 20,
        "spawn_overrides": {
            "monster": { "bounding_box": "piece", "spawns": [] },
            "creature": { "bounding_box": "full", "spawns": [] }
        },
        "start_height": {
            "type": "minecraft:uniform",
            "max_inclusive": { "absolute": -20 },
            "min_inclusive": { "absolute": -40 }
        },
        "start_pool": "minecraft:trial_chambers/chamber/end",
        "step": "underground_structures",
        "terrain_adaptation": "encapsulate",
        "use_expansion_hack": false
    })";

    const auto* def = loadOk(json, ResourceLocation("minecraft", "trial_chambers"));
    ASSERT_NE(def, nullptr);
    EXPECT_EQ(def->type, "minecraft:jigsaw");
    EXPECT_EQ(def->size, 20);
    EXPECT_EQ(def->maxDistanceFromCenter.value().horizontal, 116);
    EXPECT_FALSE(def->useExpansionHack);
    EXPECT_EQ(def->liquidSettings, LiquidSettings::IgnoreWaterlogging);
    // 简写 dimension_padding（单一数字同时应用于 top/bottom）
    EXPECT_EQ(def->dimensionPadding.bottom, 10);
    EXPECT_EQ(def->dimensionPadding.top, 10);
    EXPECT_EQ(def->terrainAdaptation, TerrainAdaptation::Encapsulate);
    EXPECT_EQ(def->step, DecorationStage::UndergroundStructures);

    // 分派 start_height（uniform）应解析为非空
    ASSERT_NE(def->startHeight, nullptr);

    // spawn_overrides：2 个类别，piece 与 full 各一
    ASSERT_EQ(def->spawnOverrides.size(), 2u);
    EXPECT_EQ(def->spawnOverrides.at("monster").boundingBoxType, SpawnOverrideType::Piece);
    EXPECT_EQ(def->spawnOverrides.at("creature").boundingBoxType, SpawnOverrideType::Full);

    // pool_aliases：3 个绑定（random_group + random + direct）
    ASSERT_EQ(def->poolAliases.bindings().size(), 3u);

    // 第 1 个：random_group，含 2 组，组1有 2 个 direct，组2有 1 个 direct
    const auto& b0 = def->poolAliases.bindings()[0];
    const auto* groupBinding = dynamic_cast<const RandomGroupPoolAliasBinding*>(b0.get());
    ASSERT_NE(groupBinding, nullptr);
    ASSERT_EQ(groupBinding->groups().size(), 2u);
    EXPECT_EQ(groupBinding->groups()[0].bindings.size(), 2u);
    EXPECT_EQ(groupBinding->groups()[1].bindings.size(), 1u);

    // 第 2 个：random，含 3 个候选
    const auto& b1 = def->poolAliases.bindings()[1];
    const auto* randomBinding = dynamic_cast<const RandomPoolAliasBinding*>(b1.get());
    ASSERT_NE(randomBinding, nullptr);
    ASSERT_EQ(randomBinding->targets().size(), 3u);
    EXPECT_EQ(randomBinding->alias(), ResourceLocation("minecraft:trial_chambers/spawner/contents/melee"));

    // 第 3 个：direct，alias→target 一对一
    const auto& b2 = def->poolAliases.bindings()[2];
    const auto* directBinding = dynamic_cast<const DirectPoolAliasBinding*>(b2.get());
    ASSERT_NE(directBinding, nullptr);
    EXPECT_EQ(directBinding->alias(), ResourceLocation("minecraft:trial_chambers/spawner/contents/baby_melee"));
    EXPECT_EQ(directBinding->target(), ResourceLocation("minecraft:trial_chambers/spawner/baby_melee/zombie"));
}

/// 废弃矿井：程序化类型 minecraft:mineshaft，无 jigsaw 字段，start_height 缺省
TEST_F(StructureDefinitionLoaderTest, ParseProceduralMineshaftMinimal)
{
    const std::string json = R"({
        "type": "minecraft:mineshaft",
        "biomes": "#minecraft:has_structure/mineshaft",
        "mineshaft_type": "normal",
        "spawn_overrides": {},
        "step": "underground_structures"
    })";

    const auto* def = loadOk(json, ResourceLocation("minecraft", "mineshaft"));
    ASSERT_NE(def, nullptr);
    EXPECT_EQ(def->type, "minecraft:mineshaft");
    EXPECT_EQ(def->biomes, ResourceLocation("#minecraft:has_structure/mineshaft"));
    EXPECT_EQ(def->step, DecorationStage::UndergroundStructures);
    // 程序化类型无 jigsaw 字段
    EXPECT_EQ(def->startHeight, nullptr);
    EXPECT_TRUE(def->spawnOverrides.empty());
    EXPECT_TRUE(def->poolAliases.empty());
    EXPECT_EQ(def->terrainAdaptation, TerrainAdaptation::None);
}

/// 缺 type 字段应返回错误
TEST_F(StructureDefinitionLoaderTest, MissingTypeReturnsError)
{
    const std::string json = R"({ "biomes": "#minecraft:x" })";
    auto result = StructureDefinitionLoader::loadFromJson(json, ResourceLocation("minecraft", "bad"));
    EXPECT_FALSE(result.success());
}

/// start_height 分派形式（above_bottom/below_top 简写锚点 + constant type）
TEST_F(StructureDefinitionLoaderTest, ParseConstantHeightWithAnchorShorthand)
{
    const std::string json = R"({
        "type": "minecraft:jigsaw",
        "biomes": "#minecraft:x",
        "start_height": { "type": "minecraft:constant", "value": { "below_top": 5 } },
        "start_pool": "minecraft:p",
        "step": "surface_structures"
    })";

    const auto* def = loadOk(json, ResourceLocation("minecraft", "anchored"));
    ASSERT_NE(def, nullptr);
    ASSERT_NE(def->startHeight, nullptr);
}
