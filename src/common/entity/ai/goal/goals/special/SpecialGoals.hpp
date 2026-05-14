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

#include "../../../../../util/math/Vector3.hpp"
#include "../../Goal.hpp"

namespace mc {

// Forward declarations
class AbstractHorseEntity;
class CreeperEntity;
class Player;
class LivingEntity;
class PufferfishEntity;

namespace entity::ai::goal {

/**
 * @brief 苦力怕膨胀目标
 *
 * 当玩家靠近时膨胀并最终爆炸。
 *
 * MC 1.16.5 参考: net.minecraft.entity.ai.goal.CreeperSwellGoal
 *
 * 执行条件:
 * - 苦力怕已经有膨胀状态 (getCreeperState() > 0)，或者
 * - 攻击目标在 9 格距离内 (3x3 范围)
 *
 * tick 行为:
 * - 如果攻击目标为空：取消膨胀
 * - 如果攻击目标距离 > 49 格 (7x7 范围)：取消膨胀
 * - 如果无法看到攻击目标：取消膨胀
 * - 否则：设置膨胀状态为 1
 */
class CreeperSwellGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param creeper 苦力怕实体
     */
    explicit CreeperSwellGoal(CreeperEntity* creeper);

    ~CreeperSwellGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "CreeperSwellGoal"; }

private:
    CreeperEntity* m_creeper;
    LivingEntity* m_attackTarget = nullptr;

    // MC 1.16.5 常量
    static constexpr f32 SWELL_TRIGGER_DISTANCE_SQ = 9.0f; // 3.0 * 3.0
    static constexpr f32 SWELL_CANCEL_DISTANCE_SQ = 49.0f; // 7.0 * 7.0
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
    explicit EndermanTeleportGoal(void* enderman)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    {}
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
    LlamaFollowCaravanGoal(void* llama, f32 speed)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Move})
    {}
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

    static constexpr i32 MIN_RUN_TICKS = 20;  // 最少运行时间
    static constexpr i32 MAX_RUN_TICKS = 100; // 最多运行时间
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
    explicit DolphinJumpGoal(void* dolphin)
        : Goal(EnumSet<GoalFlag>{GoalFlag::Jump})
    {}
    bool shouldExecute() override { return false; }
    bool shouldContinueExecuting() override { return false; }
    void startExecuting() override {}
    void tick() override {}
};

/**
 * @brief 河豚膨胀目标
 *
 * 当检测到敌对生物或玩家靠近时触发膨胀行为。
 *
 * MC 1.16.5 参考: net.minecraft.entity.passive.fish.PufferfishEntity.PuffGoal
 *
 * 检测规则：
 * - 检测碰撞箱向外扩展 2 格范围内的 LivingEntity
 * - 玩家：非旁观者模式且非创造模式视为威胁
 * - 其他生物：非水生生物（CreatureAttribute != Water）视为威胁
 *
 * 行为：
 * - shouldExecute(): 检测范围内是否有威胁实体
 * - startExecuting(): 开始膨胀计时器 (puffTimer = 1)
 * - resetTask(): 重置膨胀计时器 (puffTimer = 0)
 */
class PuffGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param fish 河豚实体
     */
    explicit PuffGoal(::mc::PufferfishEntity* fish);

    ~PuffGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;

    [[nodiscard]] std::string getTypeName() const override { return "PuffGoal"; }

private:
    /**
     * @brief 判断实体是否为河豚的威胁
     *
     * MC 1.16.5 ENEMY_MATCHER 谓词：
     * - 玩家：非旁观者模式且非创造模式
     * - 其他生物：非水生生物
     *
     * @param entity 生物实体
     * @return 如果是威胁返回 true
     */
    [[nodiscard]] static bool isEnemy(const LivingEntity* entity);

    /**
     * @brief 查找附近的威胁实体
     * @return 如果找到威胁实体返回 true
     */
    [[nodiscard]] bool findNearbyEnemy();

    ::mc::PufferfishEntity* m_fish;
    LivingEntity* m_nearbyEnemy = nullptr;

    // MC 1.16.5 常量
    static constexpr f32 DETECTION_RANGE = 2.0f; // 检测范围（碰撞箱向外扩展）
};

} // namespace entity::ai::goal
} // namespace mc
