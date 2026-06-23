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
#include "common/item/core/ItemStack.hpp"
#include <memory>

// Forward declarations
namespace mc::entity::ai::goal {
class RangedBowAttackGoal;
class MeleeAttackGoal;
} // namespace mc::entity::ai::goal

namespace mc {

// Forward declarations
class Item;

/**
 * @brief 骷髅系怪物公共中间层
 *
 * 集中承载：
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
 * 关键设计：
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

    /// 战斗目标优先级
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
     * @brief 重新评估战斗目标
     *
     * 根据装备动态选择战斗目标：
     * - 如果持有弓，使用 RangedBowAttackGoal
     * - 否则使用 MeleeAttackGoal
     *
     * 此方法会先移除所有战斗目标，再根据装备添加正确的目标。
     * 在以下时机被调用：
     * - 构造函数末尾
     * - finalizeSpawn() 中
     * - 装备变更时（setEquipment 触发）
     * - NBT 加载后（addAdditionalSaveData / readAdditionalSaveData）
     */
    virtual void setCombatTask();

    /**
     * @brief 检查实体是否可以使用非近战武器
     *
     * 当实体手持指定物品时，返回 true 表示该物品被视为远程武器。
     * 默认实现检查物品是否为弓。
     * 凋灵骷髅重写此方法返回 false，因为它不使用远程攻击。
     *
     * 对应 MC 原版 AbstractSkeleton.canUseNonMeleeWeapon()。
     *
     * @param stack 要检查的物品堆
     * @return 如果该物品是远程武器则返回 true
     */
    [[nodiscard]] virtual bool canUseNonMeleeWeapon(const ItemStack& stack) const;

    // ========== 生命周期 ==========

    void tick() override;

    /**
     * @brief 完成骷髅的生成初始化
     *
     * 重写 MobEntity::finalizeSpawn() 以实现骷髅特有的初始化：
     * - 填充默认装备和附魔
     * - 重新评估战斗目标（远程/近战）
     * - 设置拾取物品能力
     * - 万圣节南瓜头（10月31日，25% 概率）
     *
     * @param world 世界引用
     * @param difficulty 区域难度实例
     * @param spawnReason 生成原因
     */
    void finalizeSpawn(mc::IWorld& world,
        const mc::entity::combat::DifficultyInstance& difficulty,
        mc::world::spawn::SpawnReason spawnReason) override;

protected:
    AbstractSkeletonEntity(EntityId id);

    void registerGoals() override;
    void registerAttributes() override;

    /**
     * @brief 装备变更回调
     *
     * 当装备槽位发生变化时，重新评估战斗目标（远程/近战切换）。
     * 对应 MC 原版 AbstractSkeleton.onEquipItem() 中的 reassessWeaponGoal() 调用。
     */
    void setEquipment(EquipmentSlot slot, const ItemStack& stack) override;

    // ========== 弓箭状态 ==========

    bool m_chargingBow = false;
    i32 m_attackTimer = 0;
    i32 m_attackCooldown = 0;

    // ========== 战斗目标 ==========
    /// 注意：这些目标指针在 GoalSelector 中被复制，所以需要通过原始指针操作

    /// 远程攻击目标
    std::unique_ptr<entity::ai::goal::RangedBowAttackGoal> m_rangedAttackGoal;

    /// 近战攻击目标
    std::unique_ptr<entity::ai::goal::MeleeAttackGoal> m_meleeAttackGoal;
};

} // namespace mc
