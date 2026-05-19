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

#include "../../../../core/Types.hpp"
#include "AbstractSkeletonEntity.hpp"

#include <memory>

namespace mc {

// 前向声明
class LivingEntity;

/**
 * @brief 凋灵骷髅实体
 *
 * 对齐 MC 1.16.5 `WitherSkeletonEntity` 的骷髅变种。
 *
 * 与普通骷髅的区别：
 * - 使用石剑进行近战攻击（普通骷髅使用弓远程攻击）
 * - 攻击目标会被施加凋零效果（持续10秒）
 * - 攻击伤害更高（4.0 vs 2.0）
 * - 免疫凋零效果
 * - 不会在阳光下燃烧
 * - 主动攻击猪灵
 *
 * MC 1.16.5 关键实现：
 * - 重写 setCombatTask() 使用近战攻击而非远程
 * - 重写 registerGoals() 添加攻击猪灵的目标
 * - 重写 attackEntityAsMob() 施加凋零效果
 */
class WitherSkeletonEntity : public AbstractSkeletonEntity {
public:
    WitherSkeletonEntity(EntityId id);

    ~WitherSkeletonEntity() override = default;

    WitherSkeletonEntity(const WitherSkeletonEntity&) = delete;
    WitherSkeletonEntity& operator=(const WitherSkeletonEntity&) = delete;
    WitherSkeletonEntity(WitherSkeletonEntity&&) = default;
    WitherSkeletonEntity& operator=(WitherSkeletonEntity&&) = default;

    static std::unique_ptr<Entity> create(IWorld* world);

    /// 凋零效果持续时间（ticks），MC 1.16.5: 200 ticks = 10 秒
    static constexpr i32 WITHER_DURATION_TICKS = 200;

    /**
     * @brief 凋灵骷髅不使用远程攻击
     *
     * 重写 IRangedAttackMob 接口方法，凋灵骷髅使用近战攻击。
     */
    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override
    {
        (void)target;
        (void)charge;
        // 凋灵骷髅不使用远程攻击
    }

    /**
     * @brief 近战攻击实体
     *
     * 重写 MobEntity::attackEntityAsMob() 以施加凋零效果。
     * 参考 MC 1.16.5 WitherSkeletonEntity.attackEntityAsMob()
     *
     * @param target 攻击目标
     * @return 是否攻击成功
     */
    bool attackEntityAsMob(LivingEntity& target) override;

    /**
     * @brief 设置战斗目标
     *
     * 重写父类方法，凋灵骷髅始终使用近战攻击。
     * MC 1.16.5: 凋灵骷髅装备石剑，setCombatTask() 会选择近战目标。
     */
    void setCombatTask() override;

    /**
     * @brief 检查是否免疫凋零效果
     *
     * 参考 MC 1.16.5 WitherSkeletonEntity.isPotionApplicable()
     * 凋灵骷髅免疫凋零效果。
     *
     * @param effect 效果实例
     * @return 如果免疫该效果返回 true
     */
    [[nodiscard]] bool isImmuneWitherEffect() const { return true; }

    [[nodiscard]] bool hasStoneSword() const { return m_hasStoneSword; }
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }
    [[nodiscard]] f32 eyeHeight() const override { return 2.1f; }

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    bool m_hasStoneSword = true;
};

} // namespace mc
