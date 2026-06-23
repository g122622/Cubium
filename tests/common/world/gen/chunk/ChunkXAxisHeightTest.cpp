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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND, EXPRESS OR IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

// ============================================================================
// 区块内 X 轴高度不对称性测试
//
// 排查用户报告的 bug：区块内 x=0~7 的表面高度约 50~60，
// 而 x=8~15 的表面高度约 250，存在严重的 X 轴不对称。
// Z 方向不受此规律影响。
//
// 测试策略：
//   1. 生成多个区块，统计 x=0~7 和 x=8~15 的平均表面高度
//   2. 验证两半的高度差异不应超过合理范围（正常地形差异 < 40 格）
//   3. 对比 Z 轴方向（z=0~7 vs z=8~15）的高度差异，确认 Z 轴无此问题
//   4. 多种子、多区块位置测试，排除种子相关的偶然性
//   5. 直接输出诊断信息，帮助定位问题
// ============================================================================

#include <gtest/gtest.h>

#include "common/core/Constants.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/density/Beardifier.hpp"

#include <cmath>
#include <numeric>
#include <vector>

namespace mc {
namespace {

// ============================================================================
// 测试夹具
// ============================================================================

class ChunkXAxisHeightTest : public ::testing::Test {
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

        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(seed, false);
        result->generator =
            std::make_unique<NoiseChunkGenerator>(seed, DimensionSettings::overworld(), std::move(biomeSource));

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

    // 收集区块内指定 X 范围的表面高度
    static std::vector<i32> collectHeights(const ChunkPrimer& chunk, i32 xStart, i32 xEnd)
    {
        std::vector<i32> heights;
        for (i32 x = xStart; x <= xEnd; ++x) {
            for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
                i32 h = chunk.getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
                if (h >= world::MIN_BUILD_HEIGHT) {
                    heights.push_back(h);
                }
            }
        }
        return heights;
    }

    // 计算均值
    static double mean(const std::vector<i32>& v)
    {
        if (v.empty()) {
            return 0.0;
        }
        return static_cast<double>(std::accumulate(v.begin(), v.end(), i64(0))) / static_cast<double>(v.size());
    }

    // 计算标准差
    static double stddev(const std::vector<i32>& v)
    {
        if (v.size() < 2) {
            return 0.0;
        }
        double m = mean(v);
        double sum = 0.0;
        for (i32 val : v) {
            double d = static_cast<double>(val) - m;
            sum += d * d;
        }
        return std::sqrt(sum / static_cast<double>(v.size()));
    }
};

// ============================================================================
// 1. 核心诊断测试：X 轴两半高度对比
// ============================================================================

TEST_F(ChunkXAxisHeightTest, XAxis_HeightDiagnostic_Seed42_Chunk00)
{
    // 诊断测试：打印 x=0~7 和 x=8~15 的高度分布详情
    auto result = generateOverworld(42, 0, 0);
    ASSERT_NE(result->centerChunk, nullptr);

    auto leftHeights = collectHeights(*result->centerChunk, 0, 7);
    auto rightHeights = collectHeights(*result->centerChunk, 8, 15);

    ASSERT_FALSE(leftHeights.empty());
    ASSERT_FALSE(rightHeights.empty());

    double leftMean = mean(leftHeights);
    double rightMean = mean(rightHeights);
    double leftStd = stddev(leftHeights);
    double rightStd = stddev(rightHeights);
    double heightDiff = std::abs(rightMean - leftMean);

    // 输出详细诊断信息
    std::cout << "=== X-Axis Height Diagnostic (seed=42, chunk 0,0) ===" << std::endl;
    std::cout << "Left half  (x=0~7):  mean=" << leftMean << " stddev=" << leftStd
              << " min=" << *std::min_element(leftHeights.begin(), leftHeights.end())
              << " max=" << *std::max_element(leftHeights.begin(), leftHeights.end()) << std::endl;
    std::cout << "Right half (x=8~15): mean=" << rightMean << " stddev=" << rightStd
              << " min=" << *std::min_element(rightHeights.begin(), rightHeights.end())
              << " max=" << *std::max_element(rightHeights.begin(), rightHeights.end()) << std::endl;
    std::cout << "Height difference: " << heightDiff << std::endl;

    // 逐列打印高度（帮助可视化 X 轴分布）
    std::cout << "\nPer-column surface height (z=0):" << std::endl;
    for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
        i32 h = result->centerChunk->getTopBlockY(HeightmapType::WorldSurfaceWG, x, 0);
        std::cout << "  x=" << x << ": Y=" << h << std::endl;
    }

