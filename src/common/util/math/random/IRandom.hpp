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

#include "../../../core/Types.hpp"
#include <cmath>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace mc::math {

/**
 * @brief 随机数生成器接口
 *
 * 所有随机数算法实现此接口。提供 MC 风格的随机数方法。
 *
 * 使用方法：
 * @code
 * IRandom& rng = ...;
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
     * @brief 跳过指定数量的随机数
     * @param count 要跳过的随机数数量
     *
     * 用于快速前进随机数生成器状态。
     */
    virtual void skip(u64 count);

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
