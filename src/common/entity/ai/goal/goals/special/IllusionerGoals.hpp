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

#include "common/core/EnumSet.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/entities/monster/illager/SpellcastingIllagerEntity.hpp"
#include <string>

namespace mc {

// Forward declarations
class IllusionerEntity;

namespace entity::ai::goal {

/**
 * @brief 幻术师法术目标基类
 *
 * 提供幻术师法术目标的基础框架，包括施法准备时间、施法持续时间和冷却管理。
 *
 * 施法流程：
 * 1. shouldExecute() 检查前置条件（未在施法、冷却完成）
 * 2. startExecuting() 设置 warmup 和 spellCastingTicks，播放施法准备音效
 * 3. tick() 中 warmup 递减，warmup 归零时调用 castSpell() 并播放施法完成音效
 * 4. resetTask() 清除施法状态
 */
class IllusionerSpellGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param illusioner 幻术师实体
     *
     * flag 对齐原版 SpellcasterIllager.SpellcasterUseSpellGoal：原版施法 goal 不设置任何
     * Goal.Flag（默认空 EnumSet）。施法 goal 必须无 flag，否则会与 IllusionerCastingSpellGoal
     * (优先级1, 占 Move+Look) 互斥抢占——施法 goal startExecuting 调 setSpellTicks 设
     * isSpellcasting()=true，CastingSpellGoal 随即 shouldExecute=true 启动，若两者共享 flag
     * (如 Look)，CastingSpellGoal 启动会 reset 施法 goal（_startGoal 抢占共享 flag 的运行中 goal），
     * 施法 goal 的 warmup 计数被 resetTask 清掉，castSpell 永不执行。
     *   无 flag 后施法 goal 不被任何 goal 抢占，warmup 持续递减到 0 执行 castSpell 施放
     * 失明/镜像法术。施法期间的"停步 + 看向目标"由 IllusionerCastingSpellGoal(优先级1, Move+Look)
     * 接管，对齐原版 SpellcasterCastingSpellGoal(MOVE+LOOK) 语义。
     */
    explicit IllusionerSpellGoal(IllusionerEntity* illusioner);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

protected:
    /**
     * @brief 获取施法准备时间（warmup ticks）
     */
    [[nodiscard]] virtual i32 getCastWarmupTime() const = 0;

    /**
     * @brief 获取施法持续时间（spellCastingTicks）
     *
     * 这是施法动画持续的总 tick 数，在此期间实体处于施法姿态。
     */
    [[nodiscard]] virtual i32 getCastingTime() const = 0;

    /**
     * @brief 获取施法冷却时间
     */
    [[nodiscard]] virtual i32 getCastingInterval() const = 0;

    /**
     * @brief 执行施法
     */
    virtual void castSpell() = 0;

    /**
     * @brief 获取施法类型
     */
    [[nodiscard]] virtual SpellcastingIllagerEntity::SpellType getSpellType() const = 0;

    /**
     * @brief 获取施法准备音效的资源路径
     *
     * 在 startExecuting() 中播放，用于提示玩家幻术师正在准备施法。
     */
    [[nodiscard]] virtual const char* getSpellPrepareSoundId() const = 0;

    IllusionerEntity* m_illusioner;
    i32 m_spellWarmup = 0;
    i32 m_spellCooldown = 0;
};

/**
 * @brief 幻术师施法时看向目标并停步
 *
 * 对齐原版 SpellcasterIllager.SpellcasterCastingSpellGoal：占 Move+Look flag，
 * isSpellcasting() 期间启动，使幻术师施法时停步并持续看向攻击目标。
 * 原版 Illusioner.registerGoals 优先级1 显式注册（Illusioner.java:66 addGoal(1,
 * new SpellcasterCastingSpellGoal())），Cubium 此前注释误称"父类已注册"实未注册，
 * 导致施法期间幻术师不停步、不强制看向目标——本次补齐。
 *
 * 优先级1 高于 RangedBowAttackGoal(6)，施法期间 RangedBowAttackGoal 无法抢占 Move，
 * 幻术师施法期间不射箭、不被走位打断，对齐原版 SpellcasterCastingSpellGoal(MOVE+LOOK) 语义。
 */
class IllusionerCastingSpellGoal : public Goal {
public:
    explicit IllusionerCastingSpellGoal(IllusionerEntity* illusioner);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "IllusionerCastingSpellGoal"; }

private:
    IllusionerEntity* m_illusioner;
};

