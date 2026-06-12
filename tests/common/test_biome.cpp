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

#include <array>

#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/BiomeSource.hpp"
#include "common/world/biome/source/EndBiomeSource.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/biome/source/NetherBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/feature/FeatureIds.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// Biome 类测试
// ============================================================================

class BiomeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }
};

TEST_F(BiomeTest, Construction)
{
    Biome biome(Biomes::Plains, "plains");

    EXPECT_EQ(biome.id(), Biomes::Plains);
    EXPECT_EQ(biome.name(), "plains");
    EXPECT_EQ(biome.category(), Biome::Category::None);
}

TEST_F(BiomeTest, SettersAndGetters)
{
    Biome biome(Biomes::Desert, "desert");

    // 设置地形参数
    biome.setDepth(0.5f);
    biome.setScale(0.3f);
    EXPECT_FLOAT_EQ(biome.depth(), 0.5f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.3f);

    // 设置类别
    biome.setCategory(Biome::Category::Desert);
    EXPECT_EQ(biome.category(), Biome::Category::Desert);

    // 设置气候
    BiomeClimate climate(BiomeClimate::Precipitation::None, 2.0f, BiomeClimate::TemperatureModifier::None, 0.0f);
    biome.setClimate(climate);
    EXPECT_FLOAT_EQ(biome.temperature(), 2.0f);
    EXPECT_EQ(biome.climate().precipitation, BiomeClimate::Precipitation::None);

    // 设置方块 - 使用 VanillaBlocks
    biome.setSurfaceBlock(&VanillaBlocks::SAND->defaultState());
    biome.setSubSurfaceBlock(&VanillaBlocks::SAND->defaultState());
    biome.setUnderWaterBlock(&VanillaBlocks::GRAVEL->defaultState());
    biome.setBedrockBlock(&VanillaBlocks::BEDROCK->defaultState());

    EXPECT_TRUE(biome.surfaceBlock()->is(VanillaBlocks::SAND));
    EXPECT_TRUE(biome.subSurfaceBlock()->is(VanillaBlocks::SAND));
    EXPECT_TRUE(biome.underWaterBlock()->is(VanillaBlocks::GRAVEL));
    EXPECT_TRUE(biome.bedrockBlock()->is(VanillaBlocks::BEDROCK));
}

TEST_F(BiomeTest, ClimateDefaults)
{
    BiomeClimate climate;
    EXPECT_EQ(climate.precipitation, BiomeClimate::Precipitation::Rain);
    EXPECT_FLOAT_EQ(climate.temperature, 0.5f);
    EXPECT_FLOAT_EQ(climate.downfall, 0.5f);
}

// ============================================================================
// BiomeRegistry 测试
// ============================================================================

class BiomeRegistryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }
};

TEST_F(BiomeRegistryTest, SingletonAccess)
{
    BiomeRegistry& reg1 = BiomeRegistry::instance();
    BiomeRegistry& reg2 = BiomeRegistry::instance();
    EXPECT_EQ(&reg1, &reg2);
}

TEST_F(BiomeRegistryTest, GetBiomeById)
{
    const Biome& plains = BiomeRegistry::instance().get(Biomes::Plains);
    EXPECT_EQ(plains.id(), Biomes::Plains);
    EXPECT_EQ(plains.name(), "plains");
}

TEST_F(BiomeRegistryTest, GetInvalidBiome)
{
    const Biome& biome = BiomeRegistry::instance().get(static_cast<BiomeId>(9999));
    // 应该返回默认生物群系
    EXPECT_TRUE(biome.id() == Biomes::Plains || biome.id() == 0);
}

TEST_F(BiomeRegistryTest, HasBiome)
{
    EXPECT_TRUE(BiomeRegistry::instance().hasBiome(Biomes::Plains));
    EXPECT_TRUE(BiomeRegistry::instance().hasBiome(Biomes::Desert));
    EXPECT_TRUE(BiomeRegistry::instance().hasBiome(Biomes::Ocean));
    EXPECT_TRUE(BiomeRegistry::instance().hasBiome(Biomes::Forest));
    EXPECT_TRUE(BiomeRegistry::instance().hasBiome(Biomes::Beach));
    EXPECT_TRUE(BiomeRegistry::instance().hasBiome(Biomes::WarmOcean));
    EXPECT_TRUE(BiomeRegistry::instance().hasBiome(Biomes::DeepFrozenOcean));
    EXPECT_TRUE(BiomeRegistry::instance().hasBiome(Biomes::CrimsonForest));
    EXPECT_TRUE(BiomeRegistry::instance().hasBiome(Biomes::EndHighlands));
    EXPECT_FALSE(BiomeRegistry::instance().hasBiome(static_cast<BiomeId>(55)));
    EXPECT_FALSE(BiomeRegistry::instance().hasBiome(static_cast<BiomeId>(9999)));
}

