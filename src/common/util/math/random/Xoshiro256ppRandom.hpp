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

namespace mc::math {

/**
 * @brief xoshiro256++ 随机数生成器
 *
 * 高质量、中等状态的随机数生成器。
 * 具有以下特点：
 * - 周期：2^256 - 1
 * - 状态：32 字节
 * - 速度快：比 Mersenne Twister 快约 2 倍
 * - 质量极高：通过了所有统计测试
 * - 支持 256 位输出
 *
 * 使用方法：
 * @code
 * Xoshiro256ppRandom rng(seed);
 * i32 value = rng.nextInt(100);
 * @endcode
 *
 * @note 适合需要高质量随机数的场景
 * @note 参考 http://xoroshiro.di.unimi.it/
 */
class Xoshiro256ppRandom : public IRandom {
public:
    /**
     * @brief 使用种子构造随机数生成器
     * @param seed 随机种子
     *
     * @note 种子通过 SplitMix64 算法扩展为 256 位状态
     */
    explicit Xoshiro256ppRandom(u64 seed = 0);

    // === IRandom 接口 ===

    void setSeed(u64 seed) override;
    [[nodiscard]] u64 nextU64() override;

    /**
     * @brief 跳过指定数量的随机数
     * @param count 要跳过的随机数数量
     *
     * xoshiro256++ 支持快速跳转
     */
    void skip(u64 count) override;

private:
    u64 m_state[4];

    /**
     * @brief 左旋转
     */
    [[nodiscard]] static u64 rotl(u64 x, int k) { return (x << k) | (x >> (64 - k)); }

    /**
     * @brief 使用 SplitMix64 扩展种子
     */
    static u64 splitMix64(u64& state);
};

} // namespace mc::math
