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

#include "common/world/gen/settings/Settings.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/source/OverworldBiomeBuilder.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/gen/settings/FlatLayerInfo.hpp"
#include "common/world/gen/settings/FlatLevelGeneratorSettings.hpp"
#include "common/world/gen/settings/NoiseSettings.hpp"
#include "common/world/gen/settings/ScalingSettings.hpp"
#include "common/world/gen/settings/SlideSettings.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace {

class SettingsTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }
};

// ============================================================================
// NoiseSettings 预设值全字段验证
// ============================================================================

TEST_F(SettingsTest, NoiseSettings_Overworld_AllFields)
{
    auto s = NoiseSettings::overworld();
    EXPECT_EQ(s.minY, -64);
    EXPECT_EQ(s.height, 384);
    EXPECT_EQ(s.sizeHorizontal, 1);
    EXPECT_EQ(s.sizeVertical, 2);
    EXPECT_FLOAT_EQ(s.densityFactor, 1.0f);
    EXPECT_FLOAT_EQ(s.densityOffset, -0.46875f);
    EXPECT_EQ(s.topSlide.target, -10);
    EXPECT_EQ(s.topSlide.size, 3);
    EXPECT_EQ(s.topSlide.offset, 0);
    EXPECT_EQ(s.bottomSlide.target, -30);
    EXPECT_EQ(s.bottomSlide.size, 0);
    EXPECT_EQ(s.bottomSlide.offset, 0);
    EXPECT_TRUE(s.simplexSurfaceNoise);
    EXPECT_TRUE(s.randomDensityOffset);
    EXPECT_FALSE(s.isAmplified);
    EXPECT_TRUE(s.aquifersEnabled);
    EXPECT_FALSE(s.useLegacyRandomSource);
}

TEST_F(SettingsTest, NoiseSettings_Nether_AllFields)
{
    auto s = NoiseSettings::nether();
    EXPECT_EQ(s.minY, 0);
    EXPECT_EQ(s.height, 128);
    EXPECT_EQ(s.sizeHorizontal, 1);
    EXPECT_EQ(s.sizeVertical, 2);
    EXPECT_FLOAT_EQ(s.densityFactor, 0.0f);
    EXPECT_FLOAT_EQ(s.densityOffset, 0.019921875f);
    EXPECT_EQ(s.topSlide.target, 120);
    EXPECT_EQ(s.topSlide.size, 3);
    EXPECT_EQ(s.topSlide.offset, 0);
    EXPECT_EQ(s.bottomSlide.target, 320);
    EXPECT_EQ(s.bottomSlide.size, 4);
    EXPECT_EQ(s.bottomSlide.offset, -1);
    EXPECT_FALSE(s.simplexSurfaceNoise);
    EXPECT_FALSE(s.randomDensityOffset);
    EXPECT_FALSE(s.isAmplified);
    EXPECT_FALSE(s.aquifersEnabled);
    EXPECT_TRUE(s.useLegacyRandomSource);
}

TEST_F(SettingsTest, NoiseSettings_End_AllFields)
{
    auto s = NoiseSettings::end();
    EXPECT_EQ(s.minY, 0);
    EXPECT_EQ(s.height, 128);
    EXPECT_EQ(s.sizeHorizontal, 2);
    EXPECT_EQ(s.sizeVertical, 1);
    EXPECT_FLOAT_EQ(s.densityFactor, 0.0f);
    EXPECT_FLOAT_EQ(s.densityOffset, 0.0f);
    EXPECT_FALSE(s.simplexSurfaceNoise);
    EXPECT_FALSE(s.randomDensityOffset);
    EXPECT_FALSE(s.isAmplified);
    EXPECT_FALSE(s.aquifersEnabled);
    EXPECT_TRUE(s.useLegacyRandomSource);
}

TEST_F(SettingsTest, NoiseSettings_Amplified_InheritsOverworld)
{
    auto s = NoiseSettings::amplified();
    EXPECT_TRUE(s.isAmplified);
    EXPECT_FLOAT_EQ(s.densityFactor, 2.0f);
    // amplified 应继承 overworld 的基本参数
    EXPECT_EQ(s.minY, -64);
    EXPECT_EQ(s.height, 384);
    EXPECT_EQ(s.sizeHorizontal, 1);
    EXPECT_EQ(s.sizeVertical, 2);
    EXPECT_FLOAT_EQ(s.densityOffset, -0.46875f);
    EXPECT_TRUE(s.simplexSurfaceNoise);
    EXPECT_TRUE(s.randomDensityOffset);
    EXPECT_TRUE(s.aquifersEnabled);
    EXPECT_FALSE(s.useLegacyRandomSource);
}

TEST_F(SettingsTest, NoiseSettings_Caves_AllFields)
{
    auto s = NoiseSettings::caves();
    EXPECT_EQ(s.minY, -64);
    EXPECT_EQ(s.height, 192);
    EXPECT_EQ(s.sizeHorizontal, 1);
    EXPECT_EQ(s.sizeVertical, 2);
    EXPECT_FALSE(s.aquifersEnabled);
    EXPECT_TRUE(s.useLegacyRandomSource);
}

TEST_F(SettingsTest, NoiseSettings_FloatingIslands_AllFields)
{
    auto s = NoiseSettings::floatingIslands();
    EXPECT_EQ(s.minY, 0);
    EXPECT_EQ(s.height, 256);
    EXPECT_EQ(s.sizeHorizontal, 2);
    EXPECT_EQ(s.sizeVertical, 1);
    EXPECT_FALSE(s.aquifersEnabled);
    EXPECT_TRUE(s.useLegacyRandomSource);
}

// ============================================================================
// NoiseSettings 计算方法测试
// ============================================================================

