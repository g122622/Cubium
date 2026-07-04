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

#include "../src/common/world/biome/Biome.hpp"
#include "../src/common/world/biome/BiomeGenerationSettings.hpp"
#include "../src/common/world/block/registry/VanillaBlocks.hpp"
#include "../src/common/world/gen/feature/ConfiguredFeature.hpp"
#include "../src/common/world/gen/feature/DecorationStage.hpp"
#include "../src/common/world/gen/feature/FeatureIds.hpp"
#include "../src/common/world/gen/feature/vegetation/FlowerFeature.hpp"
#include <array>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// DecorationStage Tests
// ============================================================================

class DecorationStageTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(DecorationStageTest, GetAllReturnsCorrectOrder)
{
    const auto& stages = DecorationStages::getAll();

    EXPECT_EQ(stages.size(), static_cast<size_t>(DecorationStage::Count));
    EXPECT_EQ(stages[0], DecorationStage::RawGeneration);
    EXPECT_EQ(stages[1], DecorationStage::Lakes);
    EXPECT_EQ(stages[2], DecorationStage::LocalModifications);
    EXPECT_EQ(stages[3], DecorationStage::UndergroundStructures);
    EXPECT_EQ(stages[4], DecorationStage::SurfaceStructures);
    EXPECT_EQ(stages[5], DecorationStage::Strongholds);
    EXPECT_EQ(stages[6], DecorationStage::UndergroundOres);
    EXPECT_EQ(stages[7], DecorationStage::UndergroundDecoration);
    EXPECT_EQ(stages[8], DecorationStage::FluidSprings);
    EXPECT_EQ(stages[9], DecorationStage::VegetalDecoration);
    EXPECT_EQ(stages[10], DecorationStage::TopLayerModification);
}

TEST_F(DecorationStageTest, GetNameReturnsCorrectStrings)
{
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::RawGeneration), "raw_generation");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::Lakes), "lakes");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::LocalModifications), "local_modifications");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::UndergroundStructures), "underground_structures");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::SurfaceStructures), "surface_structures");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::Strongholds), "strongholds");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::UndergroundOres), "underground_ores");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::UndergroundDecoration), "underground_decoration");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::FluidSprings), "fluid_springs");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::VegetalDecoration), "vegetal_decoration");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::TopLayerModification), "top_layer_modification");
}

TEST_F(DecorationStageTest, GetIndexReturnsCorrectValues)
{
    EXPECT_EQ(DecorationStages::getIndex(DecorationStage::RawGeneration), 0);
    EXPECT_EQ(DecorationStages::getIndex(DecorationStage::Lakes), 1);
    EXPECT_EQ(DecorationStages::getIndex(DecorationStage::UndergroundOres), 6);
    EXPECT_EQ(DecorationStages::getIndex(DecorationStage::FluidSprings), 8);
    EXPECT_EQ(DecorationStages::getIndex(DecorationStage::VegetalDecoration), 9);
    EXPECT_EQ(DecorationStages::getIndex(DecorationStage::TopLayerModification), 10);
}

TEST_F(DecorationStageTest, FromIndexReturnsCorrectStage)
{
    EXPECT_EQ(DecorationStages::fromIndex(0), DecorationStage::RawGeneration);
    EXPECT_EQ(DecorationStages::fromIndex(6), DecorationStage::UndergroundOres);
    EXPECT_EQ(DecorationStages::fromIndex(8), DecorationStage::FluidSprings);
    EXPECT_EQ(DecorationStages::fromIndex(10), DecorationStage::TopLayerModification);
}

TEST_F(DecorationStageTest, FromIndexInvalidReturnsRawGeneration)
{
    EXPECT_EQ(DecorationStages::fromIndex(100), DecorationStage::RawGeneration);
    EXPECT_EQ(DecorationStages::fromIndex(255), DecorationStage::RawGeneration);
}

TEST_F(DecorationStageTest, FromNameReturnsCorrectStage)
{
    EXPECT_EQ(DecorationStages::fromName("raw_generation"), DecorationStage::RawGeneration);
    EXPECT_EQ(DecorationStages::fromName("lakes"), DecorationStage::Lakes);
    EXPECT_EQ(DecorationStages::fromName("underground_ores"), DecorationStage::UndergroundOres);
    EXPECT_EQ(DecorationStages::fromName("vegetal_decoration"), DecorationStage::VegetalDecoration);
    EXPECT_EQ(DecorationStages::fromName("top_layer_modification"), DecorationStage::TopLayerModification);
}

TEST_F(DecorationStageTest, FromNameInvalidReturnsRawGeneration)
{
    EXPECT_EQ(DecorationStages::fromName("invalid"), DecorationStage::RawGeneration);
    EXPECT_EQ(DecorationStages::fromName(""), DecorationStage::RawGeneration);
}

