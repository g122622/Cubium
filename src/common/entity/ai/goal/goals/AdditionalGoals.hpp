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

#include "../../../../core/Types.hpp"
#include "../Goal.hpp"
#include <functional>

namespace mc {

class CreatureEntity;
class LivingEntity;
class Player;
class MobEntity;

namespace entity::ai::goal {

/**
 * @brief 吃草目标
 *
 * 控制生物低头吃草的行为。用于羊等动物吃草和恢复生命值。
 */
class EatGrassGoal : public Goal {
public:
    explicit EatGrassGoal(CreatureEntity* creature)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look})
    {}

    // TODO: 暂时的简化实现，待完善
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
    void tick() override {}
};

/**
 * @brief 飞行目标
 *
 * 控制生物在空中飞行的行为。用于鹦鹉、蜜蜂等飞行生物。
 */
class FlyGoal : public Goal {
public:
    FlyGoal(CreatureEntity* creature, f64 speed)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    {}

    // TODO: 暂时的简化实现，待完善
    bool shouldExecute() override { return false; }
    void startExecuting() override {}
    void tick() override {}
};

/**
 * @brief 睡眠目标
 *
 * 控制生物在夜间睡眠的行为。用于村民等生物。
 */
class SleepGoal : public Goal {
public:
    explicit SleepGoal(CreatureEntity* creature)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look, GoalFlag::Jump})
    {}

    // TODO: 暂时的简化实现，待完善
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
};

/**
 * @brief 在POI工作目标
 *
 * 控制村民在工作站点（POI）工作的行为。
 */
class WorkAtPoiGoal : public Goal {
public:
    explicit WorkAtPoiGoal(CreatureEntity* creature)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    {}

    // TODO: 暂时的简化实现，待完善
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
    void tick() override {}
};

/**
 * @brief 寻找庇护所目标
 *
 * 控制生物在危险情况下寻找安全庇护所的行为。
 */
class FindShelterGoal : public Goal {
public:
    FindShelterGoal(CreatureEntity* creature, f64 speed)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    {}

    // TODO: 暂时的简化实现，待完善
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void tick() override {}
};

/**
 * @brief 躲避阳光目标
 *
 * 控制生物躲避阳光的行为。用于僵尸等会在阳光下燃烧的生物。
 */
class FleeSunGoal : public Goal {
public:
    FleeSunGoal(CreatureEntity* creature, f64 speed)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    {}

    // TODO: 暂时的简化实现，待完善
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
};

/**
 * @brief 返回家园目标
 *
 * 控制生物返回其家园位置的行为。用于村民返回床位等场景。
 */
class ReturnToHomeGoal : public Goal {
public:
    ReturnToHomeGoal(CreatureEntity* creature, f64 speed, f32 homeRadius = 16.0f)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    {}

    // TODO: 暂时的简化实现，待完善
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void tick() override {}
};

/**
 * @brief 与玩家交易目标
 *
 * 控制村民与玩家进行交易的行为。
 */
class TradeWithPlayerGoal : public Goal {
public:
    explicit TradeWithPlayerGoal(CreatureEntity* creature)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Look, GoalFlag::Move})
    {}

    // TODO: 暂时的简化实现，待完善
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
    void tick() override {}
};

/**
 * @brief 展示商品目标
 *
 * 控制村民向玩家展示商品的行为。
 */
class ShowWaresGoal : public Goal {
public:
    explicit ShowWaresGoal(CreatureEntity* creature)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Look})
    {}

    // TODO: 暂时的简化实现，待完善
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
    void tick() override {}
};

/**
 * @brief 被攻击后反击目标
 *
 * 当生物受到攻击时，记住攻击者并进行反击的行为目标。
 */
class HurtByTargetGoal : public Goal {
public:
    explicit HurtByTargetGoal(MobEntity* mob)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Target})
    {}

    // TODO: 暂时的简化实现，待完善
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
};

/**
 * @brief 最近可攻击目标目标
 *
 * 寻找并锁定最近的可攻击目标。用于怪物寻找攻击目标。
 */
class NearestAttackableTargetGoal : public Goal {
public:
    NearestAttackableTargetGoal(
        MobEntity* mob, const std::string& targetClass, bool checkSight = true, bool nearbyOnly = false)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Target})
    {}

    // TODO: 暂时的简化实现，待完善
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
};

} // namespace entity::ai::goal
} // namespace mc
