/**
 * @file BatGoals.hpp
 * @brief 蝙蝠专用的AI目标类
 *
 * 参考 MC 1.16.5: net.minecraft.entity.passive.BatEntity
 *
 * 蝙蝠有两个特有AI目标：
 * - BatRandomFlyGoal: 随机飞行目标，选择随机目标点并平滑转向飞行
 * - BatRestGoal: 挂墙休息目标，在白天或合适位置倒挂休息
 *
 * 注意：MC原版蝙蝠实际上不使用传统AI目标系统，而是在 updateAITasks() 中直接实现行为。
 * 但为了遵循项目的架构风格，这里将其拆分为独立的Goal类。
 */

#pragma once

#include "../../Goal.hpp"
#include "../../GoalFlag.hpp"
#include "../../../core/Types.hpp"
#include "../../../world/World.hpp"
#include "../../../util/math/Vector3.hpp"
#include "../../../util/math/random/Random.hpp"

namespace mc {
class BatEntity;
class IWorld;

namespace entity::ai::goal {

/**
 * @brief 蝙蝠随机飞行目标
 *
 * 实现 MC 1.16.5 蝙蝠的飞行行为：
 * 1. 选择随机目标点（当前位置±7格X/Z，-2到+4格Y）
 * 2. 目标点不可用或到达（距离<2）或1/30概率时更换目标点
 * 3. 平滑转向朝目标点飞行
 *
 * 参考: net.minecraft.entity.passive.BatEntity 第142-159行
 */
class BatRandomFlyGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param bat 蝙蝠实体指针
     */
    explicit BatRandomFlyGoal(BatEntity* bat);

    /**
     * @brief 检查是否应该执行
     * @return 蝙蝠不在休息状态时返回true
     */
    bool shouldExecute() override;

    /**
     * @brief 检查是否应该继续执行
     * @return 蝙蝠不在休息状态时返回true
     */
    bool shouldContinueExecuting() override;

    /**
     * @brief 开始执行时调用
     * 选择初始目标点
     */
    void startExecuting() override;

    /**
     * @brief 重置时调用
     * 清除目标点
     */
    void resetTask() override;

    /**
     * @brief 每tick执行
     * 更新飞行方向和速度
     */
    void tick() override;

    /**
     * @brief 获取目标名称
     */
    std::string getTypeName() const override { return "BatRandomFlyGoal"; }

private:
    /**
     * @brief 选择新的随机目标点
     *
     * 目标点范围：
     * - X: 当前位置 ±7 格
     * - Y: 当前位置 -2 到 +4 格
     * - Z: 当前位置 ±7 格
     */
    void selectNewTarget();

    /**
     * @brief 检查目标点是否有效
     * @param pos 目标位置
     * @return 目标点是空气且在有效高度范围内返回true
     */
    bool isTargetValid(const BlockPos& pos) const;

    BatEntity* m_bat;          ///< 蝙蝠实体
    BlockPos m_targetPos;       ///< 目标位置
    bool m_hasTarget = false;   ///< 是否有有效目标
    i32 m_cooldown = 0;         ///< 冷却计时器
};

/**
 * @brief 蝙蝠挂墙休息目标
 *
 * 实现 MC 1.16.5 蝙蝠的休息行为：
 * 1. 白天时尝试挂墙休息（1/100概率/tick）
 * 2. 检查上方是否有固体方块可以倒挂
 * 3. 挂墙时偶尔转头
 * 4. 玩家靠近（4格内）或失去支撑时飞走
 *
 * 参考: net.minecraft.entity.passive.BatEntity 第125-163行
 */
class BatRestGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param bat 蝙蝠实体指针
     */
    explicit BatRestGoal(BatEntity* bat);

    /**
     * @brief 检查是否应该执行
     * @return 白天且可以休息（上方有固体方块）时返回true
     */
    bool shouldExecute() override;

    /**
     * @brief 检查是否应该继续执行
     * @return 应该继续休息时返回true
     */
    bool shouldContinueExecuting() override;

    /**
     * @brief 目标是否可以被抢占
     * @return 可以被抢占（玩家靠近时会飞走）
     */
    bool isPreemptible() const override { return true; }

    /**
     * @brief 开始执行时调用
     * 进入休息状态
     */
    void startExecuting() override;

    /**
     * @brief 重置时调用
     * 退出休息状态
     */
    void resetTask() override;

    /**
     * @brief 每tick执行
     * 处理转头和状态检查
     */
    void tick() override;

    /**
     * @brief 获取目标名称
     */
    std::string getTypeName() const override { return "BatRestGoal"; }

private:
    /**
     * @brief 检查是否应该停止休息
     * @return 玩家靠近或失去支撑时返回true
     */
    bool shouldStopResting() const;

    /**
     * @brief 检查上方是否有固体方块可以倒挂
     * @return 可以休息返回true
     */
    bool canRestAtCurrentPosition() const;

    BatEntity* m_bat;           ///< 蝙蝠实体
    i32 m_turnTimer = 0;        ///< 转头计时器
    f32 m_targetYaw = 0.0f;     ///< 目标转头角度
};

} // namespace entity::ai::goal
} // namespace mc
