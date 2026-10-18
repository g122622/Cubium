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

#include "common/entity/ai/pathfinding/PathPoint.hpp"
#include "core/Types.hpp"
#include "entity/ai/goal/GoalConstants.hpp"
#include "entity/ai/pathfinding/Path.hpp"
#include "entity/ai/pathfinding/PathFinder.hpp"
#include <memory>

namespace mc {

// 前向声明
class Entity;
class IWorld;
class LivingEntity;
class MobEntity;

namespace entity::ai::pathfinding {

/**
 * @brief 路径导航器基类
 *
 * 负责计算路径、沿路径移动实体、处理路径中断等。
 * 不同移动类型（地面、飞行、游泳）有不同的实现。
 */
class PathNavigator {
public:
    /**
     * @brief 构造函数
     * @param finder 寻路器
     */
    explicit PathNavigator(std::unique_ptr<PathFinder> finder);

    /**
     * @brief 构造函数（从 MobEntity）
     * @param mob 拥有此导航器的生物
     */
    explicit PathNavigator(MobEntity* mob);

    PathNavigator(PathNavigator&& other) noexcept = default;
    PathNavigator& operator=(PathNavigator&& other) noexcept = default;
    virtual ~PathNavigator() = default;

    // ========== 路径计算 ==========

    /**
     * @brief 计算到指定坐标的路径
     * @param x 目标X
     * @param y 目标Y
     * @param z 目标Z
     * @param speed 移动速度
     * @return 是否找到路径
     */
    [[nodiscard]] bool moveTo(f64 x, f64 y, f64 z, f64 speed = 1.0);

    /**
     * @brief 计算到实体的路径
     * @param target 目标实体
     * @param speed 移动速度
     * @return 是否找到路径
     */
    [[nodiscard]] bool moveTo(const Entity& target, f64 speed = 1.0);

    /**
     * @brief 计算到目标范围内的路径
     * @param x 目标X
     * @param y 目标Y
     * @param z 目标Z
     * @param range 目标范围
     * @param speed 移动速度
     * @return 是否找到路径
     */
    [[nodiscard]] bool moveToRange(f64 x, f64 y, f64 z, f32 range, f64 speed = 1.0);

    // ========== 路径状态 ==========

    /**
     * @brief 检查是否有路径
     */
    [[nodiscard]] bool hasPath() const noexcept { return m_path && !m_path->empty(); }

    /**
     * @brief 检查是否没有路径（用于AI目标判断）
     */
    [[nodiscard]] bool noPath() const noexcept { return !hasPath(); }

    /**
     * @brief 获取当前路径
     */
    [[nodiscard]] const Path* getPath() const noexcept { return m_path.get(); }

    /**
     * @brief 获取当前路径点索引
     */
    [[nodiscard]] i32 getCurrentIndex() const;

    /**
     * @brief 检查路径是否完成
     */
    [[nodiscard]] bool isDone() const noexcept { return !m_path || m_path->isFinished(); }

    /**
     * @brief 检查是否正在跟随路径
     */
    [[nodiscard]] bool isInProgress() const noexcept { return hasPath() && !isDone(); }

    // ========== 路径控制 ==========

    /**
     * @brief 清除当前路径
     */
    void clearPath() noexcept { m_path.reset(); }

    /**
     * @brief 停止导航
     */
    void stop() noexcept
    {
        clearPath();
        m_speed = 1.0;
    }

    /**
     * @brief 重试路径计算
     * @return 是否成功
     */
    [[nodiscard]] bool recomputePath();

    // ========== 更新 ==========

    /**
     * @brief 每tick更新
     */
    virtual void tick();

    // ========== 配置 ==========

    /**
     * @brief 设置最大搜索距离
     */
    void setMaxDistance(i32 distance) noexcept { m_maxDistance = distance; }

    /**
     * @brief 设置已访问节点数倍率
     *
     * 对应 MC Java 的 PathNavigation.setMaxVisitedNodesMultiplier()。
     * 此倍率与 PathFinder 的 maxNodes 相乘，得到实际搜索节点上限。
     * 例如：0.5F 表示搜索量减半（降低精度但提高性能），
     *       10.0F 表示搜索量扩大10倍（提高精度确保路径可达）。
     * 默认值为 1.0F。
     *
     * @param multiplier 倍率，必须为正数
     */
    void setMaxVisitedNodesMultiplier(f32 multiplier) noexcept { m_maxVisitedNodesMultiplier = multiplier; }

