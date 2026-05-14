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
#include "IRandom.hpp"
#include <limits>

namespace mc::math {

/**
 * @brief 均匀整数分布生成器
 *
 * 避免每次调用都创建 std::uniform_int_distribution。
 * 提供便捷的整数范围随机数生成。
 *
 * 使用方法：
 * @code
 * UniformIntDistribution dist(1, 100);
 *
 * // 使用已有随机数生成器
 * Random rng(seed);
 * i32 value = dist(rng);
 *
 * // 或者便捷方法（内部创建临时生成器）
 * i32 value = dist.generate();
 * @endcode
 */
class UniformIntDistribution {
public:
    /**
     * @brief 构造均匀整数分布
     * @param min 最小值（包含）
     * @param max 最大值（包含）
     */
    explicit UniformIntDistribution(i32 min = 0, i32 max = std::numeric_limits<i32>::max())
        : m_min(min)
        , m_max(max)
    {}

    /**
     * @brief 使用指定的随机数生成器生成随机数
     * @param random 随机数生成器
     * @return [min, max] 范围内的随机整数
     */
    [[nodiscard]] i32 operator()(IRandom& random) const { return random.nextInt(m_min, m_max); }

    /**
     * @brief 创建一个新的 Random 并生成随机数（便捷方法）
     * @return [min, max] 范围内的随机整数
     *
     * @note 不推荐在高频调用中使用，会创建新的生成器
     */
    [[nodiscard]] i32 generate() const;

    /**
     * @brief 设置最小值
     */
    void setMin(i32 min) { m_min = min; }

    /**
     * @brief 设置最大值
     */
    void setMax(i32 max) { m_max = max; }

    /**
     * @brief 获取最小值
     */
    [[nodiscard]] i32 min() const { return m_min; }

    /**
     * @brief 获取最大值
     */
    [[nodiscard]] i32 max() const { return m_max; }

private:
    i32 m_min;
    i32 m_max;
};

} // namespace mc::math