TEST_F(SettingsTest, NoiseSettings_Overworld_NoiseSizeCalculations)
{
    auto s = NoiseSettings::overworld();
    // sizeHorizontal=1, sizeVertical=2, CHUNK_WIDTH=16
    // noiseSizeX = 16/(1*4) = 4
    // noiseSizeY = 384/(2*4) = 48
    // noiseSizeZ = 16/(1*4) = 4
    EXPECT_EQ(s.noiseSizeX(), 4);
    EXPECT_EQ(s.noiseSizeY(), 48);
    EXPECT_EQ(s.noiseSizeZ(), 4);
}

TEST_F(SettingsTest, NoiseSettings_Nether_NoiseSizeCalculations)
{
    auto s = NoiseSettings::nether();
    EXPECT_EQ(s.noiseSizeX(), 4);  // 16/(1*4) = 4
    EXPECT_EQ(s.noiseSizeY(), 16); // 128/(2*4) = 16
    EXPECT_EQ(s.noiseSizeZ(), 4);
}

TEST_F(SettingsTest, NoiseSettings_End_NoiseSizeCalculations)
{
    auto s = NoiseSettings::end();
    EXPECT_EQ(s.noiseSizeX(), 2);  // 16/(2*4) = 2
    EXPECT_EQ(s.noiseSizeY(), 32); // 128/(1*4) = 32
    EXPECT_EQ(s.noiseSizeZ(), 2);
}

TEST_F(SettingsTest, NoiseSettings_Caves_NoiseSizeCalculations)
{
    auto s = NoiseSettings::caves();
    EXPECT_EQ(s.noiseSizeX(), 4);  // 16/(1*4) = 4
    EXPECT_EQ(s.noiseSizeY(), 24); // 192/(2*4) = 24
    EXPECT_EQ(s.noiseSizeZ(), 4);
}

TEST_F(SettingsTest, NoiseSettings_FloatingIslands_NoiseSizeCalculations)
{
    auto s = NoiseSettings::floatingIslands();
    EXPECT_EQ(s.noiseSizeX(), 2);  // 16/(2*4) = 2
    EXPECT_EQ(s.noiseSizeY(), 64); // 256/(1*4) = 64
    EXPECT_EQ(s.noiseSizeZ(), 2);
}

TEST_F(SettingsTest, NoiseSettings_Overworld_NoiseGranularity)
{
    auto s = NoiseSettings::overworld();
    EXPECT_EQ(s.verticalNoiseGranularity(), 8);   // sizeVertical*4 = 2*4 = 8
    EXPECT_EQ(s.horizontalNoiseGranularity(), 4); // sizeHorizontal*4 = 1*4 = 4
}

TEST_F(SettingsTest, NoiseSettings_End_NoiseGranularity)
{
    auto s = NoiseSettings::end();
    EXPECT_EQ(s.verticalNoiseGranularity(), 4);   // sizeVertical*4 = 1*4 = 4
    EXPECT_EQ(s.horizontalNoiseGranularity(), 8); // sizeHorizontal*4 = 2*4 = 8
}

TEST_F(SettingsTest, NoiseSettings_DefaultConstructed_MinYIsZero)
{
    NoiseSettings s;
    EXPECT_EQ(s.minY, 0);
    // 默认 height 是 MAX_BUILD_HEIGHT
    EXPECT_EQ(s.height, world::MAX_BUILD_HEIGHT);
}

// ============================================================================
// NoiseSettings guardY 验证
// ============================================================================

TEST_F(SettingsTest, NoiseSettings_GuardY_ValidSettings)
{
    // 正常参数不应抛出
    EXPECT_NO_THROW(NoiseSettings::create(-64, 384, 1, 2));
    EXPECT_NO_THROW(NoiseSettings::create(0, 128, 1, 2));
    EXPECT_NO_THROW(NoiseSettings::create(0, 256, 2, 1));
}

TEST_F(SettingsTest, NoiseSettings_GuardY_HeightNotMultipleOf16)
{
    // height 不是16的倍数应抛出
    EXPECT_THROW(NoiseSettings::create(0, 100, 1, 2), std::invalid_argument);
}

TEST_F(SettingsTest, NoiseSettings_GuardY_MinYNotMultipleOf16)
{
    // minY 不是16的倍数应抛出
    EXPECT_THROW(NoiseSettings::create(-10, 384, 1, 2), std::invalid_argument);
}

TEST_F(SettingsTest, NoiseSettings_GuardY_SizeHorizontalOutOfRange)
{
    // sizeHorizontal 超出 [1,4] 范围应抛出
    EXPECT_THROW(NoiseSettings::create(0, 128, 0, 2), std::invalid_argument);
    EXPECT_THROW(NoiseSettings::create(0, 128, 5, 2), std::invalid_argument);
}

TEST_F(SettingsTest, NoiseSettings_GuardY_SizeVerticalOutOfRange)
{
    // sizeVertical 超出 [1,4] 范围应抛出
    EXPECT_THROW(NoiseSettings::create(0, 128, 1, 0), std::invalid_argument);
    EXPECT_THROW(NoiseSettings::create(0, 128, 1, 5), std::invalid_argument);
}

// ============================================================================
// DimensionSettings 预设值全字段验证
// ============================================================================

TEST_F(SettingsTest, DimensionSettings_Overworld_AllFields)
{
    auto s = DimensionSettings::overworld();
    EXPECT_EQ(s.dimensionKind, DimensionKind::Overworld);
    ASSERT_NE(s.defaultBlock, nullptr);
    EXPECT_TRUE(s.defaultBlock->is(VanillaBlocks::STONE));
    ASSERT_NE(s.defaultFluid, nullptr);
    EXPECT_TRUE(s.defaultFluid->is(VanillaBlocks::WATER));
    EXPECT_EQ(s.seaLevel, world::SEA_LEVEL);
    EXPECT_FALSE(s.largeBiomes);
    EXPECT_TRUE(s.oreVeinsEnabled);
    EXPECT_FALSE(s.disableMobGeneration);
    // NoiseSettings 子字段
    EXPECT_EQ(s.noise.minY, -64);
    EXPECT_EQ(s.noise.height, 384);
    EXPECT_TRUE(s.noise.aquifersEnabled);
    EXPECT_FALSE(s.noise.useLegacyRandomSource);
}

