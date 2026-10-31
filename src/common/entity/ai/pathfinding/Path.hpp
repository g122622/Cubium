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

/**
 * @file Path.hpp
 * @brief 一条寻路结果（对应原版 `Path`）。
 *
 * 路径是**快照**：一旦生成就不再改动（`trimPath` 是对它的显式后处理），因此节点以值
 * 的形式持有拷贝，而不是指向 `NodeEvaluator` 缓存里的对象——后者只活一次寻路会话。
 *
 * 路径与实体是解耦的：`getEntityPosAtNode` 需要实体的碰撞箱宽度来决定「走到节点中心」
 * 还是「贴着节点边缘」，因此调用时要传入宽度。
 */

#pragma once

#include "PathPoint.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

namespace mc::entity::ai::pathfinding {

/**
 * @brief 一条寻路结果
 */
class Path {
public:
    Path() = default;

    /**
     * @brief 用节点序列构造路径
     * @param nodes 节点序列（起点在前）
     * @param target 本次寻路的目标方块坐标
     * @param reached 是否真的到达了目标
     */
    Path(std::vector<PathPoint> nodes, const BlockPos& target, bool reached)
        : m_nodes(std::move(nodes))
        , m_target(target)
        , m_reached(reached)
        , m_distanceToTarget(m_nodes.empty() ? std::numeric_limits<f32>::max()
                                             : m_nodes.back().distanceManhattan(m_target.x, m_target.y, m_target.z))
    {}

    // ========== 状态 ==========

    [[nodiscard]] const std::vector<PathPoint>& nodes() const
    {
        return m_nodes;
    }

    [[nodiscard]] size_t nodeCount() const
    {
        return m_nodes.size();
    }

    [[nodiscard]] bool empty() const
    {
        return m_nodes.empty();
    }

    /**
     * @brief 读取指定下标的节点
     * @param index 下标；调用方保证在范围内（原版同样不做边界检查）
     */
    [[nodiscard]] const PathPoint& node(size_t index) const
    {
        return m_nodes[index];
    }

    /// 终点节点；空路径返回 nullptr。
    [[nodiscard]] const PathPoint* endNode() const
    {
        return m_nodes.empty() ? nullptr : &m_nodes.back();
    }

    /// 本次寻路的目标坐标。
    [[nodiscard]] const BlockPos& target() const
    {
        return m_target;
    }

    /// 是否真的到达了目标（false 表示这是「尽可能靠近」的路径）。
    [[nodiscard]] bool canReach() const
    {
        return m_reached;
    }

    /// 终点到目标的曼哈顿距离；空路径为 f32 最大值。
    [[nodiscard]] f32 distanceToTarget() const
    {
        return m_distanceToTarget;
    }

    // ========== 游标 ==========

    [[nodiscard]] i32 nextNodeIndex() const
    {
        return m_nextNodeIndex;
    }

    void setNextNodeIndex(i32 index)
    {
        m_nextNodeIndex = index;
    }

    [[nodiscard]] bool notStarted() const
    {
        return m_nextNodeIndex <= 0;
    }

    [[nodiscard]] bool isDone() const
    {
        return m_nextNodeIndex >= static_cast<i32>(m_nodes.size());
    }

    /// 前进到下一个节点。
    void advance()
    {
        ++m_nextNodeIndex;
    }

    /// 下一个节点；调用方保证 `!isDone()`。
    [[nodiscard]] const PathPoint& nextNode() const
    {
        return m_nodes[static_cast<size_t>(m_nextNodeIndex)];
    }

    /// 下一个节点的方块坐标；调用方保证 `!isDone()`。
    [[nodiscard]] BlockPos nextNodePos() const
    {
        const PathPoint& point = nextNode();
        return BlockPos(point.x(), point.y(), point.z());
    }

    /// 上一个节点；还没开始时返回 nullptr。
    [[nodiscard]] const PathPoint* previousNode() const
    {
        return m_nextNodeIndex > 0 ? &m_nodes[static_cast<size_t>(m_nextNodeIndex - 1)] : nullptr;
    }

    /**
     * @brief 实体走到指定节点时应处的坐标
     *
     * 原版 `Path.getEntityPosAtNode`：半格偏移量为 `floor(宽度 + 1) / 2`，
     * 因此窄实体（宽度 < 1）走格子中心，宽实体走格子中心偏右半格（与它们碰撞箱的
     * 横向偏移一致）。两侧必须用同一个公式，否则路径跟随会系统性偏移。
     *
     * @param width 实体碰撞箱宽度
     * @param index 节点下标
     */
    [[nodiscard]] Vector3d entityPosAtNode(f32 width, size_t index) const;

    /// 下一个节点对应的实体坐标；调用方保证 `!isDone()`。
    [[nodiscard]] Vector3d nextEntityPos(f32 width) const;

    /// 实体坐标所处节点（原版 `Path.getNodePos`）。
    [[nodiscard]] BlockPos nodePos(size_t index) const
    {
        const PathPoint& point = m_nodes[index];
        return BlockPos(point.x(), point.y(), point.z());
    }

    // ========== 后处理 ==========

    /**
     * @brief 截断到指定长度
     * @param count 保留的节点数（从起点数起）
     */
    void truncateNodes(i32 count)
    {
        if (static_cast<i32>(m_nodes.size()) > count) {
            m_nodes.resize(static_cast<size_t>(count < 0 ? 0 : count));
        }
    }

    /**
     * @brief 替换指定下标的节点
     * @param index 下标
     * @param point 新节点
     */
    void replaceNode(size_t index, const PathPoint& point)
    {
        m_nodes[index] = point;
    }

    /**
     * @brief 两个路径是否由同一串节点构成
     *
     * 只比较坐标：游标、代价与调试信息都不参与。原版 `Path.sameAs` 依赖 `Node.equals`，
     * 而 `Node.equals` 只比较坐标，语义一致。
     */
    [[nodiscard]] bool sameAs(const Path* other) const
    {
        if (other == nullptr || m_nodes.size() != other->m_nodes.size()) {
            return false;
        }
        for (size_t index = 0; index < m_nodes.size(); ++index) {
            const PathPoint& left = m_nodes[index];
            const PathPoint& right = other->m_nodes[index];
            if (left.x() != right.x() || left.y() != right.y() || left.z() != right.z()) {
                return false;
            }
        }
        return true;
    }

    /// 复制一条路径（连同游标），对应原版 `Path.copy()`。
    [[nodiscard]] Path copy() const
    {
        Path copy = *this;
        return copy;
    }

private:
    std::vector<PathPoint> m_nodes;
    BlockPos m_target;
    bool m_reached = false;
    f32 m_distanceToTarget = std::numeric_limits<f32>::max();
    i32 m_nextNodeIndex = 0;
};

} // namespace mc::entity::ai::pathfinding