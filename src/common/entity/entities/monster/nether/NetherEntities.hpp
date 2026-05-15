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
#include "../../../interfaces/ICrossbowUser.hpp"
#include "../../../interfaces/IFlinging.hpp"
#include "../MonsterEntity.hpp"

namespace mc {

/**
 * @brief 恶魂实体
 *
 * 下界的飞行敌对生物，发射火球。
 *
 * 参考 MC 1.16.5 GhastEntity
 */
class GhastEntity : public MonsterEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    GhastEntity(LegacyEntityType type, EntityId id);
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
     * MC 1.16.5: getFireballStrength()
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
    i32 m_explosionPower = 1; // MC 1.16.5: explosionStrength
};

/**
 * @brief 岩浆怪实体
 *
 * 下界的史莱姆变种，免疫火焰。
 *
 * 参考 MC 1.16.5 MagmaCubeEntity
 */
class MagmaCubeEntity : public MonsterEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    MagmaCubeEntity(LegacyEntityType type, EntityId id);
    ~MagmaCubeEntity() override = default;

    // ========== 体型 ==========

    [[nodiscard]] i32 getSize() const { return m_size; }
    void setSize(i32 size);

    // ========== 火焰免疫 ==========

    [[nodiscard]] bool isImmuneToFire() const override { return m_immuneToFire; }
    void setImmuneToFire(bool immune) { m_immuneToFire = immune; }

    // ========== 生命周期 ==========

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    i32 m_size = 1;
    bool m_immuneToFire = true;
};

/**
 * @brief 猪灵基类
 *
 * 猪灵和猪灵蛮兵的共同基类。
 *
 * 参考 MC 1.16.5 AbstractPiglinEntity
 */
class AbstractPiglinEntity : public MonsterEntity {
public:
    AbstractPiglinEntity(LegacyEntityType type, EntityId id);
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
 *
 * 参考 MC 1.16.5 PiglinEntity
 */
class PiglinEntity : public AbstractPiglinEntity, public entity::ICrossbowUser {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    PiglinEntity(LegacyEntityType type, EntityId id);
    ~PiglinEntity() override = default;

    // ========== 交易相关 ==========

    [[nodiscard]] bool isBaby() const { return m_isBaby; }
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

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    bool m_isBaby = false;
    bool m_isChargingCrossbow = false;
};

/**
 * @brief 猪灵蛮兵实体
 *
 * 下界堡垒的强力敌对生物。
 *
 * 参考 MC 1.16.5 PiglinBruteEntity
 */
class PiglinBruteEntity : public AbstractPiglinEntity {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    PiglinBruteEntity(LegacyEntityType type, EntityId id);
    ~PiglinBruteEntity() override = default;

protected:
    void registerGoals() override;
    void registerAttributes() override;
};

/**
 * @brief 僵尸猪灵实体
 *
 * 下界的中立生物，被攻击后会激怒所有附近的僵尸猪灵。
 *
 * 参考 MC 1.16.5 ZombifiedPiglinEntity
 */
class ZombifiedPiglinEntity : public MonsterEntity {
public:
    ZombifiedPiglinEntity(LegacyEntityType type, EntityId id);
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
 *
 * 参考 MC 1.16.5 HoglinEntity
 */
class HoglinEntity : public MonsterEntity, public entity::IFlinging {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    HoglinEntity(LegacyEntityType type, EntityId id);
    ~HoglinEntity() override = default;

    [[nodiscard]] bool isImmuneToFire() const override { return m_immuneToFire; }
    void setImmuneToFire(bool immune) { m_immuneToFire = immune; }

    [[nodiscard]] bool isBaby() const { return m_isBaby; }
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
 *
 * 参考 MC 1.16.5 ZoglinEntity
 */
class ZoglinEntity : public MonsterEntity, public entity::IFlinging {
public:
    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    ZoglinEntity(LegacyEntityType type, EntityId id);
    ~ZoglinEntity() override = default;

    [[nodiscard]] bool isBaby() const { return m_isBaby; }
    void setBaby(bool baby)
    {
        if (m_isBaby == baby) {
            return;
        }

        m_isBaby = baby;
        refreshDimensions();
    }
    [[nodiscard]] i32 getFlingAnimationTicks() const override { return m_attackAnimationTicks; }

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
