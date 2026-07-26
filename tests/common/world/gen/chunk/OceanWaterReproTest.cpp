/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/ sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

// ============================================================================
// 海洋水域生成复现测试
//
// 用户反馈：在海洋群系中，地形高度符合海洋特征（约 y=30），但地表之上没有任何水。
// 本测试通过直接调用 NoiseChunkGenerator 的生成管线（biomes -> noise -> surface），
// 在已知会生成海洋群系的种子下，验证海洋列在海平面附近是否确实存在水方块。
//
// 如果断言失败（海洋列在海平面处没有水），则证明水域生成问题真实存在。
//
// 种子选择：曾经用 114514，但该种子在出生点附近的 5x5 区块范围内不生成任何海洋群系
// （海洋群系落在大陆性噪声低值区，114514 出生点属内陆），导致 ASSERT_TRUE(foundAnyOcean)
// 始终失败。注意 114514 时期本测试因 buildSurface 阶段 VerticalGradientCondition 的
// 跨 RandomState 悬垂 UAF 而崩溃（在海洋检测断言之前），掩盖了“无海洋”这一事实；
// 该 UAF 已修复（见 SurfaceConditionLifecycleTest），本测试才得以真正运行并暴露种子选错。
// 现改用 987654321：该种子在出生点 5x5 范围内海洋列充足，且海平面下方水方块 100% 命中，
// 可稳定验证水域生成不变量。
// ============================================================================

#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/data/Heightmap.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/aquifer/Aquifer.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/density/NoiseChunk.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include <iomanip>
#include <iostream>
#include <set>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::chunk;