// ============================================================================
// BiomeGenerationSettings Tests
// ============================================================================

class BiomeGenerationSettingsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(BiomeGenerationSettingsTest, DefaultConstruction)
{
    BiomeGenerationSettings settings;

    // 验证所有阶段为空
    for (DecorationStage stage : DecorationStages::getAll()) {
        EXPECT_TRUE(settings.getFeatures(stage).empty());
    }

    EXPECT_FALSE(settings.hasFeatures());
}

TEST_F(BiomeGenerationSettingsTest, AddFeature)
{
    BiomeGenerationSettings settings;

    settings.addFeature(DecorationStage::UndergroundOres, 0);
    settings.addFeature(DecorationStage::UndergroundOres, 1);
    settings.addFeature(DecorationStage::VegetalDecoration, 10);

    const auto& ores = settings.getFeatures(DecorationStage::UndergroundOres);
    EXPECT_EQ(ores.size(), 2u);
    EXPECT_EQ(ores[0], 0u);
    EXPECT_EQ(ores[1], 1u);

    const auto& vegetal = settings.getFeatures(DecorationStage::VegetalDecoration);
    EXPECT_EQ(vegetal.size(), 1u);
    EXPECT_EQ(vegetal[0], 10u);

    EXPECT_TRUE(settings.hasFeatures());
}

TEST_F(BiomeGenerationSettingsTest, Clear)
{
    BiomeGenerationSettings settings;

    settings.addFeature(DecorationStage::UndergroundOres, 0);
    settings.addFeature(DecorationStage::VegetalDecoration, 10);
    EXPECT_TRUE(settings.hasFeatures());

    settings.clear();

    EXPECT_FALSE(settings.hasFeatures());
    EXPECT_TRUE(settings.getFeatures(DecorationStage::UndergroundOres).empty());
    EXPECT_TRUE(settings.getFeatures(DecorationStage::VegetalDecoration).empty());
}

TEST_F(BiomeGenerationSettingsTest, CreateDefault)
{
    BiomeGenerationSettings settings = BiomeGenerationSettings::createDefault();

    // 默认设置应该包含矿石
    EXPECT_TRUE(settings.hasFeatures());

    // 矿石应该在 UndergroundOres 阶段
    const auto& ores = settings.getFeatures(DecorationStage::UndergroundOres);
    EXPECT_GE(ores.size(), 1u);

    // 默认应挂接基础湖泊特征
    const auto& lakes = settings.getFeatures(DecorationStage::Lakes);
    ASSERT_EQ(lakes.size(), 2u);
    EXPECT_EQ(lakes[0], LakeFeatureIds::WaterLake);
    EXPECT_EQ(lakes[1], LakeFeatureIds::LavaLake);
}

TEST_F(BiomeGenerationSettingsTest, CreatePlains)
{
    BiomeGenerationSettings settings = BiomeGenerationSettings::createPlains();

    EXPECT_TRUE(settings.hasFeatures());

    // 平原应该有矿石
    const auto& ores = settings.getFeatures(DecorationStage::UndergroundOres);
    EXPECT_GE(ores.size(), 1u);
}

TEST_F(BiomeGenerationSettingsTest, CreateForest)
{
    BiomeGenerationSettings settings = BiomeGenerationSettings::createForest();

    EXPECT_TRUE(settings.hasFeatures());

    // 森林应该有矿石
    const auto& ores = settings.getFeatures(DecorationStage::UndergroundOres);
    EXPECT_GE(ores.size(), 1u);
}

TEST_F(BiomeGenerationSettingsTest, CreateMountains)
{
    BiomeGenerationSettings settings = BiomeGenerationSettings::createMountains();

    EXPECT_TRUE(settings.hasFeatures());

    // 山地应该有矿石和绿宝石
    const auto& ores = settings.getFeatures(DecorationStage::UndergroundOres);
    EXPECT_GE(ores.size(), 6u); // 基础矿石 + 绿宝石
}

TEST_F(BiomeGenerationSettingsTest, CreateOcean)
{
    BiomeGenerationSettings settings = BiomeGenerationSettings::createOcean();

    EXPECT_TRUE(settings.hasFeatures());

    // 海洋现在有海带和海草
    const auto& vegetal = settings.getFeatures(DecorationStage::VegetalDecoration);
    EXPECT_GE(vegetal.size(), 2u); // 至少有海带和海草

    // 海洋也有矿石
    const auto& ores = settings.getFeatures(DecorationStage::UndergroundOres);
    EXPECT_GE(ores.size(), 7u); // 基础矿石
}

// ============================================================================
// FeatureRegistry Tests
// ============================================================================

class FeatureRegistryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 清除注册表以确保测试隔离
        FeatureRegistry::instance().clear();
    }

    void TearDown() override { FeatureRegistry::instance().clear(); }
};

