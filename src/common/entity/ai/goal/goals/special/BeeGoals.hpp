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
#include <vector>

namespace mc {

// Forward declarations
class BeeEntity;
class CreatureEntity;
class LivingEntity;
class MobEntity;
class Player;
class IWorld;
class BlockState;

namespace entity::ai::goal {

/**
 * @brief 蜜蜂被动目标基类
 *
 * 当蜜蜂处于愤怒状态时，所有被动行为会被打断。
 */
class BeePassiveGoal : public Goal {
public:
    explicit BeePassiveGoal(BeeEntity* bee);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;

protected:
    /**
     * @brief 检查蜜蜂是否可以开始被动行为
     * 子类实现具体条件
     */
    [[nodiscard]] virtual bool canBeeStart() = 0;

    /**
     * @brief 检查蜜蜂是否可以继续被动行为
     * 子类实现具体条件
     */
    [[nodiscard]] virtual bool canBeeContinue() = 0;

    BeeEntity* m_bee;
};

// ============================================================================
// 蜜蜂行为目标 (Goal Selector)
// ============================================================================

/**
 * @brief 蜜蜂蛰刺攻击目标
 *
 * 继承自 MeleeAttackGoal，添加了蛰刺后的特殊处理：
 * - 只有愤怒且未蛰刺过时才执行
 * - 攻击后设置 hasStung 标志
 * - 蛰刺后蜜蜂会逐渐死亡
 *
 * 优先级: 0 (最高)
 */
class BeeStingGoal : public MeleeAttackGoal {
public:
    explicit BeeStingGoal(BeeEntity* bee);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void tick() override;

    // 蜇刺攻击命中后设 hasStung=true，激活 BeeEntity::tick() 中"螫刺后逐渐死亡"链路
    // （对齐 vanilla Bee.doHurtTarget：造成伤害后 this.setHasStung(true)）。
    // override MeleeAttackGoal 的攻击入口：基类在距离+冷却达标时调 _attackTarget 造成伤害，
    // 此处在基类执行后比较目标 HP 变化判定命中，命中且未螫刺过则设 stung。此前 BeeStingGoal::tick
    // 仅转调基类、从不设 hasStung，致 m_hasStung 恒 false、蜇人后死亡链路为不可达死代码、
    // 蜜蜂可无限蜇人（与 vanilla 偏差）。
    void checkAndPerformAttack(LivingEntity* target, f64 distToEnemySqr) override;

    [[nodiscard]] std::string getTypeName() const noexcept override { return "BeeStingGoal"; }

private:
    BeeEntity* m_beeEntity;
};

/**
 * @brief 蜜蜂进入蜂巢目标
 *
 * 当蜜蜂满足进入蜂巢条件时，尝试进入蜂巢方块。
 *
 * 触发条件：
 * - 有蜂巢位置
 * - 能进入蜂巢（无花粉超时/下雨/夜晚/有花粉）
 * - 在蜂巢附近（2格内）
 * - 蜂巢未满
 *
 * 优先级: 1
 */
class BeeEnterHiveGoal : public BeePassiveGoal {
public:
    explicit BeeEnterHiveGoal(BeeEntity* bee);

    [[nodiscard]] bool canBeeStart() override;
    [[nodiscard]] bool canBeeContinue() override;
    void startExecuting() override;

    [[nodiscard]] std::string getTypeName() const override { return "BeeEnterHiveGoal"; }
};

/**
 * @brief 蜜蜂授粉目标
 *
 * 蜜蜂飞向花朵并采集花粉。
 *
 * 行为：
 * - 搜索附近5格内的花朵
 * - 飞向花朵并在附近徘徊
 * - 400 tick 后获得花粉
 * - 下雨时停止
 *
 * 优先级: 4
 * Mutex: MOVE
 */
class BeePollinateGoal : public BeePassiveGoal {
public:
    explicit BeePollinateGoal(BeeEntity* bee);

    [[nodiscard]] bool canBeeStart() override;
    [[nodiscard]] bool canBeeContinue() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "BeePollinateGoal"; }

    /// 检查是否正在授粉
    [[nodiscard]] bool isRunning() const { return m_running; }

