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
#include "../../../../../core/Types.hpp"
#include "../../../../../core/EnumSet.hpp"
#include "../../../../entities/monster/illager/SpellcastingIllagerEntity.hpp"

namespace mc {

// Forward declarations
class IllusionerEntity;

namespace entity::ai::goal {

/**
 * @brief 幻术师法术目标基类
 *
 * 提供幻术师法术目标的基础框架，包括施法准备时间和冷却管理。
 * 参考 MC 1.16.5 SpellcastingIllagerEntity.UseSpellGoal
 */
class IllusionerSpellGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param illusioner 幻术师实体
     */
    explicit IllusionerSpellGoal(IllusionerEntity* illusioner);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

protected:
    /**
     * @brief 获取施法准备时间
     * MC 1.16.5: getCastWarmupTime()
     */
    [[nodiscard]] virtual i32 getCastWarmupTime() const = 0;

    /**
     * @brief 获取施法持续时间
     * MC 1.16.5: getCastingTime()
     */
    [[nodiscard]] virtual i32 getCastingTime() const = 0;

    /**
     * @brief 获取施法冷却时间
     * MC 1.16.5: getCastingInterval()
     */
    [[nodiscard]] virtual i32 getCastingInterval() const = 0;

    /**
     * @brief 执行施法
     * MC 1.16.5: castSpell()
     */
    virtual void castSpell() = 0;

    /**
     * @brief 获取施法类型
     */
    [[nodiscard]] virtual SpellcastingIllagerEntity::SpellType getSpellType() const = 0;

    IllusionerEntity* m_illusioner;
    i32 m_spellWarmup = 0;
    i32 m_spellCooldown = 0;
};

/**
 * @brief 幻术师失明法术目标
 *
 * 幻术师对目标施放失明效果。
 * - 只有当难度为困难时才会施放
 * - 不能对同一个目标重复施法
 * - 持续时间：400 ticks (20秒)
 *
 * 施法参数：
 * - 准备时间：20 ticks
 * - 施法时间：无
 * - 冷却时间：180 ticks (9秒)
 *
 * 参考 MC 1.16.5 IllusionerEntity.BlindnessSpellGoal
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

private:
    EntityId m_lastTargetId = 0; // 上一个失明目标的实体ID

    // MC 1.16.5 参数
    static constexpr i32 WARMUP_TIME = 20;        // 准备时间 20 ticks
    static constexpr i32 CASTING_TIME = 0;        // 无施法时间
    static constexpr i32 COOLDOWN = 180;          // 冷却时间 180 ticks (9秒)
    static constexpr i32 BLINDNESS_DURATION = 400; // 失明持续 400 ticks (20秒)
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
 * - 施法时间：无
 * - 冷却时间：340 ticks (17秒)
 *
 * 参考 MC 1.16.5 IllusionerEntity.MirrorSpellGoal
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

private:
    // MC 1.16.5 参数
    static constexpr i32 WARMUP_TIME = 20;          // 准备时间 20 ticks
    static constexpr i32 CASTING_TIME = 0;          // 无施法时间
    static constexpr i32 COOLDOWN = 340;            // 冷却时间 340 ticks (17秒)
    static constexpr i32 INVISIBILITY_DURATION = 1200; // 隐身持续 1200 ticks (60秒)
};

} // namespace entity::ai::goal
} // namespace mc