TEST_F(FeatureRegistryTest, InstanceReturnsSingleton)
{
    FeatureRegistry& reg1 = FeatureRegistry::instance();
    FeatureRegistry& reg2 = FeatureRegistry::instance();

    EXPECT_EQ(&reg1, &reg2);
}

TEST_F(FeatureRegistryTest, ClearRemovesAllFeatures)
{
    // 验证清除后为空
    FeatureRegistry::instance().clear();

    for (DecorationStage stage : DecorationStages::getAll()) {
        EXPECT_TRUE(FeatureRegistry::instance().getFeatures(stage).empty());
    }
}

TEST_F(FeatureRegistryTest, GetFeaturesReturnsEmptyForUnregisteredStage)
{
    FeatureRegistry::instance().clear();

    const auto& features = FeatureRegistry::instance().getFeatures(DecorationStage::UndergroundOres);
    EXPECT_TRUE(features.empty());
}

TEST_F(FeatureRegistryTest, GetFeatureByIdReturnsNullptrForEmptyRegistry)
{
    FeatureRegistry::instance().clear();

    EXPECT_EQ(FeatureRegistry::instance().getFeatureById(0), nullptr);
    EXPECT_EQ(FeatureRegistry::instance().getFeatureById(100), nullptr);
}

TEST_F(FeatureRegistryTest, GetFeatureByIdReturnsFeatureAfterRegistration)
{
    FeatureRegistry::instance().clear();
    FeatureRegistry::instance().initialize();

    // 初始化后应有注册的特征，ID=0 应返回非空
    ConfiguredFeatureBase* feature0 = FeatureRegistry::instance().getFeatureById(0);
    ASSERT_NE(feature0, nullptr);
    EXPECT_EQ(feature0->featureId(), 0u);

    // 从 VegetalDecoration 阶段动态查找 ConfiguredFlowerFeature
    const auto& vegFeatures = FeatureRegistry::instance().getFeatures(DecorationStage::VegetalDecoration);
    ASSERT_FALSE(vegFeatures.empty());

    ConfiguredFlowerFeature* flowerFeature = nullptr;
    for (auto* feature : vegFeatures) {
        flowerFeature = dynamic_cast<ConfiguredFlowerFeature*>(feature);
        if (flowerFeature != nullptr) {
            break;
        }
    }

    // 应至少有一个花卉特征
    ASSERT_NE(flowerFeature, nullptr);

    // 通过 getFeatureById 查找同一特征应返回相同指针
    ConfiguredFeatureBase* lookedUp = FeatureRegistry::instance().getFeatureById(flowerFeature->featureId());
    ASSERT_NE(lookedUp, nullptr);
    EXPECT_EQ(lookedUp, flowerFeature);

    FeatureRegistry::instance().clear();
}

TEST_F(FeatureRegistryTest, GetFeatureByIdOutOfRangeReturnsNullptr)
{
    FeatureRegistry::instance().clear();
    FeatureRegistry::instance().initialize();

    // 越界 ID 应返回 nullptr
    EXPECT_EQ(FeatureRegistry::instance().getFeatureById(99999), nullptr);

    FeatureRegistry::instance().clear();
}

// ============================================================================
// BiomeGenerationSettings::getFlowerFeatureIds Tests
// ============================================================================

class BiomeGenerationSettingsFlowerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 确保 FeatureRegistry 已初始化（getFlowerFeatureIds 需要 dynamic_cast）
        FeatureRegistry::instance().clear();
        FeatureRegistry::instance().initialize();
    }

    void TearDown() override { FeatureRegistry::instance().clear(); }
};

TEST_F(BiomeGenerationSettingsFlowerTest, PlainsHasFlowerFeatures)
{
    BiomeGenerationSettings settings = BiomeGenerationSettings::createPlains();
    const auto& flowerIds = settings.getFlowerFeatureIds();

    EXPECT_FALSE(flowerIds.empty());

    // 平原应有 PlainsFlowers 特征
    bool hasPlainsFlowers = false;
    for (u32 id : flowerIds) {
        if (id == FlowerFeatureIds::PlainsFlowers) {
            hasPlainsFlowers = true;
            break;
        }
    }
    EXPECT_TRUE(hasPlainsFlowers);
}

TEST_F(BiomeGenerationSettingsFlowerTest, ForestHasFlowerFeatures)
{
    BiomeGenerationSettings settings = BiomeGenerationSettings::createForest();
    const auto& flowerIds = settings.getFlowerFeatureIds();

    EXPECT_FALSE(flowerIds.empty());

    // 森林应有 ForestFlowers 特征
    bool hasForestFlowers = false;
    for (u32 id : flowerIds) {
        if (id == FlowerFeatureIds::ForestFlowers) {
            hasForestFlowers = true;
            break;
        }
    }
    EXPECT_TRUE(hasForestFlowers);
}

