#pragma once

#include "../../Goal.hpp"
#include "../../../../../util/math/Vector3.hpp"

namespace mc {

// Forward declarations
class AbstractHorseEntity;
class Player;

namespace entity::ai::goal {

/**
 * @brief 苦力怕膨胀目标
 *
 * 当玩家靠近时膨胀并最终爆炸。
 *
 * 参考 MC 1.16.5 CreeperSwellGoal
 */
class CreeperSwellGoal : public Goal {
public:
    explicit CreeperSwellGoal(void* creeper) : Goal(EnumSet<GoalFlag>{GoalFlag::Move}) {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
    void tick() override {}
};

/**
 * @brief 末影人传送目标
 *
 * 当受到攻击或看向玩家时传送。
 *
 * 参考 MC 1.16.5 EndermanTeleportGoal
 */
class EndermanTeleportGoal : public Goal {
public:
    explicit EndermanTeleportGoal(void* enderman) : Goal(EnumSet<GoalFlag>{GoalFlag::Move}) {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void tick() override {}
};

/**
 * @brief 羊驼跟随商队目标
 *
 * 羊驼会跟随领头的羊驼形成商队。
 *
 * 参考 MC 1.16.5 LlamaFollowCaravanGoal
 */
class LlamaFollowCaravanGoal : public Goal {
public:
    LlamaFollowCaravanGoal(void* llama, f32 speed) : Goal(EnumSet<GoalFlag>{GoalFlag::Move}) {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void resetTask() override {}
    void tick() override {}
};

/**
 * @brief 疯狂奔跑目标
 *
 * 未驯服的马被骑乘时会四处乱跑，增加驯服难度。
 * 每次改变方向时有概率增加驯服进度。
 *
 * 参考 MC 1.16.5 RunAroundLikeCrazyGoal
 */
class RunAroundLikeCrazyGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param horse 马类实体
     * @param speed 移动速度
     */
    RunAroundLikeCrazyGoal(AbstractHorseEntity* horse, f64 speed);

    ~RunAroundLikeCrazyGoal() override = default;

    bool shouldExecute() override;
    bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

private:
    /**
     * @brief 计算疯狂跑动目标位置
     * @return 是否找到有效位置
     */
    bool findTarget();

    AbstractHorseEntity* m_horse;
    f64 m_speed;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;

    static constexpr i32 MIN_RUN_TICKS = 20;   // 最少运行时间
    static constexpr i32 MAX_RUN_TICKS = 100;  // 最多运行时间
};

/**
 * @brief 海豚跳跃目标
 *
 * 海豚跳出水面跳跃。
 *
 * 参考 MC 1.16.5 DolphinJumpGoal
 */
class DolphinJumpGoal : public Goal {
public:
    explicit DolphinJumpGoal(void* dolphin) : Goal(EnumSet<GoalFlag>{GoalFlag::Jump}) {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void tick() override {}
};

} // namespace entity::ai::goal
} // namespace mc
