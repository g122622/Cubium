/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so so, subject to the following conditions:
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

#include "../../../../../core/Types.hpp"
#include "../../Goal.hpp"
#include "../../GoalFlag.hpp"

namespace mc {

// 前向声明
class SlimeEntity;
class LivingEntity;
class Player;
class MobEntity;

namespace entity::ai::goal {

/**
 * @brief 史莱姆水中漂浮目标
 *
 * 当史莱姆在水中或岩浆中时，执行跳跃以浮起。
 *
 * MC 1.16.5 参考: net.minecraft.entity.monster.SlimeEntity.FloatGoal
 *
 * 执行条件:
 * - 史莱姆在水中或岩浆中
 *
 * tick 行为:
 * - 80% 概率触发跳跃
 * - 设置移动速度为 1.2 倍
 *
 * 互斥标志: Jump, Move
 */
class SlimeFloatGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param slime 史莱姆实体
     */
    explicit SlimeFloatGoal(SlimeEntity* slime);

    ~SlimeFloatGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "SlimeFloatGoal"; }

private:
    SlimeEntity* m_slime;

    // MC 1.16.5 常量
    static constexpr f32 JUMP_CHANCE = 0.8f; // 跳跃概率
    static constexpr f64 SWIM_SPEED = 1.2;   // 游泳速度倍率
};

/**
 * @brief 史莱姆攻击目标
 *
 * 史莱姆面向并追逐攻击目标。
 *
 * MC 1.16.5 参考: net.minecraft.entity.monster.SlimeEntity.AttackGoal
 *
 * 执行条件:
 * - 有攻击目标
 * - 目标存活
 * - 目标不是创造模式玩家
 *
 * tick 行为:
 * - 面向攻击目标
 * - 设置移动方向，aggressive=true 时可以伤害玩家
 *
 * 持续时间: 300 tick (15秒)
 *
 * 互斥标志: Look
 */
class SlimeAttackGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param slime 史莱姆实体
     */
    explicit SlimeAttackGoal(SlimeEntity* slime);

    ~SlimeAttackGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "SlimeAttackGoal"; }

private:
    SlimeEntity* m_slime;
    LivingEntity* m_attackTarget = nullptr;
    i32 m_attackTimer = 0;

    // MC 1.16.5 常量
    static constexpr i32 ATTACK_DURATION = 300; // 攻击持续时间 (tick)
};

/**
 * @brief 史莱姆随机转向目标
 *
 * 史莱姆在没有攻击目标时随机选择方向。
 *
 * MC 1.16.5 参考: net.minecraft.entity.monster.SlimeEntity.FaceRandomGoal
 *
 * 执行条件:
 * - 没有攻击目标
 * - 在地面、水中、岩浆中或有效果
 *
 * tick 行为:
 * - 每 40-99 tick 随机选择一个面向角度 (0-359度)
 * - 设置移动方向，aggressive=false
 *
 * 互斥标志: Look
 */
class SlimeFaceRandomGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param slime 史莱姆实体
     */
    explicit SlimeFaceRandomGoal(SlimeEntity* slime);

    ~SlimeFaceRandomGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "SlimeFaceRandomGoal"; }

private:
    SlimeEntity* m_slime;
    f32 m_chosenDegrees = 0.0f;
    i32 m_nextRandomizeTime = 0;

    // MC 1.16.5 常量
    static constexpr i32 RANDOMIZE_TIME_MIN = 40;   // 最小随机时间
    static constexpr i32 RANDOMIZE_TIME_RANGE = 60; // 随机时间范围 (40-99)
};

/**
 * @brief 史莱姆跳跃目标
 *
 * 史莱姆持续跳跃移动。
 *
 * MC 1.16.5 参考: net.minecraft.entity.monster.SlimeEntity.HopGoal
 *
 * 执行条件:
 * - 史莱姆不是骑乘状态
 *
 * tick 行为:
 * - 设置移动速度为 1.0
 *
 * 互斥标志: Jump, Move
 */
class SlimeHopGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param slime 史莱姆实体
     */
    explicit SlimeHopGoal(SlimeEntity* slime);

    ~SlimeHopGoal() override = default;

    [[nodiscard]] bool shouldExecute() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "SlimeHopGoal"; }

private:
    SlimeEntity* m_slime;
};

} // namespace entity::ai::goal
} // namespace mc
