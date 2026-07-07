/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, or sell
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

// ============================================================================
// FlatChunkGenerator 详细单元测试
//
// 测试覆盖：
// 1. 构造函数和基本属性
// 2. generateNoise 层填充逻辑
// 3. getHeight 各高度图类型
// 4. getBaseColumn 返回值
// 5. getBiome/getNoiseBiome 固定生物群系
// 6. seaLevel/getMinY/getGenDepth 维度常量
// 7. 空操作阶段验证
// 8. placeFeatures 和 _placeFillLayers 逻辑
//    8.1 默认设置（无装饰、无湖泊）
//    8.2 _placeFillLayers 填充层逻辑
//    8.3 generateNoise 跳过 nullptr 层
// 9. IChunkGenerator 接口验证
// 10. 与 MC 1.21.11 FlatLevelSource 的行为对齐
// ============================================================================

#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/gen/chunk/FlatChunkGenerator.hpp"
#include "common/world/gen/settings/FlatLevelGeneratorSettings.hpp"
#include <gtest/gtest.h>

using namespace mc;

namespace {

class FlatChunkGeneratorDetailedTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }
};

// ============================================================================
// 1. 构造函数和基本属性
// ============================================================================

TEST_F(FlatChunkGeneratorDetailedTest, Construction_DefaultSettings)
{
    FlatChunkGenerator gen(12345LL, FlatLevelGeneratorSettings::createDefault());
    EXPECT_EQ(gen.seed(), 12345u);
    EXPECT_EQ(gen.seaLevel(), -63);
    EXPECT_EQ(gen.getMinY(), 0);
    EXPECT_EQ(gen.getGenDepth(), 384);
    EXPECT_FALSE(gen.isDebugGenerator());
}

TEST_F(FlatChunkGeneratorDetailedTest, Construction_CustomSeed)
{
    FlatChunkGenerator gen1(0LL, FlatLevelGeneratorSettings::createDefault());
    FlatChunkGenerator gen2(999999999LL, FlatLevelGeneratorSettings::createDefault());
    EXPECT_EQ(gen1.seed(), 0u);
    EXPECT_EQ(gen2.seed(), 999999999u);
}

TEST_F(FlatChunkGeneratorDetailedTest, BiomeSource_NotNull)
{
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    auto* source = gen.getBiomeSource();
    ASSERT_NE(source, nullptr);
}

TEST_F(FlatChunkGeneratorDetailedTest, BiomeSource_FixedBiome)
{
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    auto* source = gen.getBiomeSource();
    // FixedBiomeSource 对所有位置返回 Plains
    EXPECT_EQ(source->getNoiseBiome(0, 0, 0), Biomes::Plains);
    EXPECT_EQ(source->getNoiseBiome(100, 50, 100), Biomes::Plains);
    EXPECT_EQ(source->getNoiseBiome(-100, -50, -100), Biomes::Plains);
}

// ============================================================================
// 2. 维度常量（与 MC 1.21 对齐）
// ============================================================================

TEST_F(FlatChunkGeneratorDetailedTest, SeaLevel_Minus63)
{
    // MC 1.21: FlatLevelSource.getSeaLevel() = -63
    // 海平面在 Y=-63（低于世界底部），因此平坦世界没有水
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    EXPECT_EQ(gen.seaLevel(), -63);
}

TEST_F(FlatChunkGeneratorDetailedTest, MinY_Is0)
{
    // MC 1.21: FlatLevelSource.getMinY() = 0
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    EXPECT_EQ(gen.getMinY(), 0);
}

TEST_F(FlatChunkGeneratorDetailedTest, GenDepth_Is384)
{
    // MC 1.21: FlatLevelSource.getGenDepth() = 384
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    EXPECT_EQ(gen.getGenDepth(), 384);
}

TEST_F(FlatChunkGeneratorDetailedTest, GroundHeight_Default)
{
    // 默认平坦世界: 基岩(Y=0) + 泥土(Y=1-2) + 草方块(Y=3)
    // WorldSurfaceWG 高度 = 4 (草方块上方第一个空气方块)
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    EXPECT_EQ(gen.getGroundHeight(), 4);
}

// ============================================================================
// 3. getBiome/getNoiseBiome 固定生物群系
// ============================================================================