    // 逐 Z 行打印（帮助检查 Z 轴是否受影响）
    std::cout << "\nPer-Z-row average height (x=0~7 vs x=8~15):" << std::endl;
    for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
        i32 leftSum = 0, rightSum = 0;
        for (i32 x = 0; x < 8; ++x) {
            leftSum += result->centerChunk->getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
        }
        for (i32 x = 8; x < 16; ++x) {
            rightSum += result->centerChunk->getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
        }
        std::cout << "  z=" << z << ": left_avg=" << (leftSum / 8) << " right_avg=" << (rightSum / 8) << std::endl;
    }

    // 断言：两半的高度差不应超过 40 格（正常地形在 16 格宽度内不会出现 200 格落差）
    EXPECT_LT(heightDiff, 40.0) << "X-axis height asymmetry detected! Left mean=" << leftMean
                                 << " Right mean=" << rightMean << " Diff=" << heightDiff;
}

// ============================================================================
// 2. Z 轴对照测试：确认 Z 轴无此问题
// ============================================================================

TEST_F(ChunkXAxisHeightTest, ZAxis_HeightSymmetric_Seed42_Chunk00)
{
    // Z 轴对照：z=0~7 vs z=8~15 的平均高度应相近
    auto result = generateOverworld(42, 0, 0);
    ASSERT_NE(result->centerChunk, nullptr);

    std::vector<i32> topHeights;    // z=0~7
    std::vector<i32> bottomHeights; // z=8~15

    for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
        for (i32 z = 0; z < 8; ++z) {
            i32 h = result->centerChunk->getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
            if (h >= world::MIN_BUILD_HEIGHT) {
                topHeights.push_back(h);
            }
        }
        for (i32 z = 8; z < world::CHUNK_WIDTH; ++z) {
            i32 h = result->centerChunk->getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
            if (h >= world::MIN_BUILD_HEIGHT) {
                bottomHeights.push_back(h);
            }
        }
    }

    ASSERT_FALSE(topHeights.empty());
    ASSERT_FALSE(bottomHeights.empty());

    double topMean = mean(topHeights);
    double bottomMean = mean(bottomHeights);
    double zDiff = std::abs(bottomMean - topMean);

    std::cout << "Z-axis check: top(z=0~7) mean=" << topMean << " bottom(z=8~15) mean=" << bottomMean
              << " diff=" << zDiff << std::endl;

    // Z 轴两半高度差不应超过 40 格
    EXPECT_LT(zDiff, 40.0) << "Z-axis should not have significant height asymmetry";
}

// ============================================================================
// 3. 多种子测试
// ============================================================================

TEST_F(ChunkXAxisHeightTest, XAxis_HeightConsistent_MultiSeed)
{
    // 多种子测试：排除种子相关的偶然性
    constexpr u64 seeds[] = {42ULL, 12345ULL, 987654321ULL, 0xCAFEBABEULL, 0xDEADBEEFULL};

    for (u64 seed : seeds) {
        auto result = generateOverworld(seed, 0, 0);
        ASSERT_NE(result->centerChunk, nullptr) << "Failed for seed " << seed;

        auto leftHeights = collectHeights(*result->centerChunk, 0, 7);
        auto rightHeights = collectHeights(*result->centerChunk, 8, 15);

        ASSERT_FALSE(leftHeights.empty()) << "No left heights for seed " << seed;
        ASSERT_FALSE(rightHeights.empty()) << "No right heights for seed " << seed;

        double leftMean = mean(leftHeights);
        double rightMean = mean(rightHeights);
        double heightDiff = std::abs(rightMean - leftMean);

        EXPECT_LT(heightDiff, 40.0) << "X-axis height asymmetry for seed=" << seed
                                     << ": left_mean=" << leftMean << " right_mean=" << rightMean
                                     << " diff=" << heightDiff;
    }
}

// ============================================================================
// 4. 多区块位置测试
// ============================================================================

TEST_F(ChunkXAxisHeightTest, XAxis_HeightConsistent_MultiChunkPosition)
{
    // 在不同区块坐标测试，排除原点附近的特殊性
    constexpr ChunkCoord positions[][2] = {{0, 0}, {1, 0}, {0, 1}, {5, 3}, {-2, -2}};

    for (const auto& pos : positions) {
        auto result = generateOverworld(42, pos[0], pos[1]);
        ASSERT_NE(result->centerChunk, nullptr) << "Failed for chunk (" << pos[0] << "," << pos[1] << ")";

        auto leftHeights = collectHeights(*result->centerChunk, 0, 7);
        auto rightHeights = collectHeights(*result->centerChunk, 8, 15);

        if (leftHeights.empty() || rightHeights.empty()) {
            continue;
        }

        double leftMean = mean(leftHeights);
        double rightMean = mean(rightHeights);
        double heightDiff = std::abs(rightMean - leftMean);

        EXPECT_LT(heightDiff, 40.0) << "X-axis height asymmetry at chunk (" << pos[0] << "," << pos[1]
                                     << "): left_mean=" << leftMean << " right_mean=" << rightMean
                                     << " diff=" << heightDiff;
    }
}

