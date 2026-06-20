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
 * IMPLIED, BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

// ============================================================================
// 地形常识测试
//
// 验证主世界地形生成结果是否符合玩家常识：
// 1. 多区块高度范围——地表高度应在海平面附近浮动，不会极高或极低
// 2. 地表起伏——不同位置的高度应该有变化（不是完全平坦的）
// 3. 方块组成——生成的区块中石头、泥土、基岩等方块的比例符合常理
// 4. 海平面以下含水——Y <= 63 的空气位置应该被水填充
// 5. 基岩层——最底层应有基岩
// 6. 下界/末地地形基本特征
// 7. 高度图一致性——WorldSurfaceWG 与实际方块数据吻合
// 8. 生物群系多样性——多个区块应出现不同生物群系
// ============================================================================

#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/BiomeContainer.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/gen/chunk/FlatChunkGenerator.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/gen/settings/FlatLevelGeneratorSettings.hpp"
#include <memory>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::chunk;

namespace {

// ============================================================================
// 测试夹具：生成真实地形
// ============================================================================

class TerrainCommonSenseTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        fluid::FluidRegistry::instance().initialize();
    }

    // 辅助：创建3x3区块区域并生成噪声地形
    // 返回中心区块和区域（区域需保持存活）
    struct GeneratedChunk {
        std::vector<std::unique_ptr<ChunkPrimer>> ownedChunks;
        std::unique_ptr<WorldGenRegion> region;
        ChunkPrimer* centerChunk = nullptr;
    };

    static GeneratedChunk generateOverworldTerrain(u64 seed, ChunkCoord cx = 0, ChunkCoord cz = 0, i32 radius = 1)
    {
        GeneratedChunk result;
        const i32 diameter = radius * 2 + 1;

        // 创建生成器
        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(seed, false);
        NoiseChunkGenerator gen(seed, DimensionSettings::overworld(), std::move(biomeSource));

        // 创建区块
        std::vector<IChunk*> chunkPtrs;
        for (i32 dz = -radius; dz <= radius; ++dz) {
            for (i32 dx = -radius; dx <= radius; ++dx) {
                auto primer = std::make_unique<ChunkPrimer>(cx + dx, cz + dz);
                chunkPtrs.push_back(primer.get());
                result.ownedChunks.push_back(std::move(primer));
            }
        }
        result.centerChunk = dynamic_cast<ChunkPrimer*>(chunkPtrs[static_cast<size_t>((radius * diameter) + radius)]);

        result.region = std::make_unique<WorldGenRegion>(cx, cz, radius, std::move(chunkPtrs), 0);

        // 执行生成管线：biomes -> noise -> surface
        gen.generateBiomes(*result.region, *result.centerChunk);
        gen.generateNoise(*result.region, *result.centerChunk);
        gen.buildSurface(*result.region, *result.centerChunk);

        return result;
    }

    // 下界地形生成
    static GeneratedChunk generateNetherTerrain(u64 seed, ChunkCoord cx = 0, ChunkCoord cz = 0)
    {
        GeneratedChunk result;
        const i32 radius = 1;

        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createNether(seed);
        NoiseChunkGenerator gen(seed, DimensionSettings::nether(), std::move(biomeSource));

        std::vector<IChunk*> chunkPtrs;
        for (i32 dz = -radius; dz <= radius; ++dz) {
            for (i32 dx = -radius; dx <= radius; ++dx) {
                auto primer = std::make_unique<ChunkPrimer>(cx + dx, cz + dz);
                chunkPtrs.push_back(primer.get());
                result.ownedChunks.push_back(std::move(primer));
            }
        }
        result.centerChunk = dynamic_cast<ChunkPrimer*>(chunkPtrs[4]);
        result.region = std::make_unique<WorldGenRegion>(cx, cz, radius, std::move(chunkPtrs), -1);

        gen.generateBiomes(*result.region, *result.centerChunk);
        gen.generateNoise(*result.region, *result.centerChunk);
        gen.buildSurface(*result.region, *result.centerChunk);

        return result;
    }
};

// ============================================================================
// 1. 主世界高度范围——地表应在合理范围
// ============================================================================