TEST_F(SettingsTest, DimensionSettings_Nether_AllFields)
{
    auto s = DimensionSettings::nether();
    EXPECT_EQ(s.dimensionKind, DimensionKind::Nether);
    ASSERT_NE(s.defaultBlock, nullptr);
    EXPECT_TRUE(s.defaultBlock->is(VanillaBlocks::NETHERRACK));
    ASSERT_NE(s.defaultFluid, nullptr);
    EXPECT_TRUE(s.defaultFluid->is(VanillaBlocks::LAVA));
    EXPECT_EQ(s.seaLevel, 32);
    EXPECT_FALSE(s.largeBiomes);
    EXPECT_FALSE(s.oreVeinsEnabled);
    EXPECT_FALSE(s.disableMobGeneration);
    EXPECT_EQ(s.noise.minY, 0);
    EXPECT_EQ(s.noise.height, 128);
    EXPECT_FALSE(s.noise.aquifersEnabled);
    EXPECT_TRUE(s.noise.useLegacyRandomSource);
}

TEST_F(SettingsTest, DimensionSettings_End_AllFields)
{
    auto s = DimensionSettings::end();
    EXPECT_EQ(s.dimensionKind, DimensionKind::End);
    ASSERT_NE(s.defaultBlock, nullptr);
    EXPECT_TRUE(s.defaultBlock->is(VanillaBlocks::END_STONE));
    ASSERT_NE(s.defaultFluid, nullptr);
    EXPECT_TRUE(s.defaultFluid->is(VanillaBlocks::AIR));
    EXPECT_EQ(s.seaLevel, 0);
    EXPECT_FALSE(s.largeBiomes);
    EXPECT_FALSE(s.oreVeinsEnabled);
    EXPECT_TRUE(s.disableMobGeneration);
    EXPECT_EQ(s.noise.minY, 0);
    EXPECT_EQ(s.noise.height, 128);
    EXPECT_FALSE(s.noise.aquifersEnabled);
    EXPECT_TRUE(s.noise.useLegacyRandomSource);
}

TEST_F(SettingsTest, DimensionSettings_Flat_AllFields)
{
    auto s = DimensionSettings::flat();
    EXPECT_EQ(s.dimensionKind, DimensionKind::Flat);
    ASSERT_NE(s.defaultBlock, nullptr);
    EXPECT_TRUE(s.defaultBlock->is(VanillaBlocks::STONE));
    ASSERT_NE(s.defaultFluid, nullptr);
    EXPECT_TRUE(s.defaultFluid->is(VanillaBlocks::AIR));
    EXPECT_EQ(s.seaLevel, 0);
    EXPECT_FLOAT_EQ(s.noise.densityFactor, 0.0f);
    EXPECT_FLOAT_EQ(s.noise.densityOffset, 0.0f);
}

TEST_F(SettingsTest, DimensionSettings_Caves_AllFields)
{
    auto s = DimensionSettings::caves();
    EXPECT_EQ(s.dimensionKind, DimensionKind::Caves);
    ASSERT_NE(s.defaultBlock, nullptr);
    EXPECT_TRUE(s.defaultBlock->is(VanillaBlocks::STONE));
    ASSERT_NE(s.defaultFluid, nullptr);
    EXPECT_TRUE(s.defaultFluid->is(VanillaBlocks::WATER));
    EXPECT_EQ(s.seaLevel, 32);
    EXPECT_FALSE(s.oreVeinsEnabled);
    EXPECT_FALSE(s.disableMobGeneration);
    EXPECT_FALSE(s.noise.aquifersEnabled);
    EXPECT_TRUE(s.noise.useLegacyRandomSource);
}

TEST_F(SettingsTest, DimensionSettings_FloatingIslands_AllFields)
{
    auto s = DimensionSettings::floatingIslands();
    EXPECT_EQ(s.dimensionKind, DimensionKind::FloatingIslands);
    ASSERT_NE(s.defaultBlock, nullptr);
    EXPECT_TRUE(s.defaultBlock->is(VanillaBlocks::STONE));
    ASSERT_NE(s.defaultFluid, nullptr);
    EXPECT_TRUE(s.defaultFluid->is(VanillaBlocks::WATER));
    EXPECT_EQ(s.seaLevel, -64);
    EXPECT_FALSE(s.oreVeinsEnabled);
    EXPECT_FALSE(s.disableMobGeneration);
    EXPECT_FALSE(s.noise.aquifersEnabled);
    EXPECT_TRUE(s.noise.useLegacyRandomSource);
}

TEST_F(SettingsTest, DimensionSettings_LargeBiomes_AllFields)
{
    auto s = DimensionSettings::largeBiomesPreset();
    EXPECT_EQ(s.dimensionKind, DimensionKind::LargeBiomes);
    ASSERT_NE(s.defaultBlock, nullptr);
    EXPECT_TRUE(s.defaultBlock->is(VanillaBlocks::STONE));
    ASSERT_NE(s.defaultFluid, nullptr);
    EXPECT_TRUE(s.defaultFluid->is(VanillaBlocks::WATER));
    EXPECT_EQ(s.seaLevel, world::SEA_LEVEL);
    EXPECT_TRUE(s.largeBiomes);
    EXPECT_TRUE(s.oreVeinsEnabled);
    EXPECT_FALSE(s.disableMobGeneration);
    EXPECT_TRUE(s.noise.aquifersEnabled);
    EXPECT_FALSE(s.noise.useLegacyRandomSource);
}

