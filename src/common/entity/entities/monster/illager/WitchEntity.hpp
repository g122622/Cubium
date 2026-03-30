#pragma once

#include "../MonsterEntity.hpp"
#include "../../../../core/Types.hpp"
#include <memory>

namespace mc {

/**
 * @brief 女巫实体
 *
 * 使用药水的敌对生物。
 *
 * 特性：
 * - 药水攻击：向玩家投掷负面药水
 * - 治疗：受伤时会使用治疗药水
 * - 掉落：药水材料
 * - 生成：在沼泽小屋
 *
 * 参考 MC 1.16.5 WitchEntity
 */
class WitchEntity : public MonsterEntity {
public:
    /**
     * @brief 构造函数
     * @param type 实体类型
     * @param id 实体ID
     */
    WitchEntity(LegacyEntityType type, EntityId id);
    ~WitchEntity() override = default;

    // 禁止拷贝
    WitchEntity(const WitchEntity&) = delete;
    WitchEntity& operator=(const WitchEntity&) = delete;

    // 允许移动
    WitchEntity(WitchEntity&&) = default;
    WitchEntity& operator=(WitchEntity&&) = default;

    /**
     * @brief 创建女巫实体
     * @param world 世界实例
     * @return 新的女巫实体
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    // ========== 药水系统 ==========

    /**
     * @brief 是否正在喝药水
     */
    [[nodiscard]] bool isDrinking() const { return m_drinking; }

    /**
     * @brief 设置喝药水状态
     */
    void setDrinking(bool drinking) { m_drinking = drinking; }

    /**
     * @brief 获取喝药水计时器
     */
    [[nodiscard]] i32 getDrinkTimer() const { return m_drinkTimer; }

    /**
     * @brief 设置喝药水计时器
     */
    void setDrinkTimer(i32 timer) { m_drinkTimer = timer; }

    /**
     * @brief 检查是否需要治疗
     */
    [[nodiscard]] bool needsHealing() const;

    /**
     * @brief 尝试使用治疗药水
     */
    void tryDrinkHealingPotion();

    // ========== 攻击 ==========

    /**
     * @brief 获取攻击冷却
     */
    [[nodiscard]] i32 getAttackCooldown() const { return m_attackCooldown; }

    /**
     * @brief 重置攻击冷却
     */
    void resetAttackCooldown() { m_attackCooldown = ATTACK_COOLDOWN; }

    // ========== 阳光燃烧 ==========

    /**
     * @brief 女巫不在阳光下燃烧
     */
    [[nodiscard]] bool shouldBurnInDaylight() const override { return false; }

    // ========== 属性 ==========

    /**
     * @brief 获取眼睛高度
     */
    [[nodiscard]] f32 eyeHeight() const override { return 1.62f; }

    // ========== 生命周期 ==========

    void tick() override;

protected:
    // ========== AI 目标注册 ==========
    void registerGoals() override;

    // ========== 属性注册 ==========
    void registerAttributes() override;

private:
    // 药水状态
    bool m_drinking = false;
    i32 m_drinkTimer = 0;

    // 攻击冷却
    i32 m_attackCooldown = 0;

    // 常量
    static constexpr i32 ATTACK_COOLDOWN = 60;       // 3秒攻击冷却
    static constexpr i32 DRINK_DURATION = 32;        // 喝药水时间
    static constexpr f32 HEAL_THRESHOLD = 0.5f;      // 生命值低于50%时治疗
};

} // namespace mc
