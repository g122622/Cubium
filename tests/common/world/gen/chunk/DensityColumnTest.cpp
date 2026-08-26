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
 * IMPLIED, WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

// ============================================================================
// 密度函数列采样测试 — 精确定位 x=8 处高度异常的根因
//
// 在区块生成循环中，逐 Y 打印 x=4 和 x=12 处的密度值，
// 找到密度值开始出现显著差异的 Y 位置。
// ============================================================================

#include <cmath>
#include <iomanip>
#include <vector>

#include <gtest/gtest.h>

#include "common/core/Constants.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/density/Beardifier.hpp"

namespace mc {
namespace {

class DensityColumnTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        fluid::FluidRegistry::instance().initialize();
    }

    struct GeneratedChunk {
        std::vector<std::unique_ptr<ChunkPrimer>> ownedChunks;
        std::unique_ptr<WorldGenRegion> region;
        std::unique_ptr<NoiseChunkGenerator> generator;
        ChunkPrimer* centerChunk = nullptr;
    };

    static std::unique_ptr<GeneratedChunk> generateOverworld(u64 seed, ChunkCoord cx = 0, ChunkCoord cz = 0)
    {
        auto result = std::make_unique<GeneratedChunk>();
        constexpr i32 radius = 1;
        constexpr i32 diameter = radius * 2 + 1;

        auto settings = DimensionSettings::overworld();
        auto randomState = world::gen::RandomState::create(settings, seed);
        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        result->generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));

        std::vector<IChunk*> chunkPtrs;
        for (i32 dz = -radius; dz <= radius; ++dz) {
            for (i32 dx = -radius; dx <= radius; ++dx) {
                auto primer = std::make_unique<ChunkPrimer>(cx + dx, cz + dz);
                chunkPtrs.push_back(primer.get());
                result->ownedChunks.push_back(std::move(primer));
            }
        }
        result->centerChunk = dynamic_cast<ChunkPrimer*>(chunkPtrs[static_cast<size_t>((radius * diameter) + radius)]);
        result->region = std::make_unique<WorldGenRegion>(cx, cz, radius, std::move(chunkPtrs), 0);

        for (i32 dz = -radius; dz <= radius; ++dz) {
            for (i32 dx = -radius; dx <= radius; ++dx) {
                size_t idx = static_cast<size_t>((dz + radius) * diameter + (dx + radius));
                ChunkPrimer* chunk = result->ownedChunks[idx].get();
                result->generator->generateBiomes(*result->region, *chunk);
                result->generator->generateNoise(*result->region, *chunk);
                result->generator->buildSurface(*result->region, *chunk);
            }
        }

        return result;
    }
};

// ============================================================================
// 1. 在区块生成过程中，逐 Y 打印 x=4 和 x=12 处的密度值
//    通过 NoiseChunk::sampleFinalDensity 直接采样
// ============================================================================

TEST_F(DensityColumnTest, DensityColumnAtX4VsX12)
{
    const u64 seed = 42;
    const i32 cellWidth = 4;
    const i32 cellHeight = 8;
    const i32 startY = world::MIN_BUILD_HEIGHT;
    const i32 noiseHeight = world::MAX_BUILD_HEIGHT - world::MIN_BUILD_HEIGHT;
    const i32 cellCountY = noiseHeight / cellHeight;

    auto randomState = world::gen::RandomState::create(DimensionSettings::overworld(), seed);

    // 为 x=4 (cell 1) 和 x=12 (cell 3) 创建独立的 NoiseChunk
    // 模拟 getHeight 的方式（cellCountXZ=1）
    for (i32 testX : {4, 12}) {
        const i32 cellX = math::floorDiv(testX, cellWidth);
        const i32 alignedX = cellX * cellWidth;
        const f64 deltaX = static_cast<f64>(testX - alignedX) / static_cast<f64>(cellWidth);

        auto noiseChunk = std::make_unique<world::gen::density::NoiseChunk>(*randomState,
            cellWidth,
            cellHeight,
            cellCountY,
            alignedX,
            startY,
            0,
            std::make_unique<world::gen::density::BeardifierMarker>(),
            1);

        // 设置 DisabledAquiferFiller
        {
            std::vector<std::unique_ptr<world::gen::density::BlockStateFiller>> fillers;
            fillers.push_back(std::make_unique<world::gen::density::DisabledAquiferFiller>(
                VanillaBlocks::getState(VanillaBlocks::WATER), 63));
            noiseChunk->setBlockStateRule(std::make_unique<world::gen::density::MaterialRuleList>(std::move(fillers)));
        }

        noiseChunk->initializeForFirstCellX();
        noiseChunk->advanceCellX(0);

        std::cout << "=== Density column at x=" << testX << ", z=0 (single-column NoiseChunk) ===" << std::endl;

        // 逐 Y 采样密度值
        for (i32 cellY = cellCountY - 1; cellY >= 0; --cellY) {
            noiseChunk->selectCellYZ(cellY, 0);
            for (i32 inCellY = cellHeight - 1; inCellY >= 0; --inCellY) {
                const i32 blockY = (math::floorDiv(startY, cellHeight) + cellY) * cellHeight + inCellY;
                const f64 yLerp = static_cast<f64>(inCellY) / static_cast<f64>(cellHeight);
                noiseChunk->updateForY(blockY, yLerp);
                noiseChunk->updateForX(testX, deltaX);
                noiseChunk->updateForZ(0, 0.0);

                const f64 density = noiseChunk->finalDensity().compute(testX, blockY, 0);

                // 只打印关键 Y 位置
                if (blockY >= 60 && blockY <= 80) {
                    std::cout << "  y=" << std::setw(4) << blockY << ": density=" << std::fixed << std::setprecision(6)
                              << density << (density > 0.0 ? " (solid)" : " (air)") << std::endl;
                }
            }
        }
    }
}