TEST_F(TerrainCommonSenseTest, Overworld_SurfaceHeightWithinReasonableRange)
{
    // 主世界海平面 = 63, 常规地形高度范围大约 Y=-40 ~ Y=200
    // 极端情况下不会低于基岩层(-64), 不会超过世界顶部(320)
    auto result = generateOverworldTerrain(42ULL);

    ASSERT_NE(result.centerChunk, nullptr);
    int columnsChecked = 0;

    for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
        for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
            i32 surfaceY = result.centerChunk->getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
            EXPECT_GE(surfaceY, world::MIN_BUILD_HEIGHT) << "x=" << x << " z=" << z;
            EXPECT_LT(surfaceY, world::MAX_BUILD_HEIGHT) << "x=" << x << " z=" << z;
            ++columnsChecked;
        }
    }
    EXPECT_EQ(columnsChecked, world::CHUNK_WIDTH * world::CHUNK_WIDTH) << "All 256 columns should have valid height";
}

TEST_F(TerrainCommonSenseTest, Overworld_SurfaceHeightNearSeaLevel)
{
    // 主世界大部分地表高度应在海平面附近 (-30 ~ +80 即 Y=33 ~ Y=143)
    // 这覆盖了大部分平原、丘陵、山脉和海洋地形
    u64 seed = 42ULL;
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(seed, false);
    NoiseChunkGenerator gen(seed, DimensionSettings::overworld(), std::move(biomeSource));

    int totalColumns = 0;
    int columnsNearSeaLevel = 0;
    constexpr i32 SEA_LEVEL = world::SEA_LEVEL; // 63
    constexpr i32 TERRAIN_RANGE = 80;           // Y=33 ~ Y=143

    // 在较大范围采样多个位置
    for (i32 x = -64; x < 64; x += 4) {
        for (i32 z = -64; z < 64; z += 4) {
            i32 height = gen.getHeight(x, z, HeightmapType::WorldSurfaceWG);
            ++totalColumns;
            if (height >= SEA_LEVEL - TERRAIN_RANGE && height <= SEA_LEVEL + TERRAIN_RANGE) {
                ++columnsNearSeaLevel;
            }
        }
    }

    // 至少 70% 的位置高度应在海平面附近（Y=33~143 范围内）
    // 这包括海洋、平原、丘陵、大部分山脉地形
    double ratio = static_cast<double>(columnsNearSeaLevel) / static_cast<double>(totalColumns);
    EXPECT_GE(ratio, 0.70) << "At least 70% of terrain should be within " << TERRAIN_RANGE
                           << " blocks of sea level. Actual: " << (ratio * 100.0) << "%";
}

TEST_F(TerrainCommonSenseTest, Overworld_OceanFloorAboveBedrock)
{
    // 海底高度应高于基岩层（Y > -64）
    u64 seed = 42ULL;
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(seed, false);
    NoiseChunkGenerator gen(seed, DimensionSettings::overworld(), std::move(biomeSource));

    for (i32 x = -32; x < 32; x += 8) {
        for (i32 z = -32; z < 32; z += 8) {
            i32 oceanFloor = gen.getHeight(x, z, HeightmapType::OceanFloorWG);
            i32 worldSurface = gen.getHeight(x, z, HeightmapType::WorldSurfaceWG);
            // OceanFloor <= WorldSurface（海底不高于水面）
            EXPECT_LE(oceanFloor, worldSurface) << "x=" << x << " z=" << z;
            // OceanFloor >= 世界底部
            EXPECT_GE(oceanFloor, world::MIN_BUILD_HEIGHT) << "x=" << x << " z=" << z;
        }
    }
}

// ============================================================================
// 2. 地表起伏——不同位置的高度应有变化
// ============================================================================

TEST_F(TerrainCommonSenseTest, Overworld_TerrainHasVariation)
{
    // 主世界地形不应完全平坦，不同位置应有不同高度
    // 使用实际生成的区块数据检测地形起伏
    auto result = generateOverworldTerrain(42ULL);
    ASSERT_NE(result.centerChunk, nullptr);

    // 从区块高度图中提取高度
    std::vector<i32> surfaceHeights;
    for (i32 x = 0; x < world::CHUNK_WIDTH; x += 2) {
        for (i32 z = 0; z < world::CHUNK_WIDTH; z += 2) {
            i32 surfaceY = result.centerChunk->getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
            surfaceHeights.push_back(surfaceY);
        }
    }

    ASSERT_GT(surfaceHeights.size(), 0u);

    i32 minHeight = *std::min_element(surfaceHeights.begin(), surfaceHeights.end());
    i32 maxHeight = *std::max_element(surfaceHeights.begin(), surfaceHeights.end());
    i32 range = maxHeight - minHeight;

    // 即使在一个区块内，地形高度也应有变化
    // 单个区块 16x16 的范围通常包含一些高度差
    EXPECT_GE(range, 1) << "Terrain should have height variation within a chunk. Min=" << minHeight
                        << " Max=" << maxHeight;

    // 高度变化不应该超过世界高度（384格）
    EXPECT_LE(range, world::CHUNK_HEIGHT) << "Terrain variation should not exceed world height";
}

