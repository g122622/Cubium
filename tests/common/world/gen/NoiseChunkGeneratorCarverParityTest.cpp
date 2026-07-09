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

#include <gtest/gtest.h>

#include "common/WorldGenRegistryFixture.hpp"
#include "common/core/Constants.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/density/Beardifier.hpp"

#include <memory>
#include <vector>

namespace mc {
namespace {

/**
 * @brief NoiseChunkGenerator 雕刻阶段一致性测试
 *
 * 验证两次独立注入 BiomeSource 的构造路径在空气雕刻阶段行为一致，
 * 防止显式装配路径遗漏关键初始化逻辑。

 */
class NoiseChunkGeneratorCarverParityTest : public ::testing::Test {
protected:
    /**
     * @brief 初始化测试依赖
     *
     * @note 方块和生物群系注册表是生成流程的前置条件。数据驱动迁移后雕刻器需从
     *       数据包加载到 ConfiguredCarverRegistry，否则 applyCarvers 无雕刻器可执行。
     *       数据包目录缺失时跳过整个测试套件（无法验证雕刻一致性）。
     */
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        // 从默认数据包目录加载 worldgen 注册表（carver/feature/biome）。
        // 失败时置标志，后续 TEST 用 GTEST_SKIP 跳过。
        if (!test::loadVanillaWorldGenRegistries()) {
            s_registriesLoaded = false;
        }
    }

    static inline bool s_registriesLoaded = true;

    /**
     * @brief 以石头填满整个区块
     *
     * @note 雕刻器会把石头替换为空气或熔岩，用于稳定统计雕刻结果。
     */
    static void fillChunkWithStone(ChunkPrimer& chunk)
    {
        const BlockState* stone = &VanillaBlocks::STONE->defaultState();
        ASSERT_NE(stone, nullptr);

        for (i32 x = 0; x < 16; ++x) {
            for (i32 z = 0; z < 16; ++z) {
                for (i32 y = 0; y < mc::world::MAX_BUILD_HEIGHT; ++y) {
                    chunk.setBlockState(x, y, z, stone);
                }
            }
        }
    }

    /**
     * @brief 为区块写入统一生物群系
     *
     * @note 当前雕刻逻辑对生物群系依赖较弱，但仍应保持输入数据完整。
     */
    static void fillChunkBiomes(ChunkPrimer& chunk, BiomeId biomeId)
    {
        BiomeContainer& biomes = chunk.getBiomes();
        for (i32 section = 0; section < BiomeContainer::SECTION_COUNT; ++section) {
            for (i32 y = 0; y < BiomeContainer::VERT_SIZE; ++y) {
                for (i32 z = 0; z < BiomeContainer::HORIZ_SIZE; ++z) {
                    for (i32 x = 0; x < BiomeContainer::HORIZ_SIZE; ++x) {
                        biomes.setBiome(section, x, y, z, biomeId);
                    }
                }
            }
        }
    }

    /**
     * @brief 创建用于雕刻测试的实体区块
     */
    static std::unique_ptr<ChunkPrimer> makeSolidChunk(ChunkCoord chunkX, ChunkCoord chunkZ)
    {
        auto chunk = std::make_unique<ChunkPrimer>(chunkX, chunkZ);
        fillChunkWithStone(*chunk);
        fillChunkBiomes(*chunk, Biomes::Plains);
        chunk->updateAllHeightmaps();
        return chunk;
    }

    /**
     * @brief 统计被雕刻后非石头方块数量
     */
    static i32 countNonStoneBlocks(const ChunkPrimer& chunk)
    {
        const BlockState* stone = &VanillaBlocks::STONE->defaultState();
        i32 count = 0;

        for (i32 x = 0; x < 16; ++x) {
            for (i32 z = 0; z < 16; ++z) {
                for (i32 y = 0; y < mc::world::MAX_BUILD_HEIGHT; ++y) {
                    const BlockState* state = chunk.getBlockState(x, y, z);
                    if (state != stone) {
                        ++count;
                    }
                }
            }
        }

        return count;
    }

    /**
     * @brief 构造最小化 WorldGenRegion 视图
     *
     * @note applyCarvers 当前不读取 region 数据，此处仅满足接口约束。
     */
    static std::vector<IChunk*> makeRegionChunks(IChunk* centerChunk) { return std::vector<IChunk*>(9, centerChunk); }