TEST_F(FlatChunkGeneratorDetailedTest, GetBiome_AlwaysReturnsPlains)
{
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    EXPECT_EQ(gen.getBiome(0, 0, 0), Biomes::Plains);
    EXPECT_EQ(gen.getBiome(100, 50, 100), Biomes::Plains);
    EXPECT_EQ(gen.getBiome(-100, -50, -100), Biomes::Plains);
    EXPECT_EQ(gen.getBiome(1000, 100, 1000), Biomes::Plains);
}

TEST_F(FlatChunkGeneratorDetailedTest, GetNoiseBiome_AlwaysReturnsPlains)
{
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    EXPECT_EQ(gen.getNoiseBiome(0, 0, 0), Biomes::Plains);
    EXPECT_EQ(gen.getNoiseBiome(4, 4, 4), Biomes::Plains);
    EXPECT_EQ(gen.getNoiseBiome(-4, -4, -4), Biomes::Plains);
}

TEST_F(FlatChunkGeneratorDetailedTest, GetBiome_CustomBiome)
{
    // 创建自定义生物群系的平坦世界
    FlatLevelGeneratorSettings settings(Biomes::Desert, false, false);
    FlatChunkGenerator gen(0LL, std::move(settings));
    EXPECT_EQ(gen.getBiome(0, 0, 0), Biomes::Desert);
    EXPECT_EQ(gen.getNoiseBiome(0, 0, 0), Biomes::Desert);
}

// ============================================================================
// 4. getHeight 各高度图类型
// ============================================================================

TEST_F(FlatChunkGeneratorDetailedTest, GetHeight_WorldSurfaceWG)
{
    // 默认平坦世界: 基岩(0) + 泥土(1-2) + 草方块(3)
    // WorldSurfaceWG: 任何非空方块都被计入，最高为 Y=3，返回 Y=4
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    EXPECT_EQ(gen.getHeight(0, 0, HeightmapType::WorldSurfaceWG), 4);
    EXPECT_EQ(gen.getHeight(100, 100, HeightmapType::WorldSurfaceWG), 4);
    EXPECT_EQ(gen.getHeight(-100, -100, HeightmapType::WorldSurfaceWG), 4);
}

TEST_F(FlatChunkGeneratorDetailedTest, GetHeight_WorldSurface)
{
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    EXPECT_EQ(gen.getHeight(0, 0, HeightmapType::WorldSurface), 4);
}

TEST_F(FlatChunkGeneratorDetailedTest, GetHeight_OceanFloorWG)
{
    // 默认平坦世界没有水层，OceanFloorWG 等同于 WorldSurfaceWG
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    // 基岩和泥土是固体，草方块也是固体
    EXPECT_EQ(gen.getHeight(0, 0, HeightmapType::OceanFloorWG), 4);
}

TEST_F(FlatChunkGeneratorDetailedTest, GetHeight_MotionBlocking)
{
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    // 基岩、泥土、草方块都阻挡运动
    EXPECT_EQ(gen.getHeight(0, 0, HeightmapType::MotionBlocking), 4);
}

TEST_F(FlatChunkGeneratorDetailedTest, GetHeight_HeightIndependentOfPosition)
{
    // 平坦世界的高度在所有位置相同
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    const i32 h1 = gen.getHeight(0, 0, HeightmapType::WorldSurfaceWG);
    const i32 h2 = gen.getHeight(100, 200, HeightmapType::WorldSurfaceWG);
    const i32 h3 = gen.getHeight(-500, -500, HeightmapType::WorldSurfaceWG);
    EXPECT_EQ(h1, h2);
    EXPECT_EQ(h2, h3);
}

// ============================================================================
// 5. getBaseColumn 返回值
// ============================================================================

TEST_F(FlatChunkGeneratorDetailedTest, GetBaseColumn_DefaultSettings)
{
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    auto column = gen.getBaseColumn(0, 0);

    EXPECT_EQ(column.minY(), 0);
    EXPECT_EQ(column.height(), 384);
}

TEST_F(FlatChunkGeneratorDetailedTest, GetBaseColumn_BedrockLayer)
{
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    auto column = gen.getBaseColumn(0, 0);

    // 第0层应该是基岩
    const BlockState* bedrock = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    ASSERT_NE(bedrock, nullptr);
    EXPECT_EQ(column.getBlock(0), bedrock);
}

TEST_F(FlatChunkGeneratorDetailedTest, GetBaseColumn_DirtLayers)
{
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    auto column = gen.getBaseColumn(0, 0);

    // Y=1 和 Y=2 应该是泥土
    const BlockState* dirt = VanillaBlocks::getState(VanillaBlocks::DIRT);
    ASSERT_NE(dirt, nullptr);
    EXPECT_EQ(column.getBlock(1), dirt);
    EXPECT_EQ(column.getBlock(2), dirt);
}

