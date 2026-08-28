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
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/biome/climate/ParameterTypes.hpp"
#include "world/biome/climate/RTree.hpp"
#include <cstddef>
#include <utility>
#include <vector>

namespace mc::world::biome::climate {

/**
 * @brief 参数列表，支持基于最近邻匹配的生物群系查找
 *
 * 存储 ParameterPoint → T 的映射，通过 RTree 加速最近邻搜索。
 */
template <typename T>
class ParameterList {
public:
    using Entry = std::pair<ParameterPoint, T>;

    ParameterList() = default;

    /**
     * @brief 从条目列表构造参数列表并构建 RTree 索引
     */
    explicit ParameterList(std::vector<Entry> entries)
        : m_entries(std::move(entries))
    {
        // 父级 buildParameterList 已带 trace；此处作为 subpart 量化参数列表构造
        // （内部触发 RTree::create → build 递归构建 7 维空间索引）。
        MC_TRACE_SCOPED_EVENT(::mc::trace::TraceEvents.Server.Initialization, "ParameterList::ParameterList");

        if (!m_entries.empty()) {
            m_index = RTree<T>::create(m_entries);
        }
    }

    /** 添加条目并自动重建 RTree 索引 */
    void add(ParameterPoint point, T value)
    {
        m_entries.emplace_back(std::move(point), std::move(value));
        if (!m_entries.empty()) {
            m_index = RTree<T>::create(m_entries);
        }
    }

    /** 获取所有条目 */
    [[nodiscard]] const std::vector<Entry>& entries() const { return m_entries; }

    /** 迭代器支持 */
    auto begin() const { return m_entries.begin(); }
    auto end() const { return m_entries.end(); }

    /** 条目数量 */
    [[nodiscard]] size_t size() const { return m_entries.size(); }

    /** 是否为空 */
    [[nodiscard]] bool empty() const { return m_entries.empty(); }

    /**
     * @brief 使用 RTree 查找最匹配的值
     *
     * 通过 RTree 分支限界搜索加速最近邻查找。
     */
    [[nodiscard]] const T& findValue(const TargetPoint& target) const
    {
        MC_ASSERT_RELEASE(!m_entries.empty());
        return m_index.search(target);
    }

    /**
     * @brief 暴力搜索最匹配的值（用于测试验证 RTree 结果）
     *
     * 线性扫描所有条目，用于验证 RTree 搜索结果的正确性。
     */
    [[nodiscard]] const T& findValueBruteForce(const TargetPoint& target) const
    {
        MC_ASSERT_RELEASE(!m_entries.empty());

        auto it = m_entries.begin();
        i64 bestFitness = it->first.fitness(target);
        const T* bestValue = &it->second;
        ++it;

        for (; it != m_entries.end(); ++it) {
            const i64 fitness = it->first.fitness(target);
            if (fitness < bestFitness) {
                bestFitness = fitness;
                bestValue = &it->second;
            }
        }

        return *bestValue;
    }

private:
    std::vector<Entry> m_entries;
    RTree<T> m_index;
};

} // namespace mc::world::biome::climate
