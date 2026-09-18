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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN EVENT OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/util/math/random/JavaLegacyRandom.hpp"
#include <gtest/gtest.h>

namespace mc {
namespace {

// ============================================================================
// JavaLegacyRandom 种子设置和状态测试
// ============================================================================

TEST(JavaLegacyRandomTest, SetSeedInitializesState)
{
    // Java: state = (seed ^ 0x5DEECE66D) & ((1L << 48) - 1)
    // seed = 0 -> state = (0 ^ 0x5DEECE66D) & mask = 0x5DEECE66D & mask
    // 0x5DEECE66D = 25214903917
    // (1L << 48) - 1 = 281474976710655
    // 25214903917 & 281474976710655 = 25214903917 (fits in 48 bits)
    math::JavaLegacyRandom rng(0);
    // Just test that next() produces a finite value
    i32 value = rng.next(32);
    (void)value; // Should not crash
}

TEST(JavaLegacyRandomTest, SameSeedProducesSameSequence)
{
    math::JavaLegacyRandom rng1(12345);
    math::JavaLegacyRandom rng2(12345);

    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(rng1.next(32), rng2.next(32)) << "Sequence mismatch at step " << i;
    }
}

TEST(JavaLegacyRandomTest, DifferentSeedsProduceDifferentSequences)
{
    math::JavaLegacyRandom rng1(12345);
    math::JavaLegacyRandom rng2(54321);

    bool anyDifferent = false;
    for (int i = 0; i < 10; ++i) {
        if (rng1.next(32) != rng2.next(32)) {
            anyDifferent = true;
            break;
        }
    }
    EXPECT_TRUE(anyDifferent) << "Different seeds should produce different sequences";
}

// ============================================================================
// JavaLegacyRandom next() 测试
// ============================================================================

TEST(JavaLegacyRandomTest, NextBitsRange)
{
    // next(bits) 应返回 [0, 2^bits) 范围内的值
    math::JavaLegacyRandom rng(42);

    for (int i = 0; i < 1000; ++i) {
        i32 value = rng.next(8);
        EXPECT_GE(value, 0);
        EXPECT_LT(value, 256); // 2^8
    }
}

TEST(JavaLegacyRandomTest, Next32Bits)
{
    math::JavaLegacyRandom rng(42);
    i32 value = rng.next(32);
    // next(32) 可以返回任何 i32 值
    (void)value; // 只验证不崩溃
}

// ============================================================================
// JavaLegacyRandom nextInt() 测试
// ============================================================================

TEST(JavaLegacyRandomTest, NextIntBoundRange)
{
    math::JavaLegacyRandom rng(42);
    const i32 bound = 100;

    for (int i = 0; i < 1000; ++i) {
        i32 value = rng.nextInt(bound);
        EXPECT_GE(value, 0) << "nextInt(bound) returned negative at step " << i;
        EXPECT_LT(value, bound) << "nextInt(bound) exceeded bound at step " << i;
    }
}

TEST(JavaLegacyRandomTest, NextIntPowerOfTwo)
{
    // 2 的幂边界应使用快速路径
    math::JavaLegacyRandom rng(42);
    const i32 bound = 64;

    for (int i = 0; i < 100; ++i) {
        i32 value = rng.nextInt(bound);
        EXPECT_GE(value, 0);
        EXPECT_LT(value, bound);
    }
}

TEST(JavaLegacyRandomTest, NextIntDeterminism)
{
    math::JavaLegacyRandom rng1(42);
    math::JavaLegacyRandom rng2(42);

    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(rng1.nextInt(256), rng2.nextInt(256)) << "nextInt determinism mismatch at step " << i;
    }
}

// ============================================================================
// JavaLegacyRandom nextLong() 测试
// ============================================================================

TEST(JavaLegacyRandomTest, NextLongDeterminism)
{
    math::JavaLegacyRandom rng1(42);
    math::JavaLegacyRandom rng2(42);

    for (int i = 0; i < 50; ++i) {
        EXPECT_EQ(rng1.nextLong(), rng2.nextLong()) << "nextLong determinism mismatch at step " << i;
    }
}

TEST(JavaLegacyRandomTest, NextLongDiversity)
{
    // nextLong 应产生多样的 64 位值
    math::JavaLegacyRandom rng(42);
    std::set<i64> values;
    for (int i = 0; i < 100; ++i) {
        values.insert(rng.nextLong());
    }
    // 100 个 nextLong 值中应至少有 95 个不同的
    EXPECT_GT(values.size(), 95u) << "nextLong should produce diverse values";
}

// ============================================================================
// JavaLegacyRandom consumeCount() 测试
// ============================================================================

TEST(JavaLegacyRandomTest, ConsumeCountAdvancesState)
{
    // consumeCount(n) 应等价于调用 n 次 next(1)
    math::JavaLegacyRandom rng1(42);
    math::JavaLegacyRandom rng2(42);

    rng1.consumeCount(262);

    for (int i = 0; i < 262; ++i) {
        rng2.next(1);
    }

    // 两者现在应产生相同的序列
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(rng1.next(32), rng2.next(32)) << "consumeCount mismatch at step " << i;
    }
}

TEST(JavaLegacyRandomTest, ConsumeCountZeroIsNoOp)
{
    math::JavaLegacyRandom rng1(42);
    math::JavaLegacyRandom rng2(42);

    rng1.consumeCount(0);

    // 不消耗任何步骤，应与原始序列相同
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(rng1.next(32), rng2.next(32)) << "consumeCount(0) should be no-op";
    }
}

// ============================================================================
// JavaLegacyRandom 与 MC Java 对齐的关键种子操作测试
// ============================================================================

TEST(JavaLegacyRandomTest, JavaRandomSeedXor)
{
    // Java: setSeed(seed) -> state = (seed ^ MULTIPLIER) & MASK
    // MULTIPLIER = 25214903917 = 0x5DEECE66D
    // seed = 1 -> state = (1 ^ 0x5DEECE66D) & MASK = 0x5DEECE66C & MASK
    // 0x5DEECE66C = 25214903916
    // 25214903916 fits in 48 bits, so state = 25214903916
    math::JavaLegacyRandom rng(1);
    // After setSeed(1), next(32) should give a deterministic value
    i32 value = rng.next(32);
    // We just verify it's deterministic
    math::JavaLegacyRandom rng2(1);
    EXPECT_EQ(value, rng2.next(32));
}

TEST(JavaLegacyRandomTest, SeedZeroProducesDeterministicSequence)
{
    math::JavaLegacyRandom rng(0);
    // seed = 0 -> state = (0 ^ 0x5DEECE66D) & MASK = 0x5DEECE66D & MASK
    std::vector<i32> values;
    for (int i = 0; i < 10; ++i) {
        values.push_back(rng.next(32));
    }
    // Verify determinism
    math::JavaLegacyRandom rng2(0);
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(values[static_cast<size_t>(i)], rng2.next(32)) << "Mismatch at step " << i;
    }
}

} // namespace
} // namespace mc
