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

// ============================================================================
// WorldgenRandom 原版黄金值回归测试
//
// 本文件是**外部真值**门禁：期望值由一份逐行复刻 MC 1.21.11 WorldgenRandom /
// BitRandomSource / XoroshiroRandomSource / RandomSupport 的 Java 参考实现独立算出，
// 不依赖本项目任何一次运行，因此能捕捉"实现与实现自洽、但与原版不一致"。
//
// 为什么必须单独立测：WorldgenRandom 的语义反直觉——它把所有位宽抽取都折算成
// "反复调用内层 nextLong() 取高 bits 位"，于是 nextLong() 会消耗**两个**内层
// nextLong 并各取高 32 位，与直接在内层上调 nextLong()（消耗一个、取全 64 位）
// 完全不同。若误用内层，placeFeatures 的 decorSeed 会整体偏移（实测
// chunk(2,-2) 0xF53DCD247D852AF7 vs 0x91D8CBEBCC2C2BF7），进而使**所有**
// placed_feature 的 setFeatureSeed 种子错位，表现为石头变体/矿石/树木的大规模
// 落位偏差——但没有任何既有测试能发现。
// ============================================================================

#include "common/core/Types.hpp"
#include "common/util/math/random/WorldgenRandom.hpp"
#include "common/util/math/random/Xoroshiro128ppRandom.hpp"

#include <array>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace {

using math::WorldgenRandom;
using math::Xoroshiro128ppRandom;

/**
 * @brief 构造被测对象：WorldgenRandom 包装 XoroshiroRandomSource(seed)
 */
std::unique_ptr<WorldgenRandom> makeWorldgenRandom(u64 seed)
{
    return std::make_unique<WorldgenRandom>(std::make_unique<Xoroshiro128ppRandom>(seed));
}

// ============================================================================
// setDecorationSeed：parity 测试三个目标区块的 decorSeed 必须逐位一致
// ============================================================================
//
// 原版路径（ChunkGenerator.applyBiomeDecoration）：
//   WorldgenRandom worldgenrandom = new WorldgenRandom(new XoroshiroRandomSource(uniqueSeed));
//   long i = worldgenrandom.setDecorationSeed(level.getSeed(), blockpos.getX(), blockpos.getZ());
// 其中 uniqueSeed 来自 WorldgenRandom(new XoroshiroRandomSource(...)) 构造前的
// RandomSupport.generateUniqueSeed()——真正的原版运行里它每次都不同，
// 但种子生成器本身在构造时即重置，故 i = nextLong()|1 与 j = nextLong()|1
// **只取决于 levelSeed / blockX / blockZ**（setSeed 先于 nextLong 调用）。
// 因此下面的期望值可以固定。
TEST(WorldgenRandomVanillaGoldenTest, DecorationSeedMatchesVanilla)
{
    // level.dat 的 WorldGenSettings.seed
    constexpr u64 kLevelSeed = static_cast<u64>(-6671478382168981129LL);

    struct Case {
        i32 chunkX;
        i32 chunkZ;
        i64 expected;
    };
    // 期望值由 Java 参考实现导出（见文件头）
    const std::vector<Case> cases = {
        {2, -2, -775238004229461257LL},
        {2, 10, 3740127436111011639LL},
        {-10, -2, -5370338800552185161LL},
    };

    for (const auto& c : cases) {
        auto rng = makeWorldgenRandom(42ULL);
        const u64 decorSeed = rng->setDecorationSeed(kLevelSeed, c.chunkX * 16, c.chunkZ * 16);
        EXPECT_EQ(static_cast<i64>(decorSeed), c.expected)
            << "chunk(" << c.chunkX << "," << c.chunkZ << ") 的 decorSeed 与原版不一致（0x" << std::hex << decorSeed
            << " vs 0x" << c.expected << std::dec << "）";
    }
}

// ============================================================================
// 固定种子下的抽取序列：逐位对齐原版 BitRandomSource 的位分解
// ============================================================================

constexpr u64 kDrawSeed = 42ULL;

TEST(WorldgenRandomVanillaGoldenTest, NextIntBoundedMatchesVanilla)
{
    const std::array<i32, 12> expected = {306, 973, 898, 641, 384, 998, 89, 997, 151, 868, 782, 397};
    auto rng = makeWorldgenRandom(kDrawSeed);
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(rng->nextInt(1000), expected[i]) << "nextInt(1000) 第 " << i << " 次抽取不一致";
    }
}

