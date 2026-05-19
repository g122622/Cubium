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

#include "../../Goal.hpp"
#include "../target/TargetGoals.hpp"
#include "core/EnumSet.hpp"
#include "core/Types.hpp"
#include <memory>

namespace mc {

// Forward declarations
class IronGolemEntity;
class LivingEntity;
class MobEntity;
class Player;

namespace entity {
class VillagerEntity;
}

namespace entity::ai::goal {

/**
 * @brief 铁傀儡近战攻击目标
 *
 * 铁傀儡特有的攻击行为：
 * - 攻击时举起手臂
 * - 造成击退效果
 * - 攻击后重置冷却
 * - 不攻击玩家创建的铁傀儡的主人
 * - 不攻击苦力怕
 *
 * 参考 MC 1.16.5: net.minecraft.entity.passive.IronGolemEntity 使用 MeleeAttackGoal
 */
class IronGolemAttackGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param golem 拥有此目标的铁傀儡
     * @param speed 移动速度倍率
     */
    IronGolemAttackGoal(IronGolemEntity* golem, f64 speed);

    ~IronGolemAttackGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "IronGolemAttackGoal"; }

protected:
    /**
     * @brief 检查是否可以攻击目标
     * @param target 目标实体
     * @return 是否可以攻击
     */
    [[nodiscard]] bool canAttack(LivingEntity* target) const;

    /**
     * @brief 检查并执行攻击
     * @param target 目标实体
     * @param distanceSquared 到目标的距离平方
     */
    void checkAndPerformAttack(LivingEntity* target, f64 distanceSquared);

    /**
     * @brief 执行攻击
     * @param target 目标实体
     */
    void attackTarget(LivingEntity* target);

    /**
     * @brief 获取攻击范围平方
     * @param target 目标实体
     * @return 攻击范围平方
     */
    [[nodiscard]] f32 getAttackReachSqr(LivingEntity* target) const;

private:
    IronGolemEntity* m_golem;
    f64 m_speed;
    LivingEntity* m_attackTarget = nullptr;
    i32 m_attackCooldown = 0;
    i32 m_pathRecalculateTimer = 0;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    u32 m_lastCheckTime = 0;

    // 攻击冷却（tick）- 铁傀儡攻击间隔 1 秒
    static constexpr i32 ATTACK_COOLDOWN_TICKS = 20;
    // 停止追踪距离
    static constexpr f32 STOP_ATTACK_DISTANCE_SQ = 1024.0f; // 32 * 32
    // 路径重新计算间隔基础值
    static constexpr i32 PATH_RECALC_BASE_MIN = 4;
    static constexpr i32 PATH_RECALC_BASE_MAX = 10;
};

/**
 * @brief 铁傀儡给村民展示花朵目标
 *
 * 铁傀儡偶尔会看向附近的村民并展示手中的罂粟花。
 * 只在白天执行，概率为 1/8000。
 *
 * 参考 MC 1.16.5 ShowVillagerFlowerGoal
 */
class ShowVillagerFlowerGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param ironGolem 铁傀儡实体
     */
    explicit ShowVillagerFlowerGoal(IronGolemEntity* ironGolem);

    ~ShowVillagerFlowerGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;
    [[nodiscard]] std::string getTypeName() const override { return "ShowVillagerFlowerGoal"; }

private:
    IronGolemEntity* m_ironGolem;
    entity::VillagerEntity* m_villager = nullptr;
    i32 m_lookTime = 0;

    // MC 1.16.5 常量
    static constexpr f32 SEARCH_RANGE = 6.0f;  // 搜索村民范围
    static constexpr f32 SEARCH_HEIGHT = 2.0f; // 搜索村民高度
    static constexpr i32 LOOK_DURATION = 400;  // 看向持续时间（ticks = 20秒）
    static constexpr i32 CHANCE = 8000;        // 执行概率倒数（1/8000）
};

/**
 * @brief 铁傀儡保护村庄目标选择器
 *
 * 当村民被攻击时，铁傀儡会攻击攻击者。
 *
 * 参考 MC 1.16.5: net.minecraft.entity.ai.goal.DefendVillageTargetGoal
 * 铁傀儡会记住攻击村民的实体并追击
 */
class DefendVillageTargetGoal : public TargetGoal {
public:
    /**
     * @brief 构造函数
     * @param golem 拥有此目标的铁傀儡
     */
    DefendVillageTargetGoal(IronGolemEntity* golem);

    ~DefendVillageTargetGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    void resetTask() override;

    [[nodiscard]] std::string getTypeName() const override { return "DefendVillageTargetGoal"; }

private:
    IronGolemEntity* m_golem;
    LivingEntity* m_villageAggressor = nullptr;
};

/**
 * @brief 铁傀儡攻击最近敌对目标选择器
 *
 * 搜索附近的敌对生物（IMob）并攻击，但不攻击苦力怕。
 *
 * 参考 MC 1.16.5: NearestAttackableTargetGoal<MobEntity>
 * 对铁傀儡的特殊限制：
 * - 不攻击苦力怕
 * - 玩家创建的铁傀儡不攻击玩家
 */
class IronGolemNearestAttackableTargetGoal : public TargetGoal {
public:
    /**
     * @brief 构造函数
     * @param golem 拥有此目标的铁傀儡
     * @param chance 检查概率倒数（0=每tick都检查）
     */
    IronGolemNearestAttackableTargetGoal(IronGolemEntity* golem, i32 chance = 5);

    ~IronGolemNearestAttackableTargetGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;

    [[nodiscard]] std::string getTypeName() const override { return "IronGolemNearestAttackableTargetGoal"; }

private:
    IronGolemEntity* m_golem;
    i32 m_chance;
    LivingEntity* m_targetEntity = nullptr;

    // 搜索范围
    static constexpr f32 SEARCH_RANGE = 16.0f;
};

} // namespace entity::ai::goal
} // namespace mc
