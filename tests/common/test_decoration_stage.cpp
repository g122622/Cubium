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
#include "../src/common/world/block/VanillaBlocks.hpp"
#include "../src/common/world/chunk/ChunkPrimer.hpp"
#include "../src/common/world/gen/chunk/DebugChunkGenerator.hpp"
#include "../src/common/world/gen/feature/ConfiguredFeature.hpp"
#include "../src/common/world/gen/feature/DecorationStage.hpp"
#include "../src/common/world/gen/feature/FeatureIds.hpp"
#include <array>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;

namespace {

struct SeedProbeCall {
    u32 featureOrdinal;
    u64 sampledValue;
};

class SeedProbeFeature final : public ConfiguredFeatureBase {
public:
    explicit SeedProbeFeature(u32 featureOrdinal)
        : m_featureOrdinal(featureOrdinal)
    {}

    bool place(WorldGenRegion& region,
        ChunkPrimer& chunk,
        IChunkGenerator& generator,
        math::Random& random,
        const BlockPos& pos) override
    {
        (void)region;
        (void)chunk;
        (void)generator;
        (void)pos;

        s_calls.push_back(SeedProbeCall{m_featureOrdinal, random.nextU64()});
        return true;
    }

    [[nodiscard]] const char* name() const override { return "seed_probe"; }
    [[nodiscard]] DecorationStage stage() const override { return DecorationStage::VegetalDecoration; }

    static void clearCalls() { s_calls.clear(); }
    [[nodiscard]] static const std::vector<SeedProbeCall>& calls() { return s_calls; }

private:
    u32 m_featureOrdinal;
    static std::vector<SeedProbeCall> s_calls;
};

std::vector<SeedProbeCall> SeedProbeFeature::s_calls;

} // namespace

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
    EXPECT_EQ(stages[8], DecorationStage::VegetalDecoration);
    EXPECT_EQ(stages[9], DecorationStage::TopLayerModification);
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
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::VegetalDecoration), "vegetal_decoration");
    EXPECT_STREQ(DecorationStages::getName(DecorationStage::TopLayerModification), "top_layer_modification");
}

TEST_F(DecorationStageTest, GetIndexReturnsCorrectValues)
{
    EXPECT_EQ(DecorationStages::getIndex(DecorationStage::RawGeneration), 0);
    EXPECT_EQ(DecorationStages::getIndex(DecorationStage::Lakes), 1);
    EXPECT_EQ(DecorationStages::getIndex(DecorationStage::UndergroundOres), 6);
    EXPECT_EQ(DecorationStages::getIndex(DecorationStage::VegetalDecoration), 8);
    EXPECT_EQ(DecorationStages::getIndex(DecorationStage::TopLayerModification), 9);
}

TEST_F(DecorationStageTest, FromIndexReturnsCorrectStage)
{
    EXPECT_EQ(DecorationStages::fromIndex(0), DecorationStage::RawGeneration);
    EXPECT_EQ(DecorationStages::fromIndex(6), DecorationStage::UndergroundOres);
    EXPECT_EQ(DecorationStages::fromIndex(9), DecorationStage::TopLayerModification);
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

TEST_F(FeatureRegistryTest, FeatureGeneratorUsesMcStyleFeatureSeed)
{
    VanillaBlocks::initialize();
    SeedProbeFeature::clearCalls();

    FeatureRegistry::instance().registerFeature(
        std::make_unique<SeedProbeFeature>(0), DecorationStage::VegetalDecoration);
    FeatureRegistry::instance().registerFeature(
        std::make_unique<SeedProbeFeature>(1), DecorationStage::VegetalDecoration);

    Biome biome(Biomes::Plains, "seed_probe_biome");
    BiomeGenerationSettings settings;
    settings.addFeature(DecorationStage::VegetalDecoration, 0);
    settings.addFeature(DecorationStage::VegetalDecoration, 1);
    biome.setGenerationSettings(settings);

    ChunkPrimer chunk(7, -3);
    std::vector<IChunk*> chunks(9, &chunk);
    WorldGenRegion region(chunk.x(), chunk.z(), 1, std::move(chunks));
    DebugChunkGenerator generator;

    constexpr u64 worldSeed = 0xABCDEF1234567890ULL;
    FeatureGenerator::placeFeatures(region, chunk, generator, biome, DecorationStage::VegetalDecoration, worldSeed);

    const auto& calls = SeedProbeFeature::calls();
    ASSERT_EQ(calls.size(), 2u);
    EXPECT_EQ(calls[0].featureOrdinal, 0u);
    EXPECT_EQ(calls[1].featureOrdinal, 1u);

    const i32 startX = chunk.x() * 16;
    const i32 startZ = chunk.z() * 16;
    math::Random decorRng(worldSeed);
    const u64 i = decorRng.nextLong() | 1ULL;
    const u64 j = decorRng.nextLong() | 1ULL;
    const u64 decorSeed = (static_cast<u64>(startX) * i + static_cast<u64>(startZ) * j) ^ worldSeed;

    const i32 stageOrdinal = static_cast<i32>(DecorationStage::VegetalDecoration);
    for (u32 featureIndex = 0; featureIndex < 2u; ++featureIndex) {
        const u64 featureSeed = decorSeed + static_cast<u64>(featureIndex) + static_cast<u64>(10000 * stageOrdinal);
        math::Random expectedRng(featureSeed);
        const u64 expectedSample = expectedRng.nextU64();
        EXPECT_EQ(calls[featureIndex].sampledValue, expectedSample);
    }
}
