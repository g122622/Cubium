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
 * @file PathNavigator.hpp
 * @brief 路径导航器（对应原版 `PathNavigation` + `GroundPathNavigation`）。
 *
 * 三件事：
 *   1. **建路径**：把目标解析成方块坐标，圈出一块只读世界区域，交给 `PathFinder` 求解；
 *   2. **跟随路径**：每 tick 判断是否已走到当前路径点、能否「抄近路」跳过它，然后让
 *      移动控制器朝「下一个路径点的实体坐标 + 地面高度」进发；
 *   3. **异常处理**：卡住检测（100 tick 内位移不足）、路径超时（原地徘徊超过预计耗时
 *      的 3 倍）、路径修剪（锅需要抬高、阳光避让需要截断）。
 *
 * 时间口径：用内部 tick 计数器而不是系统时钟。寻路跟随的所有时间量（重算间隔 20 tick、
 * 卡住采样 100 tick、超时累计）都以 tick 计，读时钟既慢又会让同一 tick 内结果不确定。
 *
 * 子类通过 `_canUpdatePath` / `_getTempMobPosition` / `_trimPath` / `_canMoveDirectly`
 * 表达不同移动方式（地面、飞行、水生、攀爬）。
 */

#pragma once

#include "NodeEvaluator.hpp"
#include "Path.hpp"
#include "PathFinder.hpp"
#include "PathType.hpp"
#include "Region.hpp"
#include "WorldRegion.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>
#include <optional>
#include <vector>

namespace mc {

class LivingEntity;
class MobEntity;

namespace entity::ai::pathfinding {

/**
 * @brief 路径导航器
 */
class PathNavigator {
public:
    /// 重算路径的最小间隔（tick）。
    static constexpr i32 MAX_TIME_RECOMPUTE = 20;

    /// 卡住检测的采样间隔（tick）。
    static constexpr i32 STUCK_CHECK_INTERVAL = 100;

    /// 卡住判定：位移小于「速度 × 100 × 0.25」的平方。
    static constexpr f32 STUCK_THRESHOLD_DISTANCE_FACTOR = 0.25f;

    /**
     * @brief 构造
     * @param mob 拥有本导航器的生物
     * @param pathFinder 寻路器（接管所有权）
     */
    PathNavigator(MobEntity* mob, std::unique_ptr<PathFinder> pathFinder);

    virtual ~PathNavigator() = default;

    PathNavigator(const PathNavigator&) = delete;
    PathNavigator& operator=(const PathNavigator&) = delete;

    // ========== 建路径 ==========

    /**
     * @brief 走到指定坐标
     * @param x, y, z 目标坐标（小数会被取整到方块）
     * @param reachRange 判定「到达」的曼哈顿半径
     * @param speed 速度倍率
     * @return 是否成功建立并开始跟随路径
     */
    [[nodiscard]] bool moveTo(f64 x, f64 y, f64 z, i32 reachRange, f64 speed);

    /**
     * @brief 走到指定坐标（到达半径 1）
     */
    [[nodiscard]] bool moveTo(f64 x, f64 y, f64 z, f64 speed);

    /**
     * @brief 走到与目标保持给定距离（曼哈顿）的位置
     */
    [[nodiscard]] bool moveToRange(f64 x, f64 y, f64 z, i32 reachRange, f64 speed);

    /**
     * @brief 走到某个实体所在位置（从该实体上方一格起算，到达半径 16）
     */
    [[nodiscard]] bool moveTo(const LivingEntity& target, f64 speed);

    /**
     * @brief 直接采用一条已算好的路径
     * @param path 路径；空表示放弃导航
     * @param speed 速度倍率
     * @return 是否开始跟随
     */
    [[nodiscard]] bool moveTo(std::optional<Path> path, f64 speed);

    // ========== 路径状态 ==========

    [[nodiscard]] bool hasPath() const noexcept
    {
        return m_path.has_value() && !m_path->isDone() && !m_path->empty();
    }

    [[nodiscard]] bool noPath() const noexcept
    {
        return !hasPath();
    }

