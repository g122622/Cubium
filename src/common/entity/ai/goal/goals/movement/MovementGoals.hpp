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

#include "common/core/Types.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include <string>

namespace mc {

// Forward declarations
class CreatureEntity;
class LivingEntity;
class MobEntity;

namespace entity::ai::goal {

/**
 * @brief 避开水随机行走目标
 *
 * 与RandomWalkingGoal类似，但会避开水域。
 * 适用于大多数陆地生物。
 */
class WaterAvoidingRandomWalkingGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 拥有此目标的生物
     * @param speed 移动速度倍率
     */
    WaterAvoidingRandomWalkingGoal(CreatureEntity* creature, f64 speed);

    /**
     * @brief 构造函数（带概率）
     * @param creature 拥有此目标的生物
     * @param speed 移动速度倍率
     * @param chance 每tick执行的概率（0.0-1.0）
     */
    WaterAvoidingRandomWalkingGoal(CreatureEntity* creature, f64 speed, f32 chance);

    ~WaterAvoidingRandomWalkingGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

protected:
    /**
     * @brief 获取随机目标位置
     * @return 是否找到有效位置
     */
    [[nodiscard]] bool getRandomPosition();

    /**
     * @brief 检查位置是否在水或岩浆中
     * @param x X坐标
     * @param y Y坐标
     * @param z Z坐标
     * @return 如果在水中或岩浆中返回true
     */
    [[nodiscard]] bool isInWaterOrLava(f64 x, f64 y, f64 z) const;

    CreatureEntity* m_creature;
    f64 m_speed;
    f32 m_chance;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    i32 m_timeout = 0;
    bool m_isRunning = false;

    static constexpr i32 MAX_TIMEOUT = 600; // 最大行走时间（30秒）
};

/**
 * @brief 跳跃攻击目标
 *
 * 使实体跳跃向目标攻击。
 * 适用于蜘蛛等会跳跃攻击的生物。
 */
class LeapAtTargetGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param mob 拥有此目标的生物
     * @param leapHeight 跳跃高度
     */
    LeapAtTargetGoal(MobEntity* mob, f32 leapHeight);

    ~LeapAtTargetGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;

private:
    MobEntity* m_mob;
    LivingEntity* m_target = nullptr;
    f32 m_leapHeight;
    bool m_leaped = false;

    static constexpr f32 MIN_DISTANCE = 4.0f; // 最小跳跃距离
    static constexpr f32 MAX_DISTANCE = 8.0f; // 最大跳跃距离
};

/**
 * @brief 向目标移动目标
 *
 * 向攻击目标移动，但不执行攻击。
 * 与MeleeAttackGoal不同，此目标只负责移动。
 */
class MoveTowardsTargetGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 拥有此目标的生物
     * @param speed 移动速度倍率
     * @param maxTargetDistance 最大目标距离
     */
    MoveTowardsTargetGoal(CreatureEntity* creature, f64 speed, f32 maxTargetDistance);

    ~MoveTowardsTargetGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    [[nodiscard]] std::string getTypeName() const override { return "MoveTowardsTargetGoal"; }

private:
    CreatureEntity* m_creature;
    LivingEntity* m_targetEntity = nullptr;
    f64 m_speed;
    f32 m_maxTargetDistance;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
};

/**
 * @brief 向家范围移动目标
 *
 * 当生物离开其家范围时，向家位置移动。
 * 守卫者使用此目标来限制它们在海底神殿附近的移动。
 * MC原版对应: MoveTowardsRestrictionGoal
 */
class MoveTowardsRestrictionGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creature 拥有此目标的生物
     * @param speed 移动速度倍率
     */
    MoveTowardsRestrictionGoal(CreatureEntity* creature, f64 speed);

    ~MoveTowardsRestrictionGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;
    [[nodiscard]] std::string getTypeName() const override { return "MoveTowardsRestrictionGoal"; }

private:
    /**
     * @brief 重新计算朝向家位置的路径
     */
    void _recalculatePath();

    CreatureEntity* m_creature;
    f64 m_speed;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    i32 m_pathRecalcTimer = 0;

    static constexpr i32 XZ_RANGE = 16;             ///< 水平搜索范围
    static constexpr i32 Y_RANGE = 7;               ///< 垂直搜索范围
    static constexpr i32 PATH_RECALC_INTERVAL = 10; ///< 路径重算间隔（ticks）
};

} // namespace entity::ai::goal
} // namespace mc
