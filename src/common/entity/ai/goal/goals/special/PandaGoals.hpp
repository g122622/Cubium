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
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/goal/goals/AvoidEntityGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include <string>

namespace mc {

// Forward declarations
class PandaEntity;
class LivingEntity;

namespace entity::ai::goal {

/**
 * @brief 熊猫打滚目标
 *
 * 顽皮性格的熊猫或幼年熊猫会随机打滚。
 *
 * 行为流程：
 * 1. 检查是否可以执行打滚（幼年或顽皮、在地面、无其他状态）
 * 2. 概率触发或前方是悬崖时触发
 * 3. 设置打滚状态，由 PandaEntity::updateRoll() 处理物理
 *
 * 优先级: 12
 * 互斥标志: MOVE, LOOK, JUMP
 * 不可被抢占
 * 触发概率：
 *   - 前方是悬崖：100%
 *   - 顽皮性格：1/60
 *   - 幼年熊猫：1/500
 */
class PandaRollGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param panda 熊猫实体
     */
    explicit PandaRollGoal(PandaEntity* panda);

    ~PandaRollGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;

    [[nodiscard]] bool isPreemptible() const override
    {
        return false;
    }

    void startExecuting() override;

    [[nodiscard]] std::string getTypeName() const override
    {
        return "PandaRollGoal";
    }

private:
    /**
     * @brief 检查前方是否有悬崖
     * @return 如果前方一格下方是空气返回true
     */
    [[nodiscard]] bool _isCliffInFront() const;

    PandaEntity* m_panda;

    // 常量
    static constexpr i32 PLAYFUL_ROLL_CHANCE = 60; // 顽皮熊猫触发概率 1/60
    static constexpr i32 NORMAL_ROLL_CHANCE = 500; // 普通触发概率 1/500
};

/**
 * @brief 熊猫打喷嚏目标
 *
 * 幼年熊猫随机打喷嚏。对齐 vanilla 1.21.11 Panda.PandaSneezeGoal（Panda.java:1103-1130）。
 *
 * 行为流程（vanilla canUse）：
 *   if (isBaby() && canPerformAction()) {
 *       return isWeak() && random.nextInt(reducedTickDelay(500)) == 1
 *            ? true
 *            : random.nextInt(reducedTickDelay(6000)) == 1;
 *   }
 *   return false;
 *
 * 即幼年熊猫且无其他动作时：虚弱性格 1/500 概率（nextInt==1），否则 1/6000 概率。
 * start 调 panda.sneeze(true)，由 PandaEntity::tick 递减 m_sneezeTimer 驱动预喷嚏音效
 * 与 _onSneezeComplete（掉粘液球+弱化周围实体+成年熊猫跳跃）。
 *
 * 优先级: 8（对齐 vanilla Panda.registerGoals:270）
 * 互斥标志: 无（vanilla PandaSneezeGoal 未设 mutexFlags，canContinueToUse=false 一次性）
 */
class PandaSneezeGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param panda 熊猫实体
     */
    explicit PandaSneezeGoal(PandaEntity* panda);

    ~PandaSneezeGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;

    [[nodiscard]] std::string getTypeName() const override
    {
        return "PandaSneezeGoal";
    }

private:
    PandaEntity* m_panda;

    // 常量（对齐 vanilla reducedTickDelay(500)/reducedTickDelay(6000)，Goal::reducedTickDelay 减半补偿
    // GoalSelector 半 tick 评估）。vanilla 用 nextInt(N)==1（非 ==0），此处原样对齐。
    static constexpr i32 WEAK_SNEEZE_CHANCE = 500;    // 虚弱幼年熊猫触发概率
    static constexpr i32 NORMAL_SNEEZE_CHANCE = 6000; // 普通幼年熊猫触发概率
};

/**
 * @brief 熊猫近战攻击目标
 *
 * 好斗性格熊猫被攻击后反击攻击者。对齐 vanilla 1.21.11 Panda.PandaAttackGoal
 * （Panda.java:759-771）。
 *
 * 行为流程（vanilla canUse）：
 *   return this.panda.canPerformAction() && super.canUse();
 *
 * 即仅在熊猫不在打喷嚏/吃东西/躺/打滚等动作状态时，才委托 MeleeAttackGoal 的目标选取
 * （攻击目标由 PandaHurtByTargetGoal 写入 attackTarget）。MeleeAttackGoal 内部完成
 * 寻路靠近与 checkAndPerformAttack（攻击力由 ATTACK_DAMAGE 属性决定，好斗熊猫=6.0）。
 *
 * 优先级: 3（对齐 vanilla Panda.registerGoals:264）
 * 互斥标志: Move, Look（继承自 MeleeAttackGoal）
 */
class PandaAttackGoal : public MeleeAttackGoal {
public:
    /**
     * @brief 构造函数
     * @param panda 熊猫实体
     * @param speed 移动速度倍率（vanilla 1.2）
     * @param useLongMemory 是否使用长期记忆追踪（vanilla true）
     */
    PandaAttackGoal(PandaEntity* panda, f64 speed, bool useLongMemory);

    ~PandaAttackGoal() override = default;