    [[nodiscard]] const Path* getPath() const noexcept
    {
        return m_path.has_value() ? &*m_path : nullptr;
    }

    /// 是否已走完（没有路径，或路径游标到头）。
    [[nodiscard]] bool isDone() const noexcept
    {
        return !m_path.has_value() || m_path->isDone();
    }

    [[nodiscard]] bool isInProgress() const noexcept
    {
        return !isDone();
    }

    /// 是否处于卡住状态。
    [[nodiscard]] bool isStuck() const noexcept
    {
        return m_isStuck;
    }

    /// 当前跟随速度倍率。
    [[nodiscard]] f64 getSpeed() const noexcept
    {
        return m_speed;
    }

    void setSpeed(f64 speed) noexcept
    {
        m_speed = speed;
    }

    /**
     * @brief 放弃当前路径
     */
    void stop() noexcept
    {
        m_path.reset();
    }

    /**
     * @brief 立即重算到当前目标的路径（20 tick 内重复请求会延迟到下次 tick）
     */
    void recomputePath();

    // ========== 更新 ==========

    /**
     * @brief 每 tick 推进：处理延迟重算、跟随路径、把下一个路径点交给移动控制器
     */
    virtual void tick();

    // ========== 配置 ==========

    void setMaxVisitedNodesMultiplier(f32 multiplier) noexcept
    {
        m_maxVisitedNodesMultiplier = multiplier;
    }

    void resetMaxVisitedNodesMultiplier() noexcept
    {
        m_maxVisitedNodesMultiplier = 1.0f;
    }

    [[nodiscard]] f32 maxVisitedNodesMultiplier() const noexcept
    {
        return m_maxVisitedNodesMultiplier;
    }

    /// 允许的最小行走距离（与跟随范围取较大者作为搜索距离上限）。
    void setRequiredPathLength(f32 length) noexcept;

    [[nodiscard]] f32 requiredPathLength() const noexcept
    {
        return m_requiredPathLength;
    }

    void setCanFloat(bool canFloat);

    [[nodiscard]] bool canFloat() const;

    void setCanPassDoors(bool canPassDoors);

    [[nodiscard]] bool canPassDoors() const;

    void setCanOpenDoors(bool canOpenDoors);

    [[nodiscard]] bool canOpenDoors() const;

    /// 是否允许走栅栏上方。
    void setCanWalkOverFences(bool canWalkOverFences);

    /// 是否避开阳光（亡灵生物白天用）。
    void setAvoidSun(bool avoidSun) noexcept
    {
        m_avoidSun = avoidSun;
    }

    [[nodiscard]] bool avoidsSun() const noexcept
    {
        return m_avoidSun;
    }

    /// 当前到目标的判定半径。
    [[nodiscard]] i32 reachRange() const noexcept
    {
        return m_reachRange;
    }

    /// 目标方块坐标（尚未建过路径时为空）。
    [[nodiscard]] const BlockPos* targetPos() const noexcept
    {
        return m_targetPos.has_value() ? &*m_targetPos : nullptr;
    }

    [[nodiscard]] f32 maxDistanceToWaypoint() const noexcept
    {
        return m_maxDistanceToWaypoint;
    }

    [[nodiscard]] PathFinder* pathFinder() noexcept
    {
        return m_pathFinder.get();
    }

    [[nodiscard]] NodeEvaluator* nodeEvaluator() noexcept;

    /**
     * @brief 给定方块是否应触发「附近路径需要重算」
     *
     * 用于方块变化（门开了、路被堵了）后由外部主动发起重算。
     */
    [[nodiscard]] bool shouldRecomputePath(const BlockPos& pos) const;

    /**
     * @brief 给定位置是否是稳定的落脚点（下方方块渲染为实心）
     */
    [[nodiscard]] bool isStableDestination(const BlockPos& pos) const;

    // ========== 子类接口 ==========

    [[nodiscard]] virtual bool canNavigateGround() const = 0;

