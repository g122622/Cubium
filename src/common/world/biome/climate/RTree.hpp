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
#include "common/util/assert/AssertAll.hpp"
#include "world/biome/climate/ParameterTypes.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

namespace mc::world::biome::climate {

// ============================================================================
// RTree 节点基类
// ============================================================================

/**
 * @brief RTree 节点基类
 *
 * 每个节点持有 7 维参数空间边界（parameterSpace），
 * 支持 search（分支限界最近邻搜索）和 distance（到目标点的平方距离）。
 */
template <typename T>
class RTreeNode {
public:
    /// 参数空间边界数组（7 个维度）
    std::array<Parameter, PARAMETER_COUNT> parameterSpace;

    virtual ~RTreeNode() = default;

    /**
     * @brief 搜索最近的叶节点
     *
     * @param target 目标点参数数组（7 元素）
     * @param lastResult 上次搜索结果缓存，用于加速搜索
     * @return 最近的叶节点
     */
    virtual const RTreeNode* search(const i64 target[], const RTreeNode* lastResult) const = 0;

    /**
     * @brief 计算此节点参数空间到目标点的距离（各维度距离平方和）
     */
    [[nodiscard]] i64 distance(const i64 target[]) const
    {
        i64 result = 0;
        for (i32 i = 0; i < PARAMETER_COUNT; ++i) {
            const i64 d = parameterSpace[i].distance(target[i]);
            result += d * d;
        }
        return result;
    }

protected:
    explicit RTreeNode(std::array<Parameter, PARAMETER_COUNT> ps)
        : parameterSpace(std::move(ps))
    {}
};

// ============================================================================
// RTree 叶节点
// ============================================================================

/// RTree 叶节点，持有单个值
template <typename T>
class RTreeLeaf final : public RTreeNode<T> {
public:
    T value;

    RTreeLeaf(const ParameterPoint& point, T val)
        : RTreeNode<T>(point.parameterSpace())
        , value(std::move(val))
    {}

    const RTreeNode<T>* search(const i64[], const RTreeNode<T>*) const override { return this; }
};

// ============================================================================
// RTree 子树节点
// ============================================================================

/// RTree 子树（内部）节点，持有子节点列表
template <typename T>
class RTreeSubTree final : public RTreeNode<T> {
public:
    std::vector<std::unique_ptr<RTreeNode<T>>> children;

    explicit RTreeSubTree(std::vector<std::unique_ptr<RTreeNode<T>>> childNodes)
        : RTreeNode<T>(buildParameterSpace(childNodes))
        , children(std::move(childNodes))
    {}

    const RTreeNode<T>* search(const i64 target[], const RTreeNode<T>* lastResult) const override
    {
        i64 bestDist = lastResult ? lastResult->distance(target) : std::numeric_limits<i64>::max();
        const RTreeNode<T>* bestLeaf = lastResult;

        for (const auto& child : children) {
            const i64 childDist = child->distance(target);
            if (bestDist > childDist) {
                const RTreeNode<T>* result = child->search(target, bestLeaf);
                const i64 resultDist = (result == child.get()) ? childDist : result->distance(target);
                if (bestDist > resultDist) {
                    bestDist = resultDist;
                    bestLeaf = result;
                }
            }
        }

        return bestLeaf;
    }

private:
    /// 从子节点构建参数空间边界框
    static std::array<Parameter, PARAMETER_COUNT> buildParameterSpace(
        const std::vector<std::unique_ptr<RTreeNode<T>>>& nodes)
    {
        MC_ASSERT_RELEASE(!nodes.empty() && "RTreeSubTree needs at least one child");

        std::array<Parameter, PARAMETER_COUNT> space;
        bool initialized = false;
        for (const auto& node : nodes) {
            for (i32 i = 0; i < PARAMETER_COUNT; ++i) {
                if (!initialized) {
                    space[i] = node->parameterSpace[i];
                } else {
                    space[i] = Parameter::merge(space[i], node->parameterSpace[i]);
                }
            }
            if (!initialized) {
                initialized = true;
            }
        }
        return space;
    }
};

// ============================================================================
// RTree 搜索结果缓存
// ============================================================================

/// RTree 搜索结果缓存
template <typename T>
class RTreeCache {
public:
    const RTreeNode<T>* lastResult = nullptr;
};

// ============================================================================
// RTree 公开接口
// ============================================================================

/**
 * @brief RTree 空间索引，用于加速 7 维参数空间中的最近邻搜索
 *
 * 构建算法：
 * - 少于等于 6 个节点时，按质心绝对值之和排序后创建 SubTree
 * - 超过 6 个节点时，尝试所有 7 个维度作为排序键，选择总代价最小的分桶方案
 * - 分桶大小 = 6^floor(log_6(size - 0.01))
 *
 * 搜索算法：
 * - 分支限界最近邻搜索，剪枝掉不可能更近的子树
 * - 缓存上次搜索结果以利用空间局部性
 */
template <typename T>
class RTree {
public:
    static constexpr i32 CHILDREN_PER_NODE = 6;

