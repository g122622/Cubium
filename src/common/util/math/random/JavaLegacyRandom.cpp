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

#include "JavaLegacyRandom.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::math {

JavaLegacyRandom::JavaLegacyRandom(u64 seed)
{
    setSeed(seed);
}

void JavaLegacyRandom::setSeed(u64 seed)
{
    // Java: this.seed = (seed ^ MULTIPLIER) & MASK;
    m_state = (seed ^ MULTIPLIER) & MASK;
    // IRandom 约定：setSeed 清除高斯缓存
    m_hasGaussian = false;
}

i32 JavaLegacyRandom::next(i32 bits)
{
    // Java: seed = (seed * MULTIPLIER + INCREMENT) & MASK;
    //       return (int)(seed >>> (48 - bits));
    m_state = (m_state * MULTIPLIER + INCREMENT) & MASK;
    return static_cast<i32>(m_state >> (48 - bits));
}

u64 JavaLegacyRandom::nextU64()
{
    // 使用两个 next(32) 调用组合成 64 位值
    return (static_cast<u64>(static_cast<u32>(next(32))) << 32) | static_cast<u64>(static_cast<u32>(next(32)));
}

i32 JavaLegacyRandom::nextInt()
{
    return next(32);
}

i32 JavaLegacyRandom::nextInt(i32 bound)
{
    // Java: Random.nextInt(bound) - 拒绝采样法
    // bound 必须为正：Java 原版对非正 bound 抛 IllegalArgumentException，此处用断言暴露调用方缺陷。
    MC_ASSERT_RELEASE(bound > 0);

    if ((bound & (bound - 1)) == 0) {
        // 2 的幂：直接取高位
        return static_cast<i32>((static_cast<u64>(bound) * static_cast<u64>(next(31))) >> 31);
    }

    // 非 2 的幂：拒绝采样
    i32 bits = 0;
    i32 val = 0;
    u32 rejected = 0;
    do {
        bits = next(31);
        val = bits % bound;
        // Java 的 `bits - val + (bound - 1) < 0` 依赖 int 回绕判定溢出，
        // 而 C++ 的有符号溢出是 UB，故用 u32 显式回绕后按有符号解释，逐位复刻 Java 语义。
        rejected = static_cast<u32>(bits) - static_cast<u32>(val) + static_cast<u32>(bound - 1);
    } while (static_cast<i32>(rejected) < 0);

    return val;
}

bool JavaLegacyRandom::nextBoolean()
{
    return next(1) != 0;
}

f32 JavaLegacyRandom::nextFloat()
{
    // Java: next(24) / (1 << 24)
    return static_cast<f32>(next(24)) / static_cast<f32>(1 << 24);
}

f64 JavaLegacyRandom::nextDouble()
{
    // Java: RandomSource.nextDouble()（BitRandomSource 的默认实现）
    //   int i = this.next(26);
    //   int j = this.next(27);
    //   long k = ((long)i << 27) + j;
    //   return k * 1.110223E-16F;
    // 【字面量是 float 后缀】1.110223E-16F 的值恰好等于 2^-53（2 的整数次幂在 float
    // 里可精确表示），但 Java 的 `long * float` 会做二元数值提升——k 先被拓宽为
    // **float**（丢 29 位有效位）再做 float 乘法，结果只有 24 位精度，最后才拓宽为
    // double 返回。写成 double 除法会得到 53 位精度的不同值，进而让 PerlinNoise 的
    // 原点偏移（nextDouble() * 256.0）在 1e-5 量级上偏离原版，整片地形随之分叉。
    const i64 k = (static_cast<i64>(next(26)) << 27) + static_cast<i64>(next(27));
    return static_cast<f64>(static_cast<f32>(k) * 1.1102230246251565E-16f);
}

i64 JavaLegacyRandom::nextLong()
{
    // Java: return ((i64)next(32) << 32) + (i64)next(32);
    return (static_cast<i64>(next(32)) << 32) + static_cast<i64>(next(32));
}

void JavaLegacyRandom::skip(u64 count)
{
    // IRandom::skip - 推进 count 步
    for (u64 i = 0; i < count; ++i) {
        (void)next(1);
    }
}

PositionalRandomFactory JavaLegacyRandom::forkPositional()
{
    // Java: LegacyRandomSource.forkPositional() → new LegacyPositionalRandomFactory(this.nextLong())
    return PositionalRandomFactory(static_cast<u64>(nextLong()));
}

void JavaLegacyRandom::consumeCount(i32 count)
{
    // Java: RandomSource.consumeCount(count) - 推进 count 步
    for (i32 i = 0; i < count; ++i) {
        (void)next(1);
    }
}

} // namespace mc::math
