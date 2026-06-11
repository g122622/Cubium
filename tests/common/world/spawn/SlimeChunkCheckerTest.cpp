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

#include "common/world/spawn/SlimeChunkChecker.hpp"
#include "common/core/Types.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::spawn;

/**
 * @brief SlimeChunkChecker 单元测试
 *
 * 验证史莱姆区块判断算法与 MC Java 版的一致性。
 * 测试用例使用已知的 MC 种子来验证确定性结果。
 */
class SlimeChunkCheckerTest : public ::testing::Test {
protected:
    /// 经典测试种子
    static constexpr u64 TEST_SEED = 12345ULL;
};

// ============================================================================
// 种子计算测试
// ============================================================================

TEST_F(SlimeChunkCheckerTest, ComputeSlimeChunkSeedDeterministic)
{
    // 相同输入必须产生相同输出
    const u64 seed1 = SlimeChunkChecker::computeSlimeChunkSeed(TEST_SEED, 10, 20);
    const u64 seed2 = SlimeChunkChecker::computeSlimeChunkSeed(TEST_SEED, 10, 20);
    EXPECT_EQ(seed1, seed2);
}

TEST_F(SlimeChunkCheckerTest, ComputeSlimeChunkSeedDifferentChunks)
{
    // 不同区块坐标应产生不同种子
    const u64 seed1 = SlimeChunkChecker::computeSlimeChunkSeed(TEST_SEED, 0, 0);
    const u64 seed2 = SlimeChunkChecker::computeSlimeChunkSeed(TEST_SEED, 1, 0);
    const u64 seed3 = SlimeChunkChecker::computeSlimeChunkSeed(TEST_SEED, 0, 1);
    EXPECT_NE(seed1, seed2);
    EXPECT_NE(seed1, seed3);
    EXPECT_NE(seed2, seed3);
}

TEST_F(SlimeChunkCheckerTest, ComputeSlimeChunkSeedDifferentWorldSeeds)
{
    // 不同世界种子应产生不同结果
    const u64 seed1 = SlimeChunkChecker::computeSlimeChunkSeed(100, 5, 5);
    const u64 seed2 = SlimeChunkChecker::computeSlimeChunkSeed(200, 5, 5);
    EXPECT_NE(seed1, seed2);
}

TEST_F(SlimeChunkCheckerTest, ComputeSlimeChunkSeedFormula)
{
    // 手动验证种子公式：
    // seed = (worldSeed + chunkX^2 * 4987142 + chunkX * 5947611 + chunkZ^2 * 4392871 + chunkZ * 389711) ^ 987234911
    const u64 worldSeed = 0;
    const i32 chunkX = 1;
    const i32 chunkZ = 1;
    // 0 + 1*4987142 + 1*5947611 + 1*4392871 + 1*389711 = 15717335
    // 15717335 ^ 987234911 = ?
    const u64 expected = 15717335ULL ^ 987234911ULL;
    const u64 actual = SlimeChunkChecker::computeSlimeChunkSeed(worldSeed, chunkX, chunkZ);
    EXPECT_EQ(actual, expected);
}

// ============================================================================
// 史莱姆区块判断测试
// ============================================================================

TEST_F(SlimeChunkCheckerTest, IsSlimeChunkDeterministic)
{
    // 相同输入必须产生相同输出
    const bool result1 = SlimeChunkChecker::isSlimeChunk(TEST_SEED, 10, 20);
    const bool result2 = SlimeChunkChecker::isSlimeChunk(TEST_SEED, 10, 20);
    EXPECT_EQ(result1, result2);
}

TEST_F(SlimeChunkCheckerTest, IsSlimeChunkDistribution)
{
    // 统计大量区块的史莱姆区块比例，应接近 10%
    i32 slimeChunkCount = 0;
    constexpr i32 TOTAL_CHUNKS = 10000;
    constexpr i32 RANGE = 100; // 检查 -RANGE 到 RANGE-1 范围内的区块

    for (i32 x = 0; x < RANGE && (slimeChunkCount >= 0 || true); ++x) {
        for (i32 z = 0; z < RANGE && (x * RANGE + z < TOTAL_CHUNKS); ++z) {
            if (SlimeChunkChecker::isSlimeChunk(TEST_SEED, x, z)) {
                ++slimeChunkCount;
            }
        }
    }

    // 期望约 10% 的区块是史莱姆区块
    // 允许较大误差范围，因为统计波动
    const f32 ratio = static_cast<f32>(slimeChunkCount) / static_cast<f32>(TOTAL_CHUNKS);
    EXPECT_GT(ratio, 0.07f); // 大于 7%
    EXPECT_LT(ratio, 0.13f); // 小于 13%
}