// ============================================================================
// 2. 使用完整区块生成，检查 CellCache 的状态
//    在每个 cell 边界处打印密度值
// ============================================================================

TEST_F(DensityColumnTest, CellBoundaryDensityTest)
{
    const u64 seed = 42;
    const i32 cellWidth = 4;
    const i32 cellHeight = 8;
    const i32 startY = world::MIN_BUILD_HEIGHT;
    const i32 noiseHeight = world::MAX_BUILD_HEIGHT - world::MIN_BUILD_HEIGHT;
    const i32 cellCountY = noiseHeight / cellHeight;

    // 创建完整区块的 NoiseChunk（cellCountXZ=4）
    auto randomState = world::gen::RandomState::create(DimensionSettings::overworld(), seed);

    auto noiseChunk = std::make_unique<world::gen::density::NoiseChunk>(*randomState,
        cellWidth,
        cellHeight,
        cellCountY,
        0,
        startY,
        0,
        std::make_unique<world::gen::density::BeardifierMarker>(),
        4);

    {
        std::vector<std::unique_ptr<world::gen::density::BlockStateFiller>> fillers;
        fillers.push_back(std::make_unique<world::gen::density::DisabledAquiferFiller>(
            VanillaBlocks::getState(VanillaBlocks::WATER), 63));
        noiseChunk->setBlockStateRule(std::make_unique<world::gen::density::MaterialRuleList>(std::move(fillers)));
    }

    const auto& cellConfig = noiseChunk->cellConfig();
    noiseChunk->initializeForFirstCellX();

    // 在每个 cell 的 (0,0,0) 位置采样密度值，打印 Y=64 处的值
    std::cout << "=== Cell-boundary density at y=64, z=0 (full chunk NoiseChunk) ===" << std::endl;
    std::cout << "cellX | localX | blockX | density" << std::endl;
    std::cout << "------|--------|--------|--------" << std::endl;

    for (i32 cellX = 0; cellX < cellConfig.cellCountXZ; ++cellX) {
        noiseChunk->advanceCellX(cellX);

        for (i32 cellZ = 0; cellZ < cellConfig.cellCountXZ; ++cellZ) {
            if (cellZ != 0) continue; // 只看 z=0 的 cell

            for (i32 cellY = cellConfig.cellCountY - 1; cellY >= 0; --cellY) {
                const i32 cellStartY = (noiseChunk->firstCellY() + cellY) * cellHeight;
                if (cellStartY + cellHeight < 60 || cellStartY > 70) continue;

                noiseChunk->selectCellXYZ(cellX, cellY, cellZ);

                for (i32 inCellY = cellHeight - 1; inCellY >= 0; --inCellY) {
                    const i32 blockY = cellStartY + inCellY;
                    if (blockY != 64) continue;

                    const f64 yLerp = static_cast<f64>(inCellY) / static_cast<f64>(cellHeight);
                    noiseChunk->updateForY(blockY, yLerp);

                    for (i32 inCellX = 0; inCellX < cellConfig.cellWidth; ++inCellX) {
                        const i32 localX = cellX * cellConfig.cellWidth + inCellX;
                        const i32 blockX = localX;
                        const f64 xLerp = static_cast<f64>(inCellX) / static_cast<f64>(cellConfig.cellWidth);
                        noiseChunk->updateForX(blockX, xLerp);

                        noiseChunk->updateForZ(0, 0.0);
                        const f64 density = noiseChunk->finalDensity().compute(blockX, blockY, 0);

                        std::cout << "  " << cellX << "   |   " << inCellX << "    |   " << std::setw(3) << blockX
                                  << "   | " << std::fixed << std::setprecision(6) << density << std::endl;
                    }
                }
            }
        }

        noiseChunk->swapSlices();
    }
}

} // namespace
} // namespace mc