/**
 * @brief 幻术师失明法术目标
 *
 * 幻术师对目标施放失明效果。
 * - 难度 >= Normal 时可施放（原版 isHarderThan(Normal)）
 * - 不能对同一个目标重复施法
 * - 持续时间：400 ticks (20秒)
 *
 * 施法参数：
 * - 准备时间：20 ticks
 * - 施法持续时间：20 ticks
 * - 冷却时间：180 ticks (9秒)
 */
class IllusionerBlindnessSpellGoal : public IllusionerSpellGoal {
public:
    explicit IllusionerBlindnessSpellGoal(IllusionerEntity* illusioner);

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;

    [[nodiscard]] std::string getTypeName() const override { return "IllusionerBlindnessSpellGoal"; }

protected:
    [[nodiscard]] i32 getCastWarmupTime() const override;
    [[nodiscard]] i32 getCastingTime() const override;
    [[nodiscard]] i32 getCastingInterval() const override;
    void castSpell() override;
    [[nodiscard]] SpellcastingIllagerEntity::SpellType getSpellType() const override;
    [[nodiscard]] const char* getSpellPrepareSoundId() const override;

private:
    EntityInstanceId m_lastTargetId = 0; ///< 上一个失明目标的实体ID

    static constexpr i32 WARMUP_TIME = 20;         ///< 准备时间 20 ticks
    static constexpr i32 CASTING_TIME = 20;        ///< 施法持续时间 20 ticks
    static constexpr i32 COOLDOWN = 180;           ///< 冷却时间 180 ticks (9秒)
    static constexpr i32 BLINDNESS_DURATION = 400; ///< 失明持续 400 ticks (20秒)
};

/**
 * @brief 幻术师镜像法术目标（隐身）
 *
 * 幻术师使自己隐身，生成假象迷惑敌人。
 * - 只有当幻术师没有隐身效果时才会施放
 * - 持续时间：1200 ticks (60秒)
 *
 * 施法参数：
 * - 准备时间：20 ticks
 * - 施法持续时间：20 ticks
 * - 冷却时间：340 ticks (17秒)
 */
class IllusionerMirrorSpellGoal : public IllusionerSpellGoal {
public:
    explicit IllusionerMirrorSpellGoal(IllusionerEntity* illusioner);

    [[nodiscard]] bool shouldExecute() override;

    [[nodiscard]] std::string getTypeName() const override { return "IllusionerMirrorSpellGoal"; }

protected:
    [[nodiscard]] i32 getCastWarmupTime() const override;
    [[nodiscard]] i32 getCastingTime() const override;
    [[nodiscard]] i32 getCastingInterval() const override;
    void castSpell() override;
    [[nodiscard]] SpellcastingIllagerEntity::SpellType getSpellType() const override;
    [[nodiscard]] const char* getSpellPrepareSoundId() const override;

private:
    static constexpr i32 WARMUP_TIME = 20;             ///< 准备时间 20 ticks
    static constexpr i32 CASTING_TIME = 20;            ///< 施法持续时间 20 ticks
    static constexpr i32 COOLDOWN = 340;               ///< 冷却时间 340 ticks (17秒)
    static constexpr i32 INVISIBILITY_DURATION = 1200; ///< 隐身持续 1200 ticks (60秒)
};

} // namespace entity::ai::goal
} // namespace mc