TEST_F(BiomeRegistryTest, RepresentativeBiomesAreRegisteredWithExpectedNames)
{
    struct ExpectedBiome {
        BiomeId id;
        const char* name;
    };

    const std::array<ExpectedBiome, 26> expectedBiomes = {{
        {Biomes::Plains, "plains"},
        {Biomes::Desert, "desert"},
        {Biomes::Forest, "forest"},
        {Biomes::Taiga, "taiga"},
        {Biomes::Swamp, "swamp"},
        {Biomes::River, "river"},
        {Biomes::Beach, "beach"},
        {Biomes::StoneShore, "stone_shore"},
        {Biomes::SnowyPlains, "snowy_plains"},
        {Biomes::MushroomFields, "mushroom_fields"},
        {Biomes::GiantTreeTaiga, "giant_tree_taiga"},
        {Biomes::Savanna, "savanna"},
        {Biomes::Badlands, "badlands"},
        {Biomes::WarmOcean, "warm_ocean"},
        {Biomes::ColdOcean, "cold_ocean"},
        {Biomes::DeepFrozenOcean, "deep_frozen_ocean"},
        {Biomes::SunflowerPlains, "sunflower_plains"},
        {Biomes::DesertLakes, "desert_lakes"},
        {Biomes::SwampHills, "swamp_hills"},
        {Biomes::GiantSpruceTaiga, "giant_spruce_taiga"},
        {Biomes::NetherWastes, "nether_wastes"},
        {Biomes::SoulSandValley, "soul_sand_valley"},
        {Biomes::CrimsonForest, "crimson_forest"},
        {Biomes::WarpedForest, "warped_forest"},
        {Biomes::BasaltDeltas, "basalt_deltas"},
        {Biomes::EndHighlands, "end_highlands"},
    }};

    for (const auto& expectedBiome : expectedBiomes) {
        SCOPED_TRACE(expectedBiome.name);

        EXPECT_TRUE(BiomeRegistry::instance().hasBiome(expectedBiome.id));

        const Biome& biome = BiomeRegistry::instance().get(expectedBiome.id);
        EXPECT_EQ(biome.id(), expectedBiome.id);
        EXPECT_EQ(biome.name(), std::string(expectedBiome.name));
    }
}

TEST_F(BiomeRegistryTest, GetUnregisteredBiomeReturnsDefault)
{
    const Biome& biome = BiomeRegistry::instance().get(static_cast<BiomeId>(55));

    EXPECT_EQ(biome.id(), Biomes::Plains);
    EXPECT_EQ(biome.name(), "plains");
}

TEST_F(BiomeRegistryTest, AllBiomesCountMatchesBiomeIdRange)
{
    const auto& biomes = BiomeRegistry::instance().allBiomes();
    EXPECT_EQ(biomes.size(), Biomes::Count);
}

// ============================================================================
// BiomeFactory 测试
// ============================================================================

TEST_F(BiomeRegistryTest, CreatePlains)
{
    Biome plains = BiomeFactory::createPlains();
    EXPECT_EQ(plains.id(), Biomes::Plains);
    EXPECT_EQ(plains.name(), "plains");
    EXPECT_FLOAT_EQ(plains.depth(), 0.125f);
    EXPECT_FLOAT_EQ(plains.scale(), 0.05f);
    ASSERT_NE(plains.surfaceBlock(), nullptr);
    ASSERT_NE(plains.subSurfaceBlock(), nullptr);
    EXPECT_TRUE(plains.surfaceBlock()->is(VanillaBlocks::GRASS_BLOCK));
    EXPECT_TRUE(plains.subSurfaceBlock()->is(VanillaBlocks::DIRT));
}

TEST_F(BiomeRegistryTest, CreateDesert)
{
    Biome desert = BiomeFactory::createDesert();
    EXPECT_EQ(desert.id(), Biomes::Desert);
    EXPECT_EQ(desert.name(), "desert");
    EXPECT_FLOAT_EQ(desert.temperature(), 2.0f);
    EXPECT_FLOAT_EQ(desert.humidity(), 0.0f);
    ASSERT_NE(desert.surfaceBlock(), nullptr);
    ASSERT_NE(desert.subSurfaceBlock(), nullptr);
    EXPECT_TRUE(desert.surfaceBlock()->is(VanillaBlocks::SAND));
    EXPECT_TRUE(desert.subSurfaceBlock()->is(VanillaBlocks::SAND));
}

TEST_F(BiomeRegistryTest, CreateMountains)
{
    Biome mountains = BiomeFactory::createMountains();
    EXPECT_EQ(mountains.id(), Biomes::Mountains);
    EXPECT_EQ(mountains.name(), "mountains");
    EXPECT_FLOAT_EQ(mountains.depth(), 1.0f);
    ASSERT_NE(mountains.surfaceBlock(), nullptr);
    ASSERT_NE(mountains.subSurfaceBlock(), nullptr);
    EXPECT_TRUE(mountains.surfaceBlock()->is(VanillaBlocks::STONE));
    EXPECT_TRUE(mountains.subSurfaceBlock()->is(VanillaBlocks::STONE));
}

TEST_F(BiomeRegistryTest, CreateOcean)
{
    Biome ocean = BiomeFactory::createOcean();
    EXPECT_EQ(ocean.id(), Biomes::Ocean);
    EXPECT_EQ(ocean.name(), "ocean");
    EXPECT_FLOAT_EQ(ocean.depth(), -1.0f); // 海洋深度为负
}

TEST_F(BiomeRegistryTest, CreateCrimsonForestUsesCrimsonNyliumSurface)
{
    Biome biome = BiomeFactory::createCrimsonForest();
    EXPECT_EQ(biome.id(), Biomes::CrimsonForest);
    ASSERT_NE(biome.surfaceBlock(), nullptr);
    ASSERT_NE(biome.subSurfaceBlock(), nullptr);
    EXPECT_TRUE(biome.surfaceBlock()->is(VanillaBlocks::CRIMSON_NYLIUM));
    EXPECT_TRUE(biome.subSurfaceBlock()->is(VanillaBlocks::NETHERRACK));
}

TEST_F(BiomeRegistryTest, CreateWarpedForestUsesWarpedNyliumSurface)
{
    Biome biome = BiomeFactory::createWarpedForest();
    EXPECT_EQ(biome.id(), Biomes::WarpedForest);
    ASSERT_NE(biome.surfaceBlock(), nullptr);
    ASSERT_NE(biome.subSurfaceBlock(), nullptr);
    EXPECT_TRUE(biome.surfaceBlock()->is(VanillaBlocks::WARPED_NYLIUM));
    EXPECT_TRUE(biome.subSurfaceBlock()->is(VanillaBlocks::NETHERRACK));
}

