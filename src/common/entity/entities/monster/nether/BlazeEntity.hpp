#pragma once

#include "../MonsterEntity.hpp"
#include "../../../core/Types.hpp"
#include "../../../interfaces/IRangedAttackMob.hpp"
#include <memory>

namespace mc {

/**
 * @brief 烈焰人实体
 *
 * 生活在下界的火焰怪物。
 *
 * 特性：
 * - 飞行：可以飞行
 * - 火球：发射火球
 * - 火焰免疫：免疫火焰伤害
 * - 弱水：接触水会受伤
 *
 * 参考 MC 1.16.5 BlazeEntity
 */
class BlazeEntity : public MonsterEntity, public entity::IRangedAttackMob {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    BlazeEntity(LegacyEntityType type, EntityId id);
    ~BlazeEntity() override = default;

    // 禁止拷贝
    BlazeEntity(const BlazeEntity&) = delete;
    BlazeEntity& operator=(const BlazeEntity&) = delete;

    // 允许移动
    BlazeEntity(BlazeEntity&&) = default;
    BlazeEntity& operator=(BlazeEntity&&) = default;

    /**
     * @brief 创建烈焰人实体
     * @param world 世界实例
     * @return 新的烈焰人实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== IRangedAttackMob 接口实现 ==========

    void attackEntityWithRangedAttack(LivingEntity* target, f32 charge) override;

    // ========== 火球攻击 ==========

    /**
     * @brief 是否正在发射火球
     */
    [[nodiscard]] bool isCharging() const { return m_charging; }

    /**
     * @brief 设置发射状态
     */
    void setCharging(bool charging) { m_charging = charging; }

    /**
     * @brief 获取火球数量
     */
    [[nodiscard]] i32 getFireballCount() const { return m_fireballCount; }

    /**
     * @brief 设置火球数量
     */
    void setFireballCount(i32 count) { m_fireballCount = count; }

    // ========== 飞行 ==========

    /**
     * @brief 是否正在飞行
     */
    [[nodiscard]] bool isFlying() const { return m_flying; }

    /**
     * @brief 设置飞行状态
     */
    void setFlying(bool flying) { m_flying = flying; }

    // ========== 阳光燃烧 ==========

    /**
     * @brief 烈焰人不在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.0f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // 攻击状态
    bool m_charging = false;
    i32 m_fireballCount = 0;
    i32 m_attackCooldown = 0;

    // 飞行状态
    bool m_flying = false;

    // 常量
    static constexpr i32 ATTACK_COOLDOWN = 100; // 5秒攻击冷却
    static constexpr i32 MAX_FIREBALLS = 3;     // 最多连发3个火球
    static constexpr f32 FIREBALL_DAMAGE = 5.0f;
};

} // namespace mc
