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
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <vector>

namespace mc {

/**
 * @brief 均匀分布的坐标点列表
 *
 * 表示 0/n, 1/n, 2/n, ..., n/n 的均匀分布坐标点序列。
 * 与 MC Java 版的 CubePointRange 对应，用于表示与单位立方体对齐的形状坐标。
 * 不实际存储坐标值，而是按需计算，节省内存。
 */
class CubePointRange {
public:
    /**
     * @brief 构造均匀坐标点列表
     * @param count 段数（坐标点数量为 count + 1）
     */
    explicit CubePointRange(i32 count)
        : m_count(count)
    {}

    /**
     * @brief 获取坐标点数量
     */
    i32 size() const { return m_count + 1; }

    /**
     * @brief 获取指定索引处的坐标值
     * @param index 索引（0 到 m_count）
     * @return 坐标值 = index / m_count
     */
    f64 getDouble(i32 index) const { return static_cast<f64>(index) / static_cast<f64>(m_count); }

    /**
     * @brief 获取段数
     */
    i32 getCount() const { return m_count; }

    /**
     * @brief 将坐标值导出为 vector
     */
    std::vector<f64> toVector() const
    {
        std::vector<f64> result(static_cast<size_t>(m_count + 1));
        for (i32 i = 0; i <= m_count; ++i) {
            result[static_cast<size_t>(i)] = getDouble(i);
        }
        return result;
    }

    /**
     * @brief 检查两个 CubePointRange 是否相等
     */
    bool operator==(const CubePointRange& other) const { return m_count == other.m_count; }
    bool operator!=(const CubePointRange& other) const { return m_count != other.m_count; }

private:
    i32 m_count;
};

/**
 * @brief 检查坐标列表是否为 CubePointRange
 *
 * 如果坐标点列表满足均匀分布特征（0/n, 1/n, ..., n/n），则识别为 CubePointRange。
 * 容差为 1.0E-7。
 *
 * @param coords 坐标点列表
 * @return 如果是 CubePointRange，返回段数；否则返回 -1
 */
inline i32 detectCubePointRange(const std::vector<f64>& coords)
{
    if (coords.size() < 2) {
        return -1;
    }

    const i32 count = static_cast<i32>(coords.size()) - 1;

    // 检查第一个和最后一个值
    if (std::abs(coords[0]) > 1.0E-7 || std::abs(coords[static_cast<size_t>(count)] - 1.0) > 1.0E-7) {
        return -1;
    }

    // 检查均匀分布
    for (i32 i = 1; i < count; ++i) {
        const f64 expected = static_cast<f64>(i) / static_cast<f64>(count);
        if (std::abs(coords[static_cast<size_t>(i)] - expected) > 1.0E-7) {
            return -1;
        }
    }

    return count;
}

/**
 * @brief 计算两个正整数的最小公倍数
 */
inline i64 lcm(i64 a, i64 b)
{
    if (a == 0 || b == 0) {
        return 0;
    }
    return std::abs(a / std::gcd(a, b) * b);
}

} // namespace mc