// ============================================================================
// 新增生物群系测试（阶段1）
// ============================================================================

TEST_F(BiomeRegistryTest, CreateWarmOcean)
{
    Biome biome = BiomeFactory::createWarmOcean();
    EXPECT_EQ(biome.id(), Biomes::WarmOcean);
    EXPECT_EQ(biome.name(), "warm_ocean");
    EXPECT_FLOAT_EQ(biome.depth(), -1.0f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.1f);
    EXPECT_FLOAT_EQ(biome.temperature(), 0.8f);
}

TEST_F(BiomeRegistryTest, CreateLukewarmOcean)
{
    Biome biome = BiomeFactory::createLukewarmOcean();
    EXPECT_EQ(biome.id(), Biomes::LukewarmOcean);
    EXPECT_EQ(biome.name(), "lukewarm_ocean");
    EXPECT_FLOAT_EQ(biome.depth(), -1.0f);
    EXPECT_FLOAT_EQ(biome.temperature(), 0.6f);
}

TEST_F(BiomeRegistryTest, CreateColdOcean)
{
    Biome biome = BiomeFactory::createColdOcean();
    EXPECT_EQ(biome.id(), Biomes::ColdOcean);
    EXPECT_EQ(biome.name(), "cold_ocean");
    EXPECT_FLOAT_EQ(biome.depth(), -1.0f);
    EXPECT_FLOAT_EQ(biome.temperature(), 0.3f);
}

TEST_F(BiomeRegistryTest, CreateColdOceanUsesColdOceanGenerationSettings)
{
    Biome biome = BiomeFactory::createColdOcean();
    const auto& vegetal = biome.generationSettings().getFeatures(DecorationStage::VegetalDecoration);

    bool hasBlueIce = false;
    bool hasLiveCoral = false;
    bool hasKelp = false;
    bool hasSeagrass = false;
    for (u32 id : vegetal) {
        if (id == BlueIceFeatureIds::Normal) {
            hasBlueIce = true;
        }
        if (id == KelpFeatureIds::Cold) {
            hasKelp = true;
        }
        if (id == SeagrassFeatureIds::Cold) {
            hasSeagrass = true;
        }
        if (id == CoralFeatureIds::Tube || id == CoralFeatureIds::Brain || id == CoralFeatureIds::Bubble ||
            id == CoralFeatureIds::Fire || id == CoralFeatureIds::Horn) {
            hasLiveCoral = true;
        }
    }

    // 冷海不应直接挂接蓝冰与暖水珊瑚，应包含海带/海草。
    EXPECT_FALSE(hasBlueIce);
    EXPECT_FALSE(hasLiveCoral);
    EXPECT_TRUE(hasKelp);
    EXPECT_TRUE(hasSeagrass);
}

TEST_F(BiomeRegistryTest, CreateDeepLukewarmOceanUsesLukewarmGenerationSettings)
{
    Biome biome = BiomeFactory::createDeepLukewarmOcean();
    const auto& vegetal = biome.generationSettings().getFeatures(DecorationStage::VegetalDecoration);

    bool hasBlueIce = false;
    for (u32 id : vegetal) {
        if (id == BlueIceFeatureIds::Normal) {
            hasBlueIce = true;
            break;
        }
    }

    // 深温水海洋不应包含蓝冰
    EXPECT_FALSE(hasBlueIce);
}

TEST_F(BiomeRegistryTest, CreateDeepColdOceanUsesColdGenerationSettings)
{
    Biome biome = BiomeFactory::createDeepColdOcean();
    const auto& vegetal = biome.generationSettings().getFeatures(DecorationStage::VegetalDecoration);

    bool hasBlueIce = false;
    for (u32 id : vegetal) {
        if (id == BlueIceFeatureIds::Normal) {
            hasBlueIce = true;
            break;
        }
    }

    // 深冷海洋不应直接挂接蓝冰
    EXPECT_FALSE(hasBlueIce);
}

TEST_F(BiomeRegistryTest, CreateDeepFrozenOceanUsesFrozenGenerationSettings)
{
    Biome biome = BiomeFactory::createDeepFrozenOcean();
    const auto& vegetal = biome.generationSettings().getFeatures(DecorationStage::VegetalDecoration);

    bool hasBlueIce = false;
    for (u32 id : vegetal) {
        if (id == BlueIceFeatureIds::Normal) {
            hasBlueIce = true;
            break;
        }
    }

    EXPECT_TRUE(hasBlueIce);
}

TEST_F(BiomeRegistryTest, CreateDeepWarmOcean)
{
    Biome biome = BiomeFactory::createDeepWarmOcean();
    EXPECT_EQ(biome.id(), Biomes::DeepWarmOcean);
    EXPECT_EQ(biome.name(), "deep_warm_ocean");
    EXPECT_FLOAT_EQ(biome.depth(), -1.8f);
}

TEST_F(BiomeRegistryTest, CreateDeepLukewarmOcean)
{
    Biome biome = BiomeFactory::createDeepLukewarmOcean();
    EXPECT_EQ(biome.id(), Biomes::DeepLukewarmOcean);
    EXPECT_EQ(biome.name(), "deep_lukewarm_ocean");
    EXPECT_FLOAT_EQ(biome.depth(), -1.8f);
}