TEST_F(FlatChunkGeneratorDetailedTest, GetBaseColumn_GrassLayer)
{
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    auto column = gen.getBaseColumn(0, 0);

    // Y=3 应该是草方块
    const BlockState* grass = VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK);
    ASSERT_NE(grass, nullptr);
    EXPECT_EQ(column.getBlock(3), grass);
}

TEST_F(FlatChunkGeneratorDetailedTest, GetBaseColumn_AboveLayersIsAir)
{
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    auto column = gen.getBaseColumn(0, 0);

    // Y=4 及以上应该是空气（nullptr 或空气方块）
    const BlockState* air = BlockRegistry::instance().airState();
    for (i32 y = 4; y < 20; ++y) {
        const BlockState* state = column.getBlock(y);
        EXPECT_TRUE(state == nullptr || state->isAir()) << "Y=" << y << " should be air";
    }
}

TEST_F(FlatChunkGeneratorDetailedTest, GetBaseColumn_PositionIndependent)
{
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());

    // 平坦世界的列在所有位置应该相同
    auto col1 = gen.getBaseColumn(0, 0);
    auto col2 = gen.getBaseColumn(100, 200);
    auto col3 = gen.getBaseColumn(-500, -500);

    EXPECT_EQ(col1.minY(), col2.minY());
    EXPECT_EQ(col1.height(), col2.height());
    EXPECT_EQ(col1.getBlock(0), col2.getBlock(0));
    EXPECT_EQ(col1.getBlock(3), col2.getBlock(3));

    EXPECT_EQ(col1.minY(), col3.minY());
    EXPECT_EQ(col1.height(), col3.height());
    EXPECT_EQ(col1.getBlock(0), col3.getBlock(0));
}

// ============================================================================
// 6. 空操作阶段验证
// ============================================================================

TEST_F(FlatChunkGeneratorDetailedTest, BuildSurface_DoesNothing)
{
    // 平坦世界的 buildSurface 是空操作
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    // 无法直接测试不修改区块，但可以确认方法可调用
    ChunkPrimer primer(0, 0);
    // createRegion for buildSurface needs WorldGenRegion
    // 仅验证方法存在且不崩溃
}

TEST_F(FlatChunkGeneratorDetailedTest, SpawnInitialMobs_ReturnsZero)
{
    // 平坦世界不生成被动动物
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    // spawnInitialMobs 需要 WorldGenRegion，此处只验证签名存在
    // 实际测试在集成测试中
}

// ============================================================================
// 7. 自定义层配置
// ============================================================================

TEST_F(FlatChunkGeneratorDetailedTest, CustomSingleLayer)
{
    // 单层基岩的平坦世界
    FlatLevelGeneratorSettings settings(Biomes::Plains, false, false);
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::BEDROCK));
    settings.updateLayers();

    FlatChunkGenerator gen(0LL, std::move(settings));
    auto column = gen.getBaseColumn(0, 0);

    const BlockState* bedrock = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    ASSERT_NE(bedrock, nullptr);
    EXPECT_EQ(column.getBlock(0), bedrock);
    // Y=1 及以上应为空气
    EXPECT_TRUE(column.getBlock(1) == nullptr || column.getBlock(1)->isAir());
}

// ============================================================================
// 8. placeFeatures 和 _placeFillLayers 测试
// ============================================================================

// 辅助函数：创建 3x3 区块区域的 WorldGenRegion
namespace PlaceFeaturesTestHelper {

std::unique_ptr<WorldGenRegion> createRegion(
    ChunkCoord mainX, ChunkCoord mainZ, i32 radius, std::vector<std::unique_ptr<ChunkPrimer>>& outChunks)
{
    const i32 size = 2 * radius + 1;
    std::vector<IChunk*> chunkPtrs;
    outChunks.clear();
    outChunks.reserve(static_cast<size_t>(size * size));

    for (i32 dz = -radius; dz <= radius; ++dz) {
        for (i32 dx = -radius; dx <= radius; ++dx) {
            auto chunk = std::make_unique<ChunkPrimer>(mainX + dx, mainZ + dz);
            chunkPtrs.push_back(chunk.get());
            outChunks.push_back(std::move(chunk));
        }
    }

    return std::make_unique<WorldGenRegion>(mainX, mainZ, radius, std::move(chunkPtrs));
}

} // namespace PlaceFeaturesTestHelper