TEST_F(SettingsTest, DimensionSettings_Amplified_AllFields)
{
    auto s = DimensionSettings::amplified();
    EXPECT_EQ(s.dimensionKind, DimensionKind::Amplified);
    ASSERT_NE(s.defaultBlock, nullptr);
    EXPECT_TRUE(s.defaultBlock->is(VanillaBlocks::STONE));
    ASSERT_NE(s.defaultFluid, nullptr);
    EXPECT_TRUE(s.defaultFluid->is(VanillaBlocks::WATER));
    EXPECT_EQ(s.seaLevel, world::SEA_LEVEL);
    EXPECT_FALSE(s.largeBiomes);
    EXPECT_TRUE(s.oreVeinsEnabled);
    EXPECT_FALSE(s.disableMobGeneration);
    EXPECT_TRUE(s.noise.aquifersEnabled);
    EXPECT_FALSE(s.noise.useLegacyRandomSource);
    // amplified 的 NoiseSettings 与 overworld 相同（amplified 效果由数据驱动 noise_router 的 amplified 模板实现）
    EXPECT_EQ(s.noise.minY, -64);
    EXPECT_EQ(s.noise.height, 384);
    EXPECT_EQ(s.noise.sizeHorizontal, 1);
    EXPECT_EQ(s.noise.sizeVertical, 2);
}

// ============================================================================
// DimensionKind 枚举完整性
// ============================================================================

TEST_F(SettingsTest, DimensionKind_AllValues)
{
    // 验证所有枚举值都有对应的预设
    EXPECT_EQ(DimensionSettings::overworld().dimensionKind, DimensionKind::Overworld);
    EXPECT_EQ(DimensionSettings::nether().dimensionKind, DimensionKind::Nether);
    EXPECT_EQ(DimensionSettings::end().dimensionKind, DimensionKind::End);
    EXPECT_EQ(DimensionSettings::flat().dimensionKind, DimensionKind::Flat);
    EXPECT_EQ(DimensionSettings::caves().dimensionKind, DimensionKind::Caves);
    EXPECT_EQ(DimensionSettings::floatingIslands().dimensionKind, DimensionKind::FloatingIslands);
    EXPECT_EQ(DimensionSettings::largeBiomesPreset().dimensionKind, DimensionKind::LargeBiomes);
    EXPECT_EQ(DimensionSettings::amplified().dimensionKind, DimensionKind::Amplified);
}

// ============================================================================
// ScalingSettings 默认值
// ============================================================================

TEST_F(SettingsTest, ScalingSettings_DefaultValues)
{
    ScalingSettings s;
    EXPECT_FLOAT_EQ(s.xzScale, 0.9999999814507745f);
    EXPECT_FLOAT_EQ(s.yScale, 0.9999999814507745f);
    EXPECT_FLOAT_EQ(s.xzFactor, 80.0f);
    EXPECT_FLOAT_EQ(s.yFactor, 160.0f);
}

// ============================================================================
// SlideSettings 构造和字段
// ============================================================================

TEST_F(SettingsTest, SlideSettings_DefaultConstructor)
{
    SlideSettings s;
    EXPECT_EQ(s.target, 0);
    EXPECT_EQ(s.size, 0);
    EXPECT_EQ(s.offset, 0);
}

TEST_F(SettingsTest, SlideSettings_ParameterizedConstructor)
{
    SlideSettings s{-10, 3, 0};
    EXPECT_EQ(s.target, -10);
    EXPECT_EQ(s.size, 3);
    EXPECT_EQ(s.offset, 0);
}

// ============================================================================
// FlatLayerInfo 测试
// ============================================================================

TEST_F(SettingsTest, FlatLayerInfo_DefaultConstructor)
{
    FlatLayerInfo info;
    EXPECT_EQ(info.height(), 0);
    EXPECT_EQ(info.blockState(), nullptr);
}

TEST_F(SettingsTest, FlatLayerInfo_ParameterizedConstructor)
{
    const BlockState* stone = VanillaBlocks::getState(VanillaBlocks::STONE);
    FlatLayerInfo info(3, stone);
    EXPECT_EQ(info.height(), 3);
    EXPECT_EQ(info.blockState(), stone);
}

TEST_F(SettingsTest, FlatLayerInfo_Setters)
{
    FlatLayerInfo info;
    const BlockState* dirt = VanillaBlocks::getState(VanillaBlocks::DIRT);
    info.setHeight(5);
    info.setBlockState(dirt);
    EXPECT_EQ(info.height(), 5);
    EXPECT_EQ(info.blockState(), dirt);
}

TEST_F(SettingsTest, FlatLayerInfo_HeightLimited_LessThanMax)
{
    const BlockState* stone = VanillaBlocks::getState(VanillaBlocks::STONE);
    FlatLayerInfo info(3, stone);
    auto limited = info.heightLimited(10);
    EXPECT_EQ(limited.height(), 3);
    EXPECT_EQ(limited.blockState(), stone);
}

TEST_F(SettingsTest, FlatLayerInfo_HeightLimited_ExceedsMax)
{
    const BlockState* stone = VanillaBlocks::getState(VanillaBlocks::STONE);
    FlatLayerInfo info(10, stone);
    auto limited = info.heightLimited(5);
    EXPECT_EQ(limited.height(), 5);
    EXPECT_EQ(limited.blockState(), stone);
}

TEST_F(SettingsTest, FlatLayerInfo_HeightLimited_EqualToMax)
{
    const BlockState* stone = VanillaBlocks::getState(VanillaBlocks::STONE);
    FlatLayerInfo info(5, stone);
    auto limited = info.heightLimited(5);
    EXPECT_EQ(limited.height(), 5);
    EXPECT_EQ(limited.blockState(), stone);
}