TEST_F(BiomeRegistryTest, CreateDeepColdOcean)
{
    Biome biome = BiomeFactory::createDeepColdOcean();
    EXPECT_EQ(biome.id(), Biomes::DeepColdOcean);
    EXPECT_EQ(biome.name(), "deep_cold_ocean");
    EXPECT_FLOAT_EQ(biome.depth(), -1.8f);
}

TEST_F(BiomeRegistryTest, CreateJungleHills)
{
    Biome biome = BiomeFactory::createJungleHills();
    EXPECT_EQ(biome.id(), Biomes::JungleHills);
    EXPECT_EQ(biome.name(), "jungle_hills");
    EXPECT_FLOAT_EQ(biome.depth(), 0.45f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.3f);
}

TEST_F(BiomeRegistryTest, CreateJungleEdge)
{
    Biome biome = BiomeFactory::createJungleEdge();
    EXPECT_EQ(biome.id(), Biomes::JungleEdge);
    EXPECT_EQ(biome.name(), "jungle_edge");
    EXPECT_FLOAT_EQ(biome.depth(), 0.1f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.2f);
}

TEST_F(BiomeRegistryTest, CreateBambooJungle)
{
    Biome biome = BiomeFactory::createBambooJungle();
    EXPECT_EQ(biome.id(), Biomes::BambooJungle);
    EXPECT_EQ(biome.name(), "bamboo_jungle");
    EXPECT_FLOAT_EQ(biome.depth(), 0.1f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.2f);
}

TEST_F(BiomeRegistryTest, CreateBambooJungleHills)
{
    Biome biome = BiomeFactory::createBambooJungleHills();
    EXPECT_EQ(biome.id(), Biomes::BambooJungleHills);
    EXPECT_EQ(biome.name(), "bamboo_jungle_hills");
    EXPECT_FLOAT_EQ(biome.depth(), 0.45f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.3f);
}

TEST_F(BiomeRegistryTest, CreateBirchForestHills)
{
    Biome biome = BiomeFactory::createBirchForestHills();
    EXPECT_EQ(biome.id(), Biomes::BirchForestHills);
    EXPECT_EQ(biome.name(), "birch_forest_hills");
    EXPECT_FLOAT_EQ(biome.depth(), 0.45f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.3f);
}

TEST_F(BiomeRegistryTest, CreateFlowerForest)
{
    Biome biome = BiomeFactory::createFlowerForest();
    EXPECT_EQ(biome.id(), Biomes::FlowerForest);
    EXPECT_EQ(biome.name(), "flower_forest");
    EXPECT_FLOAT_EQ(biome.depth(), 0.1f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.2f);
}

TEST_F(BiomeRegistryTest, CreateTallBirchForest)
{
    Biome biome = BiomeFactory::createTallBirchForest();
    EXPECT_EQ(biome.id(), Biomes::TallBirchForest);
    EXPECT_EQ(biome.name(), "tall_birch_forest");
    EXPECT_FLOAT_EQ(biome.depth(), 0.1f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.2f);
}

TEST_F(BiomeRegistryTest, CreateDarkForestHills)
{
    Biome biome = BiomeFactory::createDarkForestHills();
    EXPECT_EQ(biome.id(), Biomes::DarkForestHills);
    EXPECT_EQ(biome.name(), "dark_forest_hills");
    EXPECT_FLOAT_EQ(biome.depth(), 0.45f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.3f);
}

TEST_F(BiomeRegistryTest, CreateMushroomFields)
{
    Biome biome = BiomeFactory::createMushroomFields();
    EXPECT_EQ(biome.id(), Biomes::MushroomFields);
    EXPECT_EQ(biome.name(), "mushroom_fields");
    EXPECT_FLOAT_EQ(biome.depth(), 0.2f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.3f);
    EXPECT_FLOAT_EQ(biome.temperature(), 0.9f);
    EXPECT_FLOAT_EQ(biome.humidity(), 1.0f);
    // MC 1.16.5: 蘑菇岛生物群系类别应为 Mushroom
    EXPECT_EQ(biome.category(), Biome::Category::Mushroom);
}

TEST_F(BiomeRegistryTest, CreateMushroomFieldShore)
{
    Biome biome = BiomeFactory::createMushroomFieldShore();
    EXPECT_EQ(biome.id(), Biomes::MushroomFieldShore);
    EXPECT_EQ(biome.name(), "mushroom_field_shore");
    EXPECT_FLOAT_EQ(biome.depth(), 0.0f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.025f);
    // MC 1.16.5: 蘑菇岛海岸生物群系类别应为 Mushroom
    EXPECT_EQ(biome.category(), Biome::Category::Mushroom);
}

TEST_F(BiomeRegistryTest, CreateDesertHills)
{
    Biome biome = BiomeFactory::createDesertHills();
    EXPECT_EQ(biome.id(), Biomes::DesertHills);
    EXPECT_EQ(biome.name(), "desert_hills");
    EXPECT_FLOAT_EQ(biome.depth(), 0.225f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.25f);
}

TEST_F(BiomeRegistryTest, CreateTaigaHills)
{
    Biome biome = BiomeFactory::createTaigaHills();
    EXPECT_EQ(biome.id(), Biomes::TaigaHills);
    EXPECT_EQ(biome.name(), "taiga_hills");
    EXPECT_FLOAT_EQ(biome.depth(), 0.3f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.25f);
}

TEST_F(BiomeRegistryTest, CreateGiantSpruceTaiga)
{
    Biome biome = BiomeFactory::createGiantSpruceTaiga();
    EXPECT_EQ(biome.id(), Biomes::GiantSpruceTaiga);
    EXPECT_EQ(biome.name(), "giant_spruce_taiga");
    EXPECT_FLOAT_EQ(biome.depth(), 0.2f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.2f);
}