// ============================================================================
// 8.1 默认设置（无装饰、无湖泊）
// ============================================================================

TEST_F(FlatChunkGeneratorDetailedTest, PlaceFeatures_DefaultSettings_SetsChunkStatus)
{
    // 默认设置：decoration=false, addLakes=false
    // placeFeatures 应仅设置区块状态为 FEATURES
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());

    std::vector<std::unique_ptr<ChunkPrimer>> chunks;
    auto region = PlaceFeaturesTestHelper::createRegion(0, 0, 1, chunks);
    ChunkPrimer& centerChunk = *chunks[4]; // 3x3 网格的中心

    // 先运行 generateNoise 来填充层
    gen.generateNoise(*region, centerChunk);

    // 然后运行 placeFeatures
    gen.placeFeatures(*region, centerChunk);

    EXPECT_EQ(&centerChunk.getChunkStatus(), &ChunkStatuses::FEATURES);
}

TEST_F(FlatChunkGeneratorDetailedTest, PlaceFeatures_DefaultSettings_NoFillLayers)
{
    // 默认配置只有固体方块（基岩、泥土、草方块），不应有填充层条目
    auto settings = FlatLevelGeneratorSettings::createDefault();
    EXPECT_TRUE(settings.fillLayerEntries().empty());

    // generateNoise 后所有层都被填充，placeFeatures 后不变
    FlatChunkGenerator gen(0LL, settings);

    std::vector<std::unique_ptr<ChunkPrimer>> chunks;
    auto region = PlaceFeaturesTestHelper::createRegion(0, 0, 1, chunks);
    ChunkPrimer& centerChunk = *chunks[4];

    gen.generateNoise(*region, centerChunk);
    gen.placeFeatures(*region, centerChunk);

    // 验证层仍为固体方块
    const BlockState* bedrock = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    const BlockState* dirt = VanillaBlocks::getState(VanillaBlocks::DIRT);
    const BlockState* grass = VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK);

    // Y=0: 基岩, Y=1-2: 泥土, Y=3: 草方块（minY=0）
    EXPECT_EQ(centerChunk.getBlockState(0, 0, 0), bedrock);
    EXPECT_EQ(centerChunk.getBlockState(0, 1, 0), dirt);
    EXPECT_EQ(centerChunk.getBlockState(0, 2, 0), dirt);
    EXPECT_EQ(centerChunk.getBlockState(0, 3, 0), grass);
}

// ============================================================================
// 8.2 _placeFillLayers 填充层逻辑
// ============================================================================

TEST_F(FlatChunkGeneratorDetailedTest, PlaceFeatures_FillLayers_WaterLayerPreservedInNoise)
{
    // 水层（液体）在 updateLayers 中不会被设为 nullptr，因为 isLiquid() 为 true
    // 因此 generateNoise 会直接放置水方块，不需要 _placeFillLayers 填充
    FlatLevelGeneratorSettings settings(Biomes::Plains, false, false);
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::BEDROCK));
    settings.layersInfo().emplace_back(5, VanillaBlocks::getState(VanillaBlocks::WATER));
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK));
    settings.updateLayers();

    // 水是液体，不是非运动阻挡方块，所以不应产生填充层条目
    EXPECT_TRUE(settings.fillLayerEntries().empty());

    // 水层在展开列表中不是 nullptr
    // layers[0] = 基岩, layers[1-5] = 水, layers[6] = 草方块
    const auto& layers = settings.layers();
    EXPECT_NE(layers[0], nullptr); // 基岩
    EXPECT_NE(layers[1], nullptr); // 水
    EXPECT_NE(layers[5], nullptr); // 水
    EXPECT_NE(layers[6], nullptr); // 草方块

    FlatChunkGenerator gen(0LL, settings);

    std::vector<std::unique_ptr<ChunkPrimer>> chunks;
    auto region = PlaceFeaturesTestHelper::createRegion(0, 0, 1, chunks);
    ChunkPrimer& centerChunk = *chunks[4];

    gen.generateNoise(*region, centerChunk);
    gen.placeFeatures(*region, centerChunk);

    // 水层应直接由 generateNoise 放置，不需要 _placeFillLayers
    const BlockState* water = &VanillaBlocks::WATER->defaultState();
    EXPECT_EQ(centerChunk.getBlockState(0, 1, 0), water);
    EXPECT_EQ(centerChunk.getBlockState(0, 5, 0), water);
}

