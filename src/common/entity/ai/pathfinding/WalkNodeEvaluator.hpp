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
 * @file WalkNodeEvaluator.hpp
 * @brief 地面寻路求值器（对应原版 `WalkNodeEvaluator`）。
 *
 * 这是寻路正确性的核心：它决定「一格能不能站、站上去要付多少代价、能不能横跳上去、
 * 掉下去会不会摔死」。所有判定按原版的书写顺序逐条对齐，顺序本身是语义的一部分：
 *
 * 1. `getPathTypeFromState`：单格类型，只看该格及其流体；
 * 2. `getPathTypeStatic`：本格是 OPEN 时向下看一格，把「站在火上 / 甜浆果上 / 细雪上 /
 *    活板门上」改写成对应的 DANGER 类型，否则检查 3×3×3 邻域（火 / 岩浆→危险，水→水边，
 *    甜浆果 / 仙人掌→其他危险）；
 * 3. `getPathTypeWithinMobBB`：按实体占用的格子取集合，门与铁轨在此按能力改写；
 * 4. `getPathTypeOfMob`：集合里只要有 FENCE / UNPASSABLE_RAIL 就直接返回，否则取「对该实体
 *    代价最大」的类型，全部可走且有负代价类型则返回第一个负代价类型；
 * 5. `findAcceptedNode`：邻居是否可接受，含跳跃、跌落、找水底、找地面四类改写。
 */

#pragma once

#include "NodeEvaluator.hpp"
#include "PathPoint.hpp"
#include "PathType.hpp"
#include "Region.hpp"
#include "Target.hpp"
#include "common/core/Types.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include <array>
#include <cstddef>
#include <unordered_map>
#include <set>

namespace mc::entity::ai::pathfinding {

/**
 * @brief 地面寻路求值器
 */
class WalkNodeEvaluator : public NodeEvaluator {
public:
    /// 墙柱之间的空隙（原版 `SPACE_BETWEEN_WALL_POSTS`）。
    static constexpr f64 SPACE_BETWEEN_WALL_POSTS = 0.5;

    /// 实体的最小跳跃高度（原版 `DEFAULT_MOB_JUMP_HEIGHT`）。
    static constexpr f64 DEFAULT_MOB_JUMP_HEIGHT = 1.125;

    WalkNodeEvaluator() = default;
    ~WalkNodeEvaluator() override = default;

    // ========== 会话 ==========

    void prepare(const Region& region, const PathfindingEntity& entity, const Vector3d& entityPosition) override;
    void done() override;

    // ========== 搜索 ==========

    [[nodiscard]] PathPoint* getStart() override;
    [[nodiscard]] Target getTarget(f64 x, f64 y, f64 z) override;
    i32 getNeighbors(PathPoint** neighbors, PathPoint* point) override;

    // ========== 类型判定 ==========

    [[nodiscard]] PathType getPathType(const PathfindingContext& context, i32 x, i32 y, i32 z) const override;
    [[nodiscard]] PathType getPathTypeOfMob(const PathfindingContext& context, i32 x, i32 y, i32 z) const override;

    /**
     * @brief 单格静态类型（原版 `getPathTypeStatic`），与实体无关
     *
     * 差分探针直接拿它做逐格比对；生产路径经 `getPathType` → 缓存调用同一个实现。
     */
    [[nodiscard]] static PathType getPathTypeStatic(const Region& region, i32 x, i32 y, i32 z);

    /**
     * @brief 单格原始类型（原版 `getPathTypeFromState`），只看该格
     */
    [[nodiscard]] static PathType getPathTypeFromState(const Region& region, i32 x, i32 y, i32 z);

    /**
     * @brief 3×3×3 邻域危险检查（原版 `checkNeighbourBlocks`）
     * @param fallback 邻域内没有任何危险时返回的类型
     */
    [[nodiscard]] static PathType checkNeighbourBlocks(
        const Region& region, i32 x, i32 y, i32 z, PathType fallback);

    /**
     * @brief 某坐标是否为燃烧方块（原版 `NodeEvaluator.isBurningBlock`）
     */
    [[nodiscard]] static bool isBurningBlock(const BlockState& state);

    /**
     * @brief 地面高度（原版静态 `getFloorLevel`）：下方方块碰撞形状的顶面
     *
     * 这是「脚踩在多高」的唯一口径，跳跃、抬脚与路径跟随都靠它。台阶 / 半砖的顶面与
     * 整块不同，因此不能简单用 `y` 代替。
     */
    [[nodiscard]] static f64 getFloorLevel(const Region& region, i32 x, i32 y, i32 z);

    /**
     * @brief 实体脚下的地面高度（原版实例方法 `getFloorLevel`）：可漂浮生物在水里取水面
     */
    [[nodiscard]] f64 getFloorLevel(const BlockPos& pos) const;

protected:
    /**
     * @brief 指定坐标能否作为搜索起点（原版 `canStartAt`）
     */
    [[nodiscard]] virtual bool canStartAt(i32 x, i32 y, i32 z) const;

    /**
     * @brief 由坐标构造起点节点并写入类型与代价
     */
    [[nodiscard]] PathPoint* getStartNode(i32 x, i32 y, i32 z);

    /// 能否两栖（水面按地面处理）；只有两栖求值器覆写。
    [[nodiscard]] virtual bool isAmphibious() const
    {
        return false;
    }