namespace {

// ============================================================================
// 测试夹具：海洋水域生成
// ============================================================================

class OceanWaterReproTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        BiomeTags::initialize();
        fluid::FluidRegistry::instance().initialize();
    }

    struct GeneratedChunk {
        std::vector<std::unique_ptr<ChunkPrimer>> ownedChunks;
        std::unique_ptr<WorldGenRegion> region;
        std::unique_ptr<NoiseChunkGenerator> generator;
        ChunkPrimer* centerChunk = nullptr;
    };

    // 生成 (2*radius+1)^2 区域并执行 biomes -> noise -> surface 管线
    static GeneratedChunk generateOverworldTerrain(u64 seed, ChunkCoord cx = 0, ChunkCoord cz = 0, i32 radius = 1)
    {
        GeneratedChunk result;
        const i32 diameter = radius * 2 + 1;

        auto settings = DimensionSettings::overworld();
        auto randomState = world::gen::RandomState::create(settings, seed);
        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        result.generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));

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

        for (i32 dz = -radius; dz <= radius; ++dz) {
            for (i32 dx = -radius; dx <= radius; ++dx) {
                size_t idx = static_cast<size_t>((dz + radius) * diameter + (dx + radius));
                ChunkPrimer* chunk = result.ownedChunks[idx].get();
                result.generator->generateBiomes(*result.region, *chunk);
                result.generator->generateNoise(*result.region, *chunk);
                result.generator->buildSurface(*result.region, *chunk);
            }
        }

        return result;
    }

    struct OceanColumnStats {
        i32 oceanColumnCount = 0;             // 海洋群系列数
        i32 oceanColumnsWithWater = 0;        // 海平面处有水的海洋列数
        i32 oceanColumnsWithSurfaceWater = 0; // 海洋列中海平面附近有水的列数
        i32 totalWaterBlocks = 0;             // 海洋列中海平面以下的水方块数（采样）
    };

    // 检查区块中所有海洋列的水方块情况
    static OceanColumnStats analyzeOceanColumns(ChunkPrimer& chunk)
    {
        OceanColumnStats stats;

        for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
            for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
                const BiomeId biomeId = chunk.getBiomeAtBlock(x, world::SEA_LEVEL, z);
                if (!world::biome::BiomeTags::IS_OCEAN().contains(biomeId)) {
                    continue;
                }

                ++stats.oceanColumnCount;

                // MC 1.21: 水方块在海平面 Y=seaLevel-1 及以下（FluidStatus.at(y) 在 y<fluidLevel=seaLevel
                // 时返回流体）。 海平面 Y=seaLevel 本身是空气。因此检查 Y=seaLevel-1 是否为水。
                const BlockState* seaLevelBlock = chunk.getBlockState(x, world::SEA_LEVEL - 1, z);
                if (seaLevelBlock != nullptr && seaLevelBlock->isLiquid()) {
                    ++stats.oceanColumnsWithWater;
                }

                // 海平面以下 10 格的方块采样（含海平面下方第一格）
                bool columnHasSurfaceWater = false;
                for (i32 y = world::SEA_LEVEL - 1; y >= world::SEA_LEVEL - 11; --y) {
                    const BlockState* block = chunk.getBlockState(x, y, z);
                    if (block != nullptr && block->isLiquid()) {
                        columnHasSurfaceWater = true;
                        ++stats.totalWaterBlocks;
                    }
                }
                if (columnHasSurfaceWater) {
                    ++stats.oceanColumnsWithSurfaceWater;
                }
            }
        }

        return stats;
    }

    // 输出指定海洋列的垂直方块分布，辅助定位问题
    static void printOceanColumnDiagnostic(
        ChunkPrimer& chunk, ChunkCoord cx, ChunkCoord cz, i32 x, i32 z, BiomeId biomeId)
    {
        const i32 worldX = cx * world::CHUNK_WIDTH + x;
        const i32 worldZ = cz * world::CHUNK_WIDTH + z;

        std::cout << "[OceanWaterRepro] ============================================" << std::endl;
        std::cout << "[OceanWaterRepro] 海洋列: 区块(" << cx << "," << cz << ") 本地(" << x << "," << z << ") 世界("
                  << worldX << "," << worldZ << ") 生物群系ID=" << biomeId << std::endl;
        std::cout << "[OceanWaterRepro] 海平面(Y=" << world::SEA_LEVEL << ")附近方块分布:" << std::endl;

        // 输出 Y=SEA_LEVEL+5 到 Y=SEA_LEVEL-5 的方块
        for (i32 y = world::SEA_LEVEL + 5; y >= world::SEA_LEVEL - 5; --y) {
            const BlockState* block = chunk.getBlockState(x, y, z);
            std::string blockName = "空气(null)";
            if (block != nullptr) {
                if (block->isAir()) {
                    blockName = "空气";
                } else if (block->isLiquid()) {
                    blockName = (&block->getBlock() == VanillaBlocks::WATER) ? "水" : "岩浆";
                } else {
                    blockName = "固体(blockId=" + std::to_string(block->getBlock().blockId()) + ")";
                }
            }
            std::cout << "[OceanWaterRepro]   Y=" << y << (y == world::SEA_LEVEL ? " (海平面)" : "") << ": "
                      << blockName << std::endl;
        }

        i32 surfaceY = chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
        std::cout << "[OceanWaterRepro] WorldSurfaceWG 高度: " << surfaceY << std::endl;
        const BlockState* surfaceBlock = chunk.getBlockState(x, surfaceY, z);
        std::string surfaceName =
            surfaceBlock != nullptr ? ("blockId=" + std::to_string(surfaceBlock->getBlock().blockId())) : "null";
        std::cout << "[OceanWaterRepro] 地表方块(" << surfaceY << "): " << surfaceName << std::endl;
        std::cout << "[OceanWaterRepro] 地表是否为水: "
                  << (surfaceBlock != nullptr && surfaceBlock->isLiquid() ? "是" : "否") << std::endl;
    }
};

// ============================================================================
// 测试 1：种子 987654321 在出生点 5x5 范围内生成海洋，海洋列必须有水（回归测试）
//
// 曾经的 bug：generateBiomes 阶段先创建 NoiseChunk 并缓存，导致 generateNoise 阶段
// 的 getOrCreateNoiseChunk factory 不执行，含水层（Aquifer）和方块状态规则链
// （MaterialRuleList）从未被设置，海洋列海平面以下全是空气。
// 修复后：海洋列海平面以下（Y=seaLevel-1 及更低）应有水方块。
// ============================================================================

