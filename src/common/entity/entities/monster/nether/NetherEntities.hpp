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
 * THE SOFTWARE IS PROVIDED "IS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
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
#include "common/entity/core/Entity.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/monster/basic/SlimeEntity.hpp"
#include "common/entity/interfaces/IAngerable.hpp"
#include "common/entity/interfaces/ICrossbowUser.hpp"
#include "common/entity/interfaces/IFlinging.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <optional>

namespace mc {

// Forward declarations
class DamageSource;

/**
 * @brief 恶魂实体
 *
 * 下界的飞行敌对生物，发射火球。
 */
class GhastEntity : public MonsterEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    GhastEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~GhastEntity() override = default;

    // ========== 飞行特性 ==========

    [[nodiscard]] bool canFly() const { return m_canFly; }
    void setCanFly(bool canFly) { m_canFly = canFly; }

    // ========== 火球攻击 ==========

    [[nodiscard]] bool isCharging() const { return m_isCharging; }
    void setCharging(bool charging) { m_isCharging = charging; }

    [[nodiscard]] i32 getAttackCooldown() const { return m_attackCooldown; }
    void setAttackCooldown(i32 cooldown) { m_attackCooldown = cooldown; }

    /**
     * @brief 获取火球爆炸威力
     */
    [[nodiscard]] i32 getFireballStrength() const { return m_explosionPower; }

    /**
     * @brief 设置火球爆炸威力
     */
    void setFireballStrength(i32 power) { m_explosionPower = power; }

    void shootFireball();

    // ========== 生命周期 ==========

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    bool m_canFly = true;
    bool m_isCharging = false;
    i32 m_attackCooldown = 0;
    i32 m_chargeTime = 0;
    i32 m_explosionPower = 1;
};

/**
 * @brief 岩浆怪实体
 *
 * 下界的史莱姆变种，免疫火焰。
 * 继承自 SlimeEntity，复用史莱姆的 AI 目标和分裂机制。
 *
 * 与史莱姆的差异：
 * - 跳跃延迟是史莱姆的 4 倍（40-120 tick）
 * - 跳跃高度随尺寸增加（+0.1F * size）
 * - 拥有护甲属性（size * 3）
 * - 攻击伤害 +2
 * - 小型岩浆怪也能伤害玩家
 * - 挤压动画衰减更慢（0.9 vs 0.6）
 * - 发光效果（亮度始终为 1.0）
 * - 无摔落伤害
 * - 火焰粒子代替史莱姆粒子
 */
class MagmaCubeEntity : public SlimeEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    MagmaCubeEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~MagmaCubeEntity() override = default;

    // ========== 体型 ==========

    /**
     * @brief 设置尺寸
     * 重写以添加护甲属性设置，护甲 = size * 3
     */
    void setSlimeSize(i32 size, bool resetHealth = true) override;

    // ========== 火焰免疫 ==========

    [[nodiscard]] bool isImmuneToFire() const override { return true; }

    // ========== 攻击 ==========

    /**
     * @brief 是否可以对玩家造成伤害
     * 小型岩浆怪也能伤害玩家（与史莱姆不同）
     */
    [[nodiscard]] bool canDamagePlayer() const override;

    /**
     * @brief 获取攻击伤害
     * 攻击伤害 = 属性值 + 2.0F
     */
    [[nodiscard]] f32 getAttackDamage() const;

    // ========== 跳跃 ==========

    /**
     * @brief 获取跳跃延迟
     * 返回史莱姆跳跃延迟的 4 倍（40-120 tick）
     */
    [[nodiscard]] i32 getJumpDelay() const override;

    // ========== 音效 ==========

    [[nodiscard]] std::optional<ResourceLocation> getHurtSound(DamageSource& source) const override;
    [[nodiscard]] std::optional<ResourceLocation> getDeathSound() const override;
    [[nodiscard]] std::optional<ResourceLocation> getSquishSound() const override;
    [[nodiscard]] std::optional<ResourceLocation> getJumpSound() const override;

    // ========== 粒子 ==========

    /**
     * @brief 获取着地粒子类型
     * 岩浆怪使用火焰粒子
     */
    [[nodiscard]] particle::ParticleTypeId getSquishParticle() const override;

