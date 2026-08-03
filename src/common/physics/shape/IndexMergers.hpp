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

#include "Shapes.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <numeric>
#include <utility>
#include <vector>

namespace mc {

/**
 * @brief 相同坐标列表合并器
 *
 * 当两个形状在某条轴上的坐标列表完全相同时使用。
 * 合并后的坐标列表就是原始列表，索引一一对应。
 *
 * 对应 MC Java 版的 IdenticalMerger。
 */
class IdenticalMerger : public Shapes::IndexMerger {
public:
    explicit IdenticalMerger(std::vector<f64> coords)
        : m_coords(std::move(coords))
    {}

    [[nodiscard]] const std::vector<f64>& getList() const noexcept override { return m_coords; }

    [[nodiscard]] bool forMergedIndexes(const std::function<bool(i32, i32, i32)>& consumer) const override
    {
        const i32 n = static_cast<i32>(m_coords.size()) - 1;
        for (i32 i = 0; i < n; ++i) {
            if (!consumer(i, i, i)) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] i32 size() const noexcept override { return static_cast<i32>(m_coords.size()); }

private:
    std::vector<f64> m_coords;
};

/**
 * @brief 不重叠坐标列表合并器
 *
 * 当两个形状在某条轴上的坐标范围完全不重叠时使用
 * （一个形状完全在另一个形状的前面）。
 *
 * 对应 MC Java 版的 NonOverlappingMerger。
 *
 * 索引映射规则（严格遵循 MC Java 版 NonOverlappingMerger.forNonSwappedIndexes）：
 * - lower 的每个坐标点 j（0 <= j < lower.size()）产生一个段：
 *   firstIdx = j, secondIdx = -1, mergedIdx = j
 *   注意：这产生 lower.size() 个段，比 lower 的实际段数多 1。
 *   最后一个段（j = lower.size()-1）是从 lower 的最后一个坐标到 upper 的第一个坐标的过渡段。
 *   对于这个过渡段，firstIdx = lower.size()-1 指向 lower 的最后一个体素，
 *   但由于 isFullWide 对超出 lower 实际体素范围的索引返回 false，
 *   这个过渡段在实际布尔运算中会正确地被视为空。
 *
 * - upper 的每个段 k（0 <= k < upper.size()-1）产生一个段：
 *   firstIdx = lower.size()-1, secondIdx = k, mergedIdx = lower.size() + k
 *   注意：upper 段的 firstIdx = lower.size()-1，这是 MC Java 的设计。
 *   由于 lower.size()-1 对 lower 来说可能超出实际体素范围，
 *   isFullWide 会返回 false，使得 upper 段只反映 upper 形状的内容。
 *
 * 当 swap=true 时，forMergedIndexes 回调中交换 firstIdx 和 secondIdx 参数，
 * 这是因为 createIndexMerger 在检测到 b < a 时交换了参数顺序，
 * 需要在回调中恢复原始的 (a, b) 顺序。
 * - upper 的每个段：firstIdx = lower.size()-1（lower 最后段的索引）, secondIdx = 段索引
 * 当 swap=true 时，forMergedIndexes 交换 firstIdx 和 secondIdx 参数
 */
class NonOverlappingMerger : public Shapes::IndexMerger {
public:
    /**
     * @param lower 较低的坐标列表
     * @param upper 较高的坐标列表
     * @param swapped 如果为 true，表示参数顺序与实际 lower/upper 相反，
     *                需要在回调中交换 firstIdx 和 secondIdx
     */
    NonOverlappingMerger(std::vector<f64> lower, std::vector<f64> upper, bool swapped)
        : m_lower(std::move(lower))
        , m_upper(std::move(upper))
        , m_swapped(swapped)
    {
        // 预计算合并后的坐标列表：lower + upper
        m_merged.reserve(m_lower.size() + m_upper.size());
        for (f64 v : m_lower) {
            m_merged.push_back(v);
        }
        for (f64 v : m_upper) {
            m_merged.push_back(v);
        }
    }

    [[nodiscard]] const std::vector<f64>& getList() const noexcept override { return m_merged; }

    [[nodiscard]] bool forMergedIndexes(const std::function<bool(i32, i32, i32)>& consumer) const override
    {
        if (m_swapped) {
            // swap=true 时交换 firstIdx 和 secondIdx
            return forNonSwappedIndexes([&consumer](i32 firstIdx, i32 secondIdx, i32 mergedIdx) {
                return consumer(secondIdx, firstIdx, mergedIdx);
            });
        }
        return forNonSwappedIndexes(consumer);
    }

