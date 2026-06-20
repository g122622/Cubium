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
#include "common/util/math/random/IRandom.hpp"

namespace mc::math {

/**
 * @brief Java LegacyRandomSource (48位 LCG) 复刻
 *
 * 复刻 MC Java 版 java.util.Random / LegacyRandomSource 的 48 位 LCG 随机数生成器。
 * 用于需要与 Java 版种子确定性对齐的场景（如 PerlinSimplexNoise 负倍频层种子派生）。
 *
 * Java LCG 参数：
 * - 乘数 A = 25214903917 (0x5DEECE66D)
 * - 增量 C = 11 (0xB)
 * - 掩码 = (1 << 48) - 1
 * - setSeed: state = (seed ^ A) & mask
 * - next(bits): state = (state * A + C) & mask; return (int)(state >>> (48 - bits))
 */
class JavaLegacyRandom : public IRandom {
public:
    /**
     * @brief 使用种子构造
     * @param seed 初始种子（等价于 Java 的 new LegacyRandomSource(seed)）
     */
    explicit JavaLegacyRandom(u64 seed = 0);

    ~JavaLegacyRandom() override = default;

    // === IRandom 接口实现 ===

    void setSeed(u64 seed) override;
    [[nodiscard]] u64 nextU64() override;

    // 将基类的所有 nextInt/nextFloat/nextDouble 重载引入作用域，避免 override 隐藏
    using IRandom::nextDouble;
    using IRandom::nextFloat;
    using IRandom::nextInt;

    [[nodiscard]] i32 nextInt() override;
    [[nodiscard]] i32 nextInt(i32 bound) override;
    [[nodiscard]] bool nextBoolean() override;
    [[nodiscard]] f32 nextFloat() override;
    [[nodiscard]] f64 nextDouble() override;
    [[nodiscard]] i64 nextLong() override;
    void skip(u64 count) override;

    // === Java LegacyRandom 专用方法 ===

    /**
     * @brief 生成指定位数的随机数（等价于 Java Random.next(bits)）
     * @param bits 位数 (1-32)
     * @return 随机整数的低 bits 位
     */
    [[nodiscard]] i32 next(i32 bits);

    /**
     * @brief 推进指定步数（等价于 Java RandomSource.consumeCount(count)）
     * @param count 推进步数
     */
    void consumeCount(i32 count);

private:
    /// Java LCG 乘数常量
    static constexpr u64 MULTIPLIER = 25214903917ULL;
    /// Java LCG 增量常量
    static constexpr u64 INCREMENT = 11ULL;
    /// Java LCG 48位掩码
    static constexpr u64 MASK = (1ULL << 48) - 1;

    u64 m_state = 0;
};

} // namespace mc::math