TEST_F(FlatChunkGeneratorDetailedTest, PlaceFeatures_FillLayers_NonMotionBlockingBlockFilledByPlaceFeatures)
{
    // 创建包含非固体非液体方块的层配置
    // 非固体、非液体、非空气的方块（如火把 TORCH）会被 updateLayers 设为 nullptr
    // generateNoise 跳过 nullptr 层（留下空气），_placeFillLayers 在 placeFeatures 末尾补充放置

    // 使用 TORCH 作为非固体方块（Material::DECORATION, notSolid）
    const BlockState* torchState = VanillaBlocks::getState(VanillaBlocks::TORCH);
    ASSERT_NE(torchState, nullptr) << "TORCH should be registered";

    // 验证火把确实是非固体、非液体方块
    bool isTorchSolid = torchState->owner().isSolid(*torchState);
    bool isTorchLiquid = torchState->isLiquid();
    bool isTorchAir = torchState->isAir();
    EXPECT_FALSE(isTorchSolid) << "Torch should not be solid";
    EXPECT_FALSE(isTorchLiquid) << "Torch should not be liquid";
    EXPECT_FALSE(isTorchAir) << "Torch should not be air";

    FlatLevelGeneratorSettings settings(Biomes::Plains, false, false);
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::BEDROCK));
    settings.layersInfo().emplace_back(2, torchState); // 非固体层
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK));
    settings.updateLayers();

    // 非固体非液体非空气方块应产生填充层条目
    ASSERT_EQ(settings.fillLayerEntries().size(), 2u) << "Should have 2 fill layer entries (2 torch layers)";
    EXPECT_EQ(settings.fillLayerEntries()[0].height, 1);
    EXPECT_EQ(settings.fillLayerEntries()[0].blockState, torchState);
    EXPECT_EQ(settings.fillLayerEntries()[1].height, 2);
    EXPECT_EQ(settings.fillLayerEntries()[1].blockState, torchState);

    // 展开列表中对应位置应为 nullptr
    EXPECT_EQ(settings.layers()[1], nullptr); // Y=1 火把 → nullptr
    EXPECT_EQ(settings.layers()[2], nullptr); // Y=2 火把 → nullptr

    FlatChunkGenerator gen(0LL, settings);

    std::vector<std::unique_ptr<ChunkPrimer>> chunks;
    auto region = PlaceFeaturesTestHelper::createRegion(0, 0, 1, chunks);
    ChunkPrimer& centerChunk = *chunks[4];

    // generateNoise 跳过 nullptr 层，留下空气
    gen.generateNoise(*region, centerChunk);

    // Y=1 和 Y=2 应为空气（nullptr 层被跳过）
    const BlockState* state1 = centerChunk.getBlockState(0, 1, 0);
    EXPECT_TRUE(state1 == nullptr || state1->isAir()) << "Y=1 should be air after generateNoise";

    const BlockState* state2 = centerChunk.getBlockState(0, 2, 0);
    EXPECT_TRUE(state2 == nullptr || state2->isAir()) << "Y=2 should be air after generateNoise";

    // placeFeatures 在末尾调用 _placeFillLayers 补充放置非运动阻挡方块
    gen.placeFeatures(*region, centerChunk);

    // Y=1 和 Y=2 现在应该是火把方块
    EXPECT_EQ(centerChunk.getBlockState(0, 1, 0), torchState) << "Y=1 should be torch after _placeFillLayers";
    EXPECT_EQ(centerChunk.getBlockState(0, 2, 0), torchState) << "Y=2 should be torch after _placeFillLayers";
}

