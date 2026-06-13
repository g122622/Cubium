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
 * copies of substantial portions of the Software.
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

#include "common/entity/entities/monster/MonsterEntity.hpp"
#include <memory>

namespace mc {

namespace entity {
class ProjectileEntity;
} // namespace entity

/**
 * @brief 旋风人实体
 *
 * MC 1.21 新增的敌对生物，在试炼密室中生成。
 * 能够投掷风弹攻击目标，具有独特的滑行和长跳移动能力。
 * 可以弹开除风弹外的投射物。
 *
 * 属性：
 * - 生命值：30
 * - 移动速度：0.6
 * - 跟随距离：24
 * - 攻击伤害：3
 * - 击退抗性：0.0
 *
 * AI行为：
 * - Shoot：向目标投掷风弹，范围4-24格
 * - LongJump：长跳移动，跳跃距离3-5格
 * - Slide：在地面上滑行移动
 * - ShootWhenStuck：卡住时紧急射击
 *
 * 掉落：
 * - 风弹 0-1（受抢夺影响，每级+1最大）
 *
 * 命名空间ID: minecraft:breeze
 */
class BreezeEntity final : public MonsterEntity {
public:
    /// 基础生命值
    static constexpr f32 MAX_HEALTH = 30.0f;

    /// 基础移动速度
    static constexpr f32 MOVEMENT_SPEED = 0.6f;

    /// 跟随距离
    static constexpr f32 FOLLOW_RANGE = 24.0f;

    /// 基础攻击伤害
    static constexpr f32 ATTACK_DAMAGE = 3.0f;

    /// 风弹射击最小距离
    static constexpr f32 SHOOT_MIN_RANGE = 4.0f;

    /// 风弹射击最大距离
    static constexpr f32 SHOOT_MAX_RANGE = 24.0f;

    /// 长跳最小距离
    static constexpr f32 LONG_JUMP_MIN_RANGE = 3.0f;

    /// 长跳最大距离
    static constexpr f32 LONG_JUMP_MAX_RANGE = 5.0f;

    /// 滑行速度范围
    static constexpr f32 SLIDE_SPEED_MIN = 0.3f;
    static constexpr f32 SLIDE_SPEED_MAX = 0.6f;

    /**
     * @brief 构造旋风人
     * @param id 实体ID
     */
    explicit BreezeEntity(EntityId id);

    ~BreezeEntity() override = default;

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 实体尺寸 ==========

    f32 width() const override { return 0.6f; }
    f32 height() const override { return 1.77f; }
    f32 eyeHeight() const override { return 1.52f; }

    // ========== 生命周期 ==========

    void tick() override;

    // ========== 掉落物 ==========

    // TODO(trial_chambers): 实现掉落风弹 (0-1，受抢夺影响)

    /**
     * @brief 检查是否可以攻击指定类型的实体
     *
     * MC 原版 Breeze.canAttackType() 仅允许攻击玩家和铁傀儡。
     * 旋风人采用白名单模式，只攻击这两种实体类型。
     */
    [[nodiscard]] bool canAttackType(entity::EntityTypeId typeId) const override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

    /**
     * @brief 弹射物偏转
     *
     * 旋风人弹开除风弹外的所有投射物。
     * TODO(trial_chambers): 实现弹射物偏转逻辑，需要在Entity/MobEntity基类中添加虚方法
     */
    // bool canProjectileDeflect() const override { return true; }

    /**
     * @brief 判断投射物是否可被偏转
     * @param projectile 投射物实体
     * @return 风弹返回false（不偏转），其他返回true
     */
    bool shouldDeflectProjectile(const entity::ProjectileEntity& projectile) const;

private:
    /**
     * @brief 投掷风弹
     *
     * 向当前攻击目标投掷一个风弹弹射物。
     */
    void shootWindCharge();

    /// 是否正在滑行
    bool m_sliding = false;

    /// 风弹射击冷却（ticks）
    i32 m_shootCooldown = 0;
};

} // namespace mc