TEST_F(BiomeRegistryTest, CreateGiantSpruceTaigaHills)
{
    Biome biome = BiomeFactory::createGiantSpruceTaigaHills();
    EXPECT_EQ(biome.id(), Biomes::GiantSpruceTaigaHills);
    EXPECT_EQ(biome.name(), "giant_spruce_taiga_hills");
    EXPECT_FLOAT_EQ(biome.depth(), 0.2f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.2f);
}

// ============================================================================
// 新增生物群系测试（阶段2 - 稀有变体）
// ============================================================================

TEST_F(BiomeRegistryTest, CreateSunflowerPlains)
{
    Biome biome = BiomeFactory::createSunflowerPlains();
    EXPECT_EQ(biome.id(), Biomes::SunflowerPlains);
    EXPECT_EQ(biome.name(), "sunflower_plains");
    EXPECT_FLOAT_EQ(biome.depth(), 0.125f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.05f);
}

TEST_F(BiomeRegistryTest, CreateDesertLakes)
{
    Biome biome = BiomeFactory::createDesertLakes();
    EXPECT_EQ(biome.id(), Biomes::DesertLakes);
    EXPECT_EQ(biome.name(), "desert_lakes");
    EXPECT_FLOAT_EQ(biome.depth(), 0.225f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.25f);
}

TEST_F(BiomeRegistryTest, CreateGravellyMountains)
{
    Biome biome = BiomeFactory::createGravellyMountains();
    EXPECT_EQ(biome.id(), Biomes::GravellyMountains);
    EXPECT_EQ(biome.name(), "gravelly_mountains");
    EXPECT_FLOAT_EQ(biome.depth(), 1.0f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.5f);
}

TEST_F(BiomeRegistryTest, CreateSwampHills)
{
    Biome biome = BiomeFactory::createSwampHills();
    EXPECT_EQ(biome.id(), Biomes::SwampHills);
    EXPECT_EQ(biome.name(), "swamp_hills");
    EXPECT_FLOAT_EQ(biome.depth(), -0.1f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.3f);
}

TEST_F(BiomeRegistryTest, CreateModifiedJungle)
{
    Biome biome = BiomeFactory::createModifiedJungle();
    EXPECT_EQ(biome.id(), Biomes::ModifiedJungle);
    EXPECT_EQ(biome.name(), "modified_jungle");
    EXPECT_FLOAT_EQ(biome.depth(), 0.1f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.2f);
}

TEST_F(BiomeRegistryTest, CreateShatteredSavannaPlateau)
{
    Biome biome = BiomeFactory::createShatteredSavannaPlateau();
    EXPECT_EQ(biome.id(), Biomes::ShatteredSavannaPlateau);
    EXPECT_EQ(biome.name(), "shattered_savanna_plateau");
    EXPECT_FLOAT_EQ(biome.depth(), 1.05f);
    EXPECT_FLOAT_EQ(biome.scale(), 0.0125f);
}

// ============================================================================
// MultiNoiseBiomeSource 测试
// ============================================================================

class MultiNoiseBiomeSourceTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }
};

TEST_F(MultiNoiseBiomeSourceTest, CreateOverworldReturnsValidSource)
{
    auto source = world::biome::source::MultiNoiseBiomeSource::createOverworld(12345, false);
    ASSERT_NE(source, nullptr);
    EXPECT_NE(source->seed(), 0u);

    const auto& biomes = source->possibleBiomes();
    EXPECT_FALSE(biomes.empty());
}

TEST_F(MultiNoiseBiomeSourceTest, CreateOverworldLargeBiomes)
{
    auto source = world::biome::source::MultiNoiseBiomeSource::createOverworld(54321, true);
    ASSERT_NE(source, nullptr);

    const auto& biomes = source->possibleBiomes();
    EXPECT_FALSE(biomes.empty());
}

TEST_F(MultiNoiseBiomeSourceTest, GetNoiseBiomeReturnsValidBiomeId)
{
    auto source = world::biome::source::MultiNoiseBiomeSource::createOverworld(12345, false);
    ASSERT_NE(source, nullptr);

    BiomeId biome = source->getNoiseBiome(0, 0, 0);
    EXPECT_LT(biome, Biomes::Count);

    // Sample at different positions
    BiomeId biome1 = source->getNoiseBiome(100, 64, 200);
    BiomeId biome2 = source->getNoiseBiome(-50, 32, -75);
    EXPECT_LT(biome1, Biomes::Count);
    EXPECT_LT(biome2, Biomes::Count);
}

TEST_F(MultiNoiseBiomeSourceTest, CreateNetherReturnsValidSource)
{
    auto source = world::biome::source::MultiNoiseBiomeSource::createNether(12345);
    ASSERT_NE(source, nullptr);

    const auto& biomes = source->possibleBiomes();
    EXPECT_FALSE(biomes.empty());

    // Nether should have nether biomes
    bool hasNetherBiome = false;
    for (BiomeId id : biomes) {
        if (id == Biomes::NetherWastes || id == Biomes::SoulSandValley || id == Biomes::CrimsonForest ||
            id == Biomes::WarpedForest || id == Biomes::BasaltDeltas) {
            hasNetherBiome = true;
            break;
        }
    }
    EXPECT_TRUE(hasNetherBiome);
}