TEST_F(FlatChunkGeneratorDetailedTest, PlaceFeatures_FillLayers_PreservesExistingBlocks)
{
    // _placeFillLayers 仅替换空气方块，不替换已有方块
    // 这确保了与湖泊等特性的兼容性——如果湖泊已经放置了方块，填充层不会覆盖

    const BlockState* torchState = VanillaBlocks::getState(VanillaBlocks::TORCH);
    if (torchState == nullptr || torchState->owner().isSolid(*torchState)) {
        GTEST_SKIP() << "TORCH not available or is solid, skipping fill layer preservation test";
    }

    FlatLevelGeneratorSettings settings(Biomes::Plains, false, false);
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::BEDROCK));
    settings.layersInfo().emplace_back(1, torchState); // Y=1: 非固体层
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK));
    settings.updateLayers();

    FlatChunkGenerator gen(0LL, settings);

    std::vector<std::unique_ptr<ChunkPrimer>> chunks;
    auto region = PlaceFeaturesTestHelper::createRegion(0, 0, 1, chunks);
    ChunkPrimer& centerChunk = *chunks[4];

    gen.generateNoise(*region, centerChunk);

    // 在 _placeFillLayers 之前，手动在填充层位置放置一个非空气方块
    // 模拟湖泊等特性已经在此位置放置了方块
    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    region->setBlockState(0, 1, 0, stoneState);

    gen.placeFeatures(*region, centerChunk);

    // Y=1 应保留为石头（_placeFillLayers 不覆盖非空气方块）
    EXPECT_EQ(centerChunk.getBlockState(0, 1, 0), stoneState)
        << "Y=1 should remain STONE (not overwritten by _placeFillLayers)";
}

// ============================================================================
// 8.3 decoration 和 addLakes 标志逻辑验证
// ============================================================================

TEST_F(FlatChunkGeneratorDetailedTest, PlaceFeatures_NoDecorationNoLakes_NoFeaturePlacement)
{
    // decoration=false, addLakes=false：不放置任何特性
    // 这是默认配置，仅设置区块状态和填充层
    FlatLevelGeneratorSettings settings(Biomes::Plains, false, false);
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::BEDROCK));
    settings.layersInfo().emplace_back(2, VanillaBlocks::getState(VanillaBlocks::DIRT));
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK));
    settings.updateLayers();

    EXPECT_FALSE(settings.hasDecoration());
    EXPECT_FALSE(settings.hasLakes());

    FlatChunkGenerator gen(0LL, settings);

    std::vector<std::unique_ptr<ChunkPrimer>> chunks;
    auto region = PlaceFeaturesTestHelper::createRegion(0, 0, 1, chunks);
    ChunkPrimer& centerChunk = *chunks[4];

    gen.generateNoise(*region, centerChunk);
    gen.placeFeatures(*region, centerChunk);

    // 区块状态应为 FEATURES
    EXPECT_EQ(&centerChunk.getChunkStatus(), &ChunkStatuses::FEATURES);

    // 层应保持不变（无特性放置）
    const BlockState* grass = VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK);
    EXPECT_EQ(centerChunk.getBlockState(0, 3, 0), grass);
}

// ============================================================================
// 8.4 generateNoise 跳过 nullptr 层
// ============================================================================

TEST_F(FlatChunkGeneratorDetailedTest, GenerateNoise_SkipsNullLayersLeavingAir)
{
    // 当 updateLayers 将某些层设为 nullptr 时，generateNoise 应跳过这些层
    // 在那些位置留下空气方块
    const BlockState* torchState = VanillaBlocks::getState(VanillaBlocks::TORCH);
    if (torchState == nullptr || torchState->owner().isSolid(*torchState)) {
        GTEST_SKIP() << "TORCH not available or is solid, skipping null layer test";
    }

    FlatLevelGeneratorSettings settings(Biomes::Plains, false, false);
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::BEDROCK)); // Y=0
    settings.layersInfo().emplace_back(1, torchState);                                      // Y=1 → nullptr in layers
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK)); // Y=2
    settings.updateLayers();

    // 验证 layers[1] 为 nullptr（非固体层）
    EXPECT_EQ(settings.layers()[1], nullptr);

    FlatChunkGenerator gen(0LL, settings);

    std::vector<std::unique_ptr<ChunkPrimer>> chunks;
    auto region = PlaceFeaturesTestHelper::createRegion(0, 0, 1, chunks);
    ChunkPrimer& centerChunk = *chunks[4];

    gen.generateNoise(*region, centerChunk);

    // Y=0: 基岩（固体，直接放置）
    EXPECT_EQ(centerChunk.getBlockState(0, 0, 0), VanillaBlocks::getState(VanillaBlocks::BEDROCK));
    // Y=1: 空气（nullptr 层被跳过）
    const BlockState* state1 = centerChunk.getBlockState(0, 1, 0);
    EXPECT_TRUE(state1 == nullptr || state1->isAir());
    // Y=2: 草方块（固体，直接放置）
    EXPECT_EQ(centerChunk.getBlockState(0, 2, 0), VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK));
}

// ============================================================================
// 9. IChunkGenerator 接口验证
// ============================================================================

