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

class PositionalRandomFactory;

/**
 * @brief xoroshiro128++ 随机数生成器
 *
 * 高性能、小状态的随机数生成器。
 * 具有以下特点：
 * - 周期：2^128 - 1
 * - 状态小：仅 16 字节
 * - 速度快：比 Mersenne Twister 快约 2-3 倍
 * - 质量高：通过了 BigCrush 测试
 *
 * 使用方法：
 * @code
 * Xoroshiro128ppRandom rng(seed);
 * i32 value = rng.nextInt(100);
 * @endcode
 *
 * @note 适合需要高性能和小状态的场景
 * @note 参考 http://xoroshiro.di.unimi.it/
 */
class Xoroshiro128ppRandom : public IRandom {
public:
    /**
     * @brief 使用种子构造随机数生成器
     * @param seed 随机种子
     *
     * @note 种子通过 MC 的 upgradeSeedTo128bit 算法扩展为 128 位状态
     *       （mixStafford13(seed ^ SILVER_RATIO_64), mixStafford13(seed + GOLDEN_RATIO_64)）
     */
    explicit Xoroshiro128ppRandom(u64 seed = 0);

    /**
     * @brief 使用 128 位种子构造随机数生成器
     * @param seedLo 种子低 64 位
     * @param seedHi 种子高 64 位
     *
     * @note 直接设置 128 位状态，不经过 upgradeSeedTo128bit 扩展。
     *       对应 MC Xoroshiro128PlusPlus(long, long) 构造函数。
     */
    Xoroshiro128ppRandom(u64 seedLo, u64 seedHi);

    // === IRandom 接口 ===

    void setSeed(u64 seed) override;
    [[nodiscard]] u64 nextU64() override;

    // 将基类的所有 nextInt/nextFloat/nextDouble 重载引入作用域，避免 override 隐藏
    using IRandom::nextDouble;
    using IRandom::nextFloat;
    using IRandom::nextInt;

    /**
     * @brief 返回 [0.0, 1.0) 范围的随机双精度浮点数
     *
     * MC XoroshiroRandomSource.nextDouble() 使用 float 精度：
     *   (float)(nextLong() >>> 11) * 1.1102230246251565E-16F
     * Java 的 long * float 运算会将 long 拓宽为 float（丢失约29位精度），
     * 结果为 float 精度后再拓宽为 double。
     */
    [[nodiscard]] f64 nextDouble() override;

    /**
     * @brief 返回 [0.0, 1.0) 范围的随机浮点数
     *
     * MC XoroshiroRandomSource.nextFloat() 使用 24 位精度：
     *   (float)next(24) * 5.9604645E-8F
     */
    [[nodiscard]] f32 nextFloat() override;

    /**
     * @brief 返回 [0, bound) 范围的随机整数
     *
     * MC XoroshiroRandomSource.nextInt(int) 使用 Lemire 无除法算法：
     *   long i = Integer.toUnsignedLong(nextInt());
     *   long j = i * bound;
     *   long k = j & 0xFFFFFFFFL;
     *   if (k < bound) { rejection loop } return (int)(j >> 32);
     */
    [[nodiscard]] i32 nextInt(i32 bound) override;

    /**
     * @brief 跳过指定数量的随机数
     * @param count 要跳过的随机数数量（注意：当前实现每次跳过 2^64 个状态）
     */
    void skip(u64 count) override;

    /**
     * @brief 创建位置随机工厂
     *
     * 消耗两个 nextLong() 调用获取 128 位种子来创建工厂。
     * 与 MC 1.21 XoroshiroRandomSource.forkPositional() 完全一致。
     *
     * @return 位置随机工厂
     */
    [[nodiscard]] PositionalRandomFactory forkPositional();

private:
    u64 m_state[2];

    /**
     * @brief 左旋转
     */
    [[nodiscard]] static u64 rotl(u64 x, int k) { return (x << k) | (x >> (64 - k)); }
};

} // namespace mc::math