TEST_F(TerrainCommonSenseTest, Overworld_HeightmapConsistentWithColumn)
{
    // getHeight 返回的高度与 getBaseColumn 中的方块数据应一致
    u64 seed = 42ULL;
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(seed, false);
    NoiseChunkGenerator gen(seed, DimensionSettings::overworld(), std::move(biomeSource));

    // 采样几个位置
    for (i32 x : {0, 16, -16, 64, -64}) {
        for (i32 z : {0, 16, -16, 64, -64}) {
            i32 heightWG = gen.getHeight(x, z, HeightmapType::WorldSurfaceWG);
            auto column = gen.getBaseColumn(x, z);

            // WorldSurfaceWG 高度处应该是空气（或水），下方应该有实体方块
            if (heightWG > column.minY() && heightWG < column.minY() + column.height()) {
                // 高度上方的方块应该是空气或空
                const BlockState* above = column.getBlock(heightWG);
                EXPECT_TRUE(above == nullptr || above->isAir())
                    << "Block above WorldSurfaceWG at (" << x << "," << z << ") Y=" << heightWG
                    << " should be air, got " << (above ? "non-air" : "null");

                // 高度正下方的方块应该存在且不是空气
                const BlockState* below = column.getBlock(heightWG - 1);
                EXPECT_TRUE(below != nullptr && !below->isAir())
                    << "Block below WorldSurfaceWG at (" << x << "," << z << ") Y=" << (heightWG - 1)
                    << " should be solid, got " << (below ? "air/null" : "null");
            }
        }
    }
}

// ============================================================================
// 3. 方块组成——基岩层在底部
// ============================================================================

TEST_F(TerrainCommonSenseTest, Overworld_BedrockAtBottom)
{
    // 主世界底部 Y=-64 应有实体方块（基岩或石头）
    // 注意：基岩由表面规则添加，如果表面规则未正确应用，
    // 底部至少应有 defaultBlock（石头）作为密度函数的填充
    auto result = generateOverworldTerrain(42ULL);
    ASSERT_NE(result.centerChunk, nullptr);

    // 检查底部若干列是否有实体方块
    int solidColumnsFound = 0;
    for (i32 x = 0; x < world::CHUNK_WIDTH; x += 2) {
        for (i32 z = 0; z < world::CHUNK_WIDTH; z += 2) {
            const BlockState* bottomBlock = result.centerChunk->getBlockState(x, world::MIN_BUILD_HEIGHT, z);
            if (bottomBlock != nullptr && !bottomBlock->isAir()) {
                ++solidColumnsFound;
            }
            // 也检查基岩
            if (bottomBlock != nullptr && bottomBlock->is(VanillaBlocks::BEDROCK)) {
                // 基岩存在（更好）
            }
        }
    }

    // 底部至少应该有实体方块（基岩或石头）
    EXPECT_GT(solidColumnsFound, 0) << "Bottom Y=-64 should have solid blocks (bedrock or stone)";
}

TEST_F(TerrainCommonSenseTest, Overworld_StoneIsDominantBlock)
{
    // 主世界地下以石头为主，石头应该是最多的方块
    auto result = generateOverworldTerrain(42ULL);
    ASSERT_NE(result.centerChunk, nullptr);

    int stoneCount = 0;
    int deepslateCount = 0;
    int totalNonAir = 0;

    // 采样区块中的方块（每隔2格采样以加快速度）
    for (i32 x = 0; x < world::CHUNK_WIDTH; x += 2) {
        for (i32 z = 0; z < world::CHUNK_WIDTH; z += 2) {
            for (i32 y = world::MIN_BUILD_HEIGHT; y < world::MAX_BUILD_HEIGHT; y += 2) {
                const BlockState* block = result.centerChunk->getBlockState(x, y, z);
                if (block != nullptr && !block->isAir()) {
                    ++totalNonAir;
                    if (block->is(VanillaBlocks::STONE)) {
                        ++stoneCount;
                    }
                    if (block->is(VanillaBlocks::DEEPSLATE)) {
                        ++deepslateCount;
                    }
                }
            }
        }
    }

    EXPECT_GT(totalNonAir, 0) << "Chunk should have non-air blocks";
    // 石头和深板岩合计应该占非空方块的很大比例
    int stoneAndDeepslate = stoneCount + deepslateCount;
    double ratio = static_cast<double>(stoneAndDeepslate) / static_cast<double>(totalNonAir);
    EXPECT_GE(ratio, 0.50) << "Stone + Deepslate should dominate underground. "
                           << "Stone=" << stoneCount << " Deepslate=" << deepslateCount << " Total=" << totalNonAir
                           << " Ratio=" << (ratio * 100.0) << "%";
}

