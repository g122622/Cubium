/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the the rights
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
#include <numeric>
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
 */
class NonOverlappingMerger : public Shapes::IndexMerger {
public:
    /**
     * @param lower 较低的坐标列表
     * @param upper 较高的坐标列表
     * @param swapped 如果为 true，表示第一个形状实际上是 upper（索引交换）
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
        const i32 lowerSegs = static_cast<i32>(m_lower.size()) - 1;
        const i32 upperSegs = static_cast<i32>(m_upper.size()) - 1;

        if (!m_swapped) {
            // first = lower, second = upper
            // lower 段：secondIdx = -1（upper 不存在）
            for (i32 i = 0; i < lowerSegs; ++i) {
                if (!consumer(i, -1, i)) {
                    return false;
                }
            }
            // upper 段：firstIdx = lower 的最后一个段索引
            for (i32 i = 0; i < upperSegs; ++i) {
                if (!consumer(lowerSegs - 1, i, lowerSegs + i)) {
                    return false;
                }
            }
        } else {
            // first = upper, second = lower（构造时交换了）
            // lower 段：firstIdx = -1（upper 不存在）
            for (i32 i = 0; i < lowerSegs; ++i) {
                if (!consumer(-1, i, i)) {
                    return false;
                }
            }
            // upper 段：secondIdx = lower 的最后一个段索引
            for (i32 i = 0; i < upperSegs; ++i) {
                if (!consumer(i, lowerSegs - 1, lowerSegs + i)) {
                    return false;
                }
            }
        }
        return true;
    }

    [[nodiscard]] i32 size() const noexcept override { return static_cast<i32>(m_merged.size()); }

private:
    std::vector<f64> m_lower;
    std::vector<f64> m_upper;
    std::vector<f64> m_merged;
    bool m_swapped;
};

/**
 * @brief 通用坐标列表合并器（归并排序式）
 *
 * 对两个有序坐标列表执行归并排序，生成合并后的坐标列表，
 * 同时跟踪每个合并段对应的原始段索引。
 *
 * 对应 MC Java 版的 IndirectMerger。
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

        // 预分配空间
        const size_t maxSize = static_cast<size_t>(firstLen + secondLen);
        m_coords.reserve(maxSize);
        m_firstIndices.reserve(maxSize);
        m_secondIndices.reserve(maxSize);

        i32 i1 = 0;           // first 的指针
        i32 j1 = 0;           // second 的指针
        i32 segIdxFirst = 0;  // 当前 first 段索引
        i32 segIdxSecond = 0; // 当前 second 段索引

        while (i1 < firstLen || j1 < secondLen) {
            // 选择下一个坐标
            f64 coord;
            bool fromFirst = false;
            bool fromSecond = false;

            if (i1 >= firstLen) {
                coord = second[static_cast<size_t>(j1)];
                fromSecond = true;
            } else if (j1 >= secondLen) {
                coord = first[static_cast<size_t>(i1)];
                fromFirst = true;
            } else {
                const f64 v1 = first[static_cast<size_t>(i1)];
                const f64 v2 = second[static_cast<size_t>(j1)];

                if (v1 < v2 - 1.0E-7) {
                    coord = v1;
                    fromFirst = true;
                } else if (v2 < v1 - 1.0E-7) {
                    coord = v2;
                    fromSecond = true;
                } else {
                    // 相等（在容差范围内）
                    coord = v1;
                    fromFirst = true;
                    fromSecond = true;
                }
            }

            // 更新段索引（在前进指针之前，当前坐标属于 i1/j1 之前的段）
            // 段索引 = 当前坐标在对应列表中的位置 - 1
            // 因为坐标点 i 定义了段 [i-1, i) 的右边界
            const i32 firstSegIdx = fromFirst ? (i1 - 1) : segIdxFirst;
            const i32 secondSegIdx = fromSecond ? (j1 - 1) : segIdxSecond;

            // 前进指针
            if (fromFirst) {
                ++i1;
            }
            if (fromSecond) {
                ++j1;
            }

            segIdxFirst = fromFirst ? (i1 - 1) : segIdxFirst;
            segIdxSecond = fromSecond ? (j1 - 1) : segIdxSecond;

            // 去重：如果当前坐标与前一个坐标几乎相同，覆盖
            if (!m_coords.empty() && coord >= m_coords.back() - 1.0E-7) {
                m_coords.back() = coord;
                m_firstIndices.back() = segIdxFirst;
                m_secondIndices.back() = segIdxSecond;
            } else {
                m_coords.push_back(coord);
                m_firstIndices.push_back(segIdxFirst);
                m_secondIndices.push_back(segIdxSecond);
            }
        }
    }

    [[nodiscard]] const std::vector<f64>& getList() const noexcept override { return m_coords; }

    [[nodiscard]] bool forMergedIndexes(const std::function<bool(i32, i32, i32)>& consumer) const override
    {
        // 段数 = 坐标数 - 1
        const i32 segCount = static_cast<i32>(m_coords.size()) - 1;
        for (i32 i = 0; i < segCount; ++i) {
            if (!consumer(m_firstIndices[static_cast<size_t>(i)], m_secondIndices[static_cast<size_t>(i)], i)) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] i32 size() const noexcept override { return static_cast<i32>(m_coords.size()); }

private:
    std::vector<f64> m_coords;
    std::vector<i32> m_firstIndices;
    std::vector<i32> m_secondIndices;
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