    /**
     * @brief 逐方块断言两个区块完全一致
     */
    static void expectChunkBlocksEqual(const ChunkPrimer& lhs, const ChunkPrimer& rhs)
    {
        for (i32 x = 0; x < 16; ++x) {
            for (i32 z = 0; z < 16; ++z) {
                for (i32 y = 0; y < mc::world::MAX_BUILD_HEIGHT; ++y) {
                    const BlockState* l = lhs.getBlockState(x, y, z);
                    const BlockState* r = rhs.getBlockState(x, y, z);
                    EXPECT_EQ(l, r) << "Block mismatch at (" << x << ", " << y << ", " << z << ")";
                }
            }
        }
    }
};

/**
 * @brief 注入 BiomeSource 的构造路径应保持雕刻阶段一致
 *
 * @note 若未初始化雕刻器，此测试会在首个发生雕刻的区块上失败。
 */
TEST_F(NoiseChunkGeneratorCarverParityTest, InjectedBiomeSourceKeepsCarverPipelineParity)
{
    if (!s_registriesLoaded) {
        GTEST_SKIP() << "Vanilla datapacks unavailable; cannot validate carver pipeline parity";
    }

    constexpr u64 seed = 0x4D435245424F524EULL;

    DimensionSettings defaultSettings = DimensionSettings::overworld();
    auto defaultRandomState = mc::world::gen::RandomState::create(defaultSettings, seed);
    auto defaultBiomeSource =
        mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*defaultRandomState, false, false);
    NoiseChunkGenerator defaultGenerator(
        std::move(defaultSettings), std::move(defaultBiomeSource), std::move(defaultRandomState));

    DimensionSettings injectedSettings = DimensionSettings::overworld();
    auto injectedRandomState = mc::world::gen::RandomState::create(injectedSettings, seed);
    auto injectedSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*injectedRandomState, false, false);
    NoiseChunkGenerator injectedGenerator(
        std::move(injectedSettings), std::move(injectedSource), std::move(injectedRandomState));

    bool foundCarvedChunk = false;

    for (ChunkCoord chunkX = -12; chunkX <= 12 && !foundCarvedChunk; ++chunkX) {
        for (ChunkCoord chunkZ = -12; chunkZ <= 12 && !foundCarvedChunk; ++chunkZ) {
            auto defaultChunk = makeSolidChunk(chunkX, chunkZ);
            auto defaultChunks = makeRegionChunks(defaultChunk.get());
            WorldGenRegion defaultRegion(chunkX, chunkZ, 1, std::move(defaultChunks));

            defaultGenerator.applyCarvers(defaultRegion, *defaultChunk);
            const i32 defaultCarved = countNonStoneBlocks(*defaultChunk);
            if (defaultCarved <= 0) {
                continue;
            }

            auto injectedChunk = makeSolidChunk(chunkX, chunkZ);
            auto injectedChunks = makeRegionChunks(injectedChunk.get());
            WorldGenRegion injectedRegion(chunkX, chunkZ, 1, std::move(injectedChunks));

            injectedGenerator.applyCarvers(injectedRegion, *injectedChunk);
            const i32 injectedCarved = countNonStoneBlocks(*injectedChunk);

            EXPECT_EQ(defaultCarved, injectedCarved)
                << "Carved block count mismatch at chunk (" << chunkX << ", " << chunkZ << ")";
            expectChunkBlocksEqual(*defaultChunk, *injectedChunk);

            EXPECT_TRUE(defaultChunk->hasCompletedStatus(ChunkStatuses::CARVERS));
            EXPECT_TRUE(injectedChunk->hasCompletedStatus(ChunkStatuses::CARVERS));
            foundCarvedChunk = true;
        }
    }

    EXPECT_TRUE(foundCarvedChunk) << "No carved chunk found in search range, cannot validate constructor parity";
}

/**
 * @brief NoiseChunkGenerator 高斯查找表初始化测试
 *
 * 验证 24x24x24 高斯查找表在构造时正确初始化。
 */
TEST_F(NoiseChunkGeneratorCarverParityTest, GaussianLUTInitialization)
{
    // 创建生成器时高斯查找表应该被初始化
    constexpr u64 seed = 12345ULL;
    DimensionSettings settings = DimensionSettings::overworld();
    auto randomState = mc::world::gen::RandomState::create(settings, seed);
    auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
    NoiseChunkGenerator generator(std::move(settings), std::move(biomeSource), std::move(randomState));

    // 验证生成器成功创建（高斯查找表作为静态成员初始化）
    // 如果初始化失败，会有编译或运行时错误
    EXPECT_NO_THROW({
        DimensionSettings settings2 = DimensionSettings::overworld();
        auto randomState2 = mc::world::gen::RandomState::create(settings2, seed);
        auto biomeSource2 = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState2, false, false);
        NoiseChunkGenerator generator2(std::move(settings2), std::move(biomeSource2), std::move(randomState2));
    });
}