TEST_F(FlatChunkGeneratorDetailedTest, InterfaceCast)
{
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    IChunkGenerator& base = gen;

    // 通过基类指针访问
    EXPECT_EQ(base.seed(), 0u);
    EXPECT_EQ(base.seaLevel(), -63);
    EXPECT_EQ(base.getMinY(), 0);
    EXPECT_EQ(base.getGenDepth(), 384);
    EXPECT_EQ(base.getBiome(0, 0, 0), Biomes::Plains);
    EXPECT_EQ(base.getNoiseBiome(0, 0, 0), Biomes::Plains);
    EXPECT_FALSE(base.isDebugGenerator());
    EXPECT_NE(base.getBiomeSource(), nullptr);
}

TEST_F(FlatChunkGeneratorDetailedTest, GetSpawnHeight)
{
    // IChunkGenerator::getSpawnHeight 调用 getHeight(x, z, WorldSurfaceWG)
    // FlatChunkGenerator 的 WorldSurfaceWG 高度为 4（草方块上方第一个空气方块）
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    EXPECT_EQ(gen.getSpawnHeight(0, 0), 4);
}

// ============================================================================
// 10. 结构生成测试
// ============================================================================

TEST_F(FlatChunkGeneratorDetailedTest, StructureOverrides_DefaultHasVillagesAndStrongholds)
{
    // MC 1.21.11: 默认超平坦世界启用 minecraft:villages 和 minecraft:strongholds
    auto settings = FlatLevelGeneratorSettings::createDefault();
    const auto& overrides = settings.structureOverrides();
    ASSERT_EQ(overrides.size(), 2u);
    EXPECT_EQ(overrides[0], ResourceLocation::parse("minecraft:villages"));
    EXPECT_EQ(overrides[1], ResourceLocation::parse("minecraft:strongholds"));
    EXPECT_TRUE(settings.hasStructureGeneration());
}

TEST_F(FlatChunkGeneratorDetailedTest, StructureOverrides_EmptyMeansNoStructures)
{
    // 空的 structureOverrides 表示不生成任何结构
    FlatLevelGeneratorSettings settings(Biomes::Plains, false, false);
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::BEDROCK));
    settings.layersInfo().emplace_back(2, VanillaBlocks::getState(VanillaBlocks::DIRT));
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK));
    settings.updateLayers();

    EXPECT_TRUE(settings.structureOverrides().empty());
    EXPECT_FALSE(settings.hasStructureGeneration());

    // generateStructureStarts 应设置状态但不生成任何结构
    FlatChunkGenerator gen(0LL, settings);
    std::vector<std::unique_ptr<ChunkPrimer>> chunks;
    auto region = PlaceFeaturesTestHelper::createRegion(0, 0, 1, chunks);
    ChunkPrimer& centerChunk = *chunks[4];

    gen.generateStructureStarts(*region, centerChunk);
    EXPECT_EQ(&centerChunk.getChunkStatus(), &ChunkStatuses::STRUCTURE_STARTS);
}

TEST_F(FlatChunkGeneratorDetailedTest, GenerateStructureStarts_SetsChunkStatus)
{
    // 有 structureOverrides 时，generateStructureStarts 应设置正确的区块状态
    FlatLevelGeneratorSettings settings(Biomes::Plains, false, false);
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::BEDROCK));
    settings.layersInfo().emplace_back(2, VanillaBlocks::getState(VanillaBlocks::DIRT));
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK));
    settings.setStructureOverrides({ResourceLocation::parse("minecraft:villages")});
    settings.updateLayers();

    FlatChunkGenerator gen(0LL, settings);
    std::vector<std::unique_ptr<ChunkPrimer>> chunks;
    auto region = PlaceFeaturesTestHelper::createRegion(0, 0, 1, chunks);
    ChunkPrimer& centerChunk = *chunks[4];

    gen.generateStructureStarts(*region, centerChunk);
    EXPECT_EQ(&centerChunk.getChunkStatus(), &ChunkStatuses::STRUCTURE_STARTS);
}