protected:
    void registerAttributes() override;

    /**
     * @brief 更新挤压量
     * 挤压动画衰减更慢（史莱姆: 0.6, 岩浆怪: 0.9）
     */
    void alterSquishAmount() override;
};

/**
 * @brief 猪灵基类
 *
 * 猪灵和猪灵蛮兵的共同基类。
 */
class AbstractPiglinEntity : public MonsterEntity {
public:
    AbstractPiglinEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~AbstractPiglinEntity() override = default;

    // ========== 猪灵状态 ==========

    [[nodiscard]] bool isImmuneToFire() const override { return m_immuneToFire; }
    void setImmuneToFire(bool immune) { m_immuneToFire = immune; }

    [[nodiscard]] bool isConverting() const { return m_converting; }
    void setConverting(bool converting) { m_converting = converting; }

    [[nodiscard]] i32 getTimeInOverworld() const { return m_timeInOverworld; }
    void setTimeInOverworld(i32 time) { m_timeInOverworld = time; }

protected:
    void registerGoals() override;

private:
    bool m_immuneToFire = true;
    bool m_converting = false;
    i32 m_timeInOverworld = 0;
};

/**
 * @brief 猪灵实体
 *
 * 下界的敌对/中立生物，可交易。
 */
class PiglinEntity : public AbstractPiglinEntity, public entity::ICrossbowUser, public entity::IAngerable {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    PiglinEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~PiglinEntity() override = default;

    // ========== 交易相关 ==========

    [[nodiscard]] bool isBaby() const { return m_isBaby; }
    [[nodiscard]] bool isChild() const override { return m_isBaby; }
    void setBaby(bool baby)
    {
        if (m_isBaby == baby) {
            return;
        }

        m_isBaby = baby;
        refreshDimensions();
    }

    // ========== IRangedAttackMob 接口 ==========

    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;
    [[nodiscard]] i32 getAttackInterval() const override { return 20; }
    [[nodiscard]] bool canRangedAttack() const override { return true; }

    // ========== ICrossbowUser 接口 ==========

    void setChargingCrossbow(bool charging) override { m_isChargingCrossbow = charging; }
    [[nodiscard]] bool isChargingCrossbow() const override { return m_isChargingCrossbow; }
    void onCrossbowLoadComplete(::mc::ItemStack& crossbow) override;
    void shootCrossbow(::mc::LivingEntity* target, ::mc::ItemStack& crossbow, f32 charge) override;
    [[nodiscard]] i32 getCrossbowChargeTime() const override { return 25; }

    // ========== IAngerable 接口 ==========

    void setAttackTarget(LivingEntity* target) override;
    [[nodiscard]] LivingEntity* getAttackTarget() const override;
    void setRevengeTarget(LivingEntity* target) override;
    [[nodiscard]] LivingEntity* getRevengeTarget() const override;
    [[nodiscard]] i32 getRevengeTimer() const override;
    [[nodiscard]] bool isAngry() const override;
    void setAngry(bool angry) override;
    [[nodiscard]] i32 getAngerTime() const override;
    void setAngerTime(i32 time) override;

    // ========== tick ==========

    void tick() override;

    // ========== 寻路权重 ==========

    /**
     * @brief 获取猪灵在指定位置的寻路权重
     *
     * 猪灵排斥物（PIGLIN_REPELLENTS）附近返回 -1.0f，
     * 其他位置返回父类默认值。
     */
    [[nodiscard]] f32 getPathWeight(f32 x, f32 y, f32 z) const override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    bool m_isBaby = false;
    bool m_isChargingCrossbow = false;

    // IAngerable 成员（m_attackTarget 使用 MobEntity::m_attackTarget，不重复声明）
    LivingEntity* m_revengeTarget = nullptr;
    i32 m_revengeTimer = 0;
    bool m_angry = false;
    i32 m_angerTime = 0;
};

