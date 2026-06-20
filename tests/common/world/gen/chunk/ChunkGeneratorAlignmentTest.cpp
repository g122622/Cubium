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

#include "common/world/biome/BiomeIds.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/chunk/DebugChunkGenerator.hpp"
#include "common/world/gen/chunk/FlatChunkGenerator.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/gen/settings/FlatLevelGeneratorSettings.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace {

class ChunkGeneratorAlignmentTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }
};

// ============================================================================
// 2.1 getGenDepth / getMinY / getSeaLevel
// ============================================================================

TEST_F(ChunkGeneratorAlignmentTest, DebugChunkGenerator_GetGenDepth_Is384)
{
    DebugChunkGenerator gen;
    EXPECT_EQ(gen.getGenDepth(), 384);
}

TEST_F(ChunkGeneratorAlignmentTest, DebugChunkGenerator_GetMinY_Is0)
{
    DebugChunkGenerator gen;
    EXPECT_EQ(gen.getMinY(), 0);
}

TEST_F(ChunkGeneratorAlignmentTest, DebugChunkGenerator_GetBiomeSource_NotNull)
{
    DebugChunkGenerator gen;
    // MC 1.21: DebugLevelSource 返回 FixedBiomeSource(Plains)
    auto* source = gen.getBiomeSource();
    ASSERT_NE(source, nullptr);
    EXPECT_EQ(source->getNoiseBiome(0, 0, 0), Biomes::Plains);
}

TEST_F(ChunkGeneratorAlignmentTest, DebugChunkGenerator_GetBaseColumn_Empty)
{
    DebugChunkGenerator gen;
    auto column = gen.getBaseColumn(0, 0);
    // MC 1.21: DebugLevelSource.getBaseColumn() 返回空列
    EXPECT_EQ(column.height(), 0);
}

TEST_F(ChunkGeneratorAlignmentTest, DebugChunkGenerator_IsDebugGenerator)
{
    DebugChunkGenerator gen;
    EXPECT_TRUE(gen.isDebugGenerator());
}

// ============================================================================
// FlatChunkGenerator
// ============================================================================

class FlatChunkGeneratorAlignmentTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }
};

TEST_F(FlatChunkGeneratorAlignmentTest, DefaultSettings_LayerCount)
{
    auto settings = FlatLevelGeneratorSettings::createDefault();
    // MC 默认: 1x Bedrock + 2x Dirt + 1x Grass Block = 3 个 FlatLayerInfo, 4 个展开层
    EXPECT_EQ(settings.layersInfo().size(), 3u);
    EXPECT_EQ(settings.layers().size(), 4u);
}

TEST_F(FlatChunkGeneratorAlignmentTest, DefaultSettings_BiomeIsPlains)
{
    auto settings = FlatLevelGeneratorSettings::createDefault();
    EXPECT_EQ(settings.biomeId(), Biomes::Plains);
}

TEST_F(FlatChunkGeneratorAlignmentTest, FlatChunkGenerator_SeaLevel_IsMinus63)
{
    FlatChunkGenerator gen(12345LL, FlatLevelGeneratorSettings::createDefault());
    // MC 1.21: FlatLevelSource.getSeaLevel() = -63
    EXPECT_EQ(gen.seaLevel(), -63);
}

TEST_F(FlatChunkGeneratorAlignmentTest, FlatChunkGenerator_GetMinY_Is0)
{
    FlatChunkGenerator gen(12345LL, FlatLevelGeneratorSettings::createDefault());
    EXPECT_EQ(gen.getMinY(), 0);
}

TEST_F(FlatChunkGeneratorAlignmentTest, FlatChunkGenerator_GetGenDepth_Is384)
{
    FlatChunkGenerator gen(12345LL, FlatLevelGeneratorSettings::createDefault());
    EXPECT_EQ(gen.getGenDepth(), 384);
}

TEST_F(FlatChunkGeneratorAlignmentTest, FlatChunkGenerator_GetBiomeSource_NotNull)
{
    FlatChunkGenerator gen(12345LL, FlatLevelGeneratorSettings::createDefault());
    auto* source = gen.getBiomeSource();
    ASSERT_NE(source, nullptr);
    EXPECT_EQ(source->getNoiseBiome(0, 0, 0), Biomes::Plains);
}

TEST_F(FlatChunkGeneratorAlignmentTest, FlatChunkGenerator_GetHeight_WorldSurface)
{
    FlatChunkGenerator gen(12345LL, FlatLevelGeneratorSettings::createDefault());
    // 默认平坦世界: 基岩(0) + 泥土(1-2) + 草方块(3)
    // WorldSurfaceWG 高度 = 4 (草方块上方第一个空气方块)
    const i32 h = gen.getHeight(0, 0, HeightmapType::WorldSurfaceWG);
    EXPECT_EQ(h, 4);
}

TEST_F(FlatChunkGeneratorAlignmentTest, FlatChunkGenerator_GetBaseColumn_ReturnsColumn)
{
    FlatChunkGenerator gen(12345LL, FlatLevelGeneratorSettings::createDefault());
    auto column = gen.getBaseColumn(0, 0);
    EXPECT_EQ(column.minY(), 0);
    EXPECT_EQ(column.height(), 384);
    // 第0层应该是基岩
    const BlockState* bedrock = VanillaBlocks::getState(VanillaBlocks::BEDROCK);
    ASSERT_NE(bedrock, nullptr);
    EXPECT_EQ(column.getBlock(0), bedrock);
}

TEST_F(FlatChunkGeneratorAlignmentTest, FlatChunkGenerator_IsNotDebugGenerator)
{
    FlatChunkGenerator gen(12345LL, FlatLevelGeneratorSettings::createDefault());
    EXPECT_FALSE(gen.isDebugGenerator());
}

TEST_F(FlatChunkGeneratorAlignmentTest, FlatChunkGenerator_GetBiome_Plains)
{
    FlatChunkGenerator gen(12345LL, FlatLevelGeneratorSettings::createDefault());
    EXPECT_EQ(gen.getBiome(0, 0, 0), Biomes::Plains);
    EXPECT_EQ(gen.getNoiseBiome(0, 0, 0), Biomes::Plains);
}

// ============================================================================
// 2.6 Cell 大小来自配置
// ============================================================================

TEST_F(ChunkGeneratorAlignmentTest, OverworldDimensionSettings_CellSize)
{
    // MC 1.21: 主世界 cellWidth=4, cellHeight=8（来自 NoiseSettings）
    const auto& noise = DimensionSettings::overworld().noise;
    EXPECT_EQ(noise.sizeHorizontal * 4, 4); // cellWidth
    EXPECT_EQ(noise.sizeVertical * 4, 8);   // cellHeight
}

} // namespace
} // namespace mc
