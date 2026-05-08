#pragma once

#include "../Goal.hpp"
#include "../../../../core/Types.hpp"
#include <functional>

namespace mc {

class CreatureEntity;
class LivingEntity;
class Player;
class MobEntity;

namespace entity::ai::goal {

class EatGrassGoal : public Goal {
public:
    explicit EatGrassGoal(CreatureEntity* creature) : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look}) {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
    void tick() override {}
};

class FlyGoal : public Goal {
public:
    FlyGoal(CreatureEntity* creature, f64 speed) : Goal(EnumSet<GoalFlag>{GoalFlag::Move}) {}
    bool shouldExecute() override { return false; }
    void startExecuting() override {}
    void tick() override {}
};

class SleepGoal : public Goal {
public:
    explicit SleepGoal(CreatureEntity* creature) : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look, GoalFlag::Jump}) {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
};

class WorkAtPoiGoal : public Goal {
public:
    explicit WorkAtPoiGoal(CreatureEntity* creature) : Goal(EnumSet<GoalFlag>{GoalFlag::Move}) {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
    void tick() override {}
};

class FindShelterGoal : public Goal {
public:
    FindShelterGoal(CreatureEntity* creature, f64 speed) : Goal(EnumSet<GoalFlag>{GoalFlag::Move}) {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
    void tick() override {}
};

class FleeSunGoal : public Goal {
public:
    FleeSunGoal(CreatureEntity* creature, f64 speed) : Goal(EnumSet<GoalFlag>{GoalFlag::Move}) {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
};

class ReturnToHomeGoal : public Goal {
public:
    ReturnToHomeGoal(CreatureEntity* creature, f64 speed, f32 homeRadius = 16.0f)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Move}) {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void tick() override {}
};

class TradeWithPlayerGoal : public Goal {
public:
    explicit TradeWithPlayerGoal(CreatureEntity* creature) : Goal(EnumSet<GoalFlag>{GoalFlag::Look, GoalFlag::Move}) {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
    void tick() override {}
};

class ShowWaresGoal : public Goal {
public:
    explicit ShowWaresGoal(CreatureEntity* creature) : Goal(EnumSet<GoalFlag>{GoalFlag::Look}) {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
    void tick() override {}
};

class HurtByTargetGoal : public Goal {
public:
    explicit HurtByTargetGoal(MobEntity* mob) : Goal(EnumSet<GoalFlag>{GoalFlag::Target}) {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
};

class NearestAttackableTargetGoal : public Goal {
public:
    NearestAttackableTargetGoal(MobEntity* mob, const std::string& targetClass, bool checkSight = true, bool nearbyOnly = false)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Target}) {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
};

} // namespace entity::ai::goal
} // namespace mc
