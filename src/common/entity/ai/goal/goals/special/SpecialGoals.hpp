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
#include "common/core/Types.hpp"
#include <string>

namespace mc {

// Forward declarations
class AbstractHorseEntity;
class CreeperEntity;
class Player;
class LivingEntity;
class PufferfishEntity;
class LlamaEntity;
class WolfEntity;
class TraderLlamaEntity;
class SkeletonHorseEntity;

namespace entity::ai::goal {

/**
 * @brief 苦力怕膨胀目标
 *
 * 当玩家靠近时膨胀并最终爆炸。
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

    static constexpr f32 SWELL_TRIGGER_DISTANCE_SQ = 9.0f; // 3.0 * 3.0
    static constexpr f32 SWELL_CANCEL_DISTANCE_SQ = 49.0f; // 7.0 * 7.0
};

/**
 * @brief 羊驼跟随商队目标
 *
 * 羊驼会跟随领头的羊驼形成商队。
 *
 * 执行条件:
 * - 羊驼未被拴绳且未在商队中
 * - 附近有可加入的商队（被拴绳拴住的羊驼或已有商队链）
 *
 * 商队规则:
 * - 商队最多 8 只羊驼
 * - 跟随距离保持 2 格
 * - 商队头领必须被拴绳拴住
 */
class LlamaFollowCaravanGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param llama 羊驼实体
     * @param speed 移动速度
     */
    LlamaFollowCaravanGoal(LlamaEntity* llama, f32 speed);

    ~LlamaFollowCaravanGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "LlamaFollowCaravanGoal"; }

    // 常量（公开用于测试）
    static constexpr f64 SEARCH_RADIUS = 9.0;            // 搜索半径
    static constexpr f64 SEARCH_HEIGHT = 4.0;            // 搜索高度
    static constexpr f64 MIN_JOIN_DISTANCE_SQ = 4.0;     // 最小加入距离平方 (2格)
    static constexpr f64 MAX_FOLLOW_DISTANCE_SQ = 676.0; // 最大跟随距离平方 (26格)
    static constexpr f64 CARAVAN_FOLLOW_DISTANCE = 2.0;  // 跟随间距
    static constexpr i32 MAX_CARAVAN_LENGTH = 8;         // 商队最大长度

private:
    /**
     * @brief 递归检查商队头领是否被拴绳拴住
     * @param llama 当前羊驼
     * @param depth 递归深度
     * @return 如果商队头领被拴住返回 true
     */
    [[nodiscard]] bool _firstIsLeashed(const LlamaEntity* llama, i32 depth) const;

    LlamaEntity* m_llama;
    f32 m_speed;
    f64 m_speedModifier;    // 速度修正（距离太远时加速）
    i32 m_distCheckCounter; // 距离检查计数器
};

/**
 * @brief 疯狂奔跑目标
 *
 * 未驯服的马被骑乘时会四处乱跑，增加驯服难度。
 * 每次改变方向时有概率增加驯服进度。
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
    bool _findTarget();

    AbstractHorseEntity* m_horse;
    f64 m_speed;
    f64 m_targetX = 0.0;
    f64 m_targetY = 0.0;
    f64 m_targetZ = 0.0;

    static constexpr i32 MIN_RUN_TICKS = 20;  // 最少运行时间
    static constexpr i32 MAX_RUN_TICKS = 100; // 最多运行时间
};

/**
 * @brief 河豚膨胀目标
 *
 * 当检测到敌对生物或玩家靠近时触发膨胀行为。
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
     * - 玩家：非旁观者模式且非创造模式
     * - 其他生物：非水生生物
     *
     * @param entity 生物实体
     * @return 如果是威胁返回 true
     */
    [[nodiscard]] static bool _isEnemy(const LivingEntity* entity);

    /**
     * @brief 查找附近的威胁实体
     * @return 如果找到威胁实体返回 true
     */
    [[nodiscard]] bool _findNearbyEnemy();

    ::mc::PufferfishEntity* m_fish;
    LivingEntity* m_nearbyEnemy = nullptr;

    static constexpr f32 DETECTION_RANGE = 2.0f; // 检测范围（碰撞箱向外扩展）
};

/**
 * @brief 羊驼防御目标
 *
 * 羊驼攻击附近的未驯服的狼。
 *
 * 这是一个内部类，用于羊驼防御狼。
 * 检测范围 16 格的 1/4（即 4 格）。
 */
class LlamaDefendTargetGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param llama 羊驼实体
     */
    explicit LlamaDefendTargetGoal(LlamaEntity* llama);

    ~LlamaDefendTargetGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    void resetTask() override;

    [[nodiscard]] std::string getTypeName() const override { return "LlamaDefendTargetGoal"; }

    // 常量（公开用于测试）
    static constexpr f64 TARGET_RANGE = 16.0;          // 基础检测范围
    static constexpr f64 TARGET_RANGE_MODIFIER = 0.25; // 范围修正系数（实际范围 = 16 * 0.25 = 4格）

private:
    LlamaEntity* m_llama;
    LivingEntity* m_target = nullptr;
};

/**
 * @brief 商队羊驼保卫流浪商人目标
 *
 * 当商队羊驼被拴在流浪商人身上时，如果流浪商人受到攻击，
 * 商队羊驼会反击攻击者。类似于 OwnerHurtByTargetGoal，
 * 但目标来源是拴绳持有者（流浪商人）而非驯服者。
 *
 * TODO: 需要集成测试验证 TraderLlamaDefendWanderingTraderGoal 的完整行为：
 * - 被拴在流浪商人身上且商人受攻击时 shouldExecute() 返回 true
 * - 未拴绳或拴在其他实体上时 shouldExecute() 返回 false
 * - 时间戳更新防止重复触发
 * 当前缺少 World mock 来设置拴绳+流浪商人+攻击者交互场景。
 */
class TraderLlamaDefendWanderingTraderGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param llama 商队羊驼实体
     */
    explicit TraderLlamaDefendWanderingTraderGoal(TraderLlamaEntity* llama);

    ~TraderLlamaDefendWanderingTraderGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;

    [[nodiscard]] std::string getTypeName() const override { return "TraderLlamaDefendWanderingTraderGoal"; }

private:
    TraderLlamaEntity* m_llama;
    LivingEntity* m_ownerLastHurtBy = nullptr;
    i32 m_timestamp = 0;
};

/**
 * @brief 骷髅马陷阱触发目标
 *
 * 当玩家接近陷阱骷髅马时触发陷阱，生成骷髅骑手。
 *
 * 执行条件:
 * - 骷髅马是陷阱马 (isTrap() == true)
 * - 玩家在 10 格范围内
 *
 * tick 行为:
 * - 触发陷阱（生成骷髅骑手）
 * - 困难模式下生成额外 3 只骷髅马+骑手
 *
 * 注意：此 Goal 在 setTrap(true) 时注册，setTrap(false) 时移除
 */
class TriggerSkeletonTrapGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param horse 骷髅马实体
     */
    explicit TriggerSkeletonTrapGoal(SkeletonHorseEntity* horse);

    ~TriggerSkeletonTrapGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "TriggerSkeletonTrapGoal"; }

    // 常量（公开用于测试）
    static constexpr f64 PLAYER_DETECTION_RANGE = 10.0;     // 玩家检测范围
    static constexpr f64 PLAYER_DETECTION_RANGE_SQ = 100.0; // 玩家检测范围平方

private:
    SkeletonHorseEntity* m_horse;
};

} // namespace entity::ai::goal
} // namespace mc