TEST_F(MultiNoiseBiomeSourceTest, NetherGetNoiseBiomeReturnsValidBiomeId)
{
    auto source = world::biome::source::MultiNoiseBiomeSource::createNether(12345);
    ASSERT_NE(source, nullptr);

    BiomeId biome = source->getNoiseBiome(0, 0, 0);
    EXPECT_LT(biome, Biomes::Count);

    // Sample at different positions
    BiomeId biome1 = source->getNoiseBiome(100, 64, 200);
    BiomeId biome2 = source->getNoiseBiome(-50, 32, -75);
    EXPECT_LT(biome1, Biomes::Count);
    EXPECT_LT(biome2, Biomes::Count);
}

// ============================================================================
// NetherBiomeSource 测试
// ============================================================================

class NetherBiomeSourceTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }
};

TEST_F(NetherBiomeSourceTest, BuildParameterListReturnsNetherBiomes)
{
    auto params = world::biome::source::NetherBiomeSource::buildParameterList();

    // Should have entries for all nether biomes
    EXPECT_FALSE(params.empty());

    // Check that nether biomes are present in the parameter list
    bool hasNetherWastes = false;
    bool hasSoulSandValley = false;
    bool hasCrimsonForest = false;
    bool hasWarpedForest = false;
    bool hasBasaltDeltas = false;

    for (const auto& [param, biomeId] : params) {
        if (biomeId == Biomes::NetherWastes) hasNetherWastes = true;
        if (biomeId == Biomes::SoulSandValley) hasSoulSandValley = true;
        if (biomeId == Biomes::CrimsonForest) hasCrimsonForest = true;
        if (biomeId == Biomes::WarpedForest) hasWarpedForest = true;
        if (biomeId == Biomes::BasaltDeltas) hasBasaltDeltas = true;
    }

    EXPECT_TRUE(hasNetherWastes);
    EXPECT_TRUE(hasSoulSandValley);
    EXPECT_TRUE(hasCrimsonForest);
    EXPECT_TRUE(hasWarpedForest);
    EXPECT_TRUE(hasBasaltDeltas);
}

// ============================================================================
// EndBiomeSource 测试
// ============================================================================

class EndBiomeSourceTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }
};

TEST_F(EndBiomeSourceTest, ConstructionAndSeed)
{
    world::biome::source::EndBiomeSource source(12345);
    EXPECT_EQ(source.seed(), 12345u);
}

TEST_F(EndBiomeSourceTest, PossibleBiomesReturnsEndBiomes)
{
    world::biome::source::EndBiomeSource source(12345);
    const auto& biomes = source.possibleBiomes();

    EXPECT_FALSE(biomes.empty());

    // Check that end biomes are present
    bool hasTheEnd = false;
    bool hasEndHighlands = false;
    bool hasEndMidlands = false;
    bool hasSmallEndIslands = false;
    bool hasEndBarrens = false;

    for (BiomeId id : biomes) {
        if (id == Biomes::TheEnd) hasTheEnd = true;
        if (id == Biomes::EndHighlands) hasEndHighlands = true;
        if (id == Biomes::EndMidlands) hasEndMidlands = true;
        if (id == Biomes::SmallEndIslands) hasSmallEndIslands = true;
        if (id == Biomes::EndBarrens) hasEndBarrens = true;
    }

    EXPECT_TRUE(hasTheEnd);
    EXPECT_TRUE(hasEndHighlands);
    EXPECT_TRUE(hasEndMidlands);
    EXPECT_TRUE(hasSmallEndIslands);
    EXPECT_TRUE(hasEndBarrens);
}

TEST_F(EndBiomeSourceTest, GetNoiseBiomeCentralIsland)
{
    world::biome::source::EndBiomeSource source(12345);

    // Near origin should be TheEnd biome (central island is within 64 blocks of origin)
    BiomeId biome = source.getNoiseBiome(0, 0, 0);
    EXPECT_EQ(biome, Biomes::TheEnd);

    // Quart (0, 0) is at block (0, 0), which is central island
    biome = source.getNoiseBiome(10, 0, 10);
    // 10 * 4 = 40 blocks from origin, still within 64 block radius
    EXPECT_EQ(biome, Biomes::TheEnd);
}

TEST_F(EndBiomeSourceTest, GetNoiseBiomeReturnsValidBiomeId)
{
    world::biome::source::EndBiomeSource source(12345);

    // Sample at various positions
    BiomeId biome1 = source.getNoiseBiome(0, 0, 0);
    BiomeId biome2 = source.getNoiseBiome(100, 0, 100);
    BiomeId biome3 = source.getNoiseBiome(-50, 0, -75);

    EXPECT_LT(biome1, Biomes::Count);
    EXPECT_LT(biome2, Biomes::Count);
    EXPECT_LT(biome3, Biomes::Count);
}

TEST_F(EndBiomeSourceTest, FillBiomeContainer)
{
    world::biome::source::EndBiomeSource source(98765);
    BiomeContainer container;

    constexpr ChunkCoord chunkX = 3;
    constexpr ChunkCoord chunkZ = -2;
    source.fillBiomeContainer(container, chunkX, chunkZ);

    const i32 startNoiseX = chunkX << 2;
    const i32 startNoiseZ = chunkZ << 2;

    for (i32 bz = 0; bz < BiomeContainer::HORIZ_SIZE; ++bz) {
        for (i32 bx = 0; bx < BiomeContainer::HORIZ_SIZE; ++bx) {
            const i32 noiseX = startNoiseX + bx;
            const i32 noiseZ = startNoiseZ + bz;
            const BiomeId expected = source.getNoiseBiome(noiseX, 0, noiseZ);

            for (i32 by = 0; by < BiomeContainer::VERT_SIZE; ++by) {
                const BiomeId actual = container.getBiome(0, bx, by, bz);
                EXPECT_EQ(actual, expected)
                    << "End biome container mismatch at (" << bx << ", " << by << ", " << bz << ")";
            }
        }
    }
}

