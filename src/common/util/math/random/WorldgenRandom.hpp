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

#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/IRandom.hpp"
#include <memory>
#include <utility>

namespace mc::math {

/**
 * @brief 世界生成用的随机源包装器（对应 MC 1.21.11 net.minecraft.world.level.levelgen.WorldgenRandom）
 *
 * 原版 WorldgenRandom 本身是一个 RandomSource 实现，内部**委托**另一个 RandomSource，
 * 并把所有位宽抽取都折算成"反复调用内层 nextLong() 取高位"：
 *
 *   next(bits)  = (int)(inner.nextLong() >>> (64 - bits))
 *   nextInt()   = next(32)                                          // 消耗 1 次内层 nextLong
 *   nextLong()  = ((long)next(32) << 32) + next(32)                 // 消耗 2 次内层 nextLong
 *   nextBoolean() = next(1) != 0
 *   nextFloat()   = next(24) * 5.9604645E-8F
 *   nextDouble()  = (((long)next(26) << 27) + next(27)) * 1.110223E-16F
 *   nextInt(bound) — BitRandomSource 默认实现，基于 next(31) 的拒绝采样
 *
 * 【为什么必须单独实现，不能直接用内层的 nextXxx()】
 * 以 setDecorationSeed 为例，原版写法是：
 *   WorldgenRandom worldgenrandom = new WorldgenRandom(new XoroshiroRandomSource(uniqueSeed));
 *   long decorSeed = worldgenrandom.setDecorationSeed(levelSeed, blockX, blockZ);
 * 其中 setDecorationSeed 连续调用两次 `nextLong() | 1`。在 WorldgenRandom 下每次
 * nextLong() 消耗 **两个** 内层 nextLong 并各自只取高 32 位；若直接在内层
 * XoroshiroRandomSource 上调 nextLong()，则每次只消耗一个内层 nextLong 且取全 64 位。
 * 两者得到的 decorSeed 完全不同（实测 chunk(2,-2) 为 0xF53DCD247D852AF7 对
 * 0x91D8CBEBCC2C2BF7），进而使**所有** placed_feature 的 setFeatureSeed 种子错位，
 * 表现为石头变体/矿石/树木的大规模落位偏差。
 *
 * 【为什么重写 nextInt 而非 next(bits)】
 * 内层可能是 Xoroshiro（无 next(bits) 方法，只有 nextLong）也可能是 Legacy（有 next(bits)）。
 * 本类统一把 nextInt() 作为唯一原语：每次消耗一个内层 nextLong 的高 32 位。这对两种内层
 * 都恰好等价于原版 WorldgenRandom.next(32)——Xoroshiro 直接 `(int)(nextLong() >>> 32)`，
 * Legacy 的 `(int)(next(32))` 也等于其 nextLong 的高 32 位。
 *
 * 【nextGaussian 不在此类中派生】
 * 原版 WorldgenRandom 继承 LegacyRandomSource，其 MarsagliaPolarGaussian 会回调
 * `this.nextDouble()`（即上面的位分解版本）。WorldgenRandom 与它的内层各自持有一份
 * 高斯缓存，不能共用。此处用 IRandom 自带的 m_hasGaussian/m_nextGaussian 复刻该语义。
 */
class WorldgenRandom : public IRandom {
public:
    /**
     * @brief 接管一个内部随机源
     * @param inner 被委托的随机源（拥有所有权）
     *
     * 对应原版 `new WorldgenRandom(RandomSource)`。
     */
    explicit WorldgenRandom(std::unique_ptr<IRandom> inner)
        : m_inner(std::move(inner))
    {
        MC_ASSERT_RELEASE(m_inner != nullptr);
    }

    ~WorldgenRandom() override = default;

    WorldgenRandom(const WorldgenRandom&) = delete;
    WorldgenRandom& operator=(const WorldgenRandom&) = delete;

    // === IRandom 接口实现 ===

    /// 转发给内层（原版 WorldgenRandom.setSeed 即 inner.setSeed）
    void setSeed(u64 seed) override
    {
        m_inner->setSeed(seed);
        m_hasGaussian = false;
    }

    [[nodiscard]] u64 nextU64() override { return static_cast<u64>(nextLong()); }

    using IRandom::nextDouble;
    using IRandom::nextFloat;
    using IRandom::nextInt;

    /// next(32)
    [[nodiscard]] i32 nextInt() override { return nextBits(32); }

    /// BitRandomSource 默认的 nextInt(bound)：2 的幂走 next(31) 乘法，否则走 next(31) 拒绝采样
    [[nodiscard]] i32 nextInt(i32 bound) override;

    /// next(1) != 0
    [[nodiscard]] bool nextBoolean() override { return nextBits(1) != 0; }

    /// next(24) * 5.9604645E-8F
    [[nodiscard]] f32 nextFloat() override;

    /// (((long)next(26) << 27) + next(27)) * 1.110223E-16F
    [[nodiscard]] f64 nextDouble() override;

    /// ((long)next(32) << 32) + next(32)
    [[nodiscard]] i64 nextLong() override
    {
        const i64 hi = static_cast<i64>(nextBits(32));
        const i64 lo = static_cast<i64>(nextBits(32));
        return (hi << 32) + lo;
    }

    /// nextBits 直接转发给内层，以保留内层的状态推进语义（Legacy 只推进一次 LCG）
    [[nodiscard]] i32 nextBits(i32 bits) override { return m_inner->nextBits(bits); }

    /**
     * @brief 消费指定数量的随机数
     *
     * WorldgenRandom 未覆写 consumeCount，故走 RandomSource 默认实现（循环 nextInt()），
     * 即每次消耗一个内层 nextLong。
     */
    void consumeCount(i32 count) override
    {
        for (i32 i = 0; i < count; ++i) {
            (void)nextInt();
        }
    }

    /// 转发内层的工厂（原版 WorldgenRandom.forkPositional 即 inner.forkPositional）
    [[nodiscard]] PositionalRandomFactory forkPositional() override;

private:
    std::unique_ptr<IRandom> m_inner;
};

} // namespace mc::math
