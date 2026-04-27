#pragma once

#include "Path.hpp"
#include "PathFinder.hpp"
#include "../goal/GoalConstants.hpp"
#include "../../../core/Types.hpp"
#include <memory>

namespace mc {

// 前向声明
class Entity;
class LivingEntity;
class MobEntity;

namespace entity::ai::pathfinding {

/**
 * @brief 路径导航器基类
 *
 * 负责计算路径、沿路径移动实体、处理路径中断等。
 * 不同移动类型（地面、飞行、游泳）有不同的实现。
 *
 * 参考 MC 1.16.5 PathNavigator
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
    [[nodiscard]] bool hasPath() const { return m_path && !m_path->empty(); }

    /**
     * @brief 检查是否没有路径（用于AI目标判断）
     */
    [[nodiscard]] bool noPath() const { return !hasPath(); }

    /**
     * @brief 获取当前路径
     */
    [[nodiscard]] const Path* getPath() const { return m_path.get(); }

    /**
     * @brief 获取当前路径点索引
     */
    [[nodiscard]] i32 getCurrentIndex() const;

    /**
     * @brief 检查路径是否完成
     */
    [[nodiscard]] bool isDone() const { return !m_path || m_path->isFinished(); }

    /**
     * @brief 检查是否正在跟随路径
     */
    [[nodiscard]] bool isInProgress() const { return hasPath() && !isDone(); }

    // ========== 路径控制 ==========

    /**
     * @brief 清除当前路径
     */
    void clearPath() { m_path.reset(); }

    /**
     * @brief 停止导航
     */
    void stop() {
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
    void setMaxDistance(i32 distance) { m_maxDistance = distance; }

    /**
     * @brief 设置重试间隔
     */
    void setRetryInterval(i32 interval) { m_retryInterval = interval; }

    /**
     * @brief 设置是否可以游泳
     */
    void setCanSwim(bool canSwim) { m_canSwim = canSwim; }

    /**
     * @brief 设置是否可以开门
     */
    void setCanOpenDoors(bool canOpenDoors) { m_canOpenDoors = canOpenDoors; }

    /**
     * @brief 设置是否可以通过门
     */
    void setCanEnterDoors(bool canEnterDoors) { m_canEnterDoors = canEnterDoors; }

    /**
     * @brief 设置关联的实体
     */
    void setEntity(LivingEntity* entity) { m_entity = entity; }

    /**
     * @brief 设置移动速度
     * MC 1.16.5: 设置导航器当前使用的速度
     */
    void setSpeed(f64 speed) { m_speed = speed; }

    // ========== 调试 ==========

    /**
     * @brief 获取寻路器
     */
    [[nodiscard]] PathFinder* getPathFinder() { return m_pathFinder.get(); }

    /**
     * @brief 获取当前速度
     */
    [[nodiscard]] f64 getSpeed() const { return m_speed; }

protected:
    std::unique_ptr<PathFinder> m_pathFinder;
    std::unique_ptr<Path> m_path;
    LivingEntity* m_entity = nullptr;

    f64 m_speed = 1.0;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    i32 m_maxDistance = 100;
    i32 m_retryInterval = 20;
    i32 m_retryTimer = 0;
    i32 m_ticksSinceLastPath = 0;

    // MC 1.16.5: 卡住检测相关字段
    f64 m_lastPosX = 0.0;
    f64 m_lastPosY = 0.0;
    f64 m_lastPosZ = 0.0;
    i32 m_stuckTimer = 0;
    // 使用 GoalConstants.hpp 中的常量: PATH_STUCK_THRESHOLD, PATH_STUCK_DISTANCE_THRESHOLD

    bool m_canSwim = false;
    bool m_canOpenDoors = false;
    bool m_canEnterDoors = false;

    // ========== 内部方法 ==========

    /**
     * @brief 沿路径移动实体
     */
    virtual void followPath();

    /**
     * @brief 检查是否需要重新计算路径
     */
    [[nodiscard]] bool shouldRecomputePath() const;

    /**
     * @brief 检查是否到达当前路径点
     */
    [[nodiscard]] bool isAtCurrentWaypoint() const;

    /**
     * @brief 前进到下一个路径点
     */
    void advanceToNextWaypoint();

    /**
     * @brief 计算到目标的最短距离
     */
    [[nodiscard]] f32 getDistanceToTarget() const;

    /**
     * @brief 获取当前目标路径点
     */
    [[nodiscard]] const PathPoint* getCurrentWaypoint() const;
};

} // namespace entity::ai::pathfinding
} // namespace mc
