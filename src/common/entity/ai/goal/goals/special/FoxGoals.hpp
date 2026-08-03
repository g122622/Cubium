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
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockPos.hpp"
#include <string>

namespace mc {

// Forward declarations
class FoxEntity;
class CreatureEntity;
class LivingEntity;
class MobEntity;
class IWorld;
class BlockState;
class ItemEntity;

namespace blocks {
class SweetBerryBushBlock;
} // namespace blocks

namespace entity::ai::goal {

/**
 * @brief 狐狸被动目标基类
 *
 * 当狐狸处于激怒状态时，所有被动行为会被打断。
 */
class FoxPassiveGoal : public Goal {
public:
    explicit FoxPassiveGoal(FoxEntity* fox);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;

protected:
    /**
     * @brief 检查当前天气是否有遮蔽（看不到天空）
     */
    [[nodiscard]] bool hasShelter() const;

    /**
     * @brief 检查周围是否有警觉目标
     */
    [[nodiscard]] bool hasAlertableTarget() const;

    /**
     * @brief 检查狐狸是否可以行动（非激怒、非坐下等状态）
     */
    [[nodiscard]] bool canAct() const;

    /**
     * @brief 子类实现具体开始条件
     */
    [[nodiscard]] virtual bool canFoxStart() = 0;

    /**
     * @brief 子类实现具体继续条件
     */
    [[nodiscard]] virtual bool canFoxContinue() = 0;

    FoxEntity* m_fox;
};

// ============================================================================
// 狐狸行为目标 (Goal Selector)
// ============================================================================

/**
 * @brief 狐狸跟踪猎物目标
 *
 * 这是扑击的前置阶段，狐狸会：
 * 1. 接近猎物（距离 > 6 格）
 * 2. 当距离 <= 6 格时进入蹲伏状态
 * 3. 检查路径是否畅通
 *
 * 优先级: 5
 * Mutex: MOVE, LOOK
 */
class FoxFollowTargetGoal : public Goal {
public:
    explicit FoxFollowTargetGoal(FoxEntity* fox);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "FoxFollowTargetGoal"; }

    /**
     * @brief 检查从狐狸到目标的路径是否畅通
     */
    [[nodiscard]] static bool isPathClear(FoxEntity* fox, LivingEntity* target);

private:
    FoxEntity* m_fox;
    LivingEntity* m_target = nullptr;

    // 常量
    static constexpr f64 START_FOLLOW_DISTANCE_SQ = 36.0; // 6^2 = 开始跟踪距离
    static constexpr f64 STOP_FOLLOW_DISTANCE_SQ = 36.0;  // 6^2 = 停止跟踪距离（进入蹲伏）
    static constexpr f64 APPROACH_SPEED = 1.5;            // 接近速度
};

/**
 * @brief 狐狸扑击目标
 *
 * 狐狸从蹲伏状态跳起扑向猎物。
 *
 * 行为流程：
 * 1. 在完全蹲伏状态时触发
 * 2. 计算扑击向量并发射
 * 3. 空中调整俯仰角
 * 4. 接近目标时攻击
 * 5. 落地可能卡在雪中
 *
 * 优先级: 6
 * Mutex: MOVE, LOOK, JUMP
 */
class FoxPounceGoal : public Goal {
public:
    explicit FoxPounceGoal(FoxEntity* fox);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    [[nodiscard]] bool isPreemptible() const override { return false; } // 扑击不可中断
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "FoxPounceGoal"; }

private:
    FoxEntity* m_fox;
    LivingEntity* m_target = nullptr;

    // 常量
    static constexpr f64 POUNCE_HORIZONTAL_FACTOR = 0.8; // 水平方向扑击因子
    static constexpr f64 POUNCE_VERTICAL_FACTOR = 0.9;   // 垂直方向扑击因子
    static constexpr f32 ATTACK_DISTANCE = 2.0f;         // 攻击距离
    static constexpr f32 MIN_MOTION_Y_SQ = 0.05f;        // 最小垂直速度平方
    static constexpr f32 MAX_PITCH_ANGLE = 15.0f;        // 最大俯仰角
    static constexpr f32 STUCK_PITCH_ANGLE = 60.0f;      // 卡在雪中时的俯仰角
};

