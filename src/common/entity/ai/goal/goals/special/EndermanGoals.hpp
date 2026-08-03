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

#include "../../Goal.hpp"
#include "../../GoalFlag.hpp"
#include "../target/TargetGoals.hpp"
#include "core/Types.hpp"
#include <functional>
#include <string>

namespace mc {

// Forward declarations
class EndermanEntity;
class Player;
class LivingEntity;
class BlockState;
class BlockPos;
class IWorld;
class BlockState;
class BlockPos;

namespace entity::ai::goal {

/**
 * @brief 末影人注视目标
 *
 * 当玩家正在注视末影人时，末影人会停止移动并注视玩家。
 * 这是末影人的特有行为，在被激怒前会先注视玩家。
 *
 * 行为：
 * - 当攻击目标是玩家且正在注视末影人时激活
 * - 停止移动，注视目标
 * - 设置互斥标志：JUMP, MOVE
 */
class EndermanStareGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param enderman 拥有此目标的末影人实体
     */
    explicit EndermanStareGoal(EndermanEntity* enderman);

    ~EndermanStareGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "EndermanStareGoal"; }

private:
    EndermanEntity* m_enderman;
    LivingEntity* m_targetPlayer = nullptr;

    // 注视距离阈值（16格的平方）
    static constexpr f64 STARE_RANGE_SQ = 256.0;
};

/**
 * @brief 末影人查找玩家目标
 *
 * 查找正在注视末影人的玩家并激怒末影人。
 * 这是末影人的特有目标选择器，用于实现"被注视时激怒"的行为。
 *
 * 行为：
 * - 查找 10 格内正在注视末影人的玩家
 * - 激怒末影人并设置攻击目标
 * - 近距离时尝试瞬移躲避
 * - 远距离时瞬移到目标附近
 *
 * 这个目标使用自定义的玩家筛选逻辑。
 */
class EndermanFindPlayerGoal : public TargetGoal {
public:
    /**
     * @brief 构造函数
     * @param enderman 拥有此目标的末影人实体
     */
    explicit EndermanFindPlayerGoal(EndermanEntity* enderman);

    ~EndermanFindPlayerGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    void resetTask() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "EndermanFindPlayerGoal"; }

private:
    /**
     * @brief 检查玩家是否正在注视末影人
     * @param player 要检查的玩家
     * @return 如果玩家正在注视末影人返回 true
     */
    [[nodiscard]] bool shouldAttackPlayer(Player* player) const;

    EndermanEntity* m_enderman;
    Player* m_targetPlayer = nullptr;
    i32 m_aggroTime = 0;    // 激怒计时器
    i32 m_teleportTime = 0; // 瞬移计时器

    // 常量
    static constexpr i32 AGGRO_DURATION = 5;               // 激怒持续时间（ticks）
    static constexpr f64 TELEPORT_NEAR_DISTANCE_SQ = 16.0; // 近距离瞬移阈值（4格的平方）
    static constexpr f64 TELEPORT_FAR_DISTANCE_SQ = 256.0; // 远距离瞬移阈值（16格的平方）
    static constexpr i32 TELEPORT_COOLDOWN_TICKS = 30;     // 瞬移冷却（ticks）
    static constexpr i32 TARGET_DISTANCE = 10;             // 目标搜索距离（格）
};

/**
 * @brief 末影人放置方块目标
 *
 * 末影人将拿着的方块放置到世界中。
 *
 * 行为：
 * - 只有当末影人拿着方块时才执行
 * - 需要 mobGriefing 游戏规则为 true
 * - 执行概率：1/2000 (每tick)
 * - 在末影人周围 2x2x2 范围内随机选择放置位置
 * - 检查目标位置是否为空气、下方方块是否有效、是否有实体碰撞
 *
 * 互斥标志：无（低优先级后台任务）
 */
class EndermanPlaceBlockGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param enderman 拥有此目标的末影人实体
     */
    explicit EndermanPlaceBlockGoal(EndermanEntity* enderman);

    ~EndermanPlaceBlockGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "EndermanPlaceBlockGoal"; }

private:
    /**
     * @brief 检查是否可以在指定位置放置方块
     * @param world 世界实例
     * @param pos 放置位置
     * @param state 要放置的方块状态
     * @param currentState 当前位置的方块状态
     * @param belowState 下方方块的方块状态
     * @param belowPos 下方方块的位置
     * @return 如果可以放置返回 true
     */
    [[nodiscard]] bool canPlaceBlock(IWorld* world,
        const BlockPos& pos,
        const BlockState* state,
        const BlockState* currentState,
        const BlockState* belowState,
        const BlockPos& belowPos) const;

    EndermanEntity* m_enderman;

    // 执行概率：1/2000 (每tick)
    static constexpr i32 PLACE_CHANCE = 2000;
};

/**
 * @brief 末影人拾取方块目标
 *
 * 末影人从世界中拾取方块。
 *
 * 行为：
 * - 只有当末影人没有拿着方块时才执行
 * - 需要 mobGriefing 游戏规则为 true
 * - 执行概率：1/20 (每tick)
 * - 在末影人周围 4x3x4 范围内随机选择拾取位置
 * - 使用射线检测确保可以到达目标方块
 * - 只拾取 ENDERMAN_HOLDABLE 标签中的方块
 *
 * 互斥标志：无（低优先级后台任务）
 */
class EndermanTakeBlockGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param enderman 拥有此目标的末影人实体
     */
    explicit EndermanTakeBlockGoal(EndermanEntity* enderman);

    ~EndermanTakeBlockGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "EndermanTakeBlockGoal"; }

private:
    EndermanEntity* m_enderman;

    // 执行概率：1/20 (每tick)
    static constexpr i32 TAKE_CHANCE = 20;
};

} // namespace entity::ai::goal
} // namespace mc