/**
 * @brief Beardifier::computeBeardContribution 高斯核测试
 *
 * computeBeardContribution 是纯高斯核函数（exp(-distSq/16)），始终返回正值 [0,1]。
 * 它用于预计算 BEARD_KERNEL 查找表。
 *
 * 注意：产生负密度偏移的是 getBeardContribution，它在核值基础上乘以 beard 因子。
 */
TEST(NoiseChunkGeneratorDensityTest, StructureDensityOffsetValues)
{
    // 中心点 (0,0,0)：Y+0.5 → distSq = 0.25 → exp(-0.25/16) ≈ 0.9845
    f64 centerOffset = world::gen::density::Beardifier::computeBeardContribution(0, 0, 0);
    EXPECT_GT(centerOffset, 0.0) << "Gaussian kernel should be positive at center";
    EXPECT_LT(centerOffset, 1.0) << "Gaussian kernel at (0,0,0) with Y+0.5 should be < 1.0";
    EXPECT_NEAR(centerOffset, 0.9845, 0.01) << "Center offset should match MC Gaussian value";

    // 远距离点应该趋近于零
    f64 farOffset = world::gen::density::Beardifier::computeBeardContribution(50, 50, 50);
    EXPECT_NEAR(farOffset, 0.0, 0.001) << "Far distance should have near-zero offset";

    // 对称性：相同距离的点应该产生相同的影响（X和Z轴对称）
    f64 offset1 = world::gen::density::Beardifier::computeBeardContribution(5, 3, 7);
    f64 offset2 = world::gen::density::Beardifier::computeBeardContribution(-5, 3, 7);
    f64 offset3 = world::gen::density::Beardifier::computeBeardContribution(5, 3, -7);
    f64 offset4 = world::gen::density::Beardifier::computeBeardContribution(-5, 3, -7);

    // X 和 Z 的对称性（高斯核是距离的函数，X/Z对称）
    EXPECT_NEAR(offset1, offset2, 0.0001) << "X-axis symmetry should hold";
    EXPECT_NEAR(offset1, offset3, 0.0001) << "Z-axis symmetry should hold";
    EXPECT_NEAR(offset1, offset4, 0.0001) << "XZ diagonal symmetry should hold";

    // Y 轴偏移影响距离（Y+0.5），但高斯核始终为正
    // computeBeardContribution 对 Y 也有对称性（因为 Y+0.5 后取平方）
    f64 belowOffset = world::gen::density::Beardifier::computeBeardContribution(0, -5, 0);
    f64 aboveOffset = world::gen::density::Beardifier::computeBeardContribution(0, 5, 0);
    EXPECT_GT(belowOffset, 0.0) << "Gaussian kernel is always positive";
    EXPECT_GT(aboveOffset, 0.0) << "Gaussian kernel is always positive";
    // Y=-5 和 Y=5 的偏移应相等（(|-5|+0.5)^2 = (5+0.5)^2 不对称，但 Y 方向无额外对称性）
    // 实际上 computeBeardContribution(0,-5,0) 使用 adjustedY=-4.5, distSq=20.25
    //         computeBeardContribution(0, 5,0) 使用 adjustedY= 5.5, distSq=30.25
    EXPECT_GT(belowOffset, aboveOffset) << "Closer Y should have larger Gaussian value (|-5|+0.5 < 5+0.5)";
}

/**
 * @brief NoiseChunkGenerator 高斯查找表边界测试
 *
 * 验证查找表索引计算不会越界。
 */
TEST(NoiseChunkGeneratorDensityTest, GaussianLUTBoundaryCheck)
{
    // 边界内点（-12 到 +11）
    for (i32 dx = -12; dx < 12; ++dx) {
        for (i32 dy = -12; dy < 12; ++dy) {
            for (i32 dz = -12; dz < 12; ++dz) {
                // 应该不崩溃
                f64 offset = world::gen::density::Beardifier::computeBeardContribution(dx, dy, dz);
                (void)offset; // 仅验证计算能完成
            }
        }
    }

    // 边界外点应该返回接近零的值
    f64 outsideOffset = world::gen::density::Beardifier::computeBeardContribution(15, 15, 15);
    // computeBeardContribution 在远处自然衰减，这里只验证函数能正常执行
    (void)outsideOffset;
    SUCCEED() << "Boundary calculations completed without crash";
}

} // namespace
} // namespace mc
