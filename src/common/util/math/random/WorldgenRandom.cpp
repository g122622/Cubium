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

#include "WorldgenRandom.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/PositionalRandomFactory.hpp"

namespace mc::math {

i32 WorldgenRandom::nextInt(i32 bound)
{
    // BitRandomSource.nextInt(bound)：所有位宽抽取都走 next(bits)，见头文件对 nextBits 的说明。
    MC_ASSERT_RELEASE(bound > 0);

    if ((bound & (bound - 1)) == 0) {
        // 2 的幂：直接取高位
        return static_cast<i32>((static_cast<i64>(bound) * nextBits(31)) >> 31);
    }

    // 非 2 的幂：拒绝采样。Java 的 `i - j + (bound - 1) < 0` 依赖 int 回绕判定溢出，
    // 而 C++ 的有符号溢出是 UB，故用 u32 显式回绕后按有符号解释，逐位复刻 Java 语义。
    i32 bits = 0;
    i32 val = 0;
    u32 rejected = 0;
    do {
        bits = nextBits(31);
        val = bits % bound;
        rejected = static_cast<u32>(bits) - static_cast<u32>(val) + static_cast<u32>(bound - 1);
    } while (static_cast<i32>(rejected) < 0);

    return val;
}

f32 WorldgenRandom::nextFloat()
{
    // BitRandomSource.nextFloat(): next(24) * 5.9604645E-8F
    // 常量是 float 字面量 = 2^-24（float 可精确表示），乘法的中间精度是 float。
    return static_cast<f32>(nextBits(24)) * 5.9604645E-8f;
}

f64 WorldgenRandom::nextDouble()
{
    // BitRandomSource.nextDouble():
    //   int i = this.next(26);
    //   int j = this.next(27);
    //   long k = ((long)i << 27) + j;
    //   return k * 1.110223E-16F;
    // 【字面量是 float 后缀】Java 的 `long * float` 会做二元数值提升——k 先被拓宽为
    // **float**（丢 29 位有效位）再做 float 乘法，结果只有 24 位精度（见
    // JavaLegacyRandom::nextDouble 的同类说明）。必须复刻该中间精度。
    const i32 i = nextBits(26);
    const i32 j = nextBits(27);
    const i64 k = (static_cast<i64>(i) << 27) + static_cast<i64>(j);
    return static_cast<f64>(static_cast<f32>(k) * 1.1102230246251565E-16f);
}

PositionalRandomFactory WorldgenRandom::forkPositional()
{
    return m_inner->forkPositional();
}

} // namespace mc::math
