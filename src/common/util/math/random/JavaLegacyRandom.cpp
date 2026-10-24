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
    //       return (i32)(seed >>> (48 - bits));
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
        // Java 的 `bits - val + (bound - 1) < 0` 依赖 i32 回绕判定溢出，
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
    // Java: LegacyRandomSource.nextDouble()（java.util.Random 的原版公式）
    //   (((long) next(26) << 27) + next(27)) * 0x1.0p-53
    // next(b) 的结果非负（state >>> (48 - b)），所以拼接后是 [0, 2^53) 的 53 位整数；
    // 乘数 2^-53 是 f64 精确值，中间**不**经过 f32（与 BitRandomSource 的写法不同）。
    const u64 high = static_cast<u32>(next(26));
    const u64 low = static_cast<u32>(next(27));
    return static_cast<f64>((high << 27) | low) * 1.1102230246251565E-16;
}

i64 JavaLegacyRandom::nextLong()
{
    // Java: return ((i64)next(32) << 32) + (i64)next(32);
    // 按调用顺序抽取两个半部，低半部仍需符号扩展；最终加法按 64 位回绕。
    const u64 hi = static_cast<u64>(next(32));
    const u64 lo = static_cast<u64>(next(32));
    return static_cast<i64>((hi << 32) + lo);
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
