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
#include "common/world/block/BlockPos.hpp"
#include <string>

namespace mc {

// 前向声明
class CreatureEntity;
class DamageTypeTag;

namespace entity::ai::goal {

/**
 * @brief 恐慌逃跑目标
 *
 * 当实体最近受到的伤害源属于 PANIC_CAUSES 标签（或子类指定的标签）时随机逃跑；
 * 若同时着火则优先逃向水源。对齐 vanilla PanicGoal：shouldPanic 判定
 * lastDamageSource.is(panicCausingDamageTypes)，而非"是否有攻击者实体"。
 */
class PanicGoal : public Goal {
public:
    /**
     * @brief 构造函数（使用默认 PANIC_CAUSES 标签，对齐 vanilla 第一构造）
     * @param creature 拥有此目标的生物
     * @param speed 逃跑速度倍率
     */
    PanicGoal(CreatureEntity* creature, f64 speed);

    /**
     * @brief 构造函数（指定恐慌伤害标签，对齐 vanilla 第二构造 PanicGoal(mob, speed, TagKey)）
     * @param creature 拥有此目标的生物
     * @param speed 逃跑速度倍率
     * @param panicCauses 触发恐慌的伤害类型标签（如 PANIC_CAUSES / PANIC_ENVIRONMENTAL_CAUSES）
     */
    PanicGoal(CreatureEntity* creature, f64 speed, DamageTypeTag& panicCauses);

    [[nodiscard]] bool shouldExecute() override;
    [[nodiscard]] bool shouldContinueExecuting() override;
    void startExecuting() override;
    void resetTask() override;
    void tick() override;

    /**
     * @brief 检查是否正在逃跑
     */
    [[nodiscard]] bool isRunning() const noexcept { return m_running; }

    [[nodiscard]] std::string getTypeName() const override { return "PanicGoal"; }

protected:
    /**
     * @brief 判定是否应触发恐慌（对齐 vanilla PanicGoal.shouldPanic:61-63）
     *
     * vanilla：mob.getLastDamageSource() != null && lastDamageSource.is(panicCausingDamageTypes)。
     * 即"最近伤害源非空且属于恐慌标签"。子类（如 PolarBearPanicGoal）可重写以按实体状态
     * （如 isChild）选择不同标签，对齐 vanilla PolarBear 用 Function<PathfinderMob, TagKey>
     * 动态选 PANIC_CAUSES（幼崽）/ PANIC_ENVIRONMENTAL_CAUSES（成年）。
     *
     * 注意：此判定用 lastDamageSource（最近伤害源 DamageSource）而非 getLastHurtBy（攻击者实体），
     * 故环境伤害（仙人掌/岩浆/闪电等无攻击者的 PANIC_ENVIRONMENTAL_CAUSES）也能触发恐慌，
     * 而 mob_attack_no_aggro（有攻击者但非 PANIC_CAUSES）不触发——对齐 vanilla 语义。
     *
     * @return 最近伤害源非空且属于恐慌标签时返回 true
     */
    [[nodiscard]] virtual bool shouldPanic() const;

    /**
     * @brief 获取最近的水源位置（着火时）
     *
     * 遍历立方体区域找最近的水方块。
     *
     * @param horizontalRange 水平搜索范围
     * @param verticalRange 垂直搜索范围
     * @return 水源方块位置，如果没有则返回 (0, 0, 0)
     */
    [[nodiscard]] BlockPos _getRandomWaterPosition(i32 horizontalRange, i32 verticalRange);

private:
    /**
     * @brief 寻找随机逃跑位置
     * @return 是否找到有效位置
     */
    [[nodiscard]] bool _findRandomPosition();

    CreatureEntity* m_creature;
    f64 m_speed;
    f32 m_targetX = 0.0f;
    f32 m_targetY = 0.0f;
    f32 m_targetZ = 0.0f;
    bool m_running = false;
    // 恐慌伤害标签（对齐 vanilla panicCausingDamageTypes）。nullptr 时 shouldPanic 用默认
    // DamageTypeTags::PANIC_CAUSES（对齐 vanilla 第一构造默认值）。
    DamageTypeTag* m_panicCauses = nullptr;
};

} // namespace entity::ai::goal
} // namespace mc
