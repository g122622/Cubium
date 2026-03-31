#pragma once

#include "AbstractRaiderEntity.hpp"
#include "../../../interfaces/ICrossbowUser.hpp"
#include "../../../../core/Types.hpp"

// Forward declarations
namespace mc {
class ItemStack;
class LivingEntity;
}

namespace mc {

/**
 * @brief 掠夺者实体
 *
 * 使用弩进行远程攻击的灾厄村民。
 *
 * 特性：
 * - 装备弩
 * - 可以加入掠夺事件
 * - 可以成为掠夺队长
 *
 * 参考 MC 1.16.5 PillagerEntity
 */
class PillagerEntity : public AbstractRaiderEntity, public entity::ICrossbowUser {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    PillagerEntity(LegacyEntityType type, EntityId id);
    ~PillagerEntity() override = default;

    // 禁止拷贝
    PillagerEntity(const PillagerEntity&) = delete;
    PillagerEntity& operator=(const PillagerEntity&) = delete;

    // 允许移动
    PillagerEntity(PillagerEntity&&) = default;
    PillagerEntity& operator=(PillagerEntity&&) = default;

    // ========== IRangedAttackMob 接口 ==========

    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;
    [[nodiscard]] i32 getAttackInterval() const override { return 20; }
    [[nodiscard]] bool canRangedAttack() const override { return true; }

    // ========== ICrossbowUser 接口 ==========

    void setChargingCrossbow(bool charging) override { m_isCharging = charging; }
    [[nodiscard]] bool isChargingCrossbow() const override { return m_isCharging; }
    void onCrossbowLoadComplete(::mc::ItemStack& crossbow) override;
    void shootCrossbow(::mc::LivingEntity* target, ::mc::ItemStack& crossbow, f32 charge) override;
    [[nodiscard]] i32 getCrossbowChargeTime() const override { return 25; }

    // ========== 弩相关（旧接口，保持兼容） ==========

    /**
     * @brief 是否正在装填弩
     */
    [[nodiscard]] bool isCharging() const { return m_isCharging; }

    /**
     * @brief 设置装填状态
     */
    void setCharging(bool charging) { m_isCharging = charging; }

    // ========== 生命周期 ==========

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    bool m_isCharging = false;
};

/**
 * @brief 卫道士实体
 *
 * 手持铁斧进行近战攻击的灾厄村民。
 *
 * 特性：
 * - 高攻击伤害
 * - 可以加入掠夺事件
 * - 攻击村民和玩家
 *
 * 参考 MC 1.16.5 VindicatorEntity
 */
class VindicatorEntity : public AbstractRaiderEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    VindicatorEntity(LegacyEntityType type, EntityId id);
    ~VindicatorEntity() override = default;

    // 禁止拷贝
    VindicatorEntity(const VindicatorEntity&) = delete;
    VindicatorEntity& operator=(const VindicatorEntity&) = delete;

    // 允许移动
    VindicatorEntity(VindicatorEntity&&) = default;
    VindicatorEntity& operator=(VindicatorEntity&&) = default;

    // ========== 行为状态 ==========

    /**
     * @brief 是否处于攻击状态
     */
    [[nodiscard]] bool isAggressive() const { return m_aggressive; }

    /**
     * @brief 设置攻击状态
     */
    void setAggressive(bool aggressive) { m_aggressive = aggressive; }

    // ========== 生命周期 ==========

protected:
    void registerGoals() override;
    void registerAttributes() override;

private:
    bool m_aggressive = false;
};

} // namespace mc
