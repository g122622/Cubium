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
#include <string>

namespace mc {

// 前向声明
class EvokerEntity;
class LivingEntity;
class SheepEntity;

namespace entity::ai::goal {

/**
 * @brief 唤魔者施法目标基类
 *
 * 提供施法目标的基础框架，包括施法时间和冷却管理。
 */
class EvokerSpellGoal : public Goal {
public:
    /**
     * @brief 构造函数
     * @param evoker 唤魔者实体
     */
    explicit EvokerSpellGoal(EvokerEntity* evoker);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

protected:
    /**
     * @brief 获取施法准备时间
     *
     * 对齐原版 SpellcasterIllager.SpellcasterUseSpellGoal.getCastWarmupTime() 默认返回 20。
     * 唤魔者尖牙/召唤恼鬼施法 goal 不重写此方法，用默认 20 tick 蓄力；Wololo 重写为 40。
     */
    [[nodiscard]] virtual i32 getCastWarmupTime() const noexcept { return 20; }

    /**
     * @brief 获取施法持续时间
     */
    [[nodiscard]] virtual i32 getCastingTime() const noexcept = 0;

    /**
     * @brief 获取施法冷却时间
     */
    [[nodiscard]] virtual i32 getCastingInterval() const noexcept = 0;

    /**
     * @brief 执行施法
     */
    virtual void castSpell() = 0;

    /**
     * @brief 获取施法类型
     */
    [[nodiscard]] virtual i32 getSpellTypeId() const noexcept = 0;

    EvokerEntity* m_evoker;
    i32 m_spellWarmup = 0;
    i32 m_spellCooldown = 0;
};

/**
 * @brief 唤魔者尖牙攻击目标
 *
 * 唤魔者使用尖牙攻击敌人。
 * - 近距离（<3格）：两圈尖牙（内圈5个，外圈8个）
 * - 远距离：直线16个尖牙
 *
 * 施法参数：
 * - 准备时间：0 ticks
 * - 施法时间：40 ticks
 * - 冷却时间：100 ticks
 */
class EvokerAttackSpellGoal : public EvokerSpellGoal {
public:
    explicit EvokerAttackSpellGoal(EvokerEntity* evoker);

    [[nodiscard]] std::string getTypeName() const override { return "EvokerAttackSpellGoal"; }

protected:
    [[nodiscard]] i32 getCastingTime() const noexcept override { return 40; }
    [[nodiscard]] i32 getCastingInterval() const noexcept override { return 100; }

    void castSpell() override;
    [[nodiscard]] i32 getSpellTypeId() const noexcept override { return 2; } // SpellType::Fangs

private:
    LivingEntity* m_target = nullptr;
};

/**
 * @brief 唤魔者召唤恼鬼目标
 *
 * 唤魔者召唤3个恼鬼助战。
 * - 只有当周围恼鬼数量少于8个时才会召唤
 * - 恼鬼有30-120秒的有限生命
 *
 * 施法参数：
 * - 准备时间：0 ticks
 * - 施法时间：100 ticks
 * - 冷却时间：340 ticks
 */
class EvokerSummonSpellGoal : public EvokerSpellGoal {
public:
    explicit EvokerSummonSpellGoal(EvokerEntity* evoker);

    [[nodiscard]] bool shouldExecute() override;

    [[nodiscard]] std::string getTypeName() const override { return "EvokerSummonSpellGoal"; }

protected:
    [[nodiscard]] i32 getCastingTime() const noexcept override { return 100; }
    [[nodiscard]] i32 getCastingInterval() const noexcept override { return 340; }

    void castSpell() override;
    [[nodiscard]] i32 getSpellTypeId() const noexcept override { return 1; } // SpellType::SummonVex

private:
    /**
     * @brief 检查周围恼鬼数量
     * @return 周围恼鬼数量
     */
    [[nodiscard]] i32 _countNearbyVexes() const;
};

/**
 * @brief 唤魔者施法时的看向目标
 *
 * 施法期间让唤魔者看向目标。
 */
class EvokerCastingSpellGoal : public Goal {
public:
    explicit EvokerCastingSpellGoal(EvokerEntity* evoker);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "EvokerCastingSpellGoal"; }

private:
    EvokerEntity* m_evoker;
};

/**
 * @brief 唤魔者唔噜噜法术目标（Wololo）
 *
 * 将附近的蓝色羊变成红色羊。
 * - 只有在没有攻击目标时才会执行
 * - 搜索16格内的蓝色羊
 * - 施法后将其羊毛颜色变为红色
 *
 * 施法参数：
 * - 准备时间：40 ticks
 * - 施法时间：60 ticks
 * - 冷却时间：140 ticks
 */
class EvokerWololoSpellGoal : public Goal {
public:
    explicit EvokerWololoSpellGoal(EvokerEntity* evoker);

    [[nodiscard]] bool shouldExecute() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    [[nodiscard]] std::string getTypeName() const override { return "EvokerWololoSpellGoal"; }

private:
    /**
     * @brief 寻找附近的蓝色羊
     * @return 如果找到返回羊实体指针，否则返回 nullptr
     */
    [[nodiscard]] class SheepEntity* _findBlueSheep() const;

    EvokerEntity* m_evoker;
    SheepEntity* m_wololoTarget = nullptr;
    i32 m_spellWarmup = 0;
    i32 m_spellCooldown = 0;

    // 施法常量
    static constexpr i32 CAST_WARMUP_TIME = 40;  // 准备时间 40 ticks
    static constexpr i32 CASTING_TIME = 60;      // 施法时间 60 ticks
    static constexpr i32 CASTING_INTERVAL = 140; // 冷却时间 140 ticks
    static constexpr f32 SEARCH_RANGE = 16.0f;   // 搜索范围 16 格
};

} // namespace entity::ai::goal
} // namespace mc