TEST_F(SettingsTest, FlatLayerInfo_HeightLimited_ZeroMax)
{
    const BlockState* stone = VanillaBlocks::getState(VanillaBlocks::STONE);
    FlatLayerInfo info(5, stone);
    auto limited = info.heightLimited(0);
    EXPECT_EQ(limited.height(), 0);
}

TEST_F(SettingsTest, FlatLayerInfo_HeightLimited_ZeroHeight)
{
    const BlockState* stone = VanillaBlocks::getState(VanillaBlocks::STONE);
    FlatLayerInfo info(0, stone);
    auto limited = info.heightLimited(10);
    EXPECT_EQ(limited.height(), 0);
}

// ============================================================================
// FlatLevelGeneratorSettings 测试
// ============================================================================

TEST_F(SettingsTest, FlatLevelGeneratorSettings_CreateDefault_AllFields)
{
    auto s = FlatLevelGeneratorSettings::createDefault();
    EXPECT_EQ(s.biomeId(), Biomes::Plains);
    EXPECT_EQ(s.layersInfo().size(), 3u);
    EXPECT_EQ(s.layers().size(), 4u);
    EXPECT_FALSE(s.hasDecoration());
    EXPECT_FALSE(s.hasLakes());
    EXPECT_FALSE(s.isVoidGen());

    // 验证层内容
    EXPECT_EQ(s.layersInfo()[0].height(), 1);
    EXPECT_TRUE(s.layersInfo()[0].blockState()->is(VanillaBlocks::BEDROCK));
    EXPECT_EQ(s.layersInfo()[1].height(), 2);
    EXPECT_TRUE(s.layersInfo()[1].blockState()->is(VanillaBlocks::DIRT));
    EXPECT_EQ(s.layersInfo()[2].height(), 1);
    EXPECT_TRUE(s.layersInfo()[2].blockState()->is(VanillaBlocks::GRASS_BLOCK));
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_CreateDefault_LayersExpandedCorrectly)
{
    auto s = FlatLevelGeneratorSettings::createDefault();
    const auto& layers = s.layers();
    ASSERT_EQ(layers.size(), 4u);
    EXPECT_TRUE(layers[0]->is(VanillaBlocks::BEDROCK));     // Y=0
    EXPECT_TRUE(layers[1]->is(VanillaBlocks::DIRT));        // Y=1
    EXPECT_TRUE(layers[2]->is(VanillaBlocks::DIRT));        // Y=2
    EXPECT_TRUE(layers[3]->is(VanillaBlocks::GRASS_BLOCK)); // Y=3
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_UpdateLayers_EmptyLayers)
{
    FlatLevelGeneratorSettings s(Biomes::Plains);
    s.updateLayers();
    EXPECT_EQ(s.layers().size(), 0u);
    EXPECT_TRUE(s.isVoidGen());
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_UpdateLayers_SingleLayer)
{
    FlatLevelGeneratorSettings s(Biomes::Plains);
    s.layersInfo().emplace_back(5, VanillaBlocks::getState(VanillaBlocks::STONE));
    s.updateLayers();
    ASSERT_EQ(s.layers().size(), 5u);
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(s.layers()[i]->is(VanillaBlocks::STONE));
    }
    EXPECT_FALSE(s.isVoidGen());
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_UpdateLayers_MultipleLayers)
{
    FlatLevelGeneratorSettings s(Biomes::Plains);
    s.layersInfo().emplace_back(2, VanillaBlocks::getState(VanillaBlocks::BEDROCK));
    s.layersInfo().emplace_back(3, VanillaBlocks::getState(VanillaBlocks::STONE));
    s.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK));
    s.updateLayers();
    ASSERT_EQ(s.layers().size(), 6u);
    EXPECT_TRUE(s.layers()[0]->is(VanillaBlocks::BEDROCK));
    EXPECT_TRUE(s.layers()[1]->is(VanillaBlocks::BEDROCK));
    EXPECT_TRUE(s.layers()[2]->is(VanillaBlocks::STONE));
    EXPECT_TRUE(s.layers()[3]->is(VanillaBlocks::STONE));
    EXPECT_TRUE(s.layers()[4]->is(VanillaBlocks::STONE));
    EXPECT_TRUE(s.layers()[5]->is(VanillaBlocks::GRASS_BLOCK));
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_UpdateLayers_ZeroHeightLayer)
{
    FlatLevelGeneratorSettings s(Biomes::Plains);
    s.layersInfo().emplace_back(0, VanillaBlocks::getState(VanillaBlocks::STONE));
    s.updateLayers();
    EXPECT_EQ(s.layers().size(), 0u);
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_UpdateLayers_AllAirLayers_IsVoidGen)
{
    FlatLevelGeneratorSettings s(Biomes::Plains);
    const BlockState* air = BlockRegistry::instance().airState();
    s.layersInfo().emplace_back(5, air);
    s.updateLayers();
    EXPECT_TRUE(s.isVoidGen());
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_SetBiomeId)
{
    FlatLevelGeneratorSettings s(Biomes::Plains);
    s.setBiomeId(Biomes::Desert);
    EXPECT_EQ(s.biomeId(), Biomes::Desert);
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_SetDecoration)
{
    FlatLevelGeneratorSettings s(Biomes::Plains);
    EXPECT_FALSE(s.hasDecoration());
    s.setDecoration(true);
    EXPECT_TRUE(s.hasDecoration());
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_SetLakes)
{
    FlatLevelGeneratorSettings s(Biomes::Plains);
    EXPECT_FALSE(s.hasLakes());
    s.setLakes(true);
    EXPECT_TRUE(s.hasLakes());
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_DefaultConstructor)
{
    FlatLevelGeneratorSettings s;
    EXPECT_EQ(s.biomeId(), Biomes::Plains);
    EXPECT_FALSE(s.hasDecoration());
    EXPECT_FALSE(s.hasLakes());
    EXPECT_TRUE(s.isVoidGen());
    EXPECT_EQ(s.layersInfo().size(), 0u);
    EXPECT_EQ(s.layers().size(), 0u);
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_ParameterizedConstructor)
{
    FlatLevelGeneratorSettings s(Biomes::Desert, true, true);
    EXPECT_EQ(s.biomeId(), Biomes::Desert);
    EXPECT_TRUE(s.hasDecoration());
    EXPECT_TRUE(s.hasLakes());
    EXPECT_TRUE(s.isVoidGen()); // 还没有 updateLayers
    EXPECT_EQ(s.layersInfo().size(), 0u);
}