    /**
     * @brief 重置已访问节点数倍率为默认值（1.0F）
     *
     * 对应 MC Java 的 PathNavigation.resetMaxVisitedNodesMultiplier()。
     * 通常在 AI Goal 的 stop() 或 resetTask() 中调用，
     * 确保倍率不会影响后续不相关的寻路请求。
     */
    void resetMaxVisitedNodesMultiplier() noexcept { m_maxVisitedNodesMultiplier = 1.0f; }

    /**
     * @brief 获取已访问节点数倍率
     */
    [[nodiscard]] f32 getMaxVisitedNodesMultiplier() const noexcept { return m_maxVisitedNodesMultiplier; }

    /**
     * @brief 设置重试间隔
     */
    void setRetryInterval(i32 interval) noexcept { m_retryInterval = interval; }

    /**
     * @brief 设置是否可以游泳
     */
    void setCanSwim(bool canSwim) noexcept { m_canSwim = canSwim; }

    /**
     * @brief 设置是否可以开门
     *
     * 当设置为 true 时，寻路系统会识别关闭的木门为 WalkableDoor 类型，
     * 允许实体生成穿过门的路径。此设置会传播到关联的 WalkNodeProcessor。
     */
    void setCanOpenDoors(bool canOpenDoors);

    /**
     * @brief 获取是否可以开门
     */
    [[nodiscard]] bool canOpenDoors() const noexcept { return m_canOpenDoors; }

    /**
     * @brief 设置是否可以通过门
     *
     * 当设置为 true 时，打开的门被视为可通行（DoorOpen），关闭的栅栏门也可通行（FenceGate）。
     * 当设置为 false 时，打开的门被视为阻塞，栅栏门也不可通行。
     * 此设置会传播到关联的 WalkNodeProcessor。
     * 默认值为 true，与 MC Java 的 NodeEvaluator.canPassDoors 默认值一致。
     */
    void setCanEnterDoors(bool canEnterDoors);

    /**
     * @brief 获取是否可以通过门
     */
    [[nodiscard]] bool canEnterDoors() const noexcept { return m_canEnterDoors; }

    /**
     * @brief 设置寻路时是否避开阳光
     *
     * 当设置为 true 时，路径导航器在计算路径后会截断暴露在阳光下的部分：
     * - 如果实体当前位置已在阳光下，保留完整路径（实体需要移动来逃离）
     * - 如果实体当前位置不在阳光下，截断路径中第一个暴露在阳光下的节点及之后的所有节点
     * 这使亡灵生物（如骷髅）在白天会避免走入阳光直射区域。
     *
     * @param avoidSun 是否避开阳光
     */
    void setAvoidSunPathing(bool avoidSun);

    /**
     * @brief 设置关联的实体
     */
    void setEntity(LivingEntity* entity) noexcept { m_entity = entity; }

    /**
     * @brief 设置移动速度
     */
    void setSpeed(f64 speed) noexcept { m_speed = speed; }

    // ========== 调试 ==========

    /**
     * @brief 获取寻路器
     */
    [[nodiscard]] PathFinder* getPathFinder() noexcept { return m_pathFinder.get(); }

    /**
     * @brief 获取当前速度
     */
    [[nodiscard]] f64 getSpeed() const noexcept { return m_speed; }

    /**
     * @brief 检查是否卡住
     */
    [[nodiscard]] bool isStuck() const noexcept { return m_isStuck; }

protected:
    std::unique_ptr<PathFinder> m_pathFinder;
    std::unique_ptr<Path> m_path;
    LivingEntity* m_entity = nullptr;

    f64 m_speed = 1.0;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    i32 m_maxDistance = 100;
    f32 m_maxVisitedNodesMultiplier = 1.0f; ///< 已访问节点数倍率，对应 MC Java 的 maxVisitedNodesMultiplier
    i32 m_retryInterval = 20;
    i32 m_retryTimer = 0;
    i32 m_ticksSinceLastPath = 0;

