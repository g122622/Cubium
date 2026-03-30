#pragma once

#include "../../Goal.hpp"
#include "../../../../../core/Types.hpp"

namespace mc::entity::ai::goal {

// Forward declarations only

class CreeperSwellGoal : public Goal {
public:
    explicit CreeperSwellGoal(void* creeper) : Goal(EnumSet<GoalFlag>{GoalFlag::Move}) {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
    void tick() override {}
};

class EndermanTeleportGoal : public Goal {
public:
    explicit EndermanTeleportGoal(void* enderman) : Goal(EnumSet<GoalFlag>{GoalFlag::Move}) {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void tick() override {}
};

class LlamaFollowCaravanGoal : public Goal {
public:
    LlamaFollowCaravanGoal(void* llama, f32 speed) : Goal(EnumSet<GoalFlag>{GoalFlag::Move}) {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
    void tick() override {}
};

class RunAroundLikeCrazyGoal : public Goal {
public:
    RunAroundLikeCrazyGoal(void* horse, f32 speed) : Goal(EnumSet<GoalFlag>{GoalFlag::Move}) {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
    void tick() override {}
};

class DolphinJumpGoal : public Goal {
public:
    explicit DolphinJumpGoal(void* dolphin) : Goal(EnumSet<GoalFlag>{GoalFlag::Jump}) {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
    void tick() override {}
};

} // namespace mc::entity::ai::goal