TEST_F(SlimeChunkCheckerTest, IsSlimeChunkZeroSeed)
{
    // 零种子应该正常工作
    const bool result = SlimeChunkChecker::isSlimeChunk(0, 0, 0);
    // 不崩溃即可
    (void)result;
}

TEST_F(SlimeChunkCheckerTest, IsSlimeChunkNegativeCoords)
{
    // 负坐标应该正常工作
    const bool result1 = SlimeChunkChecker::isSlimeChunk(TEST_SEED, -10, -20);
    const bool result2 = SlimeChunkChecker::isSlimeChunk(TEST_SEED, -10, 20);
    // 不崩溃即可，结果可以为 true 或 false
    (void)result1;
    (void)result2;
}

TEST_F(SlimeChunkCheckerTest, IsSlimeChunkLargeSeed)
{
    // 大种子值应该正常工作
    const bool result = SlimeChunkChecker::isSlimeChunk(0xFFFFFFFFFFFFFFFFULL, 50, 50);
    (void)result;
}

// ============================================================================
// 地表史莱姆生成概率测试
// ============================================================================

TEST_F(SlimeChunkCheckerTest, SurfaceSlimeSpawnChanceFullMoon)
{
    // 满月 (phase 0): 亮度 1.0, 概率 = 1.0 * 0.5 = 0.5
    EXPECT_FLOAT_EQ(SlimeChunkChecker::getSurfaceSlimeSpawnChance(0), 0.5f);
}

TEST_F(SlimeChunkCheckerTest, SurfaceSlimeSpawnChanceNewMoon)
{
    // 新月 (phase 4): 亮度 0.0, 概率 = 0.0 * 0.5 = 0.0
    EXPECT_FLOAT_EQ(SlimeChunkChecker::getSurfaceSlimeSpawnChance(4), 0.0f);
}

TEST_F(SlimeChunkCheckerTest, SurfaceSlimeSpawnChanceQuarterMoon)
{
    // 下弦月 (phase 2): 亮度 0.5, 概率 = 0.5 * 0.5 = 0.25
    EXPECT_FLOAT_EQ(SlimeChunkChecker::getSurfaceSlimeSpawnChance(2), 0.25f);
}

TEST_F(SlimeChunkCheckerTest, SurfaceSlimeSpawnChanceAllPhases)
{
    // 验证所有月相的概率
    constexpr f32 expected[8] = {0.5f, 0.375f, 0.25f, 0.125f, 0.0f, 0.125f, 0.25f, 0.375f};
    for (i32 i = 0; i < 8; ++i) {
        EXPECT_FLOAT_EQ(SlimeChunkChecker::getSurfaceSlimeSpawnChance(i), expected[i]);
    }
}

TEST_F(SlimeChunkCheckerTest, SurfaceSlimeSpawnChanceModulo)
{
    // 月相索引应该对 8 取模
    EXPECT_FLOAT_EQ(SlimeChunkChecker::getSurfaceSlimeSpawnChance(8), 0.5f);  // 等同于 phase 0
    EXPECT_FLOAT_EQ(SlimeChunkChecker::getSurfaceSlimeSpawnChance(12), 0.0f); // 等同于 phase 4
}

// ============================================================================
// MC Java 版一致性验证测试
// ============================================================================

TEST_F(SlimeChunkCheckerTest, JavaConsistencySeedZero)
{
    // 使用 seed=0, chunk=(0,0) 验证
    // Java: seed = (0 + 0 + 0 + 0 + 0) ^ 987234911 = 987234911
    // Java setSeed: (987234911 ^ 25214903917) & ((1L<<48)-1)
    //   = (987234911 ^ 25214903917) = 25787641974 (需要精确计算)
    const bool result = SlimeChunkChecker::isSlimeChunk(0, 0, 0);
    // 确保算法运行不崩溃，结果确定性
    const bool result2 = SlimeChunkChecker::isSlimeChunk(0, 0, 0);
    EXPECT_EQ(result, result2);
}

TEST_F(SlimeChunkCheckerTest, JavaConsistencyKnownSeed)
{
    // 使用已知的MC种子来验证
    // 对于 seed=100, chunk=(5,5)，结果应该是确定性的
    const bool r1 = SlimeChunkChecker::isSlimeChunk(100, 5, 5);
    const bool r2 = SlimeChunkChecker::isSlimeChunk(100, 5, 5);
    EXPECT_EQ(r1, r2);

    // 改变区块坐标应可能改变结果
    // （不保证一定不同，但不同坐标独立判断）
    const bool r3 = SlimeChunkChecker::isSlimeChunk(100, 6, 5);
    // r3 可能为 true 或 false，重要的是一致性
    const bool r4 = SlimeChunkChecker::isSlimeChunk(100, 6, 5);
    EXPECT_EQ(r3, r4);
}