    /**
     * @brief 立即停止授粉（不触发完整结束逻辑）
     *
     * 对齐 MC Java 1.21.11 BeePollinateGoal.stopPollinating（Bee.java:1150-1152）：
     *   void stopPollinating() { this.pollinating = false; }
     * 仅置授粉标志为 false，不调 resetTask 的完整结束逻辑（setHasNectar/清路径/花朵冷却）。
     * Bee.hurtServer 受击时调用此方法立即中断授粉。
     * Cubium 同步 m_bee->setPollinating(false) 保持 bee 状态与 goal 一致（Cubium bee 单独
     * 存 m_pollinating，vanilla bee.isPollinating 读 goal.pollinating）。
     */
    void stopPollinating();

protected:
    /// 检查位置是否是花朵
    [[nodiscard]] bool _isFlower(const BlockPos& pos) const;

private:
    /// 搜索附近的花朵
    [[nodiscard]] bool _findFlower();

    /// 移动到下一个目标位置
    void _moveToNextTarget();

    /// 检查是否完成授粉
    [[nodiscard]] bool _completedPollination() const { return m_pollinationTicks > 400; }

    i32 m_pollinationTicks = 0;  ///< 授粉进度
    i32 m_lastSoundTick = 0;     ///< 上次播放声音的tick
    i32 m_totalTicks = 0;        ///< 总计时间
    bool m_running = false;      ///< 是否正在授粉
    math::Vector3f m_nextTarget; ///< 下一个飞向目标

    static constexpr f32 FLOWER_SEARCH_RANGE = 5.0f; ///< 花朵搜索范围
    static constexpr i32 POLLINATION_DURATION = 400; ///< 授粉所需时间
    static constexpr i32 MAX_POLLINATION_TIME = 600; ///< 最大授粉时间
};

/**
 * @brief 蜜蜂更新蜂巢位置目标
 *
 * 当蜜蜂没有蜂巢时，搜索附近可用的蜂巢。
 *
 * 优先级: 5
 */
class BeeUpdateHiveGoal : public BeePassiveGoal {
public:
    explicit BeeUpdateHiveGoal(BeeEntity* bee);

    [[nodiscard]] bool canBeeStart() override;
    [[nodiscard]] bool canBeeContinue() override;
    void startExecuting() override;

    [[nodiscard]] std::string getTypeName() const override { return "BeeUpdateHiveGoal"; }

private:
    /// 搜索附近可用的蜂巢
    [[nodiscard]] std::vector<BlockPos> _findNearbyFreeHives() const;

    /// 检查蜂巢是否有空间
    [[nodiscard]] bool _doesHiveHaveSpace(const BlockPos& pos) const;
};

/**
 * @brief 蜜蜂寻找蜂巢目标
 *
 * 当蜜蜂需要返回蜂巢时，导航到蜂巢位置。
 *
 * 优先级: 5
 * Mutex: MOVE
 */
class BeeFindHiveGoal : public BeePassiveGoal {
public:
    explicit BeeFindHiveGoal(BeeEntity* bee);

    [[nodiscard]] bool canBeeStart() override;
    [[nodiscard]] bool canBeeContinue() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "BeeFindHiveGoal"; }

    /// 检查位置是否是可能的蜂巢（用于避免重复搜索）
    [[nodiscard]] bool isPossibleHive(const BlockPos& pos) const;

    /// 清除可能的蜂巢列表
    void clearPossibleHives() { m_possibleHives.clear(); }

private:
    /// 检查是否足够靠近蜂巢
    [[nodiscard]] bool _isCloseEnough(const BlockPos& pos) const;

    /// 检查是否太远
    [[nodiscard]] bool _isTooFar(const BlockPos& pos) const;

    i32 m_ticks = 0;                       ///< 计时器
    std::vector<BlockPos> m_possibleHives; ///< 可能的蜂巢列表
    i32 m_stuckCounter = 0;                ///< 路径卡住计数器

    static constexpr i32 MAX_NAVIGATION_TIME = 600; ///< 最大导航时间
    static constexpr i32 STUCK_THRESHOLD = 60;      ///< 路径卡住阈值
};

/**
 * @brief 蜜蜂寻找花朵目标
 *
 * 当蜜蜂长时间没有花粉时，飞向记忆中的花朵位置。
 *
 * 优先级: 6
 * Mutex: MOVE
 */
class BeeFindFlowerGoal : public BeePassiveGoal {
public:
    explicit BeeFindFlowerGoal(BeeEntity* bee);

    [[nodiscard]] bool canBeeStart() override;
    [[nodiscard]] bool canBeeContinue() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "BeeFindFlowerGoal"; }

private:
    /// 检查花朵是否太远（超过48格）
    [[nodiscard]] bool _isTooFar(const BlockPos& pos) const;

    i32 m_ticks = 0; ///< 计时器

    static constexpr i32 MAX_NAVIGATION_TIME = 600;            ///< 最大导航时间
    static constexpr i32 TICKS_WITHOUT_NECTAR_THRESHOLD = 600; ///< 30秒无花粉阈值，超过此值蜜蜂尝试飞向已知花朵
};

