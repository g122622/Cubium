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

#pragma once

#include "common/core/Types.hpp"
#include <bit>

namespace mc::math {

/**
 * @brief MC 线性同余生成器 — MC 1.21.11 LinearCongruentialGenerator
 *
 * 复刻 MC Java 版 net.minecraft.util.LinearCongruentialGenerator。
 * 这是一个非标准的二次映射 LCG，使用 Knuth MMIX 参数。
 *
 * 算法：
 *   seed = seed * (seed * MULTIPLIER + INCREMENT)  (signed 64-bit overflow wrap)
 *   return seed + value
 *
 * 与标准 LCG (LcgRandom) 不同：
 * - 标准 LCG: state = A * state + C (线性)
 * - 本类: seed *= seed * A + C (二次映射，更强的混合)
 *
 * 主要用途：
 * - BiomeManager.getFiddledDistance() 中的 Voronoi 缩放 fiddling
 * - 位置依赖的伪随机哈希
 */
class LinearCongruentialGenerator {
public:
    /// Knuth MMIX 乘数常量
    static constexpr i64 MULTIPLIER = 6364136223846793005LL;
    /// Knuth MMIX 增量常量
    static constexpr i64 INCREMENT = 1442695040888963407LL;

    LinearCongruentialGenerator() = delete;

    /**
     * @brief 推进 LCG 状态并混合输入值
     *
     * 等价于 MC Java 版 LinearCongruentialGenerator.next(seed, value)。
     *
     * 算法步骤：
     * 1. seed = seed * (seed * MULTIPLIER + INCREMENT)  (signed 64-bit wrap)
     * 2. return seed + value
     *
     * @param seed 当前 LCG 状态
     * @param value 要混合的输入值（通常是坐标）
     * @return 新的 LCG 状态
     *
     * @note Java 的 long 运算使用 signed 64-bit overflow wrap（defined behavior）。
     *       C++ 的 signed overflow 是 UB，因此使用 unsigned 中转来保证一致性。
     */
    [[nodiscard]] static i64 next(i64 seed, i64 value)
    {
        // Java: seed *= seed * MULTIPLIER + INCREMENT; return seed + value;
        // 使用 unsigned 中转避免 C++ signed overflow UB
        u64 s = static_cast<u64>(seed);
        s *= s * static_cast<u64>(MULTIPLIER) + static_cast<u64>(INCREMENT);
        // seed + value 也要做 signed wrap
        return static_cast<i64>(s) + value;
    }
};

} // namespace mc::math
