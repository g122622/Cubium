#pragma once

#include "../../core/MobEntity.hpp"
#include "../../interfaces/IRangedAttackMob.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace entity {

/**
 * @brief 凋灵Boss实体
 *
 * 地狱Boss，具有三个头和多种攻击模式。
 *
 * 参考 MC 1.16.5 WitherEntity
 */
class WitherEntity : public MobEntity, public IRangedAttackMob {
public:
    /**
     * @brief 凋灵阶段
     */
    enum class Phase : u8 {
        Invulnerable,   // 无敌阶段（生成中）
        Charging,       // 充能阶段（准备发射凋灵之首）
        Attacking       // 攻击阶段
    };

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    /**
     * @brief 构造函数
     */
    WitherEntity(LegacyEntityType type, EntityId id);

    ~WitherEntity() override = default;

    // ========== Entity 接口重写 ==========

    [[nodiscard]] f32 width() const override { return 0.9f; }
    [[nodiscard]] f32 height() const override { return 3.5f; }
    [[nodiscard]] f32 eyeHeight() const override { return 2.0f; }

    void tick() override;

    // ========== LivingEntity 接口重写 ==========

    /**
     * @brief 凋灵免疫火焰和溺水
     */
    [[nodiscard]] bool isInvulnerableTo(DamageType type) const override;

    // ========== IRangedAttackMob 接口实现 ==========

    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;
    [[nodiscard]] i32 getAttackInterval() const override { return 40; }
    [[nodiscard]] bool canRangedAttack() const override { return m_phase == Phase::Attacking; }

    // ========== Boss 功能 ==========

    /**
     * @brief 获取Boss名称
     */
    [[nodiscard]] String getBossName() const { return "Wither"; }

    /**
     * @brief 是否显示生命条
     */
    [[nodiscard]] bool shouldDisplayHealthBar() const { return true; }

    /**
     * @brief 获取生命条颜色
     */
    [[nodiscard]] u32 getHealthBarColor() const { return 0x1A1A1A; }  // 深灰色

    // ========== 凋灵特有 ==========

    /**
     * @brief 获取当前阶段
     */
    [[nodiscard]] Phase phase() const { return m_phase; }

    /**
     * @brief 是否在充能
     */
    [[nodiscard]] bool isCharging() const { return m_phase == Phase::Charging; }

    /**
     * @brief 是否无敌
     */
    [[nodiscard]] bool isInvulnerablePhase() const { return m_phase == Phase::Invulnerable; }

    /**
     * @brief 获取充能时间
     */
    [[nodiscard]] i32 chargeTime() const { return m_chargeTime; }

    /**
     * @brief 获取主头目标
     */
    [[nodiscard]] LivingEntity* getHeadTarget() const { return m_headTarget; }

    /**
     * @brief 设置主头目标
     */
    void setHeadTarget(LivingEntity* target) { m_headTarget = target; }

    /**
     * @brief 获取左侧头目标
     */
    [[nodiscard]] LivingEntity* getLeftHeadTarget() const { return m_leftHeadTarget; }

    /**
     * @brief 获取右侧头目标
     */
    [[nodiscard]] LivingEntity* getRightHeadTarget() const { return m_rightHeadTarget; }

    /**
     * @brief 发射凋灵之首
     * @param target 目标实体
     * @param isBlue 是否为蓝色凋灵之首（更强）
     */
    void shootWitherSkull(LivingEntity* target, bool isBlue = false);

    /**
     * @brief 爆炸（生成时）
     */
    void explodeOnSpawn();

protected:
    void registerGoals() override;
    void registerAttributes() override;

    /**
     * @brief 更新无敌阶段
     */
    void updateInvulnerablePhase();

    /**
     * @brief 更新充能阶段
     */
    void updateChargingPhase();

    /**
     * @brief 更新攻击阶段
     */
    void updateAttackingPhase();

    /**
     * @brief 更新头部目标
     */
    void updateHeadTargets();

private:
    Phase m_phase = Phase::Invulnerable;
    i32 m_chargeTime = 0;         // 充能时间
    i32 m_invulnerableTime = 0;   // 无敌时间

    // 三个头的目标
    LivingEntity* m_headTarget = nullptr;
    LivingEntity* m_leftHeadTarget = nullptr;
    LivingEntity* m_rightHeadTarget = nullptr;

    // 攻击冷却
    i32 m_mainHeadCooldown = 0;
    i32 m_leftHeadCooldown = 0;
    i32 m_rightHeadCooldown = 0;

    // 生成的凋灵之首数量（用于成就）
    i32 m_skullsSpawned = 0;
};

} // namespace entity
} // namespace mc