    /**
     * @brief 是否应该开始执行
     *
     * 对齐 vanilla PandaAttackGoal.canUse：canPerformAction() 门控后委托基类。
     */
    [[nodiscard]] bool shouldExecute() override;

    [[nodiscard]] std::string getTypeName() const noexcept override
    {
        return "PandaAttackGoal";
    }

private:
    PandaEntity* m_panda;
};

/**
 * @brief 懒惰熊猫仰躺目标（对齐 vanilla Panda.PandaLieOnBackGoal）
 *
 * 行为：
 * - canUse：冷却结束 + 懒惰性格 + canPerformAction + 1/400 概率
 * - canContinueToUse：不在水中，且（仍是懒惰 或 未触发 1/600 概率）时，
 *   再以 1/2000 概率结束（简化自 vanilla 的复合条件）
 * - start/stop 设置/清除 Entity::isOnBack 标志，stop 后 200 tick 冷却
 */
class PandaLieOnBackGoal : public Goal {
public:
    explicit PandaLieOnBackGoal(PandaEntity* panda);

    ~PandaLieOnBackGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;

    [[nodiscard]] bool shouldContinueExecuting() override;

    void startExecuting() override;

    void resetTask() override;

    [[nodiscard]] std::string getTypeName() const noexcept override
    {
        return "PandaLieOnBackGoal";
    }

private:
    PandaEntity* m_panda;
    i32 m_cooldown = 0;
};

/**
 * @brief 熊猫坐下进食目标（对齐 vanilla Panda.PandaSitGoal）
 *
 * 行为：
 * - canUse：冷却结束 + 非幼体 + 不在水中 + canPerformAction + 非不满状态，并且
 *   主手有食物或附近有可吃物品时成立
 * - tick：坐下后若主手仍有食物则继续尝试坐下
 * - start：主手有食物立即坐下；否则走向最近的可吃物品
 * - stop：把手里的食物丢回地上、清空主手、按性格设置 10~60 / 10~160 秒冷却，并站起
 *
 * 互斥标志：MOVE
 */
/**
 * @brief 熊猫看向玩家目标（对齐 vanilla Panda.PandaLookAtPlayerGoal）
 *
 * 与基类 LookAtGoal 的差异：
 * - canUse：沿用 2% 概率与"自己找最近的玩家"（不是攻击目标、不是任意生物），
 *   但额外要求 canPerformAction()（打喷嚏/进食/仰躺/打滚时不看人）；
 *   已通过 setLookTarget 预置目标时不再重新搜索。
 * - canContinueToUse：除基类的"存活 + 距离 + 剩余时间"外，还要求引用未失效。
 * - tick：引用失效时不继续——避免目标被移除后解引用空指针。
 *
 * 互斥标志：LOOK（继承自基类）
 * 优先级：9
 */
class PandaLookAtPlayerGoal : public LookAtGoal {
public:
    /**
     * @brief 构造熊猫看向玩家的目标
     * @param panda 熊猫实例
     * @param lookDistance 最大看向距离（vanilla 传 6.0）
     */
    explicit PandaLookAtPlayerGoal(PandaEntity* panda, f32 lookDistance = 6.0f);

    ~PandaLookAtPlayerGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void tick() override;

    /**
     * @brief 预先锁定看向目标（对齐 vanilla PandaLookAtPlayerGoal.setTarget）
     *
     * 由 PandaBreedGoal 在附近找不到竹子时调用：把最近的玩家写进来，
     * 使熊猫在原地"不满地"盯着玩家看。写入引用可自动跟随实体销毁失效。
     */
    void setLookTarget(LivingEntity* target);

    [[nodiscard]] std::string getTypeName() const override
    {
        return "PandaLookAtPlayerGoal";
    }

private:
    PandaEntity* m_panda;
};

class PandaSitGoal : public Goal {
public:
    explicit PandaSitGoal(PandaEntity* panda);

    ~PandaSitGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;

    [[nodiscard]] bool shouldContinueExecuting() override;

    void startExecuting() override;

    void resetTask() override;

    void tick() override;

    [[nodiscard]] std::string getTypeName() const noexcept override
    {
        return "PandaSitGoal";
    }

private:
    /// 附近 6/8 格内是否有熊猫能吃的物品（对齐 vanilla canPickUpAndEat 过滤）
    [[nodiscard]] bool _hasNearbyEdibleItem(f32 inflate) const;

    PandaEntity* m_panda;
    i32 m_cooldown = 0;
};

/**
 * @brief 忧愁熊猫逃避目标（对齐 vanilla Panda.PandaAvoidGoal）
 *
 * 仅在忧愁性格且 canPerformAction 时委托 AvoidEntityGoal 的逃避逻辑；
 * 玩家与敌对生物的逃避距离/速度由构造参数给出。
 */
class PandaAvoidGoal : public AvoidEntityGoal {
public:
    PandaAvoidGoal(
        PandaEntity* panda, f32 avoidDistance, f64 farSpeed, f64 nearSpeed, AvoidEntityGoal::EntityPredicate predicate);

    ~PandaAvoidGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;

    [[nodiscard]] std::string getTypeName() const noexcept override
    {
        return "PandaAvoidGoal";
    }

private:
    PandaEntity* m_panda;
};

} // namespace entity::ai::goal
} // namespace mc