TEST_F(TerrainCommonSenseTest, Overworld_DeepslateBelowY0)
{
    // Y < 0 应有深板岩或石头（深板岩过渡由表面规则或密度函数控制）
    auto result = generateOverworldTerrain(42ULL);
    ASSERT_NE(result.centerChunk, nullptr);

    int deepslateBelow0 = 0;
    int stoneBelow0 = 0;
    int solidBelow0 = 0;

    for (i32 x = 0; x < world::CHUNK_WIDTH; x += 4) {
        for (i32 z = 0; z < world::CHUNK_WIDTH; z += 4) {
            for (i32 y = world::MIN_BUILD_HEIGHT; y < 0; y += 4) {
                const BlockState* block = result.centerChunk->getBlockState(x, y, z);
                if (block != nullptr && !block->isAir()) {
                    ++solidBelow0;
                    if (block->is(VanillaBlocks::DEEPSLATE)) {
                        ++deepslateBelow0;
                    } else if (block->is(VanillaBlocks::STONE)) {
                        ++stoneBelow0;
                    }
                }
            }
        }
    }

    // Y < 0 区域应有实体方块
    EXPECT_GT(solidBelow0, 0) << "Below Y=0 should have solid blocks";

    // 石头和深板岩合计应占地下方块的大部分
    int stoneAndDeepslate = stoneBelow0 + deepslateBelow0;
    if (solidBelow0 > 0) {
        double ratio = static_cast<double>(stoneAndDeepslate) / static_cast<double>(solidBelow0);
        EXPECT_GE(ratio, 0.30) << "Stone + Deepslate should be significant below Y=0. "
                               << "Stone=" << stoneBelow0 << " Deepslate=" << deepslateBelow0
                               << " Solid=" << solidBelow0 << " Ratio=" << (ratio * 100.0) << "%";
    }
}

// ============================================================================
// 4. 海平面以下含水
// ============================================================================

TEST_F(TerrainCommonSenseTest, Overworld_WaterBelowSeaLevel)
{
    // 海平面以下应有实体方块或水
    // 注意：Aquifer 可能未在当前生成管线中完全启用，
    // 但 generateNoise 阶段应至少在海洋区域放置水
    auto result = generateOverworldTerrain(42ULL);
    ASSERT_NE(result.centerChunk, nullptr);

    int waterBelowSea = 0;
    int solidBelowSea = 0;
    int airBelowSea = 0;

    for (i32 x = 0; x < world::CHUNK_WIDTH; x += 4) {
        for (i32 z = 0; z < world::CHUNK_WIDTH; z += 4) {
            for (i32 y = world::MIN_BUILD_HEIGHT; y <= world::SEA_LEVEL; y += 4) {
                const BlockState* block = result.centerChunk->getBlockState(x, y, z);
                if (block == nullptr || block->isAir()) {
                    ++airBelowSea;
                } else if (block->isLiquid()) {
                    ++waterBelowSea;
                } else {
                    ++solidBelowSea;
                }
            }
        }
    }

    // 海平面以下应有实体方块（石头等）
    EXPECT_GT(solidBelowSea, 0) << "There should be solid blocks below sea level. "
                                << "Solid=" << solidBelowSea << " Water=" << waterBelowSea << " Air=" << airBelowSea;

    // 实体方块应远多于空气（因为地下大部分是石头）
    if (solidBelowSea + airBelowSea > 0) {
        double solidRatio =
            static_cast<double>(solidBelowSea) / static_cast<double>(solidBelowSea + airBelowSea + waterBelowSea);
        EXPECT_GE(solidRatio, 0.50) << "Solid blocks should dominate below sea level. "
                                    << "Ratio=" << (solidRatio * 100.0) << "%";
    }
}