/**
 * @brief 猪灵蛮兵实体
 *
 * 下界堡垒的强力敌对生物。
 */
class PiglinBruteEntity : public AbstractPiglinEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    PiglinBruteEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~PiglinBruteEntity() override = default;

protected:
    void registerGoals() override;
    void registerAttributes() override;
};

/**
 * @brief 僵尸猪灵实体
 *
 * 下界的中立生物，被攻击后会激怒所有附近的僵尸猪灵。
 */
class ZombifiedPiglinEntity : public MonsterEntity {
public:
    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    ZombifiedPiglinEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~ZombifiedPiglinEntity() override = default;

    [[nodiscard]] bool isImmuneToFire() const override { return m_immuneToFire; }
    void setImmuneToFire(bool immune) { m_immuneToFire = immune; }

    [[nodiscard]] bool isAngry() const { return m_angry; }
    void setAngry(bool angry) { m_angry = angry; }

    [[nodiscard]] i32 getAngerTime() const { return m_angerTime; }
    void setAngerTime(i32 time) { m_angerTime = time; }

    void tick() override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    bool m_immuneToFire = true;
    bool m_angry = false;
    i32 m_angerTime = 0;
};

/**
 * @brief 疣猪兽实体
 *
 * 下界的敌对生物（成年）或中立生物（幼年）。
 */
class HoglinEntity : public MonsterEntity, public entity::IFlinging {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    HoglinEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~HoglinEntity() override = default;

    [[nodiscard]] bool isImmuneToFire() const override { return m_immuneToFire; }
    void setImmuneToFire(bool immune) { m_immuneToFire = immune; }

    [[nodiscard]] bool isBaby() const { return m_isBaby; }
    [[nodiscard]] bool isChild() const override { return m_isBaby; }
    void setBaby(bool baby)
    {
        if (m_isBaby == baby) {
            return;
        }

        m_isBaby = baby;
        refreshDimensions();
    }

    [[nodiscard]] bool isHuntable() const { return !m_isBaby; }
    [[nodiscard]] i32 getFlingAnimationTicks() const override { return m_attackAnimationTicks; }

    void tick() override;
    bool attackLivingTarget(LivingEntity& target);

    // ========== 寻路权重 ==========

    [[nodiscard]] f32 getPathWeight(f32 x, f32 y, f32 z) const override;

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    bool m_immuneToFire = true;
    bool m_isBaby = false;
    i32 m_attackCooldown = 0;
    i32 m_attackAnimationTicks = 0;
};

/**
 * @brief 僵尸疣兽实体
 *
 * 疣猪兽在主世界的僵尸化变体。
 */
class ZoglinEntity : public MonsterEntity, public entity::IFlinging {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world, ecs::EntityRegistry& registry);

    ZoglinEntity(EntityInstanceId id, ecs::EntityRegistry& registry);
    ~ZoglinEntity() override = default;

    [[nodiscard]] bool isBaby() const { return m_isBaby; }
    [[nodiscard]] bool isChild() const override { return m_isBaby; }
    void setBaby(bool baby)
    {
        if (m_isBaby == baby) {
            return;
        }

        m_isBaby = baby;
        refreshDimensions();
    }
    [[nodiscard]] i32 getFlingAnimationTicks() const override { return m_attackAnimationTicks; }

    /**
     * @brief 判断僵尸疣兽是否可以攻击指定类型的实体
     *
     * 对应 MC 原版 Zoglin.isTargetable 的类型过滤部分。
     * 僵尸疣兽不攻击同类（ZOGLIN）和苦力怕（CREEPER）。
     * 此方法由 TargetGoal::isSuitableTarget() 自动调用，
     * 同时 registerGoals() 中的 TargetPredicate 也会进行同样的过滤。
     */
    [[nodiscard]] bool canAttackType(const entity::EntityType& type) const override;

    void tick() override;
    bool attackLivingTarget(LivingEntity& target);

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    bool m_isBaby = false;
    i32 m_attackAnimationTicks = 0;
};

} // namespace mc