    /**
     * @brief 用指定区域计算「走到某坐标时脚应到达的 Y」（差分探针与单元测试复用同一口径）
     */
    [[nodiscard]] static f64 groundYAt(const Region& region, const Vector3d& position);

protected:
    /**
     * @brief 实体当前是否处于「可以跟随路径」的状态
     *
     * 地面寻路要求站在地面、液体中或骑乘中；悬空下落时无法跟随路径。
     */
    [[nodiscard]] virtual bool _canUpdatePath() const;

    /**
     * @brief 用于卡住检测与路径跟随的实体位置
     *
     * 地面导航取「实体水平坐标 + 地表 Y」，因此实体在空中下落时不会触发卡住判定。
     */
    [[nodiscard]] virtual Vector3d _getTempMobPosition() const;

    /**
     * @brief 修剪路径（锅需要把路径点抬高，阳光避让需要截断）
     */
    virtual void _trimPath();

    /**
     * @brief 阳光避让：路径在第一个暴露于天空的路径点处截断
     *
     * 仅当 `avoidsSun()` 为真时生效；实体自己已经暴露在阳光下时不截断，
     * 否则它会连逃离的路径都没有。
     */
    void _trimPathSun();

    /**
     * @brief 能否从当前位置直接走到目标位置（飞行 / 水生导航实现）
     */
    [[nodiscard]] virtual bool _canMoveDirectly(const Vector3d& from, const Vector3d& to) const;

    /**
     * @brief 能否「抄拐角」：跳过当前路径点直接奔下一个
     */
    [[nodiscard]] virtual bool canCutCorner(PathType type) const;

    /**
     * @brief 由目标坐标集合建路径
     *
     * @param targets 目标坐标（至少一个）
     * @param searchRange 世界区域的水平半径（格）
     * @param above 是否从实体上方一格起算
     * @param reachRange 判定到达的曼哈顿半径
     * @param maxPathLength 允许的最大行走距离
     */
    [[nodiscard]] std::optional<Path> _createPath(const std::vector<BlockPos>& targets,
        i32 searchRange,
        bool above,
        i32 reachRange,
        f32 maxPathLength);

    /**
     * @brief 用单个方块建路径
     */
    [[nodiscard]] std::optional<Path> _createPath(const BlockPos& target, i32 reachRange, f32 maxPathLength);

    /**
     * @brief 每个 tick 的路径跟随
     */
    virtual void _followThePath();

    /**
     * @brief 能否直接奔「下一个路径点之后」的那个节点
     */
    [[nodiscard]] bool _shouldTargetNextNodeInDirection(const Vector3d& position) const;

    /**
     * @brief 卡住与超时检测
     */
    void _doStuckDetection(const Vector3d& position);

    /**
     * @brief 路径点对应的地面高度
     *
     * 路径点坐标是「格子坐标」，而实体要站在格子底面上方。当下方是空气时（空中路径）
     * 沿用路径点 Y；否则取下方方块碰撞形状的顶面——营火、半砖、台阶都需要这个修正，
     * 否则实体会被要求走进方块内部。
     */
    [[nodiscard]] f64 _getGroundY(const Vector3d& position) const;

    /**
     * @brief 清除超时状态
     */
    void _resetStuckTimeout();

    /// 实际的生物与寻路器。
    MobEntity* m_mob;
    std::unique_ptr<PathFinder> m_pathFinder;

    std::optional<Path> m_path;
    std::optional<BlockPos> m_targetPos;
    f64 m_speed = 0.0;
    f32 m_requiredPathLength = 16.0f;
    i32 m_reachRange = 0;
    f32 m_maxVisitedNodesMultiplier = 1.0f;

    /// 内部 tick 计数器；所有时间量都以它为单位。
    i64 m_tick = 0;

    i32 m_lastStuckCheck = 0;
    Vector3d m_lastStuckCheckPos;
    bool m_isStuck = false;

    BlockPos m_timeoutCachedNode;
    i64 m_timeoutTimer = 0;
    i64 m_lastTimeoutCheck = 0;
    f64 m_timeoutLimit = 0.0;

    f32 m_maxDistanceToWaypoint = 0.5f;
    bool m_hasDelayedRecomputation = false;
    i64 m_timeLastRecompute = 0;
    bool m_avoidSun = false;
};

} // namespace entity::ai::pathfinding
} // namespace mc