    // 卡住检测相关字段
    f64 m_lastPosX = 0.0;
    f64 m_lastPosY = 0.0;
    f64 m_lastPosZ = 0.0;
    i32 m_stuckTimer = 0;

    // 超时检测字段
    i32 m_timeoutCachedNodeX = 0;
    i32 m_timeoutCachedNodeY = 0;
    i32 m_timeoutCachedNodeZ = 0;
    i64 m_timeoutTimer = 0;
    i64 m_lastTimeoutCheck = 0;
    f64 m_timeoutLimit = 0.0;
    bool m_isStuck = false;

    bool m_canSwim = false;
    bool m_canOpenDoors = false;
    bool m_canEnterDoors = true;
    bool m_avoidSun = false;

    // ========== 内部方法 ==========

    /**
     * @brief 沿路径移动实体
     */
    virtual void _followPath();

    /**
     * @brief 检查是否需要重新计算路径
     */
    [[nodiscard]] bool _shouldRecomputePath() const;

    /**
     * @brief 检查是否到达当前路径点
     */
    [[nodiscard]] bool _isAtCurrentWaypoint() const;

    /**
     * @brief 前进到下一个路径点
     */
    void _advanceToNextWaypoint();

    /**
     * @brief 计算到目标的最短距离
     */
    [[nodiscard]] f32 _getDistanceToTarget() const;

    /**
     * @brief 获取当前目标路径点
     */
    [[nodiscard]] const PathPoint* _getCurrentWaypoint() const;

    /**
     * @brief 计算路径点对应的地面 Y 坐标
     *
     * 对应 vanilla PathNavigation.getGroundY：路径点所在格下方是空气时直接沿用路径点 Y，
     * 否则取下方方块的碰撞形状顶面（例如在营火、半砖等非整格方块上，路径点 Y 是方块
     * 格坐标，实体应停在方块实际顶面而非格子底部）。Cubium 走 MoveControl::setMoveTo，
     * Y 直接参与移动目标位置计算，因此需要与 vanilla 相同的顶面修正。
     *
     * @param waypoint 当前路径点
     * @return 移动目标 Y 坐标
     */
    [[nodiscard]] f64 _getGroundY(const PathPoint& waypoint) const;

    /**
     * @brief 在给定寻路区域上计算路径点地面 Y 坐标
     *
     * @param region 寻路区域
     * @param waypoint 当前路径点
     * @return 移动目标 Y 坐标
     */
    [[nodiscard]] f64 _getGroundY(const Region& region, const PathPoint& waypoint) const;

    /**
     * @brief 检查是否卡住并处理
     */
    void _checkForStuck();

    /**
     * @brief 修剪路径（处理锅等特殊情况 + 阳光避让截断）
     */
    void _trimPath();

    /**
     * @brief 结束寻路会话，清空寻路器持有的世界区域指针
     *
     * 对应 vanilla PathFinder.findPath 返回前调用 nodeEvaluator.done()：NodeProcessor
     * 持有的 Region* 是 PathNavigator 栈上的临时对象，会话结束后必须清空，否则后续
     * MovementController 经 NodeProcessor 查询节点类型时会解引用已析构的 Region。
     */
    void _endPathfindingSession();

public:
    /**
     * @brief 计算路径点地面 Y 坐标（测试/诊断入口）
     *
     * 供单元测试验证 getGroundY 的顶面修正；生产代码经 _followPath 调用。
     *
     * @param waypoint 路径点
     * @return 移动目标 Y 坐标
     */
    [[nodiscard]] f64 groundYForTest(const PathPoint& waypoint) const { return _getGroundY(waypoint); }

    /**
     * @brief 使用指定世界计算路径点地面高度（单元测试入口）
     *
     * 与生产路径共用 _getGroundY 的世界查询逻辑，仅替换世界来源，便于在不构造完整实体时
     * 验证整格/半砖/空气三种顶面修正。
     *
     * @param world 用于查询方块的世界
     * @param waypoint 路径点
     * @return 移动目标 Y 坐标
     */
    [[nodiscard]] f64 groundYForTest(const IWorld& world, const PathPoint& waypoint) const;

protected:
    /**
     * @brief 重置超时计时器
     */
    void _resetTimeout();
};

} // namespace entity::ai::pathfinding
} // namespace mc