    [[nodiscard]] i32 size() const noexcept override { return static_cast<i32>(m_merged.size()); }

private:
    std::vector<f64> m_lower;
    std::vector<f64> m_upper;
    std::vector<f64> m_merged;
    bool m_swapped;

    /**
     * @brief 非交换版本的索引遍历
     *
     * 索引映射：
     * - lower 的段：firstIdx = j, secondIdx = -1
     * - upper 的段：firstIdx = lower.size()-1, secondIdx = k
     */
    bool forNonSwappedIndexes(const std::function<bool(i32, i32, i32)>& consumer) const
    {
        const i32 lowerSize = static_cast<i32>(m_lower.size());

        // lower 的所有段（包括从 lower 末尾到 upper 开头的过渡段）
        for (i32 j = 0; j < lowerSize; ++j) {
            if (!consumer(j, -1, j)) {
                return false;
            }
        }

        // upper 的段（不包括从 lower 末尾到 upper 开头的过渡段，因为那已经包含在 lower 的段中了）
        const i32 upperSegs = static_cast<i32>(m_upper.size()) - 1;
        for (i32 k = 0; k < upperSegs; ++k) {
            if (!consumer(lowerSize - 1, k, lowerSize + k)) {
                return false;
            }
        }

        return true;
    }
};

/**
 * @brief 通用坐标列表合并器（归并排序式）
 *
 * 对两个有序坐标列表执行归并排序，生成合并后的坐标列表，
 * 同时跟踪每个合并段对应的原始段索引。
 *
 * 对应 MC Java 版的 IndirectMerger。
 *
 * 当 includeFirst 为 false 时，跳过仅属于第一个形状的坐标段；
 * 当 includeSecond 为 false 时，跳过仅属于第二个形状的坐标段。
 * 这是布尔运算优化的关键：如果操作不需要某个形状的独占区域，
 * 就不需要包含那些坐标段。
 */
class IndirectMerger : public Shapes::IndexMerger {
public:
    /**
     * @param first 第一个形状的坐标列表
     * @param second 第二个形状的坐标列表
     * @param includeFirst op(true,false) - 第一个形状独占区域是否包含在结果中
     * @param includeSecond op(false,true) - 第二个形状独占区域是否包含在结果中
     */
    IndirectMerger(const std::vector<f64>& first, const std::vector<f64>& second, bool includeFirst, bool includeSecond)
    {
        const i32 firstLen = static_cast<i32>(first.size());
        const i32 secondLen = static_cast<i32>(second.size());

        // skipFirst = !includeFirst: 跳过仅属于第一个形状的坐标
        // skipSecond = !includeSecond: 跳过仅属于第二个形状的坐标
        const bool skipFirst = !includeFirst;
        const bool skipSecond = !includeSecond;

        // 预分配空间（最坏情况：两个列表完全不相交）
        const size_t maxSize = static_cast<size_t>(firstLen + secondLen);
        m_coords.reserve(maxSize);
        m_firstIndices.reserve(maxSize);
        m_secondIndices.reserve(maxSize);

        i32 i1 = 0;                                            // first 的指针
        i32 j1 = 0;                                            // second 的指针
        i32 writeIdx = 0;                                      // 写入位置
        f64 prevCoord = std::numeric_limits<f64>::quiet_NaN(); // 前一个坐标值，用于去重

        while (true) {
            const bool firstDone = i1 >= firstLen;
            const bool secondDone = j1 >= secondLen;

            if (firstDone && secondDone) {
                break;
            }

            // 选择下一个坐标：取两者中较小的
            const bool takeFromFirst =
                !firstDone && (secondDone || first[static_cast<size_t>(i1)] < second[static_cast<size_t>(j1)] + 1.0E-7);

            if (takeFromFirst) {
                ++i1;
                // 如果要跳过第一个形状的独占坐标，且第二个形状尚未开始或已结束
                if (skipFirst && (j1 == 0 || secondDone)) {
                    continue;
                }
            } else {
                ++j1;
                // 如果要跳过第二个形状的独占坐标，且第一个形状尚未开始或已结束
                if (skipSecond && (i1 == 0 || firstDone)) {
                    continue;
                }
            }

            // 计算段索引：i1/j1 已自增，段索引 = 指针 - 1
            const i32 firstSegIdx = i1 - 1;
            const i32 secondSegIdx = j1 - 1;
            const f64 coord =
                takeFromFirst ? first[static_cast<size_t>(firstSegIdx)] : second[static_cast<size_t>(secondSegIdx)];

            // 去重：如果当前坐标与前一个几乎相同，覆盖前一个条目的索引
            if (!(prevCoord >= coord - 1.0E-7)) {
                // 新坐标点
                m_coords.push_back(coord);
                m_firstIndices.push_back(firstSegIdx);
                m_secondIndices.push_back(secondSegIdx);
                ++writeIdx;
                prevCoord = coord;
            } else {
                // 重复坐标点，覆盖前一个条目的索引（两个形状在此处共享边界）
                m_firstIndices[static_cast<size_t>(writeIdx - 1)] = firstSegIdx;
                m_secondIndices[static_cast<size_t>(writeIdx - 1)] = secondSegIdx;
            }
        }

        // 结果长度至少为1（确保空形状也能正常处理）
        m_resultLength = std::max(1, writeIdx);

        // 如果只有一个坐标点，清空索引（表示空形状）
        if (m_resultLength <= 1) {
            m_coords.clear();
            m_firstIndices.clear();
            m_secondIndices.clear();
            m_resultLength = 1;
        }
    }

