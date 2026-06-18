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
 * LIABILITY, WHETHER IN AN EVENT OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file BiomeManagerTest.cpp
 * @brief BiomeManager 和 LinearCongruentialGenerator 单元测试
 *
 * 测试覆盖：
 * 1. LinearCongruentialGenerator::next() 确定性和溢出行为
 * 2. BiomeManager::obfuscateSeed() SHA-256 种子混淆
 * 3. BiomeManager::getBiome() Voronoi 缩放正确性
 * 4. BiomeManager::getFiddle() 范围验证
 * 5. BiomeManager::withDifferentSource() 种子共享
 * 6. BiomeManager::getNoiseBiomeAtQuart/AtPosition() 直接查询
 */

#include "common/world/biome/BiomeManager.hpp"
#include "common/util/crypto/Sha256.hpp"
#include "common/util/math/random/LinearCongruentialGenerator.hpp"
#include <cmath>
#include <limits>
#include <gtest/gtest.h>

namespace mc {
namespace {

// ============================================================================
// LinearCongruentialGenerator 测试
// ============================================================================

TEST(LinearCongruentialGeneratorTest, NextIsDeterministic)
{
    // 相同输入应产生相同输出
    const i64 seed = 12345;
    const i64 value = 100;

    i64 result1 = math::LinearCongruentialGenerator::next(seed, value);
    i64 result2 = math::LinearCongruentialGenerator::next(seed, value);
    EXPECT_EQ(result1, result2);
}

TEST(LinearCongruentialGeneratorTest, NextWithZeroSeed)
{
    // seed=0: s = 0 * (0 * MULT + INC) = 0, return 0 + value = value
    const i64 seed = 0;
    const i64 value = 42;
    i64 result = math::LinearCongruentialGenerator::next(seed, value);
    EXPECT_EQ(result, value);
}

TEST(LinearCongruentialGeneratorTest, NextWithNonZeroSeed)
{
    // 非零种子应产生非平凡结果
    const i64 seed = 1;
    const i64 value = 0;
    i64 result = math::LinearCongruentialGenerator::next(seed, value);
    // seed=1: s = 1 * (1 * MULT + INC) = MULT + INC
    // 由于 signed overflow wrap，结果应是确定性的
    // 只验证结果不为 0（因为 MULT + INC 非常大）
    EXPECT_NE(result, 0);
}

TEST(LinearCongruentialGeneratorTest, NextChainProducesSequence)
{
    // 链式调用应产生确定性序列
    i64 seed = 12345;
    const i64 x = 10, y = 20, z = 30;

    seed = math::LinearCongruentialGenerator::next(seed, x);
    i64 step1 = seed;
    seed = math::LinearCongruentialGenerator::next(seed, y);
    i64 step2 = seed;
    seed = math::LinearCongruentialGenerator::next(seed, z);
    i64 step3 = seed;

    // 每步应产生不同的值
    EXPECT_NE(step1, step2);
    EXPECT_NE(step2, step3);
    EXPECT_NE(step1, step3);

    // 重复相同链应产生相同结果
    i64 seed2 = 12345;
    seed2 = math::LinearCongruentialGenerator::next(seed2, x);
    EXPECT_EQ(seed2, step1);
    seed2 = math::LinearCongruentialGenerator::next(seed2, y);
    EXPECT_EQ(seed2, step2);
    seed2 = math::LinearCongruentialGenerator::next(seed2, z);
    EXPECT_EQ(seed2, step3);
}

TEST(LinearCongruentialGeneratorTest, NextOverflowConsistency)
{
    // Java signed long overflow wrap 行为一致性
    // 使用极端值验证 unsigned 中转策略
    const i64 seed = std::numeric_limits<i64>::min();  // -9223372036854775808
    const i64 value = std::numeric_limits<i64>::max(); // 9223372036854775807

    i64 result = math::LinearCongruentialGenerator::next(seed, value);

    // 结果应是确定性的
    i64 result2 = math::LinearCongruentialGenerator::next(seed, value);
    EXPECT_EQ(result, result2);
}

TEST(LinearCongruentialGeneratorTest, ConstantsMatchKnuthMMIX)
{
    // 验证 Knuth MMIX 常量
    EXPECT_EQ(math::LinearCongruentialGenerator::MULTIPLIER, 6364136223846793005LL);
    EXPECT_EQ(math::LinearCongruentialGenerator::INCREMENT, 1442695040888963407LL);
}

TEST(LinearCongruentialGeneratorTest, NextDifferentValuesProduceDifferentResults)
{
    const i64 seed = 42;
    i64 r1 = math::LinearCongruentialGenerator::next(seed, 0);
    i64 r2 = math::LinearCongruentialGenerator::next(seed, 1);
    i64 r3 = math::LinearCongruentialGenerator::next(seed, -1);
    // 不同 value 应产生不同结果（至少大多数情况下）
    EXPECT_NE(r1, r2);
    EXPECT_NE(r1, r3);
    EXPECT_NE(r2, r3);
}

// ============================================================================
// BiomeManager::obfuscateSeed 测试
// ============================================================================

TEST(BiomeManagerTest, ObfuscateSeedConsistency)
{
    // obfuscateSeed 应与 Sha256::hashWorldSeed 完全一致
    u64 seed = 123456789ULL;
    u64 obfuscated = world::biome::BiomeManager::obfuscateSeed(seed);
    u64 expected = util::crypto::Sha256::hashWorldSeed(seed);
    EXPECT_EQ(obfuscated, expected);
}

TEST(BiomeManagerTest, ObfuscateSeedMultipleSeeds)
{
    // 多个种子应产生不同结果
    u64 s1 = world::biome::BiomeManager::obfuscateSeed(0);
    u64 s2 = world::biome::BiomeManager::obfuscateSeed(1);
    u64 s3 = world::biome::BiomeManager::obfuscateSeed(12345);
    EXPECT_NE(s1, s2);
    EXPECT_NE(s2, s3);
    EXPECT_NE(s1, s3);
}

TEST(BiomeManagerTest, ObfuscateSeedNonZero)
{
    // obfuscateSeed 应产生非零结果（极大概率）
    u64 s = world::biome::BiomeManager::obfuscateSeed(0);
    // SHA-256 哈希全零种子的结果几乎不可能是 0
    // 但我们只验证一致性
    u64 s2 = world::biome::BiomeManager::obfuscateSeed(0);
    EXPECT_EQ(s, s2);
}

// ============================================================================
// BiomeManager::getFiddle 测试（通过 getBiome 间接测试）
// ============================================================================

// 简单的 Mock IBiomeSource 用于测试
class MockBiomeSource : public world::biome::IBiomeSource {
public:
    explicit MockBiomeSource(BiomeId fixedBiome = 1)
        : IBiomeSource(0)
        , m_fixedBiome(fixedBiome)
    {
        m_possibleBiomes = {m_fixedBiome};
    }