// ============================================================================
// Settings.hpp 聚合头文件 include 测试
// ============================================================================

TEST_F(SettingsTest, SettingsHeader_IncludesDimensionSettings)
{
    DimensionSettings s = DimensionSettings::overworld();
    EXPECT_EQ(s.dimensionKind, DimensionKind::Overworld);
}

TEST_F(SettingsTest, SettingsHeader_IncludesNoiseSettings)
{
    NoiseSettings s = NoiseSettings::overworld();
    EXPECT_EQ(s.minY, -64);
}

TEST_F(SettingsTest, SettingsHeader_IncludesScalingSettings)
{
    ScalingSettings s;
    EXPECT_FLOAT_EQ(s.xzFactor, 80.0f);
}

TEST_F(SettingsTest, SettingsHeader_IncludesSlideSettings)
{
    SlideSettings s{-10, 3, 0};
    EXPECT_EQ(s.target, -10);
}

TEST_F(SettingsTest, SettingsHeader_IncludesFlatLevelGeneratorSettings)
{
    auto s = FlatLevelGeneratorSettings::createDefault();
    EXPECT_EQ(s.layersInfo().size(), 3u);
}

TEST_F(SettingsTest, SettingsHeader_IncludesFlatLayerInfo)
{
    const BlockState* stone = VanillaBlocks::getState(VanillaBlocks::STONE);
    FlatLayerInfo info(3, stone);
    EXPECT_EQ(info.height(), 3);
}

// ============================================================================
// 跨维度一致性测试
// ============================================================================

TEST_F(SettingsTest, OverworldNoiseSizeMatchesChunkWidth)
{
    // 主世界的噪声大小应该能被 CHUNK_WIDTH 整除
    auto s = DimensionSettings::overworld();
    EXPECT_EQ(s.noise.noiseSizeX() * s.noise.horizontalNoiseGranularity(), world::CHUNK_WIDTH);
    EXPECT_EQ(s.noise.noiseSizeZ() * s.noise.horizontalNoiseGranularity(), world::CHUNK_WIDTH);
}

TEST_F(SettingsTest, NetherNoiseSizeMatchesChunkWidth)
{
    auto s = DimensionSettings::nether();
    EXPECT_EQ(s.noise.noiseSizeX() * s.noise.horizontalNoiseGranularity(), world::CHUNK_WIDTH);
    EXPECT_EQ(s.noise.noiseSizeZ() * s.noise.horizontalNoiseGranularity(), world::CHUNK_WIDTH);
}

TEST_F(SettingsTest, EndNoiseSizeMatchesChunkWidth)
{
    auto s = DimensionSettings::end();
    EXPECT_EQ(s.noise.noiseSizeX() * s.noise.horizontalNoiseGranularity(), world::CHUNK_WIDTH);
    EXPECT_EQ(s.noise.noiseSizeZ() * s.noise.horizontalNoiseGranularity(), world::CHUNK_WIDTH);
}

TEST_F(SettingsTest, AllDimensionSeaLevelsAreValid)
{
    // 所有维度的海平面高度应在噪声范围内
    auto ow = DimensionSettings::overworld();
    EXPECT_GE(ow.seaLevel, ow.noise.minY);
    EXPECT_LT(ow.seaLevel, ow.noise.minY + ow.noise.height);

    auto nether = DimensionSettings::nether();
    EXPECT_GE(nether.seaLevel, nether.noise.minY);
    EXPECT_LT(nether.seaLevel, nether.noise.minY + nether.noise.height);

    auto end = DimensionSettings::end();
    EXPECT_GE(end.seaLevel, end.noise.minY);
    // end 的 seaLevel=0，minY=0，height=128，0 < 128 通过
    EXPECT_LT(end.seaLevel, end.noise.minY + end.noise.height);
}

TEST_F(SettingsTest, AllDimensionDefaultBlockAndFluidNotNull)
{
    // 所有预设的 defaultBlock 和 defaultFluid 应非空
    auto ow = DimensionSettings::overworld();
    ASSERT_NE(ow.defaultBlock, nullptr);
    ASSERT_NE(ow.defaultFluid, nullptr);

    auto nether = DimensionSettings::nether();
    ASSERT_NE(nether.defaultBlock, nullptr);
    ASSERT_NE(nether.defaultFluid, nullptr);

    auto end = DimensionSettings::end();
    ASSERT_NE(end.defaultBlock, nullptr);
    ASSERT_NE(end.defaultFluid, nullptr);

    auto flat = DimensionSettings::flat();
    ASSERT_NE(flat.defaultBlock, nullptr);
    ASSERT_NE(flat.defaultFluid, nullptr);

    auto caves = DimensionSettings::caves();
    ASSERT_NE(caves.defaultBlock, nullptr);
    ASSERT_NE(caves.defaultFluid, nullptr);

    auto fi = DimensionSettings::floatingIslands();
    ASSERT_NE(fi.defaultBlock, nullptr);
    ASSERT_NE(fi.defaultFluid, nullptr);

    auto lb = DimensionSettings::largeBiomesPreset();
    ASSERT_NE(lb.defaultBlock, nullptr);
    ASSERT_NE(lb.defaultFluid, nullptr);

    auto amp = DimensionSettings::amplified();
    ASSERT_NE(amp.defaultBlock, nullptr);
    ASSERT_NE(amp.defaultFluid, nullptr);
}

