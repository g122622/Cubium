#pragma once

#include "../MonsterEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../interfaces/IRangedAttackMob.hpp"
#include <memory>

namespace mc {

// Forward declarations
class ItemStack;

/**
 * @brief 骷髅实体
 *
 * 使用弓箭进行远程攻击的亡灵怪物。
 *
 * 特性：
 * - 远程攻击：使用弓箭攻击
 * - 燃烧：在阳光下燃烧
 * - 战斗AI：会拉开距离进行射击
 * - 掉落：骨头、弓
 *
 * 参考 MC 1.16.5 SkeletonEntity
 */
class SkeletonEntity : public MonsterEntity, public entity::IRangedAttackMob {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    SkeletonEntity(LegacyEntityType type, EntityId id);
    ~SkeletonEntity() override = default;

    // 禁止拷贝
    SkeletonEntity(const SkeletonEntity&) = delete;
    SkeletonEntity& operator=(const SkeletonEntity&) = delete;

    // 允许移动
    SkeletonEntity(SkeletonEntity&&) = default;
    SkeletonEntity& operator=(SkeletonEntity&&) = default;

    /**
     * @brief 创建骷髅实体
     * @param world 世界实例
     * @return 新的骷髅实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== IRangedAttackMob 接口实现 ==========

    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;

    // ========== 攻击系统 ==========

    /**
     * @brief 是否正在拉弓
     */
    [[nodiscard]] bool isChargingBow() const { return m_chargingBow; }

    /**
     * @brief 设置拉弓状态
     */
    void setChargingBow(bool charging) { m_chargingBow = charging; }

    /**
     * @brief 获取攻击计时器
     */
    [[nodiscard]] i32 getAttackTimer() const { return m_attackTimer; }

    /**
     * @brief 设置攻击计时器
     */
    void setAttackTimer(i32 timer) { m_attackTimer = timer; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.74f; }

    /**
     * @brief 获取实体宽度
     */
    [[nodiscard]] f32 width() const override { return 0.6f; }

    /**
     * @brief 获取实体高度
     */
    [[nodiscard]] f32 height() const override { return 1.99f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // 攻击状态
    bool m_chargingBow = false;
    i32 m_attackTimer = 0;
    i32 m_attackCooldown = 0;

    // 常量
    static constexpr i32 ATTACK_COOLDOWN = 60; // 3秒攻击冷却
    static constexpr f32 ARROW_DAMAGE = 2.0f;  // 箭矢基础伤害
};

} // namespace mc
