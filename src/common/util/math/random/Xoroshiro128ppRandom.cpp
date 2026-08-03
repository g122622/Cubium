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

#include "Xoroshiro128ppRandom.hpp"
#include "PositionalRandomFactory.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <cstddef>

namespace mc::math {

// ============================================================================
// Stafford13 混合函数（与 MC RandomSupport.mixStafford13 一致）
// ============================================================================

static u64 mixStafford13(u64 z)
{
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

// ============================================================================

Xoroshiro128ppRandom::Xoroshiro128ppRandom(u64 seed)
{
    setSeed(seed);
}

Xoroshiro128ppRandom::Xoroshiro128ppRandom(u64 seedLo, u64 seedHi)
{
    m_state[0] = seedLo;
    m_state[1] = seedHi;

    // 与 MC Xoroshiro128PlusPlus 一致：全零状态时使用默认值
    // 这些默认值与 upgradeSeedTo128bit 中使用的常量一致
    if ((m_state[0] | m_state[1]) == 0ULL) {
        m_state[0] = 0x9e3779b97f4a7c15ULL; // SILVER_RATIO_64
        m_state[1] = 0x6a09e667f3bcc909ULL; // GOLDEN_RATIO_64
    }

    m_hasGaussian = false;
}

void Xoroshiro128ppRandom::setSeed(u64 seed)
{
    // MC 1.21: XoroshiroRandomSource.setSeed(long)
    // 使用 upgradeSeedTo128bit 扩展种子，而非 SplitMix64
    // Java 流程：
    //   long lo = mixStafford13(seed ^ SILVER_RATIO_64)
    //   long hi = mixStafford13(seed + GOLDEN_RATIO_64)
    //   new Xoroshiro128PlusPlus(Seed128bit(lo, hi))
    //
    // 其中 SILVER_RATIO_64 = 0x9e3779b97f4a7c15L
    //      GOLDEN_RATIO_64 = 0x6a09e667f3bcc909L
    constexpr u64 SILVER_RATIO_64 = 0x9e3779b97f4a7c15ULL;
    constexpr u64 GOLDEN_RATIO_64 = 0x6a09e667f3bcc909ULL;

    m_state[0] = mixStafford13(seed ^ SILVER_RATIO_64);
    m_state[1] = mixStafford13(seed + GOLDEN_RATIO_64);

    // 与 MC Xoroshiro128PlusPlus 一致：全零状态时使用默认值
    if ((m_state[0] | m_state[1]) == 0ULL) {
        m_state[0] = SILVER_RATIO_64;
        m_state[1] = GOLDEN_RATIO_64;
    }

    m_hasGaussian = false;
}

u64 Xoroshiro128ppRandom::nextU64()
{
    const u64 s0 = m_state[0];
    u64 s1 = m_state[1];

    // xoroshiro128++ 核心算法
    const u64 result = rotl(s0 + s1, 17) + s0;

    // 更新状态
    s1 ^= s0;
    m_state[0] = rotl(s0, 49) ^ s1 ^ (s1 << 21);
    m_state[1] = rotl(s1, 28);

    return result;
}

f64 Xoroshiro128ppRandom::nextDouble()
{
    // MC XoroshiroRandomSource.nextDouble():
    //   return (double)((float)(this.nextLong() >>> 11) * 1.1102230246251565E-16F);
    // Java 的 >>> 是无符号右移，C++ 需要先转 u64 再右移
    // Java 二元数值提升：long * float → long 拓宽为 float（约24位精度），乘法结果为 float
    // 然后 float 拓宽为 double 返回
    const u64 val = static_cast<u64>(nextLong()) >> 11;
    return static_cast<f64>(static_cast<f32>(static_cast<f64>(val)) * 1.1102230246251565E-16f);
}

f32 Xoroshiro128ppRandom::nextFloat()
{
    // MC XoroshiroRandomSource.nextFloat():
    //   return (float)this.next(24) * 5.9604645E-8F;
    // next(24) = (int)(nextLong() >>> 40)，取高24位
    // Java 的 >>> 是无符号右移
    const i32 bits = static_cast<i32>(static_cast<u64>(nextLong()) >> 40);
    return static_cast<f32>(bits) * 5.9604645E-8f;
}

i32 Xoroshiro128ppRandom::nextInt(i32 bound)
{
    // MC XoroshiroRandomSource.nextInt(int) — Lemire nearly-divisionless algorithm
    // Java 源码 (XoroshiroRandomSource.java):
    //   long i = Integer.toUnsignedLong(this.nextInt());
    //   long j = i * p_190118_;
    //   long k = j & 4294967295L;
    //   if (k < p_190118_) {
    //       for (int l = Integer.remainderUnsigned(~p_190118_ + 1, p_190118_); k < l; k = j & 4294967295L) {
    //           i = Integer.toUnsignedLong(this.nextInt());
    //           j = i * p_190118_;
    //       }
    //   }
    //   return (int)(j >> 32);
    MC_ASSERT_RELEASE(bound > 0);

    // this.nextInt() = (int)nextLong() — 低32位作为有符号int
    // Integer.toUnsignedLong: 将 int 视为无符号 32 位转为 long
    const u64 i = static_cast<u64>(static_cast<u32>(nextLong())); // nextInt() as unsigned
    u64 j = i * static_cast<u64>(bound);
    u64 k = j & 0xFFFFFFFFULL; // 低 32 位

    if (k < static_cast<u64>(bound)) {
        // Integer.remainderUnsigned(~bound + 1, bound) = (-bound) % bound as unsigned
        // ~bound + 1 = -bound (two's complement)
        const u64 threshold = (static_cast<u64>(-static_cast<i64>(bound))) % static_cast<u64>(bound);
        while (k < threshold) {
            j = static_cast<u64>(static_cast<u32>(nextLong())) * static_cast<u64>(bound);
            k = j & 0xFFFFFFFFULL;
        }
    }

    return static_cast<i32>(j >> 32);
}

void Xoroshiro128ppRandom::skip(u64 /* count */)
{
    // xoroshiro128++ 的快速跳转
    // 使用预计算的跳转多项式
    // 参考 http://xoroshiro.di.unimi.it/xoroshiro128plusplus.c
    // 注意：参数 count 被忽略，因为快速跳转每次跳过 2^64 个状态
    // 这是 xoroshiro/xoshiro 系列算法的标准实现方式

    static const u64 JUMP[] = {0x2bd7a6a6e99c2ddcULL, 0x0992ccaf6a6fca05ULL};

    u64 s0 = 0;
    u64 s1 = 0;

    for (size_t i = 0; i < sizeof(JUMP) / sizeof(JUMP[0]); ++i) {
        for (int b = 0; b < 64; ++b) {
            if (JUMP[i] & (1ULL << b)) {
                s0 ^= m_state[0];
                s1 ^= m_state[1];
            }
            (void)nextU64(); // 故意丢弃返回值，用于状态更新
        }
    }

    m_state[0] = s0;
    m_state[1] = s1;
}

PositionalRandomFactory Xoroshiro128ppRandom::forkPositional()
{
    // MC 1.21: 消耗两个 nextLong() 获取 128 位种子创建工厂
    const u64 seedLo = static_cast<u64>(nextLong());
    const u64 seedHi = static_cast<u64>(nextLong());
    return PositionalRandomFactory(seedLo, seedHi);
}

} // namespace mc::math
