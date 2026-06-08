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

#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/noise/NormalNoise.hpp"
#include "common/world/gen/noise/PerlinNoise.hpp"
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <gtest/gtest.h>

namespace mc {

using namespace world::gen::noise;
namespace {

/**
 * @brief 世界生成确定性测试
 *
 * 验证使用相同种子多次生成同一区块，结果应完全相同。
 */
class WorldGenDeterminismTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }
};

/**
 * @brief 测试生物群系源生成的确定性
 */
TEST_F(WorldGenDeterminismTest, MultiNoiseBiomeSourceDeterminism)
{
    const u64 seed = 12345;

    // 创建两个生物群系源
    auto source1 = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(seed, false);
    auto source2 = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(seed, false);

    // 在相同坐标采样，结果应相同
    for (int i = 0; i < 100; ++i) {
        i32 x = (i * 17) % 1000 - 500;
        i32 z = (i * 31) % 1000 - 500;

        BiomeId biome1 = source1->getNoiseBiome(x, 64, z);
        BiomeId biome2 = source2->getNoiseBiome(x, 64, z);

        EXPECT_EQ(biome1, biome2) << "Biome mismatch at (" << x << ", " << z << ")";
    }
}

/**
 * @brief 测试噪声坐标批量采样与逐点采样一致
 */
TEST_F(WorldGenDeterminismTest, MultiNoiseBiomeSourceNoiseBatchMatchesScalarSampling)
{
    const u64 seed = 24680;
    auto source = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(seed, false);

    constexpr i32 startNoiseX = -40;
    constexpr i32 startNoiseZ = 28;
    constexpr i32 width = 7;
    constexpr i32 height = 6;

    std::array<BiomeId, width * height> batch{};
    source->fillBiomeContainer(batch, startNoiseX / 4, startNoiseZ / 4);

    size_t idx = 0;
    for (i32 z = 0; z < height; ++z) {
        for (i32 x = 0; x < width; ++x) {
            const BiomeId scalar = source->getNoiseBiome(startNoiseX + x, 0, startNoiseZ + z);
            EXPECT_EQ(batch[idx], scalar) << "Noise batch mismatch at local(" << x << ", " << z << ")";
            ++idx;
        }
    }
}

/**
 * @brief 测试区块生物群系容器使用噪声网格坐标填充
 */
TEST_F(WorldGenDeterminismTest, MultiNoiseBiomeSourceContainerMatchesNoiseGrid)
{
    const u64 seed = 13579;
    auto source = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(seed, false);

    constexpr ChunkCoord chunkX = 3;
    constexpr ChunkCoord chunkZ = -2;
    constexpr i32 startNoiseX = chunkX * 4;
    constexpr i32 startNoiseZ = chunkZ * 4;

    BiomeContainer container;
    source->fillBiomeContainer(container, chunkX, chunkZ);

    for (i32 by = 0; by < BiomeContainer::VERT_SIZE; ++by) {
        for (i32 bz = 0; bz < BiomeContainer::HORIZ_SIZE; ++bz) {
            for (i32 bx = 0; bx < BiomeContainer::HORIZ_SIZE; ++bx) {
                const BiomeId expected = source->getNoiseBiome(startNoiseX + bx, 0, startNoiseZ + bz);
                const BiomeId actual = container.getBiome(0, bx, by, bz);
                EXPECT_EQ(actual, expected) << "Biome container mismatch at (" << bx << ", " << by << ", " << bz << ")";
            }
        }
    }
}

// /**
//  * @brief 测试随机数生成器的确定性
//  */
// TEST_F(WorldGenDeterminismTest, RandomDeterminism) {
//     const u64 seed = 99999;

//     // 创建两个随机数生成器
//     math::Random rng1(seed);
//     math::Random rng2(seed);

//     // 测试各种随机数方法
//     for (int i = 0; i < 1000; ++i) {
//         EXPECT_EQ(rng1.nextInt(), rng2.nextInt()) << "nextInt() mismatch at " << i;
//         EXPECT_EQ(rng1.nextInt(100), rng2.nextInt(100)) << "nextInt(100) mismatch at " << i;
//         EXPECT_NEAR(rng1.nextFloat(), rng2.nextFloat(), 1e-6f) << "nextFloat() mismatch at " << i;
//         EXPECT_NEAR(rng1.nextDouble(), rng2.nextDouble(), 1e-10) << "nextDouble() mismatch at " << i;
//     }

//     // 测试 skip 方法
//     math::Random rng3(seed);
//     math::Random rng4(seed);

//     rng3.skip(100);
//     for (int i = 0; i < 100; ++i) {
//         rng4.nextU64();  // 手动跳过
//     }

