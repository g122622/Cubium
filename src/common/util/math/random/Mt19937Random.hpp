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

#pragma once

#include "IRandom.hpp"
#include "common/core/Types.hpp"
#include <random>

namespace mc::math {

/**
 * @brief Mersenne Twister 随机数生成器
 *
 * 基于 std::mt19937_64 实现，提供高质量的随机数生成。
 * 这是默认的随机数算法，具有以下特点：
 * - 周期长：2^19937 - 1
 * - 质量高：通过了所有统计测试
 * - 兼容性好：与 C++ 标准库完全兼容
 * - 状态大：2.5KB
 *
 * 使用方法：
 * @code
 * Mt19937Random rng(seed);
 * i32 value = rng.nextInt(100);
 * @endcode
 *
 * @note 如果需要更小的状态或更高的性能，考虑使用 Xoroshiro128ppRandom
 */
class Mt19937Random : public IRandom {
public:
    /**
     * @brief 使用种子构造随机数生成器
     * @param seed 随机种子
     */
    explicit Mt19937Random(u64 seed = 0);

    /**
     * @brief 使用随机设备构造随机数生成器
     * @return 使用真随机种子初始化的生成器
     */
    [[nodiscard]] static Mt19937Random fromRandomDevice();

    // === IRandom 接口 ===

    void setSeed(u64 seed) override;
    [[nodiscard]] u64 nextU64() override;

private:
    std::mt19937_64 m_engine;
};

} // namespace mc::math
