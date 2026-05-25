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

#include "../../../interfaces/IRangedAttackMob.hpp"
#include "../MonsterEntity.hpp"

// Forward declarations
namespace mc::entity::ai::goal {
class RangedBowAttackGoal;
class MeleeAttackGoal;
} // namespace mc::entity::ai::goal

namespace mc {

/**
 * @brief 骷髅系怪物公共中间层
 *
 * 对齐 MC 1.16.5 `AbstractSkeletonEntity`，集中承载：
 * - 远程弓箭攻击接口
 * - 拉弓状态与攻击计时
 * - 骷髅系共通属性与基础目标注册
 * - 动态战斗目标切换（setCombatTask 模式）
 *
 * 子类：
 * - SkeletonEntity: 普通骷髅，使用弓远程攻击
 * - StrayEntity: 流浪者，使用弓远程攻击，不在阳光下燃烧
 * - WitherSkeletonEntity: 凋灵骷髅，使用石剑近战攻击，施加凋零效果
 *
 * MC 1.16.5 关键设计：
 * - registerGoals() 只注册非战斗目标（移动、看向、目标选择）
 * - setCombatTask() 根据装备动态选择战斗目标（远程/近战）
 * - 子类通过装备不同武器来影响 setCombatTask() 的选择
 */
class AbstractSkeletonEntity : public MonsterEntity, public entity::IRangedAttackMob {
public:
    ~AbstractSkeletonEntity() override;

    AbstractSkeletonEntity(const AbstractSkeletonEntity&) = delete;
    AbstractSkeletonEntity& operator=(const AbstractSkeletonEntity&) = delete;
    AbstractSkeletonEntity(AbstractSkeletonEntity&&) = delete;
    AbstractSkeletonEntity& operator=(AbstractSkeletonEntity&&) = delete;

    // ========== 常量 ==========
    /// 这些常量在测试中需要访问

    static constexpr i32 ATTACK_COOLDOWN = 60;      // 攻击冷却（ticks），3秒
    static constexpr f32 ARROW_DAMAGE = 2.0f;       // 箭矢基础伤害
    static constexpr f64 RANGED_ATTACK_SPEED = 1.0; // 远程攻击移动速度
    static constexpr f64 MELEE_ATTACK_SPEED = 1.2;  // 近战攻击移动速度
    static constexpr i32 ATTACK_INTERVAL_MIN = 20;  // 最小攻击间隔（ticks）
    static constexpr i32 ATTACK_INTERVAL_MAX = 40;  // 最大攻击间隔（ticks）
    static constexpr f32 ATTACK_RADIUS = 15.0f;     // 远程攻击半径

    /// 战斗目标优先级（MC 1.16.5: priority=4）
    static constexpr i32 COMBAT_GOAL_PRIORITY = 4;

    // ========== IRangedAttackMob 接口实现 ==========

    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;

    // ========== 弓箭状态管理 ==========

    [[nodiscard]] bool isChargingBow() const { return m_chargingBow; }
    void setChargingBow(bool charging) { m_chargingBow = charging; }

    [[nodiscard]] i32 getAttackTimer() const { return m_attackTimer; }
    void setAttackTimer(i32 timer) { m_attackTimer = timer; }

    [[nodiscard]] i32 getAttackCooldown() const { return m_attackCooldown; }
    void setAttackCooldown(i32 cooldown) { m_attackCooldown = cooldown; }

    // ========== 战斗目标管理 ==========

    /**
     * @brief 设置战斗目标
     *
     * MC 1.16.5: 根据装备动态选择战斗目标
     * - 如果持有弓，使用 RangedBowAttackGoal
     * - 否则使用 MeleeAttackGoal
     *
     * 此方法会先移除所有战斗目标，再根据装备添加正确的目标。
     * 子类可以通过装备不同武器来影响战斗目标选择。
     */
    virtual void setCombatTask();

    // ========== 生命周期 ==========

    void tick() override;

protected:
    AbstractSkeletonEntity(EntityId id);

    void registerGoals() override;
    void registerAttributes() override;

    // ========== 弓箭状态 ==========

    bool m_chargingBow = false;
    i32 m_attackTimer = 0;
    i32 m_attackCooldown = 0;

    // ========== 战斗目标 ==========
    /// 注意：这些目标指针在 GoalSelector 中被复制，所以需要通过原始指针操作

    /// 远程攻击目标（MC 1.16.5: aiArrowAttack）
    std::unique_ptr<entity::ai::goal::RangedBowAttackGoal> m_rangedAttackGoal;

    /// 近战攻击目标（MC 1.16.5: aiAttackOnCollide）
    std::unique_ptr<entity::ai::goal::MeleeAttackGoal> m_meleeAttackGoal;
};

} // namespace mc