/**
 * @brief 蜜蜂寻找授粉目标
 *
 * 当蜜蜂有花粉时，飞过农作物并促进其生长。
 *
 * 优先级: 7
 */
class BeeFindPollinationTargetGoal : public BeePassiveGoal {
public:
    explicit BeeFindPollinationTargetGoal(BeeEntity* bee);

    [[nodiscard]] bool canBeeStart() override;
    [[nodiscard]] bool canBeeContinue() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "BeeFindPollinationTargetGoal"; }

protected:
    /// 检查位置是否是可授粉作物
    [[nodiscard]] bool _isPollinationTarget(const BlockPos& pos) const;

    /// 促进作物生长（返回是否成功生长）
    [[nodiscard]] bool _growCrop(const BlockPos& pos);

private:
    static constexpr i32 MAX_CROPS_GROWN = 10; ///< 每次授粉最多促进的作物数
};

/**
 * @brief 蜜蜂随机飞行目标
 *
 * 当没有其他任务时，蜜蜂会随机飞行。
 * 如果离蜂巢太远（22格），会飞回蜂巢方向。
 *
 * 优先级: 8
 * Mutex: MOVE
 */
class BeeWanderGoal : public Goal {
public:
    explicit BeeWanderGoal(BeeEntity* bee);

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "BeeWanderGoal"; }

protected:
    /// 检查位置是否有效
    [[nodiscard]] bool _isValidLocation(const math::Vector3f& pos) const;

private:
    /// 获取随机飞行位置
    [[nodiscard]] math::Vector3f _getRandomLocation();

    BeeEntity* m_bee;

    static constexpr f32 WANDER_RANGE = 8.0f;    ///< 漫游范围（回退时使用）
    static constexpr f32 WANDER_HEIGHT = 7.0f;   ///< 漫游高度范围（回退时使用）
    static constexpr i32 XZ_RANGE = 8;           ///< HoverRandomPos 水平搜索范围
    static constexpr i32 Y_RANGE = 7;            ///< HoverRandomPos 垂直搜索范围
    static constexpr i32 Y_RANGE_FALLBACK = 4;   ///< AirAndWaterRandomPos 垂直搜索范围（备选策略）
    static constexpr i32 Y_OFFSET_FALLBACK = -2; ///< AirAndWaterRandomPos Y轴偏移（备选策略）
    static constexpr i32 WANDER_CHANCE = 10;     ///< 漫游概率倒数
};

// ============================================================================
// 蜜蜂目标选择器 (Target Selector)
// ============================================================================

/**
 * @brief 蜜蜂愤怒目标
 *
 * 当蜜蜂被攻击时，记住攻击者并愤怒。
 * 会召唤附近的其他蜜蜂一起攻击。
 *
 * 优先级: 1 (Target)
 */
class BeeAngerGoal : public HurtByTargetGoal {
public:
    explicit BeeAngerGoal(BeeEntity* bee);

    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;

    [[nodiscard]] std::string getTypeName() const override { return "BeeAngerGoal"; }

private:
    BeeEntity* m_beeEntity;
};

/**
 * @brief 蜜蜂攻击玩家目标
 *
 * 当蜜蜂愤怒时，攻击附近的玩家。
 * 只有未蛰刺过的蜜蜂才会攻击。
 *
 * 优先级: 2 (Target)
 */
class BeeAttackPlayerGoal : public TargetGoal {
public:
    BeeAttackPlayerGoal(BeeEntity* bee, i32 chance);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;

    [[nodiscard]] std::string getTypeName() const override { return "BeeAttackPlayerGoal"; }

private:
    /// 检查是否可以蛰刺
    [[nodiscard]] bool _canSting() const;

    BeeEntity* m_beeEntity;
    Player* m_targetPlayer = nullptr;
    i32 m_chance;

    static constexpr f32 TARGET_RANGE = 10.0f; ///< 目标搜索范围
};

/**
 * @brief 蜜蜂重置愤怒目标
 *
 * 当愤怒时间结束后，重置愤怒状态。
 *
 * 优先级: 3 (Target)
 */
class BeeResetAngerGoal : public Goal {
public:
    explicit BeeResetAngerGoal(BeeEntity* bee);

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;

    [[nodiscard]] std::string getTypeName() const override { return "BeeResetAngerGoal"; }

private:
    BeeEntity* m_bee;
};

} // namespace entity::ai::goal
} // namespace mc