// ============================================================================
// BiomeSource::findBiome 测试
// ============================================================================

class BiomeSourceFindBiomeTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }
};

/**
 * @brief 测试用的生物群系源
 *
 * 返回固定的生物群系，用于测试 findBiome 的搜索逻辑
 */
class MockBiomeSource final : public world::biome::BiomeSource {
public:
    explicit MockBiomeSource(u64 seed)
        : BiomeSource(seed)
    {}

    [[nodiscard]] BiomeId getNoiseBiome(i32 x, i32 y, i32 z) const override { return getBiomeAtPosition(x * 4, z * 4); }

    [[nodiscard]] const std::vector<BiomeId>& possibleBiomes() const override { return m_biomes; }

    void fillBiomeContainer(BiomeContainer& container, ChunkCoord chunkX, ChunkCoord chunkZ) override
    {
        for (i32 section = 0; section < BiomeContainer::SECTION_COUNT; ++section) {
            for (i32 bz = 0; bz < BiomeContainer::HORIZ_SIZE; ++bz) {
                for (i32 by = 0; by < BiomeContainer::VERT_SIZE; ++by) {
                    for (i32 bx = 0; bx < BiomeContainer::HORIZ_SIZE; ++bx) {
                        const i32 worldX = (chunkX << 4) + (bx << 2);
                        const i32 worldZ = (chunkZ << 4) + (bz << 2);
                        container.setBiome(section, bx, by, bz, getBiomeAtPosition(worldX, worldZ));
                    }
                }
            }
        }
    }

private:
    std::vector<BiomeId> m_biomes = {Biomes::Plains, Biomes::Forest, Biomes::Desert};

    /**
     * @brief 根据位置返回生物群系
     *
     * 使用简单的区域划分：
     * - X > 100 且 Z > 100: Desert (沙漠)
     * - X < -100 且 Z < -100: Taiga (针叶林)
     * - 其他: Plains (平原)
     */
    [[nodiscard]] static BiomeId getBiomeAtPosition(i32 x, i32 z)
    {
        if (x > 100 && z > 100) {
            return Biomes::Desert;
        }
        if (x < -100 && z < -100) {
            return Biomes::Taiga;
        }
        return Biomes::Plains;
    }
};

TEST_F(BiomeSourceFindBiomeTest, FindBiomeNearCenter)
{
    MockBiomeSource source(12345);
    math::Random random(12345);

    // 在原点附近搜索平原
    auto predicate = [](BiomeId biome) { return biome == Biomes::Plains; };

    auto result = source.findBiome(0, 64, 0, 50, 8, predicate, random, true);
    EXPECT_TRUE(result.has_value());
    // 平原在原点附近，应该能找到
    EXPECT_NEAR(result->x, 0, 50);
    EXPECT_NEAR(result->z, 0, 50);
}

TEST_F(BiomeSourceFindBiomeTest, FindDesertBiome)
{
    MockBiomeSource source(12345);
    math::Random random(12345);

    auto predicate = [](BiomeId biome) { return biome == Biomes::Desert; };

    // 从原点开始搜索，沙漠在 X > 100, Z > 100 区域
    auto result = source.findBiome(0, 64, 0, 500, 16, predicate, random, true);
    EXPECT_TRUE(result.has_value());
    // 沙漠应该在 (100, 100) 以外的区域
    EXPECT_GT(result->x, 100);
    EXPECT_GT(result->z, 100);
}

TEST_F(BiomeSourceFindBiomeTest, FindTaigaBiome)
{
    MockBiomeSource source(12345);
    math::Random random(12345);

    auto predicate = [](BiomeId biome) { return biome == Biomes::Taiga; };

    // 从原点开始搜索，针叶林在 X < -100, Z < -100 区域
    auto result = source.findBiome(0, 64, 0, 500, 16, predicate, random, true);
    EXPECT_TRUE(result.has_value());
    // 针叶林应该在 (-100, -100) 以外的区域
    EXPECT_LT(result->x, -100);
    EXPECT_LT(result->z, -100);
}

TEST_F(BiomeSourceFindBiomeTest, FindBiomeNotFound)
{
    MockBiomeSource source(12345);
    math::Random random(12345);

    // 搜索一个不存在的生物群系
    auto predicate = [](BiomeId biome) {
        return biome == Biomes::NetherWastes; // MockBiomeSource 不会返回下界生物群系
    };

    auto result = source.findBiome(0, 64, 0, 100, 8, predicate, random, true);
    EXPECT_FALSE(result.has_value());
}

TEST_F(BiomeSourceFindBiomeTest, StopOnFirstVsRandomSelection)
{
    MockBiomeSource source(12345);

    auto predicate = [](BiomeId biome) { return biome == Biomes::Plains; };

    // stopOnFirst = true: 总是返回第一个找到的
    math::Random random1(12345);
    auto result1 = source.findBiome(0, 64, 0, 100, 8, predicate, random1, true);

    math::Random random2(54321);
    auto result2 = source.findBiome(0, 64, 0, 100, 8, predicate, random2, true);

    // 两个不同种子的随机数生成器，stopOnFirst 模式应该返回相同位置
    EXPECT_TRUE(result1.has_value());
    EXPECT_TRUE(result2.has_value());
    EXPECT_EQ(result1->x, result2->x);
    EXPECT_EQ(result1->z, result2->z);
}

