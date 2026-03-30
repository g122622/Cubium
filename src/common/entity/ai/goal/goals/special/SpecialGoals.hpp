#pragma once

#include "../Goal.hpp"
#include "../../../../core/Types.hpp"

namespace mc {

// Forward declarations
class CreeperEntity;

/**
 * @brief 苦力怕膨胀目标
 *
 * 控制苦力怕在接近目标时膨胀并爆炸。
 *
 * 参考 MC 1.16.5 CreeperSwellGoal
 */
class CreeperSwellGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creeper 苦力怕实体
     */
    explicit CreeperSwellGoal(CreeperEntity* creeper);

    /**
     * @brief 是否应该执行
     */
    bool shouldExecute() override;

    /**
     * @brief 是否应该继续执行
     */
    bool shouldContinueExecuting() override;

    /**
     * @brief 开始执行
     */
    void startExecuting() override;

    /**
     * @brief 重置任务
     */
    void resetTask() override;

    /**
     * @brief 每帧更新
     */
    void tick() override;

private:
    CreeperEntity* m_creeper;
    LivingEntity* m_attackTarget = nullptr;

    // 常量
    static constexpr f32 EXPLODE_DISTANCE = 3.0f;  // 爆炸距离
};

/**
 * @brief 末影人瞬移目标
 *
 * 控制末影人在受伤或遇到水时瞬移。
 *
 * 参考 MC 1.16.5 EndermanTeleportGoal
 */
class EndermanTeleportGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param enderman 末影人实体
     */
    explicit EndermanTeleportGoal(EndermanEntity* enderman);

    /**
     * @brief 是否应该执行
     */
    bool shouldExecute() override;

    /**
     * @brief 是否应该继续执行
     */
    bool shouldContinueExecuting() override;

    /**
     * @brief 开始执行
     */
    void startExecuting() override;

    /**
     * @brief 每帧更新
     */
    void tick() override;

private:
    EndermanEntity* m_enderman;
    i32 m_teleportCooldown = 0;

    // 常量
    static constexpr i32 TELEPORT_COOLDOWN = 50;
};

/**
 * @brief 羊驼跟随商队目标
 *
 * 控制羊驼跟随商队领袖。
 *
 * 参考 MC 1.16.5 LlamaFollowCaravanGoal
 */
class LlamaFollowCaravanGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param llama 羊驼实体
     * @param speed 移动速度
     */
    LlamaFollowCaravanGoal(LlamaEntity* llama, f32 speed);

    /**
     * @brief 是否应该执行
     */
    bool shouldExecute() override;

    /**
     * @brief 是否应该继续执行
     */
    bool shouldContinueExecuting() override;

    /**
     * @brief 开始执行
     */
    void startExecuting() override;

    /**
     * @brief 重置任务
     */
    void resetTask() override;

    /**
     * @brief 每帧更新
     */
    void tick() override;

private:
    LlamaEntity* m_llama;
    f32 m_speed;
    LlamaEntity* m_caravanLeader = nullptr;
    i32 m_distanceCheckCooldown = 0;

    // 常量
    static constexpr f32 FOLLOW_DISTANCE = 10.0f;
    static constexpr f32 MIN_DISTANCE = 2.0f;
};

/**
 * @brief 疯狂奔跑目标
 *
 * 控制未驯服的马匹在被骑乘时疯狂奔跑。
 *
 * 参考 MC 1.16.5 RunAroundLikeCrazyGoal
 */
class RunAroundLikeCrazyGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param horse 马实体
     * @param speed 移动速度
     */
    RunAroundLikeCrazyGoal(AbstractHorseEntity* horse, f32 speed);

    /**
     * @brief 是否应该执行
     */
    bool shouldExecute() override;

    /**
     * @brief 是否应该继续执行
     */
    bool shouldContinueExecuting() override;

    /**
     * @brief 开始执行
     */
    void startExecuting() override;

    /**
     * @brief 重置任务
     */
    void resetTask() override;

    /**
     * @brief 每帧更新
     */
    void tick() override;

private:
    AbstractHorseEntity* m_horse;
    f32 m_speed;
    Player* m_rider = nullptr;
    i32 m_runTime = 0;

    // 常量
    static constexpr i32 MAX_RUN_TIME = 200;  // 最大奔跑时间
    static constexpr f32 TAME_CHANCE = 0.01f; // 每tick驯服概率
};

/**
 * @brief 海豚跳跃目标
 *
 * 控制海豚跳出水面。
 *
 * 参考 MC 1.16.5 DolphinJumpGoal
 */
class DolphinJumpGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param dolphin 海豚实体
     */
    explicit DolphinJumpGoal(DolphinEntity* dolphin);

    /**
     * @brief 是否应该执行
     */
    bool shouldExecute() override;

    /**
     * @brief 是否应该继续执行
     */
    bool shouldContinueExecuting() override;

    /**
     * @brief 开始执行
     */
    void startExecuting() override;

    /**
     * @brief 重置任务
     */
    void resetTask() override;

    /**
     * @brief 每帧更新
     */
    void tick() override;

private:
    DolphinEntity* m_dolphin;
    i32 m_jumpCooldown = 0;
    bool m_isJumping = false;

    // 常量
    static constexpr i32 MIN_COOLDOWN = 100;
    static constexpr i32 MAX_COOLDOWN = 400;
    static constexpr f32 JUMP_CHANCE = 0.02f;
};

} // namespace mc