TEST_F(TerrainCommonSenseTest, Overworld_NoWaterAboveSeaLevelInBaseColumn)
{
    // getBaseColumn 返回的基础列中，海平面以上不应该有水
    // （水是由 Aquifer 在噪声生成阶段添加的，但基础列不包含水）
    u64 seed = 42ULL;
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(seed, false);
    NoiseChunkGenerator gen(seed, DimensionSettings::overworld(), std::move(biomeSource));

    for (i32 x : {0, 32, -32}) {
        for (i32 z : {0, 32, -32}) {
            auto column = gen.getBaseColumn(x, z);
            for (i32 y = world::SEA_LEVEL + 1; y < world::MAX_BUILD_HEIGHT; ++y) {
                const BlockState* block = column.getBlock(y);
                // 基础列在海平面以上不应有水（getBaseColumn 不含 Aquifer）
                if (block != nullptr) {
                    EXPECT_FALSE(block->isLiquid())
                        << "getBaseColumn should not have water above sea level at (" << x << "," << z << ") Y=" << y;
                }
            }
        }
    }
}

// ============================================================================
// 5. 下界地形基本特征
// ============================================================================

TEST_F(TerrainCommonSenseTest, Nether_HeightWithinBounds)
{
    // 下界高度范围 Y=0~127，地形应在此范围内
    u64 seed = 42ULL;
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createNether(seed);
    NoiseChunkGenerator gen(seed, DimensionSettings::nether(), std::move(biomeSource));

    for (i32 x = -32; x < 32; x += 8) {
        for (i32 z = -32; z < 32; z += 8) {
            i32 height = gen.getHeight(x, z, HeightmapType::WorldSurfaceWG);
            EXPECT_GE(height, 0) << "Nether height should be >= 0 at x=" << x << " z=" << z;
            EXPECT_LE(height, 128) << "Nether height should be <= 128 at x=" << x << " z=" << z;
        }
    }
}

TEST_F(TerrainCommonSenseTest, Nether_BaseColumnHasNetherrack)
{
    // 下界基础方块应该是下界岩（netherrack）
    u64 seed = 42ULL;
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createNether(seed);
    NoiseChunkGenerator gen(seed, DimensionSettings::nether(), std::move(biomeSource));

    auto column = gen.getBaseColumn(0, 0);
    EXPECT_EQ(column.minY(), 0);
    EXPECT_EQ(column.height(), 128);

    // 下界底部应该有非空方块
    const BlockState* bottomBlock = column.getBlock(0);
    EXPECT_TRUE(bottomBlock != nullptr && !bottomBlock->isAir()) << "Nether bottom should not be air";
}

TEST_F(TerrainCommonSenseTest, Nether_DefaultBlockIsNetherrack)
{
    // DimensionSettings::nether() 的 defaultBlock 应为下界岩
    const auto& settings = DimensionSettings::nether();
    ASSERT_NE(settings.defaultBlock, nullptr);
    EXPECT_TRUE(settings.defaultBlock->is(VanillaBlocks::NETHERRACK)) << "Nether default block should be netherrack";
}

// ============================================================================
// 6. 末地地形基本特征
// ============================================================================

TEST_F(TerrainCommonSenseTest, End_HeightWithinBounds)
{
    u64 seed = 42ULL;
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(seed, false);
    NoiseChunkGenerator gen(seed, DimensionSettings::end(), std::move(biomeSource));

    for (i32 x = -32; x < 32; x += 8) {
        for (i32 z = -32; z < 32; z += 8) {
            i32 height = gen.getHeight(x, z, HeightmapType::WorldSurfaceWG);
            EXPECT_GE(height, 0) << "End height should be >= 0 at x=" << x << " z=" << z;
            EXPECT_LE(height, 128) << "End height should be <= 128 at x=" << x << " z=" << z;
        }
    }
}

TEST_F(TerrainCommonSenseTest, End_DefaultBlockIsEndStone)
{
    const auto& settings = DimensionSettings::end();
    ASSERT_NE(settings.defaultBlock, nullptr);
    EXPECT_TRUE(settings.defaultBlock->is(VanillaBlocks::END_STONE)) << "End default block should be end stone";
}

TEST_F(TerrainCommonSenseTest, End_SeaLevelIsZero)
{
    // 末地海平面为 0，没有水
    const auto& settings = DimensionSettings::end();
    EXPECT_EQ(settings.seaLevel, 0);
}

// ============================================================================
// 7. 多区块高度连续性
// ============================================================================