TEST_F(OceanWaterReproTest, Overworld_OceanColumnsContainWater_Seed987654321)
{
    const u64 seed = 987654321ULL;
    bool foundAnyOcean = false;
    i32 totalOceanColumns = 0;
    i32 totalOceanColumnsWithWater = 0;
    i32 totalOceanColumnsWithSurfaceWater = 0;

    // 扫描 (0,0) 附近 5x5 区块范围寻找海洋
    for (ChunkCoord cx = -2; cx <= 2; ++cx) {
        for (ChunkCoord cz = -2; cz <= 2; ++cz) {
            auto result = generateOverworldTerrain(seed, cx, cz);
            ASSERT_NE(result.centerChunk, nullptr);

            auto stats = analyzeOceanColumns(*result.centerChunk);
            totalOceanColumns += stats.oceanColumnCount;
            totalOceanColumnsWithWater += stats.oceanColumnsWithWater;
            totalOceanColumnsWithSurfaceWater += stats.oceanColumnsWithSurfaceWater;
            if (stats.oceanColumnCount > 0) {
                foundAnyOcean = true;
            }
        }
    }

    std::cout << "[OceanWaterRepro] Seed 987654321: totalOceanColumns=" << totalOceanColumns
              << " oceanColumnsWithWater=" << totalOceanColumnsWithWater
              << " oceanColumnsWithSurfaceWater=" << totalOceanColumnsWithSurfaceWater << std::endl;

    ASSERT_TRUE(foundAnyOcean) << "在 seed=987654321 的 5x5 区块范围内未找到任何海洋群系列，"
                                  "无法验证水域生成。";

    // 核心断言：海洋列在海平面下方（Y=seaLevel-1）应有水方块
    // MC 1.21: 水方块存在于 Y < seaLevel 处，海平面 Y=seaLevel 本身是空气
    EXPECT_GT(totalOceanColumnsWithWater, 0)
        << "发现 " << totalOceanColumns << " 个海洋列，但没有一个在海平面下方有水！"
        << "这表明含水层/方块状态规则链未被正确设置（回归）。";
}

// ============================================================================
// 测试 2：详细诊断——输出海洋列的垂直方块分布
// ============================================================================

TEST_F(OceanWaterReproTest, Overworld_OceanColumnDiagnostic_Seed987654321)
{
    const u64 seed = 987654321ULL;
    bool foundOcean = false;

    for (ChunkCoord cx = -2; cx <= 2 && !foundOcean; ++cx) {
        for (ChunkCoord cz = -2; cz <= 2 && !foundOcean; ++cz) {
            auto result = generateOverworldTerrain(seed, cx, cz);
            if (result.centerChunk == nullptr) {
                continue;
            }

            for (i32 x = 0; x < world::CHUNK_WIDTH && !foundOcean; ++x) {
                for (i32 z = 0; z < world::CHUNK_WIDTH && !foundOcean; ++z) {
                    const BiomeId biomeId = result.centerChunk->getBiomeAtBlock(x, world::SEA_LEVEL, z);
                    if (!world::biome::BiomeTags::IS_OCEAN().contains(biomeId)) {
                        continue;
                    }
                    foundOcean = true;
                    printOceanColumnDiagnostic(*result.centerChunk, cx, cz, x, z, biomeId);
                }
            }
        }
    }

    if (!foundOcean) {
        GTEST_SKIP() << "未找到海洋列";
    }
}

// ============================================================================
// 测试 3：根因诊断——直接查询 NoiseChunk 和 Aquifer
//
// 在已知的海洋列上，直接查询 preliminarySurfaceLevel、finalDensity 和
// Aquifer::computeSubstance 的返回值，定位水域缺失的根因。
// 同时验证修复后含水层（Aquifer）已被正确设置且海平面下方有水方块。
// ============================================================================

