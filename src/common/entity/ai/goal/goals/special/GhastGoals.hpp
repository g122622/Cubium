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
 * @file GhastGoals.hpp
 * @brief 恶魂专用的AI目标类
 *
 * 恶魂有三个特有AI目标：
 * - GhastRandomFlyGoal: 随机飞行目标，在下界中随机漫游
 * - GhastLookAroundGoal: 环顾四周目标，根据移动方向或攻击目标调整朝向
 * - GhastFireballAttackGoal: 火球攻击目标，向攻击目标发射火球
 */

#pragma once

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include <string>

namespace mc {

class GhastEntity;
class LivingEntity;

namespace entity::ai::goal {

/**
 * @brief 恶魂随机飞行目标
 *
 * 实现恶魂的随机飞行行为：
 * 1. 当移动控制器空闲或目标太远/太近时选择新的随机目标
 * 2. 在当前位置周围选择随机飞行点（±16格范围）
 */
class GhastRandomFlyGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param ghast 恶魂实体指针
     */
    explicit GhastRandomFlyGoal(GhastEntity* ghast);

    /**
     * @brief 检查是否应该执行
     * @return 移动控制器空闲或目标距离不合适时返回true
     */
    bool shouldExecute() override;

    /**
     * @brief 是否应该继续执行
     * @return 单次执行目标，返回false
     */
    bool shouldContinueExecuting() override { return false; }

    /**
     * @brief 开始执行时调用
     * 选择随机目标位置并移动
     */
    void startExecuting() override;

    /**
     * @brief 获取目标名称
     */
    std::string getTypeName() const override { return "GhastRandomFlyGoal"; }

private:
    GhastEntity* m_ghast;

    static constexpr f64 WANDER_RANGE = 16.0;      ///< 随机漫游范围（格）
    static constexpr f64 MIN_DISTANCE_SQ = 1.0;    ///< 最小目标距离平方
    static constexpr f64 MAX_DISTANCE_SQ = 3600.0; ///< 最大目标距离平方（60^2）
};

/**
 * @brief 恶魂环顾四周目标
 *
 * 实现恶魂的朝向控制行为：
 * 1. 无攻击目标时：朝向移动方向
 * 2. 有攻击目标时：朝向攻击目标
 */
class GhastLookAroundGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param ghast 恶魂实体指针
     */
    explicit GhastLookAroundGoal(GhastEntity* ghast);

    /**
     * @brief 检查是否应该执行
     * @return 始终返回true（总是执行）
     */
    bool shouldExecute() override { return true; }

    /**
     * @brief 每tick执行
     * 根据是否有攻击目标更新朝向
     */
    void tick() override;

    /**
     * @brief 获取目标名称
     */
    std::string getTypeName() const override { return "GhastLookAroundGoal"; }

private:
    GhastEntity* m_ghast;

    static constexpr f64 LOOK_RANGE_SQ = 4096.0; ///< 看向范围平方（64^2）
};

/**
 * @brief 恶魂火球攻击目标
 *
 * 实现恶魂的火球攻击行为：
 * 1. 当有攻击目标且在64格内且能看到目标时开始充能
 * 2. 充能20 ticks后发射火球
 * 3. 发射后进入冷却（40 ticks）
 * 4. 攻击时设置攻击状态用于客户端动画
 */
class GhastFireballAttackGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param ghast 恶魂实体指针
     */
    explicit GhastFireballAttackGoal(GhastEntity* ghast);

    /**
     * @brief 检查是否应该执行
     * @return 有攻击目标时返回true
     */
    bool shouldExecute() override;

    /**
     * @brief 开始执行时调用
     * 重置攻击计时器
     */
    void startExecuting() override;

    /**
     * @brief 重置时调用
     * 清除攻击状态
     */
    void resetTask() override;

    /**
     * @brief 每tick执行
     * 处理充能和火球发射逻辑
     */
    void tick() override;

    /**
     * @brief 获取目标名称
     */
    std::string getTypeName() const override { return "GhastFireballAttackGoal"; }

private:
    GhastEntity* m_ghast;
    LivingEntity* m_target = nullptr;
    i32 m_attackTimer = 0;

    static constexpr f64 ATTACK_RANGE_SQ = 4096.0; ///< 攻击范围平方（64^2）
    static constexpr i32 CHARGE_SOUND_TICK = 10;   ///< 充能音效tick
    static constexpr i32 CHARGE_DURATION = 20;     ///< 充能持续时间
    static constexpr i32 COOLDOWN_DURATION = 40;   ///< 攻击冷却（ticks）
};

} // namespace entity::ai::goal
} // namespace mc