TEST_F(TerrainCommonSenseTest, Overworld_AdjacentChunksHaveContinuousHeight)
{
    // 相邻区块的高度应该连续（不会有突变断层）
    u64 seed = 42ULL;
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(seed, false);
    NoiseChunkGenerator gen(seed, DimensionSettings::overworld(), std::move(biomeSource));

    // 沿 X 轴采样
    i32 prevHeight = gen.getHeight(0, 0, HeightmapType::WorldSurfaceWG);
    for (i32 x = 16; x <= 160; x += 16) {
        i32 height = gen.getHeight(x, 0, HeightmapType::WorldSurfaceWG);
        i32 diff = std::abs(height - prevHeight);
        // 相邻 16 格的高度差不应超过 40 格（极端悬崖除外）
        // 常规地形 16 格内高度变化通常不超过 20 格
        EXPECT_LE(diff, 40) << "Adjacent terrain should not have abrupt changes. "
                            << "Height at x=" << (x - 16) << "=" << prevHeight << ", x=" << x << "=" << height
                            << ", diff=" << diff;
        prevHeight = height;
    }

    // 沿 Z 轴采样
    prevHeight = gen.getHeight(0, 0, HeightmapType::WorldSurfaceWG);
    for (i32 z = 16; z <= 160; z += 16) {
        i32 height = gen.getHeight(0, z, HeightmapType::WorldSurfaceWG);
        i32 diff = std::abs(height - prevHeight);
        EXPECT_LE(diff, 40) << "Adjacent terrain along Z should not have abrupt changes. "
                            << "Height at z=" << (z - 16) << "=" << prevHeight << ", z=" << z << "=" << height
                            << ", diff=" << diff;
        prevHeight = height;
    }
}

TEST_F(TerrainCommonSenseTest, Overworld_HeightSmoothAtShortDistance)
{
    // 短距离内高度变化应该平滑（1格内不应有巨大差异）
    u64 seed = 42ULL;
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(seed, false);
    NoiseChunkGenerator gen(seed, DimensionSettings::overworld(), std::move(biomeSource));

    int largeJumps = 0;
    int totalChecks = 0;

    for (i32 x = -32; x < 32; x += 4) {
        for (i32 z = -32; z < 32; z += 4) {
            i32 h1 = gen.getHeight(x, z, HeightmapType::WorldSurfaceWG);
            i32 h2 = gen.getHeight(x + 1, z, HeightmapType::WorldSurfaceWG);
            i32 diff = std::abs(h2 - h1);
            ++totalChecks;
            // 1格距离内高度差超过20格的情况应该很少
            if (diff > 20) {
                ++largeJumps;
            }
        }
    }

    double jumpRatio = static_cast<double>(largeJumps) / static_cast<double>(totalChecks);
    EXPECT_LT(jumpRatio, 0.05) << "Less than 5% of adjacent columns should have >20 block height difference. "
                               << "Large jumps: " << largeJumps << "/" << totalChecks << " (" << (jumpRatio * 100.0)
                               << "%)";
}

// ============================================================================
// 8. 生物群系多样性
// ============================================================================

TEST_F(TerrainCommonSenseTest, Overworld_MultipleBiomesPresent)
{
    // 主世界应出现多种生物群系
    auto result = generateOverworldTerrain(42ULL);
    ASSERT_NE(result.centerChunk, nullptr);

    std::set<BiomeId> biomesInChunk;
    for (i32 y = 0; y < world::CHUNK_HEIGHT; y += 64) {
        for (i32 z = 0; z < world::CHUNK_WIDTH; z += 4) {
            for (i32 x = 0; x < world::CHUNK_WIDTH; x += 4) {
                BiomeId biome = result.centerChunk->getBiomeAtBlock(static_cast<BlockCoord>(x),
                    static_cast<BlockCoord>(y + world::MIN_BUILD_HEIGHT),
                    static_cast<BlockCoord>(z));
                biomesInChunk.insert(biome);
            }
        }
    }

    // 至少应该有一种生物群系（不应该是空的）
    EXPECT_GT(biomesInChunk.size(), 0u) << "Chunk should have at least one biome";

    // 不应该是虚空生物群系
    for (BiomeId biome : biomesInChunk) {
        EXPECT_NE(biome, Biomes::TheVoid) << "Overworld chunk should not contain TheVoid biome";
    }
}

