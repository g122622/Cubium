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

class EatGrassGoal : public Goal {
public:
    explicit EatGrassGoal(CreatureEntity* creature)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look})
    {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
    void tick() override {}
};

class FlyGoal : public Goal {
public:
    FlyGoal(CreatureEntity* creature, f64 speed)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    {}
    bool shouldExecute() override { return false; }
    void startExecuting() override {}
    void tick() override {}
};

class SleepGoal : public Goal {
public:
    explicit SleepGoal(CreatureEntity* creature)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look, GoalFlag::Jump})
    {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
};

class WorkAtPoiGoal : public Goal {
public:
    explicit WorkAtPoiGoal(CreatureEntity* creature)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
    void tick() override {}
};

class FindShelterGoal : public Goal {
public:
    FindShelterGoal(CreatureEntity* creature, f64 speed)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
    void tick() override {}
};

class FleeSunGoal : public Goal {
public:
    FleeSunGoal(CreatureEntity* creature, f64 speed)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
};

class ReturnToHomeGoal : public Goal {
public:
    ReturnToHomeGoal(CreatureEntity* creature, f64 speed, f32 homeRadius = 16.0f)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void tick() override {}
};

class TradeWithPlayerGoal : public Goal {
public:
    explicit TradeWithPlayerGoal(CreatureEntity* creature)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Look, GoalFlag::Move})
    {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
    void tick() override {}
};

class ShowWaresGoal : public Goal {
public:
    explicit ShowWaresGoal(CreatureEntity* creature)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Look})
    {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
    void tick() override {}
};

class HurtByTargetGoal : public Goal {
public:
    explicit HurtByTargetGoal(MobEntity* mob)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Target})
    {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
};

class NearestAttackableTargetGoal : public Goal {
public:
    NearestAttackableTargetGoal(
        MobEntity* mob, const std::string& targetClass, bool checkSight = true, bool nearbyOnly = false)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Target})
    {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
};

} // namespace entity::ai::goal
} // namespace mc
