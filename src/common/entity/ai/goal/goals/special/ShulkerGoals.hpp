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
#include "common/core/Types.hpp"
#include "common/entity/interfaces/IAngerable.hpp"
#include <string>

namespace mc {

// Forward declarations
class ShulkerEntity;
class LivingEntity;

namespace entity::ai::goal {

/**
 * @brief 潜影贝攻击目标
 *
 * 控制潜影贝的开壳和发射子弹行为。
 * 当攻击目标在范围内时打开贝壳并发射追踪子弹。
 *
 * 执行条件:
 * - 有攻击目标
 * - 攻击目标存活
 * - 贝壳闭合或正在打开
 *
 * tick 行为:
 * - 如果攻击冷却完成且贝壳打开，发射子弹
 * - 更新攻击冷却
 */
class ShulkerAttackGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param shulker 潜影贝实体
     */
    explicit ShulkerAttackGoal(ShulkerEntity* shulker);

    ~ShulkerAttackGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "ShulkerAttackGoal"; }

private:
    ShulkerEntity* m_shulker;
    LivingEntity* m_target = nullptr;

    static constexpr f32 ATTACK_RANGE_SQ = 400.0f; // 20.0 * 20.0
};

/**
 * @brief 潜影贝张望目标
 *
 * 控制潜影贝空闲时的开壳张望行为。
 * 当没有攻击目标时，随机打开贝壳张望。
 *
 * 执行条件:
 * - 没有攻击目标
 * - 贝壳闭合
 * - 随机概率触发
 *
 * tick 行为:
 * - 打开贝壳一段时间
 * - 随机看向不同方向
 * - 然后关闭贝壳
 */
class ShulkerPeekGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param shulker 潜影贝实体
     */
    explicit ShulkerPeekGoal(ShulkerEntity* shulker);

    ~ShulkerPeekGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "ShulkerPeekGoal"; }

private:
    ShulkerEntity* m_shulker;
    i32 m_peekTime = 0;      // 张望时间
    i32 m_totalPeekTime = 0; // 总张望时长

    static constexpr i32 MIN_PEEK_TIME = 20;   // 最小张望时间 (1秒)
    static constexpr i32 MAX_PEEK_TIME = 60;   // 最大张望时间 (3秒)
    static constexpr f32 PEEK_CHANCE = 0.025f; // 每tick触发概率
};

/**
 * @brief 潜影贝最近攻击目标选择
 *
 * 选择最近的玩家作为攻击目标。
 * 和平难度下不执行此目标。
 */
class ShulkerNearestAttackGoal : public NearestAttackableTargetGoal<Player> {
public:
    /**
     * @brief 构造函数
     * @param shulker 潜影贝实体
     */
    explicit ShulkerNearestAttackGoal(ShulkerEntity* shulker);

    ~ShulkerNearestAttackGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;

    [[nodiscard]] std::string getTypeName() const override { return "ShulkerNearestAttackGoal"; }
};

/**
 * @brief 潜影贝防御攻击目标选择
 *
 * 当潜影贝处于队伍中时，攻击附近的敌对生物（IMob）。
 * 这是团队防御行为：被命令分配到队伍的潜影贝会主动攻击附近的怪物。
 */
class ShulkerDefenseAttackGoal : public NearestAttackableTargetGoal<LivingEntity> {
public:
    /**
     * @brief 构造函数
     * @param shulker 潜影贝实体
     */
    explicit ShulkerDefenseAttackGoal(ShulkerEntity* shulker);

    ~ShulkerDefenseAttackGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;

    [[nodiscard]] std::string getTypeName() const override { return "ShulkerDefenseAttackGoal"; }
};

} // namespace entity::ai::goal
} // namespace mc
