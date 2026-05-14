#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/layer/LayerUtil.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <gtest/gtest.h>

namespace mc {
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
 * @brief 测试生物群系层生成的确定性
 */
TEST_F(WorldGenDeterminismTest, LayerBiomeProviderDeterminism)
{
    const u64 seed = 12345;

    // 创建两个生物群系提供者
    auto provider1 = std::make_unique<LayerBiomeProvider>(seed, false);
    auto provider2 = std::make_unique<LayerBiomeProvider>(seed, false);

    // 在相同坐标采样，结果应相同
    for (int i = 0; i < 100; ++i) {
        i32 x = (i * 17) % 1000 - 500;
        i32 z = (i * 31) % 1000 - 500;

        BiomeId biome1 = provider1->getBiome(x, 64, z);
        BiomeId biome2 = provider2->getBiome(x, 64, z);

        EXPECT_EQ(biome1, biome2) << "Biome mismatch at (" << x << ", " << z << ")";
    }
}

/**
 * @brief 测试噪声坐标批量采样与逐点采样一致
 */
TEST_F(WorldGenDeterminismTest, LayerBiomeProviderNoiseBatchMatchesScalarSampling)
{
    const u64 seed = 24680;
    LayerBiomeProvider provider(seed, false);

    constexpr i32 startNoiseX = -40;
    constexpr i32 startNoiseZ = 28;
    constexpr i32 width = 7;
    constexpr i32 height = 6;

    std::array<BiomeId, width * height> batch{};
    provider.getNoiseBiomesBatch(startNoiseX, 0, startNoiseZ, width, height, batch.data());

    size_t idx = 0;
    for (i32 z = 0; z < height; ++z) {
        for (i32 x = 0; x < width; ++x) {
            const BiomeId scalar = provider.getNoiseBiome(startNoiseX + x, 0, startNoiseZ + z);
            EXPECT_EQ(batch[idx], scalar) << "Noise batch mismatch at local(" << x << ", " << z << ")";
            ++idx;
        }
    }
}

/**
 * @brief 测试区块生物群系容器使用噪声网格坐标填充
 */
TEST_F(WorldGenDeterminismTest, LayerBiomeProviderContainerMatchesNoiseGrid)
{
    const u64 seed = 13579;
    LayerBiomeProvider provider(seed, false);

    constexpr ChunkCoord chunkX = 3;
    constexpr ChunkCoord chunkZ = -2;
    constexpr i32 startNoiseX = chunkX * 4;
    constexpr i32 startNoiseZ = chunkZ * 4;

    BiomeContainer container;
    provider.fillBiomeContainer(container, chunkX, chunkZ);

    for (i32 by = 0; by < BiomeContainer::BIOME_HEIGHT; ++by) {
        for (i32 bz = 0; bz < BiomeContainer::BIOME_DEPTH; ++bz) {
            for (i32 bx = 0; bx < BiomeContainer::BIOME_WIDTH; ++bx) {
                const BiomeId expected = provider.getNoiseBiome(startNoiseX + bx, 0, startNoiseZ + bz);
                const BiomeId actual = container.getBiome(bx, by, bz);
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
 * @brief 测试噪声生成器的确定性
 */
TEST_F(WorldGenDeterminismTest, NoiseGeneratorDeterminism)
{
    const u64 seed = 12345;

    // 创建两个 OctavesNoiseGenerator
    math::Random rng1(seed);
    math::Random rng2(seed);

    OctavesNoiseGenerator noise1(rng1, -15, 0);
    OctavesNoiseGenerator noise2(rng2, -15, 0);

    // 测试噪声值是否相同
    for (int i = 0; i < 100; ++i) {
        f32 x = static_cast<f32>(i * 17.3f);
        f32 y = static_cast<f32>(i * 31.7f);
        f32 z = static_cast<f32>(i * 53.1f);

        f32 n1 = noise1.noise(x, y, z);
        f32 n2 = noise2.noise(x, y, z);

        EXPECT_NEAR(n1, n2, 1e-6f) << "Noise mismatch at sample " << i;
    }
}

/**
 * @brief 测试 PerlinNoiseGenerator 的倍频叠加与偏移轴一致性
 *
 * 该测试手动重建 MC 风格的 noiseAt 叠加流程，确保：
 * 1. 使用 yOffset 作为第二坐标偏移（而非 zOffset）
 * 2. 每层频率/振幅缩放顺序正确
 * 3. useNoiseOffsets 开关行为稳定
 */
TEST_F(WorldGenDeterminismTest, PerlinNoiseManualBlendParity)
{
    constexpr u64 seed = 0x20260404ULL;
    constexpr i32 minOctave = -3;
    constexpr i32 maxOctave = 0;

    math::Random rng(seed);
    PerlinNoiseGenerator perlin(rng, minOctave, maxOctave);

    auto manualNoiseAt = [&perlin](f32 x, f32 y, bool useNoiseOffsets) -> f64 {
        constexpr i32 minOct = -3;
        constexpr i32 maxOct = 0;
        constexpr i32 octaveCount = (-minOct) + maxOct + 1;

        f64 result = 0.0;
        f64 xFactor = std::pow(2.0, static_cast<f64>(maxOct));
        f64 yFactor = 1.0 / (std::pow(2.0, static_cast<f64>(octaveCount)) - 1.0);

        for (i32 octave = maxOct; octave >= minOct; --octave) {
            const SimplexNoiseGenerator* level = perlin.getOctave(octave);
            if (level != nullptr) {
                const f64 offsetX = useNoiseOffsets ? static_cast<f64>(level->xOffset()) : 0.0;
                const f64 offsetY = useNoiseOffsets ? static_cast<f64>(level->yOffset()) : 0.0;

                result +=
                    level->getValue(static_cast<f64>(x) * xFactor + offsetX, static_cast<f64>(y) * xFactor + offsetY) *
                    yFactor;
            }

            xFactor /= 2.0;
            yFactor *= 2.0;
        }

        return result;
    };

    const std::array<std::pair<f32, f32>, 5> samples{{
        {0.0f, 0.0f},
        {12.5f, -7.25f},
        {-128.75f, 64.125f},
        {333.0f, -999.0f},
        {2048.5f, 1024.25f},
    }};

    for (const auto& [x, y] : samples) {
        const f64 expectedWithOffsets = manualNoiseAt(x, y, true);
        const f64 expectedWithoutOffsets = manualNoiseAt(x, y, false);

        const f64 actualWithOffsets = static_cast<f64>(perlin.noiseAt(x, y, true));
        const f64 actualWithoutOffsets = static_cast<f64>(perlin.noiseAt(x, y, false));

        EXPECT_NEAR(actualWithOffsets, expectedWithOffsets, 1e-6)
            << "Perlin noiseAt(useOffsets=true) mismatch at (" << x << ", " << y << ")";
        EXPECT_NEAR(actualWithoutOffsets, expectedWithoutOffsets, 1e-6)
            << "Perlin noiseAt(useOffsets=false) mismatch at (" << x << ", " << y << ")";
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
 * @brief 测试生物群系层多次采样确定性
 */
TEST_F(WorldGenDeterminismTest, LayerBiomeProviderMultipleSamples)
{
    const u64 seed = 98765;

    // 使用相同种子创建两个提供者
    LayerBiomeProvider provider1(seed, false);
    LayerBiomeProvider provider2(seed, false);

    // 采样多个点
    for (int i = 0; i < 50; ++i) {
        i32 x = i * 100;
        i32 z = i * 100 + 50;

        BiomeId b1 = provider1.getBiome(x, 64, z);
        BiomeId b2 = provider2.getBiome(x, 64, z);

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
        NoiseChunkGenerator generator(seed, std::move(settings));

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