    /**
     * @brief 枚举并筛选邻居
     * @param point 当前节点
     * @param jumpHeight 允许向上跳的格数
     * @param floorLevel 当前节点所在格的地面高度
     * @param currentType 当前节点自身类型
     * @param neighbors 输出缓冲
     * @return 写入数量
     */
    i32 collectNeighbors(PathPoint* point, i32 jumpHeight, f64 floorLevel, PathType currentType, PathPoint** neighbors);

    /**
     * @brief 判定一个候选邻居是否可接受，并可把它改写成跳跃 / 跌落后的节点
     *
     * @param x, y, z 候选坐标
     * @param jumpHeight 剩余可跳跃格数
     * @param floorLevel 当前节点的地面高度
     * @param direction 相对当前节点的主方向（用于窄实体窄缝碰撞盒）
     * @param neighborType 当前节点自身类型
     * @return 可接受的节点；不可接受返回 nullptr
     */
    [[nodiscard]] virtual PathPoint* findAcceptedNode(
        i32 x, i32 y, i32 z, i32 jumpHeight, f64 floorLevel, i32 stepX, i32 stepZ, PathType neighborType);

    /**
     * @brief 邻居是否有效（原版 `isNeighborValid`）
     */
    [[nodiscard]] bool isNeighborValid(const PathPoint* neighbor, const PathPoint& point) const;

    /**
     * @brief 对角邻居的两个正交邻居是否都允许通过（原版 `isDiagonalValid`）
     */
    [[nodiscard]] bool isDiagonalValid(const PathPoint& point, const PathPoint* sideA, const PathPoint* sideB) const;

    /**
     * @brief 对角邻居自身是否有效（原版 `isDiagonalValid(Node)`）
     */
    [[nodiscard]] bool isDiagonalValid(const PathPoint* diagonal) const;

    /**
     * @brief 带缓存的、含实体尺寸的类型查询（原版 `getCachedPathType`）
     */
    [[nodiscard]] PathType cachedPathType(i32 x, i32 y, i32 z);

    /**
     * @brief 实体包围盒覆盖的全部格子类型（原版 `getPathTypeWithinMobBB`）
     */
    [[nodiscard]] std::set<PathType> pathTypeWithinMobBb(
        const PathfindingContext& context, i32 x, i32 y, i32 z) const;

    /**
     * @brief 盒子是否与实体碰撞（原版 `hasCollisions`，带 AABB 缓存）
     */
    [[nodiscard]] bool hasCollisions(const AxisAlignedBB& box);

    /// 实体跳跃高度（原版 `getMobJumpHeight`）。
    [[nodiscard]] f64 mobJumpHeight() const;

    /**
     * @brief 取得节点并把代价抬到不小于给定值（原版 `getNodeAndUpdateCostToMax`）
     */
    [[nodiscard]] PathPoint* nodeAndUpdateCostToMax(i32 x, i32 y, i32 z, PathType type, f32 malus);

    /**
     * @brief 取得一个「被阻塞」的节点（原版 `getBlockedNode`）
     */
    [[nodiscard]] PathPoint* blockedNode(i32 x, i32 y, i32 z);

    /**
     * @brief 取得一个「已关闭」的节点（原版 `getClosedNode`）
     */
    [[nodiscard]] PathPoint* closedNode(i32 x, i32 y, i32 z, PathType type);

    /**
     * @brief 尝试跳到上层坐标（原版 `tryJumpOn`）
     */
    [[nodiscard]] PathPoint* tryJumpOn(
        i32 x, i32 y, i32 z, i32 jumpHeight, f64 floorLevel, i32 stepX, i32 stepZ, PathType neighborType);

    /**
     * @brief 向下找第一个非水节点（原版 `tryFindFirstNonWaterBelow`）
     */
    [[nodiscard]] PathPoint* tryFindFirstNonWaterBelow(i32 x, i32 y, i32 z, PathPoint* fallback);

    /**
     * @brief 向下找第一个可站立的地面节点（原版 `tryFindFirstGroundNodeBelow`）
     */
    [[nodiscard]] PathPoint* tryFindFirstGroundNodeBelow(i32 x, i32 y, i32 z);

    /**
     * @brief 实体能否沿直线到达该节点而不碰撞（原版 `canReachWithoutCollision`）
     */
    [[nodiscard]] bool canReachWithoutCollision(const PathPoint& point);

    /**
     * @brief 部分阻挡类型（原版 `doesBlockHavePartialCollision`）
     */
    [[nodiscard]] static constexpr bool doesBlockHavePartialCollision(PathType type) noexcept
    {
        return type == PathType::Fence || type == PathType::DoorWoodClosed || type == PathType::DoorIronClosed;
    }

    /// 一次 `findAcceptedNode` 调用的公共参数，避免长参数列表在递归里重复传参。
    struct NeighborQuery {
        i32 jumpHeight = 0;
        f64 floorLevel = 0.0;
        i32 stepX = 0;
        i32 stepZ = 0;
        PathType selfType = PathType::Blocked;
    };

    /// 单格类型缓存（会话内有效）。
    std::unordered_map<u64, PathType> m_pathTypeCache;

    /// 碰撞查询缓存；只缓存「有碰撞」的结果不需要，统一缓存布尔值。
    std::unordered_map<u64, bool> m_collisionCache;

    /// 四个水平方向上的邻居，供对角判定复用（同原版 `reusableNeighbors`）。
    std::array<PathPoint*, 4> m_reusableNeighbors{};
};

} // namespace mc::entity::ai::pathfinding