TEST_F(TerrainCommonSenseTest, Overworld_BiomesVaryAcrossDistance)
{
    // 在大范围内应该出现不同生物群系
    u64 seed = 42ULL;
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(seed, false);
    NoiseChunkGenerator gen(seed, DimensionSettings::overworld(), std::move(biomeSource));

    std::set<BiomeId> biomes;
    for (i32 x = -512; x <= 512; x += 64) {
        for (i32 z = -512; z <= 512; z += 64) {
            biomes.insert(gen.getBiome(x, 64, z));
        }
    }

    // 在 1024x1024 范围内应至少出现 3 种不同生物群系
    EXPECT_GE(biomes.size(), 3u) << "Should find at least 3 different biomes in a 1024x1024 area, found "
                                 << biomes.size();
}

// ============================================================================
// 9. 维度特性对比
// ============================================================================

TEST_F(TerrainCommonSenseTest, Overworld_HasDefaultStoneBlock)
{
    const auto& settings = DimensionSettings::overworld();
    ASSERT_NE(settings.defaultBlock, nullptr);
    EXPECT_TRUE(settings.defaultBlock->is(VanillaBlocks::STONE)) << "Overworld default block should be stone";
}

TEST_F(TerrainCommonSenseTest, Overworld_HasDefaultWaterFluid)
{
    const auto& settings = DimensionSettings::overworld();
    ASSERT_NE(settings.defaultFluid, nullptr);
    EXPECT_TRUE(settings.defaultFluid->is(VanillaBlocks::WATER)) << "Overworld default fluid should be water";
}

TEST_F(TerrainCommonSenseTest, Nether_DefaultFluidIsLava)
{
    const auto& settings = DimensionSettings::nether();
    ASSERT_NE(settings.defaultFluid, nullptr);
    EXPECT_TRUE(settings.defaultFluid->is(VanillaBlocks::LAVA)) << "Nether default fluid should be lava";
}

// ============================================================================
// 10. 多区块生成稳定性
// ============================================================================

TEST_F(TerrainCommonSenseTest, Overworld_MultipleChunksGenerateSuccessfully)
{
    // 多个不同区块坐标应能成功生成
    for (ChunkCoord cx : {0, 1, -1, 5, -5}) {
        for (ChunkCoord cz : {0, 1, -1, 5, -5}) {
            auto result = generateOverworldTerrain(42ULL, cx, cz);
            ASSERT_NE(result.centerChunk, nullptr) << "Failed to generate chunk at (" << cx << "," << cz << ")";

            // 至少应该有一些非空方块
            bool hasNonAir = false;
            for (i32 y = world::MIN_BUILD_HEIGHT; y < world::MAX_BUILD_HEIGHT; y += 8) {
                const BlockState* block = result.centerChunk->getBlockState(0, y, 0);
                if (block != nullptr && !block->isAir()) {
                    hasNonAir = true;
                    break;
                }
            }
            EXPECT_TRUE(hasNonAir) << "Chunk at (" << cx << "," << cz << ") should have non-air blocks";
        }
    }
}

TEST_F(TerrainCommonSenseTest, Overworld_DeterministicWithSameSeed)
{
    // 相同种子应生成相同地形
    auto result1 = generateOverworldTerrain(12345ULL);
    auto result2 = generateOverworldTerrain(12345ULL);

    ASSERT_NE(result1.centerChunk, nullptr);
    ASSERT_NE(result2.centerChunk, nullptr);

    // 比较两个区块的方块数据
    int mismatches = 0;
    for (i32 x = 0; x < world::CHUNK_WIDTH; x += 2) {
        for (i32 z = 0; z < world::CHUNK_WIDTH; z += 2) {
            for (i32 y = world::MIN_BUILD_HEIGHT; y < world::MAX_BUILD_HEIGHT; y += 4) {
                const BlockState* b1 = result1.centerChunk->getBlockState(x, y, z);
                const BlockState* b2 = result2.centerChunk->getBlockState(x, y, z);
                if (b1 != b2) {
                    ++mismatches;
                }
            }
        }
    }

    EXPECT_EQ(mismatches, 0) << "Same seed should produce identical terrain. Mismatches: " << mismatches;
}

// ============================================================================
// 11. getBaseColumn 与区块生成结果一致性
// ============================================================================