// ============================================================================
// FillLayerEntry 和 updateLayers 填充层测试
// ============================================================================

TEST_F(SettingsTest, FlatLevelGeneratorSettings_UpdateLayers_SolidBlocks_NoFillEntries)
{
    // 固体方块不应产生填充层条目
    FlatLevelGeneratorSettings s(Biomes::Plains);
    s.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::BEDROCK));
    s.layersInfo().emplace_back(2, VanillaBlocks::getState(VanillaBlocks::STONE));
    s.updateLayers();
    EXPECT_TRUE(s.fillLayerEntries().empty());
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_UpdateLayers_WaterLayer_CreatesFillEntry)
{
    // 水层（液体）不应产生填充层条目——液体是运动阻挡方块，保留在 layers 中
    FlatLevelGeneratorSettings s(Biomes::Plains);
    s.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::BEDROCK));
    s.layersInfo().emplace_back(5, VanillaBlocks::getState(VanillaBlocks::WATER));
    s.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK));
    s.updateLayers();

    // 水是液体，isLiquid() 为 true，不是非运动阻挡方块，不产生填充层条目
    EXPECT_TRUE(s.fillLayerEntries().empty());
    // 水层保留在 layers 中（不是 nullptr）
    EXPECT_NE(s.layers()[1], nullptr); // 基岩
    EXPECT_NE(s.layers()[2], nullptr); // 水（不是 nullptr）
    EXPECT_NE(s.layers()[6], nullptr); // 水
    EXPECT_NE(s.layers()[7], nullptr); // 草方块
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_UpdateLayers_NonMotionBlocking_CreatesFillEntry)
{
    // 非运动阻挡（isSolid=false）、非液体、非空气的方块（如火把）应产生填充层条目
    // 参考 MC 1.21.11: FlatLevelGeneratorSettings.adjustGenerationSettings()
    // 使用 MOTION_BLOCKING 高度图判断：!blocksMotion() && fluidState.isEmpty()
    const BlockState* torchState = VanillaBlocks::getState(VanillaBlocks::TORCH);
    ASSERT_NE(torchState, nullptr) << "TORCH should be registered";

    // 确认火把是非固体、非液体、非空气
    EXPECT_FALSE(torchState->owner().isSolid(*torchState)) << "Torch should not be solid";
    EXPECT_FALSE(torchState->isLiquid()) << "Torch should not be liquid";
    EXPECT_FALSE(torchState->isAir()) << "Torch should not be air";

    FlatLevelGeneratorSettings s(Biomes::Plains);
    s.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::BEDROCK));
    s.layersInfo().emplace_back(1, torchState); // 非运动阻挡方块
    s.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK));
    s.updateLayers();

    // 非运动阻挡方块应产生填充层条目
    ASSERT_EQ(s.fillLayerEntries().size(), 1u);
    EXPECT_EQ(s.fillLayerEntries()[0].height, 1); // Y=1
    EXPECT_EQ(s.fillLayerEntries()[0].blockState, torchState);

    // 展开列表中 Y=1 应为 nullptr
    ASSERT_EQ(s.layers().size(), 3u);
    EXPECT_NE(s.layers()[0], nullptr); // 基岩
    EXPECT_EQ(s.layers()[1], nullptr); // 火把 → nullptr
    EXPECT_NE(s.layers()[2], nullptr); // 草方块
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_FillLayerEntries_DefaultEmpty)
{
    FlatLevelGeneratorSettings s(Biomes::Plains);
    EXPECT_TRUE(s.fillLayerEntries().empty());
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_CreateDefault_NoFillEntries)
{
    auto s = FlatLevelGeneratorSettings::createDefault();
    // 默认配置只有基岩、泥土、草方块——都是固体方块，无填充层
    EXPECT_TRUE(s.fillLayerEntries().empty());
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_DecorationAndLakes_DefaultFalse)
{
    FlatLevelGeneratorSettings s(Biomes::Plains);
    EXPECT_FALSE(s.hasDecoration());
    EXPECT_FALSE(s.hasLakes());

    s.setDecoration(true);
    EXPECT_TRUE(s.hasDecoration());
    EXPECT_FALSE(s.hasLakes());

    s.setLakes(true);
    EXPECT_TRUE(s.hasDecoration());
    EXPECT_TRUE(s.hasLakes());
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_ParameterizedConstructor_WithFlags)
{
    FlatLevelGeneratorSettings s(Biomes::Desert, true, true);
    EXPECT_EQ(s.biomeId(), Biomes::Desert);
    EXPECT_TRUE(s.hasDecoration());
    EXPECT_TRUE(s.hasLakes());
}

// ============================================================================
// FlatLevelGeneratorSettings structureOverrides 测试
// ============================================================================

