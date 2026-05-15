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

#include "../../../../../core/Types.hpp"
#include "../../Goal.hpp"

namespace mc {

// Forward declarations
class LivingEntity;
class MobEntity;

namespace entity::ai::goal {

/**
 * @brief 远程攻击目标
 *
 * 使实体能够在一定距离外进行远程攻击。
 * 适用于骷髅、烈焰人等使用远程攻击的怪物。
 *
 * 参考 MC 1.16.5 RangedAttackGoal
 */
class RangedAttackGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     * @param speed 移动速度倍率
     * @param attackIntervalMin 最小攻击间隔（ticks）
     * @param attackIntervalMax 最大攻击间隔（ticks）
     * @param attackRadius 攻击半径
     */
    RangedAttackGoal(MobEntity* mob, f64 speed, i32 attackIntervalMin, i32 attackIntervalMax, f32 attackRadius);

    ~RangedAttackGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "RangedAttackGoal"; }

protected:
    /**
     * @brief 执行远程攻击
     * @param target 目标实体
     * @param charge 蓄力程度（0.0-1.0）
     */
    virtual void performAttack(LivingEntity* target, f32 charge);

    MobEntity* m_mob;
    LivingEntity* m_target = nullptr;
    f64 m_speed;
    i32 m_attackIntervalMin;
    i32 m_attackIntervalMax;
    f32 m_attackRadius;
    f32 m_maxAttackDistanceSq; // MC 1.16.5: 缓存的平方距离
    i32 m_attackTime = -1;     // MC 1.16.5: 初始值为 -1
    i32 m_seenTime = 0;        // MC 1.16.5: 能看到目标的时间

    static constexpr i32 MIN_SEEN_TIME = 20; // MC 1.16.5: 停止移动前需要看到的tick数 (原版是20)
};

/**
 * @brief 弓箭攻击目标
 *
 * 使用弓进行远程攻击。
 * 需要实体实现IRangedAttackMob接口。
 *
 * 参考 MC 1.16.5 RangedBowAttackGoal
 */
class RangedBowAttackGoal : public RangedAttackGoal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     * @param speed 移动速度倍率
     * @param attackIntervalMin 最小攻击间隔
     * @param attackIntervalMax 最大攻击间隔
     */
    RangedBowAttackGoal(MobEntity* mob, f64 speed, i32 attackIntervalMin, i32 attackIntervalMax);

    ~RangedBowAttackGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "RangedBowAttackGoal"; }

protected:
    void performAttack(LivingEntity* target, f32 charge) override;

private:
    // MC 1.16.5: 走位相关字段
    bool m_strafingClockwise = false; // 是否顺时针走位
    bool m_strafingBackwards = false; // 是否向后走位
    i32 m_strafingTime = -1;          // 走位时间计数器

    static constexpr i32 STRAFE_THRESHOLD = 20; // 走位方向变化阈值（MC 1.16.5）
};

} // namespace entity::ai::goal
} // namespace mc
