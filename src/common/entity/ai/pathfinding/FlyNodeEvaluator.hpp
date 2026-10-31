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
 * @file FlyNodeEvaluator.hpp
 * @brief 飞行寻路求值器（对应原版 `FlyNodeEvaluator`）。
 *
 * 与地面寻路的差别集中在三处：
 *   1. 邻域是完整的 3×3×3（26 个邻居），斜向移动还要求经过的两个正交格都可走；
 *   2. 可站立（WALKABLE）的格子代价额外 +1，飞行生物因此更愿意走空中；
 *   3. 类型判定会把下方方块的特征「上抬」到本格（下方是火 → 本格是 DAMAGE_FIRE，
 *      下方是栅栏 → 本格是 FENCE，等等），因为飞行实体贴地飞行时同样会被这些方块影响。
 *
 * 起点候选在窄实体时可以带随机性（原版用 `BlockPos.randomBetweenClosed`）。为保证
 * 确定性，这里只用实体的包围盒角点作为候选，因此实现了原版 `aabb.getSize() >= 1` 的分支；
 * 窄实体的随机候选留待需要时再补上。
 */

#pragma once

#include "WalkNodeEvaluator.hpp"
#include "common/core/Types.hpp"

namespace mc::entity::ai::pathfinding {

/**
 * @brief 飞行寻路求值器
 */
class FlyNodeEvaluator : public WalkNodeEvaluator {
public:
    /// 视为「小体型」的尺寸阈值（原版 `SMALL_MOB_SIZE`）。
    static constexpr f32 SMALL_MOB_SIZE = 1.0f;

    /// 小体型实体起点候选区间的膨胀量（原版 `SMALL_MOB_INFLATED_START_NODE_BOUNDING_BOX`）。
    static constexpr f32 SMALL_MOB_INFLATED_START_NODE_BOUNDING_BOX = 1.1f;

    /// 起点候选的最大数量（原版 `MAX_START_NODE_CANDIDATES`）。
    static constexpr i32 MAX_START_NODE_CANDIDATES = 10;

    FlyNodeEvaluator() = default;
    ~FlyNodeEvaluator() override = default;

    void prepare(const Region& region, const PathfindingEntity& entity, const Vector3d& entityPosition) override;
    void done() override;

    [[nodiscard]] PathPoint* getStart() override;
    [[nodiscard]] Target getTarget(f64 x, f64 y, f64 z) override;
    i32 getNeighbors(PathPoint** neighbors, PathPoint* point) override;

    [[nodiscard]] PathType getPathType(const PathfindingContext& context, i32 x, i32 y, i32 z) const override;

protected:
    [[nodiscard]] bool canStartAt(i32 x, i32 y, i32 z) const override;

    /**
     * @brief 飞行求值的单格候选：可走的格子写入类型与代价，可站立格代价 +1
     */
    [[nodiscard]] PathPoint* findFlyNode(i32 x, i32 y, i32 z);

    /**
     * @brief 带缓存的含实体类型查询（飞行求值器有自己的缓存，与地面求值器互不影响）
     */
    [[nodiscard]] PathType cachedPathType(i32 x, i32 y, i32 z);

private:
    /// 节点代价是否非负（原版 `hasMalus`）。
    [[nodiscard]] static bool hasMalus(const PathPoint* node)
    {
        return node != nullptr && node->costMalus() >= 0.0f;
    }

    /// 节点是否可以被加入（原版 `isOpen`）。
    [[nodiscard]] static bool isOpen(const PathPoint* node)
    {
        return node != nullptr && !node->closed();
    }

    std::unordered_map<u64, PathType> m_flyPathTypeCache;
};

} // namespace mc::entity::ai::pathfinding