    /**
     * @brief 从条目列表构建 RTree
     *
     * @param entries ParameterPoint → T 的映射列表
     */
    static RTree<T> create(const std::vector<std::pair<ParameterPoint, T>>& entries)
    {
        MC_ASSERT_RELEASE(!entries.empty() && "RTree::create: need at least one entry");

        // 将条目转换为叶节点
        std::vector<std::unique_ptr<RTreeNode<T>>> leaves;
        leaves.reserve(entries.size());
        for (const auto& entry : entries) {
            leaves.push_back(std::make_unique<RTreeLeaf<T>>(entry.first, entry.second));
        }

        auto root = build(std::move(leaves));
        return RTree(std::move(root));
    }

    /**
     * @brief 搜索最近的值
     *
     * 使用上次搜索结果缓存以加速分支限界搜索。
     * 此方法为 const，缓存更新是逻辑 const 操作。
     */
    const T& search(const TargetPoint& target) const
    {
        const auto targetArray = target.toParameterArray();
        const RTreeNode<T>* last = m_cache.lastResult;
        const RTreeNode<T>* result = m_root->search(targetArray.data(), last);
        m_cache.lastResult = result;

        // result 一定是 Leaf
        auto* leaf = static_cast<const RTreeLeaf<T>*>(result);
        return leaf->value;
    }

    RTree() = default;
    RTree(RTree&&) = default;
    RTree& operator=(RTree&&) = default;

    // 不可拷贝（包含 unique_ptr）
    RTree(const RTree&) = delete;
    RTree& operator=(const RTree&) = delete;

private:
    std::unique_ptr<RTreeNode<T>> m_root;
    mutable RTreeCache<T> m_cache;

    explicit RTree(std::unique_ptr<RTreeNode<T>> root)
        : m_root(std::move(root))
    {}

    // ============================================================================
    // 构建算法
    // ============================================================================

    /// 递归构建 RTree
    static std::unique_ptr<RTreeNode<T>> build(std::vector<std::unique_ptr<RTreeNode<T>>> nodes)
    {
        if (nodes.size() == 1) {
            return std::move(nodes[0]);
        }

        if (nodes.size() <= static_cast<size_t>(CHILDREN_PER_NODE)) {
            // 小列表：按质心绝对值之和排序，直接创建 SubTree
            sortByMidpointAbsSum(nodes);
            return std::make_unique<RTreeSubTree<T>>(std::move(nodes));
        }

        // 大列表：尝试所有 7 个维度，选择最佳分桶
        return buildLarge(std::move(nodes));
    }

    /**
     * @brief 对大列表构建 RTree（尝试所有维度，选择代价最小的分桶）
     *
     * 算法步骤：
     * 1. 对每个维度，按该维度中点排序后分桶，计算所有桶的总代价
     * 2. 选择总代价最小的维度作为最佳维度
     * 3. 用最佳维度的绝对值模式重新排序，然后分桶
     * 4. 递归构建每个桶
     */
    static std::unique_ptr<RTreeNode<T>> buildLarge(std::vector<std::unique_ptr<RTreeNode<T>>> nodes)
    {
        i64 bestCost = std::numeric_limits<i64>::max();
        i32 bestDim = 0;

        // 使用索引方式避免所有权问题：排序索引而非移动节点
        // 对每个维度进行排序和分桶，仅计算代价以选择最佳维度
        for (i32 dim = 0; dim < PARAMETER_COUNT; ++dim) {
            // 创建索引数组
            std::vector<size_t> indices(nodes.size());
            std::iota(indices.begin(), indices.end(), size_t{0});

            // 按维度 dim 排序索引（非绝对值）
            std::sort(indices.begin(), indices.end(), [&nodes, dim](size_t a, size_t b) {
                for (i32 offset = 0; offset < PARAMETER_COUNT; ++offset) {
                    const i32 d = (dim + offset) % PARAMETER_COUNT;
                    const i64 midA = (nodes[a]->parameterSpace[d].min + nodes[a]->parameterSpace[d].max) / 2;
                    const i64 midB = (nodes[b]->parameterSpace[d].min + nodes[b]->parameterSpace[d].max) / 2;
                    if (midA != midB) {
                        return midA < midB;
                    }
                }
                return false;
            });

            // 分桶并计算代价
            auto bucketRanges = bucketizeIndices(nodes.size(), indices);

            i64 totalCost = 0;
            for (const auto& range : bucketRanges) {
                std::array<Parameter, PARAMETER_COUNT> space{};
                bool initialized = false;
                for (size_t idx : range) {
                    for (i32 i = 0; i < PARAMETER_COUNT; ++i) {
                        if (!initialized) {
                            space[i] = nodes[idx]->parameterSpace[i];
                        } else {
                            space[i] = Parameter::merge(space[i], nodes[idx]->parameterSpace[i]);
                        }
                    }
                    if (!initialized) {
                        initialized = true;
                    }
                }
                totalCost += cost(space);
            }

            if (totalCost < bestCost) {
                bestCost = totalCost;
                bestDim = dim;
            }
        }

        // 用最佳维度的绝对值模式重新排序节点
        std::sort(nodes.begin(),
            nodes.end(),
            [bestDim](const std::unique_ptr<RTreeNode<T>>& a, const std::unique_ptr<RTreeNode<T>>& b) {
                for (i32 offset = 0; offset < PARAMETER_COUNT; ++offset) {
                    const i32 d = (bestDim + offset) % PARAMETER_COUNT;
                    const i64 midA = midpoint(*a, d, true);
                    const i64 midB = midpoint(*b, d, true);
                    if (midA != midB) {
                        return midA < midB;
                    }
                }
                return false;
            });

        // 分桶并递归构建
        auto buckets = bucketize(nodes);

        std::vector<std::unique_ptr<RTreeNode<T>>> subtrees;
        subtrees.reserve(buckets.size());
        for (auto& bucket : buckets) {
            subtrees.push_back(build(std::move(bucket)));
        }

        return std::make_unique<RTreeSubTree<T>>(std::move(subtrees));
    }