TEST_F(OceanWaterReproTest, Overworld_OceanAquiferDiagnostic_Seed987654321)
{
    const u64 seed = 987654321ULL;

    // 生成区块 (-2,-2)（seed 987654321 下该区块含海洋列，世界 -32,-32 附近）
    auto result = generateOverworldTerrain(seed, -2, -2);
    ASSERT_NE(result.centerChunk, nullptr);
    ASSERT_TRUE(result.centerChunk->hasNoiseChunk());

    auto* noiseChunk = result.centerChunk->noiseChunk();
    ASSERT_NE(noiseChunk, nullptr);
    auto* aquifer = noiseChunk->aquifer();
    // 修复前：aquifer 为 nullptr（generateBiomes 先创建 NoiseChunk 缓存，
    // 导致 generateNoise 的 getOrCreateNoiseChunk factory 不执行 setAquifer）
    ASSERT_NE(aquifer, nullptr) << "NoiseChunk 的 aquifer 为 nullptr —— "
                                   "getOrCreateNoiseChunk 缓存导致 setAquifer 未执行（回归）";

    // 找到第一个海洋列
    i32 oceanX = -1;
    i32 oceanZ = -1;
    BiomeId oceanBiome = 0;
    for (i32 x = 0; x < world::CHUNK_WIDTH && oceanX < 0; ++x) {
        for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
            const BiomeId biomeId = result.centerChunk->getBiomeAtBlock(x, world::SEA_LEVEL, z);
            if (world::biome::BiomeTags::IS_OCEAN().contains(biomeId)) {
                oceanX = x;
                oceanZ = z;
                oceanBiome = biomeId;
                break;
            }
        }
    }
    ASSERT_GE(oceanX, 0) << "区块 (-2,-2) 中未找到海洋列";

    const i32 worldX = -2 * world::CHUNK_WIDTH + oceanX;
    const i32 worldZ = -2 * world::CHUNK_WIDTH + oceanZ;

    std::cout << "[OceanWaterDiag] ============================================" << std::endl;
    std::cout << "[OceanWaterDiag] 海洋列: 本地(" << oceanX << "," << oceanZ << ") 世界(" << worldX << "," << worldZ
              << ") 生物群系ID=" << oceanBiome << std::endl;

    // 1. preliminarySurfaceLevel —— 已完整实现（FindTopSurface 密度函数）
    // 注意：preliminarySurfaceLevel 影响含水层水位采样精度，但不影响海洋地表水域生成
    // （aquifer 的 blockY > skipSamplingAboveY 快速路径会返回全局流体）。
    // 此处仅记录值供调试参考，不作为断言。
    const i32 psl = noiseChunk->samplePreliminarySurfaceLevel(worldX, worldZ);
    std::cout << "[OceanWaterDiag] preliminarySurfaceLevel(" << worldX << "," << worldZ << ") = " << psl
              << " (FindTopSurface 计算结果)" << std::endl;

    // 2. 扫描 Y=20..70 的最终方块，验证海平面下方有水
    std::cout << "[OceanWaterDiag] Y 扫描 (finalDensity, placed block):" << std::endl;
    i32 waterBlocks = 0;
    for (i32 y = 20; y <= 70; ++y) {
        const f64 density = noiseChunk->sampleFinalDensity(worldX, y, worldZ);
        const BlockState* placed = result.centerChunk->getBlockState(oceanX, y, oceanZ);
        std::string placedName = "空气(null)";
        if (placed != nullptr) {
            if (placed->isAir()) {
                placedName = "空气";
            } else if (placed->isLiquid()) {
                placedName = (&placed->getBlock() == VanillaBlocks::WATER) ? "水" : "岩浆";
                if (&placed->getBlock() == VanillaBlocks::WATER) {
                    ++waterBlocks;
                }
            } else {
                placedName = "固体(id=" + std::to_string(placed->getBlock().blockId()) + ")";
            }
        }
        // 注意：sampleFinalDensity 在插值循环外调用会返回缓存值（非真实密度），仅作参考
        std::cout << "[OceanWaterDiag]   Y=" << std::setw(3) << y << " density=" << std::fixed << std::setprecision(4)
                  << density << " placed=" << placedName << std::endl;
    }

    // 核心断言：海洋列海平面下方（Y=seaLevel-1）必须有水方块
    // 修复前此处为空气（aquifer 未设置），修复后为水
    const BlockState* seaLevelMinusOne = result.centerChunk->getBlockState(oceanX, world::SEA_LEVEL - 1, oceanZ);
    ASSERT_NE(seaLevelMinusOne, nullptr);
    EXPECT_TRUE(seaLevelMinusOne->isLiquid())
        << "海平面下方第一格 (Y=" << world::SEA_LEVEL - 1 << ") 应为水，修复前为空气（aquifer 未设置）";
    EXPECT_GT(waterBlocks, 0) << "海洋列中应有水方块，但一个都没找到（aquifer 回归）";
}

} // namespace
