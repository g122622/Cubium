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
// 密度函数直接采样测试 — 隔离 NoiseChunk 插值层的问题
//
// 直接查询 NoiseRouter 的密度函数（不经过 NoiseChunk），对比 x=4 和 x=12
// 处的密度值。如果原始密度函数对称，则问题在 NoiseChunk 层；
// 如果原始密度函数不对称，则问题在密度函数本身。
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
#include "common/world/gen/density/NoiseRouterData.hpp"

namespace mc {
namespace {

// ============================================================================
// 测试夹具
// ============================================================================

class DensityDirectTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        fluid::FluidRegistry::instance().initialize();
    }
};

// ============================================================================
// 1. 直接查询 continents/erosion/ridges 密度函数值
//    这些是 2D FlatCache+ShiftedNoise，在原始层面不应有 X 轴不对称
// ============================================================================

TEST_F(DensityDirectTest, RawClimateFunctions_SymmetricAtX4VsX12)
{
    const u64 seed = 42;
    auto randomState = world::gen::RandomState::create(DimensionSettings::overworld(), seed);
    auto router = world::gen::density::NoiseRouterData::overworld(*randomState, seed, false, false);

    // 直接查询 continents 在 x=4 和 x=12 处的值
    // continents 是 FlatCache(ShiftedNoise(ShiftX, ShiftZ, ...))
    // FlatCache 在 quart 位置计算，所以 x=4 (quartX=1) 和 x=12 (quartX=3) 应该给出不同值，
    // 但不应该出现系统性偏差（一侧全部为极端值）

    std::cout << "=== Raw climate function values (seed=42) ===" << std::endl;
    std::cout << "Continents (flatCache + shiftedNoise2d):" << std::endl;
    for (i32 x = 0; x <= 15; x += 2) {
        f64 val = router.continents().compute(x, 64, 0);
        std::cout << "  x=" << x << ", y=64, z=0: " << val << std::endl;
    }

    std::cout << "\nErosion (flatCache + shiftedNoise2d):" << std::endl;
    for (i32 x = 0; x <= 15; x += 2) {
        f64 val = router.erosion().compute(x, 64, 0);
        std::cout << "  x=" << x << ", y=64, z=0: " << val << std::endl;
    }

    std::cout << "\nRidges (flatCache + shiftedNoise2d):" << std::endl;
    for (i32 x = 0; x <= 15; x += 2) {
        f64 val = router.ridges().compute(x, 64, 0);
        std::cout << "  x=" << x << ", y=64, z=0: " << val << std::endl;
    }

    std::cout << "\nFinal density (x=0..15, y=64, z=0):" << std::endl;
    for (i32 x = 0; x <= 15; ++x) {
        f64 val = router.finalDensity().compute(x, 64, 0);
        std::cout << "  x=" << x << ": density=" << val << std::endl;
    }

    // 原始密度函数不应该在 x=8 处出现巨大的跳变
    // 检查相邻 x 坐标的密度差是否合理
    f64 prevDensity = router.finalDensity().compute(0, 64, 0);
    f64 maxJump = 0.0;
    i32 maxJumpX = -1;
    for (i32 x = 1; x <= 15; ++x) {
        f64 density = router.finalDensity().compute(x, 64, 0);
        f64 jump = std::abs(density - prevDensity);
        if (jump > maxJump) {
            maxJump = jump;
            maxJumpX = x;
        }
        prevDensity = density;
    }
    std::cout << "\nMax adjacent density jump: " << maxJump << " at x=" << maxJumpX << std::endl;

    // 原始密度函数的相邻跳变不应超过 1.0（这是非常宽松的阈值）
    EXPECT_LT(maxJump, 1.0) << "Raw finalDensity has large jump at x=" << maxJumpX
                            << " — density function itself may be wrong";
}

// ============================================================================
// 2. 对比 NoiseChunk 插值前后的密度值
//    如果原始密度函数对称但 NoiseChunk 插值后不对称，则问题在插值层
// ============================================================================

