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

#include "../../../interfaces/IRangedAttackMob.hpp"
#include "../MonsterEntity.hpp"

namespace mc {

/**
 * @brief 骷髅系怪物公共中间层
 *
 * 对齐 MC 1.16.5 `AbstractSkeletonEntity`，集中承载：
 * - 远程弓箭攻击接口
 * - 拉弓状态与攻击计时
 * - 骷髅系共通属性与基础目标注册
 */
class AbstractSkeletonEntity : public MonsterEntity, public entity::IRangedAttackMob {
public:
    ~AbstractSkeletonEntity() override = default;

    AbstractSkeletonEntity(const AbstractSkeletonEntity&) = delete;
    AbstractSkeletonEntity& operator=(const AbstractSkeletonEntity&) = delete;
    AbstractSkeletonEntity(AbstractSkeletonEntity&&) = default;
    AbstractSkeletonEntity& operator=(AbstractSkeletonEntity&&) = default;

    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;

    [[nodiscard]] bool isChargingBow() const { return m_chargingBow; }
    void setChargingBow(bool charging) { m_chargingBow = charging; }

    [[nodiscard]] i32 getAttackTimer() const { return m_attackTimer; }
    void setAttackTimer(i32 timer) { m_attackTimer = timer; }

    [[nodiscard]] i32 getAttackCooldown() const { return m_attackCooldown; }
    void setAttackCooldown(i32 cooldown) { m_attackCooldown = cooldown; }

    void tick() override;

protected:
    AbstractSkeletonEntity(LegacyEntityType type, EntityId id);

    void registerGoals() override;
    void registerAttributes() override;

    bool m_chargingBow = false;
    i32 m_attackTimer = 0;
    i32 m_attackCooldown = 0;

    static constexpr i32 ATTACK_COOLDOWN = 60;
    static constexpr f32 ARROW_DAMAGE = 2.0f;
};

} // namespace mc