// ============================================================================
// 5. X 轴各列高度连续性测试
// ============================================================================

TEST_F(ChunkXAxisHeightTest, XAxis_HeightSmoothTransition_AcrossColumns)
{
    // 逐列检查 X 方向高度连续性
    // 正常地形中，相邻列的高度差不应超过 20 格
    auto result = generateOverworld(42, 0, 0);
    ASSERT_NE(result->centerChunk, nullptr);

    // 特别关注 x=7 → x=8 的边界
    i32 largeJumps = 0;
    i32 totalChecks = 0;
    i32 boundaryJumps = 0; // x=7→8 的跳跃次数

    for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
        i32 prevH = result->centerChunk->getTopBlockY(HeightmapType::WorldSurfaceWG, 0, z);
        for (i32 x = 1; x < world::CHUNK_WIDTH; ++x) {
            i32 h = result->centerChunk->getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
            i32 diff = std::abs(h - prevH);
            ++totalChecks;
            if (diff > 20) {
                ++largeJumps;
            }
            // 专门检测 x=7→x=8 边界
            if (x == 8 && diff > 20) {
                ++boundaryJumps;
            }
            prevH = h;
        }
    }

    double jumpRatio = static_cast<double>(largeJumps) / static_cast<double>(totalChecks);
    EXPECT_LT(jumpRatio, 0.10) << "Too many large height jumps across X columns: " << largeJumps << "/" << totalChecks
                                << " (" << (jumpRatio * 100.0) << "%)";

    // x=7→x=8 边界处不应出现特别多的跳跃（如果 bug 存在，这里会有大量跳跃）
    EXPECT_LT(boundaryJumps, 8) << "Too many height jumps at x=7→x=8 boundary: " << boundaryJumps << "/16";
}

// ============================================================================
// 6. getHeight API 交叉验证
// ============================================================================

TEST_F(ChunkXAxisHeightTest, XAxis_GetHeightAPIDiagnostic)
{
    // 使用 NoiseChunkGenerator::getHeight() 接口查询各 X 坐标高度
    // 此接口基于 iterateNoiseColumn，与区块生成的管线不同
    // 如果 getHeight 也出现不对称，说明噪声计算本身有问题
    // 如果 getHeight 对称但区块生成不对称，说明区块生成管线有问题
    u64 seed = 42ULL;
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(seed, false);
    NoiseChunkGenerator gen(seed, DimensionSettings::overworld(), std::move(biomeSource));

    std::vector<i32> leftHeights, rightHeights;

    // 在区块 (0,0) 范围内采样
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; z += 4) {
            i32 h = gen.getHeight(x, z, HeightmapType::WorldSurfaceWG);
            if (x < 8) {
                leftHeights.push_back(h);
            } else {
                rightHeights.push_back(h);
            }
        }
    }

    double leftMean = mean(leftHeights);
    double rightMean = mean(rightHeights);
    double heightDiff = std::abs(rightMean - leftMean);

    std::cout << "getHeight() API diagnostic:" << std::endl;
    std::cout << "  Left  (x=0~7):  mean=" << leftMean << " samples=" << leftHeights.size() << std::endl;
    std::cout << "  Right (x=8~15): mean=" << rightMean << " samples=" << rightHeights.size() << std::endl;
    std::cout << "  Diff=" << heightDiff << std::endl;

    // 逐 x 坐标打印高度（z=0 固定）
    std::cout << "  Per-x heights (z=0):" << std::endl;
    for (i32 x = 0; x < 16; ++x) {
        i32 h = gen.getHeight(x, 0, HeightmapType::WorldSurfaceWG);
        std::cout << "    x=" << x << ": Y=" << h << std::endl;
    }

    EXPECT_LT(heightDiff, 40.0) << "getHeight() also shows X-axis asymmetry! Left mean=" << leftMean
                                 << " Right mean=" << rightMean << " Diff=" << heightDiff;
}

// ============================================================================
// 7. Cell 边界细粒度测试（cellWidth=4，所以 x=0~3,4~7,8~11,12~15 各是一个 cell）
// ============================================================================