TEST_F(DensityDirectTest, NoiseChunkInterpolationVsRawDensity)
{
    const u64 seed = 42;
    const i32 cellWidth = 4;
    const i32 cellHeight = 8;
    const i32 startY = world::MIN_BUILD_HEIGHT;
    const i32 noiseHeight = world::MAX_BUILD_HEIGHT - world::MIN_BUILD_HEIGHT;
    const i32 cellCountY = noiseHeight / cellHeight;

    // 创建区块 (0,0) 的 NoiseChunk（与区块生成相同的配置）
    auto randomState = world::gen::RandomState::create(DimensionSettings::overworld(), seed);
    auto routerCopy = randomState->createRouterCopy();

    auto noiseChunk = std::make_unique<world::gen::density::NoiseChunk>(std::move(routerCopy),
        cellWidth,
        cellHeight,
        cellCountY,
        0,
        startY,
        0, // startX=0, startY=-64, startZ=0
        std::make_unique<world::gen::density::BeardifierMarker>(),
        4 // cellCountXZ=4 (full chunk)
    );

    // 同时创建一个原始路由器用于直接采样
    auto rawRouter = world::gen::density::NoiseRouterData::overworld(*randomState, seed, false, false);

    // 运行完整的插值流程（与 _generateNoiseWithDensityFunction 相同）
    const auto& cellConfig = noiseChunk->cellConfig();
    noiseChunk->initializeForFirstCellX();

    std::cout << "=== NoiseChunk interpolation vs raw density (seed=42, chunk 0,0) ===" << std::endl;

    // 记录每个 x 位置 y=64 处的密度值
    std::vector<f64> chunkDensities(16, 0.0);

    for (i32 cellX = 0; cellX < cellConfig.cellCountXZ; ++cellX) {
        noiseChunk->advanceCellX(cellX);

        for (i32 cellZ = 0; cellZ < cellConfig.cellCountXZ; ++cellZ) {
            for (i32 cellY = cellConfig.cellCountY - 1; cellY >= 0; --cellY) {
                // 只关心 y=64 附近的 cell
                const i32 cellStartY = (noiseChunk->firstCellY() + cellY) * cellHeight;
                if (cellStartY + cellHeight < 60 || cellStartY > 70) {
                    // 跳过不包含 y=64 的 cell
                    // 但仍然需要调用 selectCellXYZ 以维持状态
                    noiseChunk->selectCellXYZ(cellX, cellY, cellZ);
                    for (i32 inCellY = cellHeight - 1; inCellY >= 0; --inCellY) {
                        const i32 blockY = cellStartY + inCellY;
                        const f64 yLerp = static_cast<f64>(inCellY) / static_cast<f64>(cellHeight);
                        noiseChunk->updateForY(blockY, yLerp);
                        for (i32 inCellX = 0; inCellX < cellWidth; ++inCellX) {
                            const i32 localX = cellX * cellWidth + inCellX;
                            const i32 blockX = localX; // startX=0
                            const f64 xLerp = static_cast<f64>(inCellX) / static_cast<f64>(cellWidth);
                            noiseChunk->updateForX(blockX, xLerp);
                            for (i32 inCellZ = 0; inCellZ < cellWidth; ++inCellZ) {
                                const i32 localZ = cellZ * cellWidth + inCellZ;
                                const i32 blockZ = localZ; // startZ=0
                                const f64 zLerp = static_cast<f64>(inCellZ) / static_cast<f64>(cellWidth);
                                noiseChunk->updateForZ(blockZ, zLerp);

                                if (blockY == 64 && blockZ == 0) {
                                    const f64 density = noiseChunk->finalDensity().compute(blockX, blockY, blockZ);
                                    if (localX < 16) {
                                        chunkDensities[localX] = density;
                                    }
                                }
                            }
                        }
                    }
                    continue;
                }

                noiseChunk->selectCellXYZ(cellX, cellY, cellZ);

                for (i32 inCellY = cellHeight - 1; inCellY >= 0; --inCellY) {
                    const i32 blockY = cellStartY + inCellY;
                    const f64 yLerp = static_cast<f64>(inCellY) / static_cast<f64>(cellHeight);
                    noiseChunk->updateForY(blockY, yLerp);

                    for (i32 inCellX = 0; inCellX < cellWidth; ++inCellX) {
                        const i32 localX = cellX * cellWidth + inCellX;
                        const i32 blockX = localX;
                        const f64 xLerp = static_cast<f64>(inCellX) / static_cast<f64>(cellWidth);
                        noiseChunk->updateForX(blockX, xLerp);

                        for (i32 inCellZ = 0; inCellZ < cellWidth; ++inCellZ) {
                            const i32 localZ = cellZ * cellWidth + inCellZ;
                            const i32 blockZ = localZ;
                            const f64 zLerp = static_cast<f64>(inCellZ) / static_cast<f64>(cellWidth);

                            noiseChunk->updateForZ(blockZ, zLerp);

                            if (blockY == 64 && blockZ == 0) {
                                const f64 density = noiseChunk->finalDensity().compute(blockX, blockY, blockZ);
                                if (localX < 16) {
                                    chunkDensities[localX] = density;
                                }
                            }
                        }
                    }
                }
            }
        }

        noiseChunk->swapSlices();
    }

    // 对比原始密度函数和 NoiseChunk 插值后的值
    std::cout << "\nDensity comparison at y=64, z=0:" << std::endl;
    std::cout << "  x   | NoiseChunk   | Raw Router  | Diff" << std::endl;
    std::cout << "  ----|-------------|------------|------" << std::endl;
    for (i32 x = 0; x < 16; ++x) {
        f64 rawDensity = rawRouter.finalDensity().compute(x, 64, 0);
        f64 chunkDensity = chunkDensities[x];
        f64 diff = chunkDensity - rawDensity;
        std::cout << "  " << std::setw(3) << x << " | " << std::fixed << std::setprecision(6) << std::setw(11)
                  << chunkDensity << " | " << std::setw(10) << rawDensity << " | " << std::setw(6) << diff << std::endl;
    }

    // NoiseChunk 插值后的密度不应在 x=8 处出现巨大跳变
    f64 maxJump = 0.0;
    i32 maxJumpX = -1;
    for (i32 x = 1; x < 16; ++x) {
        f64 jump = std::abs(chunkDensities[x] - chunkDensities[x - 1]);
        if (jump > maxJump) {
            maxJump = jump;
            maxJumpX = x;
        }
    }
    std::cout << "\nMax adjacent density jump in NoiseChunk: " << maxJump << " at x=" << maxJumpX << std::endl;

    // 原始路由器不应有巨大跳变
    f64 rawMaxJump = 0.0;
    i32 rawMaxJumpX = -1;
    for (i32 x = 1; x < 16; ++x) {
        f64 rawDensity1 = rawRouter.finalDensity().compute(x - 1, 64, 0);
        f64 rawDensity2 = rawRouter.finalDensity().compute(x, 64, 0);
        f64 jump = std::abs(rawDensity2 - rawDensity1);
        if (jump > rawMaxJump) {
            rawMaxJump = jump;
            rawMaxJumpX = x;
        }
    }
    std::cout << "Max adjacent density jump in Raw Router: " << rawMaxJump << " at x=" << rawMaxJumpX << std::endl;
}

} // namespace
} // namespace mc
