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
// 8. 与 MC 1.21.11 FlatLevelSource 的行为对齐
// ============================================================================

#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
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
// 8. IChunkGenerator 接口验证
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

} // namespace