TEST_F(ChunkXAxisHeightTest, XAxis_CellLevelDiagnostic)
{
    // 将 X 轴按 cellWidth=4 分成 4 个 cell，分别统计高度
    auto result = generateOverworld(42, 0, 0);
    ASSERT_NE(result->centerChunk, nullptr);

    std::vector<i32> cellHeights[4]; // cell 0: x=0~3, cell 1: x=4~7, cell 2: x=8~11, cell 3: x=12~15

    for (i32 x = 0; x < world::CHUNK_WIDTH; ++x) {
        i32 cellIdx = x / 4;
        for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
            i32 h = result->centerChunk->getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
            if (h >= world::MIN_BUILD_HEIGHT) {
                cellHeights[cellIdx].push_back(h);
            }
        }
    }

    std::cout << "=== Cell-level height diagnostic (seed=42, chunk 0,0) ===" << std::endl;
    for (i32 i = 0; i < 4; ++i) {
        if (cellHeights[i].empty()) {
            std::cout << "  Cell " << i << " (x=" << (i * 4) << "~" << (i * 4 + 3) << "): no data" << std::endl;
            continue;
        }
        double m = mean(cellHeights[i]);
        double s = stddev(cellHeights[i]);
        i32 mn = *std::min_element(cellHeights[i].begin(), cellHeights[i].end());
        i32 mx = *std::max_element(cellHeights[i].begin(), cellHeights[i].end());
        std::cout << "  Cell " << i << " (x=" << (i * 4) << "~" << (i * 4 + 3)
                  << "): mean=" << m << " stddev=" << s << " min=" << mn << " max=" << mx << std::endl;
    }

    // 相邻 cell 之间的高度差不应超过 40 格
    for (i32 i = 0; i < 3; ++i) {
        if (cellHeights[i].empty() || cellHeights[i + 1].empty()) {
            continue;
        }
        double diff = std::abs(mean(cellHeights[i + 1]) - mean(cellHeights[i]));
        EXPECT_LT(diff, 40.0) << "Adjacent cell height difference too large between cell " << i << " and " << (i + 1)
                               << ": diff=" << diff;
    }
}

// ============================================================================
// 8. 相邻列高度差逐列分析
// ============================================================================

TEST_F(ChunkXAxisHeightTest, XAxis_PerColumnDiffAnalysis)
{
    // 对每对相邻列计算平均高度差，帮助精确定位不连续点
    auto result = generateOverworld(42, 0, 0);
    ASSERT_NE(result->centerChunk, nullptr);

    // 对每个 z，计算 x 方向相邻列高度差
    std::vector<double> avgDiffPerXPair(15, 0.0); // x=0→1, 1→2, ..., 14→15

    for (i32 x = 0; x < 15; ++x) {
        i32 totalDiff = 0;
        for (i32 z = 0; z < world::CHUNK_WIDTH; ++z) {
            i32 h1 = result->centerChunk->getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
            i32 h2 = result->centerChunk->getTopBlockY(HeightmapType::WorldSurfaceWG, x + 1, z);
            totalDiff += std::abs(h2 - h1);
        }
        avgDiffPerXPair[x] = static_cast<double>(totalDiff) / static_cast<double>(world::CHUNK_WIDTH);
    }

    std::cout << "Per-column-pair average height diff (seed=42, chunk 0,0):" << std::endl;
    for (i32 x = 0; x < 15; ++x) {
        std::cout << "  x=" << x << "->" << (x + 1) << ": avg_diff=" << avgDiffPerXPair[x] << std::endl;
    }

    // x=7→8 的平均高度差不应显著大于其他相邻对的平均高度差
    double otherPairAvg = 0.0;
    i32 otherPairCount = 0;
    for (i32 x = 0; x < 15; ++x) {
        if (x != 7) {
            otherPairAvg += avgDiffPerXPair[x];
            ++otherPairCount;
        }
    }
    if (otherPairCount > 0) {
        otherPairAvg /= static_cast<double>(otherPairCount);
    }

    double x78Diff = avgDiffPerXPair[7];
    std::cout << "x=7->8 avg_diff=" << x78Diff << ", other pairs avg=" << otherPairAvg << std::endl;

    // x=7→8 的差不应超过其他对的 5 倍（如果 bug 存在，这个比值会非常大）
    if (otherPairAvg > 0.01) {
        double ratio = x78Diff / otherPairAvg;
        EXPECT_LT(ratio, 5.0) << "x=7->8 height jump is " << ratio
                               << "x larger than average adjacent column diff. "
                               << "This indicates a cell boundary issue at x=8.";
    }
}

} // namespace
} // namespace mc
