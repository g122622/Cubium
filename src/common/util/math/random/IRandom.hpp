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
#include "common/util/math/random/PositionalRandomFactory.hpp"
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace mc::math {

/**
 * @brief 随机数生成器接口
 * 所有随机数算法实现此接口。提供 MC 风格的随机数方法。
 *
 * 使用方法：
 * @code
 * math::IRandom& rng = ...;
 * i32 value = rng.nextInt(100);  // [0, 100)
 * f32 f = rng.nextFloat();        // [0.0, 1.0)
 * bool b = rng.nextBoolean();     // true/false
 * @endcode
 *
 * @note 参考 MC 1.16.5 Random 接口设计
 */
class IRandom {
public:
    virtual ~IRandom() = default;

    // === 核心方法 ===

    /**
     * @brief 设置种子
     * @param seed 随机种子
     */
    virtual void setSeed(u64 seed) = 0;

    /**
     * @brief 返回 [0, UINT64_MAX] 范围的随机数
     * @return 64位无符号随机整数
     */
    [[nodiscard]] virtual u64 nextU64() = 0;

    /**
     * @brief 返回 [0, UINT32_MAX] 范围的随机数
     * @return 32位无符号随机整数
     *
     * @note 默认实现取 nextU64() 的高32位
     */
    [[nodiscard]] virtual u32 nextU32();

    /**
     * @brief 抽取最高的 bits 位（对应 MC BitRandomSource.next(bits) 的语义）
     *
     * 只在需要复刻原版**按位宽抽取**的路径上使用（当前唯一消费者是 WorldgenRandom）。
     * 默认实现取 nextU64() 的高 bits 位，对应原版对"非 Legacy 内层"的处理
     * （`(int)(randomSource.nextLong() >>> 64 - bits)`）。
     * JavaLegacyRandom 覆写为自身的 next(bits)，对应原版 `instanceof LegacyRandomSource` 分支。
     *
     * 【为什么必须单独一个方法而不是让调用方自己写 nextU64()>>(64-bits)】
     * 两类内层的**状态推进量不同**：Legacy 的 next(bits) 不论 bits 多少都只推进一次 LCG，
     * 而取 nextU64() 会推进两次。若统一按 nextU64() 折算，Legacy 内层的后续序列会整体错位。
     *
     * @param bits 位数（1..32）
     * @return 含符号的 bits 位抽取结果（高位置零）
     */
    [[nodiscard]] virtual i32 nextBits(i32 bits)
    {
        return static_cast<i32>(static_cast<u64>(nextU64()) >> (64 - bits));
    }

    /**
     * @brief 派生一个位置随机工厂（对应 MC RandomSource.forkPositional）
     *
     * 各实现按原版自己的语义派生：Xoroshiro 取两次 nextLong() 组 128 位种子；
     * Legacy 取一次 nextLong() 作 64 位种子。世界生成里 PerlinNoise 的现代路径
     * （`forkPositional().fromHashOf("octave_N")`）依赖它。
     */
    [[nodiscard]] virtual PositionalRandomFactory forkPositional() = 0;

    // === MC 风格方法 ===

    /**
     * @brief 返回随机 32 位有符号整数
     * @return 随机 i32 值
     *
     * @note 参考 MC Random.nextInt()
     */
    [[nodiscard]] virtual i32 nextInt();

    /**
     * @brief 返回 [0, bound) 范围的随机整数
     * @param bound 上界（不包含）
     * @return [0, bound) 范围内的随机整数
     *
     * 使用 MC 风格的无偏差算法，避免模偏差。
     *
     * @note 参考 MC Random.nextInt(int bound)
     */
    [[nodiscard]] virtual i32 nextInt(i32 bound);

    /**
     * @brief 返回 [min, max] 范围的随机整数
     * @param min 最小值（包含）
     * @param max 最大值（包含）
     * @return [min, max] 范围内的随机整数
     */
    [[nodiscard]] virtual i32 nextInt(i32 min, i32 max);

    /**
     * @brief 返回随机布尔值
     * @return true 或 false，各 50% 概率
     *
     * @note 参考 MC Random.nextBoolean()
     */
    [[nodiscard]] virtual bool nextBoolean();

    /**
     * @brief 返回 [0.0, 1.0) 范围的随机浮点数
     * @return [0.0, 1.0) 范围的随机浮点数
     *
     * @note 参考 MC Random.nextFloat()
     */
    [[nodiscard]] virtual f32 nextFloat();

    /**
     * @brief 返回 [min, max) 范围的随机浮点数
     * @param min 最小值（包含）
     * @param max 最大值（不包含）
     * @return [min, max) 范围的随机浮点数
     */
    [[nodiscard]] virtual f32 nextFloat(f32 min, f32 max);

    /**
     * @brief 返回 [0.0, 1.0) 范围的随机双精度浮点数
     * @return [0.0, 1.0) 范围的随机双精度浮点数
     *
     * @note 参考 MC Random.nextDouble()
     */
    [[nodiscard]] virtual f64 nextDouble();

    /**
     * @brief 返回 [min, max) 范围的随机双精度浮点数
     * @param min 最小值（包含）
     * @param max 最大值（不包含）
     * @return [min, max) 范围的随机双精度浮点数
     */
    [[nodiscard]] virtual f64 nextDouble(f64 min, f64 max);

    /**
     * @brief 高斯分布随机数
     * @param mean 均值
     * @param stddev 标准差
     * @return 服从正态分布的随机数
     *
     * 使用 Marsaglia polar method 生成正态分布随机数。
     * 会利用缓存的第二个高斯值来提高效率。
     *
     * @note 参考 MC Random.nextGaussian()
     */
    [[nodiscard]] virtual f32 nextGaussian(f32 mean = 0.0f, f32 stddev = 1.0f);

    /**
     * @brief 返回随机 64 位长整数
     * @return 随机 i64 值
     *
     * @note 参考 MC Random.nextLong()
     */
    [[nodiscard]] virtual i64 nextLong();

    /**
     * @brief 返回 [0, bound) 范围的随机长整数
     * @param bound 上界（不包含）
     * @return [0, bound) 范围内的随机长整数
     *
     * @note 参考 MC Random.nextLong(long bound)
     */
    [[nodiscard]] virtual i64 nextLong(i64 bound);

    // === 工具方法 ===

    /**
     * @brief 使用 MC 风格哈希设置种子
     * @param seed 输入种子
     *
     * 将种子通过哈希函数转换为内部状态，确保不同的输入种子产生不同的随机序列。
     * 参考 MC 的 setSeed 方法。
     */
    void setSeedWithHash(i64 seed);

    /**
     * @brief 设置大型特征种子（MC: WorldgenRandom.setLargeFeatureSeed）
     *
     * MC 1.21 雕刻器和结构使用的种子计算方式：
     * 1. 用输入 seed 初始化 RNG
     * 2. 生成两个随机 long 值 i, j
     * 3. 计算最终种子: chunkX * i ^ chunkZ * j ^ seed
     * 4. 用最终种子设置 RNG
     *
     * @param seed 基础种子（通常是 worldSeed + carverIndex）
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     */
    void setLargeFeatureSeed(i64 seed, i32 chunkX, i32 chunkZ)
    {
        setSeed(static_cast<u64>(seed));
        const i64 i = nextLong();
        const i64 j = nextLong();
        const i64 k = static_cast<i64>(chunkX) * i ^ static_cast<i64>(chunkZ) * j ^ seed;
        setSeed(static_cast<u64>(k));
    }

    /**
     * @brief 设置装饰种子（MC: WorldgenRandom.setDecorationSeed）
     *
     * MC 1.21 区块特征放置的初始种子。每个区块调用一次，
     * 返回的 decorSeed 供 setFeatureSeed 使用。
     *
     * 公式: seed RNG with worldSeed, then k = chunkX * i + chunkZ * j ^ worldSeed
     *
     * @param worldSeed 世界种子
     * @param chunkX 区块 X 坐标（方块坐标，通常是 chunkX * 16）
     * @param chunkZ 区块 Z 坐标（方块坐标，通常是 chunkZ * 16）
     * @return 装饰种子，供后续 setFeatureSeed 使用
     */
    u64 setDecorationSeed(u64 worldSeed, i32 chunkX, i32 chunkZ)
    {
        setSeed(worldSeed);
        const u64 i = static_cast<u64>(nextLong()) | 1ULL;
        const u64 j = static_cast<u64>(nextLong()) | 1ULL;
        const u64 k = (static_cast<u64>(chunkX) * i + static_cast<u64>(chunkZ) * j) ^ worldSeed;
        setSeed(k);
        return k;
    }

    /**
     * @brief 设置特征种子（MC: WorldgenRandom.setFeatureSeed）
     *
     * MC 1.21 单个特征放置的种子。每个特征调用一次。
     *
     * 公式: decorSeed + featureIndex + 10000 * stageOrdinal
     *
     * @param decorSeed 装饰种子（由 setDecorationSeed 返回）
     * @param featureIndex 特征在排序后列表中的索引
     * @param stageOrdinal 装饰阶段序号
     */
    void setFeatureSeed(u64 decorSeed, i32 featureIndex, i32 stageOrdinal)
    {
        setSeed(decorSeed + static_cast<u64>(featureIndex) + static_cast<u64>(10000) * static_cast<u64>(stageOrdinal));
    }

    /**
     * @brief 设置带盐的大型特征种子（MC: WorldgenRandom.setLargeFeatureWithSalt）
     *
     * MC 1.21 带盐值的结构种子计算方式，用于结构定位等场景。
     *
     * 公式: chunkX * 341873128712 + chunkZ * 132897987541 + seed + salt
     *
     * @param seed 基础种子
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @param salt 额外盐值
     */
    void setLargeFeatureWithSalt(i64 seed, i32 chunkX, i32 chunkZ, i64 salt)
    {
        const u64 k = static_cast<u64>(chunkX) * 341873128712ULL + static_cast<u64>(chunkZ) * 132897987541ULL +
            static_cast<u64>(seed) + static_cast<u64>(salt);
        setSeed(k);
    }

    /**
     * @brief 跳过指定数量的随机数
     * @param count 要跳过的随机数数量
     *
     * 用于快速前进随机数生成器状态。
     */
    virtual void skip(u64 count);

    /**
     * @brief 消费指定数量的随机数（MC: RandomSource.consumeCount）
     *
     * 与 skip() 的区别：原版 `RandomSource.consumeCount(n)` 的默认实现是循环调用
     * `nextInt()`（一次 32 位抽取），而 `XoroshiroRandomSource` 覆写为循环
     * `nextLong()`；`skip()` 在本项目里另有语义（Xoroshiro 的 skip 是 2^64 步跳转，
     * 不是"前进 n 步"）。世界生成里 PerlinNoise 的 skipOctave 依赖此语义，
     * 故必须与 skip() 分开，不能互相替代。
     *
     * 默认实现对齐原版 `RandomSource.consumeCount`：循环 nextInt()。
     */
    virtual void consumeCount(i32 count)
    {
        for (i32 i = 0; i < count; ++i) {
            (void)nextInt();
        }
    }

    // === 洗牌方法 ===

    /**
     * @brief 使用 Fisher-Yates 算法打乱向量元素
     * @tparam T 元素类型
     * @param vec 要打乱的向量
     *
     * 参考 MC 1.16.5: Collections.shuffle()
     * 使用 Fisher-Yates 洗牌算法，确保每个排列概率相等。
     *
     * @code
     * std::vector<int> items = {1, 2, 3, 4, 5};
     * rng.shuffle(items);  // 随机打乱
     * @endcode
     */
    template <typename T>
    void shuffle(std::vector<T>& vec)
    {
        for (size_t i = vec.size(); i > 1; --i) {
            size_t j = static_cast<size_t>(nextInt(static_cast<i32>(i)));
            std::swap(vec[i - 1], vec[j]);
        }
    }

protected:
    /// 是否有缓存的高斯值
    bool m_hasGaussian = false;
    /// 缓存的第二个高斯值
    f32 m_nextGaussian = 0.0f;
};

} // namespace mc::math