//     for (int i = 0; i < 100; ++i) {
//         EXPECT_EQ(rng3.nextInt(), rng4.nextInt()) << "skip() mismatch at " << i;
//     }
// }

// /**
//  * @brief 测试 skip 方法的正确实现
//  */
// TEST_F(WorldGenDeterminismTest, SkipMethodCorrectness) {
//     const u64 seed = 12345678;

//     // 测试 skip 是否真的跳过正确数量的随机数
//     math::Random rng1(seed);
//     math::Random rng2(seed);

//     // rng1 使用 skip
//     rng1.skip(262);

//     // rng2 手动调用 nextU64 262 次
//     for (int i = 0; i < 262; ++i) {
//         rng2.nextU64();
//     }

//     // 两者现在应该产生相同的随机数序列
//     for (int i = 0; i < 100; ++i) {
//         u64 val1 = rng1.nextU64();
//         u64 val2 = rng2.nextU64();
//         EXPECT_EQ(val1, val2) << "Skip method mismatch at " << i;
//     }
// }

/**
 * @brief 测试特征生成种子的确定性
 */
TEST_F(WorldGenDeterminismTest, FeatureGenerationSeedDeterminism)
{
    const u64 seed = 77777;
    const i32 chunkX = 5;
    const i32 chunkZ = -3;

    // 模拟两次计算特征种子
    auto computeFeatureSeed = [chunkX, chunkZ](u64 worldSeed, DecorationStage stage, u32 featureId) {
        const i32 startX = chunkX * 16;
        const i32 startZ = chunkZ * 16;

        math::Random decorRng(worldSeed);
        const u64 i = static_cast<u64>(decorRng.nextLong()) | 1ULL;
        const u64 j = static_cast<u64>(decorRng.nextLong()) | 1ULL;
        const u64 decorSeed = (static_cast<u64>(startX) * i + static_cast<u64>(startZ) * j) ^ worldSeed;
        decorRng.setSeed(decorSeed);

        const i32 stageOrdinal = static_cast<i32>(stage);
        const u64 featureSeed = decorSeed + static_cast<u64>(featureId) + static_cast<u64>(10000 * stageOrdinal);
        decorRng.setSeed(featureSeed);

        // 生成一些随机数用于比较
        std::vector<u64> values;
        for (int k = 0; k < 10; ++k) {
            values.push_back(decorRng.nextU64());
        }
        return values;
    };

    // 两次计算应产生相同的结果
    auto values1 = computeFeatureSeed(seed, DecorationStage::VegetalDecoration, 1);
    auto values2 = computeFeatureSeed(seed, DecorationStage::VegetalDecoration, 1);

    ASSERT_EQ(values1.size(), values2.size());
    for (size_t i = 0; i < values1.size(); ++i) {
        EXPECT_EQ(values1[i], values2[i]) << "Feature seed mismatch at " << i;
    }
}

/**
 * @brief 测试地表生成种子的确定性
 */
TEST_F(WorldGenDeterminismTest, SurfaceGenerationSeedDeterminism)
{
    const u64 seed = 11111;
    const ChunkCoord chunkX = 7;
    const ChunkCoord chunkZ = 11;

    // 两次计算地表种子
    auto computeSurfaceValues = [chunkX, chunkZ](u64 worldSeed) {
        // 参考 NoiseChunkGenerator::buildSurface
        math::Random surfaceRng(
            static_cast<u64>(chunkX) * 341873128712ULL + static_cast<u64>(chunkZ) * 132897987541ULL + worldSeed);

        std::vector<u64> values;
        for (int i = 0; i < 20; ++i) {
            values.push_back(surfaceRng.nextU64());
        }
        return values;
    };

    auto values1 = computeSurfaceValues(seed);
    auto values2 = computeSurfaceValues(seed);

    ASSERT_EQ(values1.size(), values2.size());
    for (size_t i = 0; i < values1.size(); ++i) {
        EXPECT_EQ(values1[i], values2[i]) << "Surface seed mismatch at " << i;
    }
}

/**
 * @brief 测试 PerlinNoise 生成器的确定性
 */
TEST_F(WorldGenDeterminismTest, PerlinNoiseDeterminism)
{
    const u64 seed = 12345;

    // 创建两个 PerlinNoise 实例（MC 1.18+ 新噪声系统）
    noise::PerlinNoise noise1(seed, -3, {1.0, 1.0, 1.0, 1.0});
    noise::PerlinNoise noise2(seed, -3, {1.0, 1.0, 1.0, 1.0});

    // 测试噪声值是否相同
    for (int i = 0; i < 100; ++i) {
        f64 x = static_cast<f64>(i * 17.3);
        f64 y = static_cast<f64>(i * 31.7);
        f64 z = static_cast<f64>(i * 53.1);

        f64 n1 = noise1.getValue(x, y, z);
        f64 n2 = noise2.getValue(x, y, z);

        EXPECT_NEAR(n1, n2, 1e-10) << "PerlinNoise mismatch at sample " << i;
    }
}