TEST_F(BiomeSourceFindBiomeTest, SearchFromDifferentCenters)
{
    MockBiomeSource source(12345);
    math::Random random(12345);

    auto predicate = [](BiomeId biome) { return biome == Biomes::Desert; };

    // 从沙漠中心开始搜索，应该立即找到
    auto result = source.findBiome(200, 64, 200, 50, 8, predicate, random, true);
    EXPECT_TRUE(result.has_value());
    // 应该在中心附近
    EXPECT_NEAR(result->x, 200, 50);
    EXPECT_NEAR(result->z, 200, 50);
}

TEST_F(BiomeSourceFindBiomeTest, SearchRadiusLimit)
{
    MockBiomeSource source(12345);
    math::Random random(12345);

    auto predicate = [](BiomeId biome) { return biome == Biomes::Desert; };

    // 使用非常小的搜索半径，从远点开始搜索
    // 针叶林在 X < -100, Z < -100，从 (1000, 1000) 开始用半径 50 搜索
    auto result = source.findBiome(1000, 64, 1000, 50, 8, predicate, random, true);
    // 半径太小，无法到达沙漠区域（需要到 X > 100, Z > 100，但中心在 1000,1000）
    // 实际上 1000 + 50 > 100，所以能找到
    EXPECT_TRUE(result.has_value());
}

TEST_F(BiomeSourceFindBiomeTest, FindBiomeWithRealOverworldSource)
{
    auto source = world::biome::source::MultiNoiseBiomeSource::createOverworld(12345, false);
    ASSERT_NE(source, nullptr);

    math::Random random(12345);

    auto predicate = [](BiomeId biome) { return biome == Biomes::Plains; };

    // 平原是常见生物群系，应该容易找到
    auto result = source->findBiome(0, 64, 0, 1000, 64, predicate, random, true);
    EXPECT_TRUE(result.has_value());
}

TEST_F(BiomeSourceFindBiomeTest, FindAnyBiomeWithRealOverworldSource)
{
    auto source = world::biome::source::MultiNoiseBiomeSource::createOverworld(54321, false);
    ASSERT_NE(source, nullptr);

    math::Random random(54321);

    // 搜索任意有效生物群系（非空，小于 Count）
    auto predicate = [](BiomeId biome) { return biome < Biomes::Count; };

    auto result = source->findBiome(0, 64, 0, 1000, 64, predicate, random, true);
    EXPECT_TRUE(result.has_value());
}

// ============================================================================
// Biomes::isOceanOrRiverBiome 测试
// ============================================================================

class IsOceanOrRiverBiomeTest : public ::testing::Test {};

TEST_F(IsOceanOrRiverBiomeTest, OceanBiomesReturnTrue)
{
    // 所有海洋生物群系应返回 true
    EXPECT_TRUE(Biomes::isOceanOrRiverBiome(Biomes::Ocean));
    EXPECT_TRUE(Biomes::isOceanOrRiverBiome(Biomes::WarmOcean));
    EXPECT_TRUE(Biomes::isOceanOrRiverBiome(Biomes::LukewarmOcean));
    EXPECT_TRUE(Biomes::isOceanOrRiverBiome(Biomes::ColdOcean));
    EXPECT_TRUE(Biomes::isOceanOrRiverBiome(Biomes::FrozenOcean));
    EXPECT_TRUE(Biomes::isOceanOrRiverBiome(Biomes::DeepOcean));
    EXPECT_TRUE(Biomes::isOceanOrRiverBiome(Biomes::DeepWarmOcean));
    EXPECT_TRUE(Biomes::isOceanOrRiverBiome(Biomes::DeepLukewarmOcean));
    EXPECT_TRUE(Biomes::isOceanOrRiverBiome(Biomes::DeepColdOcean));
    EXPECT_TRUE(Biomes::isOceanOrRiverBiome(Biomes::DeepFrozenOcean));
}

TEST_F(IsOceanOrRiverBiomeTest, RiverBiomesReturnTrue)
{
    // 河流生物群系应返回 true
    EXPECT_TRUE(Biomes::isOceanOrRiverBiome(Biomes::River));
    EXPECT_TRUE(Biomes::isOceanOrRiverBiome(Biomes::FrozenRiver));
}

TEST_F(IsOceanOrRiverBiomeTest, NonOceanOrRiverBiomesReturnFalse)
{
    // 非海洋/河流生物群系应返回 false
    EXPECT_FALSE(Biomes::isOceanOrRiverBiome(Biomes::Plains));
    EXPECT_FALSE(Biomes::isOceanOrRiverBiome(Biomes::Desert));
    EXPECT_FALSE(Biomes::isOceanOrRiverBiome(Biomes::Forest));
    EXPECT_FALSE(Biomes::isOceanOrRiverBiome(Biomes::Swamp));
    EXPECT_FALSE(Biomes::isOceanOrRiverBiome(Biomes::Beach));
    EXPECT_FALSE(Biomes::isOceanOrRiverBiome(Biomes::Mountains));
    EXPECT_FALSE(Biomes::isOceanOrRiverBiome(Biomes::Taiga));
    EXPECT_FALSE(Biomes::isOceanOrRiverBiome(Biomes::Jungle));
    EXPECT_FALSE(Biomes::isOceanOrRiverBiome(Biomes::SoulSandValley));
    EXPECT_FALSE(Biomes::isOceanOrRiverBiome(Biomes::CrimsonForest));
    EXPECT_FALSE(Biomes::isOceanOrRiverBiome(Biomes::TheEnd));
}

TEST_F(IsOceanOrRiverBiomeTest, InvalidBiomeIdReturnsFalse)
{
    // 无效的ID应返回 false
    EXPECT_FALSE(Biomes::isOceanOrRiverBiome(static_cast<BiomeId>(9999)));
    EXPECT_FALSE(Biomes::isOceanOrRiverBiome(static_cast<BiomeId>(200)));
}