TEST_F(BiomeGenerationSettingsFlowerTest, SwampHasFlowerFeatures)
{
    BiomeGenerationSettings settings = BiomeGenerationSettings::createSwamp();
    const auto& flowerIds = settings.getFlowerFeatureIds();

    EXPECT_FALSE(flowerIds.empty());

    // 沼泽应有 SwampFlowers 特征
    bool hasSwampFlowers = false;
    for (u32 id : flowerIds) {
        if (id == FlowerFeatureIds::SwampFlowers) {
            hasSwampFlowers = true;
            break;
        }
    }
    EXPECT_TRUE(hasSwampFlowers);
}

TEST_F(BiomeGenerationSettingsFlowerTest, FlowerForestHasFlowerFeatures)
{
    BiomeGenerationSettings settings = BiomeGenerationSettings::createFlowerForest();
    const auto& flowerIds = settings.getFlowerFeatureIds();

    EXPECT_FALSE(flowerIds.empty());

    // 繁花森林应有 FlowerForestFlowers 特征
    bool hasFlowerForestFlowers = false;
    for (u32 id : flowerIds) {
        if (id == FlowerFeatureIds::FlowerForestFlowers) {
            hasFlowerForestFlowers = true;
            break;
        }
    }
    EXPECT_TRUE(hasFlowerForestFlowers);
}

TEST_F(BiomeGenerationSettingsFlowerTest, DesertHasNoFlowerFeatures)
{
    BiomeGenerationSettings settings = BiomeGenerationSettings::createDesert();
    const auto& flowerIds = settings.getFlowerFeatureIds();

    // 沙漠没有花卉特征
    EXPECT_TRUE(flowerIds.empty());
}

TEST_F(BiomeGenerationSettingsFlowerTest, NetherHasNoFlowerFeatures)
{
    BiomeGenerationSettings settings = BiomeGenerationSettings::createNether();
    const auto& flowerIds = settings.getFlowerFeatureIds();

    // 下界没有花卉特征
    EXPECT_TRUE(flowerIds.empty());
}

TEST_F(BiomeGenerationSettingsFlowerTest, GetFlowerFeatureIdsIsConsistent)
{
    BiomeGenerationSettings settings = BiomeGenerationSettings::createPlains();

    // 第一次调用
    const auto& flowerIds1 = settings.getFlowerFeatureIds();
    // 第二次调用应返回同一引用
    const auto& flowerIds2 = settings.getFlowerFeatureIds();

    EXPECT_EQ(&flowerIds1, &flowerIds2);
}

TEST_F(BiomeGenerationSettingsFlowerTest, ClearResetsFlowerFeatureIds)
{
    BiomeGenerationSettings settings = BiomeGenerationSettings::createPlains();

    // 验证初始状态有花卉特征
    const auto& flowerIdsBefore = settings.getFlowerFeatureIds();
    EXPECT_FALSE(flowerIdsBefore.empty());

    // clear 应重置花卉特征列表
    settings.clear();

    const auto& flowerIdsAfter = settings.getFlowerFeatureIds();
    EXPECT_TRUE(flowerIdsAfter.empty());
}

TEST_F(BiomeGenerationSettingsFlowerTest, DefaultHasNoFlowerFeatures)
{
    BiomeGenerationSettings settings = BiomeGenerationSettings::createDefault();
    const auto& flowerIds = settings.getFlowerFeatureIds();

    // 默认设置没有花卉特征
    EXPECT_TRUE(flowerIds.empty());
}

TEST_F(BiomeGenerationSettingsFlowerTest, FlowerFeatureConfigReturnsCorrectFlower)
{
    // 验证从特征注册表获取的花卉特征能正确返回花朵方块状态
    // 花卉特征ID是VegetalDecoration阶段内的索引，直接用作阶段特征向量的下标
    const auto& vegFeatures = FeatureRegistry::instance().getFeatures(DecorationStage::VegetalDecoration);

    // 使用FlowerFeatureIds作为VegetalDecoration阶段向量的索引查找PlainsFlowers
    ASSERT_GT(vegFeatures.size(), FlowerFeatureIds::PlainsFlowers);
    auto* plainsFlowers = dynamic_cast<ConfiguredFlowerFeature*>(vegFeatures[FlowerFeatureIds::PlainsFlowers]);
    ASSERT_NE(plainsFlowers, nullptr);

    // 花卉列表应包含蒲公英和虞美人
    const auto& flowers = plainsFlowers->getConfig().flowers;
    EXPECT_GE(flowers.size(), 2u);

    // 验证花朵方块状态非空
    for (const auto* flower : flowers) {
        ASSERT_NE(flower, nullptr);
        EXPECT_FALSE(flower->isAir());
    }
}