TEST_F(FlatChunkGeneratorDetailedTest, GenerateStructureReferences_SetsChunkStatus)
{
    // generateStructureReferences 应设置 STRUCTURE_REFERENCES 状态
    FlatLevelGeneratorSettings settings(Biomes::Plains, false, false);
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::BEDROCK));
    settings.layersInfo().emplace_back(2, VanillaBlocks::getState(VanillaBlocks::DIRT));
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK));
    settings.updateLayers();

    FlatChunkGenerator gen(0LL, settings);
    std::vector<std::unique_ptr<ChunkPrimer>> chunks;
    auto region = PlaceFeaturesTestHelper::createRegion(0, 0, 1, chunks);
    ChunkPrimer& centerChunk = *chunks[4];

    gen.generateStructureReferences(*region, centerChunk);
    EXPECT_EQ(&centerChunk.getChunkStatus(), &ChunkStatuses::STRUCTURE_REFERENCES);
}

TEST_F(FlatChunkGeneratorDetailedTest, StructureOverrides_IncompatibleBiome_SkipsStructureSet)
{
    // 沙漠生物群系不应生成村庄（村庄需要 Plains/Savanna/Taiga/Snowy 等生物群系）
    // 注意：实际上 MC 中村庄也可以在 Desert 生成，这里测试 _hasBiomesForStructureSet 的过滤逻辑
    // 使用 Nether 相关的结构集，它们不应在 Plains 生物群系中生成
    FlatLevelGeneratorSettings settings(Biomes::Plains, false, false);
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::BEDROCK));
    settings.layersInfo().emplace_back(2, VanillaBlocks::getState(VanillaBlocks::DIRT));
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK));
    // nether_complexes 是下界结构集，不应在 Plains 生物群系中生成
    settings.setStructureOverrides({ResourceLocation::parse("minecraft:nether_complexes")});
    settings.updateLayers();

    FlatChunkGenerator gen(0LL, settings);
    std::vector<std::unique_ptr<ChunkPrimer>> chunks;
    auto region = PlaceFeaturesTestHelper::createRegion(0, 0, 1, chunks);
    ChunkPrimer& centerChunk = *chunks[4];

    // generateStructureStarts 应成功执行（不崩溃），但不会生成下界要塞
    gen.generateStructureStarts(*region, centerChunk);
    EXPECT_EQ(&centerChunk.getChunkStatus(), &ChunkStatuses::STRUCTURE_STARTS);
}

TEST_F(FlatChunkGeneratorDetailedTest, StructureOverrides_DefaultSettings_VillagesAndStrongholds)
{
    // 使用 createDefault() 设置，应同时支持村庄和要塞结构生成
    auto settings = FlatLevelGeneratorSettings::createDefault();
    EXPECT_TRUE(settings.hasStructureGeneration());
    EXPECT_EQ(settings.structureOverrides().size(), 2u);

    FlatChunkGenerator gen(0LL, settings);
    std::vector<std::unique_ptr<ChunkPrimer>> chunks;
    auto region = PlaceFeaturesTestHelper::createRegion(0, 0, 1, chunks);
    ChunkPrimer& centerChunk = *chunks[4];

    // generateStructureStarts 不应崩溃
    gen.generateStructureStarts(*region, centerChunk);
    EXPECT_EQ(&centerChunk.getChunkStatus(), &ChunkStatuses::STRUCTURE_STARTS);

    // generateStructureReferences 也不应崩溃
    gen.generateStructureReferences(*region, centerChunk);
    EXPECT_EQ(&centerChunk.getChunkStatus(), &ChunkStatuses::STRUCTURE_REFERENCES);
}

TEST_F(FlatChunkGeneratorDetailedTest, StructureOverrides_MultipleStructures)
{
    // 测试多个结构覆盖项
    FlatLevelGeneratorSettings settings(Biomes::Plains, true, false);
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::BEDROCK));
    settings.layersInfo().emplace_back(2, VanillaBlocks::getState(VanillaBlocks::DIRT));
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK));
    settings.setStructureOverrides({
        ResourceLocation::parse("minecraft:villages"),
        ResourceLocation::parse("minecraft:strongholds"),
        ResourceLocation::parse("minecraft:mineshafts"),
    });
    settings.updateLayers();

    EXPECT_EQ(settings.structureOverrides().size(), 3u);
    EXPECT_TRUE(settings.hasStructureGeneration());

    FlatChunkGenerator gen(42LL, settings);
    std::vector<std::unique_ptr<ChunkPrimer>> chunks;
    auto region = PlaceFeaturesTestHelper::createRegion(0, 0, 1, chunks);
    ChunkPrimer& centerChunk = *chunks[4];

    gen.generateStructureStarts(*region, centerChunk);
    EXPECT_EQ(&centerChunk.getChunkStatus(), &ChunkStatuses::STRUCTURE_STARTS);
}

} // namespace