    [[nodiscard]] const std::vector<f64>& getList() const noexcept override { return m_coords; }

    [[nodiscard]] bool forMergedIndexes(const std::function<bool(i32, i32, i32)>& consumer) const override
    {
        // 段数 = 坐标数 - 1
        const i32 segCount = m_resultLength - 1;
        for (i32 i = 0; i < segCount; ++i) {
            if (!consumer(m_firstIndices[static_cast<size_t>(i)], m_secondIndices[static_cast<size_t>(i)], i)) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] i32 size() const noexcept override { return m_resultLength; }

private:
    std::vector<f64> m_coords;
    std::vector<i32> m_firstIndices;
    std::vector<i32> m_secondIndices;
    i32 m_resultLength = 0;
};

/**
 * @brief 立方体对齐坐标合并器
 *
 * 当两个形状的坐标都是 CubePointRange（均匀分布）时使用。
 * 使用数学关系代替归并排序，效率更高。
 *
 * 对应 MC Java 版的 DiscreteCubeMerger。
 */
class DiscreteCubeMerger : public Shapes::IndexMerger {
public:
    /**
     * @param firstSegs 第一个形状的段数
     * @param secondSegs 第二个形状的段数
     */
    DiscreteCubeMerger(i32 firstSegs, i32 secondSegs)
        : m_firstSegs(firstSegs)
        , m_secondSegs(secondSegs)
    {
        const i64 g = std::gcd(static_cast<i64>(firstSegs), static_cast<i64>(secondSegs));
        m_lcmSegs = static_cast<i32>(static_cast<i64>(firstSegs) / g * secondSegs);
        m_firstScale = m_lcmSegs / firstSegs;
        m_secondScale = m_lcmSegs / secondSegs;

        // 预计算合并后的坐标列表
        m_coords.resize(static_cast<size_t>(m_lcmSegs + 1));
        for (i32 i = 0; i <= m_lcmSegs; ++i) {
            m_coords[static_cast<size_t>(i)] = static_cast<f64>(i) / static_cast<f64>(m_lcmSegs);
        }
    }

    [[nodiscard]] const std::vector<f64>& getList() const noexcept override { return m_coords; }

    [[nodiscard]] bool forMergedIndexes(const std::function<bool(i32, i32, i32)>& consumer) const override
    {
        for (i32 i = 0; i < m_lcmSegs; ++i) {
            const i32 firstIdx = i / m_firstScale;
            const i32 secondIdx = i / m_secondScale;
            if (!consumer(firstIdx, secondIdx, i)) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] i32 size() const noexcept override { return m_lcmSegs + 1; }

    i32 getLcmSegs() const { return m_lcmSegs; }
    i32 getFirstScale() const { return m_firstScale; }
    i32 getSecondScale() const { return m_secondScale; }

private:
    i32 m_firstSegs;
    i32 m_secondSegs;
    i32 m_lcmSegs;
    i32 m_firstScale;
    i32 m_secondScale;
    std::vector<f64> m_coords;
};

} // namespace mc