TEST_F(SettingsTest, FlatLevelGeneratorSettings_StructureOverrides_DefaultEmpty)
{
    // 默认构造的 FlatLevelGeneratorSettings 没有结构覆盖
    FlatLevelGeneratorSettings s;
    EXPECT_TRUE(s.structureOverrides().empty());
    EXPECT_FALSE(s.hasStructureGeneration());
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_StructureOverrides_CreateDefault)
{
    // MC 1.21.11: 默认超平坦世界启用 villages 和 strongholds
    auto s = FlatLevelGeneratorSettings::createDefault();
    ASSERT_EQ(s.structureOverrides().size(), 2u);
    EXPECT_EQ(s.structureOverrides()[0], ResourceLocation::parse("minecraft:villages"));
    EXPECT_EQ(s.structureOverrides()[1], ResourceLocation::parse("minecraft:strongholds"));
    EXPECT_TRUE(s.hasStructureGeneration());
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_StructureOverrides_SetOverrides)
{
    FlatLevelGeneratorSettings s(Biomes::Plains, false, false);
    EXPECT_TRUE(s.structureOverrides().empty());
    EXPECT_FALSE(s.hasStructureGeneration());

    // 设置结构覆盖列表
    std::vector<ResourceLocation> overrides = {
        ResourceLocation::parse("minecraft:villages"),
        ResourceLocation::parse("minecraft:desert_pyramids"),
    };
    s.setStructureOverrides(std::move(overrides));
    ASSERT_EQ(s.structureOverrides().size(), 2u);
    EXPECT_TRUE(s.hasStructureGeneration());
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_StructureOverrides_DirectAccess)
{
    FlatLevelGeneratorSettings s(Biomes::Plains, false, false);

    // 通过可变引用添加
    s.structureOverrides().push_back(ResourceLocation::parse("minecraft:mineshafts"));
    ASSERT_EQ(s.structureOverrides().size(), 1u);
    EXPECT_TRUE(s.hasStructureGeneration());

    // 添加更多
    s.structureOverrides().push_back(ResourceLocation::parse("minecraft:strongholds"));
    ASSERT_EQ(s.structureOverrides().size(), 2u);
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_StructureOverrides_EmptyListMeansNoStructures)
{
    // 空的覆盖列表表示不生成任何结构
    FlatLevelGeneratorSettings s(Biomes::Plains, false, false);
    s.setStructureOverrides({});
    EXPECT_TRUE(s.structureOverrides().empty());
    EXPECT_FALSE(s.hasStructureGeneration());
}

TEST_F(SettingsTest, FlatLevelGeneratorSettings_StructureOverrides_PreservesOtherSettings)
{
    // 设置结构覆盖不应影响其他设置
    FlatLevelGeneratorSettings s(Biomes::Desert, true, true);
    s.setStructureOverrides({ResourceLocation::parse("minecraft:villages")});

    EXPECT_EQ(s.biomeId(), Biomes::Desert);
    EXPECT_TRUE(s.hasDecoration());
    EXPECT_TRUE(s.hasLakes());
    EXPECT_TRUE(s.hasStructureGeneration());
    ASSERT_EQ(s.structureOverrides().size(), 1u);
}

// ============================================================================
// DimensionSettings.spawnTarget 测试
// ============================================================================
//
// MC 1.21.11: NoiseGeneratorSettings.spawnTarget
// - overworld / large_biomes / amplified: 由 OverworldBiomeBuilder.spawnTarget() 提供（2 项）
// - nether / end / caves / floating_islands / flat: 空列表

TEST_F(SettingsTest, DimensionSettings_Overworld_HasSpawnTarget)
{
    auto s = DimensionSettings::overworld();
    EXPECT_EQ(s.spawnTarget.size(), 2u);
}

TEST_F(SettingsTest, DimensionSettings_LargeBiomes_HasSpawnTarget)
{
    auto s = DimensionSettings::largeBiomesPreset();
    EXPECT_EQ(s.spawnTarget.size(), 2u);
}

TEST_F(SettingsTest, DimensionSettings_Amplified_HasSpawnTarget)
{
    auto s = DimensionSettings::amplified();
    EXPECT_EQ(s.spawnTarget.size(), 2u);
}

TEST_F(SettingsTest, DimensionSettings_Nether_HasEmptySpawnTarget)
{
    auto s = DimensionSettings::nether();
    EXPECT_TRUE(s.spawnTarget.empty());
}

TEST_F(SettingsTest, DimensionSettings_End_HasEmptySpawnTarget)
{
    auto s = DimensionSettings::end();
    EXPECT_TRUE(s.spawnTarget.empty());
}

TEST_F(SettingsTest, DimensionSettings_Caves_HasEmptySpawnTarget)
{
    auto s = DimensionSettings::caves();
    EXPECT_TRUE(s.spawnTarget.empty());
}

TEST_F(SettingsTest, DimensionSettings_FloatingIslands_HasEmptySpawnTarget)
{
    auto s = DimensionSettings::floatingIslands();
    EXPECT_TRUE(s.spawnTarget.empty());
}

TEST_F(SettingsTest, DimensionSettings_Flat_HasEmptySpawnTarget)
{
    auto s = DimensionSettings::flat();
    EXPECT_TRUE(s.spawnTarget.empty());
}

TEST_F(SettingsTest, DimensionSettings_OverworldSpawnTarget_MatchesOverworldBiomeBuilder)
{
    // 主世界 spawnTarget 应与 OverworldBiomeBuilder.spawnTarget() 等价
    auto s = DimensionSettings::overworld();
    ASSERT_EQ(s.spawnTarget.size(), 2u);

    world::biome::source::OverworldBiomeBuilder builder;
    auto expected = builder.spawnTarget();
    ASSERT_EQ(expected.size(), 2u);

    for (size_t i = 0; i < 2; ++i) {
        EXPECT_EQ(s.spawnTarget[i].temperature, expected[i].temperature);
        EXPECT_EQ(s.spawnTarget[i].humidity, expected[i].humidity);
        EXPECT_EQ(s.spawnTarget[i].continentalness, expected[i].continentalness);
        EXPECT_EQ(s.spawnTarget[i].erosion, expected[i].erosion);
        EXPECT_EQ(s.spawnTarget[i].depth, expected[i].depth);
        EXPECT_EQ(s.spawnTarget[i].weirdness, expected[i].weirdness);
        EXPECT_EQ(s.spawnTarget[i].offset, expected[i].offset);
    }
}

} // namespace
} // namespace mc
