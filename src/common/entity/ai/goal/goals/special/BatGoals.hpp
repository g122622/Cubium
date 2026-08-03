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

#include "../../../../../core/Constants.hpp"
#include "../../../../../world/block/BlockPos.hpp"
#include "../../Goal.hpp"
#include "../../GoalFlag.hpp"
#include "common/core/Types.hpp"
#include <string>

namespace mc {
class BatEntity;
class IWorld;

namespace entity::ai::goal {

/**
 * @brief 蝙蝠随机飞行目标
 *
 * 实现蝙蝠的飞行行为：
 * 1. 选择随机目标点（当前位置±7格X/Z，-2到+4格Y）
 * 2. 目标点不可用或到达（距离<2）或1/30概率时更换目标点
 * 3. 平滑转向朝目标点飞行
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
    void _selectNewTarget();

    /**
     * @brief 检查目标点是否有效
     * @param pos 目标位置
     * @return 目标点是空气且在有效高度范围内返回true
     */
    bool _isTargetValid(const BlockPos& pos) const;

    BatEntity* m_bat;         ///< 蝙蝠实体
    BlockPos m_targetPos;     ///< 目标位置
    bool m_hasTarget = false; ///< 是否有有效目标
    i32 m_cooldown = 0;       ///< 冷却计时器
};

/**
 * @brief 蝙蝠挂墙休息目标
 *
 * 实现蝙蝠的休息行为：
 * 1. 白天时尝试挂墙休息（1/100概率/tick）
 * 2. 检查上方是否有固体方块可以倒挂
 * 3. 挂墙时偶尔转头
 * 4. 玩家靠近（4格内）或失去支撑时飞走
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
    bool _shouldStopResting() const;

    /**
     * @brief 检查上方是否有固体方块可以倒挂
     * @return 可以休息返回true
     */
    bool _canRestAtCurrentPosition() const;

    BatEntity* m_bat;       ///< 蝙蝠实体
    i32 m_turnTimer = 0;    ///< 转头计时器
    f32 m_targetYaw = 0.0f; ///< 目标转头角度
};

} // namespace entity::ai::goal
} // namespace mc