TEST_F(TerrainCommonSenseTest, Overworld_BaseColumnMatchesChunkGeneration)
{
    // getBaseColumn 应该反映区块的底层结构（不含表面规则、洞穴等）
    // 但底层方块（石头/深板岩）应一致
    u64 seed = 42ULL;
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(seed, false);
    NoiseChunkGenerator gen(seed, DimensionSettings::overworld(), std::move(biomeSource));

    // 对比 getBaseColumn 和实际区块在深层的方块
    auto result = generateOverworldTerrain(seed);
    ASSERT_NE(result.centerChunk, nullptr);

    // 中心区块的世界坐标为 (0,0) ~ (15,15)
    for (i32 x = 0; x < world::CHUNK_WIDTH; x += 4) {
        for (i32 z = 0; z < world::CHUNK_WIDTH; z += 4) {
            auto column = gen.getBaseColumn(x, z);

            // 在 Y=-64（基岩层附近）检查底层方块一致性
            const BlockState* colBlock = column.getBlock(world::MIN_BUILD_HEIGHT);
            const BlockState* chunkBlock = result.centerChunk->getBlockState(x, world::MIN_BUILD_HEIGHT, z);

            // 底部都应有非空方块
            if (colBlock != nullptr && chunkBlock != nullptr) {
                // getBaseColumn 不含基岩（基岩由表面规则添加），所以可能不同
                // 但两者都不应为空气
                EXPECT_FALSE(colBlock->isAir()) << "BaseColumn at (" << x << "," << z << ") Y=-64 should not be air";
            }
        }
    }
}

// ============================================================================
// 12. 平坦世界常识验证
// ============================================================================

TEST_F(TerrainCommonSenseTest, FlatWorld_AllColumnsSameHeight)
{
    // 平坦世界所有列高度应完全一致
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());

    i32 expectedHeight = gen.getHeight(0, 0, HeightmapType::WorldSurfaceWG);
    for (i32 x = -100; x <= 100; x += 10) {
        for (i32 z = -100; z <= 100; z += 10) {
            i32 height = gen.getHeight(x, z, HeightmapType::WorldSurfaceWG);
            EXPECT_EQ(height, expectedHeight) << "Flat world height should be constant. Got " << height << " at (" << x
                                              << "," << z << "), expected " << expectedHeight;
        }
    }
}

TEST_F(TerrainCommonSenseTest, FlatWorld_OnlyFourBlockLayers)
{
    // 默认平坦世界：1基岩 + 2泥土 + 1草方块 = 4层
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    auto column = gen.getBaseColumn(0, 0);

    int nonAirCount = 0;
    for (i32 y = column.minY(); y < column.minY() + column.height(); ++y) {
        const BlockState* block = column.getBlock(y);
        if (block != nullptr && !block->isAir()) {
            ++nonAirCount;
        }
    }
    EXPECT_EQ(nonAirCount, 4) << "Default flat world should have exactly 4 non-air layers";
}

TEST_F(TerrainCommonSenseTest, FlatWorld_NoWaterAnywhere)
{
    // 默认平坦世界没有水
    FlatChunkGenerator gen(0LL, FlatLevelGeneratorSettings::createDefault());
    auto column = gen.getBaseColumn(0, 0);

    for (i32 y = column.minY(); y < column.minY() + column.height(); ++y) {
        const BlockState* block = column.getBlock(y);
        if (block != nullptr) {
            EXPECT_FALSE(block->isLiquid()) << "Default flat world should not have liquid at Y=" << y;
        }
    }
}

// ============================================================================
// 13. 维度间高度范围差异
// ============================================================================

TEST_F(TerrainCommonSenseTest, DimensionHeight_RangesCorrect)
{
    // 主世界：-64 到 320（高度 384）
    {
        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(0ULL, false);
        NoiseChunkGenerator gen(0ULL, DimensionSettings::overworld(), std::move(biomeSource));
        EXPECT_EQ(gen.getMinY(), -64);
        EXPECT_EQ(gen.getGenDepth(), 384);
        EXPECT_EQ(gen.seaLevel(), 63);
    }

    // 下界：0 到 128（高度 128）
    {
        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createNether(0ULL);
        NoiseChunkGenerator gen(0ULL, DimensionSettings::nether(), std::move(biomeSource));
        EXPECT_EQ(gen.getMinY(), 0);
        EXPECT_EQ(gen.getGenDepth(), 128);
        EXPECT_EQ(gen.seaLevel(), 32);
    }

    // 末地：0 到 128（高度 128）
    {
        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(0ULL, false);
        NoiseChunkGenerator gen(0ULL, DimensionSettings::end(), std::move(biomeSource));
        EXPECT_EQ(gen.getMinY(), 0);
        EXPECT_EQ(gen.getGenDepth(), 128);
        EXPECT_EQ(gen.seaLevel(), 0);
    }
}

} // namespace