/**
 * @brief 测试 NormalNoise 生成器的确定性
 */
TEST_F(WorldGenDeterminismTest, NormalNoiseDeterminism)
{
    const u64 seed = 54321;

    // 创建两个 NormalNoise 实例
    noise::NormalNoise noise1(seed, -3, {1.0, 1.0, 1.0, 1.0});
    noise::NormalNoise noise2(seed, -3, {1.0, 1.0, 1.0, 1.0});

    // 测试噪声值是否相同
    for (int i = 0; i < 100; ++i) {
        f64 x = static_cast<f64>(i * 17.3);
        f64 y = static_cast<f64>(i * 31.7);
        f64 z = static_cast<f64>(i * 53.1);

        f64 n1 = noise1.getValue(x, y, z);
        f64 n2 = noise2.getValue(x, y, z);

        EXPECT_NEAR(n1, n2, 1e-10) << "NormalNoise mismatch at sample " << i;
    }
}

/**
 * @brief 测试结构生成种子的确定性
 */
TEST_F(WorldGenDeterminismTest, StructureSeedDeterminism)
{
    const u64 seed = 42;
    const ChunkCoord chunkX = 100;
    const ChunkCoord chunkZ = -200;

    // 两次计算结构生成种子
    auto computeStructureValues = [chunkX, chunkZ](u64 worldSeed) {
        // 参考 NoiseChunkGenerator::generateStructureStarts
        math::Random rng(
            static_cast<u64>(chunkX) * 341873128712ULL + static_cast<u64>(chunkZ) * 132897987541ULL + worldSeed);

        std::vector<u64> values;
        for (int i = 0; i < 20; ++i) {
            values.push_back(rng.nextU64());
        }
        return values;
    };

    auto values1 = computeStructureValues(seed);
    auto values2 = computeStructureValues(seed);

    ASSERT_EQ(values1.size(), values2.size());
    for (size_t i = 0; i < values1.size(); ++i) {
        EXPECT_EQ(values1[i], values2[i]) << "Structure seed mismatch at " << i;
    }
}

/**
 * @brief 测试 nextLong() 无参数方法
 */
TEST_F(WorldGenDeterminismTest, NextLongNoArgs)
{
    const u64 seed = 54321;
    math::Random rng(seed);

    // nextLong() 应该返回完整的 64 位随机数
    i64 val = rng.nextLong();
    (void)val; // 只检查不会崩溃

    // 测试多次调用产生不同值
    std::set<i64> values;
    for (int i = 0; i < 100; ++i) {
        values.insert(rng.nextLong());
    }
    EXPECT_GT(values.size(), 90) << "Random long values should be diverse";
}

/**
 * @brief 测试生物群系源多次采样确定性
 */
TEST_F(WorldGenDeterminismTest, MultiNoiseBiomeSourceMultipleSamples)
{
    const u64 seed = 98765;

    // 使用相同种子创建两个生物群系源
    auto source1 = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(seed, false);
    auto source2 = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(seed, false);

    // 采样多个点
    for (int i = 0; i < 50; ++i) {
        i32 x = i * 100;
        i32 z = i * 100 + 50;

        BiomeId b1 = source1->getNoiseBiome(x, 64, z);
        BiomeId b2 = source2->getNoiseBiome(x, 64, z);

        EXPECT_EQ(b1, b2) << "Biome mismatch at (" << x << ", " << z << ") iteration " << i;
    }
}

/**
 * @brief 测试主世界在采样窗口内具备足够地形起伏
 */
TEST_F(WorldGenDeterminismTest, OverworldTerrainHasTallReliefInSampleWindow)
{
    const std::array<u64, 3> seeds{12345ULL, 987654321ULL, 20260404ULL};

    i32 maxHeight = std::numeric_limits<i32>::min();
    i32 minHeight = std::numeric_limits<i32>::max();

    for (u64 seed : seeds) {
        DimensionSettings settings = DimensionSettings::overworld();
        NoiseChunkGenerator generator(
            seed, std::move(settings), mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(seed, false));

        for (i32 z = -512; z <= 512; z += 32) {
            for (i32 x = -512; x <= 512; x += 32) {
                const i32 height = generator.getHeight(x, z, HeightmapType::WorldSurfaceWG);
                maxHeight = std::max(maxHeight, height);
                minHeight = std::min(minHeight, height);
            }
        }
    }

    EXPECT_GE(maxHeight, 96) << "Terrain peak is unexpectedly low";
    EXPECT_GE(maxHeight - minHeight, 30) << "Terrain relief is unexpectedly flat";
}

} // namespace
} // namespace mc