/**
 * @brief 狐狸咬击目标
 *
 * 继承自 MeleeAttackGoal，添加了狐狸特有的条件检查：
 * - 不在坐下、睡眠、蹲伏、卡住状态时执行
 * - 攻击时播放咬音效
 *
 * 优先级: 7
 */
class FoxBiteGoal : public MeleeAttackGoal {
public:
    FoxBiteGoal(FoxEntity* fox, f64 speed, bool useLongMemory);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;

    [[nodiscard]] std::string getTypeName() const noexcept override { return "FoxBiteGoal"; }

protected:
    void checkAndPerformAttack(LivingEntity* enemy, f64 distToEnemySqr) override;

private:
    FoxEntity* m_foxEntity;
};

/**
 * @brief 狐狸寻找庇护所目标
 *
 * 在白天或雷暴天气时寻找遮蔽处休息。
 *
 * 优先级: 6
 */
class FoxFindShelterGoal : public FoxPassiveGoal {
public:
    explicit FoxFindShelterGoal(FoxEntity* fox, f64 speed);

    [[nodiscard]] bool canFoxStart() override;
    [[nodiscard]] bool canFoxContinue() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "FoxFindShelterGoal"; }

private:
    f64 m_speed;
    i32 m_cooldown = 100;
    BlockPos m_shelterPos;
    bool m_hasShelter = false;

    static constexpr i32 COOLDOWN_MIN = 100;
    static constexpr i32 COOLDOWN_MAX = 140;
};

/**
 * @brief 狐狸睡眠目标
 *
 * 狐狸在白天有遮蔽处睡觉。
 *
 * 优先级: 7
 * Mutex: MOVE, LOOK, JUMP
 */
class FoxSleepGoal : public FoxPassiveGoal {
public:
    explicit FoxSleepGoal(FoxEntity* fox);

    [[nodiscard]] bool canFoxStart() override;
    [[nodiscard]] bool canFoxContinue() override;
    void startExecuting() override;
    void resetTask() override;

    [[nodiscard]] std::string getTypeName() const override { return "FoxSleepGoal"; }

private:
    i32 m_cooldown;

    static constexpr i32 COOLDOWN_MAX = 140;
};

/**
 * @brief 狐狸吃浆果目标
 *
 * 狐狸寻找并吃成熟的甜浆果丛或发光浆果藤。
 * 继承自 Goal 而非 MoveToBlockGoal，因为需要自定义到达后的等待和交互逻辑。
 *
 * 行为流程：
 * 1. shouldExecute: 搜索附近成熟的甜浆果丛或发光浆果藤
 * 2. startExecuting: 取消坐下状态，导航到目标
 * 3. tick: 到达后等待 40 tick，然后采摘浆果
 * 4. 采摘甜浆果丛：AGE 重置为 1，掉落浆果，优先放入主手
 * 5. 采摘发光浆果：BERRIES 设为 false，优先放入主手
 *
 * 优先级: 10
 */
class FoxEatBerriesGoal : public Goal {
public:
    FoxEatBerriesGoal(FoxEntity* fox, f64 speed, i32 searchRange, i32 verticalSearchRange);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "FoxEatBerriesGoal"; }

private:
    /**
     * @brief 检查目标方块是否是有效的浆果来源
     * 甜浆果丛 AGE >= 2 或洞穴藤蔓 BERRIES == true
     */
    [[nodiscard]] bool _isValidTarget(const IWorld* world, const BlockPos& pos) const;

    /**
     * @brief 到达目标后执行采摘
     */
    void _eatBerry();

    /**
     * @brief 采摘甜浆果丛
     */
    void _pickSweetBerries(const BlockState& state);

    /**
     * @brief 采摘发光浆果
     */
    void _pickGlowBerry(const BlockState& state);

    /**
     * @brief 在搜索范围内寻找有效的浆果方块
     * @return 是否找到目标
     */
    [[nodiscard]] bool _searchForTarget();

    /**
     * @brief 导航到目标方块位置
     */
    void _moveToTarget();

    FoxEntity* m_fox;
    f64 m_speed;
    i32 m_searchRange;
    i32 m_verticalSearchRange;
    BlockPos m_targetPos;
    i32 m_eatTimer = 0;
    bool m_reached = false;

    static constexpr i32 EAT_DURATION = 40;        // 吃浆果需要 40 tick
    static constexpr f32 REACH_DISTANCE_SQ = 2.0f; // 到达目标的距离平方
};

