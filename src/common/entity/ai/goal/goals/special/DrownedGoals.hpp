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
#include "../../../../../util/math/Vector3.hpp"
#include "../../../../../world/block/BlockPos.hpp"
#include "../../Goal.hpp"
#include "../../GoalFlag.hpp"
#include "../MeleeAttackGoal.hpp"
#include "../attack/RangedAttackGoals.hpp"
#include "../special/MoveToBlockGoal.hpp"
#include <string>

namespace mc {

// 前向声明
class CreatureEntity;
class DrownedEntity;
class MobEntity;

namespace entity::ai::goal {

// ============================================================================
// DrownedGoToWaterGoal
// ============================================================================

/**
 * @brief 溺尸前往水源目标
 *
 * 白天溺尸在陆地时，寻找附近的水源并移动过去。
 * 仅在室外明亮（白天）且不在水中时激活。
 */
class DrownedGoToWaterGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param drowned 溺尸实体
     * @param speed 移动速度倍率
     */
    DrownedGoToWaterGoal(DrownedEntity* drowned, f64 speed);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "DrownedGoToWaterGoal"; }

private:
    /**
     * @brief 搜索附近的水源位置
     * @return 是否找到水源
     */
    [[nodiscard]] bool _findWater();

    DrownedEntity* m_drowned;
    f64 m_speed;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;
    bool m_foundWater = false;

    static constexpr i32 SEARCH_RANGE_HORIZONTAL = 10; // 水平搜索范围
    static constexpr i32 SEARCH_RANGE_VERTICAL = 8;    // 垂直搜索范围（溺尸可深可浅）
};

// ============================================================================
// DrownedTridentAttackGoal
// ============================================================================

/**
 * @brief 溺尸三叉戟远程攻击目标
 *
 * 当溺尸手持三叉戟时进行远程攻击。
 * 继承自 RangedAttackGoal，增加了三叉戟持有检查和使用动画。
 */
class DrownedTridentAttackGoal : public RangedAttackGoal {
public:
    /**
     * @brief 构造函数
     * @param drowned 溺尸实体
     * @param speed 移动速度倍率
     * @param attackIntervalMin 最小攻击间隔（ticks）
     * @param attackRadius 攻击半径
     */
    DrownedTridentAttackGoal(DrownedEntity* drowned, f64 speed, i32 attackIntervalMin, f32 attackRadius);

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    void resetTask() override;

    [[nodiscard]] std::string getTypeName() const override { return "DrownedTridentAttackGoal"; }

private:
    DrownedEntity* m_drowned;
};

// ============================================================================
// DrownedAttackGoal
// ============================================================================

/**
 * @brief 溺尸近战攻击目标
 *
 * 溺尸专用的近战攻击目标，在标准近战攻击基础上增加了 okTarget 过滤。
 * 只有在非白天或目标在水中时才执行攻击。
 */
class DrownedAttackGoal : public MeleeAttackGoal {
public:
    /**
     * @brief 构造函数
     * @param drowned 溺尸实体
     * @param speed 移动速度倍率
     * @param useLongMemory 是否使用长期记忆
     */
    DrownedAttackGoal(DrownedEntity* drowned, f64 speed, bool useLongMemory);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;

    [[nodiscard]] std::string getTypeName() const noexcept override { return "DrownedAttackGoal"; }

private:
    DrownedEntity* m_drowned;
};

// ============================================================================
// DrownedGoToBeachGoal
// ============================================================================

/**
 * @brief 溺尸前往海滩目标
 *
 * 夜晚时，溺尸在水中靠近海平面时会前往附近的沙滩/陆地。
 * 继承自 MoveToBlockGoal，目标是找到可以站立的地面。
 */
class DrownedGoToBeachGoal : public MoveToBlockGoal {
public:
    /**
     * @brief 构造函数
     * @param drowned 溺尸实体
     * @param speed 移动速度倍率
     */
    DrownedGoToBeachGoal(DrownedEntity* drowned, f64 speed);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;

    [[nodiscard]] std::string getTypeName() const override { return "DrownedGoToBeachGoal"; }

protected:
    [[nodiscard]] bool shouldMoveTo(IWorld* world, const BlockPos& pos) override;

private:
    DrownedEntity* m_drowned;
};

// ============================================================================
// DrownedSwimUpGoal
// ============================================================================

/**
 * @brief 溺尸向上游泳目标
 *
 * 夜晚时，溺尸在深水中会向海平面方向游泳。
 * 设置 searchingForLand 标志，配合 DrownedMoveControl 实现向上游泳。
 */
class DrownedSwimUpGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param drowned 溺尸实体
     * @param speed 游泳速度倍率
     * @param seaLevel 海平面高度
     */
    DrownedSwimUpGoal(DrownedEntity* drowned, f64 speed, i32 seaLevel);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "DrownedSwimUpGoal"; }

private:
    DrownedEntity* m_drowned;
    f64 m_speed;
    i32 m_seaLevel;
    bool m_stuck = false;
};

} // namespace entity::ai::goal
} // namespace mc
