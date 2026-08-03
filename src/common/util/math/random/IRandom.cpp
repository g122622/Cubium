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

#include "IRandom.hpp"
#include "../../assert/AssertAll.hpp"
#include "common/core/Types.hpp"
#include <cmath>
#include <limits>

namespace mc::math {

// ============================================================================
// 默认实现
// ============================================================================

u32 IRandom::nextU32()
{
    return static_cast<u32>(nextU64() >> 32);
}

i32 IRandom::nextInt()
{
    return static_cast<i32>(nextU32());
}

i32 IRandom::nextInt(i32 bound)
{
    MC_ASSERT_RELEASE(bound > 0);

    // MC 风格的无偏差随机数生成
    // 参考 MC Random.nextInt(int)
    u32 r = nextU32();
    u32 m = static_cast<u32>(bound);
    u32 u = r % m;

    // 如果 r % bound 产生的结果有偏差，则重新采样
    // 这是因为 2^32 不是 bound 的倍数
    if (r - u > std::numeric_limits<u32>::max() - m + 1) {
        // 需要重新采样
        do {
            r = nextU32();
        } while (r >= std::numeric_limits<u32>::max() - (std::numeric_limits<u32>::max() % m));
        u = r % m;
    }

    return static_cast<i32>(u);
}

i32 IRandom::nextInt(i32 min, i32 max)
{
    MC_ASSERT_RELEASE(min <= max);
    return min + nextInt(max - min + 1);
}

bool IRandom::nextBoolean()
{
    return (nextU64() & 1) != 0;
}

f32 IRandom::nextFloat()
{
    // 返回 [0.0, 1.0) 范围的浮点数
    // 使用 24 位精度（float 的尾数位）
    return static_cast<f32>(nextU64() >> 40) / static_cast<f32>(1ULL << 24);
}

f32 IRandom::nextFloat(f32 min, f32 max)
{
    return min + nextFloat() * (max - min);
}

f64 IRandom::nextDouble()
{
    // 返回 [0.0, 1.0) 范围的双精度浮点数
    // 使用 53 位精度（double 的尾数位）
    return static_cast<f64>((nextU64() >> 11)) / static_cast<f64>(1ULL << 53);
}

f64 IRandom::nextDouble(f64 min, f64 max)
{
    return min + nextDouble() * (max - min);
}

f32 IRandom::nextGaussian(f32 mean, f32 stddev)
{
    // Marsaglia polar method
    // 参考 MC Random.nextGaussian()
    if (m_hasGaussian) {
        m_hasGaussian = false;
        return mean + stddev * m_nextGaussian;
    }

    f32 v1, v2, s;
    do {
        v1 = 2.0f * nextFloat() - 1.0f; // [-1, 1]
        v2 = 2.0f * nextFloat() - 1.0f; // [-1, 1]
        s = v1 * v1 + v2 * v2;
    } while (s >= 1.0f || s == 0.0f);

    f32 multiplier = std::sqrt(-2.0f * std::log(s) / s);
    m_nextGaussian = v2 * multiplier;
    m_hasGaussian = true;

    return mean + stddev * v1 * multiplier;
}

i64 IRandom::nextLong()
{
    // 返回随机 64 位长整数
    // 参考 MC Random.nextLong()
    return static_cast<i64>(nextU64());
}

i64 IRandom::nextLong(i64 bound)
{
    MC_ASSERT_RELEASE(bound > 0);

    u64 r = nextU64();
    u64 m = static_cast<u64>(bound);
    u64 u = r % m;

    // 类似 nextInt 的无偏差算法
    if (r - u > std::numeric_limits<u64>::max() - m + 1) {
        do {
            r = nextU64();
        } while (r >= std::numeric_limits<u64>::max() - (std::numeric_limits<u64>::max() % m));
        u = r % m;
    }

    return static_cast<i64>(u);
}

// ============================================================================
// 工具方法
// ============================================================================

void IRandom::setSeedWithHash(i64 seed)
{
    // MC 风格的种子哈希
    // 参考 MC Random.setSeed: seed = (seed ^ 0x5DEECE66DL) & ((1L << 48) - 1)
    u64 hashed = static_cast<u64>(seed) ^ 0x5DEECE66DULL;
    setSeed(hashed);
    m_hasGaussian = false;
}

void IRandom::skip(u64 count)
{
    for (u64 i = 0; i < count; ++i) {
        (void)nextU64(); // 故意丢弃返回值，用于跳过随机数序列
    }
}

} // namespace mc::math