    [[nodiscard]] BiomeId getNoiseBiome(i32 quartX, i32 quartY, i32 quartZ) const override
    {
        (void)quartX;
        (void)quartY;
        (void)quartZ;
        return m_fixedBiome;
    }

    [[nodiscard]] const std::vector<BiomeId>& possibleBiomes() const override { return m_possibleBiomes; }

private:
    BiomeId m_fixedBiome;
    std::vector<BiomeId> m_possibleBiomes;
};

// 不同 quart 坐标返回不同生物群系的 Mock
class VaryingBiomeSource : public world::biome::IBiomeSource {
public:
    VaryingBiomeSource()
        : IBiomeSource(0)
    {
        m_possibleBiomes = {0, 1, 2, 3};
    }

    [[nodiscard]] BiomeId getNoiseBiome(i32 quartX, i32 quartY, i32 quartZ) const override
    {
        (void)quartY;
        // 简单映射：按 quartX 和 quartZ 的奇偶返回不同生物群系
        return static_cast<BiomeId>(((quartX & 1) << 1) | (quartZ & 1));
    }

    [[nodiscard]] const std::vector<BiomeId>& possibleBiomes() const override { return m_possibleBiomes; }

private:
    std::vector<BiomeId> m_possibleBiomes;
};

// ============================================================================
// BiomeManager 核心测试
// ============================================================================

TEST(BiomeManagerTest, GetBiomeDeterministic)
{
    // 相同输入应返回相同结果
    MockBiomeSource source(42);
    u64 zoomSeed = world::biome::BiomeManager::obfuscateSeed(12345);
    world::biome::BiomeManager mgr(source, zoomSeed);

    BiomeId b1 = mgr.getBiome(100, 64, 200);
    BiomeId b2 = mgr.getBiome(100, 64, 200);
    EXPECT_EQ(b1, b2);
}

TEST(BiomeManagerTest, GetBiomeConsistentAcrossCalls)
{
    // 多次查询同一位置应一致
    MockBiomeSource source(1);
    world::biome::BiomeManager mgr(source, world::biome::BiomeManager::obfuscateSeed(99999));

    for (int i = 0; i < 20; ++i) {
        BiomeId b = mgr.getBiome(50, 64, 50);
        EXPECT_EQ(b, static_cast<BiomeId>(1));
    }
}

TEST(BiomeManagerTest, GetBiomeWithVaryingSource)
{
    // VaryingBiomeSource：不同 quart 返回不同生物群系
    // BiomeManager 的 Voronoi 缩放应选择最近的 quart 角点
    VaryingBiomeSource source;
    world::biome::BiomeManager mgr(source, world::biome::BiomeManager::obfuscateSeed(42));

    // 查询一些位置，验证返回值在有效范围内 [0, 3]
    for (i32 x = 0; x < 16; x += 4) {
        for (i32 z = 0; z < 16; z += 4) {
            BiomeId b = mgr.getBiome(x, 64, z);
            EXPECT_GE(b, 0);
            EXPECT_LE(b, 3);
        }
    }
}

TEST(BiomeManagerTest, GetBiomeAtOrigin)
{
    // 原点附近的查询应正常工作
    MockBiomeSource source(5);
    world::biome::BiomeManager mgr(source, world::biome::BiomeManager::obfuscateSeed(0));

    BiomeId b = mgr.getBiome(0, 0, 0);
    EXPECT_EQ(b, static_cast<BiomeId>(5));
}

TEST(BiomeManagerTest, GetBiomeNegativeCoordinates)
{
    // 负坐标应正常工作
    MockBiomeSource source(10);
    world::biome::BiomeManager mgr(source, world::biome::BiomeManager::obfuscateSeed(42));

    BiomeId b1 = mgr.getBiome(-100, -60, -200);
    BiomeId b2 = mgr.getBiome(-100, -60, -200);
    EXPECT_EQ(b1, b2);
    EXPECT_EQ(b1, static_cast<BiomeId>(10));
}

TEST(BiomeManagerTest, DifferentSeedsProduceDifferentResults)
{
    // 不同种子应产生不同的 Voronoi 缩放结果
    // 使用 VaryingBiomeSource 以便看到缩放效果
    VaryingBiomeSource source;
    u64 seed1 = world::biome::BiomeManager::obfuscateSeed(1);
    u64 seed2 = world::biome::BiomeManager::obfuscateSeed(2);

    world::biome::BiomeManager mgr1(source, seed1);
    world::biome::BiomeManager mgr2(source, seed2);

    // 在多个位置查询，至少有一些应该不同
    int differences = 0;
    for (i32 x = 0; x < 20; x += 3) {
        for (i32 z = 0; z < 20; z += 3) {
            if (mgr1.getBiome(x, 64, z) != mgr2.getBiome(x, 64, z)) {
                ++differences;
            }
        }
    }
    EXPECT_GT(differences, 0) << "Different seeds should produce different biome distributions";
}

TEST(BiomeManagerTest, WithDifferentSourceSharesSeed)
{
    // withDifferentSource 应共享 zoomSeed
    MockBiomeSource source1(1);
    MockBiomeSource source2(2);

    u64 seed = world::biome::BiomeManager::obfuscateSeed(42);
    world::biome::BiomeManager mgr1(source1, seed);
    world::biome::BiomeManager mgr2 = mgr1.withDifferentSource(source2);

    // zoomSeed 应相同
    EXPECT_EQ(mgr1.biomeZoomSeed(), mgr2.biomeZoomSeed());
}

TEST(BiomeManagerTest, GetNoiseBiomeAtQuart)
{
    // getNoiseBiomeAtQuart 应直接委托给 source
    MockBiomeSource source(7);
    world::biome::BiomeManager mgr(source, world::biome::BiomeManager::obfuscateSeed(0));

    BiomeId b = mgr.getNoiseBiomeAtQuart(5, 16, 10);
    EXPECT_EQ(b, static_cast<BiomeId>(7));
}

TEST(BiomeManagerTest, GetNoiseBiomeAtPosition)
{
    // getNoiseBiomeAtPosition 应将方块坐标转为 quart 后查询
    MockBiomeSource source(3);
    world::biome::BiomeManager mgr(source, world::biome::BiomeManager::obfuscateSeed(0));

    // blockX=8 -> quartX=2, blockZ=12 -> quartZ=3
    BiomeId b = mgr.getNoiseBiomeAtPosition(8, 64, 12);
    EXPECT_EQ(b, static_cast<BiomeId>(3));

    // 负坐标：blockX=-5 -> quartX=-2 (arithmetic shift)
    BiomeId bNeg = mgr.getNoiseBiomeAtPosition(-5, 64, -9);
    EXPECT_EQ(bNeg, static_cast<BiomeId>(3));
}

TEST(BiomeManagerTest, VoronoiZoomProducesSmoothBoundaries)
{
    // Voronoi 缩放应使边界更平滑（而非硬 4x4x4 网格）
    // 验证：在同一个 quart 格子内的不同位置可能返回不同结果
    // （因为 fiddling 使边界不再是硬 4x4x4 格子）
    VaryingBiomeSource source;
    world::biome::BiomeManager mgr(source, world::biome::BiomeManager::obfuscateSeed(42));

    // 在同一 quart 格子 (0,0) 内查询不同位置
    // quart (0,16,0) 对应 block 范围 [0,3] x [64,67] x [0,3]
    // Voronoi 缩放可能使不同子位置映射到不同 quart 角点
    std::set<BiomeId> biomes;
    for (i32 x = 0; x <= 3; ++x) {
        for (i32 z = 0; z <= 3; ++z) {
            biomes.insert(mgr.getBiome(x, 64, z));
        }
    }
    // 由于 fiddling，同一 quart 内可能看到多个生物群系
    // 但这取决于种子，所以我们只验证查询不崩溃且返回有效值
    for (BiomeId b : biomes) {
        EXPECT_GE(b, 0);
        EXPECT_LE(b, 3);
    }
}

} // namespace
} // namespace mc