    // ============================================================================
    // 排序方法
    // ============================================================================

    /// 按质心绝对值之和排序（小列表时使用）
    static void sortByMidpointAbsSum(std::vector<std::unique_ptr<RTreeNode<T>>>& nodes)
    {
        std::sort(nodes.begin(),
            nodes.end(),
            [](const std::unique_ptr<RTreeNode<T>>& a, const std::unique_ptr<RTreeNode<T>>& b) {
                i64 sumA = 0, sumB = 0;
                for (i32 i = 0; i < PARAMETER_COUNT; ++i) {
                    sumA += std::abs(midpoint(*a, i, true));
                    sumB += std::abs(midpoint(*b, i, true));
                }
                return sumA < sumB;
            });
    }

    // ============================================================================
    // 分桶方法
    // ============================================================================

    /**
     * @brief 将排序后的节点分桶
     *
     * 桶大小 = 6^floor(log_6(size - 0.01))
     */
    static std::vector<std::vector<std::unique_ptr<RTreeNode<T>>>> bucketize(
        std::vector<std::unique_ptr<RTreeNode<T>>>& nodes)
    {
        if (nodes.empty()) {
            return {};
        }

        const f64 logBase = std::log(static_cast<f64>(nodes.size()) - 0.01) / std::log(6.0);
        const i32 bucketSize = std::max(static_cast<i32>(std::pow(6.0, std::floor(logBase))), 1);

        std::vector<std::vector<std::unique_ptr<RTreeNode<T>>>> buckets;
        std::vector<std::unique_ptr<RTreeNode<T>>> current;

        for (auto& node : nodes) {
            current.push_back(std::move(node));
            if (static_cast<i32>(current.size()) >= bucketSize) {
                buckets.push_back(std::move(current));
                current.clear();
            }
        }

        if (!current.empty()) {
            buckets.push_back(std::move(current));
        }

        return buckets;
    }

    // ============================================================================
    // 辅助方法
    // ============================================================================

    /// 基于索引的分桶（不移动节点所有权，仅返回索引分组）
    static std::vector<std::vector<size_t>> bucketizeIndices(size_t totalSize, const std::vector<size_t>& sortedIndices)
    {
        if (sortedIndices.empty()) {
            return {};
        }

        const f64 logBase = std::log(static_cast<f64>(totalSize) - 0.01) / std::log(6.0);
        const i32 bucketSize = std::max(static_cast<i32>(std::pow(6.0, std::floor(logBase))), 1);

        std::vector<std::vector<size_t>> buckets;
        std::vector<size_t> current;

        for (size_t idx : sortedIndices) {
            current.push_back(idx);
            if (static_cast<i32>(current.size()) >= bucketSize) {
                buckets.push_back(std::move(current));
                current.clear();
            }
        }

        if (!current.empty()) {
            buckets.push_back(std::move(current));
        }

        return buckets;
    }

    /// 计算参数空间代价（各维度跨度之和）
    static i64 cost(const std::array<Parameter, PARAMETER_COUNT>& space)
    {
        i64 total = 0;
        for (i32 i = 0; i < PARAMETER_COUNT; ++i) {
            total += std::abs(space[i].max - space[i].min);
        }
        return total;
    }

    /// 获取节点在指定维度的中点值（可选绝对值）
    static i64 midpoint(const RTreeNode<T>& node, i32 dim, bool useAbs)
    {
        const i64 mid = (node.parameterSpace[dim].min + node.parameterSpace[dim].max) / 2;
        return useAbs ? std::abs(mid) : mid;
    }
};

} // namespace mc::world::biome::climate