/**
 * @brief 狐狸寻找物品目标
 *
 * 狐狸捡起地上的物品（如食物）。
 * 搜索 8 格范围内的 ItemEntity，导航到最近的物品实体。
 * 实际拾取逻辑由 FoxEntity::pickUpItem() 处理（由 ItemPickupManager 调用）。
 *
 * 优先级: 11
 * Mutex: MOVE
 */
class FoxFindItemsGoal : public Goal {
public:
    explicit FoxFindItemsGoal(FoxEntity* fox);

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "FoxFindItemsGoal"; }

private:
    /**
     * @brief 在搜索范围内寻找最近的可用物品实体
     * @return 最近的物品实体指针，如果没有则返回 nullptr
     */
    [[nodiscard]] ItemEntity* _findNearestItem() const;

    FoxEntity* m_fox;

    static constexpr f64 SEARCH_RADIUS = 8.0; // 搜索半径
    static constexpr f64 MOVE_SPEED = 1.2;    // 移动速度
    static constexpr i32 CHANCE = 10;         // 1/10 概率触发
};

/**
 * @brief 狐狸坐下观察目标
 *
 * 狐狸坐下并随机观察周围。
 *
 * 优先级: 13
 * Mutex: MOVE, LOOK
 */
class FoxSitAndLookGoal : public FoxPassiveGoal {
public:
    explicit FoxSitAndLookGoal(FoxEntity* fox);

    [[nodiscard]] bool canFoxStart() override;
    [[nodiscard]] bool canFoxContinue() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "FoxSitAndLookGoal"; }

private:
    /**
     * @brief 随机选择观察方向
     */
    void _chooseRandomLookDirection();

    f64 m_lookX = 0.0;
    f64 m_lookZ = 0.0;
    i32 m_lookTimer = 0;
    i32 m_lookCount = 0;

    static constexpr i32 LOOK_DURATION_MIN = 80;  // 最小观察时间（tick）
    static constexpr i32 LOOK_DURATION_MAX = 100; // 最大观察时间（tick）
    static constexpr i32 LOOK_COUNT_MIN = 2;      // 最小观察次数
    static constexpr i32 LOOK_COUNT_MAX = 4;      // 最大观察次数
    static constexpr f32 TRIGGER_CHANCE = 0.02f;  // 触发概率
};

/**
 * @brief 狐狸卡在雪中目标
 *
 * 当狐狸扑击后落在雪方块上时，会进入卡住（faceplanted）状态。
 * 此目标使狐狸在 40 tick（2 秒）内保持卡住状态，然后自动脱离。
 *
 * 对应 MC Java: Fox.FaceplantGoal
 * 优先级: 1
 * Mutex: MOVE, LOOK, JUMP（卡住期间禁止移动、观察和跳跃）
 */
class FoxStuckInSnowGoal : public Goal {
public:
    explicit FoxStuckInSnowGoal(FoxEntity* fox);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "FoxStuckInSnowGoal"; }

private:
    FoxEntity* m_fox;
    i32 m_countdown = 0;

    /// 卡住持续时间（tick），对应 MC 的 adjustedTickDelay(40)
    static constexpr i32 STUCK_DURATION = 40;
};

// ============================================================================
// 狐狸目标选择器目标 (Target Selector)
// ============================================================================

/**
 * @brief 狐狸复仇目标
 *
 * 当信任的玩家被攻击时，狐狸会攻击攻击者。
 *
 * 优先级: 3
 */
class FoxRevengeGoal : public entity::ai::goal::NearestAttackableTargetGoal<LivingEntity> {
public:
    FoxRevengeGoal(FoxEntity* fox);

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;

    [[nodiscard]] std::string getTypeName() const override { return "FoxRevengeGoal"; }

private:
    FoxEntity* m_foxEntity;
    LivingEntity* m_attackerOfTrusted = nullptr;
    LivingEntity* m_trustedEntity = nullptr;
    i32 m_revengeTimestamp = 0;
};

} // namespace entity::ai::goal
} // namespace mc