TEST(WorldgenRandomVanillaGoldenTest, NextFloatMatchesVanilla)
{
    // Java: next(24) * 5.9604645E-8F，中间精度是 float
    const std::array<f32, 12> expected = {0.745432079f,
        0.397995055f,
        0.591107547f,
        0.265027225f,
        0.456430197f,
        0.648071468f,
        0.448829949f,
        0.338609278f,
        0.500398755f,
        0.541411698f,
        0.499060690f,
        0.492290735f};
    auto rng = makeWorldgenRandom(kDrawSeed);
    for (size_t i = 0; i < expected.size(); ++i) {
        // 期望值以 %.9g 打印，float 可精确往返，故用 EXPECT_FLOAT_EQ（4 ulp）
        EXPECT_FLOAT_EQ(rng->nextFloat(), expected[i]) << "nextFloat() 第 " << i << " 次抽取不一致";
    }
}

TEST(WorldgenRandomVanillaGoldenTest, NextLongMatchesVanilla)
{
    // nextLong() 消耗两个内层 nextLong、各取高 32 位再拼接——与内层 nextLong() 完全不同
    const std::array<i64, 12> expected = {-4695948378802814517LL,
        -7542733517267348717LL,
        8419651029966521324LL,
        8279452172130654682LL,
        -9216016241610066487LL,
        9206045780119831739LL,
        1714950527300168706LL,
        -7688159163963057199LL,
        -3066404184412101200LL,
        -1441244675890220626LL,
        -3782256110022334102LL,
        5120018415295827939LL};
    auto rng = makeWorldgenRandom(kDrawSeed);
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(rng->nextLong(), expected[i]) << "nextLong() 第 " << i << " 次抽取不一致";
    }
}

TEST(WorldgenRandomVanillaGoldenTest, NextDoubleMatchesVanilla)
{
    // Java: (((long)next(26) << 27) + next(27)) * 1.110223E-16F
    // 常量是 float 字面量，long*float 的中间精度只有 24 位
    const std::array<f64, 10> expected = {0.74543213844299320,
        0.59110760688781740,
        0.45643019676208496,
        0.44883000850677490,
        0.50039875507354740,
        0.49906075000762940,
        0.092967644333839420,
        0.58322405815124510,
        0.83376991748809810,
        0.92186999320983890};
    auto rng = makeWorldgenRandom(kDrawSeed);
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_DOUBLE_EQ(rng->nextDouble(), expected[i]) << "nextDouble() 第 " << i << " 次抽取不一致";
    }
}

TEST(WorldgenRandomVanillaGoldenTest, NextBooleanMatchesVanilla)
{
    const std::string expected = "1010010011000010";
    auto rng = makeWorldgenRandom(kDrawSeed);
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(rng->nextBoolean(), expected[i] == '1') << "nextBoolean() 第 " << i << " 次抽取不一致";
    }
}

TEST(WorldgenRandomVanillaGoldenTest, NextIntPowerOfTwoBoundMatchesVanilla)
{
    // bound=64 是 2 的幂，走 `(long)bound * next(31) >> 31` 分支
    const std::array<i32, 10> expected = {47, 25, 37, 16, 29, 41, 28, 21, 32, 34};
    auto rng = makeWorldgenRandom(kDrawSeed);
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(rng->nextInt(64), expected[i]) << "nextInt(64) 第 " << i << " 次抽取不一致";
    }
}

// ============================================================================
// 结构性不变量：setFeatureSeed 之后 setSeed 必须透传到内层
// ============================================================================

TEST(WorldgenRandomVanillaGoldenTest, SetFeatureSeedIsDeterministicAcrossInstances)
{
    constexpr u64 kDecorSeed = 1234567890123456789ULL;
    auto a = makeWorldgenRandom(1ULL);
    auto b = makeWorldgenRandom(999ULL); // 不同初始种子，setFeatureSeed 后应完全一致
    a->setFeatureSeed(kDecorSeed, 3, 5);
    b->setFeatureSeed(kDecorSeed, 3, 5);
    for (i32 i = 0; i < 8; ++i) {
        EXPECT_EQ(a->nextInt(1000), b->nextInt(1000)) << "setFeatureSeed 后第 " << i << " 次抽取不一致";
    }
}

} // namespace
} // namespace mc
